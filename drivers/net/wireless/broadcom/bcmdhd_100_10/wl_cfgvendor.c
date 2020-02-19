/*
 * Linux cfg80211 Vendor Extension Code
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
 * $Id: wl_cfgvendor.c 799472 2019-01-16 03:22:07Z $
 */

/*
 * New vendor interface additon to nl80211/cfg80211 to allow vendors
 * to implement proprietary features over the cfg80211 stack.
*/

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>

#include <bcmutils.h>
#include <bcmstdlib_s.h>
#include <bcmwifi_channels.h>
#include <bcmendian.h>
#include <ethernet.h>
#include <802.11.h>
#include <linux/if_arp.h>
#include <asm/uaccess.h>

#include <dngl_stats.h>
#include <dhd.h>
#include <dhd_debug.h>
#include <dhdioctl.h>
#include <wlioctl.h>
#include <wlioctl_utils.h>
#include <dhd_cfg80211.h>
#ifdef DHD_PKT_LOGGING
#include <dhd_pktlog.h>
#endif /* DHD_PKT_LOGGING */
#ifdef PNO_SUPPORT
#include <dhd_pno.h>
#endif /* PNO_SUPPORT */

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
#include <wl_cfgp2p.h>
#ifdef WL_NAN
#include <wl_cfgnan.h>
#endif /* WL_NAN */
#include <wl_android.h>
#include <wl_cfgvendor.h>
#ifdef PROP_TXSTATUS
#include <dhd_wlfc.h>
#endif // endif
#include <brcm_nl80211.h>

char*
wl_get_kernel_timestamp(void)
{
	static char buf[16];
	u64 ts_nsec;
	unsigned long rem_nsec;

	ts_nsec = local_clock();
	rem_nsec = DIV_AND_MOD_U64_BY_U32(ts_nsec, NSEC_PER_SEC);
	snprintf(buf, sizeof(buf), "%5lu.%06lu",
		(unsigned long)ts_nsec, rem_nsec / NSEC_PER_USEC);

	return buf;
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 13, 0)) || defined(WL_VENDOR_EXT_SUPPORT)
#if defined(WL_SUPP_EVENT)
int
wl_cfgvendor_send_supp_eventstring(const char *func_name, const char *fmt, ...)
{
	char buf[SUPP_LOG_LEN] = {0};
	struct bcm_cfg80211 *cfg;
	struct wiphy *wiphy;
	va_list args;
	int len;
	int prefix_len;
	int rem_len;

	cfg = wl_cfg80211_get_bcmcfg();
	if (!cfg || !cfg->wdev) {
		WL_DBG(("supp evt invalid arg\n"));
		return BCME_OK;
	}

	wiphy = cfg->wdev->wiphy;
	prefix_len = snprintf(buf, SUPP_LOG_LEN, "[DHD]<%s> %s: ",
		wl_get_kernel_timestamp(), __func__);
	/* Remaining buffer len */
	rem_len = SUPP_LOG_LEN - (prefix_len + 1);
	/* Print the arg list on to the remaining part of the buffer */
	va_start(args, fmt);
	len = vsnprintf((buf + prefix_len), rem_len, fmt, args);
	va_end(args);
	if (len < 0) {
		return -EINVAL;
	}

	if (len > rem_len) {
		/* If return length is greater than buffer len,
		 * then its truncated buffer case.
		 */
		len = rem_len;
	}

	/* Ensure the buffer is null terminated */
	len += prefix_len;
	buf[len] = '\0';
	len++;

	return wl_cfgvendor_send_async_event(wiphy,
		bcmcfg_to_prmry_ndev(cfg), BRCM_VENDOR_EVENT_PRIV_STR, buf, len);
}

int
wl_cfgvendor_notify_supp_event_str(const char *evt_name, const char *fmt, ...)
{
	char buf[SUPP_LOG_LEN] = {0};
	struct bcm_cfg80211 *cfg;
	struct wiphy *wiphy;
	va_list args;
	int len;
	int prefix_len;
	int rem_len;

	cfg = wl_cfg80211_get_bcmcfg();
	if (!cfg || !cfg->wdev) {
		WL_DBG(("supp evt invalid arg\n"));
		return BCME_OK;
	}
	wiphy = cfg->wdev->wiphy;
	prefix_len = snprintf(buf, SUPP_LOG_LEN, "%s ", evt_name);
	/* Remaining buffer len */
	rem_len = SUPP_LOG_LEN - (prefix_len + 1);
	/* Print the arg list on to the remaining part of the buffer */
	va_start(args, fmt);
	len = vsnprintf((buf + prefix_len), rem_len, fmt, args);
	va_end(args);
	if (len < 0) {
		return -EINVAL;
	}

	if (len > rem_len) {
		/* If return length is greater than buffer len,
		 * then its truncated buffer case.
		 */
		len = rem_len;
	}

	/* Ensure the buffer is null terminated */
	len += prefix_len;
	buf[len] = '\0';
	len++;

	return wl_cfgvendor_send_async_event(wiphy,
		bcmcfg_to_prmry_ndev(cfg), BRCM_VENDOR_EVENT_PRIV_STR, buf, len);
}
#endif /* WL_SUPP_EVENT */

/*
 * This API is to be used for asynchronous vendor events. This
 * shouldn't be used in response to a vendor command from its
 * do_it handler context (instead wl_cfgvendor_send_cmd_reply should
 * be used).
 */
int wl_cfgvendor_send_async_event(struct wiphy *wiphy,
	struct net_device *dev, int event_id, const void  *data, int len)
{
	gfp_t kflags;
	struct sk_buff *skb;

	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;

	/* Alloc the SKB for vendor_event */
#if (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	skb = cfg80211_vendor_event_alloc(wiphy, ndev_to_wdev(dev), len, event_id, kflags);
#else
	skb = cfg80211_vendor_event_alloc(wiphy, len, event_id, kflags);
#endif /* (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || */
		/* LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0) */
	if (!skb) {
		WL_ERR(("skb alloc failed"));
		return -ENOMEM;
	}

	/* Push the data to the skb */
	nla_put_nohdr(skb, len, data);

	cfg80211_vendor_event(skb, kflags);

	return 0;
}

static int
wl_cfgvendor_send_cmd_reply(struct wiphy *wiphy,
	const void  *data, int len)
{
	struct sk_buff *skb;
	int err;

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, len);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		err = -ENOMEM;
		goto exit;
	}

	/* Push the data to the skb */
	nla_put_nohdr(skb, len, data);
	err = cfg80211_vendor_cmd_reply(skb);
exit:
	WL_DBG(("wl_cfgvendor_send_cmd_reply status %d", err));
	return err;
}

static int
wl_cfgvendor_get_feature_set(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int reply;

	reply = dhd_dev_get_feature_set(bcmcfg_to_prmry_ndev(cfg));

	err =  wl_cfgvendor_send_cmd_reply(wiphy, &reply, sizeof(int));
	if (unlikely(err))
		WL_ERR(("Vendor Command reply failed ret:%d \n", err));

	return err;
}

static int
wl_cfgvendor_get_feature_set_matrix(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	struct sk_buff *skb;
	int reply;
	int mem_needed, i;

	mem_needed = VENDOR_REPLY_OVERHEAD +
		(ATTRIBUTE_U32_LEN * MAX_FEATURE_SET_CONCURRRENT_GROUPS) + ATTRIBUTE_U32_LEN;

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		err = -ENOMEM;
		goto exit;
	}

	nla_put_u32(skb, ANDR_WIFI_ATTRIBUTE_NUM_FEATURE_SET, MAX_FEATURE_SET_CONCURRRENT_GROUPS);
	for (i = 0; i < MAX_FEATURE_SET_CONCURRRENT_GROUPS; i++) {
		reply = dhd_dev_get_feature_set_matrix(bcmcfg_to_prmry_ndev(cfg), i);
		if (reply != WIFI_FEATURE_INVALID) {
			nla_put_u32(skb, ANDR_WIFI_ATTRIBUTE_FEATURE_SET, reply);
		}
	}

	err =  cfg80211_vendor_cmd_reply(skb);

	if (unlikely(err)) {
		WL_ERR(("Vendor Command reply failed ret:%d \n", err));
	}
exit:
	return err;
}

static int
wl_cfgvendor_set_rand_mac_oui(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type;

	type = nla_type(data);

	if (type == ANDR_WIFI_ATTRIBUTE_RANDOM_MAC_OUI) {
		if (nla_len(data) != DOT11_OUI_LEN) {
			WL_ERR(("nla_len not matched.\n"));
			err = -EINVAL;
			goto exit;
		}
		err = dhd_dev_cfg_rand_mac_oui(bcmcfg_to_prmry_ndev(cfg), nla_data(data));

		if (unlikely(err))
			WL_ERR(("Bad OUI, could not set:%d \n", err));

	} else {
		err = -EINVAL;
	}
exit:
	return err;
}
#ifdef CUSTOM_FORCE_NODFS_FLAG
static int
wl_cfgvendor_set_nodfs_flag(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type;
	u32 nodfs;

	type = nla_type(data);
	if (type == ANDR_WIFI_ATTRIBUTE_NODFS_SET) {
		nodfs = nla_get_u32(data);
		err = dhd_dev_set_nodfs(bcmcfg_to_prmry_ndev(cfg), nodfs);
	} else {
		err = -1;
	}
	return err;
}
#endif /* CUSTOM_FORCE_NODFS_FLAG */

static int
wl_cfgvendor_set_country(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	int err = BCME_ERROR, rem, type;
	char country_code[WLC_CNTRY_BUF_SZ] = {0};
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	struct net_device *primary_ndev = bcmcfg_to_prmry_ndev(cfg);

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case ANDR_WIFI_ATTRIBUTE_COUNTRY:
				err = memcpy_s(country_code, WLC_CNTRY_BUF_SZ,
					nla_data(iter), nla_len(iter));
				if (err) {
					WL_ERR(("Failed to copy country code: %d\n", err));
					return err;
				}
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				return err;
		}
	}
	/* country code is unique for dongle..hence using primary interface. */
	err = wl_cfg80211_set_country_code(primary_ndev, country_code, true, true, -1);
	if (err < 0) {
		WL_ERR(("Set country failed ret:%d\n", err));
	}

	return err;
}

#ifdef GSCAN_SUPPORT
int
wl_cfgvendor_send_hotlist_event(struct wiphy *wiphy,
	struct net_device *dev, void  *data, int len, wl_vendor_event_t event)
{
	gfp_t kflags;
	const void *ptr;
	struct sk_buff *skb;
	int malloc_len, total, iter_cnt_to_send, cnt;
	gscan_results_cache_t *cache = (gscan_results_cache_t *)data;

	total = len/sizeof(wifi_gscan_result_t);
	while (total > 0) {
		malloc_len = (total * sizeof(wifi_gscan_result_t)) + VENDOR_DATA_OVERHEAD;
		if (malloc_len > NLMSG_DEFAULT_SIZE) {
			malloc_len = NLMSG_DEFAULT_SIZE;
		}
		iter_cnt_to_send =
		   (malloc_len - VENDOR_DATA_OVERHEAD)/sizeof(wifi_gscan_result_t);
		total = total - iter_cnt_to_send;

		kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;

		/* Alloc the SKB for vendor_event */
#if (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
		skb = cfg80211_vendor_event_alloc(wiphy, ndev_to_wdev(dev),
		malloc_len, event, kflags);
#else
		skb = cfg80211_vendor_event_alloc(wiphy, malloc_len, event, kflags);
#endif /* (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || */
		/* LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0) */
		if (!skb) {
			WL_ERR(("skb alloc failed"));
			return -ENOMEM;
		}

		while (cache && iter_cnt_to_send) {
			ptr = (const void *) &cache->results[cache->tot_consumed];

			if (iter_cnt_to_send < (cache->tot_count - cache->tot_consumed)) {
				cnt = iter_cnt_to_send;
			} else {
				cnt = (cache->tot_count - cache->tot_consumed);
			}

			iter_cnt_to_send -= cnt;
			cache->tot_consumed += cnt;
			/* Push the data to the skb */
			nla_append(skb, cnt * sizeof(wifi_gscan_result_t), ptr);
			if (cache->tot_consumed == cache->tot_count) {
				cache = cache->next;
			}

		}

		cfg80211_vendor_event(skb, kflags);
	}

	return 0;
}

static int
wl_cfgvendor_gscan_get_capabilities(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pno_gscan_capabilities_t *reply = NULL;
	uint32 reply_len = 0;

	reply = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg),
	   DHD_PNO_GET_CAPABILITIES, NULL, &reply_len);
	if (!reply) {
		WL_ERR(("Could not get capabilities\n"));
		err = -EINVAL;
		return err;
	}

	err =  wl_cfgvendor_send_cmd_reply(wiphy, reply, reply_len);

	if (unlikely(err)) {
		WL_ERR(("Vendor Command reply failed ret:%d \n", err));
	}

	MFREE(cfg->osh, reply, reply_len);
	return err;
}

static int
wl_cfgvendor_gscan_get_batch_results(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_results_cache_t *results, *iter;
	uint32 reply_len, is_done = 1;
	int32 mem_needed, num_results_iter;
	wifi_gscan_result_t *ptr;
	uint16 num_scan_ids, num_results;
	struct sk_buff *skb;
	struct nlattr *scan_hdr, *complete_flag;

	err = dhd_dev_wait_batch_results_complete(bcmcfg_to_prmry_ndev(cfg));
	if (err != BCME_OK)
		return -EBUSY;

	err = dhd_dev_pno_lock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
	if (err != BCME_OK) {
		WL_ERR(("Can't obtain lock to access batch results %d\n", err));
		return -EBUSY;
	}
	results = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg),
	             DHD_PNO_GET_BATCH_RESULTS, NULL, &reply_len);
	if (!results) {
		WL_ERR(("No results to send %d\n", err));
		err =  wl_cfgvendor_send_cmd_reply(wiphy, results, 0);

		if (unlikely(err))
			WL_ERR(("Vendor Command reply failed ret:%d \n", err));
		dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
		return err;
	}
	num_scan_ids = reply_len & 0xFFFF;
	num_results = (reply_len & 0xFFFF0000) >> 16;
	mem_needed = (num_results * sizeof(wifi_gscan_result_t)) +
	             (num_scan_ids * GSCAN_BATCH_RESULT_HDR_LEN) +
	             VENDOR_REPLY_OVERHEAD + SCAN_RESULTS_COMPLETE_FLAG_LEN;

	if (mem_needed > (int32)NLMSG_DEFAULT_SIZE) {
		mem_needed = (int32)NLMSG_DEFAULT_SIZE;
	}

	WL_TRACE(("is_done %d mem_needed %d max_mem %d\n", is_done, mem_needed,
		(int)NLMSG_DEFAULT_SIZE));
	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
		return -ENOMEM;
	}
	iter = results;
	complete_flag = nla_reserve(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS_COMPLETE,
	                    sizeof(is_done));
	mem_needed = mem_needed - (SCAN_RESULTS_COMPLETE_FLAG_LEN + VENDOR_REPLY_OVERHEAD);

	while (iter) {
		num_results_iter = (mem_needed - (int32)GSCAN_BATCH_RESULT_HDR_LEN);
		num_results_iter /= (int32)sizeof(wifi_gscan_result_t);
		if (num_results_iter <= 0 ||
		    ((iter->tot_count - iter->tot_consumed) > num_results_iter)) {
			break;
		}
		scan_hdr = nla_nest_start(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS);
		/* no more room? we are done then (for now) */
		if (scan_hdr == NULL) {
			is_done = 0;
			break;
		}
		nla_put_u32(skb, GSCAN_ATTRIBUTE_SCAN_ID, iter->scan_id);
		nla_put_u8(skb, GSCAN_ATTRIBUTE_SCAN_FLAGS, iter->flag);
		nla_put_u32(skb, GSCAN_ATTRIBUTE_CH_BUCKET_BITMASK, iter->scan_ch_bucket);

		num_results_iter = iter->tot_count - iter->tot_consumed;

		nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_OF_RESULTS, num_results_iter);
		if (num_results_iter) {
			ptr = &iter->results[iter->tot_consumed];
			iter->tot_consumed += num_results_iter;
			nla_put(skb, GSCAN_ATTRIBUTE_SCAN_RESULTS,
			 num_results_iter * sizeof(wifi_gscan_result_t), ptr);
		}
		nla_nest_end(skb, scan_hdr);
		mem_needed -= GSCAN_BATCH_RESULT_HDR_LEN +
		    (num_results_iter * sizeof(wifi_gscan_result_t));
		iter = iter->next;
	}
	MFREE(cfg->osh, results, reply_len);
	/* Returns TRUE if all result consumed */
	is_done = dhd_dev_gscan_batch_cache_cleanup(bcmcfg_to_prmry_ndev(cfg));
	memcpy(nla_data(complete_flag), &is_done, sizeof(is_done));
	dhd_dev_pno_unlock_access_batch_results(bcmcfg_to_prmry_ndev(cfg));
	return cfg80211_vendor_cmd_reply(skb);
}

static int
wl_cfgvendor_initiate_gscan(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type, tmp = len;
	int run = 0xFF;
	int flush = 0;
	const struct nlattr *iter;

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		if (type == GSCAN_ATTRIBUTE_ENABLE_FEATURE)
			run = nla_get_u32(iter);
		else if (type == GSCAN_ATTRIBUTE_FLUSH_FEATURE)
			flush = nla_get_u32(iter);
	}

	if (run != 0xFF) {
		err = dhd_dev_pno_run_gscan(bcmcfg_to_prmry_ndev(cfg), run, flush);

		if (unlikely(err)) {
			WL_ERR(("Could not run gscan:%d \n", err));
		}
		return err;
	} else {
		return -EINVAL;
	}

}

static int
wl_cfgvendor_enable_full_scan_result(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type;
	bool real_time = FALSE;

	type = nla_type(data);

	if (type == GSCAN_ATTRIBUTE_ENABLE_FULL_SCAN_RESULTS) {
		real_time = nla_get_u32(data);

		err = dhd_dev_pno_enable_full_scan_result(bcmcfg_to_prmry_ndev(cfg), real_time);

		if (unlikely(err)) {
			WL_ERR(("Could not run gscan:%d \n", err));
		}

	} else {
		err = -EINVAL;
	}

	return err;
}

static int
wl_cfgvendor_set_scan_cfg_bucket(const struct nlattr *prev,
	gscan_scan_params_t *scan_param, int num)
{
	struct dhd_pno_gscan_channel_bucket  *ch_bucket;
	int k = 0;
	int type, err = 0, rem;
	const struct nlattr *cur, *next;

	nla_for_each_nested(cur, prev, rem) {
		type = nla_type(cur);
		ch_bucket = scan_param->channel_bucket;
		switch (type) {
		case GSCAN_ATTRIBUTE_BUCKET_ID:
			break;
		case GSCAN_ATTRIBUTE_BUCKET_PERIOD:
			if (nla_len(cur) != sizeof(uint32)) {
				err = -EINVAL;
				goto exit;
			}

			ch_bucket[num].bucket_freq_multiple =
				nla_get_u32(cur) / MSEC_PER_SEC;
			break;
		case GSCAN_ATTRIBUTE_BUCKET_NUM_CHANNELS:
			if (nla_len(cur) != sizeof(uint32)) {
				err = -EINVAL;
				goto exit;
			}
			ch_bucket[num].num_channels = nla_get_u32(cur);
			if (ch_bucket[num].num_channels >
				GSCAN_MAX_CHANNELS_IN_BUCKET) {
				WL_ERR(("channel range:%d,bucket:%d\n",
					ch_bucket[num].num_channels,
					num));
				err = -EINVAL;
				goto exit;
			}
			break;
		case GSCAN_ATTRIBUTE_BUCKET_CHANNELS:
			nla_for_each_nested(next, cur, rem) {
				if (k >= GSCAN_MAX_CHANNELS_IN_BUCKET)
					break;
				if (nla_len(next) != sizeof(uint32)) {
					err = -EINVAL;
					goto exit;
				}
				ch_bucket[num].chan_list[k] = nla_get_u32(next);
				k++;
			}
			break;
		case GSCAN_ATTRIBUTE_BUCKETS_BAND:
			if (nla_len(cur) != sizeof(uint32)) {
				err = -EINVAL;
				goto exit;
			}
			ch_bucket[num].band = (uint16)nla_get_u32(cur);
			break;
		case GSCAN_ATTRIBUTE_REPORT_EVENTS:
			if (nla_len(cur) != sizeof(uint32)) {
				err = -EINVAL;
				goto exit;
			}
			ch_bucket[num].report_flag = (uint8)nla_get_u32(cur);
			break;
		case GSCAN_ATTRIBUTE_BUCKET_STEP_COUNT:
			if (nla_len(cur) != sizeof(uint32)) {
				err = -EINVAL;
				goto exit;
			}
			ch_bucket[num].repeat = (uint16)nla_get_u32(cur);
			break;
		case GSCAN_ATTRIBUTE_BUCKET_MAX_PERIOD:
			if (nla_len(cur) != sizeof(uint32)) {
				err = -EINVAL;
				goto exit;
			}
			ch_bucket[num].bucket_max_multiple =
				nla_get_u32(cur) / MSEC_PER_SEC;
			break;
		default:
			WL_ERR(("unknown attr type:%d\n", type));
			err = -EINVAL;
			goto exit;
		}
	}

exit:
	return err;
}

static int
wl_cfgvendor_set_scan_cfg(struct wiphy *wiphy, struct wireless_dev *wdev,
	const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_scan_params_t *scan_param;
	int j = 0;
	int type, tmp;
	const struct nlattr *iter;

	scan_param = (gscan_scan_params_t *)MALLOCZ(cfg->osh,
		sizeof(gscan_scan_params_t));
	if (!scan_param) {
		WL_ERR(("Could not set GSCAN scan cfg, mem alloc failure\n"));
		err = -EINVAL;
		return err;

	}

	scan_param->scan_fr = PNO_SCAN_MIN_FW_SEC;
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);

		if (j >= GSCAN_MAX_CH_BUCKETS) {
			break;
		}

		switch (type) {
			case GSCAN_ATTRIBUTE_BASE_PERIOD:
				if (nla_len(iter) != sizeof(uint32)) {
					err = -EINVAL;
					goto exit;
				}
				scan_param->scan_fr = nla_get_u32(iter) / MSEC_PER_SEC;
				break;
			case GSCAN_ATTRIBUTE_NUM_BUCKETS:
				if (nla_len(iter) != sizeof(uint32)) {
					err = -EINVAL;
					goto exit;
				}
				scan_param->nchannel_buckets = nla_get_u32(iter);
				if (scan_param->nchannel_buckets >=
				    GSCAN_MAX_CH_BUCKETS) {
					WL_ERR(("ncha_buck out of range %d\n",
					scan_param->nchannel_buckets));
					err = -EINVAL;
					goto exit;
				}
				break;
			case GSCAN_ATTRIBUTE_CH_BUCKET_1:
			case GSCAN_ATTRIBUTE_CH_BUCKET_2:
			case GSCAN_ATTRIBUTE_CH_BUCKET_3:
			case GSCAN_ATTRIBUTE_CH_BUCKET_4:
			case GSCAN_ATTRIBUTE_CH_BUCKET_5:
			case GSCAN_ATTRIBUTE_CH_BUCKET_6:
			case GSCAN_ATTRIBUTE_CH_BUCKET_7:
				err = wl_cfgvendor_set_scan_cfg_bucket(iter, scan_param, j);
				if (err < 0) {
					WL_ERR(("set_scan_cfg_buck error:%d\n", err));
					goto exit;
				}
				j++;
				break;
			default:
				WL_ERR(("Unknown type %d\n", type));
				err = -EINVAL;
				goto exit;
		}
	}

	err = dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
	     DHD_PNO_SCAN_CFG_ID, scan_param, FALSE);

	if (err < 0) {
		WL_ERR(("Could not set GSCAN scan cfg\n"));
		err = -EINVAL;
	}

exit:
	MFREE(cfg->osh, scan_param, sizeof(gscan_scan_params_t));
	return err;

}

static int
wl_cfgvendor_hotlist_cfg(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_hotlist_scan_params_t *hotlist_params;
	int tmp, tmp1, tmp2, type, j = 0, dummy;
	const struct nlattr *outer, *inner = NULL, *iter;
	bool flush = FALSE;
	struct bssid_t *pbssid;

	BCM_REFERENCE(dummy);

	if (len < sizeof(*hotlist_params) || len >= WLC_IOCTL_MAXLEN) {
		WL_ERR(("buffer length :%d wrong - bail out.\n", len));
		return -EINVAL;
	}

	hotlist_params = (gscan_hotlist_scan_params_t *)MALLOCZ(cfg->osh,
		sizeof(*hotlist_params)
		+ (sizeof(struct bssid_t) * (PFN_SWC_MAX_NUM_APS - 1)));

	if (!hotlist_params) {
		WL_ERR(("Cannot Malloc memory.\n"));
		return -ENOMEM;
	}

	hotlist_params->lost_ap_window = GSCAN_LOST_AP_WINDOW_DEFAULT;

	nla_for_each_attr(iter, data, len, tmp2) {
		type = nla_type(iter);
		switch (type) {
		case GSCAN_ATTRIBUTE_HOTLIST_BSSID_COUNT:
			if (nla_len(iter) != sizeof(uint32)) {
				WL_DBG(("type:%d length:%d not matching.\n",
					type, nla_len(iter)));
				err = -EINVAL;
				goto exit;
			}
			hotlist_params->nbssid = (uint16)nla_get_u32(iter);
			if ((hotlist_params->nbssid == 0) ||
			    (hotlist_params->nbssid > PFN_SWC_MAX_NUM_APS)) {
				WL_ERR(("nbssid:%d exceed limit.\n",
					hotlist_params->nbssid));
				err = -EINVAL;
				goto exit;
			}
			break;
		case GSCAN_ATTRIBUTE_HOTLIST_BSSIDS:
			if (hotlist_params->nbssid == 0) {
				WL_ERR(("nbssid not retrieved.\n"));
				err = -EINVAL;
				goto exit;
			}
			pbssid = hotlist_params->bssid;
			nla_for_each_nested(outer, iter, tmp) {
				if (j >= hotlist_params->nbssid)
					break;
				nla_for_each_nested(inner, outer, tmp1) {
					type = nla_type(inner);

					switch (type) {
					case GSCAN_ATTRIBUTE_BSSID:
						if (nla_len(inner) != sizeof(pbssid[j].macaddr)) {
							WL_ERR(("type:%d length:%d not matching.\n",
								type, nla_len(inner)));
							err = -EINVAL;
							goto exit;
						}
						memcpy(
							&(pbssid[j].macaddr),
							nla_data(inner),
							sizeof(pbssid[j].macaddr));
						break;
					case GSCAN_ATTRIBUTE_RSSI_LOW:
						if (nla_len(inner) != sizeof(uint8)) {
							WL_ERR(("type:%d length:%d not matching.\n",
								type, nla_len(inner)));
							err = -EINVAL;
							goto exit;
						}
						pbssid[j].rssi_reporting_threshold =
							(int8)nla_get_u8(inner);
						break;
					case GSCAN_ATTRIBUTE_RSSI_HIGH:
						if (nla_len(inner) != sizeof(uint8)) {
							WL_ERR(("type:%d length:%d not matching.\n",
								type, nla_len(inner)));
							err = -EINVAL;
							goto exit;
						}
						dummy = (int8)nla_get_u8(inner);
						WL_DBG(("dummy %d\n", dummy));
						break;
					default:
						WL_ERR(("ATTR unknown %d\n", type));
						err = -EINVAL;
						goto exit;
					}
				}
				j++;
			}
			if (j != hotlist_params->nbssid) {
				WL_ERR(("bssid_cnt:%d != nbssid:%d.\n", j,
					hotlist_params->nbssid));
				err = -EINVAL;
				goto exit;
			}
			break;
		case GSCAN_ATTRIBUTE_HOTLIST_FLUSH:
			if (nla_len(iter) != sizeof(uint8)) {
				WL_ERR(("type:%d length:%d not matching.\n",
					type, nla_len(iter)));
				err = -EINVAL;
				goto exit;
			}
			flush = nla_get_u8(iter);
			break;
		case GSCAN_ATTRIBUTE_LOST_AP_SAMPLE_SIZE:
			if (nla_len(iter) != sizeof(uint32)) {
				WL_ERR(("type:%d length:%d not matching.\n",
					type, nla_len(iter)));
				err = -EINVAL;
				goto exit;
			}
			hotlist_params->lost_ap_window = (uint16)nla_get_u32(iter);
			break;
		default:
			WL_ERR(("Unknown type %d\n", type));
			err = -EINVAL;
			goto exit;
		}

	}

	if (dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
	      DHD_PNO_GEOFENCE_SCAN_CFG_ID, hotlist_params, flush) < 0) {
		WL_ERR(("Could not set GSCAN HOTLIST cfg error: %d\n", err));
		err = -EINVAL;
		goto exit;
	}
