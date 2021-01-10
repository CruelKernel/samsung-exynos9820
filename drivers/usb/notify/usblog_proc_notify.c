// SPDX-License-Identifier: GPL-2.0
/*
 *  drivers/usb/notify/usblog_proc_notify.c
 *
 * Copyright (C) 2016-2020 Samsung, Inc.
 * Author: Dongrak Shin <dongrak.shin@samsung.com>
 *
 */

 /* usb notify layer v3.5 */

 #define pr_fmt(fmt) "usb_notify: " fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/security.h>
#include <linux/syscalls.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/seq_file.h>
/* clock.h should be included since kernel 4.14 */
#include <linux/sched/clock.h>
#include <linux/usb_notify.h>

#define USBLOG_MAX_BUF_SIZE	(1 << 6) /* 64 */
#define USBLOG_MAX_BUF2_SIZE	(1 << 7) /* 128 */
#define USBLOG_MAX_BUF3_SIZE	(1 << 8) /* 256 */
#define USBLOG_MAX_BUF4_SIZE	(1 << 9) /* 512 */
#define USBLOG_MAX_STRING_SIZE	(1 << 4) /* 16 */
#define USBLOG_CMP_INDEX	3
#define USBLOG_MAX_STORE_PORT	(1 << 6) /* 64 */

#define USBLOG_CCIC_BUFFER_SIZE	USBLOG_MAX_BUF4_SIZE
#define USBLOG_MODE_BUFFER_SIZE	USBLOG_MAX_BUF_SIZE
#define USBLOG_STATE_BUFFER_SIZE	USBLOG_MAX_BUF3_SIZE
#define USBLOG_EVENT_BUFFER_SIZE	USBLOG_MAX_BUF_SIZE
#define USBLOG_PORT_BUFFER_SIZE		USBLOG_MAX_BUF2_SIZE
#define USBLOG_PCM_BUFFER_SIZE		USBLOG_MAX_BUF_SIZE
#define USBLOG_EXTRA_BUFFER_SIZE	USBLOG_MAX_BUF2_SIZE

struct ccic_buf {
	unsigned long long ts_nsec;
	int cc_type;
	uint64_t noti;
};

struct mode_buf {
	unsigned long long ts_nsec;
	char usbmode_str[USBLOG_MAX_STRING_SIZE];
};

struct state_buf {
	unsigned long long ts_nsec;
	int usbstate;
};

struct event_buf {
	unsigned long long ts_nsec;
	unsigned long event;
	int enable;
};

struct port_buf {
	unsigned long long ts_nsec;
	int type;
	uint16_t param1;
	uint16_t param2;
	uint16_t count;
};

struct port_count {
	uint16_t vid;
	uint16_t pid;
	uint16_t count;
};

struct pcm_buf {
	unsigned long long ts_nsec;
	int type;
	int enable;
};

struct extra_buf {
	unsigned long long ts_nsec;
	int event;
};

struct usblog_buf {
	unsigned long long ccic_count;
	unsigned long long mode_count;
	unsigned long long state_count;
	unsigned long long event_count;
	unsigned long long port_count;
	unsigned long long extra_count;
	unsigned long ccic_index;
	unsigned long mode_index;
	unsigned long state_index;
	unsigned long event_index;
	unsigned long port_index;
	unsigned long extra_index;
	struct ccic_buf ccic_buffer[USBLOG_CCIC_BUFFER_SIZE];
	struct mode_buf mode_buffer[USBLOG_MODE_BUFFER_SIZE];
	struct state_buf state_buffer[USBLOG_STATE_BUFFER_SIZE];
	struct event_buf event_buffer[USBLOG_EVENT_BUFFER_SIZE];
	struct port_buf port_buffer[USBLOG_PORT_BUFFER_SIZE];
	struct port_count store_port_cnt[USBLOG_MAX_STORE_PORT];
	struct extra_buf extra_buffer[USBLOG_EXTRA_BUFFER_SIZE];
};

struct usblog_vm_buf {
	unsigned long long pcm_count;
	unsigned long pcm_index;
	struct pcm_buf pcm_buffer[USBLOG_PCM_BUFFER_SIZE];
};

struct ccic_version {
	unsigned char hw_version[4];
	unsigned char sw_main[3];
	unsigned char sw_boot;
};

struct usblog_root_str {
	struct usblog_buf *usblog_buffer;
	struct usblog_vm_buf *usblog_vm_buffer;
	struct ccic_version ccic_ver;
	struct ccic_version ccic_bin_ver;
	spinlock_t usblog_lock;
	int init;
};

struct ccic_type {
	uint64_t src:4;
	uint64_t dest:4;
	uint64_t id:8;
	uint64_t sub1:16;
	uint64_t sub2:16;
	uint64_t sub3:16;
};

static struct usblog_root_str usblog_root;

static const char *usbstate_string(enum usblog_state usbstate)
{
	switch (usbstate) {
	case NOTIFY_CONFIGURED:
		return "CONFIGURED";
	case NOTIFY_CONNECTED:
		return "CONNECTED";
	case NOTIFY_DISCONNECTED:
		return "DISCONNECTED";
	case NOTIFY_RESET:
		return "RESET";
	case NOTIFY_RESET_FULL:
		return "RESET : FULL";
	case NOTIFY_RESET_HIGH:
		return "RESET: HIGH";
	case NOTIFY_RESET_SUPER:
		return "RESET : SUPER";
	case NOTIFY_PULLUP:
		return "VBUS_PULLUP (EN OR DIS)";
	case NOTIFY_PULLUP_ENABLE:
		return "VBUS_PULLUP_EN";
	case NOTIFY_PULLUP_EN_SUCCESS:
		return "VBUS_PULLUP_EN : S";
	case NOTIFY_PULLUP_EN_FAIL:
		return "VBUS_PULLUP_EN : F";
	case NOTIFY_PULLUP_DISABLE:
		return "VBUS_PULLUP_DIS";
	case NOTIFY_PULLUP_DIS_SUCCESS:
		return "VBUS_PULLUP_DIS : S";
	case NOTIFY_PULLUP_DIS_FAIL:
		return "VBUS_PULLUP_DIS : F";
	case NOTIFY_VBUS_SESSION:
		return "VBUS_SESSION (EN OR DIS)";
	case NOTIFY_VBUS_SESSION_ENABLE:
		return "VBUS_SESSION_EN";
	case NOTIFY_VBUS_EN_SUCCESS:
		return "VBUS_SESSION_EN : S";
	case NOTIFY_VBUS_EN_FAIL:
		return "VBUS_SESSION_EN : F";
	case NOTIFY_VBUS_SESSION_DISABLE:
		return "VBUS_SESSIOIN_DIS";
	case NOTIFY_VBUS_DIS_SUCCESS:
		return "VBUS_SESSION_DIS : S";
	case NOTIFY_VBUS_DIS_FAIL:
		return "VBUS_SESSION_DIS : F";
	case NOTIFY_ACCSTART:
		return "ACCSTART";
	case NOTIFY_HIGH:
		return "HIGH SPEED";
	case NOTIFY_SUPER:
		return "SUPER SPEED";
	case NOTIFY_GET_DES:
		return "GET_DES";
	case NOTIFY_SET_CON:
		return "SET_CON";
	case NOTIFY_CONNDONE_SSP:
		return "CONNDONE SSP";
	case NOTIFY_CONNDONE_SS:
		return "CONNDONE SS";
	case NOTIFY_CONNDONE_HS:
		return "CONNDONE HS";
	case NOTIFY_CONNDONE_FS:
		return "CONNDONE FS";
	case NOTIFY_CONNDONE_LS:
		return "CONNDONE LS";
	default:
		return "UNDEFINED";
	}
}

