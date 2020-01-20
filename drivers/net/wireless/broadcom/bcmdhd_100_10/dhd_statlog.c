/*
 * DHD debugability: Status Information Logging support
 *
 * Copyright (C) 1999-2019, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: dhd_statlog.c 793017 2018-12-06 12:54:31Z $
 */

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <ethernet.h>
#include <bcmevent.h>
#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_dbg.h>

#ifdef DHD_STATUS_LOGGING

#define DHD_STATLOG_ERR_INTERNAL(fmt, ...)	DHD_ERROR(("STATLOG-" fmt, ##__VA_ARGS__))
#define DHD_STATLOG_INFO_INTERNAL(fmt, ...)	DHD_INFO(("STATLOG-" fmt, ##__VA_ARGS__))

#define DHD_STATLOG_PRINT(x)			DHD_ERROR(x)
#define DHD_STATLOG_ERR(x)			DHD_STATLOG_ERR_INTERNAL x
#define DHD_STATLOG_INFO(x)			DHD_STATLOG_INFO_INTERNAL x
#define DHD_STATLOG_VALID(stat)			(((stat) > (ST(INVALID))) && ((stat) < (ST(MAX))))

dhd_statlog_handle_t *
dhd_attach_statlog(dhd_pub_t *dhdp, uint32 num_items, uint32 logbuf_len)
{
	dhd_statlog_t *statlog = NULL;
	void *buf = NULL;

	if (!dhdp) {
		DHD_STATLOG_ERR(("%s: dhdp is NULL\n", __FUNCTION__));
		return NULL;
	}

	statlog = (dhd_statlog_t *)VMALLOCZ(dhdp->osh, sizeof(dhd_statlog_t));
	if (!statlog) {
		DHD_STATLOG_ERR(("%s: failed to allocate memory for dhd_statlog_t\n",
			__FUNCTION__));
		return NULL;
	}

	/* allocate log buffer */
	statlog->logbuf = (uint8 *)VMALLOCZ(dhdp->osh, logbuf_len);
	if (!statlog->logbuf) {
		DHD_STATLOG_ERR(("%s: failed to alloc log buffer\n", __FUNCTION__));
		goto error;
	}
	statlog->logbuf_len = logbuf_len;

	/* alloc ring buffer */
	statlog->bufsize = (uint32)(dhd_ring_get_hdr_size() +
		DHD_STATLOG_RING_SIZE(num_items));
	buf = VMALLOCZ(dhdp->osh, statlog->bufsize);
	if (!buf) {
		DHD_STATLOG_ERR(("%s: failed to allocate memory for ring buffer\n",
			__FUNCTION__));
		goto error;
	}

	statlog->ringbuf = dhd_ring_init(dhdp, buf, statlog->bufsize,
		DHD_STATLOG_ITEM_SIZE, num_items, DHD_RING_TYPE_SINGLE_IDX);
	if (!statlog->ringbuf) {
		DHD_STATLOG_ERR(("%s: failed to init ring buffer\n", __FUNCTION__));
		VMFREE(dhdp->osh, buf, statlog->bufsize);
		goto error;
	}

	statlog->items = num_items;

	return (dhd_statlog_handle_t *)statlog;

error:
	if (statlog->logbuf) {
		VMFREE(dhdp->osh, statlog->logbuf, logbuf_len);
	}

	if (statlog->ringbuf) {
		dhd_ring_deinit(dhdp, statlog->ringbuf);
		VMFREE(dhdp->osh, statlog->ringbuf, statlog->bufsize);
	}

	if (statlog) {
		VMFREE(dhdp->osh, statlog, sizeof(dhd_statlog_t));
	}

	return NULL;
}

void
dhd_detach_statlog(dhd_pub_t *dhdp)
{
	dhd_statlog_t *statlog;

	if (!dhdp) {
		DHD_STATLOG_ERR(("%s: dhdp is NULL\n", __FUNCTION__));
		return;
	}

	if (!dhdp->statlog) {
		DHD_STATLOG_ERR(("%s: statlog is NULL\n", __FUNCTION__));
		return;
	}

	statlog = (dhd_statlog_t *)(dhdp->statlog);

	if (statlog->ringbuf) {
		dhd_ring_deinit(dhdp, statlog->ringbuf);
		VMFREE(dhdp->osh, statlog->ringbuf, statlog->bufsize);
	}

	if (statlog->logbuf) {
		VMFREE(dhdp->osh, statlog->logbuf, statlog->logbuf_len);
	}

	VMFREE(dhdp->osh, statlog, sizeof(dhd_statlog_t));
	dhdp->statlog = NULL;
}