exit:
	MFREE(cfg->osh, hotlist_params, sizeof(*hotlist_params)
		+ (sizeof(struct bssid_t) * (PFN_SWC_MAX_NUM_APS - 1)));
	return err;
}

static int wl_cfgvendor_epno_cfg(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pno_ssid_t *ssid_elem = NULL;
	int tmp, tmp1, tmp2, type = 0, num = 0;
	const struct nlattr *outer, *inner, *iter;
	uint8 flush = FALSE, i = 0;
	wl_ssid_ext_params_t params;

	nla_for_each_attr(iter, data, len, tmp2) {
		type = nla_type(iter);
		switch (type) {
			case GSCAN_ATTRIBUTE_EPNO_SSID_LIST:
				nla_for_each_nested(outer, iter, tmp) {
					ssid_elem = (dhd_pno_ssid_t *)
						dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg),
						DHD_PNO_GET_NEW_EPNO_SSID_ELEM,
						NULL, &num);
					if (!ssid_elem) {
						WL_ERR(("Failed to get SSID LIST buffer\n"));
						err = -ENOMEM;
						goto exit;
					}
					i++;
					nla_for_each_nested(inner, outer, tmp1) {
						type = nla_type(inner);

						switch (type) {
							case GSCAN_ATTRIBUTE_EPNO_SSID:
								memcpy(ssid_elem->SSID,
								  nla_data(inner),
								  DOT11_MAX_SSID_LEN);
								break;
							case GSCAN_ATTRIBUTE_EPNO_SSID_LEN:
								ssid_elem->SSID_len =
									nla_get_u32(inner);
								if (ssid_elem->SSID_len >
									DOT11_MAX_SSID_LEN) {
									WL_ERR(("SSID too"
									"long %d\n",
									ssid_elem->SSID_len));
									err = -EINVAL;
									MFREE(cfg->osh, ssid_elem,
										num);
									goto exit;
								}
								break;
							case GSCAN_ATTRIBUTE_EPNO_FLAGS:
								ssid_elem->flags =
									nla_get_u32(inner);
								ssid_elem->hidden =
									((ssid_elem->flags &
									DHD_EPNO_HIDDEN_SSID) != 0);
								break;
							case GSCAN_ATTRIBUTE_EPNO_AUTH:
								ssid_elem->wpa_auth =
								        nla_get_u32(inner);
								break;
						}
					}
					if (!ssid_elem->SSID_len) {
						WL_ERR(("Broadcast SSID is illegal for ePNO\n"));
						err = -EINVAL;
						MFREE(cfg->osh, ssid_elem, num);
						goto exit;
					}
					dhd_pno_translate_epno_fw_flags(&ssid_elem->flags);
					dhd_pno_set_epno_auth_flag(&ssid_elem->wpa_auth);
					MFREE(cfg->osh, ssid_elem, num);
				}
				break;
			case GSCAN_ATTRIBUTE_EPNO_SSID_NUM:
				num = nla_get_u8(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_FLUSH:
				flush = (bool)nla_get_u32(iter);
				/* Flush attribute is expected before any ssid attribute */
				if (i && flush) {
					WL_ERR(("Bad attributes\n"));
					err = -EINVAL;
					goto exit;
				}
				/* Need to flush driver and FW cfg */
				dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
				DHD_PNO_EPNO_CFG_ID, NULL, flush);
				dhd_dev_flush_fw_epno(bcmcfg_to_prmry_ndev(cfg));
				break;
			case GSCAN_ATTRIBUTE_EPNO_5G_RSSI_THR:
				params.min5G_rssi = nla_get_s8(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_2G_RSSI_THR:
				params.min2G_rssi = nla_get_s8(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_INIT_SCORE_MAX:
				params.init_score_max = nla_get_s16(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_CUR_CONN_BONUS:
				params.cur_bssid_bonus = nla_get_s16(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_SAME_NETWORK_BONUS:
				params.same_ssid_bonus = nla_get_s16(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_SECURE_BONUS:
				params.secure_bonus = nla_get_s16(iter);
				break;
			case GSCAN_ATTRIBUTE_EPNO_5G_BONUS:
				params.band_5g_bonus = nla_get_s16(iter);
				break;
			default:
				WL_ERR(("%s: No such attribute %d\n", __FUNCTION__, type));
				err = -EINVAL;
				goto exit;
			}
	}
	if (i != num) {
		WL_ERR(("%s: num_ssid %d does not match ssids sent %d\n", __FUNCTION__,
		     num, i));
		err = -EINVAL;
	}
exit:
	/* Flush all configs if error condition */
	if (err < 0) {
		dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
		   DHD_PNO_EPNO_CFG_ID, NULL, TRUE);
		dhd_dev_flush_fw_epno(bcmcfg_to_prmry_ndev(cfg));
	} else if (type != GSCAN_ATTRIBUTE_EPNO_FLUSH) {
		/* If the last attribute was FLUSH, nothing else to do */
		dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
		DHD_PNO_EPNO_PARAMS_ID, &params, FALSE);
		err = dhd_dev_set_epno(bcmcfg_to_prmry_ndev(cfg));
	}
	return err;
}

static int
wl_cfgvendor_set_batch_scan_cfg(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0, tmp, type;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	gscan_batch_params_t batch_param;
	const struct nlattr *iter;

	batch_param.mscan = batch_param.bestn = 0;
	batch_param.buffer_threshold = GSCAN_BATCH_NO_THR_SET;

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);

		switch (type) {
			case GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN:
				batch_param.bestn = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE:
				batch_param.mscan = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_REPORT_THRESHOLD:
				batch_param.buffer_threshold = nla_get_u32(iter);
				break;
			default:
				WL_ERR(("Unknown type %d\n", type));
				break;
		}
	}

	if (dhd_dev_pno_set_cfg_gscan(bcmcfg_to_prmry_ndev(cfg),
	       DHD_PNO_BATCH_SCAN_CFG_ID, &batch_param, FALSE) < 0) {
		WL_ERR(("Could not set batch cfg\n"));
		err = -EINVAL;
		return err;
	}

	return err;
}

#endif /* GSCAN_SUPPORT */
#if defined(GSCAN_SUPPORT) || defined(DHD_GET_VALID_CHANNELS)
static int
wl_cfgvendor_gscan_get_channel_list(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0, type, band;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	uint16 *reply = NULL;
	uint32 reply_len = 0, num_channels, mem_needed;
	struct sk_buff *skb;

	type = nla_type(data);

	if (type == GSCAN_ATTRIBUTE_BAND) {
		band = nla_get_u32(data);
	} else {
		return -EINVAL;
	}

	reply = dhd_dev_pno_get_gscan(bcmcfg_to_prmry_ndev(cfg),
	   DHD_PNO_GET_CHANNEL_LIST, &band, &reply_len);

	if (!reply) {
		WL_ERR(("Could not get channel list\n"));
		err = -EINVAL;
		return err;
	}
	num_channels =  reply_len/ sizeof(uint32);
	mem_needed = reply_len + VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		err = -ENOMEM;
		goto exit;
	}

	nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_CHANNELS, num_channels);
	nla_put(skb, GSCAN_ATTRIBUTE_CHANNEL_LIST, reply_len, reply);

	err =  cfg80211_vendor_cmd_reply(skb);

	if (unlikely(err)) {
		WL_ERR(("Vendor Command reply failed ret:%d \n", err));
	}
exit:
	MFREE(cfg->osh, reply, reply_len);
	return err;
}
#endif	/* GSCAN_SUPPORT || DHD_GET_VALID_CHANNELS */

#ifdef RSSI_MONITOR_SUPPORT
static int wl_cfgvendor_set_rssi_monitor(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0, tmp, type, start = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int8 max_rssi = 0, min_rssi = 0;
	const struct nlattr *iter;

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
			case RSSI_MONITOR_ATTRIBUTE_MAX_RSSI:
				max_rssi = (int8) nla_get_u32(iter);
				break;
			case RSSI_MONITOR_ATTRIBUTE_MIN_RSSI:
				min_rssi = (int8) nla_get_u32(iter);
				break;
			case RSSI_MONITOR_ATTRIBUTE_START:
				start = nla_get_u32(iter);
		}
	}

	if (dhd_dev_set_rssi_monitor_cfg(bcmcfg_to_prmry_ndev(cfg),
	       start, max_rssi, min_rssi) < 0) {
		WL_ERR(("Could not set rssi monitor cfg\n"));
		err = -EINVAL;
	}
	return err;
}
#endif /* RSSI_MONITOR_SUPPORT */

#ifdef DHDTCPACK_SUPPRESS
static int
wl_cfgvendor_set_tcpack_sup_mode(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	int err = BCME_OK, type;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	struct net_device *ndev = wdev_to_wlc_ndev(wdev, cfg);
	uint8 enable = 0;

	if (len <= 0) {
		WL_ERR(("Length of the nlattr is not valid len : %d\n", len));
		err = BCME_BADARG;
		goto exit;
	}

	type = nla_type(data);
	if (type == ANDR_WIFI_ATTRIBUTE_TCPACK_SUP_VALUE) {
		enable = (uint8)nla_get_u32(data);
		err = dhd_dev_set_tcpack_sup_mode_cfg(ndev, enable);
		if (unlikely(err)) {
			WL_ERR(("Could not set TCP Ack Suppress mode cfg: %d\n", err));
		}
	} else {
		err = BCME_BADARG;
	}

exit:
	return err;
}
#endif /* DHDTCPACK_SUPPRESS */

#ifdef DHD_WAKE_STATUS
static int
wl_cfgvendor_get_wake_reason_stats(struct wiphy *wiphy,
        struct wireless_dev *wdev, const void *data, int len)
{
	struct net_device *ndev = wdev_to_ndev(wdev);
	wake_counts_t *pwake_count_info;
	int ret, mem_needed;
#if defined(DHD_DEBUG) && defined(DHD_WAKE_EVENT_STATUS)
	int flowid;
#endif /* DHD_DEBUG && DHD_WAKE_EVENT_STATUS */
	struct sk_buff *skb;
	dhd_pub_t *dhdp = wl_cfg80211_get_dhdp(ndev);

	WL_DBG(("Recv get wake status info cmd.\n"));

	pwake_count_info = dhd_get_wakecount(dhdp);
	mem_needed =  VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 20) +
		(WLC_E_LAST * sizeof(uint));

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("%s: can't allocate %d bytes\n", __FUNCTION__, mem_needed));
		ret = -ENOMEM;
		goto exit;
	}
#ifdef DHD_WAKE_EVENT_STATUS
	WL_ERR(("pwake_count_info->rcwake %d\n", pwake_count_info->rcwake));
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_TOTAL_CMD_EVENT, pwake_count_info->rcwake);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_CMD_EVENT_COUNT_USED, WLC_E_LAST);
	nla_put(skb, WAKE_STAT_ATTRIBUTE_CMD_EVENT_WAKE, (WLC_E_LAST * sizeof(uint)),
		pwake_count_info->rc_event);
#ifdef DHD_DEBUG
	for (flowid = 0; flowid < WLC_E_LAST; flowid++) {
		if (pwake_count_info->rc_event[flowid] != 0) {
			WL_ERR((" %s = %u\n", bcmevent_get_name(flowid),
				pwake_count_info->rc_event[flowid]));
		}
	}
#endif /* DHD_DEBUG */
#endif /* DHD_WAKE_EVENT_STATUS */
#ifdef DHD_WAKE_RX_STATUS
	WL_ERR(("pwake_count_info->rxwake %d\n", pwake_count_info->rxwake));
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_TOTAL_RX_DATA_WAKE, pwake_count_info->rxwake);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_UNICAST_COUNT, pwake_count_info->rx_ucast);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_MULTICAST_COUNT, pwake_count_info->rx_mcast);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_BROADCAST_COUNT, pwake_count_info->rx_bcast);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_ICMP_PKT, pwake_count_info->rx_arp);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_ICMP6_PKT, pwake_count_info->rx_icmpv6);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_ICMP6_RA, pwake_count_info->rx_icmpv6_ra);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_ICMP6_NA, pwake_count_info->rx_icmpv6_na);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_RX_ICMP6_NS, pwake_count_info->rx_icmpv6_ns);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_IPV4_RX_MULTICAST_ADD_CNT,
		pwake_count_info->rx_multi_ipv4);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_IPV6_RX_MULTICAST_ADD_CNT,
		pwake_count_info->rx_multi_ipv6);
	nla_put_u32(skb, WAKE_STAT_ATTRIBUTE_OTHER_RX_MULTICAST_ADD_CNT,
		pwake_count_info->rx_multi_other);
#endif /* #ifdef DHD_WAKE_RX_STATUS */
	ret = cfg80211_vendor_cmd_reply(skb);
	if (unlikely(ret)) {
		WL_ERR(("Vendor cmd reply for -get wake status failed:%d \n", ret));
	}
exit:
	return ret;
}
#endif /* DHD_WAKE_STATUS */

#if defined(WL_CFG80211) && defined(DHD_FILE_DUMP_EVENT)
static int
wl_cfgvendor_notify_dump_completion(struct wiphy *wiphy,
        struct wireless_dev *wdev, const void *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;
	unsigned long flags = 0;

	WL_INFORM(("%s, [DUMP] received file dump notification from HAL\n", __FUNCTION__));

	DHD_GENERAL_LOCK(dhd_pub, flags);
	/* call wmb() to synchronize with the previous memory operations */
	OSL_SMP_WMB();
	DHD_BUS_BUSY_CLEAR_IN_HALDUMP(dhd_pub);
	/* Call another wmb() to make sure wait_for_dump_completion value
	 * gets updated before waking up waiting context.
	 */
	OSL_SMP_WMB();
	dhd_os_busbusy_wake(dhd_pub);
	DHD_GENERAL_UNLOCK(dhd_pub, flags);

	return BCME_OK;
}

static int
wl_cfgvendor_set_hal_started(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void  *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	WL_INFORM(("%s,[DUMP] HAL STARTED\n", __FUNCTION__));

	cfg->hal_started = true;
	return BCME_OK;
}

static int
wl_cfgvendor_stop_hal(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void  *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	WL_INFORM(("%s,[DUMP] HAL STOPPED\n", __FUNCTION__));

	cfg->hal_started = false;
	return BCME_OK;
}
#endif /* WL_CFG80211 && DHD_FILE_DUMP_EVENT */

#ifdef GSCAN_SUPPORT
static int wl_cfgvendor_enable_lazy_roam(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = -EINVAL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int type;
	uint32 lazy_roam_enable_flag;

	type = nla_type(data);

	if (type == GSCAN_ATTRIBUTE_LAZY_ROAM_ENABLE) {
		lazy_roam_enable_flag = nla_get_u32(data);

		err = dhd_dev_lazy_roam_enable(bcmcfg_to_prmry_ndev(cfg),
		           lazy_roam_enable_flag);
		if (unlikely(err))
			WL_ERR(("Could not enable lazy roam:%d \n", err));
	}
	return err;
}

static int wl_cfgvendor_set_lazy_roam_cfg(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0, tmp, type;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	wlc_roam_exp_params_t roam_param;
	const struct nlattr *iter;

	memset(&roam_param, 0, sizeof(roam_param));

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
			case GSCAN_ATTRIBUTE_A_BAND_BOOST_THRESHOLD:
				roam_param.a_band_boost_threshold = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_A_BAND_PENALTY_THRESHOLD:
				roam_param.a_band_penalty_threshold = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_A_BAND_BOOST_FACTOR:
				roam_param.a_band_boost_factor = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_A_BAND_PENALTY_FACTOR:
				roam_param.a_band_penalty_factor = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_A_BAND_MAX_BOOST:
				roam_param.a_band_max_boost = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_LAZY_ROAM_HYSTERESIS:
				roam_param.cur_bssid_boost = nla_get_u32(iter);
				break;
			case GSCAN_ATTRIBUTE_ALERT_ROAM_RSSI_TRIGGER:
				roam_param.alert_roam_trigger_threshold = nla_get_u32(iter);
				break;
		}
	}

	if (dhd_dev_set_lazy_roam_cfg(bcmcfg_to_prmry_ndev(cfg), &roam_param) < 0) {
		WL_ERR(("Could not set batch cfg\n"));
		err = -EINVAL;
	}
	return err;
}

/* small helper function */
static wl_bssid_pref_cfg_t *
create_bssid_pref_cfg(struct bcm_cfg80211 *cfg, uint32 num, uint32 *buf_len)
{
	wl_bssid_pref_cfg_t *bssid_pref;

	*buf_len = sizeof(wl_bssid_pref_cfg_t);
	if (num) {
		*buf_len += (num - 1) * sizeof(wl_bssid_pref_list_t);
	}
	bssid_pref = (wl_bssid_pref_cfg_t *)MALLOC(cfg->osh, *buf_len);

	return bssid_pref;
}

static int
wl_cfgvendor_set_bssid_pref(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	wl_bssid_pref_cfg_t *bssid_pref = NULL;
	wl_bssid_pref_list_t *bssids;
	int tmp, tmp1, tmp2, type;
	const struct nlattr *outer, *inner, *iter;
	uint32 flush = 0, num = 0, buf_len = 0;
	uint8 bssid_found = 0, rssi_found = 0;

	/* Assumption: NUM attribute must come first */
	nla_for_each_attr(iter, data, len, tmp2) {
		type = nla_type(iter);
		switch (type) {
			case GSCAN_ATTRIBUTE_NUM_BSSID:
				if (num) {
					WL_ERR(("attempt overide bssid num.\n"));
					err = -EINVAL;
					goto exit;
				}
				if (nla_len(iter) != sizeof(uint32)) {
					WL_ERR(("nla_len not match\n"));
					err = -EINVAL;
					goto exit;
				}
				num = nla_get_u32(iter);
				if (num == 0 || num > MAX_BSSID_PREF_LIST_NUM) {
					WL_ERR(("wrong BSSID num:%d\n", num));
					err = -EINVAL;
					goto exit;
				}
				if ((bssid_pref = create_bssid_pref_cfg(cfg, num, &buf_len))
							== NULL) {
					WL_ERR(("Can't malloc memory\n"));
					err = -ENOMEM;
					goto exit;
				}
				break;
			case GSCAN_ATTRIBUTE_BSSID_PREF_FLUSH:
				if (nla_len(iter) != sizeof(uint32)) {
					WL_ERR(("nla_len not match\n"));
					err = -EINVAL;
					goto exit;
				}
				flush = nla_get_u32(iter);
				if (flush != 1) {
					WL_ERR(("wrong flush value\n"));
					err = -EINVAL;
					goto exit;
				}
				break;
			case GSCAN_ATTRIBUTE_BSSID_PREF_LIST:
				if (!num || !bssid_pref) {
					WL_ERR(("bssid list count not set\n"));
					err = -EINVAL;
					goto exit;
				}
				bssid_pref->count = 0;
				bssids = bssid_pref->bssids;
				nla_for_each_nested(outer, iter, tmp) {
					if (bssid_pref->count >= num) {
						WL_ERR(("too many bssid list\n"));
						err = -EINVAL;
						goto exit;
					}
					bssid_found = 0;
					rssi_found = 0;
					nla_for_each_nested(inner, outer, tmp1) {
						type = nla_type(inner);
						switch (type) {
						case GSCAN_ATTRIBUTE_BSSID_PREF:
							if (nla_len(inner) != ETHER_ADDR_LEN) {
								WL_ERR(("nla_len not match.\n"));
								err = -EINVAL;
								goto exit;
							}
							memcpy(&(bssids[bssid_pref->count].bssid),
							  nla_data(inner), ETHER_ADDR_LEN);
							/* not used for now */
							bssids[bssid_pref->count].flags = 0;
							bssid_found = 1;
							break;
						case GSCAN_ATTRIBUTE_RSSI_MODIFIER:
							if (nla_len(inner) != sizeof(uint32)) {
								WL_ERR(("nla_len not match.\n"));
								err = -EINVAL;
								goto exit;
							}
							bssids[bssid_pref->count].rssi_factor =
							       (int8) nla_get_u32(inner);
							rssi_found = 1;
							break;
						default:
							WL_ERR(("wrong type:%d\n", type));
							err = -EINVAL;
							goto exit;
						}
						if (bssid_found && rssi_found) {
							break;
						}
					}
					bssid_pref->count++;
				}
				break;
			default:
				WL_ERR(("%s: No such attribute %d\n", __FUNCTION__, type));
				break;
			}
	}

	if (!bssid_pref) {
		/* What if only flush is desired? */
		if (flush) {
			if ((bssid_pref = create_bssid_pref_cfg(cfg, 0, &buf_len)) == NULL) {
				WL_ERR(("%s: Can't malloc memory\n", __FUNCTION__));
				err = -ENOMEM;
				goto exit;
			}
			bssid_pref->count = 0;
		} else {
			err = -EINVAL;
			goto exit;
		}
	}
	err = dhd_dev_set_lazy_roam_bssid_pref(bcmcfg_to_prmry_ndev(cfg),
	          bssid_pref, flush);
exit:
	if (bssid_pref) {
		MFREE(cfg->osh, bssid_pref, buf_len);
	}
	return err;
}
#endif /* GSCAN_SUPPORT */
#if defined(GSCAN_SUPPORT) || defined(ROAMEXP_SUPPORT)
static int
wl_cfgvendor_set_bssid_blacklist(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	maclist_t *blacklist = NULL;
	int err = 0;
	int type, tmp;
	const struct nlattr *iter;
	uint32 mem_needed = 0, flush = 0, num = 0;

	/* Assumption: NUM attribute must come first */
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
			case GSCAN_ATTRIBUTE_NUM_BSSID:
				if (num != 0) {
					WL_ERR(("attempt to change BSSID num\n"));
					err = -EINVAL;
					goto exit;
				}
				if (nla_len(iter) != sizeof(uint32)) {
					WL_ERR(("not matching nla_len.\n"));
					err = -EINVAL;
					goto exit;
				}
				num = nla_get_u32(iter);
				if (num == 0 || num > MAX_BSSID_BLACKLIST_NUM) {
					WL_ERR(("wrong BSSID count:%d\n", num));
					err = -EINVAL;
					goto exit;
				}
				if (!blacklist) {
					mem_needed = OFFSETOF(maclist_t, ea) +
						sizeof(struct ether_addr) * (num);
					blacklist = (maclist_t *)
						MALLOCZ(cfg->osh, mem_needed);
					if (!blacklist) {
						WL_ERR(("MALLOCZ failed.\n"));
						err = -ENOMEM;
						goto exit;
					}
				}
				break;
			case GSCAN_ATTRIBUTE_BSSID_BLACKLIST_FLUSH:
				if (nla_len(iter) != sizeof(uint32)) {
					WL_ERR(("not matching nla_len.\n"));
					err = -EINVAL;
					goto exit;
				}
				flush = nla_get_u32(iter);
				if (flush != 1) {
					WL_ERR(("flush arg is worng:%d\n", flush));
					err = -EINVAL;
					goto exit;
				}
				break;
			case GSCAN_ATTRIBUTE_BLACKLIST_BSSID:
				if (num == 0 || !blacklist) {
					WL_ERR(("number of BSSIDs not received.\n"));
					err = -EINVAL;
					goto exit;
				}
				if (nla_len(iter) != ETHER_ADDR_LEN) {
					WL_ERR(("not matching nla_len.\n"));
					err = -EINVAL;
					goto exit;
				}
				if (blacklist->count >= num) {
					WL_ERR(("too many BSSIDs than expected:%d\n",
						blacklist->count));
					err = -EINVAL;
					goto exit;
				}
				memcpy(&(blacklist->ea[blacklist->count]), nla_data(iter),
						ETHER_ADDR_LEN);
				blacklist->count++;
				break;
		default:
			WL_ERR(("No such attribute:%d\n", type));
			break;
		}
	}

	if (blacklist && (blacklist->count != num)) {
		WL_ERR(("not matching bssid count:%d to expected:%d\n",
				blacklist->count, num));
		err = -EINVAL;
		goto exit;
	}

	err = dhd_dev_set_blacklist_bssid(bcmcfg_to_prmry_ndev(cfg),
	          blacklist, mem_needed, flush);
exit:
	MFREE(cfg->osh, blacklist, mem_needed);
	return err;
}

static int
wl_cfgvendor_set_ssid_whitelist(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	wl_ssid_whitelist_t *ssid_whitelist = NULL;
	wlc_ssid_t *ssid_elem;
	int tmp, tmp1, mem_needed = 0, type;
	const struct nlattr *iter, *iter1;
	uint32 flush = 0, num = 0;
	int ssid_found = 0;

	/* Assumption: NUM attribute must come first */
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
		case GSCAN_ATTRIBUTE_NUM_WL_SSID:
			if (num != 0) {
				WL_ERR(("try to change SSID num\n"));
				err = -EINVAL;
				goto exit;
			}
			if (nla_len(iter) != sizeof(uint32)) {
				WL_ERR(("not matching nla_len.\n"));
				err = -EINVAL;
				goto exit;
			}
			num = nla_get_u32(iter);
			if (num == 0 || num > MAX_SSID_WHITELIST_NUM) {
				WL_ERR(("wrong SSID count:%d\n", num));
				err = -EINVAL;
				goto exit;
			}
			mem_needed = sizeof(wl_ssid_whitelist_t) +
				sizeof(wlc_ssid_t) * num;
			ssid_whitelist = (wl_ssid_whitelist_t *)
				MALLOCZ(cfg->osh, mem_needed);
			if (ssid_whitelist == NULL) {
				WL_ERR(("failed to alloc mem\n"));
				err = -ENOMEM;
				goto exit;
			}
			break;
		case GSCAN_ATTRIBUTE_WL_SSID_FLUSH:
			if (nla_len(iter) != sizeof(uint32)) {
				WL_ERR(("not matching nla_len.\n"));
				err = -EINVAL;
				goto exit;
			}
			flush = nla_get_u32(iter);
			if (flush != 1) {
				WL_ERR(("flush arg worng:%d\n", flush));
				err = -EINVAL;
				goto exit;
			}
			break;
		case GSCAN_ATTRIBUTE_WHITELIST_SSID_ELEM:
			if (!num || !ssid_whitelist) {
				WL_ERR(("num ssid is not set!\n"));
				err = -EINVAL;
				goto exit;
			}
			if (ssid_whitelist->ssid_count >= num) {
				WL_ERR(("too many SSIDs:%d\n",
					ssid_whitelist->ssid_count));
				err = -EINVAL;
				goto exit;
			}

			ssid_elem = &ssid_whitelist->ssids[
					ssid_whitelist->ssid_count];
			ssid_found = 0;
			nla_for_each_nested(iter1, iter, tmp1) {
				type = nla_type(iter1);
				switch (type) {
				case GSCAN_ATTRIBUTE_WL_SSID_LEN:
					if (nla_len(iter1) != sizeof(uint32)) {
						WL_ERR(("not match nla_len\n"));
						err = -EINVAL;
						goto exit;
					}
					ssid_elem->SSID_len = nla_get_u32(iter1);
					if (ssid_elem->SSID_len >
							DOT11_MAX_SSID_LEN) {
						WL_ERR(("wrong SSID len:%d\n",
							ssid_elem->SSID_len));
						err = -EINVAL;
						goto exit;
					}
					break;
				case GSCAN_ATTRIBUTE_WHITELIST_SSID:
					if (ssid_elem->SSID_len == 0) {
						WL_ERR(("SSID_len not received\n"));
						err = -EINVAL;
						goto exit;
					}
					if (nla_len(iter1) != ssid_elem->SSID_len) {
						WL_ERR(("not match nla_len\n"));
						err = -EINVAL;
						goto exit;
					}
					memcpy(ssid_elem->SSID, nla_data(iter1),
							ssid_elem->SSID_len);
					ssid_found = 1;
					break;
				}
				if (ssid_found) {
					ssid_whitelist->ssid_count++;
					break;
				}
			}
			break;
		default:
			WL_ERR(("No such attribute: %d\n", type));
			break;
		}
	}

	if (ssid_whitelist && (ssid_whitelist->ssid_count != num)) {
		WL_ERR(("not matching ssid count:%d to expected:%d\n",
				ssid_whitelist->ssid_count, num));
		err = -EINVAL;
	}
	err = dhd_dev_set_whitelist_ssid(bcmcfg_to_prmry_ndev(cfg),
	          ssid_whitelist, mem_needed, flush);
