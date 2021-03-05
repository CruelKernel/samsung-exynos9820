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

#ifdef TPUT_DEBUG_DUMP
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