static int
dhd_statlog_ring_log(dhd_pub_t *dhdp, uint16 stat, uint8 ifidx, uint8 dir,
	uint16 status, uint16 reason)
{
	dhd_statlog_t *statlog;
	stat_elem_t *elem;

	if (!dhdp || !dhdp->statlog) {
		DHD_STATLOG_ERR(("%s: dhdp or dhdp->statlog is NULL\n",
			__FUNCTION__));
		return BCME_ERROR;
	}

	if (ifidx >= DHD_MAX_IFS) {
		DHD_STATLOG_ERR(("%s: invalid ifidx %d\n", __FUNCTION__, ifidx));
		return BCME_ERROR;
	}

	if (!DHD_STATLOG_VALID(stat)) {
		DHD_STATLOG_ERR(("%s: invalid stat %d\n", __FUNCTION__, stat));
		return BCME_ERROR;
	}

	statlog = (dhd_statlog_t *)(dhdp->statlog);
	elem = (stat_elem_t *)dhd_ring_get_empty(statlog->ringbuf);
	if (!elem) {
		/* no available slot */
		DHD_STATLOG_ERR(("%s: cannot allocate a new element\n",
			__FUNCTION__));
		return BCME_ERROR;
	}

	elem->ts = OSL_LOCALTIME_NS();
	elem->stat = stat;
	elem->ifidx = ifidx;
	elem->dir = dir;
	elem->reason = reason;
	elem->status = status;

	return BCME_OK;
}

int
dhd_statlog_ring_log_data(dhd_pub_t *dhdp, uint16 stat, uint8 ifidx, uint8 dir)
{
	return dhd_statlog_ring_log(dhdp, stat, ifidx,
		dir ? STDIR(TX) : STDIR(RX), 0, 0);
}

int
dhd_statlog_ring_log_data_reason(dhd_pub_t *dhdp, uint16 stat,
	uint8 ifidx, uint8 dir, uint16 reason)
{
	return dhd_statlog_ring_log(dhdp, stat, ifidx,
		dir ? STDIR(TX) : STDIR(RX), 0, reason);
}

int
dhd_statlog_ring_log_ctrl(dhd_pub_t *dhdp, uint16 stat, uint8 ifidx, uint16 reason)
{
	return dhd_statlog_ring_log(dhdp, stat, ifidx, ST(DIR_TX), 0, reason);
}

int
dhd_statlog_process_event(dhd_pub_t *dhdp, int type, uint8 ifidx,
	uint16 status, uint16 reason, uint16 flags)
{
	int stat = ST(INVALID);
	uint8 dir = STDIR(RX);

	if (!dhdp || !dhdp->statlog) {
		DHD_STATLOG_ERR(("%s: dhdp or dhdp->statlog is NULL\n",
			__FUNCTION__));
		return BCME_ERROR;
	}

	switch (type) {
	case WLC_E_SET_SSID:
		stat = ST(ASSOC_DONE);
		break;
	case WLC_E_AUTH:
		stat = ST(AUTH_DONE);
		dir = STDIR(TX);
		break;
	case WLC_E_AUTH_IND:
		stat = ST(AUTH_DONE);
		break;
	case WLC_E_DEAUTH:
		stat = ST(DEAUTH);
		dir = STDIR(TX);
		break;
	case WLC_E_DEAUTH_IND:
		stat = ST(DEAUTH);
		break;
	case WLC_E_DISASSOC:
		stat = ST(DISASSOC);
		dir = STDIR(TX);
		break;
	case WLC_E_LINK:
		if (!(flags & WLC_EVENT_MSG_LINK)) {
			stat = ST(LINKDOWN);
		}
		break;
	case WLC_E_ROAM_PREP:
		stat = ST(REASSOC_START);
		break;
	case WLC_E_ASSOC_REQ_IE:
		stat = ST(ASSOC_REQ);
		dir = STDIR(TX);
		break;
	case WLC_E_ASSOC_RESP_IE:
		stat = ST(ASSOC_RESP);
		break;
	case WLC_E_BSSID:
		stat = ST(REASSOC_DONE);
		break;
	case WLC_E_REASSOC:
		if (status == WLC_E_STATUS_SUCCESS) {
			stat = ST(REASSOC_SUCCESS);
		} else {
			stat = ST(REASSOC_FAILURE);
		}
		dir = STDIR(TX);
		break;
	case WLC_E_ASSOC_IND:
		stat = ST(ASSOC_REQ);
		break;
	default:
		break;
	}

	/* logging interested events */
	if (DHD_STATLOG_VALID(stat)) {
		dhd_statlog_ring_log(dhdp, stat, ifidx, dir, status, reason);
	}

	return BCME_OK;
}