static const char *usbstatus_string(enum usblog_status usbstatus)
{
	switch (usbstatus) {
	case NOTIFY_DETACH:
		return "DETACH";
	case NOTIFY_ATTACH_DFP:
		return "ATTACH_DFP";
	case NOTIFY_ATTACH_UFP:
		return "ATTACH_UFP";
	case NOTIFY_ATTACH_DRP:
		return "ATTACH_DRP";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_dev_string(enum ccic_device dev)
{
	switch (dev) {
	case NOTIFY_DEV_INITIAL:
		return "INITIAL";
	case NOTIFY_DEV_USB:
		return "USB";
	case NOTIFY_DEV_BATTERY:
		return "BATTERY";
	case NOTIFY_DEV_PDIC:
		return "PDIC";
	case NOTIFY_DEV_MUIC:
		return "MUIC";
	case NOTIFY_DEV_CCIC:
		return "CCIC";
	case NOTIFY_DEV_MANAGER:
		return "MANAGER";
	case NOTIFY_DEV_DP:
		return "DP";
	case NOTIFY_DEV_USB_DP:
		return "USB_DP";
	case NOTIFY_DEV_SUB_BATTERY:
		return "BATTERY2";
	case NOTIFY_DEV_SECOND_MUIC:
		return "MUIC2";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_id_string(enum ccic_id id)
{
	switch (id) {
	case NOTIFY_ID_INITIAL:
		return "ID_INITIAL";
	case NOTIFY_ID_ATTACH:
		return "ID_CONNECT";
	case NOTIFY_ID_RID:
		return "ID_RID";
	case NOTIFY_ID_USB:
		return "ID_USB";
	case NOTIFY_ID_POWER_STATUS:
		return "ID_POWER_STATUS";
	case NOTIFY_ID_WATER:
		return "ID_WATER";
	case NOTIFY_ID_VCONN:
		return "ID_VCONN";
	case NOTIFY_ID_OTG:
		return "ID_OTG";
	case NOTIFY_ID_TA:
		return "ID_TA";
	case NOTIFY_ID_DP_CONNECT:
		return "ID_DP_CONNECT";
	case NOTIFY_ID_DP_HPD:
		return "ID_DP_HPD";
	case NOTIFY_ID_DP_LINK_CONF:
		return "ID_DP_LINK_CONF";
	case NOTIFY_ID_USB_DP:
		return "ID_USB_DP";
	case NOTIFY_ID_ROLE_SWAP:
		return "ID_ROLE_SWAP";
	case NOTIFY_ID_FAC:
		return "ID_FAC";
	case NOTIFY_ID_CC_PIN_STATUS:
		return "ID_PIN_STATUS";
	case NOTIFY_ID_WATER_CABLE:
		return "ID_WATER_CABLE";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_rid_string(enum ccic_rid rid)
{
	switch (rid) {
	case NOTIFY_RID_UNDEFINED:
		return "RID_UNDEFINED";
#if defined(CONFIG_USB_CCIC_NOTIFIER_USING_QC)
	case NOTIFY_RID_GND:
		return "RID_GND";
	case NOTIFY_RID_056K:
		return "RID_056K";
#else
	case NOTIFY_RID_000K:
		return "RID_000K";
	case NOTIFY_RID_001K:
		return "RID_001K";
#endif
	case NOTIFY_RID_255K:
		return "RID_255K";
	case NOTIFY_RID_301K:
		return "RID_301K";
	case NOTIFY_RID_523K:
		return "RID_523K";
	case NOTIFY_RID_619K:
		return "RID_619K";
	case NOTIFY_RID_OPEN:
		return "RID_OPEN";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_con_string(enum ccic_con con)
{
	switch (con) {
	case NOTIFY_CON_DETACH:
		return "DETACHED";
	case NOTIFY_CON_ATTACH:
		return "ATTACHED";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_rprd_string(enum ccic_rprd rprd)
{
	switch (rprd) {
	case NOTIFY_RD:
		return "RD";
	case NOTIFY_RP:
		return "RP";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_rpstatus_string(enum ccic_rpstatus rprd)
{
	switch (rprd) {
	case NOTIFY_RP_NONE:
		return "NONE";
	case NOTIFY_RP_56K:
		return "RP_56K";
	case NOTIFY_RP_22K:
		return "RP_22K";
	case NOTIFY_RP_10K:
		return "RP_10K";
	case NOTIFY_RP_ABNORMAL:
		return "RP_ABNORMAL";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_hpd_string(enum ccic_hpd hpd)
{
	switch (hpd) {
	case NOTIFY_HPD_LOW:
		return "LOW";
	case NOTIFY_HPD_HIGH:
		return "HIGH";
	case NOTIFY_HPD_IRQ:
		return "IRQ";
	default:
		return "UNDEFINED";
	}
}

static const char *ccic_pin_string(enum ccic_pin_assignment pin)
{
	switch (pin) {
	case NOTIFY_DP_PIN_UNKNOWN:
		return "UNKNOWN";
	case NOTIFY_DP_PIN_A:
		return "DP_PIN_A";
	case NOTIFY_DP_PIN_B:
		return "DP_PIN_B";
	case NOTIFY_DP_PIN_C:
		return "DP_PIN_C";
	case NOTIFY_DP_PIN_D:
		return "DP_PIN_D";
	case NOTIFY_DP_PIN_E:
		return "DP_PIN_E";
	case NOTIFY_DP_PIN_F:
		return "DP_PIN_F";
	default:
		return "UNKNOWN";
	}
}

static const char *ccic_alternatemode_string(uint64_t id)
{
	if ((id & ALTERNATE_MODE_READY) && (id & ALTERNATE_MODE_START))
		return "READY & START";
	else if ((id & ALTERNATE_MODE_READY) && (id & ALTERNATE_MODE_STOP))
		return "READY & STOP";
	else if (id & ALTERNATE_MODE_READY)
		return "MODE READY";
	else if (id & ALTERNATE_MODE_START)
		return "START";
	else if (id & ALTERNATE_MODE_STOP)
		return "STOP";
	else if (id & ALTERNATE_MODE_RESET)
		return "RESET";
	else
		return "UNDEFINED";
}

static const char *ccic_pinstatus_string(enum ccic_pin_status pinstatus)
{
	switch (pinstatus) {
	case NOTIFY_PIN_NOTERMINATION:
		return "NO TERMINATION";
	case NOTIFY_PIN_CC1_ACTIVE:
		return "CC1_ACTIVE";
	case NOTIFY_PIN_CC2_ACTIVE:
		return "CC2_ACTIVE";
	case NOTIFY_PIN_AUDIO_ACCESSORY:
		return "AUDIO_ACCESSORY";
	default:
		return "ETC";
	}
}

static const char *extra_string(enum extra event)
{
	switch (event) {
	case NOTIFY_EXTRA_USBKILLER:
		return "USB_KILLER";
	case NOTIFY_EXTRA_HARDRESET_SENT:
		return "PDIC HARDRESET_SENT";
	case NOTIFY_EXTRA_HARDRESET_RECEIVED:
		return "PDIC HARDRESET_RECEIVED";
	case NOTIFY_EXTRA_SYSERROR_BOOT_WDT:
		return "PDIC WDT RESET";
	case NOTIFY_EXTRA_SYSMSG_BOOT_POR:
		return "PDIC POR RESET";
	case NOTIFY_EXTRA_SYSMSG_CC_SHORT:
		return "CC VBUS SHORT";
	case NOTIFY_EXTRA_SYSMSG_SBU_GND_SHORT:
		return "SBU GND SHORT";
	case NOTIFY_EXTRA_SYSMSG_SBU_VBUS_SHORT:
		return "SBU VBUS SHORT";
	case NOTIFY_EXTRA_UVDM_TIMEOUT:
		return "UVDM TIMEOUT";
	case NOTIFY_EXTRA_CCOPEN_REQ_SET:
		return "CC OPEN SET";
	case NOTIFY_EXTRA_CCOPEN_REQ_CLEAR:
		return "CC OPEN CLEAR";
	case NOTIFY_EXTRA_USB_ANALOGAUDIO:
		return "USB ANALOG AUDIO";
	case NOTIFY_EXTRA_USBHOST_OVERCURRENT:
		return "USB HOST OVERCURRENT";
	case NOTIFY_EXTRA_ROOTHUB_SUSPEND_FAIL:
		return "ROOTHUB SUSPEND FAIL";
	case NOTIFY_EXTRA_PORT_SUSPEND_FAIL:
		return "PORT SUSPEND FAIL";
	case NOTIFY_EXTRA_PORT_SUSPEND_WAKEUP_FAIL:
		return "PORT REMOTE WAKEUP FAIL";
	case NOTIFY_EXTRA_PORT_SUSPEND_LTM_FAIL:
		return "PORT LTM FAIL";
	default:
		return "ETC";
	}
}

static void print_ccic_event(struct seq_file *m, unsigned long long ts,
		unsigned long rem_nsec, int cc_type, uint64_t *noti)
{
	struct ccic_type type = *(struct ccic_type *)noti;
	int cable = type.sub3;

	switch (cc_type) {
	case NOTIFY_FUNCSTATE:
		seq_printf(m, "[%5lu.%06lu] function state = %llu\n",
			(unsigned long)ts, rem_nsec / 1000, *noti);
		break;
	case NOTIFY_ALTERNATEMODE:
		seq_printf(m, "[%5lu.%06lu] ccic alternate mode is %s 0x%04llx\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_alternatemode_string(*noti), *noti);
		break;
	case NOTIFY_CCIC_EVENT:
		if (type.id == NOTIFY_ID_ATTACH) {
			if (type.src == NOTIFY_DEV_MUIC
				|| type.src == NOTIFY_DEV_SECOND_MUIC)
				seq_printf(m,
					"[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s rprd=%s cable=%d %s\n",
				(unsigned long)ts, rem_nsec / 1000,
				ccic_id_string(type.id),
				ccic_dev_string(type.src),
				ccic_dev_string(type.dest),
				ccic_rprd_string(type.sub2),
				cable, ccic_con_string(type.sub1));
			else
				seq_printf(m,
					"[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s rprd=%s rp status=%s %s\n",
				(unsigned long)ts, rem_nsec / 1000,
				ccic_id_string(type.id),
				ccic_dev_string(type.src),
				ccic_dev_string(type.dest),
				ccic_rprd_string(type.sub2),
				ccic_rpstatus_string(type.sub3),
				ccic_con_string(type.sub1));
		} else if (type.id  == NOTIFY_ID_RID)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s rid=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_rid_string(type.sub1));
		else if (type.id  == NOTIFY_ID_USB)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s status=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			usbstatus_string(type.sub2));
		else if (type.id  == NOTIFY_ID_POWER_STATUS)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_con_string(type.sub1));
		else if (type.id  == NOTIFY_ID_WATER)
			seq_printf(m, "[%5lu.%06lu] ccic notify:   id=%s src=%s dest=%s    %s detected\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1 ? "WATER":"DRY");
		else if (type.id  == NOTIFY_ID_VCONN)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest));
		else if (type.id  == NOTIFY_ID_OTG)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub2 ? "Enable":"Disable");
		else if (type.id  == NOTIFY_ID_TA)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_con_string(type.sub1));
		else if (type.id  == NOTIFY_ID_DP_CONNECT)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s 0x%04x/0x%04x %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub2,
			type.sub3,
			ccic_con_string(type.sub1));
		else if (type.id  == NOTIFY_ID_DP_HPD)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s hpd=%s irq=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_hpd_string(type.sub1),
			type.sub2 ? "VALID":"NONE");
		else if (type.id  == NOTIFY_ID_DP_LINK_CONF)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s PIN-assign=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_pin_string(type.sub1));
		else if (type.id  == NOTIFY_ID_USB_DP)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s CON=%d HS=%d\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1,
			type.sub2);
		else if (type.id  == NOTIFY_ID_ROLE_SWAP)
			seq_printf(m, "[%5lu.%06lu] ccic notify:	id=%s src=%s dest=%s sub1=%d sub2=%d\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1,
			type.sub2);
		else if (type.id  == NOTIFY_ID_FAC)
			seq_printf(m, "[%5lu.%06lu] ccic notify:	id=%s src=%s dest=%s ErrState=%d\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1);
		else if (type.id == NOTIFY_ID_CC_PIN_STATUS)
			seq_printf(m, "[%5lu.%06lu] ccic notify:	id=%s src=%s dest=%s pinstatus=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_pinstatus_string(type.sub1));
		else if (type.id == NOTIFY_ID_WATER_CABLE)
			seq_printf(m, "[%5lu.%06lu] ccic notify:	id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_con_string(type.sub1));
		else
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s rprd=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_rprd_string(type.sub2),
			ccic_con_string(type.sub1));
		break;
	case NOTIFY_MANAGER:
		if (type.id == NOTIFY_ID_ATTACH) {
			if (type.src == NOTIFY_DEV_MUIC
				|| type.src == NOTIFY_DEV_SECOND_MUIC)
				seq_printf(m,
					"[%5lu.%06lu] manager notify: id=%s src=%s dest=%s rprd=%s cable=%d %s\n",
				(unsigned long)ts, rem_nsec / 1000,
				ccic_id_string(type.id),
				ccic_dev_string(type.src),
				ccic_dev_string(type.dest),
				ccic_rprd_string(type.sub2),
				cable, ccic_con_string(type.sub1));
			else
				seq_printf(m,
					"[%5lu.%06lu] manager notify: id=%s src=%s dest=%s rprd=%s rp status=%s %s\n",
				(unsigned long)ts, rem_nsec / 1000,
				ccic_id_string(type.id),
				ccic_dev_string(type.src),
				ccic_dev_string(type.dest),
				ccic_rprd_string(type.sub2),
				ccic_rpstatus_string(type.sub3),
				ccic_con_string(type.sub1));
		} else if (type.id  == NOTIFY_ID_RID)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s rid=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_rid_string(type.sub1));
		else if (type.id  == NOTIFY_ID_USB)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s status=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			usbstatus_string(type.sub2));
		else if (type.id  == NOTIFY_ID_POWER_STATUS)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_con_string(type.sub1));
		else if (type.id  == NOTIFY_ID_WATER)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s    %s detected\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1 ? "WATER":"DRY");
		else if (type.id  == NOTIFY_ID_VCONN)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest));
		else if (type.id  == NOTIFY_ID_OTG)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub2 ? "Enable":"Disable");
		else if (type.id  == NOTIFY_ID_TA)
			seq_printf(m, "[%5lu.%06lu] ccic notify:    id=%s src=%s dest=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_con_string(type.sub1));
		else if (type.id  == NOTIFY_ID_DP_CONNECT)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s 0x%04x/0x%04x %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub2,
			type.sub3,
			ccic_con_string(type.sub1));
		else if (type.id  == NOTIFY_ID_DP_HPD)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s hpd=%s irq=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_hpd_string(type.sub1),
			type.sub2 ? "VALID":"NONE");
		else if (type.id  == NOTIFY_ID_DP_LINK_CONF)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s PIN-assign=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_pin_string(type.sub1));
		else if (type.id  == NOTIFY_ID_USB_DP)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s CON=%d HS=%d\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1,
			type.sub2);
		else if (type.id  == NOTIFY_ID_ROLE_SWAP)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s sub1=%d sub2=%d\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1,
			type.sub2);
		else if (type.id  == NOTIFY_ID_FAC)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s ErrState=%d\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			type.sub1);
		else if (type.id == NOTIFY_ID_CC_PIN_STATUS)
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s pinstatus=%s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_pinstatus_string(type.sub1));
		else
			seq_printf(m, "[%5lu.%06lu] manager notify: id=%s src=%s dest=%s rprd=%s %s\n",
			(unsigned long)ts, rem_nsec / 1000,
			ccic_id_string(type.id),
			ccic_dev_string(type.src),
			ccic_dev_string(type.dest),
			ccic_rprd_string(type.sub2),
			ccic_con_string(type.sub1));
		break;
	default:
		seq_printf(m, "[%5lu.%06lu] undefined event\n",
			(unsigned long)ts, rem_nsec / 1000);
		break;
	}
}

