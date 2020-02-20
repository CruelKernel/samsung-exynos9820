/*
 * Copyright (C) 2010 Samsung Electronics.
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

#ifndef __MODEM_PRJ_H__
#define __MODEM_PRJ_H__

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/skbuff.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/wakelock.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/pm_runtime.h>

#include "modem_debug.h"
#include "modem_v1.h"
#include "modem_pktlog.h"

#include "include/circ_queue.h"
#include "include/sipc5.h"

#define DEBUG_MODEM_IF
#ifdef DEBUG_MODEM_IF
#define DEBUG_MODEM_IF_LINK_TX
#define DEBUG_MODEM_IF_LINK_RX

/* #define DEBUG_MODEM_IF_IODEV_TX */
/* #define DEBUG_MODEM_IF_IODEV_RX */

/* #define DEBUG_MODEM_IF_FLOW_CTRL */

/* #define DEBUG_MODEM_IF_PS_DATA */
/* #define DEBUG_MODEM_IF_IP_DATA */
#endif

/*
 * IOCTL commands
 */
#define IOCTL_MODEM_ON			_IO('o', 0x19)
#define IOCTL_MODEM_OFF			_IO('o', 0x20)
#define IOCTL_MODEM_RESET		_IO('o', 0x21)
#define IOCTL_MODEM_BOOT_ON		_IO('o', 0x22)
#define IOCTL_MODEM_BOOT_OFF		_IO('o', 0x23)
#define IOCTL_MODEM_BOOT_DONE		_IO('o', 0x24)

#define IOCTL_MODEM_PROTOCOL_SUSPEND	_IO('o', 0x25)
#define IOCTL_MODEM_PROTOCOL_RESUME	_IO('o', 0x26)

#define IOCTL_MODEM_STATUS		_IO('o', 0x27)

#define IOCTL_MODEM_DL_START		_IO('o', 0x28)
#define IOCTL_MODEM_FW_UPDATE		_IO('o', 0x29)

#define IOCTL_MODEM_NET_SUSPEND		_IO('o', 0x30)
#define IOCTL_MODEM_NET_RESUME		_IO('o', 0x31)

#define IOCTL_MODEM_DUMP_START		_IO('o', 0x32)
#define IOCTL_MODEM_DUMP_UPDATE		_IO('o', 0x33)
#define IOCTL_MODEM_FORCE_CRASH_EXIT	_IO('o', 0x34)
#define IOCTL_MODEM_CP_UPLOAD		_IO('o', 0x35)
#ifdef CONFIG_LINK_DEVICE_PCIE
#define IOCTL_MODEM_DUMP_RESET		_IO('o', 0x36)
#endif

#define IOCTL_LINK_CONNECTED		_IO('o', 0x33)
#define IOCTL_MODEM_SET_TX_LINK		_IO('o', 0x37)

#define IOCTL_MODEM_RAMDUMP_START	_IO('o', 0xCE)
#define IOCTL_MODEM_RAMDUMP_STOP	_IO('o', 0xCF)

#define IOCTL_MODEM_XMIT_BOOT		_IO('o', 0x40)
#ifdef CONFIG_LINK_DEVICE_SHMEM
#define IOCTL_MODEM_GET_SHMEM_INFO	_IO('o', 0x41)
#endif
#define IOCTL_DPRAM_INIT_STATUS		_IO('o', 0x43)

#define IOCTL_LINK_DEVICE_RESET		_IO('o', 0x44)

#if defined(CONFIG_LINK_DEVICE_SHMEM) || defined(CONFIG_LINK_DEVICE_PCIE)
#define IOCTL_MODEM_GET_SHMEM_SRINFO	_IO('o', 0x45)
#define IOCTL_MODEM_SET_SHMEM_SRINFO	_IO('o', 0x46)
#define IOCTL_MODEM_GET_CP_BOOTLOG	_IO('o', 0x47)
#define IOCTL_MODEM_CLR_CP_BOOTLOG	_IO('o', 0x48)
#endif

/* ioctl command for IPC Logger */
#define IOCTL_MIF_LOG_DUMP		_IO('o', 0x51)
#define IOCTL_MIF_DPRAM_DUMP		_IO('o', 0x52)

