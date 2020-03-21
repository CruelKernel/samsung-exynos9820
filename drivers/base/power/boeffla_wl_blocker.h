/*
 * Author: andip71, 01.09.2017
 *
 * Version 1.1.0
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define BOEFFLA_WL_BLOCKER_VERSION	"1.1.0"

#define LIST_WL_DEFAULT			"19030000.decon_f;ssp_comm_wake_lock;GPSD;ssp_wake_lock;tsp_wakelock;umts_ipc0;umts_ipc1;wlan_rx_wake;qcom_rx_wakelock;wlan;wlan_wow_wl;wlan_extscan_wl;netmgr_wl;NETLINK;IPA_WS;[timerfd];wlan_ipa;wlan_pno_wl;wcnss_filter_lock;DIAG_WS;qcom_sap_wakelock"

#define LENGTH_LIST_WL			255
#define LENGTH_LIST_WL_DEFAULT		255
#define LENGTH_LIST_WL_SEARCH		LENGTH_LIST_WL + LENGTH_LIST_WL_DEFAULT + 5