exit:
	MFREE(cfg->osh, ssid_whitelist, mem_needed);
	return err;
}
#endif /* GSCAN_SUPPORT || ROAMEXP_SUPPORT */

#ifdef ROAMEXP_SUPPORT
typedef enum {
	FW_ROAMING_ENABLE = 1,
	FW_ROAMING_DISABLE,
	FW_ROAMING_PAUSE,
	FW_ROAMING_RESUME
} fw_roaming_state_t;

static int
wl_cfgvendor_set_fw_roaming_state(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	fw_roaming_state_t requested_roaming_state;
	int type;
	int err = 0;

	/* Get the requested fw roaming state */
	type = nla_type(data);
	if (type != GSCAN_ATTRIBUTE_ROAM_STATE_SET) {
		WL_ERR(("%s: Invalid attribute %d\n", __FUNCTION__, type));
		return -EINVAL;
	}

	requested_roaming_state = nla_get_u32(data);
	WL_INFORM(("setting FW roaming state to %d\n", requested_roaming_state));

	if ((requested_roaming_state == FW_ROAMING_ENABLE) ||
		(requested_roaming_state == FW_ROAMING_RESUME)) {
		err = wldev_iovar_setint(wdev_to_ndev(wdev), "roam_off", FALSE);
	} else if ((requested_roaming_state == FW_ROAMING_DISABLE) ||
		(requested_roaming_state == FW_ROAMING_PAUSE)) {
		err = wldev_iovar_setint(wdev_to_ndev(wdev), "roam_off", TRUE);
	} else {
		err = -EINVAL;
	}

	return err;
}

static int
wl_cfgvendor_fw_roam_get_capability(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = 0;
	wifi_roaming_capabilities_t roaming_capability;

	/* Update max number of blacklist bssids supported */
	roaming_capability.max_blacklist_size = MAX_BSSID_BLACKLIST_NUM;
	roaming_capability.max_whitelist_size = MAX_SSID_WHITELIST_NUM;
	err =  wl_cfgvendor_send_cmd_reply(wiphy, &roaming_capability,
		sizeof(roaming_capability));
	if (unlikely(err)) {
		WL_ERR(("Vendor cmd reply for fw roam capability failed ret:%d \n", err));
	}

	return err;
}
#endif /* ROAMEXP_SUPPORT */

static int
wl_cfgvendor_priv_string_handler(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int ret = 0;
	int ret_len = 0, payload = 0, msglen;
	const struct bcm_nlmsg_hdr *nlioc = data;
	void *buf = NULL, *cur;
	int maxmsglen = PAGE_SIZE - 0x100;
	struct sk_buff *reply;
	dhd_pub_t *dhdp = wl_cfg80211_get_dhdp(wdev->netdev);

	WL_DBG(("entry: cmd = %d\n", nlioc->cmd));

	/* send to dongle only if we are not waiting for reload already */
	if (dhdp && dhdp->hang_was_sent) {
		WL_INFORM(("Bus down. HANG was sent up earlier\n"));
		DHD_OS_WAKE_LOCK_CTRL_TIMEOUT_ENABLE(dhdp, DHD_EVENT_TIMEOUT_MS);
		DHD_OS_WAKE_UNLOCK(dhdp);
		return OSL_ERROR(BCME_DONGLE_DOWN);
	}
	if (nlioc->len <= 0) {
		WL_ERR(("invalid len %d\n", nlioc->len));
		return BCME_BADARG;
	}

	if (nlioc->offset != sizeof(struct bcm_nlmsg_hdr) ||
		len <= sizeof(struct bcm_nlmsg_hdr)) {
		WL_ERR(("invalid offset %d\n", nlioc->offset));
		return BCME_BADARG;
	}
	len -= sizeof(struct bcm_nlmsg_hdr);
	ret_len = nlioc->len;
	if (ret_len > 0 || len > 0) {
		if (len >= DHD_IOCTL_MAXLEN) {
			WL_ERR(("oversize input buffer %d\n", len));
			len = DHD_IOCTL_MAXLEN - 1;
		}
		if (ret_len >= DHD_IOCTL_MAXLEN) {
			WL_ERR(("oversize return buffer %d\n", ret_len));
			ret_len = DHD_IOCTL_MAXLEN - 1;
		}

		payload = max(ret_len, len) + 1;
		buf = vzalloc(payload);
		if (!buf) {
			return -ENOMEM;
		}
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif // endif
		memcpy(buf, (void *)((char *)nlioc + nlioc->offset), len);
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // endif
		*((char *)buf + len) = '\0';
	}

	ret = dhd_cfgvendor_priv_string_handler(cfg, wdev, nlioc, buf);
	if (ret) {
		WL_ERR(("dhd_cfgvendor returned error %d", ret));
		vfree(buf);
		return ret;
	}
	cur = buf;
	while (ret_len > 0) {
		msglen = ret_len > maxmsglen ? maxmsglen : ret_len;
		ret_len -= msglen;
		payload = msglen + sizeof(msglen);
		reply = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, payload);
		if (!reply) {
			WL_ERR(("Failed to allocate reply msg\n"));
			ret = -ENOMEM;
			break;
		}

		if (nla_put(reply, BCM_NLATTR_DATA, msglen, cur) ||
			nla_put_u16(reply, BCM_NLATTR_LEN, msglen)) {
			kfree_skb(reply);
			ret = -ENOBUFS;
			break;
		}

		ret = cfg80211_vendor_cmd_reply(reply);
		if (ret) {
			WL_ERR(("testmode reply failed:%d\n", ret));
			break;
		}
		cur = (void *)((char *)cur + msglen);
	}

	return ret;
}

struct net_device *
wl_cfgvendor_get_ndev(struct bcm_cfg80211 *cfg, struct wireless_dev *wdev,
	const char *data, unsigned long int *out_addr)
{
	char *pos, *pos1;
	char ifname[IFNAMSIZ + 1] = {0};
	struct net_info *iter, *next;
	struct net_device *ndev = NULL;
	ulong ifname_len;
	*out_addr = (unsigned long int) data; /* point to command str by default */

	/* check whether ifname=<ifname> is provided in the command */
	pos = strstr(data, "ifname=");
	if (pos) {
		pos += strlen("ifname=");
		pos1 = strstr(pos, " ");
		if (!pos1) {
			WL_ERR(("command format error \n"));
			return NULL;
		}

		ifname_len = pos1 - pos;
		if (memcpy_s(ifname, (sizeof(ifname) - 1), pos, ifname_len) != BCME_OK) {
			WL_ERR(("Failed to copy data. len: %ld\n", ifname_len));
			return NULL;
		}
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif // endif
		for_each_ndev(cfg, iter, next) {
			if (iter->ndev) {
				if (strncmp(iter->ndev->name, ifname,
					strlen(iter->ndev->name)) == 0) {
					/* matching ifname found */
					WL_DBG(("matching interface (%s) found ndev:%p \n",
						iter->ndev->name, iter->ndev));
					*out_addr = (unsigned long int)(pos1 + 1);
					/* Returns the command portion after ifname=<name> */
					return iter->ndev;
				}
			}
		}
#if defined(STRICT_GCC_WARNINGS) && defined(__GNUC__)
#pragma GCC diagnostic pop
#endif // endif
		WL_ERR(("Couldn't find ifname:%s in the netinfo list \n",
			ifname));
		return NULL;
	}

	/* If ifname=<name> arg is not provided, use default ndev */
	ndev = wdev->netdev ? wdev->netdev : bcmcfg_to_prmry_ndev(cfg);
	WL_DBG(("Using default ndev (%s) \n", ndev->name));
	return ndev;
}

/* Max length for the reply buffer. For BRCM_ATTR_DRIVER_CMD, the reply
 * would be a formatted string and reply buf would be the size of the
 * string.
 */
#define WL_DRIVER_PRIV_CMD_LEN 512

#ifdef WL_SAE
static int
wl_cfgvendor_set_sae_password(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int err = BCME_OK;
	struct net_device *net = wdev->netdev;
	struct bcm_cfg80211 *cfg = wl_get_cfg(net);
	wsec_pmk_t pmk;
	s32 bssidx;

	if ((bssidx = wl_get_bssidx_by_wdev(cfg, net->ieee80211_ptr)) < 0) {
		WL_ERR(("Find p2p index from wdev(%p) failed\n", net->ieee80211_ptr));
		return BCME_ERROR;
	}

	if (len < WSEC_MIN_PSK_LEN || len >= WSEC_MAX_PSK_LEN) {
		WL_ERR(("Invalid passphrase length %d..should be >=8 and <=63\n",
			len));
		err = BCME_BADLEN;
		goto done;
	}
	/* Set AUTH to SAE */
	err = wldev_iovar_setint_bsscfg(net, "wpa_auth", WPA3_AUTH_SAE_PSK, bssidx);
	if (unlikely(err)) {
		WL_ERR(("could not set wpa_auth (0x%x)\n", err));
		goto done;
	}
	pmk.key_len = htod16(len);
	bcopy((u8*)data, pmk.key, len);
	pmk.flags = htod16(WSEC_PASSPHRASE);

	err = wldev_ioctl_set(net, WLC_SET_WSEC_PMK, &pmk, sizeof(pmk));
	if (err) {
		WL_ERR(("\n failed to set pmk %d\n", err));
		goto done;
	} else {
		WL_MEM(("sae passphrase set successfully\n"));
	}
done:
	return err;
}
#endif /* WL_SAE */

#ifdef BCM_PRIV_CMD_SUPPORT
/* strlen("ifname=") + IFNAMESIZE + strlen(" ") + '\0' */
#define ANDROID_PRIV_CMD_IF_PREFIX_LEN	(7 + IFNAMSIZ + 2)
static int
wl_cfgvendor_priv_bcm_handler(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	const struct nlattr *iter;
	int err = 0;
	int data_len = 0, cmd_len = 0, tmp = 0, type = 0;
	struct net_device *ndev = wdev->netdev;
	char *cmd = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int bytes_written;
	struct net_device *net = NULL;
	unsigned long int cmd_out = 0;
#if defined(WL_ANDROID_PRIV_CMD_OVER_NL80211)
	u32 cmd_buf_len = WL_DRIVER_PRIV_CMD_LEN;
	char cmd_prefix[ANDROID_PRIV_CMD_IF_PREFIX_LEN + 1] = {0};
	char *cmd_buf = NULL;
	char *current_pos;
	u32 cmd_offset;
#endif /* WL_ANDROID_PRIV_CMD_OVER_NL80211 && OEM_ANDROID */

	WL_DBG(("%s: Enter \n", __func__));

	/* hold wake lock */
	net_os_wake_lock(ndev);

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		cmd = nla_data(iter);
		cmd_len = nla_len(iter);

		WL_DBG(("%s: type: %d cmd_len:%d cmd_ptr:%p \n", __func__, type, cmd_len, cmd));
		if (!cmd || !cmd_len) {
			WL_ERR(("Invalid cmd data \n"));
			err = -EINVAL;
			goto exit;
		}

#if defined(WL_ANDROID_PRIV_CMD_OVER_NL80211)
		if (type == BRCM_ATTR_DRIVER_CMD) {
			if ((cmd_len >= WL_DRIVER_PRIV_CMD_LEN) ||
				(cmd_len < ANDROID_PRIV_CMD_IF_PREFIX_LEN)) {
				WL_ERR(("Unexpected command length (%u)."
					"Ignore the command\n", cmd_len));
				err = -EINVAL;
				goto exit;
			}

			/* check whether there is any ifname prefix provided */
			if (memcpy_s(cmd_prefix, (sizeof(cmd_prefix) - 1),
					cmd, ANDROID_PRIV_CMD_IF_PREFIX_LEN) != BCME_OK) {
				WL_ERR(("memcpy failed for cmd buffer. len:%d\n", cmd_len));
				err = -ENOMEM;
				goto exit;
			}

			net = wl_cfgvendor_get_ndev(cfg, wdev, cmd_prefix, &cmd_out);
			if (!cmd_out || !net) {
				WL_ERR(("ndev not found\n"));
				err = -ENODEV;
				goto exit;
			}

			/* find offset of the command */
			current_pos = (char *)cmd_out;
			cmd_offset = current_pos - cmd_prefix;

			if (!current_pos || (cmd_offset) > ANDROID_PRIV_CMD_IF_PREFIX_LEN) {
				WL_ERR(("Invalid len cmd_offset: %u \n", cmd_offset));
				err = -EINVAL;
				goto exit;
			}

			/* Private command data in expected to be in str format. To ensure that
			 * the data is null terminated, copy to a local buffer before use
			 */
			cmd_buf = (char *)MALLOCZ(cfg->osh, cmd_buf_len);
			if (!cmd_buf) {
				WL_ERR(("memory alloc failed for %u \n", cmd_buf_len));
				err = -ENOMEM;
				goto exit;
			}

			/* Point to the start of command */
			if (memcpy_s(cmd_buf, (WL_DRIVER_PRIV_CMD_LEN - 1),
				(const void *)(cmd + cmd_offset),
				(cmd_len - cmd_offset - 1)) != BCME_OK) {
				WL_ERR(("memcpy failed for cmd buffer. len:%d\n", cmd_len));
				err = -ENOMEM;
				goto exit;
			}
			cmd_buf[WL_DRIVER_PRIV_CMD_LEN - 1] = '\0';

			WL_DBG(("vendor_command: %s len: %u \n", cmd_buf, cmd_buf_len));
			bytes_written = wl_handle_private_cmd(net, cmd_buf, cmd_buf_len);
			WL_DBG(("bytes_written: %d \n", bytes_written));
			if (bytes_written == 0) {
				snprintf(cmd_buf, cmd_buf_len, "%s", "OK");
				data_len = strlen("OK");
			} else if (bytes_written > 0) {
				if (bytes_written >= (cmd_buf_len - 1)) {
					/* Not expected */
					ASSERT(0);
					err = -EINVAL;
					goto exit;
				}
				data_len = bytes_written;
			} else {
				/* -ve return value. Propagate the error back */
				err = bytes_written;
				goto exit;
			}
			if ((data_len > 0) && (data_len < (cmd_buf_len - 1)) && cmd_buf) {
				err =  wl_cfgvendor_send_cmd_reply(wiphy, cmd_buf, data_len+1);
				if (unlikely(err)) {
					WL_ERR(("Vendor Command reply failed ret:%d \n", err));
				} else {
					WL_DBG(("Vendor Command reply sent successfully!\n"));
				}
			} else {
				/* No data to be sent back as reply */
				WL_ERR(("Vendor_cmd: No reply expected. data_len:%u cmd_buf %p \n",
					data_len, cmd_buf));
			}
			break;
		}
#endif /* WL_ANDROID_PRIV_CMD_OVER_NL80211 && OEM_ANDROID */
	}

exit:
#if defined(WL_ANDROID_PRIV_CMD_OVER_NL80211)
	if (cmd_buf) {
		MFREE(cfg->osh, cmd_buf, cmd_buf_len);
	}
#endif /* WL_ANDROID_PRIV_CMD_OVER_NL80211 && OEM_ANDROID */
	net_os_wake_unlock(ndev);
	return err;
}
#endif /* BCM_PRIV_CMD_SUPPORT */

#ifdef WL_NAN
static const char *nan_attr_to_str(u16 cmd)
{
	switch (cmd) {
	C2S(NAN_ATTRIBUTE_HEADER)
	C2S(NAN_ATTRIBUTE_HANDLE)
	C2S(NAN_ATTRIBUTE_TRANSAC_ID)
	C2S(NAN_ATTRIBUTE_2G_SUPPORT)
	C2S(NAN_ATTRIBUTE_SDF_2G_SUPPORT)
	C2S(NAN_ATTRIBUTE_SDF_5G_SUPPORT)
	C2S(NAN_ATTRIBUTE_5G_SUPPORT)
	C2S(NAN_ATTRIBUTE_SYNC_DISC_2G_BEACON)
	C2S(NAN_ATTRIBUTE_SYNC_DISC_5G_BEACON)
	C2S(NAN_ATTRIBUTE_CLUSTER_LOW)
	C2S(NAN_ATTRIBUTE_CLUSTER_HIGH)
	C2S(NAN_ATTRIBUTE_SID_BEACON)
	C2S(NAN_ATTRIBUTE_RSSI_CLOSE)
	C2S(NAN_ATTRIBUTE_RSSI_MIDDLE)
	C2S(NAN_ATTRIBUTE_RSSI_PROXIMITY)
	C2S(NAN_ATTRIBUTE_RSSI_CLOSE_5G)
	C2S(NAN_ATTRIBUTE_RSSI_MIDDLE_5G)
	C2S(NAN_ATTRIBUTE_RSSI_PROXIMITY_5G)
	C2S(NAN_ATTRIBUTE_HOP_COUNT_LIMIT)
	C2S(NAN_ATTRIBUTE_RANDOM_TIME)
	C2S(NAN_ATTRIBUTE_MASTER_PREF)
	C2S(NAN_ATTRIBUTE_PERIODIC_SCAN_INTERVAL)
	C2S(NAN_ATTRIBUTE_PUBLISH_ID)
	C2S(NAN_ATTRIBUTE_TTL)
	C2S(NAN_ATTRIBUTE_PERIOD)
	C2S(NAN_ATTRIBUTE_REPLIED_EVENT_FLAG)
	C2S(NAN_ATTRIBUTE_PUBLISH_TYPE)
	C2S(NAN_ATTRIBUTE_TX_TYPE)
	C2S(NAN_ATTRIBUTE_PUBLISH_COUNT)
	C2S(NAN_ATTRIBUTE_SERVICE_NAME_LEN)
	C2S(NAN_ATTRIBUTE_SERVICE_NAME)
	C2S(NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO_LEN)
	C2S(NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO)
	C2S(NAN_ATTRIBUTE_RX_MATCH_FILTER_LEN)
	C2S(NAN_ATTRIBUTE_RX_MATCH_FILTER)
	C2S(NAN_ATTRIBUTE_TX_MATCH_FILTER_LEN)
	C2S(NAN_ATTRIBUTE_TX_MATCH_FILTER)
	C2S(NAN_ATTRIBUTE_SUBSCRIBE_ID)
	C2S(NAN_ATTRIBUTE_SUBSCRIBE_TYPE)
	C2S(NAN_ATTRIBUTE_SERVICERESPONSEFILTER)
	C2S(NAN_ATTRIBUTE_SERVICERESPONSEINCLUDE)
	C2S(NAN_ATTRIBUTE_USESERVICERESPONSEFILTER)
	C2S(NAN_ATTRIBUTE_SSIREQUIREDFORMATCHINDICATION)
	C2S(NAN_ATTRIBUTE_SUBSCRIBE_MATCH)
	C2S(NAN_ATTRIBUTE_SUBSCRIBE_COUNT)
	C2S(NAN_ATTRIBUTE_MAC_ADDR)
	C2S(NAN_ATTRIBUTE_MAC_ADDR_LIST)
	C2S(NAN_ATTRIBUTE_MAC_ADDR_LIST_NUM_ENTRIES)
	C2S(NAN_ATTRIBUTE_PUBLISH_MATCH)
	C2S(NAN_ATTRIBUTE_ENABLE_STATUS)
	C2S(NAN_ATTRIBUTE_JOIN_STATUS)
	C2S(NAN_ATTRIBUTE_ROLE)
	C2S(NAN_ATTRIBUTE_MASTER_RANK)
	C2S(NAN_ATTRIBUTE_ANCHOR_MASTER_RANK)
	C2S(NAN_ATTRIBUTE_CNT_PEND_TXFRM)
	C2S(NAN_ATTRIBUTE_CNT_BCN_TX)
	C2S(NAN_ATTRIBUTE_CNT_BCN_RX)
	C2S(NAN_ATTRIBUTE_CNT_SVC_DISC_TX)
	C2S(NAN_ATTRIBUTE_CNT_SVC_DISC_RX)
	C2S(NAN_ATTRIBUTE_AMBTT)
	C2S(NAN_ATTRIBUTE_CLUSTER_ID)
	C2S(NAN_ATTRIBUTE_INST_ID)
	C2S(NAN_ATTRIBUTE_OUI)
	C2S(NAN_ATTRIBUTE_STATUS)
	C2S(NAN_ATTRIBUTE_DE_EVENT_TYPE)
	C2S(NAN_ATTRIBUTE_MERGE)
	C2S(NAN_ATTRIBUTE_IFACE)
	C2S(NAN_ATTRIBUTE_CHANNEL)
	C2S(NAN_ATTRIBUTE_24G_CHANNEL)
	C2S(NAN_ATTRIBUTE_5G_CHANNEL)
	C2S(NAN_ATTRIBUTE_PEER_ID)
	C2S(NAN_ATTRIBUTE_NDP_ID)
	C2S(NAN_ATTRIBUTE_SECURITY)
	C2S(NAN_ATTRIBUTE_QOS)
	C2S(NAN_ATTRIBUTE_RSP_CODE)
	C2S(NAN_ATTRIBUTE_INST_COUNT)
	C2S(NAN_ATTRIBUTE_PEER_DISC_MAC_ADDR)
	C2S(NAN_ATTRIBUTE_PEER_NDI_MAC_ADDR)
	C2S(NAN_ATTRIBUTE_IF_ADDR)
	C2S(NAN_ATTRIBUTE_WARMUP_TIME)
	C2S(NAN_ATTRIBUTE_RECV_IND_CFG)
	C2S(NAN_ATTRIBUTE_CONNMAP)
	C2S(NAN_ATTRIBUTE_DWELL_TIME)
	C2S(NAN_ATTRIBUTE_SCAN_PERIOD)
	C2S(NAN_ATTRIBUTE_RSSI_WINDOW_SIZE)
	C2S(NAN_ATTRIBUTE_CONF_CLUSTER_VAL)
	C2S(NAN_ATTRIBUTE_CIPHER_SUITE_TYPE)
	C2S(NAN_ATTRIBUTE_KEY_TYPE)
	C2S(NAN_ATTRIBUTE_KEY_LEN)
	C2S(NAN_ATTRIBUTE_SCID)
	C2S(NAN_ATTRIBUTE_SCID_LEN)
	C2S(NAN_ATTRIBUTE_SDE_CONTROL_CONFIG_DP)
	C2S(NAN_ATTRIBUTE_SDE_CONTROL_SECURITY)
	C2S(NAN_ATTRIBUTE_SDE_CONTROL_DP_TYPE)
	C2S(NAN_ATTRIBUTE_SDE_CONTROL_RANGE_SUPPORT)
	C2S(NAN_ATTRIBUTE_NO_CONFIG_AVAIL)
	C2S(NAN_ATTRIBUTE_2G_AWAKE_DW)
	C2S(NAN_ATTRIBUTE_5G_AWAKE_DW)
	C2S(NAN_ATTRIBUTE_RSSI_THRESHOLD_FLAG)
	C2S(NAN_ATTRIBUTE_KEY_DATA)
	C2S(NAN_ATTRIBUTE_SDEA_SERVICE_SPECIFIC_INFO_LEN)
	C2S(NAN_ATTRIBUTE_SDEA_SERVICE_SPECIFIC_INFO)
	C2S(NAN_ATTRIBUTE_REASON)
	C2S(NAN_ATTRIBUTE_DISC_IND_CFG)
	C2S(NAN_ATTRIBUTE_DWELL_TIME_5G)
	C2S(NAN_ATTRIBUTE_SCAN_PERIOD_5G)
	C2S(NAN_ATTRIBUTE_SUB_SID_BEACON)
	default:
		return "NAN_ATTRIBUTE_UNKNOWN";
	}
}

nan_hal_status_t nan_status_reasonstr_map[] = {
	{NAN_STATUS_SUCCESS, "NAN status success"},
	{NAN_STATUS_INTERNAL_FAILURE, "NAN Discovery engine failure"},
	{NAN_STATUS_PROTOCOL_FAILURE, "protocol failure"},
	{NAN_STATUS_INVALID_PUBLISH_SUBSCRIBE_ID, "invalid pub_sub ID"},
	{NAN_STATUS_NO_RESOURCE_AVAILABLE, "No space available"},
	{NAN_STATUS_INVALID_PARAM, "invalid param"},
	{NAN_STATUS_INVALID_REQUESTOR_INSTANCE_ID, "invalid req inst id"},
	{NAN_STATUS_INVALID_NDP_ID, "invalid ndp id"},
	{NAN_STATUS_NAN_NOT_ALLOWED, "Nan not allowed"},
	{NAN_STATUS_NO_OTA_ACK, "No OTA ack"},
	{NAN_STATUS_ALREADY_ENABLED, "NAN is Already enabled"},
	{NAN_STATUS_FOLLOWUP_QUEUE_FULL, "Follow-up queue full"},
	{NAN_STATUS_UNSUPPORTED_CONCURRENCY_NAN_DISABLED, "unsupported concurrency"},
};

void
wl_cfgvendor_add_nan_reason_str(nan_status_type_t status, nan_hal_resp_t *nan_req_resp)
{
	int i = 0;
	int num = (int)(sizeof(nan_status_reasonstr_map)/sizeof(nan_status_reasonstr_map[0]));
	for (i = 0; i < num; i++) {
		if (nan_status_reasonstr_map[i].status == status) {
			strlcpy(nan_req_resp->nan_reason, nan_status_reasonstr_map[i].nan_reason,
				sizeof(nan_status_reasonstr_map[i].nan_reason));
			break;
		}
	}
}

nan_status_type_t
wl_cfgvendor_brcm_to_nanhal_status(int32 vendor_status)
{
	nan_status_type_t hal_status;
	switch (vendor_status) {
		case BCME_OK:
			hal_status = NAN_STATUS_SUCCESS;
			break;
		case BCME_BUSY:
		case BCME_NOTREADY:
			hal_status = NAN_STATUS_NAN_NOT_ALLOWED;
			break;
		case BCME_BADLEN:
		case BCME_BADBAND:
		case BCME_UNSUPPORTED:
		case BCME_USAGE_ERROR:
		case BCME_BADARG:
			hal_status = NAN_STATUS_INVALID_PARAM;
			break;
		case BCME_NOMEM:
		case BCME_NORESOURCE:
		case WL_NAN_E_SVC_SUB_LIST_FULL:
			hal_status = NAN_STATUS_NO_RESOURCE_AVAILABLE;
			break;
		case WL_NAN_E_SD_TX_LIST_FULL:
			hal_status = NAN_STATUS_FOLLOWUP_QUEUE_FULL;
			break;
		case WL_NAN_E_BAD_INSTANCE:
			hal_status = NAN_STATUS_INVALID_PUBLISH_SUBSCRIBE_ID;
			break;
		default:
			WL_ERR(("%s Unknown vendor status, status = %d\n",
					__func__, vendor_status));
			/* Generic error */
			hal_status = NAN_STATUS_INTERNAL_FAILURE;
	}
	return hal_status;
}

static int
wl_cfgvendor_nan_cmd_reply(struct wiphy *wiphy, int nan_cmd,
	nan_hal_resp_t *nan_req_resp, int ret, int nan_cmd_status)
{
	int err;
	int nan_reply;
	nan_req_resp->subcmd = nan_cmd;
	if (ret == BCME_OK) {
		nan_reply = nan_cmd_status;
	} else {
		nan_reply = ret;
	}
	nan_req_resp->status = wl_cfgvendor_brcm_to_nanhal_status(nan_reply);
	nan_req_resp->value = ret;
	err = wl_cfgvendor_send_cmd_reply(wiphy, nan_req_resp,
		sizeof(*nan_req_resp));
	/* giving more prio to ret than err */
	return (ret == 0) ? err : ret;
}