static void print_port_string(struct seq_file *m, unsigned long long ts,
	unsigned long rem_nsec, int type, uint16_t param1,
		uint16_t param2, uint16_t cnt)
{
	switch (type) {
	case NOTIFY_PORT_CONNECT:
		seq_printf(m, "[%5lu.%06lu] port connect - VID:0x%04x PID:0x%04x cnt:%d\n",
			(unsigned long)ts, rem_nsec / 1000,
					param1, param2, cnt);
		break;
	case NOTIFY_PORT_DISCONNECT:
		seq_printf(m, "[%5lu.%06lu] port disconnect - VID:0x%04x PID:0x%04x\n",
			(unsigned long)ts, rem_nsec / 1000, param1, param2);
		break;
	case NOTIFY_PORT_CLASS:
		seq_printf(m, "[%5lu.%06lu] device class %d, interface class %d\n",
			(unsigned long)ts, rem_nsec / 1000, param1, param2);
		break;
	default:
		seq_printf(m, "[%5lu.%06lu] undefined event\n",
			(unsigned long)ts, rem_nsec / 1000);
		break;
	}
}

static uint16_t set_port_count(uint16_t vid, uint16_t pid)
{
	int i;
	uint16_t ret = 0;
	struct port_count *temp_port;

	for (i = 0; i < USBLOG_MAX_STORE_PORT; i++) {
		temp_port = &usblog_root.usblog_buffer->store_port_cnt[i];
		if ((temp_port->vid == vid)
			&& temp_port->pid == pid) {
			temp_port->count++;
			ret = temp_port->count;
			break;
		}

		if (!temp_port->vid && !temp_port->pid) {
			temp_port->vid = vid;
			temp_port->pid = pid;
			temp_port->count++;
			ret = temp_port->count;
			break;
		}
	}

	if (i == USBLOG_MAX_STORE_PORT)
		pr_err("%s store port overflow\n", __func__);

	return ret;
}