uint32
dhd_statlog_get_logbuf_len(dhd_pub_t *dhdp)
{
	uint32 length = 0;
	dhd_statlog_t *statlog;

	if (dhdp && dhdp->statlog) {
		statlog = (dhd_statlog_t *)(dhdp->statlog);
		length = statlog->logbuf_len;
	}

	return length;
}

void *
dhd_statlog_get_logbuf(dhd_pub_t *dhdp)
{
	dhd_statlog_t *statlog;
	void *ret_addr = NULL;

	if (dhdp && dhdp->statlog) {
		statlog = (dhd_statlog_t *)(dhdp->statlog);
		ret_addr = (void *)(statlog->logbuf);
	}

	return ret_addr;
}

/*
 * called function uses buflen as 32 max.
 * So when adding a case, make sure the string is less than 32 bytes
 */
static void
dhd_statlog_stat_name(char *buf, uint32 buflen, uint32 state, uint8 dir)
{
	char *stat_str = NULL;
	bool tx = (dir == STDIR(TX));

	switch (state) {
	case ST(INVALID):
		stat_str = "INVALID_STATE";
		break;
	case ST(WLAN_POWER_ON):
		stat_str = "WLAN_POWER_ON";
		break;
	case ST(WLAN_POWER_OFF):
		stat_str = "WLAN_POWER_OFF";
		break;
	case ST(ASSOC_START):
		stat_str = "ASSOC_START";
		break;
	case ST(AUTH_DONE):
		stat_str = "AUTH_DONE";
		break;
	case ST(ASSOC_REQ):
		stat_str = tx ? "ASSOC_REQ" : "RX_ASSOC_REQ";
		break;
	case ST(ASSOC_RESP):
		stat_str = "ASSOC_RESP";
		break;
	case ST(ASSOC_DONE):
		stat_str = "ASSOC_DONE";
		break;
	case ST(DISASSOC_START):
		stat_str = "DISASSOC_START";
		break;
	case ST(DISASSOC_INT_START):
		stat_str = "DISASSOC_INTERNAL_START";
		break;
	case ST(DISASSOC_DONE):
		stat_str = "DISASSOC_DONE";
		break;
	case ST(DISASSOC):
		stat_str = tx ? "DISASSOC_EVENT" : "DISASSOC_IND_EVENT";
		break;
	case ST(DEAUTH):
		stat_str = tx ? "DEAUTH_EVENT" : "DEAUTH_IND_EVENT";
		break;
	case ST(LINKDOWN):
		stat_str = "LINKDOWN_EVENT";
		break;
	case ST(REASSOC_START):
		stat_str = "REASSOC_START";
		break;
	case ST(REASSOC_INFORM):
		stat_str = "REASSOC_INFORM";
		break;
	case ST(REASSOC_DONE):
		stat_str = "REASSOC_DONE";
		break;
	case ST(EAPOL_M1):
		stat_str = tx ? "TX_EAPOL_M1" : "RX_EAPOL_M1";
		break;
	case ST(EAPOL_M2):
		stat_str = tx ? "TX_EAPOL_M2" : "RX_EAPOL_M2";
		break;
	case ST(EAPOL_M3):
		stat_str = tx ? "TX_EAPOL_M3" : "RX_EAPOL_M3";
		break;
	case ST(EAPOL_M4):
		stat_str = tx ? "TX_EAPOL_M4" : "RX_EAPOL_M4";
		break;
	case ST(EAPOL_GROUPKEY_M1):
		stat_str = tx ? "TX_EAPOL_GROUPKEY_M1" : "RX_EAPOL_GROUPKEY_M1";
		break;
	case ST(EAPOL_GROUPKEY_M2):
		stat_str = tx ? "TX_EAPOL_GROUPKEY_M2" : "RX_EAPOL_GROUPKEY_M2";
		break;
	case ST(EAP_REQ_IDENTITY):
		stat_str = tx ? "TX_EAP_REQ_IDENTITY" : "RX_EAP_REQ_IDENTITY";
		break;
	case ST(EAP_RESP_IDENTITY):
		stat_str = tx ? "TX_EAP_RESP_IDENTITY" : "RX_EAP_RESP_IDENTITY";
		break;
	case ST(EAP_REQ_TLS):
		stat_str = tx ? "TX_EAP_REQ_TLS" : "RX_EAP_REQ_TLS";
		break;
	case ST(EAP_RESP_TLS):
		stat_str = tx ? "TX_EAP_RESP_TLS" : "RX_EAP_RESP_TLS";
		break;
	case ST(EAP_REQ_LEAP):
		stat_str = tx ? "TX_EAP_REQ_LEAP" : "RX_EAP_REQ_LEAP";
		break;
	case ST(EAP_RESP_LEAP):
		stat_str = tx ? "TX_EAP_RESP_LEAP" : "RX_EAP_RESP_LEAP";
		break;
	case ST(EAP_REQ_TTLS):
		stat_str = tx ? "TX_EAP_REQ_TTLS" : "RX_EAP_REQ_TTLS";
		break;
	case ST(EAP_RESP_TTLS):
		stat_str = tx ? "TX_EAP_RESP_TTLS" : "RX_EAP_RESP_TTLS";
		break;
	case ST(EAP_REQ_AKA):
		stat_str = tx ? "TX_EAP_REQ_AKA" : "RX_EAP_REQ_AKA";
		break;
	case ST(EAP_RESP_AKA):
		stat_str = tx ? "TX_EAP_RESP_AKA" : "RX_EAP_RESP_AKA";
		break;
	case ST(EAP_REQ_PEAP):
		stat_str = tx ? "TX_EAP_REQ_PEAP" : "RX_EAP_REQ_PEAP";
		break;
	case ST(EAP_RESP_PEAP):
		stat_str = tx ? "TX_EAP_RESP_PEAP" : "RX_EAP_RESP_PEAP";
		break;
	case ST(EAP_REQ_FAST):
		stat_str = tx ? "TX_EAP_REQ_FAST" : "RX_EAP_REQ_FAST";
		break;
	case ST(EAP_RESP_FAST):
		stat_str = tx ? "TX_EAP_RESP_FAST" : "RX_EAP_RESP_FAST";
		break;
	case ST(EAP_REQ_PSK):
		stat_str = tx ? "TX_EAP_REQ_PSK" : "RX_EAP_REQ_PSK";
		break;
	case ST(EAP_RESP_PSK):
		stat_str = tx ? "TX_EAP_RESP_PSK" : "RX_EAP_RESP_PSK";
		break;
	case ST(EAP_REQ_AKAP):
		stat_str = tx ? "TX_EAP_REQ_AKAP" : "RX_EAP_REQ_AKAP";
		break;
	case ST(EAP_RESP_AKAP):
		stat_str = tx ? "TX_EAP_RESP_AKAP" : "RX_EAP_RESP_AKAP";
		break;
	case ST(EAP_SUCCESS):
		stat_str = tx ? "TX_EAP_SUCCESS" : "RX_EAP_SUCCESS";
		break;
	case ST(EAP_FAILURE):
		stat_str = tx ? "TX_EAP_FAILURE" : "RX_EAP_FAILURE";
		break;
	case ST(EAPOL_START):
		stat_str = tx ? "TX_EAPOL_START" : "RX_EAPOL_START";
		break;
	case ST(WSC_START):
		stat_str = tx ? "TX_WSC_START" : "RX_WSC_START";
		break;
	case ST(WSC_DONE):
		stat_str = tx ? "TX_WSC_DONE" : "RX_WSC_DONE";
		break;
	case ST(WPS_M1):
		stat_str = tx ? "TX_WPS_M1" : "RX_WPS_M1";
		break;
	case ST(WPS_M2):
		stat_str = tx ? "TX_WPS_M2" : "RX_WPS_M2";
		break;
	case ST(WPS_M3):
		stat_str = tx ? "TX_WPS_M3" : "RX_WPS_M3";
		break;
	case ST(WPS_M4):
		stat_str = tx ? "TX_WPS_M4" : "RX_WPS_M4";
		break;
	case ST(WPS_M5):
		stat_str = tx ? "TX_WPS_M5" : "RX_WPS_M5";
		break;
	case ST(WPS_M6):
		stat_str = tx ? "TX_WPS_M6" : "RX_WPS_M6";
		break;
	case ST(WPS_M7):
		stat_str = tx ? "TX_WPS_M7" : "RX_WPS_M7";
		break;
	case ST(WPS_M8):
		stat_str = tx ? "TX_WPS_M8" : "RX_WPS_M8";
		break;
	case ST(8021X_OTHER):
		stat_str = tx ? "TX_OTHER_8021X" : "RX_OTHER_8021X";
		break;
	case ST(INSTALL_KEY):
		stat_str = "INSTALL_KEY";
		break;
	case ST(DELETE_KEY):
		stat_str = "DELETE_KEY";
		break;
	case ST(INSTALL_PMKSA):
		stat_str = "INSTALL_PMKSA";
		break;
	case ST(INSTALL_OKC_PMK):
		stat_str = "INSTALL_OKC_PMK";
		break;
	case ST(DHCP_DISCOVER):
		stat_str = tx ? "TX_DHCP_DISCOVER" : "RX_DHCP_DISCOVER";
		break;
	case ST(DHCP_OFFER):
		stat_str = tx ? "TX_DHCP_OFFER" : "RX_DHCP_OFFER";
		break;
	case ST(DHCP_REQUEST):
		stat_str = tx ? "TX_DHCP_REQUEST" : "RX_DHCP_REQUEST";
		break;
	case ST(DHCP_DECLINE):
		stat_str = tx ? "TX_DHCP_DECLINE" : "RX_DHCP_DECLINE";
		break;
	case ST(DHCP_ACK):
		stat_str = tx ? "TX_DHCP_ACK" : "RX_DHCP_ACK";
		break;
	case ST(DHCP_NAK):
		stat_str = tx ? "TX_DHCP_NAK" : "RX_DHCP_NAK";
		break;
	case ST(DHCP_RELEASE):
		stat_str = tx ? "TX_DHCP_RELEASE" : "RX_DHCP_RELEASE";
		break;
	case ST(DHCP_INFORM):
		stat_str = tx ? "TX_DHCP_INFORM" : "RX_DHCP_INFORM";
		break;
	case ST(ICMP_PING_REQ):
		stat_str = tx ? "TX_ICMP_PING_REQ" : "RX_ICMP_PING_REQ";
		break;
	case ST(ICMP_PING_RESP):
		stat_str = tx ? "TX_ICMP_PING_RESP" : "RX_ICMP_PING_RESP";
		break;
	case ST(ICMP_DEST_UNREACH):
		stat_str = tx ? "TX_ICMP_DEST_UNREACH" : "RX_ICMP_DEST_UNREACH";
		break;
	case ST(ICMP_OTHER):
		stat_str = tx ? "TX_ICMP_OTHER" : "RX_ICMP_OTHER";
		break;
	case ST(ARP_REQ):
		stat_str = tx ? "TX_ARP_REQ" : "RX_ARP_REQ";
		break;
	case ST(ARP_RESP):
		stat_str = tx ? "TX_ARP_RESP" : "RX_ARP_RESP";
		break;
	case ST(DNS_QUERY):
		stat_str = tx ? "TX_DNS_QUERY" : "RX_DNS_QUERY";
		break;
	case ST(DNS_RESP):
		stat_str = tx ? "TX_DNS_RESP" : "RX_DNS_RESP";
		break;
	case ST(REASSOC_SUCCESS):
		stat_str = "REASSOC_SUCCESS";
		break;
	case ST(REASSOC_FAILURE):
		stat_str = "REASSOC_FAILURE";
		break;
	default:
		stat_str = "UNKNOWN_STATUS";
		break;
	}

	strncpy(buf, stat_str, buflen);
	buf[buflen - 1] = '\0';
}