static void
wl_cfgvendor_free_disc_cmd_data(struct bcm_cfg80211 *cfg,
	nan_discover_cmd_data_t *cmd_data)
{
	if (!cmd_data) {
		WL_ERR(("Cmd_data is null\n"));
		return;
	}
	if (cmd_data->svc_info.data) {
		MFREE(cfg->osh, cmd_data->svc_info.data, cmd_data->svc_info.dlen);
	}
	if (cmd_data->svc_hash.data) {
		MFREE(cfg->osh, cmd_data->svc_hash.data, cmd_data->svc_hash.dlen);
	}
	if (cmd_data->rx_match.data) {
		MFREE(cfg->osh, cmd_data->rx_match.data, cmd_data->rx_match.dlen);
	}
	if (cmd_data->tx_match.data) {
		MFREE(cfg->osh, cmd_data->tx_match.data, cmd_data->tx_match.dlen);
	}
	if (cmd_data->mac_list.list) {
		MFREE(cfg->osh, cmd_data->mac_list.list,
			cmd_data->mac_list.num_mac_addr * ETHER_ADDR_LEN);
	}
	if (cmd_data->key.data) {
		MFREE(cfg->osh, cmd_data->key.data, NAN_MAX_PMK_LEN);
	}
	if (cmd_data->sde_svc_info.data) {
		MFREE(cfg->osh, cmd_data->sde_svc_info.data, cmd_data->sde_svc_info.dlen);
	}
	MFREE(cfg->osh, cmd_data, sizeof(*cmd_data));
}

static void
wl_cfgvendor_free_dp_cmd_data(struct bcm_cfg80211 *cfg,
	nan_datapath_cmd_data_t *cmd_data)
{
	if (!cmd_data) {
		WL_ERR(("Cmd_data is null\n"));
		return;
	}
	if (cmd_data->svc_hash.data) {
		MFREE(cfg->osh, cmd_data->svc_hash.data, cmd_data->svc_hash.dlen);
	}
	if (cmd_data->svc_info.data) {
		MFREE(cfg->osh, cmd_data->svc_info.data, cmd_data->svc_info.dlen);
	}
	if (cmd_data->key.data) {
		MFREE(cfg->osh, cmd_data->key.data, NAN_MAX_PMK_LEN);
	}
	MFREE(cfg->osh, cmd_data, sizeof(*cmd_data));
}