#define IOCTL_SECURITY_REQ		_IO('o', 0x53)	/* Request smc_call */
#define IOCTL_SHMEM_FULL_DUMP		_IO('o', 0x54)	/* For shmem dump */
#define IOCTL_MODEM_CRASH_REASON	_IO('o', 0x55)	/* Get Crash Reason */
#define IOCTL_MODEM_AIRPLANE_MODE	_IO('o', 0x56)	/* Set Airplane mode on/off */
#define IOCTL_VSS_FULL_DUMP		_IO('o', 0x57)	/* For vss dump */
#define IOCTL_ACPM_FULL_DUMP		_IO('o', 0x58)  /* for acpm memory dump */
#define IOCTL_CPLOG_FULL_DUMP		_IO('o', 0x59)  /* for cplog memory dump */
#define IOCTL_DATABUF_FULL_DUMP		_IO('o', 0x5A)	/* for databuf memory dump */

#ifdef CONFIG_LINK_DEVICE_PCIE
#define IOCTL_REGISTER_PCIE		_IO('o', 0x65)
#endif


/*
Definitions for IO devices
*/
#define MAX_IOD_RXQ_LEN		2048

#define CP_CRASH_INFO_SIZE	512
#define CP_CRASH_TAG		"CP Crash "

#define IPv6			6
#define SOURCE_MAC_ADDR		{0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC}

/* Loopback */
#define CP2AP_LOOPBACK_CHANNEL	30
#define DATA_LOOPBACK_CHANNEL	31

#define DATA_DRAIN_CHANNEL	30	/* Drain channel to drop RX packets */

/* Debugging features */
#define MIF_LOG_DIR		"/sdcard/log"
#define MIF_MAX_PATH_LEN	256

/* Does modem ctl structure will use state ? or status defined below ?*/
enum modem_state {
	STATE_OFFLINE,
	STATE_CRASH_RESET,	/* silent reset */
	STATE_CRASH_EXIT,	/* cp ramdump */
	STATE_BOOTING,
	STATE_ONLINE,
	STATE_NV_REBUILDING,	/* <= rebuilding start */
	STATE_LOADER_DONE,
	STATE_SIM_ATTACH,
	STATE_SIM_DETACH,
	STATE_CRASH_WATCHDOG,	/* cp watchdog crash */
};

enum link_state {
	LINK_STATE_OFFLINE = 0,
	LINK_STATE_IPC,
	LINK_STATE_CP_CRASH
};

#ifdef CONFIG_LINK_DEVICE_PCIE
enum runtime_link_id {
	LINK_SEND = 0,
	LINK_CP,
	LINK_SBD_TX_TIMER,
	LINK_TX_TIMER,
	LINK_REGISTER_PCI,
	LINK_ID_MAX,
};
#endif /* CONFIG_LINK_DEVICE_PCIE */

struct sim_state {
	bool online;	/* SIM is online? */
	bool changed;	/* online is changed? */
};

struct modem_firmware {
	unsigned long long binary;
	u32 size;
	u32 m_offset;
	u32 b_offset;
	u32 mode;
	u32 len;
} __packed;

struct modem_sec_req {
	u32 mode;
	u32 param2;
	u32 param3;
	u32 param4;
} __packed;

enum cp_boot_mode {
	CP_BOOT_MODE_NORMAL,
	CP_BOOT_MODE_DUMP,
	CP_BOOT_RE_INIT,
	CP_BOOT_REQ_CP_RAM_LOGGING = 5,
	CP_BOOT_MODE_MANUAL = 7,
	MAX_CP_BOOT_MODE
};

struct sec_info {
	enum cp_boot_mode mode;
	u32 size;
};

#define SIPC_MULTI_FRAME_MORE_BIT	(0b10000000)	/* 0x80 */
#define SIPC_MULTI_FRAME_ID_MASK	(0b01111111)	/* 0x7F */
#define SIPC_MULTI_FRAME_ID_BITS	7
#define NUM_SIPC_MULTI_FRAME_IDS	(2 ^ SIPC_MULTI_FRAME_ID_BITS)
#define MAX_SIPC_MULTI_FRAME_ID		(NUM_SIPC_MULTI_FRAME_IDS - 1)

struct __packed sipc_fmt_hdr {
	u16 len;
	u8  msg_seq;
	u8  ack_seq;
	u8  main_cmd;
	u8  sub_cmd;
	u8  cmd_type;
};

static inline bool sipc_ps_ch(u8 ch)
{
	return (ch >= SIPC_CH_ID_PDP_0 && ch <= SIPC_CH_ID_PDP_14) ?
		true : false;
}