static void
dhd_statlog_get_timestamp(stat_elem_t *elem, uint64 *sec, uint64 *usec)
{
	uint64 ts_nsec, rem_nsec;

	ts_nsec = elem->ts;
	rem_nsec = DIV_AND_MOD_U64_BY_U32(ts_nsec, NSEC_PER_SEC);
	*sec = ts_nsec;
	*usec = (uint64)(rem_nsec / NSEC_PER_USEC);
}

#ifdef DHD_LOG_DUMP
static int
dhd_statlog_dump(dhd_statlog_t *statlog, char *buf, uint32 buflen)
{
	stat_elem_t *elem;
	struct bcmstrbuf b;
	struct bcmstrbuf *strbuf = &b;
	char stat_str[32];
	uint64 sec = 0, usec = 0;

	if (!statlog) {
		DHD_STATLOG_ERR(("%s: statlog is NULL\n", __FUNCTION__));
		return BCME_ERROR;
	}

	bcm_binit(strbuf, buf, buflen);
	bzero(stat_str, sizeof(*stat_str));
	dhd_ring_whole_lock(statlog->ringbuf);
	elem = (stat_elem_t *)dhd_ring_get_first(statlog->ringbuf);
	while (elem) {
		if (DHD_STATLOG_VALID(elem->stat)) {
			dhd_statlog_stat_name(stat_str, sizeof(stat_str),
				elem->stat, elem->dir);
			dhd_statlog_get_timestamp(elem, &sec, &usec);
			bcm_bprintf(strbuf, "[%5lu.%06lu] status=%s, ifidx=%d, "
				"reason=%d, status=%d\n", (unsigned long)sec,
				(unsigned long)usec, stat_str, elem->ifidx,
				elem->reason, elem->status);
		}
		elem = (stat_elem_t *)dhd_ring_get_next(statlog->ringbuf, (void *)elem);
	}
	dhd_ring_whole_unlock(statlog->ringbuf);

	return (!strbuf->size ? BCME_BUFTOOSHORT : strbuf->size);
}

