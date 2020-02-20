/*
 * Wifi dongle status Filter and Report
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
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id: dhd_event_log_filter.h 789224 2018-11-15 13:43:23Z $
 */

#ifndef dhd_event_log_filter_h
#define dhd_event_log_filter_h
#include <dhd.h>
#include <event_log_tag.h>

typedef struct {
	uint16	version;
	uint8	htr_type;	/* from wl_slice_hist_XX_stats_xtlv_id */
	uint8	htr_num;	/* number of elements in htr_running or htr_rc */
	uint32	htr_rn_last;	/* last reasons along with seq, etc */
	uint32	htr_rn_ts_last;	/* last time stamps corr to htr_rn_last */
	uint32	htr_rn_prev;	/* last reasons along with seq, etc */
	uint32	htr_rn_ts_prev;	/* last time stamps corr to htr_rn_prev */
	uint32	htr_rc_max;	/* largest toss reasons and counts */
	uint32	htr_rc_ts_max;	/* latest time stamp corr to htr_rc_max */
	uint32	htr_rc_secnd;	/* second largest toss reasons and counts */
	uint32	htr_rc_ts_secnd;	/* latest time stamps corr to htr_rc_second */
} evt_hist_compact_toss_stats_v1_t;

int dhd_event_log_filter_init(dhd_pub_t *dhdp, uint8 *buf, uint32 buf_size);
void dhd_event_log_filter_deinit(dhd_pub_t *dhdp);
void dhd_event_log_filter_event_handler(
	dhd_pub_t *dhdp, prcd_event_log_hdr_t *plog_hdr, uint32 *data);

void dhd_event_log_filter_notify_connect_request(dhd_pub_t *dhdp, uint8 *bssid, int channel);
void dhd_event_log_filter_notify_connect_done(dhd_pub_t *dhdp, uint8 *bssid, int roam);
#endif /* dhd_event_log_filter_h */