/* Channel 0, 5, 6, 27, 255 are reserved in SIPC5.
 * see SIPC5 spec: 2.2.2 Channel Identification (Ch ID) Field.
 * They do not need to store in `iodevs_tree_fmt'
 */
#define sipc5_is_not_reserved_channel(ch) \
	((ch) != 0 && (ch) != 5 && (ch) != 6 && (ch) != 27 && (ch) != 255)

#if defined(CONFIG_MODEM_IF_LEGACY_QOS) || defined(CONFIG_MODEM_IF_QOS)
#define MAX_NDEV_TX_Q 2
#else
#define MAX_NDEV_TX_Q 1
#endif
#define MAX_NDEV_RX_Q 1
/* mark value for high priority packet, hex QOSH */
#define RAW_HPRIO	0x514F5348

struct vnet {
	struct io_device *iod;
	struct link_device *ld;
};

/* for fragmented data from link devices */
struct fragmented_data {
	struct sk_buff *skb_recv;
	struct sipc5_frame_data f_data;
	/* page alloc fail retry*/
	unsigned int realloc_offset;
};
#define fragdata(iod, ld) (&(iod)->fragments[(ld)->link_type])

/** struct skbuff_priv - private data of struct sk_buff
 * this is matched to char cb[48] of struct sk_buff
 */
struct skbuff_private {
	struct io_device *iod;
	struct link_device *ld;

	/* for time-stamping */
	struct timespec ts;

	u32 sipc_ch:8,	/* SIPC Channel Number			*/
	    frm_ctrl:8,	/* Multi-framing control		*/
	    reserved:15,
	    lnk_hdr:1;	/* Existence of a link-layer header	*/
} __packed;

static inline struct skbuff_private *skbpriv(struct sk_buff *skb)
{
	BUILD_BUG_ON(sizeof(struct skbuff_private) > sizeof(skb->cb));
	return (struct skbuff_private *)&skb->cb;
}

enum iod_rx_state {
	IOD_RX_ON_STANDBY = 0,
	IOD_RX_HEADER,
	IOD_RX_PAYLOAD,
	IOD_RX_PADDING,
	MAX_IOD_RX_STATE
};

static const char * const rx_state_string[] = {
	[IOD_RX_ON_STANDBY]	= "RX_ON_STANDBY",
	[IOD_RX_HEADER]		= "RX_HEADER",
	[IOD_RX_PAYLOAD]	= "RX_PAYLOAD",
	[IOD_RX_PADDING]	= "RX_PADDING",
};

static const inline char *rx_state(enum iod_rx_state state)
{
	if (unlikely(state >= MAX_IOD_RX_STATE))
		return "INVALID_STATE";
	else
		return rx_state_string[state];
}

struct io_device {
	struct list_head list;

	/* rb_tree node for an io device */
	struct rb_node node_chan;
	struct rb_node node_fmt;

	/* Name of the IO device */
	char *name;

	/* Reference count */
	atomic_t opened;

	/* Wait queue for the IO device */
	wait_queue_head_t wq;

	/* Misc and net device structures for the IO device */
	struct miscdevice  miscdev;
	struct net_device *ndev;
	struct list_head node_ndev;

	/* ID and Format for channel on the link */
	unsigned int id;
	enum modem_link link_types;
	enum dev_format format;
	enum modem_io io_typ;
	enum modem_network net_typ;

	/* Attributes of an IO device */
	u32 attrs;

	/* The name of the application that will use this IO device */
	char *app;

	/* The size of maximum Tx packet */
	unsigned int max_tx_size;

	/* Whether or not handover among 2+ link devices */
	bool use_handover;

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Whether or not IPC is over SBD-based link device */
	bool sbd_ipc;

	/* Whether or not link-layer header is required */
	bool link_header;

	/* Rx queue of sk_buff */
	struct sk_buff_head sk_rx_q;

	/* For keeping multi-frame packets temporarily */
	struct sk_buff_head sk_multi_q[NUM_SIPC_MULTI_FRAME_IDS];

	/* RX state used in RX FSM */
	enum iod_rx_state curr_rx_state;
	enum iod_rx_state next_rx_state;

	/*
	** work for each io device, when delayed work needed
	** use this for private io device rx action
	*/
	struct delayed_work rx_work;