int
dhd_statlog_write_logdump(dhd_pub_t *dhdp, const void *user_buf,
	void *fp, uint32 len, unsigned long *pos)
{
	dhd_statlog_t *statlog;
	log_dump_section_hdr_t sec_hdr;
	char *buf;
	uint32 buflen;
	int remain_len = 0;
	int ret = BCME_OK;

	if (!dhdp || !dhdp->statlog) {
		DHD_STATLOG_ERR(("%s: dhdp or dhdp->statlog is NULL\n",
			__FUNCTION__));
		return BCME_ERROR;
	}

	statlog = (dhd_statlog_t *)(dhdp->statlog);
	if (!statlog->logbuf) {
		DHD_STATLOG_ERR(("%s: logbuf is NULL\n", __FUNCTION__));
		return BCME_ERROR;
	}

	buf = statlog->logbuf;
	buflen = statlog->logbuf_len;
	bzero(buf, buflen);

	remain_len = dhd_statlog_dump(statlog, buf, buflen);
	if (remain_len < 0) {
		DHD_STATLOG_ERR(("%s: failed to write stat info to buffer\n",
			__FUNCTION__));
		return BCME_ERROR;
	}

	DHD_STATLOG_INFO(("%s: Start to write statlog\n", __FUNCTION__));

	/* write the section header first */
	ret = dhd_export_debug_data(STATUS_LOG_HDR, fp, user_buf,
		strlen(STATUS_LOG_HDR), pos);
	if (ret < 0) {
		goto exit;
	}

	dhd_init_sec_hdr(&sec_hdr);
	sec_hdr.type = LOG_DUMP_SECTION_STATUS;
	sec_hdr.length = buflen - remain_len;
	ret = dhd_export_debug_data((char *)&sec_hdr, fp, user_buf,
		sizeof(sec_hdr), pos);
	if (ret < 0) {
		goto exit;
	}

	/* write status log info */
	ret = dhd_export_debug_data(buf, fp, user_buf, buflen - remain_len, pos);
	if (ret < 0) {
		DHD_STATLOG_ERR(("%s: failed to write stat info, err=%d\n",
			__FUNCTION__, ret));
	}

	DHD_STATLOG_INFO(("%s: Complete to write statlog file, err=%d\n",
		__FUNCTION__, ret));

exit:
	return ret;
}
#endif /* DHD_LOG_DUMP */