#define WL_NAN_EVENT_MAX_BUF 256
#ifdef WL_NAN_DISC_CACHE
static int
wl_cfgvendor_nan_parse_dp_sec_info_args(struct wiphy *wiphy,
	const void *buf, int len, nan_datapath_sec_info_cmd_data_t *cmd_data)
{
	int ret = BCME_OK;
	int attr_type;
	int rem = len;
	const struct nlattr *iter;

	NAN_DBG_ENTER();

	nla_for_each_attr(iter, buf, len, rem) {
		attr_type = nla_type(iter);
		WL_TRACE(("attr: %s (%u)\n", nan_attr_to_str(attr_type), attr_type));

		switch (attr_type) {
		case NAN_ATTRIBUTE_MAC_ADDR:
			memcpy((char*)&cmd_data->mac_addr, (char*)nla_data(iter),
				ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_PUBLISH_ID:
			cmd_data->pub_id = nla_get_u16(iter);
			break;
		case NAN_ATTRIBUTE_NDP_ID:
			cmd_data->ndp_instance_id = nla_get_u32(iter);
			break;
		default:
			WL_ERR(("%s: Unknown type, %d\n", __FUNCTION__, attr_type));
			ret = BCME_BADARG;
			break;
		}
	}
	/* We need to call set_config_handler b/f calling start enable TBD */
	NAN_DBG_EXIT();
	return ret;
}
#endif /* WL_NAN_DISC_CACHE */

int8 chanbuf[CHANSPEC_STR_LEN];
static int
wl_cfgvendor_nan_parse_datapath_args(struct wiphy *wiphy,
	const void *buf, int len, nan_datapath_cmd_data_t *cmd_data)
{
	int ret = BCME_OK;
	int attr_type;
	int rem = len;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int chan;

	NAN_DBG_ENTER();

	nla_for_each_attr(iter, buf, len, rem) {
		attr_type = nla_type(iter);
		WL_TRACE(("attr: %s (%u)\n", nan_attr_to_str(attr_type), attr_type));

		switch (attr_type) {
		case NAN_ATTRIBUTE_NDP_ID:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ndp_instance_id = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_IFACE:
			if (nla_len(iter) >= sizeof(cmd_data->ndp_iface)) {
				WL_ERR(("iface_name len wrong:%d\n", nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			strlcpy((char *)cmd_data->ndp_iface, (char *)nla_data(iter),
				nla_len(iter));
			break;
		case NAN_ATTRIBUTE_SECURITY:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ndp_cfg.security_cfg = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_QOS:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ndp_cfg.qos_cfg = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_RSP_CODE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rsp_code = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_INST_COUNT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->num_ndp_instances = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_PEER_DISC_MAC_ADDR:
			if (nla_len(iter) != ETHER_ADDR_LEN) {
				ret = -EINVAL;
				goto exit;
			}
			memcpy((char*)&cmd_data->peer_disc_mac_addr,
				(char*)nla_data(iter), ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_PEER_NDI_MAC_ADDR:
			if (nla_len(iter) != ETHER_ADDR_LEN) {
				ret = -EINVAL;
				goto exit;
			}
			memcpy((char*)&cmd_data->peer_ndi_mac_addr,
				(char*)nla_data(iter), ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_MAC_ADDR:
			if (nla_len(iter) != ETHER_ADDR_LEN) {
				ret = -EINVAL;
				goto exit;
			}
			memcpy((char*)&cmd_data->mac_addr, (char*)nla_data(iter),
				ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_IF_ADDR:
			if (nla_len(iter) != ETHER_ADDR_LEN) {
				ret = -EINVAL;
				goto exit;
			}
			memcpy((char*)&cmd_data->if_addr, (char*)nla_data(iter),
				ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_ENTRY_CONTROL:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->avail_params.duration = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_AVAIL_BIT_MAP:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->avail_params.bmap = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_CHANNEL: {
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			/* take the default channel start_factor frequency */
			chan = wf_mhz2channel((uint)nla_get_u32(iter), 0);
			if (chan <= CH_MAX_2G_CHANNEL) {
				cmd_data->avail_params.chanspec[0] =
					wf_channel2chspec(chan, WL_CHANSPEC_BW_20);
			} else {
				cmd_data->avail_params.chanspec[0] =
					wf_channel2chspec(chan, WL_CHANSPEC_BW_80);
			}
			if (cmd_data->avail_params.chanspec[0] == 0) {
				WL_ERR(("Channel is not valid \n"));
				ret = -EINVAL;
				goto exit;
			}
			WL_TRACE(("valid chanspec, chanspec = 0x%04x \n",
				cmd_data->avail_params.chanspec[0]));
			break;
		}
		case NAN_ATTRIBUTE_NO_CONFIG_AVAIL:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->avail_params.no_config_avail = (bool)nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_SERVICE_NAME_LEN: {
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_hash.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->svc_hash.dlen = nla_get_u16(iter);
			if (cmd_data->svc_hash.dlen != WL_NAN_SVC_HASH_LEN) {
				WL_ERR(("invalid svc_hash length = %u\n", cmd_data->svc_hash.dlen));
				ret = -EINVAL;
				goto exit;
			}
			break;
		}
		case NAN_ATTRIBUTE_SERVICE_NAME:
			if ((!cmd_data->svc_hash.dlen) ||
				(nla_len(iter) != cmd_data->svc_hash.dlen)) {
				WL_ERR(("invalid svc_hash length = %d,%d\n",
					cmd_data->svc_hash.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_hash.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->svc_hash.data =
				MALLOCZ(cfg->osh, cmd_data->svc_hash.dlen);
			if (!cmd_data->svc_hash.data) {
				WL_ERR(("failed to allocate svc_hash data, len=%d\n",
					cmd_data->svc_hash.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->svc_hash.data, nla_data(iter),
				cmd_data->svc_hash.dlen);
			break;
		case NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO_LEN:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_info.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->svc_info.dlen = nla_get_u16(iter);
			if (cmd_data->svc_info.dlen > MAX_APP_INFO_LEN) {
				WL_ERR_RLMT(("Not allowed beyond :%d\n", MAX_APP_INFO_LEN));
				ret = -EINVAL;
				goto exit;
			}
			break;
		case NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO:
			if ((!cmd_data->svc_info.dlen) ||
				(nla_len(iter) != cmd_data->svc_info.dlen)) {
				WL_ERR(("failed to allocate svc info by invalid len=%d,%d\n",
					cmd_data->svc_info.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_info.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->svc_info.data = MALLOCZ(cfg->osh, cmd_data->svc_info.dlen);
			if (cmd_data->svc_info.data == NULL) {
				WL_ERR(("failed to allocate svc info data, len=%d\n",
					cmd_data->svc_info.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->svc_info.data,
					nla_data(iter), cmd_data->svc_info.dlen);
			break;
		case NAN_ATTRIBUTE_PUBLISH_ID:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->pub_id = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_CIPHER_SUITE_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->csid = nla_get_u8(iter);
			WL_TRACE(("CSID = %u\n", cmd_data->csid));
			break;
		case NAN_ATTRIBUTE_KEY_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->key_type = nla_get_u8(iter);
			WL_TRACE(("Key Type = %u\n", cmd_data->key_type));
			break;
		case NAN_ATTRIBUTE_KEY_LEN:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->key.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->key.dlen = nla_get_u32(iter);
			if ((!cmd_data->key.dlen) || (cmd_data->key.dlen > WL_NAN_NCS_SK_PMK_LEN)) {
				WL_ERR(("invalid key length = %u\n", cmd_data->key.dlen));
				ret = -EINVAL;
				goto exit;
			}
			WL_TRACE(("valid key length = %u\n", cmd_data->key.dlen));
			break;
		case NAN_ATTRIBUTE_KEY_DATA:
			if ((!cmd_data->key.dlen) ||
				(nla_len(iter) != cmd_data->key.dlen)) {
				WL_ERR(("failed to allocate key data by invalid len=%d,%d\n",
					cmd_data->key.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->key.data) {
				WL_ERR(("trying to overwrite key data.\n"));
				ret = -EINVAL;
				goto exit;
			}

			cmd_data->key.data = MALLOCZ(cfg->osh, NAN_MAX_PMK_LEN);
			if (cmd_data->key.data == NULL) {
				WL_ERR(("failed to allocate key data, len=%d\n",
					cmd_data->key.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->key.data, nla_data(iter),
				MIN(cmd_data->key.dlen, NAN_MAX_PMK_LEN));
			break;

		default:
			WL_ERR(("Unknown type, %d\n", attr_type));
			ret = -EINVAL;
			goto exit;
		}
	}
exit:
	/* We need to call set_config_handler b/f calling start enable TBD */
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_parse_discover_args(struct wiphy *wiphy,
	const void *buf, int len, nan_discover_cmd_data_t *cmd_data)
{
	int ret = BCME_OK;
	int attr_type;
	int rem = len;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	u8 val_u8;
	u32 bit_flag;
	u8 flag_match;

	NAN_DBG_ENTER();

	nla_for_each_attr(iter, buf, len, rem) {
		attr_type = nla_type(iter);
		WL_TRACE(("attr: %s (%u)\n", nan_attr_to_str(attr_type), attr_type));

		switch (attr_type) {
		case NAN_ATTRIBUTE_TRANSAC_ID:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->token = nla_get_u16(iter);
			break;
		case NAN_ATTRIBUTE_PERIODIC_SCAN_INTERVAL:
			break;

		/* Nan Publish/Subscribe request Attributes */
		case NAN_ATTRIBUTE_PUBLISH_ID:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->pub_id = nla_get_u16(iter);
			cmd_data->local_id = cmd_data->pub_id;
			break;
		case NAN_ATTRIBUTE_MAC_ADDR:
			if (nla_len(iter) != ETHER_ADDR_LEN) {
				ret = -EINVAL;
				goto exit;
			}
			memcpy((char*)&cmd_data->mac_addr, (char*)nla_data(iter),
				ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO_LEN:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_info.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->svc_info.dlen = nla_get_u16(iter);
			if (cmd_data->svc_info.dlen > NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
				WL_ERR_RLMT(("Not allowed beyond :%d\n",
					NAN_MAX_SERVICE_SPECIFIC_INFO_LEN));
				ret = -EINVAL;
				goto exit;
			}
			break;
		case NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO:
			if ((!cmd_data->svc_info.dlen) ||
				(nla_len(iter) != cmd_data->svc_info.dlen)) {
				WL_ERR(("failed to allocate svc info by invalid len=%d,%d\n",
					cmd_data->svc_info.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_info.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}

			cmd_data->svc_info.data = MALLOCZ(cfg->osh, cmd_data->svc_info.dlen);
			if (cmd_data->svc_info.data == NULL) {
				WL_ERR(("failed to allocate svc info data, len=%d\n",
					cmd_data->svc_info.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->svc_info.data,
					nla_data(iter), cmd_data->svc_info.dlen);
			break;
		case NAN_ATTRIBUTE_SUBSCRIBE_ID:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->sub_id = nla_get_u16(iter);
			cmd_data->local_id = cmd_data->sub_id;
			break;
		case NAN_ATTRIBUTE_SUBSCRIBE_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->flags |= nla_get_u8(iter) ? WL_NAN_SUB_ACTIVE : 0;
			break;
		case NAN_ATTRIBUTE_PUBLISH_COUNT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->life_count = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_PUBLISH_TYPE: {
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			val_u8 = nla_get_u8(iter);
			if (val_u8 == 0) {
				cmd_data->flags |= WL_NAN_PUB_UNSOLICIT;
			} else if (val_u8 == 1) {
				cmd_data->flags |= WL_NAN_PUB_SOLICIT;
			} else {
				cmd_data->flags |= WL_NAN_PUB_BOTH;
			}
			break;
		}
		case NAN_ATTRIBUTE_PERIOD: {
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u16(iter) > NAN_MAX_AWAKE_DW_INTERVAL) {
				WL_ERR(("Invalid/Out of bound value = %u\n", nla_get_u16(iter)));
				ret = BCME_BADARG;
				break;
			}
			if (nla_get_u16(iter)) {
				cmd_data->period = 1 << (nla_get_u16(iter)-1);
			}
			break;
		}
		case NAN_ATTRIBUTE_REPLIED_EVENT_FLAG:
			break;
		case NAN_ATTRIBUTE_TTL:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ttl = nla_get_u16(iter);
			break;
		case NAN_ATTRIBUTE_SERVICE_NAME_LEN: {
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_hash.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}

			cmd_data->svc_hash.dlen = nla_get_u16(iter);
			if (cmd_data->svc_hash.dlen != WL_NAN_SVC_HASH_LEN) {
				WL_ERR(("invalid svc_hash length = %u\n", cmd_data->svc_hash.dlen));
				ret = -EINVAL;
				goto exit;
			}
			break;
		}
		case NAN_ATTRIBUTE_SERVICE_NAME:
			if ((!cmd_data->svc_hash.dlen) ||
				(nla_len(iter) != cmd_data->svc_hash.dlen)) {
				WL_ERR(("invalid svc_hash length = %d,%d\n",
					cmd_data->svc_hash.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->svc_hash.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}

			cmd_data->svc_hash.data =
				MALLOCZ(cfg->osh, cmd_data->svc_hash.dlen);
			if (!cmd_data->svc_hash.data) {
				WL_ERR(("failed to allocate svc_hash data, len=%d\n",
					cmd_data->svc_hash.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->svc_hash.data, nla_data(iter),
				cmd_data->svc_hash.dlen);
			break;
		case NAN_ATTRIBUTE_PEER_ID:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->remote_id = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_INST_ID:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->local_id = nla_get_u16(iter);
			break;
		case NAN_ATTRIBUTE_SUBSCRIBE_COUNT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->life_count = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_SSIREQUIREDFORMATCHINDICATION: {
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			bit_flag = (u32)nla_get_u8(iter);
			cmd_data->flags |=
				bit_flag ? WL_NAN_SUB_MATCH_IF_SVC_INFO : 0;
			break;
		}
		case NAN_ATTRIBUTE_SUBSCRIBE_MATCH:
		case NAN_ATTRIBUTE_PUBLISH_MATCH: {
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			flag_match = nla_get_u8(iter);

			switch (flag_match) {
			case NAN_MATCH_ALG_MATCH_CONTINUOUS:
				/* Default fw behaviour, no need to set explicitly */
				break;
			case NAN_MATCH_ALG_MATCH_ONCE:
				cmd_data->flags |= WL_NAN_MATCH_ONCE;
				break;
			case NAN_MATCH_ALG_MATCH_NEVER:
				cmd_data->flags |= WL_NAN_MATCH_NEVER;
				break;
			default:
				WL_ERR(("invalid nan match alg = %u\n", flag_match));
				ret = -EINVAL;
				goto exit;
			}
			break;
		}
		case NAN_ATTRIBUTE_SERVICERESPONSEFILTER:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->srf_type = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_SERVICERESPONSEINCLUDE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->srf_include = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_USESERVICERESPONSEFILTER:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->use_srf = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_RX_MATCH_FILTER_LEN:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->rx_match.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rx_match.dlen = nla_get_u16(iter);
			if (cmd_data->rx_match.dlen > MAX_MATCH_FILTER_LEN) {
				ret = -EINVAL;
				WL_ERR_RLMT(("Not allowed beyond %d\n", MAX_MATCH_FILTER_LEN));
				goto exit;
			}
			break;
		case NAN_ATTRIBUTE_RX_MATCH_FILTER:
			if ((!cmd_data->rx_match.dlen) ||
			    (nla_len(iter) != cmd_data->rx_match.dlen)) {
				WL_ERR(("RX match filter len wrong:%d,%d\n",
					cmd_data->rx_match.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->rx_match.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rx_match.data =
				MALLOCZ(cfg->osh, cmd_data->rx_match.dlen);
			if (cmd_data->rx_match.data == NULL) {
				WL_ERR(("failed to allocate LEN=[%u]\n",
					cmd_data->rx_match.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->rx_match.data, nla_data(iter),
				cmd_data->rx_match.dlen);
			break;
		case NAN_ATTRIBUTE_TX_MATCH_FILTER_LEN:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->tx_match.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->tx_match.dlen = nla_get_u16(iter);
			if (cmd_data->tx_match.dlen > MAX_MATCH_FILTER_LEN) {
				ret = -EINVAL;
				WL_ERR_RLMT(("Not allowed beyond %d\n", MAX_MATCH_FILTER_LEN));
				goto exit;
			}
			break;
		case NAN_ATTRIBUTE_TX_MATCH_FILTER:
			if ((!cmd_data->tx_match.dlen) ||
			    (nla_len(iter) != cmd_data->tx_match.dlen)) {
				WL_ERR(("TX match filter len wrong:%d,%d\n",
					cmd_data->tx_match.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->tx_match.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->tx_match.data =
				MALLOCZ(cfg->osh, cmd_data->tx_match.dlen);
			if (cmd_data->tx_match.data == NULL) {
				WL_ERR(("failed to allocate LEN=[%u]\n",
					cmd_data->tx_match.dlen));
				ret = -EINVAL;
				goto exit;
			}
			memcpy(cmd_data->tx_match.data, nla_data(iter),
					cmd_data->tx_match.dlen);
			break;
		case NAN_ATTRIBUTE_MAC_ADDR_LIST_NUM_ENTRIES:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->mac_list.num_mac_addr) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->mac_list.num_mac_addr = nla_get_u16(iter);
			break;
		case NAN_ATTRIBUTE_MAC_ADDR_LIST:
			if ((!cmd_data->mac_list.num_mac_addr) ||
			    (nla_len(iter) != (cmd_data->mac_list.num_mac_addr * ETHER_ADDR_LEN))) {
				WL_ERR(("wrong mac list len:%d,%d\n",
					cmd_data->mac_list.num_mac_addr, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->mac_list.list) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->mac_list.list =
				MALLOCZ(cfg->osh, (cmd_data->mac_list.num_mac_addr
						* ETHER_ADDR_LEN));
			if (cmd_data->mac_list.list == NULL) {
				WL_ERR(("failed to allocate LEN=[%u]\n",
				(cmd_data->mac_list.num_mac_addr * ETHER_ADDR_LEN)));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->mac_list.list, nla_data(iter),
				(cmd_data->mac_list.num_mac_addr * ETHER_ADDR_LEN));
			break;
		case NAN_ATTRIBUTE_TX_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			val_u8 =  nla_get_u8(iter);
			if (val_u8 == 0) {
				cmd_data->flags |= WL_NAN_PUB_BCAST;
				WL_TRACE(("NAN_ATTRIBUTE_TX_TYPE: flags=NAN_PUB_BCAST\n"));
			}
			break;
		case NAN_ATTRIBUTE_SDE_CONTROL_CONFIG_DP:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u8(iter) == 1) {
				cmd_data->sde_control_flag
					|= NAN_SDE_CF_DP_REQUIRED;
				break;
			}
			break;
		case NAN_ATTRIBUTE_SDE_CONTROL_RANGE_SUPPORT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->sde_control_config = TRUE;
			if (nla_get_u8(iter) == 1) {
				cmd_data->sde_control_flag
					|= NAN_SDE_CF_RANGING_REQUIRED;
				break;
			}
			break;
		case NAN_ATTRIBUTE_SDE_CONTROL_DP_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u8(iter) == 1) {
				cmd_data->sde_control_flag
					|= NAN_SDE_CF_MULTICAST_TYPE;
				break;
			}
			break;
		case NAN_ATTRIBUTE_SDE_CONTROL_SECURITY:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u8(iter) == 1) {
				cmd_data->sde_control_flag
					|= NAN_SDE_CF_SECURITY_REQUIRED;
				break;
			}
			break;
		case NAN_ATTRIBUTE_RECV_IND_CFG:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->recv_ind_flag = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_CIPHER_SUITE_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->csid = nla_get_u8(iter);
			WL_TRACE(("CSID = %u\n", cmd_data->csid));
			break;
		case NAN_ATTRIBUTE_KEY_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->key_type = nla_get_u8(iter);
			WL_TRACE(("Key Type = %u\n", cmd_data->key_type));
			break;
		case NAN_ATTRIBUTE_KEY_LEN:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->key.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->key.dlen = nla_get_u32(iter);
			if ((!cmd_data->key.dlen) || (cmd_data->key.dlen > WL_NAN_NCS_SK_PMK_LEN)) {
				WL_ERR(("invalid key length = %u\n",
					cmd_data->key.dlen));
				break;
			}
			WL_TRACE(("valid key length = %u\n", cmd_data->key.dlen));
			break;
		case NAN_ATTRIBUTE_KEY_DATA:
			if (!cmd_data->key.dlen ||
			    (nla_len(iter) != cmd_data->key.dlen)) {
				WL_ERR(("failed to allocate key data by invalid len=%d,%d\n",
					cmd_data->key.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->key.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}

			cmd_data->key.data = MALLOCZ(cfg->osh, NAN_MAX_PMK_LEN);
			if (cmd_data->key.data == NULL) {
				WL_ERR(("failed to allocate key data, len=%d\n",
					cmd_data->key.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->key.data, nla_data(iter),
				MIN(cmd_data->key.dlen, NAN_MAX_PMK_LEN));
			break;
		case NAN_ATTRIBUTE_RSSI_THRESHOLD_FLAG:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u8(iter) == 1) {
				cmd_data->flags |=
					WL_NAN_RANGE_LIMITED;
				break;
			}
			break;
		case NAN_ATTRIBUTE_DISC_IND_CFG:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->disc_ind_cfg = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_SDEA_SERVICE_SPECIFIC_INFO_LEN:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->sde_svc_info.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->sde_svc_info.dlen = nla_get_u16(iter);
			if (cmd_data->sde_svc_info.dlen > MAX_SDEA_SVC_INFO_LEN) {
				ret = -EINVAL;
				WL_ERR_RLMT(("Not allowed beyond %d\n", MAX_SDEA_SVC_INFO_LEN));
				goto exit;
			}
			break;
		case NAN_ATTRIBUTE_SDEA_SERVICE_SPECIFIC_INFO:
			if ((!cmd_data->sde_svc_info.dlen) ||
			    (nla_len(iter) != cmd_data->sde_svc_info.dlen)) {
				WL_ERR(("wrong sdea info len:%d,%d\n",
					cmd_data->sde_svc_info.dlen, nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->sde_svc_info.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->sde_svc_info.data = MALLOCZ(cfg->osh,
				cmd_data->sde_svc_info.dlen);
			if (cmd_data->sde_svc_info.data == NULL) {
				WL_ERR(("failed to allocate svc info data, len=%d\n",
					cmd_data->sde_svc_info.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->sde_svc_info.data,
				nla_data(iter), cmd_data->sde_svc_info.dlen);
			break;
		case NAN_ATTRIBUTE_SECURITY:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ndp_cfg.security_cfg = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_RANGING_INTERVAL:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ranging_intvl_msec = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_RANGING_INGRESS_LIMIT:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ingress_limit = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_RANGING_EGRESS_LIMIT:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->egress_limit = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_RANGING_INDICATION:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->ranging_indication = nla_get_u32(iter);
			break;
		/* Nan accept policy: Per service basis policy
		 * Based on this policy(ALL/NONE), responder side
		 * will send ACCEPT/REJECT
		 */
		case NAN_ATTRIBUTE_SVC_RESPONDER_POLICY:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->service_responder_policy = nla_get_u8(iter);
			break;
		default:
			WL_ERR(("Unknown type, %d\n", attr_type));
			ret = -EINVAL;
			goto exit;
		}
	}
exit:
	/* We need to call set_config_handler b/f calling start enable TBD */
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_parse_args(struct wiphy *wiphy, const void *buf,
	int len, nan_config_cmd_data_t *cmd_data, uint32 *nan_attr_mask)
{
	int ret = BCME_OK;
	int attr_type;
	int rem = len;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int chan;
	u8 sid_beacon = 0, sub_sid_beacon = 0;

	NAN_DBG_ENTER();

	nla_for_each_attr(iter, buf, len, rem) {
		attr_type = nla_type(iter);
		WL_TRACE(("attr: %s (%u)\n", nan_attr_to_str(attr_type), attr_type));

		switch (attr_type) {
		/* NAN Enable request attributes */
		case NAN_ATTRIBUTE_2G_SUPPORT:{
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->support_2g = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_SUPPORT_2G_CONFIG;
			break;
		}
		case NAN_ATTRIBUTE_5G_SUPPORT:{
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->support_5g = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_SUPPORT_5G_CONFIG;
			break;
		}
		case NAN_ATTRIBUTE_CLUSTER_LOW: {
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->clus_id.octet[5] = nla_get_u16(iter);
			break;
		}
		case NAN_ATTRIBUTE_CLUSTER_HIGH: {
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->clus_id.octet[4] = nla_get_u16(iter);
			break;
		}
		case NAN_ATTRIBUTE_SID_BEACON: {
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			sid_beacon = nla_get_u8(iter);
			cmd_data->sid_beacon.sid_enable = (sid_beacon & 0x01);
			if (cmd_data->sid_beacon.sid_enable) {
				cmd_data->sid_beacon.sid_count = (sid_beacon >> 1);
				*nan_attr_mask |= NAN_ATTR_SID_BEACON_CONFIG;
			}
			break;
		}
		case NAN_ATTRIBUTE_SUB_SID_BEACON: {
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			sub_sid_beacon = nla_get_u8(iter);
			cmd_data->sid_beacon.sub_sid_enable = (sub_sid_beacon & 0x01);
			if (cmd_data->sid_beacon.sub_sid_enable) {
				cmd_data->sid_beacon.sub_sid_count = (sub_sid_beacon >> 1);
				*nan_attr_mask |= NAN_ATTR_SUB_SID_BEACON_CONFIG;
			}
			break;
		}
		case NAN_ATTRIBUTE_SYNC_DISC_2G_BEACON:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->beacon_2g_val = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_SYNC_DISC_2G_BEACON_CONFIG;
			break;
		case NAN_ATTRIBUTE_SYNC_DISC_5G_BEACON:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->beacon_5g_val = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_SYNC_DISC_5G_BEACON_CONFIG;
			break;
		case NAN_ATTRIBUTE_SDF_2G_SUPPORT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->sdf_2g_val = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_SDF_2G_SUPPORT_CONFIG;
			break;
		case NAN_ATTRIBUTE_SDF_5G_SUPPORT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->sdf_5g_val = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_SDF_5G_SUPPORT_CONFIG;
			break;
		case NAN_ATTRIBUTE_HOP_COUNT_LIMIT:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->hop_count_limit = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_HOP_COUNT_LIMIT_CONFIG;
			break;
		case NAN_ATTRIBUTE_RANDOM_TIME:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->metrics.random_factor = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_RAND_FACTOR_CONFIG;
			break;
		case NAN_ATTRIBUTE_MASTER_PREF:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->metrics.master_pref = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_OUI:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->nan_oui = nla_get_u32(iter);
			*nan_attr_mask |= NAN_ATTR_OUI_CONFIG;
			WL_TRACE(("nan_oui=%d\n", cmd_data->nan_oui));
			break;
		case NAN_ATTRIBUTE_WARMUP_TIME:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->warmup_time = nla_get_u16(iter);
			break;
		case NAN_ATTRIBUTE_AMBTT:
		case NAN_ATTRIBUTE_MASTER_RANK:
			WL_DBG(("Unhandled attribute, %d\n", attr_type));
			break;
		case NAN_ATTRIBUTE_CHANNEL: {
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			/* take the default channel start_factor frequency */
			chan = wf_mhz2channel((uint)nla_get_u32(iter), 0);
			if (chan <= CH_MAX_2G_CHANNEL) {
				cmd_data->chanspec[0] = wf_channel2chspec(chan, WL_CHANSPEC_BW_20);
			} else {
				cmd_data->chanspec[0] = wf_channel2chspec(chan, WL_CHANSPEC_BW_80);
			}
			if (cmd_data->chanspec[0] == 0) {
				WL_ERR(("Channel is not valid \n"));
				ret = -EINVAL;
				goto exit;
			}
			WL_TRACE(("valid chanspec, chanspec = 0x%04x \n",
				cmd_data->chanspec[0]));
			break;
		}
		case NAN_ATTRIBUTE_24G_CHANNEL: {
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			/* take the default channel start_factor frequency */
			chan = wf_mhz2channel((uint)nla_get_u32(iter), 0);
			/* 20MHz as BW */
			cmd_data->chanspec[1] = wf_channel2chspec(chan, WL_CHANSPEC_BW_20);
			if (cmd_data->chanspec[1] == 0) {
				WL_ERR((" 2.4GHz Channel is not valid \n"));
				ret = -EINVAL;
				break;
			}
			*nan_attr_mask |= NAN_ATTR_2G_CHAN_CONFIG;
			WL_TRACE(("valid 2.4GHz chanspec, chanspec = 0x%04x \n",
				cmd_data->chanspec[1]));
			break;
		}
		case NAN_ATTRIBUTE_5G_CHANNEL: {
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			/* take the default channel start_factor frequency */
			chan = wf_mhz2channel((uint)nla_get_u32(iter), 0);
			/* 20MHz as BW */
			cmd_data->chanspec[2] = wf_channel2chspec(chan, WL_CHANSPEC_BW_20);
			if (cmd_data->chanspec[2] == 0) {
				WL_ERR((" 5GHz Channel is not valid \n"));
				ret = -EINVAL;
				break;
			}
			*nan_attr_mask |= NAN_ATTR_5G_CHAN_CONFIG;
			WL_TRACE(("valid 5GHz chanspec, chanspec = 0x%04x \n",
				cmd_data->chanspec[2]));
			break;
		}
		case NAN_ATTRIBUTE_CONF_CLUSTER_VAL:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->config_cluster_val = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_CLUSTER_VAL_CONFIG;
			break;
		case NAN_ATTRIBUTE_DWELL_TIME:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->dwell_time[0] = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_2G_DWELL_TIME_CONFIG;
			break;
		case NAN_ATTRIBUTE_SCAN_PERIOD:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->scan_period[0] = nla_get_u16(iter);
			*nan_attr_mask |= NAN_ATTR_2G_SCAN_PERIOD_CONFIG;
			break;
		case NAN_ATTRIBUTE_DWELL_TIME_5G:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->dwell_time[1] = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_5G_DWELL_TIME_CONFIG;
			break;
		case NAN_ATTRIBUTE_SCAN_PERIOD_5G:
			if (nla_len(iter) != sizeof(uint16)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->scan_period[1] = nla_get_u16(iter);
			*nan_attr_mask |= NAN_ATTR_5G_SCAN_PERIOD_CONFIG;
			break;
		case NAN_ATTRIBUTE_AVAIL_BIT_MAP:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->bmap = nla_get_u32(iter);
			break;
		case NAN_ATTRIBUTE_ENTRY_CONTROL:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->avail_params.duration = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_RSSI_CLOSE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_close_2dot4g_val = nla_get_s8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_CLOSE_CONFIG;
			break;
		case NAN_ATTRIBUTE_RSSI_MIDDLE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_middle_2dot4g_val = nla_get_s8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_MIDDLE_2G_CONFIG;
			break;
		case NAN_ATTRIBUTE_RSSI_PROXIMITY:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_proximity_2dot4g_val = nla_get_s8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_PROXIMITY_2G_CONFIG;
			break;
		case NAN_ATTRIBUTE_RSSI_CLOSE_5G:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_close_5g_val = nla_get_s8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_CLOSE_5G_CONFIG;
			break;
		case NAN_ATTRIBUTE_RSSI_MIDDLE_5G:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_middle_5g_val = nla_get_s8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_MIDDLE_5G_CONFIG;
			break;
		case NAN_ATTRIBUTE_RSSI_PROXIMITY_5G:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_proximity_5g_val = nla_get_s8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_PROXIMITY_5G_CONFIG;
			break;
		case NAN_ATTRIBUTE_RSSI_WINDOW_SIZE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->rssi_attr.rssi_window_size = nla_get_u8(iter);
			*nan_attr_mask |= NAN_ATTR_RSSI_WINDOW_SIZE_CONFIG;
			break;
		case NAN_ATTRIBUTE_CIPHER_SUITE_TYPE:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->csid = nla_get_u8(iter);
			WL_TRACE(("CSID = %u\n", cmd_data->csid));
			break;
		case NAN_ATTRIBUTE_SCID_LEN:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->scid.dlen) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->scid.dlen = nla_get_u32(iter);
			if (cmd_data->scid.dlen > MAX_SCID_LEN) {
				ret = -EINVAL;
				WL_ERR_RLMT(("Not allowed beyond %d\n", MAX_SCID_LEN));
				goto exit;
			}
			WL_TRACE(("valid scid length = %u\n", cmd_data->scid.dlen));
			break;
		case NAN_ATTRIBUTE_SCID:
			if (!cmd_data->scid.dlen || (nla_len(iter) != cmd_data->scid.dlen)) {
				WL_ERR(("wrong scid len:%d,%d\n", cmd_data->scid.dlen,
					nla_len(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (cmd_data->scid.data) {
				WL_ERR(("trying to overwrite:%d\n", attr_type));
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->scid.data = MALLOCZ(cfg->osh, cmd_data->scid.dlen);
			if (cmd_data->scid.data == NULL) {
				WL_ERR(("failed to allocate scid, len=%d\n",
					cmd_data->scid.dlen));
				ret = -ENOMEM;
				goto exit;
			}
			memcpy(cmd_data->scid.data, nla_data(iter), cmd_data->scid.dlen);
			break;
		case NAN_ATTRIBUTE_2G_AWAKE_DW:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u32(iter) > NAN_MAX_AWAKE_DW_INTERVAL) {
				WL_ERR(("%s: Invalid/Out of bound value = %u\n",
						__FUNCTION__, nla_get_u32(iter)));
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u32(iter)) {
				cmd_data->awake_dws.dw_interval_2g =
					1 << (nla_get_u32(iter)-1);
			}
			*nan_attr_mask |= NAN_ATTR_2G_DW_CONFIG;
			break;
		case NAN_ATTRIBUTE_5G_AWAKE_DW:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			if (nla_get_u32(iter) > NAN_MAX_AWAKE_DW_INTERVAL) {
				WL_ERR(("%s: Invalid/Out of bound value = %u\n",
						__FUNCTION__, nla_get_u32(iter)));
				ret = BCME_BADARG;
				break;
			}
			if (nla_get_u32(iter)) {
				cmd_data->awake_dws.dw_interval_5g =
					1 << (nla_get_u32(iter)-1);
			}
			*nan_attr_mask |= NAN_ATTR_5G_DW_CONFIG;
			break;
		case NAN_ATTRIBUTE_DISC_IND_CFG:
			if (nla_len(iter) != sizeof(uint8)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->disc_ind_cfg = nla_get_u8(iter);
			break;
		case NAN_ATTRIBUTE_MAC_ADDR:
			if (nla_len(iter) != ETHER_ADDR_LEN) {
				ret = -EINVAL;
				goto exit;
			}
			memcpy((char*)&cmd_data->mac_addr, (char*)nla_data(iter),
				ETHER_ADDR_LEN);
			break;
		case NAN_ATTRIBUTE_RANDOMIZATION_INTERVAL:
			if (nla_len(iter) != sizeof(uint32)) {
				ret = -EINVAL;
				goto exit;
			}
			cmd_data->nmi_rand_intvl = nla_get_u8(iter);
			if (cmd_data->nmi_rand_intvl > 0) {
				cfg->nancfg.mac_rand = true;
			} else {
				cfg->nancfg.mac_rand = false;
			}
			break;
		default:
			WL_ERR(("%s: Unknown type, %d\n", __FUNCTION__, attr_type));
			ret = -EINVAL;
			goto exit;
		}
	}

exit:
	/* We need to call set_config_handler b/f calling start enable TBD */
	NAN_DBG_EXIT();
	if (ret) {
		WL_ERR(("%s: Failed to parse attribute %d ret %d",
			__FUNCTION__, attr_type, ret));
	}
	return ret;

}

static int
wl_cfgvendor_nan_dp_estb_event_data_filler(struct sk_buff *msg,
	nan_event_data_t *event_data) {
	int ret = BCME_OK;
	ret = nla_put_u32(msg, NAN_ATTRIBUTE_NDP_ID, event_data->ndp_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put NDP ID, ret=%d\n", ret));
		goto fail;
	}
	/*
	 * NDI mac address of the peer
	 * (required to derive target ipv6 address)
	 */
	ret = nla_put(msg, NAN_ATTRIBUTE_PEER_NDI_MAC_ADDR, ETH_ALEN,
			event_data->responder_ndi.octet);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put resp ndi, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u8(msg, NAN_ATTRIBUTE_RSP_CODE, event_data->status);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put response code, ret=%d\n", ret));
		goto fail;
	}
	if (event_data->svc_info.dlen && event_data->svc_info.data) {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO_LEN,
				event_data->svc_info.dlen);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put svc info len, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put(msg, NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO,
				event_data->svc_info.dlen, event_data->svc_info.data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put svc info, ret=%d\n", ret));
			goto fail;
		}
	}

fail:
	return ret;
}
static int
wl_cfgvendor_nan_dp_ind_event_data_filler(struct sk_buff *msg,
		nan_event_data_t *event_data) {
	int ret = BCME_OK;

	ret = nla_put_u16(msg, NAN_ATTRIBUTE_PUBLISH_ID,
			event_data->pub_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put pub ID, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u32(msg, NAN_ATTRIBUTE_NDP_ID, event_data->ndp_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put NDP ID, ret=%d\n", ret));
		goto fail;
	}
	/* Discovery MAC addr of the peer/initiator */
	ret = nla_put(msg, NAN_ATTRIBUTE_MAC_ADDR, ETH_ALEN,
			event_data->remote_nmi.octet);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put remote NMI, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u8(msg, NAN_ATTRIBUTE_SECURITY, event_data->security);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put security, ret=%d\n", ret));
		goto fail;
	}
	if (event_data->svc_info.dlen && event_data->svc_info.data) {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO_LEN,
				event_data->svc_info.dlen);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put svc info len, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put(msg, NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO,
				event_data->svc_info.dlen, event_data->svc_info.data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put svc info, ret=%d\n", ret));
			goto fail;
		}
	}

fail:
	return ret;
}

static int
wl_cfgvendor_nan_tx_followup_ind_event_data_filler(struct sk_buff *msg,
	nan_event_data_t *event_data) {
	int ret = BCME_OK;
	ret = nla_put_u16(msg, NAN_ATTRIBUTE_TRANSAC_ID, event_data->token);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put transaction id, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u8(msg, NAN_ATTRIBUTE_HANDLE, event_data->local_inst_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put handle, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u16(msg, NAN_ATTRIBUTE_STATUS, event_data->status);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put nan status, ret=%d\n", ret));
		goto fail;
	}
	if (event_data->status == NAN_STATUS_SUCCESS) {
		ret = nla_put(msg, NAN_ATTRIBUTE_REASON,
				strlen("NAN_STATUS_SUCCESS"), event_data->nan_reason);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put nan reason, ret=%d\n", ret));
			goto fail;
		}
	} else {
		ret = nla_put(msg, NAN_ATTRIBUTE_REASON,
				strlen("NAN_STATUS_NO_OTA_ACK"), event_data->nan_reason);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put nan reason, ret=%d\n", ret));
			goto fail;
		}
	}
fail:
	return ret;
}

static int
wl_cfgvendor_nan_svc_terminate_event_filler(struct sk_buff *msg,
	struct bcm_cfg80211 *cfg, int event_id, nan_event_data_t *event_data) {
	int ret = BCME_OK;
	ret = nla_put_u8(msg, NAN_ATTRIBUTE_HANDLE, event_data->local_inst_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put handle, ret=%d\n", ret));
		goto fail;
	}

	if (event_id == GOOGLE_NAN_EVENT_SUBSCRIBE_TERMINATED) {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_SUBSCRIBE_ID,
				event_data->local_inst_id);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put local inst id, ret=%d\n", ret));
			goto fail;
		}
	} else {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_PUBLISH_ID,
				event_data->local_inst_id);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put local inst id, ret=%d\n", ret));
			goto fail;
		}
	}
	ret = nla_put_u16(msg, NAN_ATTRIBUTE_STATUS, event_data->status);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put status, ret=%d\n", ret));
		goto fail;
	}
	if (event_data->status == NAN_STATUS_SUCCESS) {
		ret = nla_put(msg, NAN_ATTRIBUTE_REASON,
				strlen("NAN_STATUS_SUCCESS"), event_data->nan_reason);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put nan reason, ret=%d\n", ret));
			goto fail;
		}
	} else {
		ret = nla_put(msg, NAN_ATTRIBUTE_REASON,
				strlen("NAN_STATUS_INTERNAL_FAILURE"), event_data->nan_reason);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put nan reason, ret=%d\n", ret));
			goto fail;
		}
	}

	ret = wl_cfgnan_remove_inst_id(cfg, event_data->local_inst_id);
	if (ret) {
		WL_ERR(("failed to free svc instance-id[%d], ret=%d, event_id = %d\n",
				event_data->local_inst_id, ret, event_id));
		goto fail;
	}
fail:
	return ret;
}

static int
wl_cfgvendor_nan_opt_params_filler(struct sk_buff *msg,
	nan_event_data_t *event_data) {
	int ret = BCME_OK;
	/* service specific info data */
	if (event_data->svc_info.dlen && event_data->svc_info.data) {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO_LEN,
				event_data->svc_info.dlen);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put svc info len, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put(msg, NAN_ATTRIBUTE_SERVICE_SPECIFIC_INFO,
				event_data->svc_info.dlen, event_data->svc_info.data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put svc info, ret=%d\n", ret));
			goto fail;
		}
		WL_TRACE(("svc info len = %d\n", event_data->svc_info.dlen));
	}

	/* sdea service specific info data */
	if (event_data->sde_svc_info.dlen && event_data->sde_svc_info.data) {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_SDEA_SERVICE_SPECIFIC_INFO_LEN,
				event_data->sde_svc_info.dlen);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put sdea svc info len, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put(msg, NAN_ATTRIBUTE_SDEA_SERVICE_SPECIFIC_INFO,
				event_data->sde_svc_info.dlen,
				event_data->sde_svc_info.data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put sdea svc info, ret=%d\n", ret));
			goto fail;
		}
		WL_TRACE(("sdea svc info len = %d\n", event_data->sde_svc_info.dlen));
	}
	/* service control discovery range limit */
	/* TODO: */

	/* service control binding bitmap */
	/* TODO: */
fail:
	return ret;
}

static int
wl_cfgvendor_nan_tx_followup_event_filler(struct sk_buff *msg,
		nan_event_data_t *event_data) {
	int ret = BCME_OK;
	/* In followup pkt, instance id and requestor instance id are configured
	 * from the transmitter perspective. As the event is processed with the
	 * role of receiver, the local handle should use requestor instance
	 * id (peer_inst_id)
	 */
	WL_TRACE(("handle=%d\n", event_data->requestor_id));
	WL_TRACE(("inst id (local id)=%d\n", event_data->local_inst_id));
	WL_TRACE(("peer id (remote id)=%d\n", event_data->requestor_id));
	WL_TRACE(("peer mac addr=" MACDBG "\n",
			MAC2STRDBG(event_data->remote_nmi.octet)));
	WL_TRACE(("peer rssi: %d\n", event_data->fup_rssi));
	WL_TRACE(("attribute no: %d\n", event_data->attr_num));
	WL_TRACE(("attribute len: %d\n", event_data->attr_list_len));

	ret = nla_put_u8(msg, NAN_ATTRIBUTE_HANDLE, event_data->requestor_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put handle, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u32(msg, NAN_ATTRIBUTE_INST_ID, event_data->local_inst_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put local inst id, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u16(msg, NAN_ATTRIBUTE_PEER_ID, event_data->requestor_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put requestor inst id, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put(msg, NAN_ATTRIBUTE_MAC_ADDR, ETHER_ADDR_LEN,
			event_data->remote_nmi.octet);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put remote nmi, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_s8(msg, NAN_ATTRIBUTE_RSSI_PROXIMITY,
			event_data->fup_rssi);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put fup rssi, ret=%d\n", ret));
		goto fail;
	}
fail:
	return ret;
}

static int
wl_cfgvendor_nan_sub_match_event_filler(struct sk_buff *msg,
	nan_event_data_t *event_data) {
	int ret = BCME_OK;
	WL_TRACE(("handle (sub_id)=%d\n", event_data->sub_id));
	WL_TRACE(("pub id=%d\n", event_data->pub_id));
	WL_TRACE(("sub id=%d\n", event_data->sub_id));
	WL_TRACE(("pub mac addr=" MACDBG "\n",
			MAC2STRDBG(event_data->remote_nmi.octet)));
	WL_TRACE(("attr no: %d\n", event_data->attr_num));
	WL_TRACE(("attr len: %d\n", event_data->attr_list_len));

	ret = nla_put_u8(msg, NAN_ATTRIBUTE_HANDLE, event_data->sub_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put handle, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u16(msg, NAN_ATTRIBUTE_PUBLISH_ID, event_data->pub_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put pub id, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u16(msg, NAN_ATTRIBUTE_SUBSCRIBE_ID, event_data->sub_id);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put Sub Id, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put(msg, NAN_ATTRIBUTE_MAC_ADDR, ETHER_ADDR_LEN,
			event_data->remote_nmi.octet);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put remote NMI, ret=%d\n", ret));
		goto fail;
	}
	if (event_data->publish_rssi) {
		event_data->publish_rssi = -event_data->publish_rssi;
		ret = nla_put_u8(msg, NAN_ATTRIBUTE_RSSI_PROXIMITY,
				event_data->publish_rssi);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put publish rssi, ret=%d\n", ret));
			goto fail;
		}
	}
	if (event_data->ranging_result_present) {
		ret = nla_put_u32(msg, NAN_ATTRIBUTE_RANGING_INDICATION,
				event_data->ranging_ind);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put ranging ind, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put_u32(msg, NAN_ATTRIBUTE_RANGING_RESULT,
				event_data->range_measurement_cm);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put range measurement cm, ret=%d\n",
					ret));
			goto fail;
		}
	}
	/*
	 * handling optional service control, service response filter
	 */
	if (event_data->tx_match_filter.dlen && event_data->tx_match_filter.data) {
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_TX_MATCH_FILTER_LEN,
				event_data->tx_match_filter.dlen);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put tx match filter len, ret=%d\n",
					ret));
			goto fail;
		}
		ret = nla_put(msg, NAN_ATTRIBUTE_TX_MATCH_FILTER,
				event_data->tx_match_filter.dlen,
				event_data->tx_match_filter.data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put tx match filter data, ret=%d\n",
					ret));
			goto fail;
		}
		WL_TRACE(("tx matching filter (%d):\n",
				event_data->tx_match_filter.dlen));
	}

fail:
	return ret;
}

static int
wl_cfgvendor_nan_de_event_filler(struct sk_buff *msg, nan_event_data_t *event_data)
{
	int ret = BCME_OK;
	ret = nla_put_u8(msg, NAN_ATTRIBUTE_ENABLE_STATUS, event_data->enabled);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put event_data->enabled, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put_u8(msg, NAN_ATTRIBUTE_DE_EVENT_TYPE,
			event_data->nan_de_evt_type);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put nan_de_evt_type, ret=%d\n", ret));
		goto fail;
	}
	ret = nla_put(msg, NAN_ATTRIBUTE_CLUSTER_ID, ETH_ALEN,
			event_data->clus_id.octet);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put clust id, ret=%d\n", ret));
		goto fail;
	}
	/* OOB tests requires local nmi */
	ret = nla_put(msg, NAN_ATTRIBUTE_MAC_ADDR, ETH_ALEN,
			event_data->local_nmi.octet);
	if (unlikely(ret)) {
		WL_ERR(("Failed to put NMI, ret=%d\n", ret));
		goto fail;
	}
fail:
	return ret;
}

int
wl_cfgvendor_send_nan_event(struct wiphy *wiphy, struct net_device *dev,
	int event_id, nan_event_data_t *event_data)
{
	int ret = BCME_OK;
	int buf_len = NAN_EVENT_BUFFER_SIZE_LARGE;
	gfp_t kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;

	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	struct sk_buff *msg;

	NAN_DBG_ENTER();

	/* Allocate the skb for vendor event */
#if (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	msg = cfg80211_vendor_event_alloc(wiphy, ndev_to_wdev(dev), buf_len, event_id, kflags);
#else
	msg = cfg80211_vendor_event_alloc(wiphy, buf_len, event_id, kflags);
#endif /* (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || */
	/* LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0) */

	if (!msg) {
		WL_ERR(("%s: fail to allocate skb for vendor event\n", __FUNCTION__));
		return -ENOMEM;
	}

	switch (event_id) {
	case GOOGLE_NAN_EVENT_DE_EVENT: {
		WL_INFORM_MEM(("[NAN] GOOGLE_NAN_DE_EVENT cluster id=" MACDBG "nmi= " MACDBG "\n",
			MAC2STRDBG(event_data->clus_id.octet),
			MAC2STRDBG(event_data->local_nmi.octet)));
		ret = wl_cfgvendor_nan_de_event_filler(msg, event_data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to fill de event data, ret=%d\n", ret));
			goto fail;
		}
		break;
	}
	case GOOGLE_NAN_EVENT_SUBSCRIBE_MATCH:
	case GOOGLE_NAN_EVENT_FOLLOWUP: {
		if (event_id == GOOGLE_NAN_EVENT_SUBSCRIBE_MATCH) {
			WL_DBG(("GOOGLE_NAN_EVENT_SUBSCRIBE_MATCH\n"));
			ret = wl_cfgvendor_nan_sub_match_event_filler(msg, event_data);
			if (unlikely(ret)) {
				WL_ERR(("Failed to fill sub match event data, ret=%d\n", ret));
				goto fail;
			}
		} else if (event_id == GOOGLE_NAN_EVENT_FOLLOWUP) {
			WL_DBG(("GOOGLE_NAN_EVENT_FOLLOWUP\n"));
			ret = wl_cfgvendor_nan_tx_followup_event_filler(msg, event_data);
			if (unlikely(ret)) {
				WL_ERR(("Failed to fill sub match event data, ret=%d\n", ret));
				goto fail;
			}
		}
		ret = wl_cfgvendor_nan_opt_params_filler(msg, event_data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to fill sub match event data, ret=%d\n", ret));
			goto fail;
		}
		break;
	}

	case GOOGLE_NAN_EVENT_DISABLED: {
		WL_INFORM_MEM(("[NAN] GOOGLE_NAN_EVENT_DISABLED\n"));
		ret = nla_put_u8(msg, NAN_ATTRIBUTE_HANDLE, 0);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put handle, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put_u16(msg, NAN_ATTRIBUTE_STATUS, event_data->status);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put status, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put(msg, NAN_ATTRIBUTE_REASON,
			strlen("NAN_STATUS_SUCCESS"), event_data->nan_reason);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put reason code, ret=%d\n", ret));
			goto fail;
		}
		break;
	}

	case GOOGLE_NAN_EVENT_SUBSCRIBE_TERMINATED:
	case GOOGLE_NAN_EVENT_PUBLISH_TERMINATED: {
		WL_DBG(("GOOGLE_NAN_SVC_TERMINATED, %d\n", event_id));
		ret = wl_cfgvendor_nan_svc_terminate_event_filler(msg, cfg, event_id, event_data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to fill svc terminate event data, ret=%d\n", ret));
			goto fail;
		}
		break;
	}

	case GOOGLE_NAN_EVENT_TRANSMIT_FOLLOWUP_IND: {
		WL_DBG(("GOOGLE_NAN_EVENT_TRANSMIT_FOLLOWUP_IND %d\n",
			GOOGLE_NAN_EVENT_TRANSMIT_FOLLOWUP_IND));
		ret = wl_cfgvendor_nan_tx_followup_ind_event_data_filler(msg, event_data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to fill tx follow up ind event data, ret=%d\n", ret));
			goto fail;
		}

		break;
	}

	case GOOGLE_NAN_EVENT_DATA_REQUEST: {
		WL_INFORM_MEM(("[NAN] GOOGLE_NAN_EVENT_DATA_REQUEST\n"));
		ret = wl_cfgvendor_nan_dp_ind_event_data_filler(msg, event_data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to fill dp ind event data, ret=%d\n", ret));
			goto fail;
		}
		break;
	}

	case GOOGLE_NAN_EVENT_DATA_CONFIRMATION: {
		WL_INFORM_MEM(("[NAN] GOOGLE_NAN_EVENT_DATA_CONFIRMATION\n"));

		ret = wl_cfgvendor_nan_dp_estb_event_data_filler(msg, event_data);
		if (unlikely(ret)) {
			WL_ERR(("Failed to fill dp estb event data, ret=%d\n", ret));
			goto fail;
		}
		break;
	}

	case GOOGLE_NAN_EVENT_DATA_END: {
		WL_INFORM_MEM(("[NAN] GOOGLE_NAN_EVENT_DATA_END\n"));
		ret = nla_put_u8(msg, NAN_ATTRIBUTE_INST_COUNT, 1);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put inst count, ret=%d\n", ret));
			goto fail;
		}
		ret = nla_put_u32(msg, NAN_ATTRIBUTE_NDP_ID, event_data->ndp_id);
		if (unlikely(ret)) {
			WL_ERR(("Failed to put ndp id, ret=%d\n", ret));
			goto fail;
		}
		break;
	}

	default:
		goto fail;
	}

	cfg80211_vendor_event(msg, kflags);
	NAN_DBG_EXIT();
	return ret;

fail:
	dev_kfree_skb_any(msg);
	WL_ERR(("Event not implemented or unknown -- Free skb, event_id = %d, ret = %d\n",
			event_id, ret));
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_req_subscribe(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_discover_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	NAN_DBG_ENTER();
	/* Blocking Subscribe if NAN is not enable */
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, subscribe blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}
	cmd_data = (nan_discover_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgvendor_nan_parse_discover_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan disc vendor args, ret = %d\n", ret));
		goto exit;
	}

	if (cmd_data->sub_id == 0) {
		ret = wl_cfgnan_generate_inst_id(cfg, &cmd_data->sub_id);
		if (ret) {
			WL_ERR(("failed to generate instance-id for subscribe\n"));
			goto exit;
		}
	} else {
		cmd_data->svc_update = true;
	}

	ret = wl_cfgnan_subscribe_handler(wdev->netdev, cfg, cmd_data);
	if (unlikely(ret) || unlikely(cmd_data->status)) {
		WL_ERR(("failed to subscribe error[%d], status = [%d]\n",
				ret, cmd_data->status));
		wl_cfgnan_remove_inst_id(cfg, cmd_data->sub_id);
		goto exit;
	}

	WL_DBG(("subscriber instance id=%d\n", cmd_data->sub_id));

	if (cmd_data->status == WL_NAN_E_OK) {
		nan_req_resp.instance_id = cmd_data->sub_id;
	} else {
		nan_req_resp.instance_id = 0;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_REQUEST_SUBSCRIBE,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_disc_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_req_publish(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_discover_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	NAN_DBG_ENTER();

	/* Blocking Publish if NAN is not enable */
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled publish blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}
	cmd_data = (nan_discover_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgvendor_nan_parse_discover_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan disc vendor args, ret = %d\n", ret));
		goto exit;
	}

	if (cmd_data->pub_id == 0) {
		ret = wl_cfgnan_generate_inst_id(cfg, &cmd_data->pub_id);
		if (ret) {
			WL_ERR(("failed to generate instance-id for publisher\n"));
			goto exit;
		}
	} else {
		cmd_data->svc_update = true;
	}

	ret = wl_cfgnan_publish_handler(wdev->netdev, cfg, cmd_data);
	if (unlikely(ret) || unlikely(cmd_data->status)) {
		WL_ERR(("failed to publish error[%d], status[%d]\n",
				ret, cmd_data->status));
		wl_cfgnan_remove_inst_id(cfg, cmd_data->pub_id);
		goto exit;
	}

	WL_DBG(("publisher instance id=%d\n", cmd_data->pub_id));

	if (cmd_data->status == WL_NAN_E_OK) {
		nan_req_resp.instance_id = cmd_data->pub_id;
	} else {
		nan_req_resp.instance_id = 0;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_REQUEST_PUBLISH,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_disc_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_start_handler(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = 0;
	nan_config_cmd_data_t *cmd_data;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	uint32 nan_attr_mask = 0;

	cmd_data = (nan_config_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	NAN_DBG_ENTER();

	if (cfg->nan_enable) {
		WL_ERR(("nan is already enabled\n"));
		ret = BCME_OK;
		goto exit;
	}

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	cmd_data->sid_beacon.sid_enable = NAN_SID_ENABLE_FLAG_INVALID; /* Setting to some default */
	cmd_data->sid_beacon.sid_count = NAN_SID_BEACON_COUNT_INVALID; /* Setting to some default */

	ret = wl_cfgvendor_nan_parse_args(wiphy, data, len, cmd_data, &nan_attr_mask);
	if (ret) {
		WL_ERR(("failed to parse nan vendor args, ret %d\n", ret));
		goto exit;
	}

	ret = wl_cfgnan_start_handler(wdev->netdev, cfg, cmd_data, nan_attr_mask);
	if (ret) {
		WL_ERR(("failed to start nan error[%d]\n", ret));
		goto exit;
	}
	/* Initializing Instance Id List */
	memset(cfg->nan_inst_ctrl, 0, NAN_ID_CTRL_SIZE * sizeof(nan_svc_inst_t));
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_ENABLE,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	if (cmd_data) {
		if (cmd_data->scid.data) {
			MFREE(cfg->osh, cmd_data->scid.data, cmd_data->scid.dlen);
			cmd_data->scid.dlen = 0;
		}
		MFREE(cfg->osh, cmd_data, sizeof(*cmd_data));
	}
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_stop_handler(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	NAN_DBG_ENTER();

	if (!cfg->nan_init_state) {
		WL_ERR(("nan is not initialized/nmi doesnt exists\n"));
		ret = BCME_OK;
		goto exit;
	}

	if (cfg->nan_enable) {
		ret = wl_cfgnan_disable(cfg, NAN_USER_INITIATED);
		if (ret) {
			WL_ERR(("failed to disable nan, error[%d]\n", ret));
		}
	}
	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DISABLE,
		&nan_req_resp, ret, BCME_OK);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_config_handler(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = 0;
	nan_config_cmd_data_t *cmd_data;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	uint32 nan_attr_mask = 0;

	cmd_data = MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	NAN_DBG_ENTER();
	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	cmd_data->avail_params.duration = NAN_BAND_INVALID;  /* Setting to some default */
	cmd_data->sid_beacon.sid_enable = NAN_SID_ENABLE_FLAG_INVALID; /* Setting to some default */
	cmd_data->sid_beacon.sid_count = NAN_SID_BEACON_COUNT_INVALID; /* Setting to some default */

	ret = wl_cfgvendor_nan_parse_args(wiphy, data, len, cmd_data, &nan_attr_mask);
	if (ret) {
		WL_ERR(("failed to parse nan vendor args, ret = %d\n", ret));
		goto exit;
	}

	ret = wl_cfgnan_config_handler(wdev->netdev, cfg, cmd_data, nan_attr_mask);
	if (ret) {
		WL_ERR(("failed in config request, nan error[%d]\n", ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_CONFIG,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	if (cmd_data) {
		if (cmd_data->scid.data) {
			MFREE(cfg->osh, cmd_data->scid.data, cmd_data->scid.dlen);
			cmd_data->scid.dlen = 0;
		}
		MFREE(cfg->osh, cmd_data, sizeof(*cmd_data));
	}
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_cancel_publish(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_discover_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	/* Blocking Cancel_Publish if NAN is not enable */
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, cancel publish blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}
	cmd_data = (nan_discover_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	NAN_DBG_ENTER();

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	ret = wl_cfgvendor_nan_parse_discover_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan disc vendor args, ret= %d\n", ret));
		goto exit;
	}
	nan_req_resp.instance_id = cmd_data->pub_id;
	WL_INFORM_MEM(("[NAN] cancel publish instance_id=%d\n", cmd_data->pub_id));

	ret = wl_cfgnan_cancel_pub_handler(wdev->netdev, cfg, cmd_data);
	if (ret) {
		WL_ERR(("failed to cancel publish nan instance-id[%d] error[%d]\n",
			cmd_data->pub_id, ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_CANCEL_PUBLISH,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_disc_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_cancel_subscribe(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_discover_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	/* Blocking Cancel_Subscribe if NAN is not enableb */
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, cancel subscribe blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}
	cmd_data = MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	NAN_DBG_ENTER();

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	ret = wl_cfgvendor_nan_parse_discover_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan disc vendor args, ret= %d\n", ret));
		goto exit;
	}
	nan_req_resp.instance_id = cmd_data->sub_id;
	WL_INFORM_MEM(("[NAN] cancel subscribe instance_id=%d\n", cmd_data->sub_id));

	ret = wl_cfgnan_cancel_sub_handler(wdev->netdev, cfg, cmd_data);
	if (ret) {
		WL_ERR(("failed to cancel subscribe nan instance-id[%d] error[%d]\n",
			cmd_data->sub_id, ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_CANCEL_SUBSCRIBE,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_disc_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_transmit(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_discover_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	/* Blocking Transmit if NAN is not enable */
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, transmit blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}
	cmd_data = (nan_discover_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	NAN_DBG_ENTER();

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	ret = wl_cfgvendor_nan_parse_discover_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan disc vendor args, ret= %d\n", ret));
		goto exit;
	}
	nan_req_resp.instance_id = cmd_data->local_id;
	ret = wl_cfgnan_transmit_handler(wdev->netdev, cfg, cmd_data);
	if (ret) {
		WL_ERR(("failed to transmit-followup nan error[%d]\n", ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_TRANSMIT,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_disc_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_get_capablities(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	NAN_DBG_ENTER();

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	ret = wl_cfgnan_get_capablities_handler(wdev->netdev, cfg, &nan_req_resp.capabilities);
	if (ret) {
		WL_ERR(("Could not get capabilities\n"));
		ret = -EINVAL;
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_GET_CAPABILITIES,
		&nan_req_resp, ret, BCME_OK);
	wl_cfgvendor_send_cmd_reply(wiphy, &nan_req_resp, sizeof(nan_req_resp));

	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_data_path_iface_create(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	s32 idx;
	nan_datapath_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	dhd_pub_t *dhdp = wl_cfg80211_get_dhdp(wdev->netdev);

	if (!cfg->nan_init_state) {
		WL_ERR(("%s: NAN is not inited or Device doesn't support NAN \n", __func__));
		ret = -ENODEV;
		goto exit;
	}

	cmd_data = (nan_datapath_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	NAN_DBG_ENTER();

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));

	ret = wl_cfgvendor_nan_parse_datapath_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan datapath vendor args, ret = %d\n", ret));
		goto exit;
	}

	/* Store the iface name to pub data so that it can be used
	 * during NAN enable
	 */
	if ((idx = wl_cfgnan_get_ndi_idx(cfg)) < 0) {
		WL_ERR(("No free idx for NAN NDI\n"));
		goto exit;
	}
	wl_cfgnan_add_ndi_data(cfg, idx, (char*)cmd_data->ndp_iface);
	if (cfg->nan_enable) { /* new framework Impl, iface create called after nan enab */
		wl_cfgnan_data_path_iface_create_delete_handler(wdev->netdev,
			cfg, cmd_data->ndp_iface,
			NAN_WIFI_SUBCMD_DATA_PATH_IFACE_CREATE, dhdp->up);
		cfg->nancfg.ndi[idx].created = true;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DATA_PATH_IFACE_CREATE,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_dp_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_data_path_iface_delete(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_datapath_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	dhd_pub_t *dhdp = wl_cfg80211_get_dhdp(wdev->netdev);

	if (!cfg->nan_init_state) {
		WL_ERR(("%s: NAN is not inited or Device doesn't support NAN \n", __func__));
		/* Deinit has taken care of cleaing the virtual iface */
		ret = BCME_OK;
		goto exit;
	}

	NAN_DBG_ENTER();
	cmd_data = (nan_datapath_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}
	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgvendor_nan_parse_datapath_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan datapath vendor args, ret = %d\n", ret));
		goto exit;
	}

	ret = wl_cfgnan_data_path_iface_create_delete_handler(wdev->netdev, cfg,
		(char*)cmd_data->ndp_iface,
		NAN_WIFI_SUBCMD_DATA_PATH_IFACE_DELETE, dhdp->up);
	if (ret) {
		if (ret == -ENODEV) {
			if (wl_cfgnan_get_ndi_data(cfg, (char*)cmd_data->ndp_iface) != NULL) {
				/* NDIs have been removed by the NAN disable command */
				WL_DBG(("NDI removed by nan_disable\n"));
				ret = BCME_OK;
			}
		} else {
			WL_ERR(("failed to delete ndp iface [%d]\n", ret));
			goto exit;
		}
	}
exit:
	if (cfg->nan_init_state) {
		/* After successful delete of interface, clear up the ndi data */
		if (wl_cfgnan_del_ndi_data(cfg, (char*)cmd_data->ndp_iface) < 0) {
			WL_ERR(("Failed to find matching data for ndi:%s\n",
					(char*)cmd_data->ndp_iface));
		}
	}

	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DATA_PATH_IFACE_DELETE,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_dp_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_data_path_request(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_datapath_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	uint8 ndp_instance_id = 0;

	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, nan data path request blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}

	NAN_DBG_ENTER();
	cmd_data = (nan_datapath_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgvendor_nan_parse_datapath_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan datapath vendor args, ret = %d\n", ret));
		goto exit;
	}

	ret = wl_cfgnan_data_path_request_handler(wdev->netdev, cfg,
			cmd_data, &ndp_instance_id);
	if (ret) {
		WL_ERR(("failed to request nan data path [%d]\n", ret));
		goto exit;
	}

	if (cmd_data->status == BCME_OK) {
		nan_req_resp.ndp_instance_id = cmd_data->ndp_instance_id;
	} else {
		nan_req_resp.ndp_instance_id = 0;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DATA_PATH_REQUEST,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_dp_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_data_path_response(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_datapath_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, nan data path response blocked\n"));
		ret = BCME_ERROR;
		goto exit;
	}
	NAN_DBG_ENTER();
	cmd_data = (nan_datapath_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgvendor_nan_parse_datapath_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan datapath vendor args, ret = %d\n", ret));
		goto exit;
	}
	ret = wl_cfgnan_data_path_response_handler(wdev->netdev, cfg, cmd_data);
	if (ret) {
		WL_ERR(("failed to response nan data path [%d]\n", ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DATA_PATH_RESPONSE,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_dp_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

static int
wl_cfgvendor_nan_data_path_end(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void * data, int len)
{
	int ret = 0;
	nan_datapath_cmd_data_t *cmd_data = NULL;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;

	NAN_DBG_ENTER();
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled, nan data path end blocked\n"));
		ret = BCME_OK;
		goto exit;
	}
	cmd_data = (nan_datapath_cmd_data_t *)MALLOCZ(cfg->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}

	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgvendor_nan_parse_datapath_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse nan datapath vendor args, ret = %d\n", ret));
		goto exit;
	}
	ret = wl_cfgnan_data_path_end_handler(wdev->netdev, cfg, cmd_data);
	if (ret) {
		WL_ERR(("failed to end nan data path [%d]\n", ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DATA_PATH_END,
		&nan_req_resp, ret, cmd_data ? cmd_data->status : BCME_OK);
	wl_cfgvendor_free_dp_cmd_data(cfg, cmd_data);
	NAN_DBG_EXIT();
	return ret;
}

#ifdef WL_NAN_DISC_CACHE
static int
wl_cfgvendor_nan_data_path_sec_info(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	int ret = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	nan_hal_resp_t nan_req_resp;
	nan_datapath_sec_info_cmd_data_t *cmd_data = NULL;
	dhd_pub_t *dhdp = wl_cfg80211_get_dhdp(wdev->netdev);

	NAN_DBG_ENTER();
	if (!cfg->nan_enable) {
		WL_ERR(("nan is not enabled\n"));
		ret = BCME_UNSUPPORTED;
		goto exit;
	}
	cmd_data = MALLOCZ(dhdp->osh, sizeof(*cmd_data));
	if (!cmd_data) {
		WL_ERR(("%s: memory allocation failed\n", __func__));
		ret = BCME_NOMEM;
		goto exit;
	}

	ret = wl_cfgvendor_nan_parse_dp_sec_info_args(wiphy, data, len, cmd_data);
	if (ret) {
		WL_ERR(("failed to parse sec info args\n"));
		goto exit;
	}
	memset(&nan_req_resp, 0, sizeof(nan_req_resp));
	ret = wl_cfgnan_sec_info_handler(cfg, cmd_data, &nan_req_resp);
	if (ret) {
		WL_ERR(("failed to retrieve svc hash/pub nmi error[%d]\n", ret));
		goto exit;
	}
exit:
	ret = wl_cfgvendor_nan_cmd_reply(wiphy, NAN_WIFI_SUBCMD_DATA_PATH_SEC_INFO,
		&nan_req_resp, ret, BCME_OK);
	if (cmd_data) {
		MFREE(dhdp->osh, cmd_data, sizeof(*cmd_data));
	}
	NAN_DBG_EXIT();
	return ret;
}
#endif /* WL_NAN_DISC_CACHE */

static int
wl_cfgvendor_nan_version_info(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	int ret = BCME_OK;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	uint32 version = NAN_HAL_VERSION_1;

	BCM_REFERENCE(cfg);
	WL_DBG(("Enter %s version %d\n", __FUNCTION__, version));
	ret = wl_cfgvendor_send_cmd_reply(wiphy, &version, sizeof(version));
	return ret;
}

#endif /* WL_NAN */

#ifdef LINKSTAT_SUPPORT

#define NUM_RATE 32
#define NUM_PEER 1
#define NUM_CHAN 11
#define HEADER_SIZE sizeof(ver_len)

static int wl_cfgvendor_lstats_get_bcn_mbss(char *buf, uint32 *rxbeaconmbss)
{
	wl_cnt_info_t *cbuf = (wl_cnt_info_t *)buf;
	const void *cnt;

	if ((cnt = (const void *)bcm_get_data_from_xtlv_buf(cbuf->data, cbuf->datalen,
		WL_CNT_XTLV_CNTV_LE10_UCODE, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		*rxbeaconmbss = ((const wl_cnt_v_le10_mcst_t *)cnt)->rxbeaconmbss;
	} else if ((cnt = (const void *)bcm_get_data_from_xtlv_buf(cbuf->data, cbuf->datalen,
		WL_CNT_XTLV_LT40_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		*rxbeaconmbss = ((const wl_cnt_lt40mcst_v1_t *)cnt)->rxbeaconmbss;
	} else if ((cnt = (const void *)bcm_get_data_from_xtlv_buf(cbuf->data, cbuf->datalen,
		WL_CNT_XTLV_GE40_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		*rxbeaconmbss = ((const wl_cnt_ge40mcst_v1_t *)cnt)->rxbeaconmbss;
	} else if ((cnt = (const void *)bcm_get_data_from_xtlv_buf(cbuf->data, cbuf->datalen,
		WL_CNT_XTLV_GE80_UCODE_V1, NULL, BCM_XTLV_OPTION_ALIGN32)) != NULL) {
		*rxbeaconmbss = ((const wl_cnt_ge80mcst_v1_t *)cnt)->rxbeaconmbss;
	} else {
		*rxbeaconmbss = 0;
		return BCME_NOTFOUND;
	}

	return BCME_OK;
}

static int wl_cfgvendor_lstats_get_info(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	static char iovar_buf[WLC_IOCTL_MAXLEN];
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	int err = 0, i;
	wifi_radio_stat *radio;
	wifi_radio_stat_h radio_h;
	wl_wme_cnt_t *wl_wme_cnt;
	const wl_cnt_wlc_t *wlc_cnt;
	scb_val_t scbval;
	char *output = NULL;
	char *outdata = NULL;
	wifi_rate_stat_v1 *p_wifi_rate_stat_v1 = NULL;
	wifi_rate_stat *p_wifi_rate_stat = NULL;
	uint total_len = 0;
	uint32 rxbeaconmbss;
	wifi_iface_stat iface;
	wlc_rev_info_t revinfo;
#ifdef CONFIG_COMPAT
	compat_wifi_iface_stat compat_iface;
	int compat_task_state = is_compat_task();
#endif /* CONFIG_COMPAT */

	WL_INFORM_MEM(("%s: Enter \n", __func__));
	RETURN_EIO_IF_NOT_UP(cfg);

	/* Get the device rev info */
	memset(&revinfo, 0, sizeof(revinfo));
	err = wldev_ioctl_get(bcmcfg_to_prmry_ndev(cfg), WLC_GET_REVINFO, &revinfo,
			sizeof(revinfo));
	if (err != BCME_OK) {
		goto exit;
	}

	outdata = (void *)MALLOCZ(cfg->osh, WLC_IOCTL_MAXLEN);
	if (outdata == NULL) {
		WL_ERR(("%s: alloc failed\n", __func__));
		return -ENOMEM;
	}

	memset(&scbval, 0, sizeof(scb_val_t));
	memset(outdata, 0, WLC_IOCTL_MAXLEN);
	output = outdata;

	err = wldev_iovar_getbuf(bcmcfg_to_prmry_ndev(cfg), "radiostat", NULL, 0,
		iovar_buf, WLC_IOCTL_MAXLEN, NULL);
	if (err != BCME_OK && err != BCME_UNSUPPORTED) {
		WL_ERR(("error (%d) - size = %zu\n", err, sizeof(wifi_radio_stat)));
		goto exit;
	}
	radio = (wifi_radio_stat *)iovar_buf;

	memset(&radio_h, 0, sizeof(wifi_radio_stat_h));
	radio_h.on_time = radio->on_time;
	radio_h.tx_time = radio->tx_time;
	radio_h.rx_time = radio->rx_time;
	radio_h.on_time_scan = radio->on_time_scan;
	radio_h.on_time_nbd = radio->on_time_nbd;
	radio_h.on_time_gscan = radio->on_time_gscan;
	radio_h.on_time_roam_scan = radio->on_time_roam_scan;
	radio_h.on_time_pno_scan = radio->on_time_pno_scan;
	radio_h.on_time_hs20 = radio->on_time_hs20;
	radio_h.num_channels = NUM_CHAN;

	memcpy(output, &radio_h, sizeof(wifi_radio_stat_h));

	output += sizeof(wifi_radio_stat_h);
	output += (NUM_CHAN * sizeof(wifi_channel_stat));

	err = wldev_iovar_getbuf(bcmcfg_to_prmry_ndev(cfg), "wme_counters", NULL, 0,
		iovar_buf, WLC_IOCTL_MAXLEN, NULL);
	if (unlikely(err)) {
		WL_ERR(("error (%d)\n", err));
		goto exit;
	}
	wl_wme_cnt = (wl_wme_cnt_t *)iovar_buf;

	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VO].ac, WIFI_AC_VO);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VO].tx_mpdu, wl_wme_cnt->tx[AC_VO].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VO].rx_mpdu, wl_wme_cnt->rx[AC_VO].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VO].mpdu_lost,
		wl_wme_cnt->tx_failed[WIFI_AC_VO].packets);

	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VI].ac, WIFI_AC_VI);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VI].tx_mpdu, wl_wme_cnt->tx[AC_VI].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VI].rx_mpdu, wl_wme_cnt->rx[AC_VI].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_VI].mpdu_lost,
		wl_wme_cnt->tx_failed[WIFI_AC_VI].packets);

	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BE].ac, WIFI_AC_BE);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BE].tx_mpdu, wl_wme_cnt->tx[AC_BE].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BE].rx_mpdu, wl_wme_cnt->rx[AC_BE].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BE].mpdu_lost,
		wl_wme_cnt->tx_failed[WIFI_AC_BE].packets);

	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BK].ac, WIFI_AC_BK);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BK].tx_mpdu, wl_wme_cnt->tx[AC_BK].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BK].rx_mpdu, wl_wme_cnt->rx[AC_BK].packets);
	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BK].mpdu_lost,
		wl_wme_cnt->tx_failed[WIFI_AC_BK].packets);

	err = wldev_iovar_getbuf(bcmcfg_to_prmry_ndev(cfg), "counters", NULL, 0,
		iovar_buf, WLC_IOCTL_MAXLEN, NULL);
	if (unlikely(err)) {
		WL_ERR(("error (%d) - size = %zu\n", err, sizeof(wl_cnt_wlc_t)));
		goto exit;
	}

	CHK_CNTBUF_DATALEN(iovar_buf, WLC_IOCTL_MAXLEN);
	/* Translate traditional (ver <= 10) counters struct to new xtlv type struct */
	err = wl_cntbuf_to_xtlv_format(NULL, iovar_buf, WLC_IOCTL_MAXLEN, revinfo.corerev);
	if (err != BCME_OK) {
		WL_ERR(("%s wl_cntbuf_to_xtlv_format ERR %d\n",
			__FUNCTION__, err));
		goto exit;
	}

	if (!(wlc_cnt = GET_WLCCNT_FROM_CNTBUF(iovar_buf))) {
		WL_ERR(("%s wlc_cnt NULL!\n", __FUNCTION__));
		err = BCME_ERROR;
		goto exit;
	}

	COMPAT_ASSIGN_VALUE(iface, ac[WIFI_AC_BE].retries, wlc_cnt->txretry);

	err = wl_cfgvendor_lstats_get_bcn_mbss(iovar_buf, &rxbeaconmbss);
	if (unlikely(err)) {
		WL_ERR(("get_bcn_mbss error (%d)\n", err));
		goto exit;
	}

	err = wldev_get_rssi(bcmcfg_to_prmry_ndev(cfg), &scbval);
	if (unlikely(err)) {
		WL_ERR(("get_rssi error (%d)\n", err));
		goto exit;
	}

	COMPAT_ASSIGN_VALUE(iface, beacon_rx, rxbeaconmbss);
	COMPAT_ASSIGN_VALUE(iface, rssi_mgmt, scbval.val);
	COMPAT_ASSIGN_VALUE(iface, num_peers, NUM_PEER);
	COMPAT_ASSIGN_VALUE(iface, peer_info->num_rate, NUM_RATE);

#ifdef CONFIG_COMPAT
	if (compat_task_state) {
		memcpy(output, &compat_iface, sizeof(compat_iface));
		output += (sizeof(compat_iface) - sizeof(wifi_rate_stat));
	} else
#endif /* CONFIG_COMPAT */
	{
		memcpy(output, &iface, sizeof(iface));
		output += (sizeof(iface) - sizeof(wifi_rate_stat));
	}

	err = wldev_iovar_getbuf(bcmcfg_to_prmry_ndev(cfg), "ratestat", NULL, 0,
		iovar_buf, WLC_IOCTL_MAXLEN, NULL);
	if (err != BCME_OK && err != BCME_UNSUPPORTED) {
		WL_ERR(("error (%d) - size = %zu\n", err, NUM_RATE*sizeof(wifi_rate_stat)));
		goto exit;
	}
	for (i = 0; i < NUM_RATE; i++) {
		p_wifi_rate_stat =
			(wifi_rate_stat *)(iovar_buf + i*sizeof(wifi_rate_stat));
		p_wifi_rate_stat_v1 = (wifi_rate_stat_v1 *)output;
		p_wifi_rate_stat_v1->rate.preamble = p_wifi_rate_stat->rate.preamble;
		p_wifi_rate_stat_v1->rate.nss = p_wifi_rate_stat->rate.nss;
		p_wifi_rate_stat_v1->rate.bw = p_wifi_rate_stat->rate.bw;
		p_wifi_rate_stat_v1->rate.rateMcsIdx = p_wifi_rate_stat->rate.rateMcsIdx;
		p_wifi_rate_stat_v1->rate.reserved = p_wifi_rate_stat->rate.reserved;
		p_wifi_rate_stat_v1->rate.bitrate = p_wifi_rate_stat->rate.bitrate;
		p_wifi_rate_stat_v1->tx_mpdu = p_wifi_rate_stat->tx_mpdu;
		p_wifi_rate_stat_v1->rx_mpdu = p_wifi_rate_stat->rx_mpdu;
		p_wifi_rate_stat_v1->mpdu_lost = p_wifi_rate_stat->mpdu_lost;
		p_wifi_rate_stat_v1->retries = p_wifi_rate_stat->retries;
		p_wifi_rate_stat_v1->retries_short = p_wifi_rate_stat->retries_short;
		p_wifi_rate_stat_v1->retries_long = p_wifi_rate_stat->retries_long;
		output = (char *) &(p_wifi_rate_stat_v1->retries_long);
		output += sizeof(p_wifi_rate_stat_v1->retries_long);
	}

	total_len = sizeof(wifi_radio_stat_h) +
		NUM_CHAN * sizeof(wifi_channel_stat);

#ifdef CONFIG_COMPAT
	if (compat_task_state) {
		total_len += sizeof(compat_wifi_iface_stat);
	} else
#endif /* CONFIG_COMPAT */
	{
		total_len += sizeof(wifi_iface_stat);
	}

	total_len = total_len - sizeof(wifi_peer_info) +
		NUM_PEER * (sizeof(wifi_peer_info) - sizeof(wifi_rate_stat_v1) +
			NUM_RATE * sizeof(wifi_rate_stat_v1));

	if (total_len > WLC_IOCTL_MAXLEN) {
		WL_ERR(("Error! total_len:%d is unexpected value\n", total_len));
		err = BCME_BADLEN;
		goto exit;
	}
	err =  wl_cfgvendor_send_cmd_reply(wiphy, outdata, total_len);

	if (unlikely(err))
		WL_ERR(("Vendor Command reply failed ret:%d \n", err));

exit:
	if (outdata) {
		MFREE(cfg->osh, outdata, WLC_IOCTL_MAXLEN);
	}
	return err;
}
#endif /* LINKSTAT_SUPPORT */

#ifdef DHD_LOG_DUMP
static int
wl_cfgvendor_get_buf_data(const struct nlattr *iter, struct buf_data **buf)
{
	int ret = BCME_OK;

	if (nla_len(iter) != sizeof(struct buf_data)) {
		WL_ERR(("Invalid len : %d\n", nla_len(iter)));
		ret = BCME_BADLEN;
	}
	(*buf) = (struct buf_data *)nla_data(iter);
	if (!(*buf) || (((*buf)->len) <= 0) || !((*buf)->data_buf[0])) {
		WL_ERR(("Invalid buffer\n"));
		ret = BCME_ERROR;
	}
	return ret;
}

static int
wl_cfgvendor_dbg_file_dump(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int len)
{
	int ret = BCME_OK, rem, type = 0;
	const struct nlattr *iter;
	char *mem_buf = NULL;
	struct sk_buff *skb;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	struct buf_data *buf;
	int pos = 0;

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, CFG80211_VENDOR_CMD_REPLY_SKB_SZ);
	if (!skb) {
		WL_ERR(("skb allocation is failed\n"));
		ret = BCME_NOMEM;
		goto exit;
	}
	WL_ERR(("%s\n", __FUNCTION__));
	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		ret = wl_cfgvendor_get_buf_data(iter, &buf);
		if (ret)
			goto exit;
		switch (type) {
			case DUMP_BUF_ATTR_MEMDUMP:
				ret = dhd_os_get_socram_dump(bcmcfg_to_prmry_ndev(cfg), &mem_buf,
					(uint32 *)(&(buf->len)));
				if (ret) {
					WL_ERR(("failed to get_socram_dump : %d\n", ret));
					goto exit;
				}
				ret = dhd_export_debug_data(mem_buf, NULL, buf->data_buf[0],
					(int)buf->len, &pos);
				break;

			case DUMP_BUF_ATTR_TIMESTAMP :
				ret = dhd_print_time_str(buf->data_buf[0], NULL,
					(uint32)buf->len, &pos);
				break;
#ifdef EWP_ECNTRS_LOGGING
			case DUMP_BUF_ATTR_ECNTRS :
				ret = dhd_print_ecntrs_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
#endif /* EWP_ECNTRS_LOGGING */
#ifdef DHD_STATUS_LOGGING
			case DUMP_BUF_ATTR_STATUS_LOG :
				ret = dhd_print_status_log_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
#endif /* DHD_STATUS_LOGGING */
			case DUMP_BUF_ATTR_DHD_DUMP :
				ret = dhd_print_dump_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
			case DUMP_BUF_ATTR_EXT_TRAP :
				ret = dhd_print_ext_trap_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
#if defined(DHD_FW_COREDUMP) && defined(DNGL_EVENT_SUPPORT)
			case DUMP_BUF_ATTR_HEALTH_CHK :
				ret = dhd_print_health_chk_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
#endif // endif
			case DUMP_BUF_ATTR_COOKIE :
				ret = dhd_print_cookie_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
#ifdef DHD_DUMP_PCIE_RINGS
			case DUMP_BUF_ATTR_FLOWRING_DUMP :
				ret = dhd_print_flowring_data(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len, &pos);
				break;
#endif // endif
			case DUMP_BUF_ATTR_GENERAL_LOG :
				ret = dhd_get_dld_log_dump(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len,
					DLD_BUF_TYPE_GENERAL, &pos);
				break;

			case DUMP_BUF_ATTR_PRESERVE_LOG :
				ret = dhd_get_dld_log_dump(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len,
					DLD_BUF_TYPE_PRESERVE, &pos);
				break;

			case DUMP_BUF_ATTR_SPECIAL_LOG :
				ret = dhd_get_dld_log_dump(bcmcfg_to_prmry_ndev(cfg), NULL,
					buf->data_buf[0], NULL, (uint32)buf->len,
					DLD_BUF_TYPE_SPECIAL, &pos);
				break;
#ifdef DHD_SSSR_DUMP
			case DUMP_BUF_ATTR_SSSR_C0_D11_BEFORE :
				ret = dhd_sssr_dump_d11_buf_before(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len, 0);
				break;

			case DUMP_BUF_ATTR_SSSR_C0_D11_AFTER :
				ret = dhd_sssr_dump_d11_buf_after(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len, 0);
				break;

			case DUMP_BUF_ATTR_SSSR_C1_D11_BEFORE :
				ret = dhd_sssr_dump_d11_buf_before(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len, 1);
				break;

			case DUMP_BUF_ATTR_SSSR_C1_D11_AFTER :
				ret = dhd_sssr_dump_d11_buf_after(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len, 1);
				break;

			case DUMP_BUF_ATTR_SSSR_DIG_BEFORE :
				ret = dhd_sssr_dump_dig_buf_before(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len);
				break;

			case DUMP_BUF_ATTR_SSSR_DIG_AFTER :
				ret = dhd_sssr_dump_dig_buf_after(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len);
				break;
#endif /* DHD_SSSR_DUMP */
#ifdef DHD_PKT_LOGGING
			case DUMP_BUF_ATTR_PKTLOG:
				ret = dhd_os_get_pktlog_dump(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len);
				break;
#endif /* DHD_PKT_LOGGING */
#ifdef DNGL_AXI_ERROR_LOGGING
			case DUMP_BUF_ATTR_AXI_ERROR:
				ret = dhd_os_get_axi_error_dump(bcmcfg_to_prmry_ndev(cfg),
					buf->data_buf[0], (uint32)buf->len);
				break;
#endif /* DNGL_AXI_ERROR_LOGGING */
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_ERROR;
				goto exit;
		}
	}

	if (ret)
		goto exit;

	nla_put_u32(skb, type, (uint32)(ret));
	ret = cfg80211_vendor_cmd_reply(skb);
	if (ret) {
		WL_ERR(("Vendor Command reply failed ret:%d \n", ret));
	}

exit:
	return ret;
}
#endif /* DHD_LOG_DUMP */

#ifdef DEBUGABILITY
static int
wl_cfgvendor_dbg_trigger_mem_dump(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK;
	uint32 alloc_len;
	struct sk_buff *skb;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhdp = (dhd_pub_t *)(cfg->pub);

	WL_ERR(("wl_cfgvendor_dbg_trigger_mem_dump %d\n", __LINE__));

	dhdp->memdump_type = DUMP_TYPE_CFG_VENDOR_TRIGGERED;
	ret = dhd_os_socram_dump(bcmcfg_to_prmry_ndev(cfg), &alloc_len);
	if (ret) {
		WL_ERR(("failed to call dhd_os_socram_dump : %d\n", ret));
		goto exit;
	}
	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, CFG80211_VENDOR_CMD_REPLY_SKB_SZ);
	if (!skb) {
		WL_ERR(("skb allocation is failed\n"));
		ret = BCME_NOMEM;
		goto exit;
	}
	nla_put_u32(skb, DEBUG_ATTRIBUTE_FW_DUMP_LEN, alloc_len);

	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret) {
		WL_ERR(("Vendor Command reply failed ret:%d \n", ret));
	}

exit:
	return ret;
}

static int
wl_cfgvendor_dbg_get_mem_dump(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int len)
{
	int ret = BCME_OK, rem, type;
	int buf_len = 0;
	uintptr_t user_buf = (uintptr_t)NULL;
	const struct nlattr *iter;
	char *mem_buf = NULL;
	struct sk_buff *skb;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case DEBUG_ATTRIBUTE_FW_DUMP_LEN:
				/* Check if the iter is valid and
				 * buffer length is not already initialized.
				 */
				if ((nla_len(iter) == sizeof(uint32)) &&
						!buf_len) {
					buf_len = nla_get_u32(iter);
					if (buf_len <= 0) {
						ret = BCME_ERROR;
						goto exit;
					}
				} else {
					ret = BCME_ERROR;
					goto exit;
				}
				break;
			case DEBUG_ATTRIBUTE_FW_DUMP_DATA:
				if (nla_len(iter) != sizeof(uint64)) {
					WL_ERR(("Invalid len\n"));
					ret = BCME_ERROR;
					goto exit;
				}
				user_buf = (uintptr_t)nla_get_u64(iter);
				if (!user_buf) {
					ret = BCME_ERROR;
					goto exit;
				}
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_ERROR;
				goto exit;
		}
	}
	if (buf_len > 0 && user_buf) {
		mem_buf = vmalloc(buf_len);
		if (!mem_buf) {
			WL_ERR(("failed to allocate mem_buf with size : %d\n", buf_len));
			ret = BCME_NOMEM;
			goto exit;
		}
		ret = dhd_os_get_socram_dump(bcmcfg_to_prmry_ndev(cfg), &mem_buf, &buf_len);
		if (ret) {
			WL_ERR(("failed to get_socram_dump : %d\n", ret));
			goto free_mem;
		}
#ifdef CONFIG_COMPAT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0))
		if (in_compat_syscall()) {
#else
		if (is_compat_task()) {
#endif /* LINUX_VER >= 4.6 */
			void * usr_ptr =  compat_ptr((uintptr_t) user_buf);
			ret = copy_to_user(usr_ptr, mem_buf, buf_len);
			if (ret) {
				WL_ERR(("failed to copy memdump into user buffer : %d\n", ret));
				goto free_mem;
			}
		}
		else
#endif /* CONFIG_COMPAT */
		{
			ret = copy_to_user((void*)user_buf, mem_buf, buf_len);
			if (ret) {
				WL_ERR(("failed to copy memdump into user buffer : %d\n", ret));
				goto free_mem;
			}
		}
		/* Alloc the SKB for vendor_event */
		skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, CFG80211_VENDOR_CMD_REPLY_SKB_SZ);
		if (!skb) {
			WL_ERR(("skb allocation is failed\n"));
			ret = BCME_NOMEM;
			goto free_mem;
		}
		/* Indicate the memdump is succesfully copied */
		nla_put(skb, DEBUG_ATTRIBUTE_FW_DUMP_DATA, sizeof(ret), &ret);

		ret = cfg80211_vendor_cmd_reply(skb);

		if (ret) {
			WL_ERR(("Vendor Command reply failed ret:%d \n", ret));
		}
	}

free_mem:
	vfree(mem_buf);
exit:
	return ret;
}

static int wl_cfgvendor_dbg_start_logging(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK, rem, type;
	char ring_name[DBGRING_NAME_MAX] = {0};
	int log_level = 0, flags = 0, time_intval = 0, threshold = 0;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;
	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case DEBUG_ATTRIBUTE_RING_NAME:
				strncpy(ring_name, nla_data(iter),
					MIN(sizeof(ring_name) -1, nla_len(iter)));
				break;
			case DEBUG_ATTRIBUTE_LOG_LEVEL:
				log_level = nla_get_u32(iter);
				break;
			case DEBUG_ATTRIBUTE_RING_FLAGS:
				flags = nla_get_u32(iter);
				break;
			case DEBUG_ATTRIBUTE_LOG_TIME_INTVAL:
				time_intval = nla_get_u32(iter);
				break;
			case DEBUG_ATTRIBUTE_LOG_MIN_DATA_SIZE:
				threshold = nla_get_u32(iter);
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_BADADDR;
				goto exit;
		}
	}

	ret = dhd_os_start_logging(dhd_pub, ring_name, log_level, flags, time_intval, threshold);
	if (ret < 0) {
		WL_ERR(("start_logging is failed ret: %d\n", ret));
	}
exit:
	return ret;
}

static int wl_cfgvendor_dbg_reset_logging(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;

	ret = dhd_os_reset_logging(dhd_pub);
	if (ret < 0) {
		WL_ERR(("reset logging is failed ret: %d\n", ret));
	}

	return ret;
}

static int wl_cfgvendor_dbg_get_ring_status(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK;
	int ring_id, i;
	int ring_cnt;
	struct sk_buff *skb;
	dhd_dbg_ring_status_t dbg_ring_status[DEBUG_RING_ID_MAX];
	dhd_dbg_ring_status_t ring_status;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;
	memset(dbg_ring_status, 0, DBG_RING_STATUS_SIZE * DEBUG_RING_ID_MAX);
	ring_cnt = 0;
	for (ring_id = DEBUG_RING_ID_INVALID + 1; ring_id < DEBUG_RING_ID_MAX; ring_id++) {
		ret = dhd_os_get_ring_status(dhd_pub, ring_id, &ring_status);
		if (ret == BCME_NOTFOUND) {
			WL_DBG(("The ring (%d) is not found \n", ring_id));
		} else if (ret == BCME_OK) {
			dbg_ring_status[ring_cnt++] = ring_status;
		}
	}
	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
		(DBG_RING_STATUS_SIZE * ring_cnt) + CFG80211_VENDOR_CMD_REPLY_SKB_SZ);
	if (!skb) {
		WL_ERR(("skb allocation is failed\n"));
		ret = BCME_NOMEM;
		goto exit;
	}

	nla_put_u32(skb, DEBUG_ATTRIBUTE_RING_NUM, ring_cnt);
	for (i = 0; i < ring_cnt; i++) {
		nla_put(skb, DEBUG_ATTRIBUTE_RING_STATUS, DBG_RING_STATUS_SIZE,
				&dbg_ring_status[i]);
	}
	ret = cfg80211_vendor_cmd_reply(skb);

	if (ret) {
		WL_ERR(("Vendor Command reply failed ret:%d \n", ret));
	}
exit:
	return ret;
}

static int wl_cfgvendor_dbg_get_ring_data(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK, rem, type;
	char ring_name[DBGRING_NAME_MAX] = {0};
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case DEBUG_ATTRIBUTE_RING_NAME:
				strncpy(ring_name, nla_data(iter),
					MIN(sizeof(ring_name) -1, nla_len(iter)));
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				return ret;
		}
	}

	ret = dhd_os_trigger_get_ring_data(dhd_pub, ring_name);
	if (ret < 0) {
		WL_ERR(("trigger_get_data failed ret:%d\n", ret));
	}

	return ret;
}
#endif /* DEBUGABILITY */

static int wl_cfgvendor_dbg_get_feature(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK;
	u32 supported_features = 0;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;

	ret = dhd_os_dbg_get_feature(dhd_pub, &supported_features);
	if (ret < 0) {
		WL_ERR(("dbg_get_feature failed ret:%d\n", ret));
		goto exit;
	}
	ret = wl_cfgvendor_send_cmd_reply(wiphy, &supported_features,
		sizeof(supported_features));
exit:
	return ret;
}

#ifdef DEBUGABILITY
static void wl_cfgvendor_dbg_ring_send_evt(void *ctx,
	const int ring_id, const void *data, const uint32 len,
	const dhd_dbg_ring_status_t ring_status)
{
	struct net_device *ndev = ctx;
	struct wiphy *wiphy;
	gfp_t kflags;
	struct sk_buff *skb;
	if (!ndev) {
		WL_ERR(("ndev is NULL\n"));
		return;
	}
	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	wiphy = ndev->ieee80211_ptr->wiphy;
	/* Alloc the SKB for vendor_event */
#if (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	skb = cfg80211_vendor_event_alloc(wiphy, NULL, len + 100,
			GOOGLE_DEBUG_RING_EVENT, kflags);
#else
	skb = cfg80211_vendor_event_alloc(wiphy, len + 100,
			GOOGLE_DEBUG_RING_EVENT, kflags);
#endif /* (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || */
		/* LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0) */
	if (!skb) {
		WL_ERR(("skb alloc failed"));
		return;
	}
	nla_put(skb, DEBUG_ATTRIBUTE_RING_STATUS, sizeof(ring_status), &ring_status);
	nla_put(skb, DEBUG_ATTRIBUTE_RING_DATA, len, data);
	cfg80211_vendor_event(skb, kflags);
}
#endif /* DEBUGABILITY */

#ifdef DHD_LOG_DUMP
static void wl_cfgvendor_nla_put_sssr_dump_data(struct sk_buff *skb,
		struct net_device *ndev)
{
#ifdef DHD_SSSR_DUMP
	uint32 arr_len[DUMP_SSSR_ATTR_COUNT];
	int i = 0, j = 0;
#endif /* DHD_SSSR_DUMP */
	char memdump_path[MEMDUMP_PATH_LEN];

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN,
		"sssr_dump_core_0_before_SR");
	nla_put_string(skb, DUMP_FILENAME_ATTR_SSSR_CORE_0_BEFORE_DUMP, memdump_path);

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN,
		"sssr_dump_core_0_after_SR");
	nla_put_string(skb, DUMP_FILENAME_ATTR_SSSR_CORE_0_AFTER_DUMP, memdump_path);

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN,
		"sssr_dump_core_1_before_SR");
	nla_put_string(skb, DUMP_FILENAME_ATTR_SSSR_CORE_1_BEFORE_DUMP, memdump_path);

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN,
		"sssr_dump_core_1_after_SR");
	nla_put_string(skb, DUMP_FILENAME_ATTR_SSSR_CORE_1_AFTER_DUMP, memdump_path);

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN,
		"sssr_dump_dig_before_SR");
	nla_put_string(skb, DUMP_FILENAME_ATTR_SSSR_DIG_BEFORE_DUMP, memdump_path);

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN,
		"sssr_dump_dig_after_SR");
	nla_put_string(skb, DUMP_FILENAME_ATTR_SSSR_DIG_AFTER_DUMP, memdump_path);

#ifdef DHD_SSSR_DUMP
	memset(arr_len, 0, sizeof(arr_len));
	dhd_nla_put_sssr_dump_len(ndev, arr_len);

	for (i = 0, j = DUMP_SSSR_ATTR_START; i < DUMP_SSSR_ATTR_COUNT; i++, j++) {
		if (arr_len[i])
			nla_put_u32(skb, j, arr_len[i]);
	}
#endif /* DHD_SSSR_DUMP */
}

static void wl_cfgvendor_nla_put_debug_dump_data(struct sk_buff *skb,
		struct net_device *ndev)
{
	int ret = 0;
	uint32 len = 0;
	char dump_path[128];

	ret = dhd_get_debug_dump_file_name(ndev, NULL, dump_path, sizeof(dump_path));
	if (ret < 0) {
		WL_ERR(("%s: Failed to get debug dump filename\n", __FUNCTION__));
		return;
	}
	nla_put_string(skb, DUMP_FILENAME_ATTR_DEBUG_DUMP, dump_path);
	WL_ERR(("debug_dump path = %s%s\n", dump_path, FILE_NAME_HAL_TAG));
	wl_print_verinfo(wl_get_cfg(ndev));

	len = dhd_get_time_str_len();
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_TIMESTAMP, len);

	len = dhd_get_dld_len(DLD_BUF_TYPE_GENERAL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_GENERAL_LOG, len);
#ifdef EWP_ECNTRS_LOGGING
	len = dhd_get_ecntrs_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_ECNTRS, len);
#endif /* EWP_ECNTRS_LOGGING */
	len = dhd_get_dld_len(DLD_BUF_TYPE_SPECIAL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_SPECIAL_LOG, len);

	len = dhd_get_dhd_dump_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_DHD_DUMP, len);

	len = dhd_get_ext_trap_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_EXT_TRAP, len);

#if defined(DHD_FW_COREDUMP) && defined(DNGL_EVENT_SUPPORT)
	len = dhd_get_health_chk_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_HEALTH_CHK, len);
#endif // endif

	len = dhd_get_dld_len(DLD_BUF_TYPE_PRESERVE);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_PRESERVE_LOG, len);

	len = dhd_get_cookie_log_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_COOKIE, len);
#ifdef DHD_DUMP_PCIE_RINGS
	len = dhd_get_flowring_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_FLOWRING_DUMP, len);
#endif // endif
#ifdef DHD_STATUS_LOGGING
	len = dhd_get_status_log_len(ndev, NULL);
	if (len)
		nla_put_u32(skb, DUMP_LEN_ATTR_STATUS_LOG, len);
#endif /* DHD_STATUS_LOGGING */
}
#ifdef DNGL_AXI_ERROR_LOGGING
static void wl_cfgvendor_nla_put_axi_error_data(struct sk_buff *skb,
		struct net_device *ndev)
{
	char axierrordump_path[MEMDUMP_PATH_LEN];
	int dumpsize = dhd_os_get_axi_error_dump_size(ndev);
	if (dumpsize <= 0) {
		WL_ERR(("Failed to calcuate axi error dump len\n"));
		return;
	}
	dhd_os_get_axi_error_filename(ndev, axierrordump_path, MEMDUMP_PATH_LEN);
	nla_put_string(skb, DUMP_FILENAME_ATTR_AXI_ERROR_DUMP, axierrordump_path);
	nla_put_u32(skb, DUMP_LEN_ATTR_AXI_ERROR, dumpsize);
}
#endif /* DNGL_AXI_ERROR_LOGGING */
#ifdef DHD_PKT_LOGGING
static void wl_cfgvendor_nla_put_pktlogdump_data(struct sk_buff *skb,
		struct net_device *ndev)
{
	char pktlogdump_path[MEMDUMP_PATH_LEN];
	uint32 pktlog_dumpsize = dhd_os_get_pktlog_dump_size(ndev);
	if (pktlog_dumpsize == 0) {
		WL_ERR(("Failed to calcuate pktlog len\n"));
		return;
	}
	dhd_os_get_pktlogdump_filename(ndev, pktlogdump_path, MEMDUMP_PATH_LEN);
	nla_put_string(skb, DUMP_FILENAME_ATTR_PKTLOG_DUMP, pktlogdump_path);
	nla_put_u32(skb, DUMP_LEN_ATTR_PKTLOG, pktlog_dumpsize);
}
#endif /* DHD_PKT_LOGGING */

static void wl_cfgvendor_nla_put_memdump_data(struct sk_buff *skb,
		struct net_device *ndev, const uint32 fw_len)
{
	char memdump_path[MEMDUMP_PATH_LEN];

	dhd_get_memdump_filename(ndev, memdump_path, MEMDUMP_PATH_LEN, "mem_dump");
	nla_put_string(skb, DUMP_FILENAME_ATTR_MEM_DUMP, memdump_path);
	nla_put_u32(skb, DUMP_LEN_ATTR_MEMDUMP, fw_len);
}

static void wl_cfgvendor_dbg_send_file_dump_evt(void *ctx, const void *data,
	const uint32 len, const uint32 fw_len)
{
	struct net_device *ndev = ctx;
	struct wiphy *wiphy;
	gfp_t kflags;
	struct sk_buff *skb;
	struct bcm_cfg80211 *cfg;
	dhd_pub_t *dhd_pub;

	if (!ndev) {
		WL_ERR(("ndev is NULL\n"));
		return;
	}

	kflags = in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
	wiphy = ndev->ieee80211_ptr->wiphy;
	/* Alloc the SKB for vendor_event */
#if (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
	skb = cfg80211_vendor_event_alloc(wiphy, NULL, len + CFG80211_VENDOR_EVT_SKB_SZ,
			GOOGLE_FILE_DUMP_EVENT, kflags);
#else
	skb = cfg80211_vendor_event_alloc(wiphy, len + CFG80211_VENDOR_EVT_SKB_SZ,
			GOOGLE_FILE_DUMP_EVENT, kflags);
#endif /* (defined(CONFIG_ARCH_MSM) && defined(SUPPORT_WDEV_CFG80211_VENDOR_EVENT_ALLOC)) || */
		/* LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0) */
	if (!skb) {
		WL_ERR(("skb alloc failed"));
		return;
	}

	cfg = wiphy_priv(wiphy);
	dhd_pub = cfg->pub;
#ifdef DNGL_AXI_ERROR_LOGGING
	if (dhd_pub->smmu_fault_occurred) {
		wl_cfgvendor_nla_put_axi_error_data(skb, ndev);
	}
#endif /* DNGL_AXI_ERROR_LOGGING */
	if (dhd_pub->memdump_enabled || (dhd_pub->memdump_type == DUMP_TYPE_BY_SYSDUMP)) {
		wl_cfgvendor_nla_put_memdump_data(skb, ndev, fw_len);
		wl_cfgvendor_nla_put_debug_dump_data(skb, ndev);
		wl_cfgvendor_nla_put_sssr_dump_data(skb, ndev);
#ifdef DHD_PKT_LOGGING
		wl_cfgvendor_nla_put_pktlogdump_data(skb, ndev);
#endif /* DHD_PKT_LOGGING */
	}
	/* TODO : Similar to above function add for debug_dump, sssr_dump, and pktlog also. */
	cfg80211_vendor_event(skb, kflags);
}
#endif /* DHD_LOG_DUMP */

static int wl_cfgvendor_dbg_get_version(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	int ret = BCME_OK, rem, type;
	int buf_len = 1024;
	bool dhd_ver = FALSE;
	char *buf_ptr;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);

	buf_ptr = (char *)MALLOCZ(cfg->osh, buf_len);
	if (!buf_ptr) {
		WL_ERR(("failed to allocate the buffer for version n"));
		ret = BCME_NOMEM;
		goto exit;
	}
	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case DEBUG_ATTRIBUTE_GET_DRIVER:
				dhd_ver = TRUE;
				break;
			case DEBUG_ATTRIBUTE_GET_FW:
				dhd_ver = FALSE;
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_ERROR;
				goto exit;
		}
	}
	ret = dhd_os_get_version(bcmcfg_to_prmry_ndev(cfg), dhd_ver, &buf_ptr, buf_len);
	if (ret < 0) {
		WL_ERR(("failed to get the version %d\n", ret));
		goto exit;
	}
	ret = wl_cfgvendor_send_cmd_reply(wiphy, buf_ptr, strlen(buf_ptr));