	/* Information ID for supporting 'Multi FMT'
	 * reference SIPC Spec. 2.2.4
	 */
	u8 info_id;
	spinlock_t info_id_lock;

	struct fragmented_data fragments[LINKDEV_MAX];

	int (*recv_skb_single)(struct io_device *iod, struct link_device *ld,
			       struct sk_buff *skb);

	int (*recv_net_skb)(struct io_device *iod, struct link_device *ld,
			    struct sk_buff *skb);

	/* inform the IO device that the modem is now online or offline or
	 * crashing or whatever...
	 */
	void (*modem_state_changed)(struct io_device *iod, enum modem_state);

	/* inform the IO device that the SIM is not inserting or removing */
	void (*sim_state_changed)(struct io_device *iod, bool sim_online);

	struct modem_ctl *mc;
	struct modem_shared *msd;

	struct wake_lock wakelock;
	long waketime;

	/* DO NOT use __current_link directly
	 * you MUST use skbpriv(skb)->ld in mc, link, etc..
	 */
	struct link_device *__current_link;
};
#define to_io_device(misc) container_of(misc, struct io_device, miscdev)

/* get_current_link, set_current_link don't need to use locks.
 * In ARM, set_current_link and get_current_link are compiled to
 * each one instruction (str, ldr) as atomic_set, atomic_read.
 * And, the order of set_current_link and get_current_link is not important.
 */
#define get_current_link(iod) ((iod)->__current_link)
#define set_current_link(iod, ld) ((iod)->__current_link = (ld))

struct link_device {
	struct list_head  list;
	enum modem_link link_type;
	struct modem_ctl *mc;
	struct modem_shared *msd;
	struct device *dev;

	char *name;
	bool sbd_ipc;
	bool aligned;

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Modem data */
	struct modem_data *mdm_data;

	/* TX queue of socket buffers */
	struct sk_buff_head skb_txq[MAX_SIPC_MAP];

	/* RX queue of socket buffers */
	struct sk_buff_head skb_rxq[MAX_SIPC_MAP];

	/* Stop/resume control for network ifaces */
	spinlock_t netif_lock;

	/* bit mask for stopped channel */
	unsigned long netif_stop_mask;
	unsigned long tx_flowctrl_mask;

	struct completion raw_tx_resumed;

	/* flag of stopped state for all channels */
	atomic_t netif_stopped;

	struct workqueue_struct *rx_wq;
	struct delayed_work rx_delayed_work;

	/* MIF buffer management */
	struct mif_buff_mng *mif_buff_mng;

	/* Save reason of forced crash */
	unsigned int crash_type;

	int (*init_comm)(struct link_device *ld, struct io_device *iod);
	void (*terminate_comm)(struct link_device *ld, struct io_device *iod);

	/* called by an io_device when it has a packet to send over link
	 * - the io device is passed so the link device can look at id and
	 *   format fields to determine how to route/format the packet
	 */
	int (*send)(struct link_device *ld, struct io_device *iod,
		    struct sk_buff *skb);

	/* method for CP booting */
	void (*boot_on)(struct link_device *ld, struct io_device *iod);
	int (*xmit_boot)(struct link_device *ld, struct io_device *iod,
			 unsigned long arg);
	int (*dload_start)(struct link_device *ld, struct io_device *iod);
	int (*firm_update)(struct link_device *ld, struct io_device *iod,
			   unsigned long arg);

	/* methods for CP crash dump */
	int (*shmem_dump)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);
	int (*databuf_dump)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);
	int (*force_dump)(struct link_device *ld, struct io_device *iod);
	int (*dump_start)(struct link_device *ld, struct io_device *iod);
	int (*dump_update)(struct link_device *ld, struct io_device *iod,
			   unsigned long arg);
	int (*dump_finish)(struct link_device *ld, struct io_device *iod,
			   unsigned long arg);

	/* method for VSS dump when CP crash */
	int (*vss_dump)(struct link_device *ld, struct io_device *iod,
				unsigned long arg);

	/* method for ACPM dump when CP crash */
	int (*acpm_dump)(struct link_device *ld, struct io_device *iod,
				unsigned long arg);

	/* method for CPLOG dump when CP crash */
	int (*cplog_dump)(struct link_device *ld, struct io_device *iod,
				unsigned long arg);

	/* IOCTL extension */
	int (*ioctl)(struct link_device *ld, struct io_device *iod,
		     unsigned int cmd, unsigned long arg);

	/* Close (stop) TX with physical link (on CP crash, etc.) */
	void (*close_tx)(struct link_device *ld);

	/* Change secure mode, Call SMC API */
	int (*security_req)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);

	/* Get crash reason form modem_if driver */
	int (*crash_reason)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);

	/* Set airplane mode for power saving */
	int (*airplane_mode)(struct link_device *ld, struct io_device *iod,
			unsigned long arg);

	/* Reset buffer & dma_addr for zerocopy */
	void (*reset_zerocopy)(struct link_device *ld);