int
dhd_statlog_get_latest_info(dhd_pub_t *dhdp, void *reqbuf)
{
	dhd_statlog_t *statlog;
	stat_query_t *query;
	stat_elem_t *elem;
	uint8 *req_buf, *resp_buf, *sp;
	uint32 req_buf_len, resp_buf_len, req_num;
	int i, remain_len, cpcnt = 0;

	if (!dhdp || !dhdp->statlog) {
		DHD_STATLOG_ERR(("%s: dhdp or statlog is NULL\n",
			__FUNCTION__));
		return BCME_ERROR;
	}

	query = (stat_query_t *)reqbuf;
	if (!query) {
		DHD_STATLOG_ERR(("%s: invalid query\n", __FUNCTION__));
		return BCME_ERROR;
	}

	req_buf = query->req_buf;
	req_buf_len = query->req_buf_len;
	resp_buf = query->resp_buf;
	resp_buf_len = query->resp_buf_len;
	req_num = query->req_num;
	if (!req_buf || !resp_buf) {
		DHD_STATLOG_ERR(("%s: invalid query\n", __FUNCTION__));
		return BCME_ERROR;
	}

	sp = resp_buf;
	remain_len = resp_buf_len;
	statlog = (dhd_statlog_t *)(dhdp->statlog);
	dhd_ring_whole_lock(statlog->ringbuf);
	elem = (stat_elem_t *)dhd_ring_get_last(statlog->ringbuf);
	while (elem) {
		for (i = 0; i < req_buf_len; i++) {
			/* found the status from the list of interests */
			if (elem->stat == req_buf[i]) {
				if (remain_len < sizeof(stat_elem_t)) {
					dhd_ring_whole_unlock(statlog->ringbuf);
					return BCME_BUFTOOSHORT;
				}
				memcpy(sp, (char *)elem, sizeof(stat_elem_t));
				sp += sizeof(stat_elem_t);
				remain_len -= sizeof(stat_elem_t);
				cpcnt++;
				break;
			}
		}

		if (cpcnt >= req_num) {
			break;
		}

		/* Proceed to next item */
		elem = (stat_elem_t *)dhd_ring_get_prev(statlog->ringbuf, (void *)elem);
	}
	dhd_ring_whole_unlock(statlog->ringbuf);

	return cpcnt;
}