exit:
	MFREE(cfg->osh, buf_ptr, buf_len);
	return ret;
}

#ifdef DBG_PKT_MON
static int wl_cfgvendor_dbg_start_pkt_fate_monitoring(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;
	int ret;

	ret = dhd_os_dbg_attach_pkt_monitor(dhd_pub);
	if (unlikely(ret)) {
		WL_ERR(("failed to start pkt fate monitoring, ret=%d", ret));
	}

	return ret;
}

typedef int (*dbg_mon_get_pkts_t) (dhd_pub_t *dhdp, void __user *user_buf,
	uint16 req_count, uint16 *resp_count);

static int __wl_cfgvendor_dbg_get_pkt_fates(struct wiphy *wiphy,
	const void *data, int len, dbg_mon_get_pkts_t dbg_mon_get_pkts)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;
	struct sk_buff *skb = NULL;
	const struct nlattr *iter;
	void __user *user_buf = NULL;
	uint16 req_count = 0, resp_count = 0;
	int ret, tmp, type, mem_needed;

	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
			case DEBUG_ATTRIBUTE_PKT_FATE_NUM:
				req_count = nla_get_u32(iter);
				break;
			case DEBUG_ATTRIBUTE_PKT_FATE_DATA:
				user_buf = (void __user *)(unsigned long) nla_get_u64(iter);
				break;
			default:
				WL_ERR(("%s: no such attribute %d\n", __FUNCTION__, type));
				ret = -EINVAL;
				goto exit;
		}
	}

	if (!req_count || !user_buf) {
		WL_ERR(("%s: invalid request, user_buf=%p, req_count=%u\n",
			__FUNCTION__, user_buf, req_count));
		ret = -EINVAL;
		goto exit;
	}

	ret = dbg_mon_get_pkts(dhd_pub, user_buf, req_count, &resp_count);
	if (unlikely(ret)) {
		WL_ERR(("failed to get packets, ret:%d \n", ret));
		goto exit;
	}

	mem_needed = VENDOR_REPLY_OVERHEAD + ATTRIBUTE_U32_LEN;
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		ret = -ENOMEM;
		goto exit;
	}

	nla_put_u32(skb, DEBUG_ATTRIBUTE_PKT_FATE_NUM, resp_count);

	ret = cfg80211_vendor_cmd_reply(skb);
	if (unlikely(ret)) {
		WL_ERR(("vendor Command reply failed ret:%d \n", ret));
	}

