/*
 * Debug ability related code
 *
 * Copyright (C) 2021, Broadcom.
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
 *
 * <<Broadcom-WL-IPTag/Dual:>>
 */

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <bcmutils.h>
#include <bcmwifi_channels.h>
#include <bcmendian.h>
#include <ethernet.h>
#include <802.11.h>
#include <linux/if_arp.h>
#include <asm/uaccess.h>

#include <dngl_stats.h>
#include "wifi_stats.h"
#include <dhd.h>
#include <dhd_debug.h>
#include <dhdioctl.h>
#include <wlioctl.h>
#include <wlioctl_utils.h>

#include <ethernet.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/etherdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <linux/wait.h>
#include <net/cfg80211.h>
#include <net/rtnetlink.h>

#include <wlioctl.h>
#include <wldev_common.h>
#include <wl_cfg80211.h>

#include <wl_android.h>
#include <wl_cfgvendor.h>
#include <brcm_nl80211.h>

#ifdef DHD_PERIODIC_CNTRS
#include <dhd_linux.h>
#endif /* DHD_PERIODIC_CNTRS */

#ifdef TPUT_DEBUG_DUMP
enum tput_debug_mode_cmd {
	TPUT_DEBUG_DUMP_CMD_INVALID = 0,
	TPUT_DEBUG_DUMP_CMD_START = 1,
	TPUT_DEBUG_DUMP_CMD_LOGS = 2,
	TPUT_DEBUG_DUMP_CMD_STOP = 3
};

#define TPUT_DEBUG_MIN_BUFF_SIZE 256
#define TPUT_DEBUG_LOG_INTERVAL   1000
#define CNT_NAME(name) #name " %u "
#define	PRINT_CNT_VAL(field_name)						\
do {										\
	char cnt_str[TPUT_DEBUG_MIN_BUFF_SIZE] = {0, };				\
	snprintf(cnt_str, TPUT_DEBUG_MIN_BUFF_SIZE, CNT_NAME(field_name),	\
	dtoh32(wlc_cnt->field_name));						\
	strncat(log_buffer, cnt_str, strlen(cnt_str));				\
} while (0)

#define PRINT_MACCNT_VAL(version, field_name)                                           \
do {                                                                                    \
	char cnt_str[TPUT_DEBUG_MIN_BUFF_SIZE] = {0, };					\
	if (version == WL_CNT_XTLV_CNTV_LE10_UCODE) {					\
		snprintf(cnt_str, TPUT_DEBUG_MIN_BUFF_SIZE, CNT_NAME(field_name),	\
			dtoh32(((const wl_cnt_v_le10_mcst_t *) mac_cnt)->field_name));	\
	} else if (version == WL_CNT_XTLV_CNTV_LE10_UCODE) {				\
		snprintf(cnt_str, TPUT_DEBUG_MIN_BUFF_SIZE, CNT_NAME(field_name),	\
			dtoh32(((const wl_cnt_lt40mcst_v1_t *) mac_cnt)->field_name));	\
	} else if (version == WL_CNT_XTLV_GE40_UCODE_V1) {				\
		snprintf(cnt_str, TPUT_DEBUG_MIN_BUFF_SIZE, CNT_NAME(field_name),	\
			dtoh32(((const wl_cnt_ge40mcst_v1_t *) mac_cnt)->field_name));	\
	} else if (version == WL_CNT_XTLV_GE80_UCODE_V1) {				\
		snprintf(cnt_str, TPUT_DEBUG_MIN_BUFF_SIZE, CNT_NAME(field_name),	\
			dtoh32(((const wl_cnt_ge80mcst_v1_t *) mac_cnt)->field_name));	\
	}										\
	strncat(log_buffer, cnt_str, strlen(cnt_str));					\
} while (0)

int wl_cfgdbg_tput_dbg_get_ampdu_dump(struct net_device *ndev, char* log_buffer,
	char* cmd, char* sub_cmd)
{
	int32 err = BCME_OK;
	char* ioctl_buffer = NULL;
	struct bcm_cfg80211 *cfg = NULL;

	cfg = wl_get_cfg(ndev);
	if (!cfg) {
		WL_ERR(("Invalid cfg handle:%s\n", ndev->name));
		return BCME_ERROR;
	}

	ioctl_buffer = (char *)MALLOCZ(cfg->osh, WLC_IOCTL_MAXLEN);
	if (!ioctl_buffer) {
		WL_ERR(("ioctl_bufer: Malloc failed\n"));
		return BCME_ERROR;
	}

	err = wldev_iovar_getbuf(ndev, cmd,
	sub_cmd, (int)strlen(sub_cmd), ioctl_buffer, WLC_IOCTL_MAXLEN, NULL);
	if (err < 0) {
		WL_ERR(("AMPDU dump error:%d\n", err));
	} else {
		strncat(log_buffer, ioctl_buffer, strlen(ioctl_buffer));
	}

	MFREE(cfg->osh, ioctl_buffer, WLC_IOCTL_MAXLEN);

	return err;
}