int
dhd_statlog_query(dhd_pub_t *dhdp, char *cmd, int total_len)
{
	stat_elem_t *elem = NULL;
	stat_query_t query;
	char *pos, *token;
	uint8 *req_buf = NULL, *resp_buf = NULL;
	uint32 req_buf_len = 0, resp_buf_len = 0;
	ulong req_num, stat_num, stat;
	char stat_str[32];
	uint64 sec = 0, usec = 0;
	int i, resp_num, err = BCME_OK;

	/*
	 * DRIVER QUERY_STAT_LOG <total req num> <stat list num> <stat list>
	 */
	pos = cmd;
	/* drop command */
	token = bcmstrtok(&pos, " ", NULL);
	/* total number of request */
	token = bcmstrtok(&pos, " ", NULL);
	if (!token) {
		err = BCME_BADARG;
		goto exit;
	}

	req_num = bcm_strtoul(token, NULL, 0);

	/* total number of status list */
	token = bcmstrtok(&pos, " ", NULL);
	if (!token) {
		err = BCME_BADARG;
		goto exit;
	}

	stat_num = bcm_strtoul(token, NULL, 0);

	/* create a status list */
	req_buf_len = (uint32)(stat_num * sizeof(uint8));
	req_buf = (uint8 *)MALLOCZ(dhdp->osh, req_buf_len);
	if (!req_buf) {
		DHD_STATLOG_ERR(("%s: failed to allocate request buf\n",
			__FUNCTION__));
		err = BCME_NOMEM;
		goto exit;
	}

	/* parse the status list and update to the request buffer */
	for (i = 0; i < (uint32)stat_num; i++) {
		token = bcmstrtok(&pos, " ", NULL);
		if (!token) {
			err = BCME_BADARG;
			goto exit;
		}
		stat = bcm_strtoul(token, NULL, 0);
		req_buf[i] = (uint8)stat;
	}

	/* creat a response list */
	resp_buf_len = (uint32)DHD_STATLOG_RING_SIZE(req_num);
	resp_buf = (uint8 *)MALLOCZ(dhdp->osh, resp_buf_len);
	if (!resp_buf) {
		DHD_STATLOG_ERR(("%s: failed to allocate response buf\n",
			__FUNCTION__));
		err = BCME_NOMEM;
		goto exit;
	}

	/* create query format and query the status */
	query.req_buf = req_buf;
	query.req_buf_len = req_buf_len;
	query.resp_buf = resp_buf;
	query.resp_buf_len = resp_buf_len;
	query.req_num = (uint32)req_num;
	resp_num = dhd_statlog_get_latest_info(dhdp, (void *)&query);
	if (resp_num < 0) {
		DHD_STATLOG_ERR(("%s: failed to query the status\n", __FUNCTION__));
		err = BCME_ERROR;
		goto exit;
	}

	/* print out the results */
	DHD_STATLOG_PRINT(("=============== QUERY RESULT ===============\n"));
	if (resp_num > 0) {
		elem = (stat_elem_t *)resp_buf;
		for (i = 0; i < resp_num; i++) {
			if (DHD_STATLOG_VALID(elem->stat)) {
				dhd_statlog_stat_name(stat_str, sizeof(stat_str),
					elem->stat, elem->dir);
				dhd_statlog_get_timestamp(elem, &sec, &usec);
				DHD_STATLOG_PRINT(("[%5lu.%06lu] status=%s, ifidx=%d, "
					"reason=%d, status=%d\n", (unsigned long)sec,
					(unsigned long)usec, stat_str, elem->ifidx,
					elem->reason, elem->status));
			}
			elem++;
		}
	} else {
		DHD_STATLOG_PRINT(("No data found\n"));
	}

exit:
	if (resp_buf) {
		MFREE(dhdp->osh, resp_buf, resp_buf_len);
	}

	if (req_buf) {
		MFREE(dhdp->osh, req_buf, req_buf_len);
	}

	return err;
}