exit:
	return ret;
}

static int wl_cfgvendor_dbg_get_tx_pkt_fates(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret;

	ret = __wl_cfgvendor_dbg_get_pkt_fates(wiphy, data, len,
			dhd_os_dbg_monitor_get_tx_pkts);
	if (unlikely(ret)) {
		WL_ERR(("failed to get tx packets, ret:%d \n", ret));
	}

	return ret;
}

static int wl_cfgvendor_dbg_get_rx_pkt_fates(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	int ret;

	ret = __wl_cfgvendor_dbg_get_pkt_fates(wiphy, data, len,
			dhd_os_dbg_monitor_get_rx_pkts);
	if (unlikely(ret)) {
		WL_ERR(("failed to get rx packets, ret:%d \n", ret));
	}

	return ret;
}
#endif /* DBG_PKT_MON */

#ifdef KEEP_ALIVE
static int wl_cfgvendor_start_mkeep_alive(struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int len)
{
	/* max size of IP packet for keep alive */
	const int MKEEP_ALIVE_IP_PKT_MAX = 256;

	int ret = BCME_OK, rem, type;
	uint8 mkeep_alive_id = 0;
	uint8 *ip_pkt = NULL;
	uint16 ip_pkt_len = 0;
	uint8 src_mac[ETHER_ADDR_LEN];
	uint8 dst_mac[ETHER_ADDR_LEN];
	uint32 period_msec = 0;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case MKEEP_ALIVE_ATTRIBUTE_ID:
				mkeep_alive_id = nla_get_u8(iter);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
				ip_pkt_len = nla_get_u16(iter);
				if (ip_pkt_len > MKEEP_ALIVE_IP_PKT_MAX) {
					ret = BCME_BADARG;
					goto exit;
				}
				break;
			case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
				if (ip_pkt) {
					ret = BCME_BADARG;
					WL_ERR(("ip_pkt already allocated\n"));
					goto exit;
				}
				if (!ip_pkt_len) {
					ret = BCME_BADARG;
					WL_ERR(("ip packet length is 0\n"));
					goto exit;
				}
				ip_pkt = (u8 *)MALLOCZ(cfg->osh, ip_pkt_len);
				if (ip_pkt == NULL) {
					ret = BCME_NOMEM;
					WL_ERR(("Failed to allocate mem for ip packet\n"));
					goto exit;
				}
				memcpy(ip_pkt, (u8*)nla_data(iter), ip_pkt_len);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
				memcpy(src_mac, nla_data(iter), ETHER_ADDR_LEN);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
				memcpy(dst_mac, nla_data(iter), ETHER_ADDR_LEN);
				break;
			case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
				period_msec = nla_get_u32(iter);
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_BADARG;
				goto exit;
		}
	}

	if (ip_pkt == NULL) {
		ret = BCME_BADARG;
		WL_ERR(("ip packet is NULL\n"));
		goto exit;
	}

	ret = dhd_dev_start_mkeep_alive(dhd_pub, mkeep_alive_id, ip_pkt, ip_pkt_len, src_mac,
		dst_mac, period_msec);
	if (ret < 0) {
		WL_ERR(("start_mkeep_alive is failed ret: %d\n", ret));
	}