#ifdef CONFIG_LINK_DEVICE_NAPI
	/* Poll function for NAPI */
	int (*poll_recv_on_iod)(struct link_device *ld, struct io_device *iod,
			int budget);

	int (*enable_rx_int)(struct link_device *ld);
	int (*disable_rx_int)(struct link_device *ld);
#endif /* CONFIG_LINK_DEVICE_NAPI */

	void (*gro_flush)(struct link_device *ld);

#ifdef CONFIG_LINK_DEVICE_PCIE
	int (*register_pcie)(struct link_device *ld);
#endif
};

#define pm_to_link_device(pm)	container_of(pm, struct link_device, pm)

static inline struct sk_buff *rx_alloc_skb(unsigned int length,
		struct io_device *iod, struct link_device *ld)
{
	struct sk_buff *skb;

	skb = dev_alloc_skb(length);
	if (likely(skb)) {
		skbpriv(skb)->iod = iod;
		skbpriv(skb)->ld = ld;
	}

	return skb;
}

struct modemctl_ops {
	int (*modem_on)(struct modem_ctl *);
	int (*modem_off)(struct modem_ctl *);
	int (*modem_shutdown)(struct modem_ctl *);
	int (*modem_reset)(struct modem_ctl *);
	int (*modem_boot_on)(struct modem_ctl *);
	int (*modem_boot_off)(struct modem_ctl *);
	int (*modem_boot_done)(struct modem_ctl *);
	int (*modem_force_crash_exit)(struct modem_ctl *);
	int (*modem_dump_start)(struct modem_ctl *);
	void (*modem_boot_confirm)(struct modem_ctl *);
#if defined(CONFIG_LINK_DEVICE_PCIE)
	int (*modem_dump_reset)(struct modem_ctl *);
	int (*modem_link_down)(struct modem_ctl *);
	int (*modem_runtime_suspend)(struct modem_ctl *);
	int (*modem_runtime_resume)(struct modem_ctl *);
	int (*modem_suspend)(struct modem_ctl *);
	int (*modem_resume)(struct modem_ctl *);
#endif
};

/* for IPC Logger */
struct mif_storage {
	char *addr;
	unsigned int cnt;
};

/* modem_shared - shared data for all io/link devices and a modem ctl
 * msd : mc : iod : ld = 1 : 1 : M : N
 */
struct modem_shared {
	/* list of link devices */
	struct list_head link_dev_list;

	/* list of activated ndev */
	struct list_head activated_ndev_list;
	spinlock_t active_list_lock;

	/* Array of pointers to IO devices corresponding to ch[n] */
	struct io_device *ch2iod[256];

	/* Array of active channels */
	u8 ch[256];

	/* The number of active channels in the array @ch[] */
	unsigned int num_channels;

	/* rb_tree root of io devices. */
	struct rb_root iodevs_tree_fmt; /* group by dev_format */

	/* for IPC Logger */
	struct mif_storage storage;
	spinlock_t lock;

	/* CP crash information */
	char cp_crash_info[530];

	/* loopbacked IP address
	 * default is 0.0.0.0 (disabled)
	 * after you setted this, you can use IP packet loopback using this IP.
	 * exam: echo 1.2.3.4 > /sys/devices/virtual/misc/umts_multipdp/loopback
	 */
	__be32 loopback_ipaddr;
};

struct modem_ctl {
	struct device *dev;
	char *name;
	struct modem_data *mdm_data;

	struct modem_shared *msd;
	void __iomem *sysram_alive;

	enum modem_state phone_state;
	struct sim_state sim_state;

	/* spin lock for each modem_ctl instance */
	spinlock_t lock;

	/* list for notify to opened iod when changed modem state */
	struct list_head modem_state_notify_list;

	/* completion for waiting for CP initialization */
	struct completion init_cmpl;

	/* completion for waiting for CP power-off */
	struct completion off_cmpl;