static void print_pcm_string(struct seq_file *m, unsigned long long ts,
	unsigned long rem_nsec, int type, int enable)
{
	switch (type) {
	case NOTIFY_PCM_PLAYBACK:
		seq_printf(m, "[%5lu.%06lu] USB PCM PLAYBACK %s\n",
			(unsigned long)ts, rem_nsec / 1000,
				enable ? "OPEN" : "CLOSE");
		break;
	case NOTIFY_PCM_CAPTURE:
		seq_printf(m, "[%5lu.%06lu] USB PCM CAPTURE %s\n",
			(unsigned long)ts, rem_nsec / 1000,
				enable ? "OPEN" : "CLOSE");
		break;
	default:
		seq_printf(m, "[%5lu.%06lu] undefined event\n",
			(unsigned long)ts, rem_nsec / 1000);
		break;
	}
}

static int usblog_proc_show(struct seq_file *m, void *v)
{
	struct usblog_buf *temp_usblog_buffer;
	struct usblog_vm_buf *temp_usblog_vm_buffer;
	unsigned long long ts;
	unsigned long rem_nsec;
	unsigned long i;

	temp_usblog_buffer = usblog_root.usblog_buffer;
	temp_usblog_vm_buffer = usblog_root.usblog_vm_buffer;

	if (!temp_usblog_buffer || !temp_usblog_vm_buffer)
		goto err;

	seq_printf(m,
		"usblog CC IC version:\n");

	seq_printf(m,
		"hw version =%2x %2x %2x %2x\n",
		usblog_root.ccic_ver.hw_version[3],
		usblog_root.ccic_ver.hw_version[2],
		usblog_root.ccic_ver.hw_version[1],
		usblog_root.ccic_ver.hw_version[0]);

	seq_printf(m,
		"sw version =%2x %2x %2x %2x\n",
		usblog_root.ccic_ver.sw_main[2],
		usblog_root.ccic_ver.sw_main[1],
		usblog_root.ccic_ver.sw_main[0],
		usblog_root.ccic_ver.sw_boot);

	seq_printf(m,
		"bin version =%2x %2x %2x %2x\n",
		usblog_root.ccic_bin_ver.sw_main[2],
		usblog_root.ccic_bin_ver.sw_main[1],
		usblog_root.ccic_bin_ver.sw_main[0],
		usblog_root.ccic_bin_ver.sw_boot);


	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog CCIC EVENT: count=%llu maxline=%d\n",
			temp_usblog_buffer->ccic_count,
					USBLOG_CCIC_BUFFER_SIZE);

	if (temp_usblog_buffer->ccic_count >= USBLOG_CCIC_BUFFER_SIZE) {
		for (i = temp_usblog_buffer->ccic_index;
			i < USBLOG_CCIC_BUFFER_SIZE; i++) {
			ts = temp_usblog_buffer->ccic_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			print_ccic_event(m, ts, rem_nsec,
				temp_usblog_buffer->ccic_buffer[i].cc_type,
				&temp_usblog_buffer->ccic_buffer[i].noti);
		}
	}

	for (i = 0; i < temp_usblog_buffer->ccic_index; i++) {
		ts = temp_usblog_buffer->ccic_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		print_ccic_event(m, ts, rem_nsec,
				temp_usblog_buffer->ccic_buffer[i].cc_type,
				&temp_usblog_buffer->ccic_buffer[i].noti);
	}

	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog USB_MODE: count=%llu maxline=%d\n",
			temp_usblog_buffer->mode_count,
					USBLOG_MODE_BUFFER_SIZE);

	if (temp_usblog_buffer->mode_count >= USBLOG_MODE_BUFFER_SIZE) {
		for (i = temp_usblog_buffer->mode_index;
			i < USBLOG_MODE_BUFFER_SIZE; i++) {
			ts = temp_usblog_buffer->mode_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			seq_printf(m, "[%5lu.%06lu] %s\n", (unsigned long)ts,
				rem_nsec / 1000,
			temp_usblog_buffer->mode_buffer[i].usbmode_str);
		}
	}

	for (i = 0; i < temp_usblog_buffer->mode_index; i++) {
		ts = temp_usblog_buffer->mode_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		seq_printf(m, "[%5lu.%06lu] %s\n", (unsigned long)ts,
			rem_nsec / 1000,
		temp_usblog_buffer->mode_buffer[i].usbmode_str);
	}

	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog USB STATE: count=%llu maxline=%d\n",
			temp_usblog_buffer->state_count,
				USBLOG_STATE_BUFFER_SIZE);

	if (temp_usblog_buffer->state_count >= USBLOG_STATE_BUFFER_SIZE) {
		for (i = temp_usblog_buffer->state_index;
			i < USBLOG_STATE_BUFFER_SIZE; i++) {
			ts = temp_usblog_buffer->state_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			seq_printf(m, "[%5lu.%06lu] %s\n", (unsigned long)ts,
				rem_nsec / 1000,
			usbstate_string(temp_usblog_buffer
						->state_buffer[i].usbstate));
		}
	}

	for (i = 0; i < temp_usblog_buffer->state_index; i++) {
		ts = temp_usblog_buffer->state_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		seq_printf(m, "[%5lu.%06lu] %s\n", (unsigned long)ts,
			rem_nsec / 1000,
		usbstate_string(temp_usblog_buffer->state_buffer[i].usbstate));
	}

	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog USB EVENT: count=%llu maxline=%d\n",
			temp_usblog_buffer->event_count,
				USBLOG_EVENT_BUFFER_SIZE);

	if (temp_usblog_buffer->event_count >= USBLOG_EVENT_BUFFER_SIZE) {
		for (i = temp_usblog_buffer->event_index;
			i < USBLOG_EVENT_BUFFER_SIZE; i++) {
			ts = temp_usblog_buffer->event_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			seq_printf(m, "[%5lu.%06lu] %s %s\n", (unsigned long)ts,
				rem_nsec / 1000,
			event_string(temp_usblog_buffer->event_buffer[i].event),
			status_string(temp_usblog_buffer
				->event_buffer[i].enable));
		}
	}

	for (i = 0; i < temp_usblog_buffer->event_index; i++) {
		ts = temp_usblog_buffer->event_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		seq_printf(m, "[%5lu.%06lu] %s %s\n", (unsigned long)ts,
			rem_nsec / 1000,
		event_string(temp_usblog_buffer->event_buffer[i].event),
		status_string(temp_usblog_buffer->event_buffer[i].enable));
	}

	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog PORT: count=%llu maxline=%d\n",
			temp_usblog_buffer->port_count,
				USBLOG_PORT_BUFFER_SIZE);

	if (temp_usblog_buffer->port_count >= USBLOG_PORT_BUFFER_SIZE) {
		for (i = temp_usblog_buffer->port_index;
			i < USBLOG_PORT_BUFFER_SIZE; i++) {
			ts = temp_usblog_buffer->port_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			print_port_string(m, ts, rem_nsec,
				temp_usblog_buffer->port_buffer[i].type,
				temp_usblog_buffer->port_buffer[i].param1,
				temp_usblog_buffer->port_buffer[i].param2,
				temp_usblog_buffer->port_buffer[i].count);
		}
	}

	for (i = 0; i < temp_usblog_buffer->port_index; i++) {
		ts = temp_usblog_buffer->port_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		print_port_string(m, ts, rem_nsec,
			temp_usblog_buffer->port_buffer[i].type,
			temp_usblog_buffer->port_buffer[i].param1,
			temp_usblog_buffer->port_buffer[i].param2,
			temp_usblog_buffer->port_buffer[i].count);
	}

	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog PCM: count=%llu maxline=%d\n",
			temp_usblog_vm_buffer->pcm_count,
				USBLOG_PCM_BUFFER_SIZE);

	if (temp_usblog_vm_buffer->pcm_count >= USBLOG_PCM_BUFFER_SIZE) {
		for (i = temp_usblog_vm_buffer->pcm_index;
			i < USBLOG_PCM_BUFFER_SIZE; i++) {
			ts = temp_usblog_vm_buffer->pcm_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			print_pcm_string(m, ts, rem_nsec,
				temp_usblog_vm_buffer->pcm_buffer[i].type,
				temp_usblog_vm_buffer->pcm_buffer[i].enable);
		}
	}

	for (i = 0; i < temp_usblog_vm_buffer->pcm_index; i++) {
		ts = temp_usblog_vm_buffer->pcm_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		print_pcm_string(m, ts, rem_nsec,
			temp_usblog_vm_buffer->pcm_buffer[i].type,
			temp_usblog_vm_buffer->pcm_buffer[i].enable);
	}

	seq_printf(m,
		"\n\n");
	seq_printf(m,
		"usblog EXTRA: count=%llu maxline=%d\n",
			temp_usblog_buffer->extra_count,
				USBLOG_EXTRA_BUFFER_SIZE);

	if (temp_usblog_buffer->extra_count >= USBLOG_EXTRA_BUFFER_SIZE) {
		for (i = temp_usblog_buffer->extra_index;
			i < USBLOG_EXTRA_BUFFER_SIZE; i++) {
			ts = temp_usblog_buffer->extra_buffer[i].ts_nsec;
			rem_nsec = do_div(ts, 1000000000);
			seq_printf(m, "[%5lu.%06lu] %s\n", (unsigned long)ts,
				rem_nsec / 1000,
			extra_string(temp_usblog_buffer
				->extra_buffer[i].event));
		}
	}

	for (i = 0; i < temp_usblog_buffer->extra_index; i++) {
		ts = temp_usblog_buffer->extra_buffer[i].ts_nsec;
		rem_nsec = do_div(ts, 1000000000);
		seq_printf(m, "[%5lu.%06lu] %s\n", (unsigned long)ts,
			rem_nsec / 1000,
		extra_string(temp_usblog_buffer->extra_buffer[i].event));
	}