exit:
	if (ip_pkt) {
		MFREE(cfg->osh, ip_pkt, ip_pkt_len);
	}

	return ret;
}

static int wl_cfgvendor_stop_mkeep_alive(struct wiphy *wiphy, struct wireless_dev *wdev,
	const void *data, int len)
{
	int ret = BCME_OK, rem, type;
	uint8 mkeep_alive_id = 0;
	const struct nlattr *iter;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case MKEEP_ALIVE_ATTRIBUTE_ID:
				mkeep_alive_id = nla_get_u8(iter);
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_BADARG;
				break;
		}
	}

	ret = dhd_dev_stop_mkeep_alive(dhd_pub, mkeep_alive_id);
	if (ret < 0) {
		WL_ERR(("stop_mkeep_alive is failed ret: %d\n", ret));
	}

	return ret;
}
#endif /* KEEP_ALIVE */

#if defined(PKT_FILTER_SUPPORT) && defined(APF)
static int
wl_cfgvendor_apf_get_capabilities(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int len)
{
	struct net_device *ndev = wdev_to_ndev(wdev);
	struct sk_buff *skb;
	int ret, ver, max_len, mem_needed;

	/* APF version */
	ver = 0;
	ret = dhd_dev_apf_get_version(ndev, &ver);
	if (unlikely(ret)) {
		WL_ERR(("APF get version failed, ret=%d\n", ret));
		return ret;
	}

	/* APF memory size limit */
	max_len = 0;
	ret = dhd_dev_apf_get_max_len(ndev, &max_len);
	if (unlikely(ret)) {
		WL_ERR(("APF get maximum length failed, ret=%d\n", ret));
		return ret;
	}

	mem_needed = VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("%s: can't allocate %d bytes\n", __FUNCTION__, mem_needed));
		return -ENOMEM;
	}

	nla_put_u32(skb, APF_ATTRIBUTE_VERSION, ver);
	nla_put_u32(skb, APF_ATTRIBUTE_MAX_LEN, max_len);

	ret = cfg80211_vendor_cmd_reply(skb);
	if (unlikely(ret)) {
		WL_ERR(("vendor command reply failed, ret=%d\n", ret));
	}

	return ret;
}

static int
wl_cfgvendor_apf_set_filter(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	struct net_device *ndev = wdev_to_ndev(wdev);
	const struct nlattr *iter;
	u8 *program = NULL;
	u32 program_len = 0;
	int ret, tmp, type;
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);

	if (len <= 0) {
		WL_ERR(("Invalid len: %d\n", len));
		ret = -EINVAL;
		goto exit;
	}
	nla_for_each_attr(iter, data, len, tmp) {
		type = nla_type(iter);
		switch (type) {
			case APF_ATTRIBUTE_PROGRAM_LEN:
				/* check if the iter value is valid and program_len
				 * is not already initialized.
				 */
				if (nla_len(iter) == sizeof(uint32) && !program_len) {
					program_len = nla_get_u32(iter);
				} else {
					ret = -EINVAL;
					goto exit;
				}

				if (program_len > WL_APF_PROGRAM_MAX_SIZE) {
					WL_ERR(("program len is more than expected len\n"));
					ret = -EINVAL;
					goto exit;
				}

				if (unlikely(!program_len)) {
					WL_ERR(("zero program length\n"));
					ret = -EINVAL;
					goto exit;
				}
				break;
			case APF_ATTRIBUTE_PROGRAM:
				if (unlikely(program)) {
					WL_ERR(("program already allocated\n"));
					ret = -EINVAL;
					goto exit;
				}
				if (unlikely(!program_len)) {
					WL_ERR(("program len is not set\n"));
					ret = -EINVAL;
					goto exit;
				}
				if (nla_len(iter) != program_len) {
					WL_ERR(("program_len is not same\n"));
					ret = -EINVAL;
					goto exit;
				}
				program = MALLOCZ(cfg->osh, program_len);
				if (unlikely(!program)) {
					WL_ERR(("%s: can't allocate %d bytes\n",
					      __FUNCTION__, program_len));
					ret = -ENOMEM;
					goto exit;
				}
				memcpy(program, (u8*)nla_data(iter), program_len);
				break;
			default:
				WL_ERR(("%s: no such attribute %d\n", __FUNCTION__, type));
				ret = -EINVAL;
				goto exit;
		}
	}

	ret = dhd_dev_apf_add_filter(ndev, program, program_len);

exit:
	if (program) {
		MFREE(cfg->osh, program, program_len);
	}
	return ret;
}
#endif /* PKT_FILTER_SUPPORT && APF */

#ifdef NDO_CONFIG_SUPPORT
static int wl_cfgvendor_configure_nd_offload(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void  *data, int len)
{
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	const struct nlattr *iter;
	int ret = BCME_OK, rem, type;
	u8 enable = 0;

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case ANDR_WIFI_ATTRIBUTE_ND_OFFLOAD_VALUE:
				enable = nla_get_u8(iter);
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_BADARG;
				goto exit;
		}
	}

	ret = dhd_dev_ndo_cfg(bcmcfg_to_prmry_ndev(cfg), enable);
	if (ret < 0) {
		WL_ERR(("dhd_dev_ndo_cfg() failed: %d\n", ret));
	}

exit:
	return ret;
}
#endif /* NDO_CONFIG_SUPPORT */

/* for kernel >= 4.13 NL80211 wl_cfg80211_set_pmk have to be used. */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0))
static int wl_cfgvendor_set_pmk(struct wiphy *wiphy,
        struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = 0;
	wsec_pmk_t pmk;
	const struct nlattr *iter;
	int rem, type;
	struct net_device *ndev = wdev_to_ndev(wdev);
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	struct wl_security *sec;

	nla_for_each_attr(iter, data, len, rem) {
		type = nla_type(iter);
		switch (type) {
			case BRCM_ATTR_DRIVER_KEY_PMK:
				pmk.key_len = htod16(nla_len(iter));
				if (pmk.key_len > sizeof(pmk.key)) {
					ret = -EINVAL;
					goto exit;
				}
				pmk.flags = 0;
				bcopy((uint8 *)nla_data(iter), pmk.key, len);
				break;
			default:
				WL_ERR(("Unknown type: %d\n", type));
				ret = BCME_BADARG;
				goto exit;
		}
	}

	sec = wl_read_prof(cfg, ndev, WL_PROF_SEC);
	if ((sec->wpa_auth == WLAN_AKM_SUITE_8021X) ||
		(sec->wpa_auth == WL_AKM_SUITE_SHA256_1X)) {
		ret = wldev_iovar_setbuf(ndev, "okc_info_pmk", pmk.key, pmk.key_len, cfg->ioctl_buf,
			WLC_IOCTL_SMLEN,  &cfg->ioctl_buf_sync);
		if (ret) {
			/* could fail in case that 'okc' is not supported */
			WL_INFORM_MEM(("okc_info_pmk failed, err=%d (ignore)\n", ret));
		}
	}

	ret = wldev_ioctl_set(ndev, WLC_SET_WSEC_PMK, &pmk, sizeof(pmk));
	WL_INFORM_MEM(("IOVAR set_pmk ret:%d", ret));
exit:
	return ret;
}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0) */

static int wl_cfgvendor_get_driver_feature(struct wiphy *wiphy,
        struct wireless_dev *wdev, const void  *data, int len)
{
	int ret = BCME_OK;
	u8 supported[(BRCM_WLAN_VENDOR_FEATURES_MAX / 8) + 1] = {0};
	struct bcm_cfg80211 *cfg = wiphy_priv(wiphy);
	dhd_pub_t *dhd_pub = cfg->pub;
	struct sk_buff *skb;
	int32 mem_needed;

	mem_needed = VENDOR_REPLY_OVERHEAD + NLA_HDRLEN + sizeof(supported);

	BCM_REFERENCE(dhd_pub);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0))
	if (FW_SUPPORTED(dhd_pub, idsup)) {
		ret = wl_features_set(supported, sizeof(supported),
				BRCM_WLAN_VENDOR_FEATURE_KEY_MGMT_OFFLOAD);
	}
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0) */

	/* Alloc the SKB for vendor_event */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
	if (unlikely(!skb)) {
		WL_ERR(("skb alloc failed"));
		ret = BCME_NOMEM;
		goto exit;
	}

	nla_put(skb, BRCM_ATTR_DRIVER_FEATURE_FLAGS, sizeof(supported), supported);
	ret = cfg80211_vendor_cmd_reply(skb);
exit:
	return ret;
}

static const struct wiphy_vendor_command wl_vendor_cmds [] = {
	{
		{
			.vendor_id = OUI_BRCM,
			.subcmd = BRCM_VENDOR_SCMD_PRIV_STR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_priv_string_handler
	},
#ifdef BCM_PRIV_CMD_SUPPORT
	{
		{
			.vendor_id = OUI_BRCM,
			.subcmd = BRCM_VENDOR_SCMD_BCM_STR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_priv_bcm_handler
	},
#endif /* BCM_PRIV_CMD_SUPPORT */
#ifdef WL_SAE
	{
		{
			.vendor_id = OUI_BRCM,
			.subcmd = BRCM_VENDOR_SCMD_BCM_PSK
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_sae_password
	},
#endif /* WL_SAE */
#ifdef GSCAN_SUPPORT
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_gscan_get_capabilities
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_SET_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_scan_cfg
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_SET_SCAN_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_batch_scan_cfg
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_ENABLE_GSCAN
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_initiate_gscan
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_ENABLE_FULL_SCAN_RESULTS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_enable_full_scan_result
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_SET_HOTLIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_hotlist_cfg
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_GET_SCAN_RESULTS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_gscan_get_batch_results
	},
#endif /* GSCAN_SUPPORT */
#if defined(GSCAN_SUPPORT) || defined(DHD_GET_VALID_CHANNELS)
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_GET_CHANNEL_LIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_gscan_get_channel_list
	},
#endif /* GSCAN_SUPPORT || DHD_GET_VALID_CHANNELS */
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = ANDR_WIFI_SUBCMD_GET_FEATURE_SET
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_get_feature_set
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = ANDR_WIFI_SUBCMD_GET_FEATURE_SET_MATRIX
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_get_feature_set_matrix
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = ANDR_WIFI_RANDOM_MAC_OUI
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_rand_mac_oui
	},
#ifdef CUSTOM_FORCE_NODFS_FLAG
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = ANDR_WIFI_NODFS_CHANNELS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_nodfs_flag
	},
#endif /* CUSTOM_FORCE_NODFS_FLAG */
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = ANDR_WIFI_SET_COUNTRY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_country
	},
#ifdef LINKSTAT_SUPPORT
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = LSTATS_SUBCMD_GET_INFO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_lstats_get_info
	},
#endif /* LINKSTAT_SUPPORT */

#ifdef GSCAN_SUPPORT
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = GSCAN_SUBCMD_SET_EPNO_SSID
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_epno_cfg

	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_SET_LAZY_ROAM_PARAMS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_lazy_roam_cfg

	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_ENABLE_LAZY_ROAM
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_enable_lazy_roam

	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_SET_BSSID_PREF
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_bssid_pref

	},
#endif /* GSCAN_SUPPORT */
#if defined(GSCAN_SUPPORT) || defined(ROAMEXP_SUPPORT)
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_SET_SSID_WHITELIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_ssid_whitelist

	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_SET_BSSID_BLACKLIST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_bssid_blacklist
	},
#endif /* GSCAN_SUPPORT || ROAMEXP_SUPPORT */
#ifdef ROAMEXP_SUPPORT
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_FW_ROAM_POLICY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_fw_roaming_state
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_ROAM_CAPABILITY
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_fw_roam_get_capability
	},
#endif /* ROAMEXP_SUPPORT */
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_VER
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_version
	},
#ifdef DHD_LOG_DUMP
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_FILE_DUMP_BUF
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_file_dump
	},
#endif /* DHD_LOG_DUMP */

#ifdef DEBUGABILITY
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_TRIGGER_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_trigger_mem_dump
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_MEM_DUMP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_mem_dump
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_START_LOGGING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_start_logging
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_RESET_LOGGING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_reset_logging
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_RING_STATUS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_ring_status
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_RING_DATA
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_ring_data
	},
#endif /* DEBUGABILITY */
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_FEATURE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_feature
	},
#ifdef DBG_PKT_MON
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_START_PKT_FATE_MONITORING
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_start_pkt_fate_monitoring
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_TX_PKT_FATES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_tx_pkt_fates
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_RX_PKT_FATES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_dbg_get_rx_pkt_fates
	},
#endif /* DBG_PKT_MON */
#ifdef KEEP_ALIVE
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_start_mkeep_alive
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_stop_mkeep_alive
	},
#endif /* KEEP_ALIVE */
#ifdef WL_NAN
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_ENABLE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_start_handler
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DISABLE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_stop_handler
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_CONFIG
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_config_handler
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_REQUEST_PUBLISH
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_req_publish
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_REQUEST_SUBSCRIBE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_req_subscribe
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_CANCEL_PUBLISH
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_cancel_publish
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_CANCEL_SUBSCRIBE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_cancel_subscribe
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_TRANSMIT
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_transmit
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_get_capablities
	},

	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DATA_PATH_IFACE_CREATE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_data_path_iface_create
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DATA_PATH_IFACE_DELETE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_data_path_iface_delete
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DATA_PATH_REQUEST
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_data_path_request
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DATA_PATH_RESPONSE
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_data_path_response
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DATA_PATH_END
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_data_path_end
	},
#ifdef WL_NAN_DISC_CACHE
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_DATA_PATH_SEC_INFO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_data_path_sec_info
	},
#endif /* WL_NAN_DISC_CACHE */
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = NAN_WIFI_SUBCMD_VERSION_INFO
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_nan_version_info
	},
#endif /* WL_NAN */
#if defined(PKT_FILTER_SUPPORT) && defined(APF)
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = APF_SUBCMD_GET_CAPABILITIES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_apf_get_capabilities
	},

	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = APF_SUBCMD_SET_FILTER
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_apf_set_filter
	},
#endif /* PKT_FILTER_SUPPORT && APF */
#ifdef NDO_CONFIG_SUPPORT
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_CONFIG_ND_OFFLOAD
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_configure_nd_offload
	},
#endif /* NDO_CONFIG_SUPPORT */
#ifdef RSSI_MONITOR_SUPPORT
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_SET_RSSI_MONITOR
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_rssi_monitor
	},
#endif /* RSSI_MONITOR_SUPPORT */
#ifdef DHDTCPACK_SUPPRESS
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = WIFI_SUBCMD_CONFIG_TCPACK_SUP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_tcpack_sup_mode
	},
#endif /* DHDTCPACK_SUPPRESS */
#ifdef DHD_WAKE_STATUS
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_GET_WAKE_REASON_STATS
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_get_wake_reason_stats
	},
#endif /* DHD_WAKE_STATUS */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0))
	{
		{
			.vendor_id = OUI_BRCM,
			.subcmd = BRCM_VENDOR_SCMD_SET_PMK
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_pmk
	},
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(4, 13, 0) */
	{
		{
			.vendor_id = OUI_BRCM,
			.subcmd = BRCM_VENDOR_SCMD_GET_FEATURES
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_get_driver_feature
	},
#if defined(WL_CFG80211) && defined(DHD_FILE_DUMP_EVENT)
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_FILE_DUMP_DONE_IND
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_notify_dump_completion
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_SET_HAL_START
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_set_hal_started
	},
	{
		{
			.vendor_id = OUI_GOOGLE,
			.subcmd = DEBUG_SET_HAL_STOP
		},
		.flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
		.doit = wl_cfgvendor_stop_hal
	}
#endif /* WL_CFG80211 && DHD_FILE_DUMP_EVENT */
};

static const struct  nl80211_vendor_cmd_info wl_vendor_events [] = {
		{ OUI_BRCM, BRCM_VENDOR_EVENT_UNSPEC },
		{ OUI_BRCM, BRCM_VENDOR_EVENT_PRIV_STR },
		{ OUI_GOOGLE, GOOGLE_GSCAN_SIGNIFICANT_EVENT },
		{ OUI_GOOGLE, GOOGLE_GSCAN_GEOFENCE_FOUND_EVENT },
		{ OUI_GOOGLE, GOOGLE_GSCAN_BATCH_SCAN_EVENT },
		{ OUI_GOOGLE, GOOGLE_SCAN_FULL_RESULTS_EVENT },
		{ OUI_GOOGLE, GOOGLE_RTT_COMPLETE_EVENT },
		{ OUI_GOOGLE, GOOGLE_SCAN_COMPLETE_EVENT },
		{ OUI_GOOGLE, GOOGLE_GSCAN_GEOFENCE_LOST_EVENT },
		{ OUI_GOOGLE, GOOGLE_SCAN_EPNO_EVENT },
		{ OUI_GOOGLE, GOOGLE_DEBUG_RING_EVENT },
		{ OUI_GOOGLE, GOOGLE_FW_DUMP_EVENT },
		{ OUI_GOOGLE, GOOGLE_PNO_HOTSPOT_FOUND_EVENT },
		{ OUI_GOOGLE, GOOGLE_RSSI_MONITOR_EVENT },
		{ OUI_GOOGLE, GOOGLE_MKEEP_ALIVE_EVENT },
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_ENABLED},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_DISABLED},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_SUBSCRIBE_MATCH},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_REPLIED},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_PUBLISH_TERMINATED},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_SUBSCRIBE_TERMINATED},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_DE_EVENT},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_FOLLOWUP},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_TRANSMIT_FOLLOWUP_IND},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_DATA_REQUEST},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_DATA_CONFIRMATION},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_DATA_END},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_BEACON},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_SDF},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_TCA},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_SUBSCRIBE_UNMATCH},
		{ OUI_GOOGLE, GOOGLE_NAN_EVENT_UNKNOWN},
		{ OUI_GOOGLE, GOOGLE_ROAM_EVENT_START},
		{ OUI_BRCM, BRCM_VENDOR_EVENT_HANGED},
		{ OUI_BRCM, BRCM_VENDOR_EVENT_SAE_KEY},
		{ OUI_BRCM, BRCM_VENDOR_EVENT_BEACON_RECV},
		{ OUI_BRCM, BRCM_VENDOR_EVENT_PORT_AUTHORIZED},
		{ OUI_GOOGLE, GOOGLE_FILE_DUMP_EVENT },
};

int wl_cfgvendor_attach(struct wiphy *wiphy, dhd_pub_t *dhd)
{

	WL_INFORM_MEM(("Vendor: Register BRCM cfg80211 vendor cmd(0x%x) interface \n",
		NL80211_CMD_VENDOR));

	wiphy->vendor_commands	= wl_vendor_cmds;
	wiphy->n_vendor_commands = ARRAY_SIZE(wl_vendor_cmds);
	wiphy->vendor_events	= wl_vendor_events;
	wiphy->n_vendor_events	= ARRAY_SIZE(wl_vendor_events);

#ifdef DEBUGABILITY
	dhd_os_dbg_register_callback(FW_VERBOSE_RING_ID, wl_cfgvendor_dbg_ring_send_evt);
	dhd_os_dbg_register_callback(DHD_EVENT_RING_ID, wl_cfgvendor_dbg_ring_send_evt);
#endif /* DEBUGABILITY */
#ifdef DHD_LOG_DUMP
	dhd_os_dbg_register_urgent_notifier(dhd, wl_cfgvendor_dbg_send_file_dump_evt);
#endif /* DHD_LOG_DUMP */
	return 0;
}

int wl_cfgvendor_detach(struct wiphy *wiphy)
{
	WL_INFORM_MEM(("Vendor: Unregister BRCM cfg80211 vendor interface \n"));

	wiphy->vendor_commands  = NULL;
	wiphy->vendor_events    = NULL;
	wiphy->n_vendor_commands = 0;
	wiphy->n_vendor_events  = 0;

	return 0;
}
#endif /* (LINUX_VERSION_CODE > KERNEL_VERSION(3, 13, 0)) || defined(WL_VENDOR_EXT_SUPPORT) */

#ifdef WL_CFGVENDOR_SEND_HANG_EVENT
void
wl_cfgvendor_send_hang_event(struct net_device *dev, u16 reason, char *string, int hang_info_cnt)
{
	struct bcm_cfg80211 *cfg = wl_get_cfg(dev);
	struct wiphy *wiphy;
	char *hang_info;
	int len = 0;
	int bytes_written;
	uint32 dummy_data = 0;
	int reason_hang_info = 0;
	int cnt = 0;
	dhd_pub_t *dhd;
	int hang_reason_mismatch = FALSE;

	if (!cfg || !cfg->wdev) {
		WL_ERR(("cfg=%p wdev=%p\n", cfg, (cfg ? cfg->wdev : NULL)));
		return;
	}

	wiphy = cfg->wdev->wiphy;

	if (!wiphy) {
		WL_ERR(("wiphy is NULL\n"));
		return;
	}

	hang_info = MALLOCZ(cfg->osh, VENDOR_SEND_HANG_EXT_INFO_LEN);
	if (hang_info == NULL) {
		WL_ERR(("alloc hang_info failed\n"));
		return;
	}

	dhd = (dhd_pub_t *)(cfg->pub);

#ifdef WL_BCNRECV
	/* check fakeapscan in progress then stop scan */
	if (cfg->bcnrecv_info.bcnrecv_state == BEACON_RECV_STARTED) {
		wl_android_bcnrecv_stop(dev, WL_BCNRECV_HANG);
	}
#endif /* WL_BCNRECV */
	sscanf(string, "%d", &reason_hang_info);
	bytes_written = 0;
	len = VENDOR_SEND_HANG_EXT_INFO_LEN - bytes_written;
	if (strlen(string) == 0 || (reason_hang_info != reason)) {
		WL_ERR(("hang reason mismatch: string len %d reason_hang_info %d\n",
			(int)strlen(string), reason_hang_info));
		hang_reason_mismatch = TRUE;
		if (dhd) {
			get_debug_dump_time(dhd->debug_dump_time_hang_str);
			copy_debug_dump_time(dhd->debug_dump_time_str,
					dhd->debug_dump_time_hang_str);
		}
		bytes_written += scnprintf(&hang_info[bytes_written], len,
				"%d %d %s %08x %08x %08x %08x %08x %08x %08x",
				reason, VENDOR_SEND_HANG_EXT_INFO_VER,
				dhd->debug_dump_time_hang_str,
				0, 0, 0, 0, 0, 0, 0);
		if (dhd) {
			clear_debug_dump_time(dhd->debug_dump_time_hang_str);
		}
	} else {
		bytes_written += scnprintf(&hang_info[bytes_written], len, "%s", string);
	}

	WL_ERR(("hang reason: %d info cnt: %d\n", reason, hang_info_cnt));

	if (hang_reason_mismatch == FALSE) {
		cnt = hang_info_cnt;
	} else {
		cnt = HANG_FIELD_MISMATCH_CNT;
	}

	while (cnt < HANG_FIELD_CNT_MAX) {
		len = VENDOR_SEND_HANG_EXT_INFO_LEN - bytes_written;
		if (len <= 0) {
			break;
		}
		bytes_written += scnprintf(&hang_info[bytes_written], len,
				"%c%08x", HANG_RAW_DEL, dummy_data);
		cnt++;
	}

	WL_ERR(("hang info cnt: %d len: %d\n", cnt, (int)strlen(hang_info)));
	WL_ERR(("hang info data: %s\n", hang_info));

	wl_cfgvendor_send_async_event(wiphy,
			bcmcfg_to_prmry_ndev(cfg), BRCM_VENDOR_EVENT_HANGED,
			hang_info, (int)strlen(hang_info));

	memset(string, 0, VENDOR_SEND_HANG_EXT_INFO_LEN);

	if (hang_info) {
		MFREE(cfg->osh, hang_info, VENDOR_SEND_HANG_EXT_INFO_LEN);
	}

#ifdef DHD_LOG_DUMP
	dhd_logdump_cookie_save(dhd, dhd->debug_dump_time_hang_str, "HANG");
#endif /*  DHD_LOG_DUMP */

	if (dhd) {
		clear_debug_dump_time(dhd->debug_dump_time_str);
	}
}

void
wl_copy_hang_info_if_falure(struct net_device *dev, u16 reason, s32 ret)
{
	struct bcm_cfg80211 *cfg = NULL;
	dhd_pub_t *dhd;
	s32 err = 0;
	char ioctl_buf[WLC_IOCTL_SMLEN];
	memuse_info_t mu;
	int bytes_written = 0;
	int remain_len = 0;

	if (!dev) {
		WL_ERR(("dev is null"));
		return;

	}

	cfg = wl_get_cfg(dev);
	if (!cfg) {
		WL_ERR(("dev=%p cfg=%p\n", dev, cfg));
		return;
	}

	dhd = (dhd_pub_t *)(cfg->pub);

	if (!dhd || !dhd->hang_info) {
		WL_ERR(("%s dhd=%p hang_info=%p\n", __FUNCTION__,
			dhd, (dhd ? dhd->hang_info : NULL)));
		return;
	}

	err = wldev_iovar_getbuf_bsscfg(dev, "memuse",
			NULL, 0, ioctl_buf, WLC_IOCTL_SMLEN, 0, NULL);
	if (unlikely(err)) {
		WL_ERR(("error (%d)\n", err));
		return;
	}

	memcpy(&mu, ioctl_buf, sizeof(memuse_info_t));

	if (mu.len >= sizeof(memuse_info_t)) {
		WL_ERR(("Heap Total: %d(%dK)\n", mu.arena_size, KB(mu.arena_size)));
		WL_ERR(("Free: %d(%dK), LWM: %d(%dK)\n",
			mu.arena_free, KB(mu.arena_free),
			mu.free_lwm, KB(mu.free_lwm)));
		WL_ERR(("In use: %d(%dK), HWM: %d(%dK)\n",
			mu.inuse_size, KB(mu.inuse_size),
			mu.inuse_hwm, KB(mu.inuse_hwm)));
		WL_ERR(("Malloc failure count: %d\n", mu.mf_count));
	}

	memset(dhd->hang_info, 0, VENDOR_SEND_HANG_EXT_INFO_LEN);
	remain_len = VENDOR_SEND_HANG_EXT_INFO_LEN - bytes_written;

	get_debug_dump_time(dhd->debug_dump_time_hang_str);
	copy_debug_dump_time(dhd->debug_dump_time_str, dhd->debug_dump_time_hang_str);

	bytes_written += scnprintf(&dhd->hang_info[bytes_written], remain_len,
			"%d %d %s %d %d %d %d %d %08x %08x",
			reason, VENDOR_SEND_HANG_EXT_INFO_VER,
			dhd->debug_dump_time_hang_str,
			ret, mu.arena_size, mu.arena_free, mu.inuse_size, mu.mf_count, 0, 0);

	dhd->hang_info_cnt = HANG_FIELD_IF_FAILURE_CNT;

	clear_debug_dump_time(dhd->debug_dump_time_hang_str);

	return;
}
#endif /* WL_CFGVENDOR_SEND_HANG_EVENT */