	unsigned int gpio_cp_on;
	unsigned int gpio_cp_off;
	unsigned int gpio_reset_req_n;
	unsigned int gpio_cp_reset;

	/* for broadcasting AP's PM state (active or sleep) */
	unsigned int gpio_pda_active;
	unsigned int int_pda_active;

	/* for checking aliveness of CP */
	unsigned int gpio_phone_active;
	unsigned int irq_phone_active;
	struct modem_irq irq_cp_active;

	/* for AP-CP power management (PM) handshaking */
	unsigned int gpio_ap_wakeup;
	unsigned int irq_ap_wakeup;

	unsigned int gpio_ap_status;
	unsigned int int_ap_status;

	unsigned int gpio_cp_wakeup;
	unsigned int int_cp_wakeup;

	unsigned int gpio_cp_status;
	unsigned int irq_cp_status;

	/* for performance tuning */
	unsigned int gpio_perf_req;
	unsigned int irq_perf_req;

#ifdef CONFIG_LINK_DEVICE_HSIC
	/* for USB/HSIC PM */
	unsigned int gpio_host_wakeup;
	unsigned int irq_host_wakeup;
	unsigned int gpio_host_active;
	unsigned int gpio_slave_wakeup;

	unsigned int gpio_cp_dump_int;
	unsigned int gpio_ap_dump_int;
	unsigned int gpio_flm_uart_sel;
	unsigned int gpio_cp_warm_reset;

	unsigned int gpio_sim_detect;
	unsigned int irq_sim_detect;
#endif

#ifdef CONFIG_LINK_DEVICE_SHMEM
	unsigned int mbx_pda_active;
	unsigned int mbx_phone_active;
	unsigned int mbx_ap_wakeup;
	unsigned int mbx_ap_status;
	unsigned int mbx_cp_wakeup;
	unsigned int mbx_cp_status;
	unsigned int mbx_perf_req;

	/* for notify uart connection with direction*/
	unsigned int mbx_uart_noti;
	unsigned int int_uart_noti;

	/* for checking aliveness of CP */
	struct modem_irq irq_cp_wdt;
	struct modem_irq irq_cp_fail;

	/* Status Bit Info */
	unsigned int sbi_lte_active_mask;
	unsigned int sbi_lte_active_pos;
	unsigned int sbi_cp_status_mask;
	unsigned int sbi_cp_status_pos;

	unsigned int sbi_pda_active_mask;
	unsigned int sbi_pda_active_pos;
	unsigned int sbi_ap_status_mask;
	unsigned int sbi_ap_status_pos;

	unsigned int sbi_uart_noti_mask;
	unsigned int sbi_uart_noti_pos;

	unsigned int sbi_crash_type_mask;
	unsigned int sbi_crash_type_pos;

	unsigned int airplane_mode;

	unsigned int ap2cp_cfg_addr;
	void __iomem *ap2cp_cfg_ioaddr;
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
	struct irq_chip *apwake_irq_chip;
	struct pci_dev *s5100_pdev;
	struct workqueue_struct *wakeup_wq;
	struct work_struct link_work;
	struct work_struct dislink_work;

	struct wake_lock mc_wake_lock;

	int int_pcie_link_ack;
	int pcie_ch_num;

	bool reserve_doorbell_int;
	bool recover_pcie_link;
	bool pcie_registered;
	bool device_reboot;

	u32 boot_done_cnt;

	atomic_t runtime_link_cnt[LINK_ID_MAX];
	atomic_t runtime_dislink_cnt[LINK_ID_MAX];

	atomic_t runtime_link_iod_cnt[SIPC5_CH_ID_MAX];
	atomic_t runtime_dislink_iod_cnt[SIPC5_CH_ID_MAX];

	unsigned int airplane_mode;

	int s5100_gpio_cp_pwr;
	int s5100_gpio_cp_reset;
	int s5100_gpio_cp_ps_hold;
	int s5100_gpio_cp_wakeup;
	int s5100_gpio_cp_dump_noti;
	int s5100_gpio_ap_status;

	int s5100_gpio_ap_wakeup;
	struct modem_irq s5100_irq_ap_wakeup;

	int s5100_gpio_phone_active;
	struct modem_irq s5100_irq_phone_active;