int wl_cfgdbg_tput_dbg_clear_ampdu_dump(struct net_device *ndev,
	char* cmd, char* sub_cmd)
{
	int32 err = BCME_OK;
	char ioctl_buffer[WLC_IOCTL_SMLEN] = {0};

	err = wldev_iovar_setbuf(ndev, cmd,
	sub_cmd, (int)strlen(sub_cmd), ioctl_buffer, WLC_IOCTL_SMLEN, NULL);
	if (err < 0) {
		WL_ERR(("Dump AMPDU clear  error:%d\n", err));
	}

	return err;
}

int wl_cfgdbg_tput_dbg_clear_counters(struct net_device *ndev)
{
	int32 err = BCME_OK;
	int32 result = 0;

	err = wldev_iovar_getint(ndev, "reset_cnts", &result);
	if (err < 0) {
		WL_ERR(("counters clear error:%d\n", err));
	}
	return err;
}

int wl_cfgdbg_tput_dbg_get_counters(struct net_device *ndev, char* log_buffer)
{
	int err = BCME_ERROR;
	struct bcm_cfg80211 *cfg = NULL;
	const wl_cnt_wlc_t* wlc_cnt = NULL;
	const void *mac_cnt = NULL;
	wl_cnt_info_t *cntinfo = NULL;
	uint32 ver = 0;
	char* ioctl_buffer = NULL;

	cfg = wl_get_cfg(ndev);
	if (!cfg) {
		WL_ERR(("Invalid cfg handle:%s\n", ndev->name));
		return BCME_ERROR;
	}

	ioctl_buffer = (char *)MALLOCZ(cfg->osh, WLC_IOCTL_MAXLEN);
	if (!ioctl_buffer) {
		WL_ERR(("ioctl_bufer: Malloc failed\n"));
		return err;
	}

	err = wldev_iovar_getbuf(ndev, "counters", NULL, 0,
		ioctl_buffer, WLC_IOCTL_MAXLEN, NULL);
	if (err < 0) {
		WL_ERR(("counters error (%d) - size = %zu\n", err, sizeof(wl_cnt_wlc_t)));
		goto exit;
	}

	err  = wl_cntbuf_to_xtlv_format(NULL, ioctl_buffer, WL_CNTBUF_MAX_SIZE, 0);
	if (err != BCME_OK) {
		WL_ERR(("wl_cntbuf_to_xtlv_format ERR %d\n", err));
		goto exit;
	}

	wlc_cnt = (const wl_cnt_wlc_t*) bcm_get_data_from_xtlv_buf(
				((const wl_cnt_info_t *)ioctl_buffer)->data,
				((const wl_cnt_info_t *)ioctl_buffer)->datalen,
				WL_CNT_XTLV_WLC, NULL, BCM_XTLV_OPTION_ALIGN32);
	if (!wlc_cnt) {
		WL_ERR(("wlc_cnt NULL!\n"));
		err = BCME_ERROR;
		goto exit;
	}

	cntinfo = (wl_cnt_info_t *)ioctl_buffer;
	if ((mac_cnt = (const void *)bcm_get_data_from_xtlv_buf(cntinfo->data, cntinfo->datalen,
			WL_CNT_XTLV_CNTV_LE10_UCODE, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		ver = WL_CNT_XTLV_CNTV_LE10_UCODE;
	} else if ((mac_cnt = (const void *)bcm_get_data_from_xtlv_buf(cntinfo->data,
			cntinfo->datalen,
			WL_CNT_XTLV_LT40_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		ver = WL_CNT_XTLV_LT40_UCODE_V1;
	} else if ((mac_cnt = (const void *)bcm_get_data_from_xtlv_buf(cntinfo->data,
			cntinfo->datalen,
			WL_CNT_XTLV_GE40_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		ver = WL_CNT_XTLV_GE40_UCODE_V1;
	} else if ((mac_cnt = (const void *)bcm_get_data_from_xtlv_buf(cntinfo->data,
			cntinfo->datalen,
			WL_CNT_XTLV_GE80_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		ver = WL_CNT_XTLV_GE80_UCODE_V1;
	} else {
		WL_ERR(("Unknown macstat version\n"));
		err = BCME_ERROR;
		goto exit;
	}

	PRINT_CNT_VAL(txframe);
	PRINT_CNT_VAL(txbyte);
	PRINT_CNT_VAL(txretrans);
	PRINT_CNT_VAL(txerror);
	PRINT_CNT_VAL(txnobuf);

	PRINT_CNT_VAL(rxframe);
	PRINT_CNT_VAL(rxbyte);
	PRINT_CNT_VAL(rxerror);
	PRINT_CNT_VAL(rxnobuf);

	PRINT_MACCNT_VAL(ver, rxbadfcs);
	PRINT_MACCNT_VAL(ver, rxbadplcp);
	PRINT_MACCNT_VAL(ver, rxcrsglitch);

exit:
	if (ioctl_buffer) {
		MFREE(cfg->osh, ioctl_buffer, WLC_IOCTL_MAXLEN);
	}
	return err;
}

void wl_cfgdbg_tput_dbg_event(struct net_device *ndev, uint32 cmd,
	char* log_buffer, int log_buffer_len)
{
	struct wiphy *wiphy = NULL;
	gfp_t kflags;
	struct sk_buff *skb = NULL;
	int ret = BCME_OK;
	char debug_dump_time[DEBUG_DUMP_TIME_BUF_LEN] = {0};
	char debug_dump_path[255] = {0};
	int len = TPUT_DEBUG_MIN_BUFF_SIZE + log_buffer_len;

	WL_INFORM(("tput debug cmd:%d\n", cmd));

	if (!ndev) {
		WL_ERR(("ndev is NULL\n"));
		return;
	}

	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	wiphy = ndev->ieee80211_ptr->wiphy;
	/* Alloc the SKB for vendor_event */
	skb = CFG80211_VENDOR_EVENT_ALLOC(wiphy, ndev_to_wdev(ndev),
		len, BRCM_VENDOR_EVENT_TPUT_DUMP, kflags);

	if (!skb) {
		WL_ERR(("skb alloc failed"));
		return;
	}

	ret = nla_put_u32(skb, DUMP_TPUT_DEBUG_ATTR_CMD, cmd);
	if (ret) {
		WL_ERR(("Failed to dump tput debug state\n"));
		goto exit;
	}

	switch (cmd) {
		case TPUT_DEBUG_DUMP_CMD_START:
			get_debug_dump_time(debug_dump_time);
			snprintf(debug_dump_path, sizeof(debug_dump_path), "%stput_debug_%s_%s",
				DHD_COMMON_DUMP_PATH, ndev->name, debug_dump_time);

			ret = nla_put_string(skb, DUMP_TPUT_DEBUG_ATTR_FILE_NAME, debug_dump_path);
			if (ret) {
				WL_ERR(("Failed to put filename\n"));
			}
			break;
		case TPUT_DEBUG_DUMP_CMD_LOGS:
			ret = nla_put_string(skb, DUMP_TPUT_DEBUG_ATTR_BUF, log_buffer);
			if (ret) {
				WL_ERR(("Failed to put filename\n"));
			}
			break;
		default:
			break;
	}

	cfg80211_vendor_event(skb, kflags);
	return;

exit:
	if (skb) {
		dev_kfree_skb_any(skb);
	}

	return;
}

void wl_cfgdbg_tput_dbg_get_current_time(char* log_buffer)
{
	u64 ts_nsec;
	unsigned long rem_nsec;

	ts_nsec = local_clock();
	rem_nsec = DIV_AND_MOD_U64_BY_U32(ts_nsec, NSEC_PER_SEC);
	snprintf(log_buffer, TPUT_DEBUG_MIN_BUFF_SIZE, "\n#######[%5lu.%06lu]#######\n",
		(unsigned long)ts_nsec, rem_nsec / NSEC_PER_USEC);
}

#define DBG_STR_START			"==============================\n"
#define DBG_STR_CPUS			"CPU: \t\t"
#define DBG_STR_VALUS			"\n Val: \t\t"
#define DBG_STR_NAPI_PER_CPU_CNT	"\nnapi_percpu_run_cnt :"
#define DBG_STR_TXP_PER_CPU_CNT		"\ntxp_percpu_run_cnt :"
#define DBG_STR_TX_PER_CPU_CNT		"\ntx_start_percpu_run_cnt :"
#define DBG_STR_END			"\n==============================\n"

extern bool dhd_get_napi_sched_cnt(dhd_pub_t * dhdp,
	uint32 **napi_cnts, uint32 **txp_cnts, uint32 **tx_start_cnts);

int wl_cfgdbg_tput_dbg_host_log(dhd_pub_t * dhdp, char* log_buffer)
{
	int32 i, num_cpus = num_possible_cpus();
	char temp_buffer[255] = {0};
	uint32	*napi_sched_cnt  = NULL;
	uint32	*txp_sched_cnt	= NULL;
	uint32	*tx_start_sched_cnt = NULL;

	if (!dhdp) {
		WL_ERR(("Invaslid dhd_info\n"));
		return BCME_ERROR;
	}

	if (!dhd_get_napi_sched_cnt(dhdp, &napi_sched_cnt, &txp_sched_cnt, &tx_start_sched_cnt)) {
		return BCME_ERROR;
	}

	strncat(log_buffer, DBG_STR_START, strlen(DBG_STR_START));
	strncat(log_buffer, DBG_STR_CPUS, strlen(DBG_STR_CPUS));
	for (i = 0; i < num_cpus; i++) {
		snprintf(temp_buffer, sizeof(i), "%d\t", i);
		strncat(log_buffer, temp_buffer, strlen(temp_buffer));
	}

	if (napi_sched_cnt) {
		/* Print: tx_start_percpu_run_cnt */
		strncat(log_buffer, DBG_STR_NAPI_PER_CPU_CNT, strlen(DBG_STR_NAPI_PER_CPU_CNT));
		strncat(log_buffer, DBG_STR_VALUS, strlen(DBG_STR_VALUS));
		for (i = 0; i < num_cpus; i++) {
			memset(temp_buffer, 0, sizeof(temp_buffer));
			snprintf(temp_buffer, sizeof(temp_buffer), "%u\t", napi_sched_cnt[i]);
			strncat(log_buffer, temp_buffer, strlen(temp_buffer));
		}
	}

	if (txp_sched_cnt) {
		/* Print: txp_percpu_run_cnt */
		strncat(log_buffer, DBG_STR_TXP_PER_CPU_CNT, strlen(DBG_STR_TXP_PER_CPU_CNT));
		strncat(log_buffer, DBG_STR_VALUS, strlen(DBG_STR_VALUS));
		for (i = 0; i < num_cpus; i++) {
			memset(temp_buffer, 0, sizeof(temp_buffer));
			snprintf(temp_buffer, sizeof(temp_buffer), "%u\t", txp_sched_cnt[i]);
			strncat(log_buffer, temp_buffer, strlen(temp_buffer));
		}
	}

	if (tx_start_sched_cnt) {
		/* Print: tx_start_percpu_run_cnt */
		strncat(log_buffer, DBG_STR_TX_PER_CPU_CNT, strlen(DBG_STR_TX_PER_CPU_CNT));
		strncat(log_buffer, DBG_STR_VALUS, strlen(DBG_STR_VALUS));
		for (i = 0; i < num_cpus; i++) {
			memset(temp_buffer, 0, sizeof(temp_buffer));
			snprintf(temp_buffer, sizeof(temp_buffer), "%u\t", tx_start_sched_cnt[i]);
			strncat(log_buffer, temp_buffer, strlen(temp_buffer));
		}
	}
	strncat(log_buffer, DBG_STR_END, strlen(DBG_STR_END));

	return BCME_OK;
}

void wl_cfgdbg_tput_dbg_update(struct net_device *ndev)
{
	struct bcm_cfg80211 *cfg = NULL;
	char *log_buffer  = NULL;

	WL_INFORM(("Interface name:%s\n", ndev->name));

	if (!ndev) {
		WL_ERR(("Invalid ndev handle:%s\n", ndev->name));
		return;
	}

	cfg = wl_get_cfg(ndev);
	if (!cfg) {
		WL_ERR(("Invalid cfg handle:%s\n", ndev->name));
		return;
	}

	log_buffer = (char *)MALLOCZ(cfg->osh, WLC_IOCTL_MAXLEN);
	if (!log_buffer) {
		WL_ERR(("log_buffer: Malloc failed\n"));
		return;
	}
	/* Get current kernel timestamp */
	wl_cfgdbg_tput_dbg_get_current_time(log_buffer);
	/* Get Host scheduling count */
	wl_cfgdbg_tput_dbg_host_log((dhd_pub_t *)(cfg->pub), log_buffer);
	/* Get AMPDU dump */
	wl_cfgdbg_tput_dbg_get_ampdu_dump(ndev, log_buffer, cfg->tput_dbg_cmds[0].cmd,
		cfg->tput_dbg_cmds[0].sub_cmd);
	/* Get counters */
	wl_cfgdbg_tput_dbg_get_counters(ndev, log_buffer);

	wl_cfgdbg_tput_dbg_event(ndev, TPUT_DEBUG_DUMP_CMD_LOGS,
		log_buffer, strlen(log_buffer));

	if (log_buffer) {
		MFREE(cfg->osh, log_buffer, WLC_IOCTL_MAXLEN);
	}

	return;
}

void wl_cfgdbg_tput_debug_work(struct work_struct *work)
{
	dhd_pub_t *dhdp = NULL;
	struct net_device *ndev = NULL;
	struct bcm_cfg80211 *cfg = NULL;

	BCM_SET_CONTAINER_OF(cfg, work, struct bcm_cfg80211, tput_debug_work.work);

	WL_INFORM(("Scheduled TPUT debug dump..\n"));
	dhdp = (dhd_pub_t *)(cfg->pub);
	if (!dhdp) {
		WL_ERR(("Invaslid dhd_pub\n"));
		return;
	}
	ndev = dhd_idx2net(dhdp, cfg->tput_dbg_ifindex);
	if (!ndev) {
		WL_ERR(("Invalid ndev\n"));
		return;
	}

	if (!cfg->tput_dbg_mode_enable) {
		WL_ERR(("TPUT debug mode disabled!!\n"));
		return;
	}

	wl_cfgdbg_tput_dbg_update(ndev);

	schedule_delayed_work(&cfg->tput_debug_work,
		msecs_to_jiffies((const unsigned int)TPUT_DEBUG_LOG_INTERVAL));
}

int wl_cfgdbg_tput_debug_get_cmd(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int err = 0;
	int type, tmp;
	uint32 cmd_check = 0;
	const struct nlattr *iter;

	if (!cfg) {
		WL_ERR(("Invalid cfg handlen"));
		return 0;
	}

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
			case TPUT_DEBUG_ATTRIBUTE_CMD_STR:
				strncpy(cfg->tput_dbg_cmds[0].cmd,
					(char*)nla_data(iter), nla_len(iter));
				cmd_check += TPUT_DEBUG_ATTRIBUTE_CMD_STR;
				break;
			case TPUT_DEBUG_ATTRIBUTE_SUB_CMD_STR_AMPDU:
				strncpy(cfg->tput_dbg_cmds[0].sub_cmd,
					(char*)nla_data(iter), nla_len(iter));
				strncpy(cfg->tput_dbg_cmds[1].sub_cmd,
					(char*)nla_data(iter), nla_len(iter));
				cmd_check += TPUT_DEBUG_ATTRIBUTE_SUB_CMD_STR_AMPDU;
				break;
			case TPUT_DEBUG_ATTRIBUTE_SUB_CMD_STR_CLEAR:
				strncpy(cfg->tput_dbg_cmds[1].cmd,
					(char*)nla_data(iter), nla_len(iter));
				cmd_check += TPUT_DEBUG_ATTRIBUTE_SUB_CMD_STR_CLEAR;
				break;
		default:
			WL_ERR(("No such attribute:%d\n", type));
			break;
		}
	}

	if (cmd_check == TPUT_DEBUG_ATTRIBUTE_CMD_STR + TPUT_DEBUG_ATTRIBUTE_SUB_CMD_STR_AMPDU +
		TPUT_DEBUG_ATTRIBUTE_SUB_CMD_STR_CLEAR) {
		WL_INFORM(("TPUT_DBG: CMD configured..\n"));
		cfg->tput_dbg_cmd_config = TRUE;
	}
	return err;
}

void wl_cfgdbg_tput_debug_mode(struct net_device *ndev, bool enable)
{
	struct bcm_cfg80211 *cfg = NULL;
	cfg = wl_get_cfg(ndev);

	if (!cfg) {
		WL_ERR(("Invalid cfg handle:%s\n", ndev->name));
		return;
	}

	WL_INFORM(("interface name: %s, request debug mode:%s\n",
		ndev->name, enable ? "Enable" : "Disable"));

	if (enable) {
		dhd_pub_t *dhdp = NULL;

		dhdp = (dhd_pub_t *)(cfg->pub);
		cfg->tput_dbg_ifindex = (uint8)dhd_net2idx(dhdp->info, ndev);

		if (!cfg->tput_dbg_cmd_config) {
			WL_ERR(("TPUT debug cmd is not configured\n"));
			return;
		}

		if (cfg->tput_dbg_mode_enable) {
			WL_ERR(("TPUT debug mode already enabled\n"));
			return;
		}

		/* Clear AMPDU dump and counters */
		wl_cfgdbg_tput_dbg_clear_ampdu_dump(ndev,
			cfg->tput_dbg_cmds[1].cmd,
			cfg->tput_dbg_cmds[1].sub_cmd);
		wl_cfgdbg_tput_dbg_clear_counters(ndev);

		cfg->tput_dbg_mode_enable = TRUE;
		wl_cfgdbg_tput_dbg_event(ndev, TPUT_DEBUG_DUMP_CMD_START, NULL, 0);

		schedule_delayed_work(&cfg->tput_debug_work,
			msecs_to_jiffies((const unsigned int)TPUT_DEBUG_LOG_INTERVAL));

		WL_INFORM(("TPUT debug mode enabled!!\n"));
	} else {
		if (!cfg->tput_dbg_mode_enable) {
			WL_ERR(("TPUT debug mode already disabled\n"));
			return;
		}
		cfg->tput_dbg_mode_enable = FALSE;
		wl_cfgdbg_tput_dbg_event(ndev, TPUT_DEBUG_DUMP_CMD_STOP, NULL, 0);
		cancel_delayed_work_sync(&cfg->tput_debug_work);
		WL_INFORM(("TPUT debug mode disabled!!\n"));
	}
}
#endif /* TPUT_DEBUG_DUMP */

#ifdef DHD_PERIODIC_CNTRS
#define	DHD_EWP_CNT_VAL(src, name) compact_cntrs->name = dtoh32(src->name)
#define CNTRS_MOVE_NEXT_POS(pointer, size) pointer += size
#define CNTRS_ADD_BUF_LEN(current_length, size) current_length += size

typedef struct evt_front_header {
	uint16 length;
	uint16 block_count;
	uint16 set;
	uint16 pad;
} evt_front_header_t;

#ifdef PERIODIC_CNTRS_DBG_DUMP
void wl_cfgdbg_cntrs_dump(wl_periodic_compact_cntrs_v3_t *compact_cntrs)
{
	WL_INFORM(("txfail:%u txallfrm:%u txrtsfrm:%u txctsfrm:%u txback:%u\n",
		compact_cntrs->txfail, compact_cntrs->txallfrm, compact_cntrs->txrtsfrm,
		compact_cntrs->txctsfrm, compact_cntrs->txback));

	WL_INFORM(("txucast:%u txnoack:%u txframe:%u txretrans:%u txpspoll:%u\n",
		compact_cntrs->txucast, compact_cntrs->txnoack, compact_cntrs->txframe,
		compact_cntrs->txretrans, compact_cntrs->txpspoll));

	WL_INFORM(("rxrsptmout:%u txrtsfail:%u rxstrt:%u rxbadplcp:%u rxcrsglitch:%u\n",
		compact_cntrs->rxrsptmout, compact_cntrs->txrtsfail, compact_cntrs->rxstrt,
		compact_cntrs->rxbadplcp, compact_cntrs->rxcrsglitch));

	WL_INFORM(("rxnodelim:%u bphy_badplcp:%u bphy_badplcp:%u rxbadfcs:%u rxf0ovfl:%u\n",
		compact_cntrs->rxnodelim, compact_cntrs->bphy_badplcp, compact_cntrs->bphy_badplcp,
		compact_cntrs->rxbadfcs, compact_cntrs->rxf0ovfl));

	WL_INFORM(("rxf1ovfl:%u rxrtsucast:%u rxctsucast:%u rxackucast:%u rxback:%u\n",
		compact_cntrs->rxf1ovfl, compact_cntrs->rxrtsucast, compact_cntrs->rxctsucast,
		compact_cntrs->rxackucast, compact_cntrs->rxback));

	WL_INFORM(("rxbeaconmbss:%u rxdtucastmbss:%u rxbeaconobss:%u rxdtucastobss:%u\n",
		compact_cntrs->rxbeaconmbss, compact_cntrs->rxdtucastmbss,
		compact_cntrs->rxbeaconobss, compact_cntrs->rxdtucastobss));

	WL_INFORM(("rxrtsocast:%u rxctsocast:%u rxdtmcast:%u rxmpdu_mu:%u rxtoolate:%u\n",
		compact_cntrs->rxrtsocast, compact_cntrs->rxctsocast, compact_cntrs->rxdtmcast,
		compact_cntrs->rxmpdu_mu, compact_cntrs->rxtoolate));

	WL_INFORM(("rxframe:%u tx_toss_cnt:%u rx_toss_cnt:%u last_tx_toss_rsn:%u\n",
		compact_cntrs->rxframe, compact_cntrs->tx_toss_cnt, compact_cntrs->rx_toss_cnt,
		compact_cntrs->last_tx_toss_rsn));

	WL_INFORM(("last_rx_toss_rsn:%u txbcnfrm:%u rxretry:%u rxdtocast:%u\n",
		compact_cntrs->last_rx_toss_rsn, compact_cntrs->txbcnfrm, compact_cntrs->rxretry,
		compact_cntrs->rxdtocast));

	WL_INFORM(("slice_index:%d\n", compact_cntrs->pad));
}
#endif /* PERIODIC_CNTRS_DBG_DUMP */

bool
wl_cfgdbg_get_cntrs(struct net_device *ndev, char* ioctl_buffer,
	wl_periodic_compact_cntrs_v3_t *compact_cntrs)
{
	int err = BCME_ERROR;
	uint8 *slice_idx = NULL;
	const wl_cnt_wlc_t* wlc_cnt = NULL;
	wl_cnt_ge80mcst_v1_t* mac_cnt = NULL;

	err = wldev_iovar_getbuf(ndev, "counters", NULL, 0,
		ioctl_buffer, WLC_IOCTL_MAXLEN, NULL);

	if (err < 0) {
		WL_ERR(("Failed to get counters"));
		goto exit;
	}

	/* Convert counters buffer to xtlv format */
	err  = wl_cntbuf_to_xtlv_format(NULL, ioctl_buffer, WL_CNTBUF_MAX_SIZE, 0);
	if (err != BCME_OK) {
		WL_ERR(("wl_cntbuf_to_xtlv_format ERR %d\n", err));

	}

	/* Get wl_cnt from xtlv */
	wlc_cnt = (const wl_cnt_wlc_t*) bcm_get_data_from_xtlv_buf(
				((const wl_cnt_info_t *)ioctl_buffer)->data,
				((const wl_cnt_info_t *)ioctl_buffer)->datalen,
				WL_CNT_XTLV_WLC, NULL, BCM_XTLV_OPTION_ALIGN32);

	if (!wlc_cnt) {
		WL_ERR(("wlc_cnt is NULL\n"));
		err = BCME_ERROR;
		goto exit;
	}

	/* Get mac_cnt from xtlv */
	mac_cnt = (wl_cnt_ge80mcst_v1_t *)bcm_get_data_from_xtlv_buf(
			((const wl_cnt_info_t *)ioctl_buffer)->data,
			((const wl_cnt_info_t *)ioctl_buffer)->datalen,
			WL_CNT_XTLV_GE80_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32);

	if (!mac_cnt) {
		WL_ERR(("macstat is NULL\n"));
		err = BCME_ERROR;
		goto exit;
	}

	/* Get slice index from xtlv */
	slice_idx = (uint8 *)bcm_get_data_from_xtlv_buf(
			((const wl_cnt_info_t *)ioctl_buffer)->data,
			((const wl_cnt_info_t *)ioctl_buffer)->datalen,
			WL_CNT_XTLV_SLICE_IDX, NULL, BCM_XTLV_OPTION_ALIGN32);

	if (!slice_idx) {
		WL_ERR(("slice_index is NULL\n"));
		err = BCME_ERROR;
		goto exit;
	}
	compact_cntrs->pad = (uint16) *slice_idx;

	/* Not adding pm_dur, lqcm_report, chswitch_cnt, rxholes */
	compact_cntrs->version = WL_PERIODIC_COMPACT_CNTRS_VER_3;
	DHD_EWP_CNT_VAL(wlc_cnt, txfail);
	DHD_EWP_CNT_VAL(mac_cnt, txallfrm);
	DHD_EWP_CNT_VAL(mac_cnt, txrtsfrm);
	DHD_EWP_CNT_VAL(mac_cnt, txctsfrm);
	DHD_EWP_CNT_VAL(mac_cnt, txback);
	DHD_EWP_CNT_VAL(mac_cnt, txucast);
	DHD_EWP_CNT_VAL(wlc_cnt, txnoack);
	DHD_EWP_CNT_VAL(wlc_cnt, txframe);
	DHD_EWP_CNT_VAL(wlc_cnt, txretrans);
	DHD_EWP_CNT_VAL(wlc_cnt, txpspoll);
	DHD_EWP_CNT_VAL(mac_cnt, rxrsptmout);
	DHD_EWP_CNT_VAL(mac_cnt, txrtsfail);
	DHD_EWP_CNT_VAL(mac_cnt, rxstrt);
	DHD_EWP_CNT_VAL(mac_cnt, rxbadplcp);
	DHD_EWP_CNT_VAL(mac_cnt, rxcrsglitch);
	DHD_EWP_CNT_VAL(mac_cnt, rxnodelim);
	DHD_EWP_CNT_VAL(mac_cnt, bphy_badplcp);
	DHD_EWP_CNT_VAL(mac_cnt, bphy_rxcrsglitch);
	DHD_EWP_CNT_VAL(mac_cnt, rxbadfcs);
	DHD_EWP_CNT_VAL(mac_cnt, rxf0ovfl);
	DHD_EWP_CNT_VAL(mac_cnt, rxf1ovfl);
	DHD_EWP_CNT_VAL(mac_cnt, rxrtsucast);
	DHD_EWP_CNT_VAL(mac_cnt, rxctsucast);
	DHD_EWP_CNT_VAL(mac_cnt, rxackucast);
	DHD_EWP_CNT_VAL(mac_cnt, rxback);
	DHD_EWP_CNT_VAL(mac_cnt, rxbeaconmbss);
	DHD_EWP_CNT_VAL(mac_cnt, rxdtucastmbss);
	DHD_EWP_CNT_VAL(mac_cnt, rxbeaconobss);
	DHD_EWP_CNT_VAL(mac_cnt, rxdtucastobss);
	DHD_EWP_CNT_VAL(mac_cnt, rxdtocast);
	DHD_EWP_CNT_VAL(mac_cnt, rxrtsocast);
	DHD_EWP_CNT_VAL(mac_cnt, rxctsocast);
	DHD_EWP_CNT_VAL(mac_cnt, rxdtmcast);
	DHD_EWP_CNT_VAL(wlc_cnt, rxmpdu_mu);
	DHD_EWP_CNT_VAL(mac_cnt, rxtoolate);
	DHD_EWP_CNT_VAL(wlc_cnt, rxframe);
	DHD_EWP_CNT_VAL(wlc_cnt, tx_toss_cnt);
	DHD_EWP_CNT_VAL(wlc_cnt, rx_toss_cnt);
	DHD_EWP_CNT_VAL(wlc_cnt, last_tx_toss_rsn);
	DHD_EWP_CNT_VAL(wlc_cnt, last_rx_toss_rsn);
	DHD_EWP_CNT_VAL(mac_cnt, txbcnfrm);
	compact_cntrs->rxretry = wlc_cnt->rxrtry;

#ifdef PERIODIC_CNTRS_DBG_DUMP
	wl_cfgdbg_cntrs_dump(compact_cntrs);
#endif /* PERIODIC_CNTRS_DBG_DUMP */
	err = BCME_OK;
exit:
	return err;
}

uint32
wl_cfgdbg_current_timestamp(void)
{
	uint64 timestamp_u64 = 0;
	uint32 timestamp_u32 = 0;

	timestamp_u64 = local_clock();
	timestamp_u32 = (uint32) DIV_U64_BY_U32(timestamp_u64, NSEC_PER_MSEC);

	return timestamp_u32;
}

void
wl_cfgdbg_init_xtlv_buf(bcm_xtlvbuf_t* xtlvbuf, uint8* data, uint8 size)
{
	xtlvbuf->buf  = data;
	xtlvbuf->head = data;
	xtlvbuf->size = size;
}

int
wl_cfgdbg_fill_xtlv(xtlv_desc_t *xtlv_desc,
	struct bcm_xtlvbuf *xtlvbuf, uint16 type, uint16 *written_len)
{
	int err = BCME_ERROR;
	uint16 rlen = 0;
	struct bcm_xtlvbuf local_xtlvbuf;

	rlen = bcm_xtlv_buf_rlen(xtlvbuf);

	/* [Container: Type][data:xtlv_desc->type] */

	if (rlen <= BCM_XTLV_HDR_SIZE) {
		*written_len = 0;
		return BCME_BUFTOOSHORT;
	} else {
		*written_len = BCM_XTLV_HDR_SIZE;
	}

	bcm_xtlv_buf_init(&local_xtlvbuf, (uint8 *) (bcm_xtlv_buf(xtlvbuf) + BCM_XTLV_HDR_SIZE),
		(rlen - BCM_XTLV_HDR_SIZE), BCM_XTLV_OPTION_ALIGN32);

	/* Write data in the allocated buffer */
	err = bcm_xtlv_put_data(&local_xtlvbuf, xtlv_desc->type,
		(const uint8 *) xtlv_desc->ptr, xtlv_desc->len);

	if (err == BCME_OK) {
		*written_len += BCM_XTLV_HDR_SIZE + xtlv_desc->len;
		err = bcm_xtlv_put_data(xtlvbuf, type,
			NULL, bcm_xtlv_buf_len(&local_xtlvbuf));
	}

	return err;
}

int
wl_cfgdbg_cntrs_xtlv(wl_periodic_compact_cntrs_v3_t* compact_cntrs,
	uint8* ewp_data, uint16 size, uint16 *written_len)
{
	int err = BCME_ERROR;
	bcm_xtlvbuf_t xtlvbuf = {0};
	xtlv_desc_t xtlv_desc_ecnt;

	xtlv_desc_ecnt.type = WL_STATE_COMPACT_COUNTERS;
	xtlv_desc_ecnt.len  = sizeof(wl_periodic_compact_cntrs_v3_t);
	xtlv_desc_ecnt.ptr  = compact_cntrs;

	wl_cfgdbg_init_xtlv_buf(&xtlvbuf, ewp_data, size);
	err = wl_cfgdbg_fill_xtlv(&xtlv_desc_ecnt,
		&xtlvbuf, WL_SLICESTATS_XTLV_PERIODIC_STATE, written_len);
	if (err != BCME_OK) {
		WL_ERR(("Failed to pack counters err=%d\n", err));
	}

	return err;
}

void
wl_cfgdbg_periodic_cntrs(struct net_device *ndev, struct bcm_cfg80211 *cfg)
{
	int err = BCME_ERROR;
	dhd_pub_t *dhdp = (dhd_pub_t *)(cfg->pub);
	uint8* buffer = NULL;
	uint8* debug_msg = NULL;
	static uint32 seq_number = 1;
	wl_periodic_compact_cntrs_v3_t compact_cntrs = {0};
	evt_front_header_t *front_header = NULL;
	event_log_hdr_t* evt_log_hdr = NULL;
	uint16 xtlv_length = 0, total_length = 0, xtlv_buf_size = 0;
	uint32* kernel_timestamp = NULL;
	uint32 current_time = 0;

	if (dhdp->dongle_reset|| !dhdp->up) {
		return;
	}

	if (!wl_get_drv_status(cfg, CONNECTED, ndev)) {
		return;
	}

	if (FW_SUPPORTED(dhdp, ecounters)) {
		/* Not required DHD periodic conters */
		return;
	}

	current_time = wl_cfgdbg_current_timestamp();
	if ((current_time - dhdp->dhd_periodic_cntrs_last_time) < DHD_ECNT_INTERVAL) {
		return;
	}
	WL_INFORM(("In\n"));

	dhdp->dhd_periodic_cntrs_last_time = current_time;

	buffer = (char *)MALLOCZ(dhdp->osh, WLC_IOCTL_MAXLEN);
	if (!buffer) {
		WL_ERR(("Malloc failed\n"));
		return;
	}

	err = wl_cfgdbg_get_cntrs(ndev, buffer, &compact_cntrs);
	if (err != BCME_OK) {
		WL_ERR(("Failed to get counters, err=%d\n", err));
		goto exit;
	}

	/* clear buffer memory to set ecounter message */
	memset(buffer, 0x0, WLC_IOCTL_MAXLEN);

	/* [Front header][EWP payload][timestamp][event log header] */
	debug_msg = buffer;
	front_header = (evt_front_header_t *) debug_msg;
	front_header->block_count = 0;
	front_header->pad = 0;
	front_header->set = EVENT_LOG_SET_ECOUNTERS;
	/* front header length assign again after adding all field */
	front_header->length = 0;

	CNTRS_MOVE_NEXT_POS(debug_msg, sizeof(evt_front_header_t));
	CNTRS_ADD_BUF_LEN(total_length, sizeof(evt_front_header_t));

	/* Pack counters data */
	xtlv_buf_size = WLC_IOCTL_MAXLEN - total_length;
	err = wl_cfgdbg_cntrs_xtlv(&compact_cntrs, debug_msg, xtlv_buf_size, &xtlv_length);
	if (err != BCME_OK) {
		WL_ERR(("Failed to pack counters:%d\n", err));
		goto exit;
	}

	CNTRS_MOVE_NEXT_POS(debug_msg, xtlv_length);
	CNTRS_ADD_BUF_LEN(total_length, xtlv_length);

	/* Adding current timestamp */
	kernel_timestamp = (uint32*) debug_msg;
	*kernel_timestamp = wl_cfgdbg_current_timestamp();
	CNTRS_MOVE_NEXT_POS(debug_msg, sizeof(uint32));
	CNTRS_ADD_BUF_LEN(total_length, sizeof(uint32));

	/* Fill up event log header */
	evt_log_hdr = (event_log_hdr_t*) debug_msg;
	evt_log_hdr->count	= (total_length - sizeof(evt_front_header_t))/4;
	evt_log_hdr->tag	= EVENT_LOG_TAG_STATS;
	evt_log_hdr->fmt_num = 0xffff;

	CNTRS_ADD_BUF_LEN(total_length, sizeof(event_log_hdr_t));

	/* Update payload size in front header */
	front_header->length = total_length - sizeof(evt_front_header_t);

	/* Push Ecounters data */
	dhd_dbg_msgtrace_log_parser(dhdp, front_header, NULL, total_length, FALSE, seq_number++);

exit:
	MFREE(dhdp->osh, buffer, WLC_IOCTL_MAXLEN);
}
#endif /* DHD_PERIODIC_CNTRS */