err:
	return 0;
}

static int usblog_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, usblog_proc_show, NULL);
}

static const struct file_operations usblog_proc_fops = {
	.open		= usblog_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

void ccic_store_usblog_notify(int type, uint64_t *param1)
{
	struct ccic_buf *ccic_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;

	target_count = &usblog_root.usblog_buffer->ccic_count;
	target_index = &usblog_root.usblog_buffer->ccic_index;
	ccic_buffer = &usblog_root.usblog_buffer->ccic_buffer[*target_index];
	if (ccic_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	ccic_buffer->ts_nsec = local_clock();
	ccic_buffer->cc_type = type;
	ccic_buffer->noti = *param1;

	*target_index = (*target_index+1)%USBLOG_CCIC_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void mode_store_usblog_notify(int type, char *param1)
{
	struct mode_buf *md_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;
	char buf[256], buf2[4];
	char *b, *name;
	int param_len;

	target_count = &usblog_root.usblog_buffer->mode_count;
	target_index = &usblog_root.usblog_buffer->mode_index;
	md_buffer = &usblog_root.usblog_buffer->mode_buffer[*target_index];
	if (md_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	md_buffer->ts_nsec = local_clock();

	strlcpy(buf, param1, sizeof(buf));
	b = strim(buf);

	if (type == NOTIFY_USBMODE_EXTRA) {
		param_len = strlen(b);
		if (param_len >= USBLOG_MAX_STRING_SIZE)
			param_len = USBLOG_MAX_STRING_SIZE-1;
		strncpy(md_buffer->usbmode_str, b,
			sizeof(md_buffer->usbmode_str)-1);
	} else if (type == NOTIFY_USBMODE) {
		if (b) {
			name = strsep(&b, ",");
			strlcpy(buf2, name, sizeof(buf2));
			strncpy(md_buffer->usbmode_str, buf2,
				sizeof(md_buffer->usbmode_str)-1);
		}
		while (b) {
			name = strsep(&b, ",");
			if (!name)
				continue;
			if (USBLOG_MAX_STRING_SIZE
				- strlen(md_buffer->usbmode_str) < 5) {
				strncpy(md_buffer->usbmode_str, "overflow",
					sizeof(md_buffer->usbmode_str)-1);
				b = NULL;
			} else {
				strncat(md_buffer->usbmode_str, ",", 1);
				strncat(md_buffer->usbmode_str, name, 3);
			}
		}
	}

	*target_index = (*target_index+1)%USBLOG_MODE_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void state_store_usblog_notify(int type, char *param1)
{
	struct state_buf *st_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;
	char buf[256], index, index2, index3;
	char *b, *name;
	int usbstate;

	target_count = &usblog_root.usblog_buffer->state_count;
	target_index = &usblog_root.usblog_buffer->state_index;
	st_buffer = &usblog_root.usblog_buffer->state_buffer[*target_index];
	if (st_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	st_buffer->ts_nsec = local_clock();

	strlcpy(buf, param1, sizeof(buf));
	b = strim(buf);
	name = strsep(&b, "=");

	index = *(b+USBLOG_CMP_INDEX);

	switch (index) {
	case 'F': /* CONFIGURED */
		usbstate = NOTIFY_CONFIGURED;
		break;
	case 'N':  /* CONNECTED */
		usbstate = NOTIFY_CONNECTED;
		break;
	case 'C':  /* DISCONNECTED */
		usbstate = NOTIFY_DISCONNECTED;
		break;
	case 'E':  /* RESET */
		name = strsep(&b, ":");
		if (b) {
			index2 = *b;
			switch (index2) {
			case 'F': /* FULL SPEED */
				usbstate = NOTIFY_RESET_FULL;
				break;
			case 'H':  /* HIGH SPEED */
				usbstate = NOTIFY_RESET_HIGH;
				break;
			case 'S': /* SUPER SPEED */
				usbstate = NOTIFY_RESET_SUPER;
				break;
			default:
				usbstate = NOTIFY_RESET;
				break;
			}
		} else
			usbstate = NOTIFY_RESET;
		break;
	case 'L': /* GADGET PULL UP/DN */
		name = strsep(&b, ":");
		if (b) {
			index2 = *b;
			name = strsep(&b, ":");
			if (b)
				index3 = *b;
			else /* X means none */
				index3 = 'X';
		} else /* X means none */
			index2 = 'X';

		switch (index2) {
		case 'E': /* VBUS SESSION ENABLE */
			if (index3 == 'S')
				usbstate = NOTIFY_PULLUP_EN_SUCCESS;
			else if (index3 == 'F')
				usbstate = NOTIFY_PULLUP_EN_FAIL;
			else
				usbstate = NOTIFY_PULLUP_ENABLE;
			break;
		case 'D': /* VBUS SESSION DISABLE */
			if (index3 == 'S')
				usbstate = NOTIFY_PULLUP_DIS_SUCCESS;
			else if (index3 == 'F')
				usbstate = NOTIFY_PULLUP_DIS_FAIL;
			else
				usbstate = NOTIFY_PULLUP_DISABLE;
			break;
		default:
			usbstate = NOTIFY_PULLUP;
			break;
		}
		break;
	case 'R':  /* ACCESSORY START */
		usbstate = NOTIFY_ACCSTART;
		break;
	case 'S':  /* GADGET_VBUS EN/DN*/
		name = strsep(&b, ":");
		if (b) {
			index2 = *b;
			name = strsep(&b, ":");
			if (b)
				index3 = *b;
			else /* X means none */
				index3 = 'X';
		} else /* X means none */
			index2 = 'X';

		switch (index2) {
		case 'E': /* VBUS SESSION ENABLE */
			if (index3 == 'S')
				usbstate = NOTIFY_VBUS_EN_SUCCESS;
			else if (index3 == 'F')
				usbstate = NOTIFY_VBUS_EN_FAIL;
			else
				usbstate = NOTIFY_VBUS_SESSION_ENABLE;
			break;
		case 'D': /* VBUS SESSION DISABLE */
			if (index3 == 'S')
				usbstate = NOTIFY_VBUS_DIS_SUCCESS;
			else if (index3 == 'F')
				usbstate = NOTIFY_VBUS_DIS_FAIL;
			else
				usbstate = NOTIFY_VBUS_SESSION_DISABLE;
			break;
		default:
			usbstate = NOTIFY_VBUS_SESSION;
			break;
		}
		break;
	case 'D': /* SUPER SPEED */
		usbstate = NOTIFY_SUPER;
		break;
	case 'H': /*HIGH SPEED */
		usbstate = NOTIFY_HIGH;
		break;
	case 'M': /* USB ENUMERATION */
		name = strsep(&b, ":");
		if (b) {
			index2 = *b;
			name = strsep(&b, ":");
			if (b)
				index3 = *b;
			else /* X means none */
				index3 = 'X';
		} else /* X means none */
			index2 = 'X';

		switch (index2) {
		case 'C': /* CONNDONE */
			if (index3 == 'P') /* SUPERSPEED_PLUS */
				usbstate = NOTIFY_CONNDONE_SSP;
			else if (index3 == 'S') /* SUPERSPEED */
				usbstate = NOTIFY_CONNDONE_SS;
			else if (index3 == 'H') /* HIGHSPEED */
				usbstate = NOTIFY_CONNDONE_HS;
			else if (index3 == 'F') /* FULLSPEED */
				usbstate = NOTIFY_CONNDONE_FS;
			else if (index3 == 'L') /* LOWSPEED */
				usbstate = NOTIFY_CONNDONE_LS;
			break;
		case 'G': /* GET */
			if (index3 == 'D') /* GET_DES */
				usbstate = NOTIFY_GET_DES;
			break;
		case 'S': /* SET */
			if (index3 == 'C') /* SET_CON */
				usbstate = NOTIFY_SET_CON;
			break;
		default:
			break;
		}
		break;
	default:
		pr_err("%s state param error. state=%s\n", __func__, param1);
		goto err;
	}

	st_buffer->usbstate = usbstate;

	*target_index = (*target_index+1)%USBLOG_STATE_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void event_store_usblog_notify(int type, unsigned long *param1, int *param2)
{
	struct event_buf *ev_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;

	target_count = &usblog_root.usblog_buffer->event_count;
	target_index = &usblog_root.usblog_buffer->event_index;
	ev_buffer = &usblog_root.usblog_buffer->event_buffer[*target_index];
	if (ev_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	ev_buffer->ts_nsec = local_clock();
	ev_buffer->event = *param1;
	ev_buffer->enable = *param2;

	*target_index = (*target_index+1)%USBLOG_EVENT_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void port_store_usblog_notify(int type, void *param1, void *param2)
{
	struct port_buf *pt_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;

	target_count = &usblog_root.usblog_buffer->port_count;
	target_index = &usblog_root.usblog_buffer->port_index;
	pt_buffer = &usblog_root.usblog_buffer->port_buffer[*target_index];
	if (pt_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	pt_buffer->ts_nsec = local_clock();
	pt_buffer->type = type;
	if (type == NOTIFY_PORT_CONNECT) {
		pt_buffer->param1 = le16_to_cpu(*(__le16 *)(param1));
		pt_buffer->param2 = le16_to_cpu(*(__le16 *)(param2));
		pt_buffer->count
			= set_port_count(pt_buffer->param1, pt_buffer->param2);
	} else if (type == NOTIFY_PORT_DISCONNECT) {
		pt_buffer->param1 = le16_to_cpu(*(__le16 *)(param1));
		pt_buffer->param2 = le16_to_cpu(*(__le16 *)(param2));
	} else {
		pt_buffer->param1 = (uint16_t)(*(__u8 *)(param1));
		pt_buffer->param2 = (uint16_t)(*(__u8 *)(param2));
	}

	*target_index = (*target_index+1)%USBLOG_PORT_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void pcm_store_usblog_notify(int type, int *param1)
{
	struct pcm_buf *pcm_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;

	target_count = &usblog_root.usblog_vm_buffer->pcm_count;
	target_index = &usblog_root.usblog_vm_buffer->pcm_index;
	pcm_buffer = &usblog_root.usblog_vm_buffer->pcm_buffer[*target_index];
	if (pcm_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	pcm_buffer->ts_nsec = local_clock();
	pcm_buffer->type = type;
	pcm_buffer->enable = *param1;

	*target_index = (*target_index+1)%USBLOG_PCM_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void extra_store_usblog_notify(int type, int *param1)
{
	struct extra_buf *ex_buffer;
	unsigned long long *target_count;
	unsigned long *target_index;

	target_count = &usblog_root.usblog_buffer->extra_count;
	target_index = &usblog_root.usblog_buffer->extra_index;
	ex_buffer = &usblog_root.usblog_buffer->extra_buffer[*target_index];
	if (ex_buffer == NULL) {
		pr_err("%s target_buffer error\n", __func__);
		goto err;
	}
	ex_buffer->ts_nsec = local_clock();
	ex_buffer->event = *param1;

	*target_index = (*target_index+1)%USBLOG_EXTRA_BUFFER_SIZE;
	(*target_count)++;
err:
	return;
}

void store_usblog_notify(int type, void *param1, void *param2)
{
	unsigned long flags = 0;
	uint64_t temp = 0;

	if (!usblog_root.init)
		register_usblog_proc();

	spin_lock_irqsave(&usblog_root.usblog_lock, flags);

	if (!usblog_root.usblog_buffer) {
		pr_err("%s usblog_buffer is null\n", __func__);
		spin_unlock_irqrestore(&usblog_root.usblog_lock, flags);
		return;
	}

	if (!usblog_root.usblog_vm_buffer) {
		pr_err("%s usblog_vm_buffer is null\n", __func__);
		spin_unlock_irqrestore(&usblog_root.usblog_lock, flags);
		return;
	}

	if (type == NOTIFY_FUNCSTATE || type == NOTIFY_ALTERNATEMODE) {
		temp = *(int *)param1;
		ccic_store_usblog_notify(type, &temp);
	} else if (type == NOTIFY_CCIC_EVENT
		|| type == NOTIFY_MANAGER)
		ccic_store_usblog_notify(type, (uint64_t *)param1);
	else if (type == NOTIFY_EVENT)
		event_store_usblog_notify(type,
			(unsigned long *)param1, (int *)param2);
	else  if (type == NOTIFY_USBMODE
		|| type == NOTIFY_USBMODE_EXTRA)
		mode_store_usblog_notify(type, (char *)param1);
	else if (type == NOTIFY_USBSTATE)
		state_store_usblog_notify(type, (char *)param1);
	else if (type == NOTIFY_PORT_CONNECT ||
				type == NOTIFY_PORT_DISCONNECT ||
					type == NOTIFY_PORT_CLASS)
		port_store_usblog_notify(type, param1, param2);
	else if (type == NOTIFY_PCM_PLAYBACK ||
				type == NOTIFY_PCM_CAPTURE)
		pcm_store_usblog_notify(type, (int *)param1);
	else if (type == NOTIFY_EXTRA)
		extra_store_usblog_notify(type, (int *)param1);
	else
		pr_err("%s type error %d\n", __func__, type);

	spin_unlock_irqrestore(&usblog_root.usblog_lock, flags);
}
EXPORT_SYMBOL(store_usblog_notify);

void store_ccic_version(unsigned char *hw, unsigned char *sw_main,
			unsigned char *sw_boot)
{
	if (!hw || !sw_main || !sw_boot) {
		pr_err("%s null buffer\n", __func__);
		return;
	}

	memcpy(&usblog_root.ccic_ver.hw_version, hw, 4);
	memcpy(&usblog_root.ccic_ver.sw_main, sw_main, 3);
	memcpy(&usblog_root.ccic_ver.sw_boot, sw_boot, 1);
}
EXPORT_SYMBOL(store_ccic_version);

void store_ccic_bin_version(const unsigned char *sw_main,
				const unsigned char *sw_boot)
{
	if (!sw_main || !sw_boot) {
		pr_err("%s null buffer\n", __func__);
		return;
	}

	memcpy(&usblog_root.ccic_bin_ver.sw_main, sw_main, 3);
	memcpy(&usblog_root.ccic_bin_ver.sw_boot, sw_boot, 1);
}
EXPORT_SYMBOL(store_ccic_bin_version);

unsigned long long show_ccic_version(void)
{
	unsigned long long ret = 0;

	memcpy(&ret, &usblog_root.ccic_ver, sizeof(unsigned long long));
	return ret;
}
EXPORT_SYMBOL(show_ccic_version);

int register_usblog_proc(void)
{
	int ret = 0;
	struct otg_notify *o_notify = get_otg_notify();

	if (usblog_root.init) {
		pr_err("%s already registered\n", __func__);
		if (o_notify != NULL)
			goto err;
	}

	spin_lock_init(&usblog_root.usblog_lock);

	usblog_root.init = 1;

	proc_create("usblog", 0444, NULL, &usblog_proc_fops);

	usblog_root.usblog_buffer
		= kzalloc(sizeof(struct usblog_buf), GFP_KERNEL);
	if (!usblog_root.usblog_buffer) {
		ret = -ENOMEM;
		goto err;
	}
	usblog_root.usblog_vm_buffer
		= vzalloc(sizeof(struct usblog_vm_buf));
	if (!usblog_root.usblog_vm_buffer) {
		ret = -ENOMEM;
		goto err1;
	}
	pr_info("%s size=%zu\n", __func__, sizeof(struct usblog_buf));
	return ret;
err1:
	kfree(usblog_root.usblog_buffer);
	usblog_root.usblog_buffer = NULL;
err:
	pr_err("%s error\n", __func__);
	return ret;
}
EXPORT_SYMBOL(register_usblog_proc);

void unregister_usblog_proc(void)
{
	vfree(usblog_root.usblog_vm_buffer);
	usblog_root.usblog_vm_buffer = NULL;
	kfree(usblog_root.usblog_buffer);
	usblog_root.usblog_buffer = NULL;
	remove_proc_entry("usblog", NULL);
	usblog_root.init = 0;
}
EXPORT_SYMBOL(unregister_usblog_proc);