void
dhd_statlog_dump_scr(dhd_pub_t *dhdp)
{
	dhd_statlog_t *statlog;
	stat_elem_t *elem;
	char stat_str[32];
	uint64 sec = 0, usec = 0;

	if (!dhdp || !dhdp->statlog) {
		DHD_STATLOG_ERR(("%s: dhdp or statlog is NULL\n", __FUNCTION__));
		return;
	}

	statlog = (dhd_statlog_t *)(dhdp->statlog);
	bzero(stat_str, sizeof(*stat_str));

	DHD_STATLOG_PRINT(("=============== START OF CURRENT STATUS INFO ===============\n"));
	dhd_ring_whole_lock(statlog->ringbuf);
	elem = (stat_elem_t *)dhd_ring_get_first(statlog->ringbuf);
	while (elem) {
		if (DHD_STATLOG_VALID(elem->stat)) {
			dhd_statlog_stat_name(stat_str, sizeof(stat_str),
				elem->stat, elem->dir);
			dhd_statlog_get_timestamp(elem, &sec, &usec);
			DHD_STATLOG_PRINT(("[%5lu.%06lu] status=%s, ifidx=%d, "
				"reason=%d, status=%d\n", (unsigned long)sec,
				(unsigned long)usec, stat_str, elem->ifidx,
				elem->reason, elem->status));
		}
		elem = (stat_elem_t *)dhd_ring_get_next(statlog->ringbuf, (void *)elem);
	}
	dhd_ring_whole_unlock(statlog->ringbuf);
	DHD_STATLOG_PRINT(("=============== END OF CURRENT STATUS INFO ===============\n"));
}
#endif /* DHD_STATUS_LOGGING */