	bool s5100_cp_reset_required;
	bool s5100_iommu_map_enabled;
#endif

#ifdef CONFIG_EXYNOS_BUSMONITOR
	struct notifier_block busmon_nfb;
#endif

#if defined(CONFIG_MUIC_NOTIFIER)
	struct notifier_block uart_notifier;
#endif
	bool uart_connect;
	bool uart_dir;

	struct work_struct pm_qos_work;

	/* Switch with 2 links in a modem */
	unsigned int gpio_link_switch;

	const struct attribute_group *group;

	struct delayed_work dwork;
	struct work_struct work;

	struct modemctl_ops ops;
	struct io_device *iod;
	struct io_device *bootd;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);
	void (*modem_complete)(struct modem_ctl *mc);
	
	int receive_first_ipc;
};

static inline bool cp_offline(struct modem_ctl *mc)
{
	if (!mc)
		return true;
	return (mc->phone_state == STATE_OFFLINE);
}

static inline bool cp_online(struct modem_ctl *mc)
{
	if (!mc)
		return false;
	return (mc->phone_state == STATE_ONLINE);
}

static inline bool cp_booting(struct modem_ctl *mc)
{
	if (!mc)
		return false;
	return (mc->phone_state == STATE_BOOTING);
}

static inline bool cp_crashed(struct modem_ctl *mc)
{
	if (!mc)
		return false;
	return (mc->phone_state == STATE_CRASH_EXIT
		|| mc->phone_state == STATE_CRASH_WATCHDOG);
}

static inline bool rx_possible(struct modem_ctl *mc)
{
	if (likely(cp_online(mc)))
		return true;

	if (cp_booting(mc) || cp_crashed(mc))
		return true;

	return false;
}

int sipc5_init_io_device(struct io_device *iod);
void sipc5_deinit_io_device(struct io_device *iod);

#if defined(CONFIG_RPS) && defined(CONFIG_ARGOS)
extern struct net init_net;
extern int sec_argos_register_notifier(struct notifier_block *n, char *label);
extern int sec_argos_unregister_notifier(struct notifier_block *n, char *label);
int mif_init_argos_notifier(void);
#else
static inline int mif_init_argos_notifier(void) { return 0; }
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
static inline int cp_runtime_link(struct modem_ctl *mc,
		enum runtime_link_id link_id, int unsigned iod_id)
{
	int ret = 0;

#ifdef CONFIG_PM
	int cnt = 0;

	atomic_inc(&mc->runtime_link_cnt[link_id]);
	if (link_id == LINK_SEND)
		atomic_inc(&mc->runtime_link_iod_cnt[iod_id]);

	if (in_interrupt())
		ret = pm_runtime_get(mc->dev);
	else if (irqs_disabled()) {
		mif_info("irqs_disabled!! 1\n");
		ret = pm_runtime_get(mc->dev);
		mif_info("irqs_disabled!! 2\n");
	} else {
		while (pm_runtime_enabled(mc->dev) == false) {
			usleep_range(10000, 11000);
			cnt++;
		}
		if (cnt)
			mif_err_limited("pm_runtime_enabled wait count:%d\n",
					cnt);

		ret = pm_runtime_get_sync(mc->dev);
		if (ret < 0)
			mif_err_limited("pm_runtime_get_sync ERR:%d\n", ret);
	}
#endif /* CONFIG_PM */

	return ret;
}

static inline int cp_runtime_dislink(struct modem_ctl *mc,
		enum runtime_link_id link_id, int unsigned iod_id)
{
#ifdef CONFIG_PM
	atomic_inc(&mc->runtime_dislink_cnt[link_id]);
	if (link_id == LINK_SEND)
		atomic_inc(&mc->runtime_dislink_iod_cnt[iod_id]);

	pm_runtime_mark_last_busy(mc->dev);
	pm_runtime_put_autosuspend(mc->dev);
#endif /* CONFIG_PM */
	return 0;
}

static inline int cp_runtime_dislink_no_autosuspend(struct modem_ctl *mc,
		enum runtime_link_id link_id, int unsigned iod_id)
{
#ifdef CONFIG_PM
	atomic_inc(&mc->runtime_dislink_cnt[link_id]);
	if (link_id == LINK_SEND)
		atomic_inc(&mc->runtime_dislink_iod_cnt[iod_id]);

	pm_runtime_put_sync(mc->dev);
#endif /* CONFIG_PM */
	return 0;
}
#endif /* CONFIG_LINK_DEVICE_PCIE */

#endif
