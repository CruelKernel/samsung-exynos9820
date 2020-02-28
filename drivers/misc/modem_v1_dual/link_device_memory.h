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

#ifndef __MODEM_LINK_DEVICE_MEMORY_H__
#define __MODEM_LINK_DEVICE_MEMORY_H__

#include <linux/cpumask.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/notifier.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/netdevice.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/debugfs.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "include/link_device_memory_config.h"
#include "include/circ_queue.h"
#include "include/sbd.h"
#include "include/sipc5.h"
#include "link_rx_dit.h"
#include "link_rx_pktproc.h"

#ifdef GROUP_MEM_TYPE

enum mem_iface_type {
	MEM_EXT_DPRAM = 0x0001,	/* External DPRAM */
	MEM_AP_IDPRAM = 0x0002,	/* DPRAM in AP    */
	MEM_CP_IDPRAM = 0x0004,	/* DPRAM in CP    */
	MEM_PLD_DPRAM = 0x0008,	/* PLD or FPGA    */
	MEM_SYS_SHMEM = 0x0100,	/* Shared-memory (SHMEM) on a system bus   */
	MEM_C2C_SHMEM = 0x0200,	/* SHMEM with C2C (Chip-to-chip) interface */
	MEM_LLI_SHMEM = 0x0400,	/* SHMEM with MIPI-LLI interface           */
	MEM_PCI_SHMEM = 0x0800,	/* SHMEM with PCI interface                */
};

#define MEM_DPRAM_TYPE_MASK	0x00FF
#define MEM_SHMEM_TYPE_MASK	0xFF00

#endif

#ifdef GROUP_MEM_TYPE_SHMEM

#ifdef CONFIG_MODEM_IF_LEGACY_QOS
#define SHM_4M_RESERVED_SZ	4040
#define SHM_4M_FMT_TX_BUFF_SZ	4096
#define SHM_4M_FMT_RX_BUFF_SZ	4096
#define SHM_4M_RAW_HPRIO_TX_BUFF_SZ	518144
#define SHM_4M_RAW_HPRIO_RX_BUFF_SZ	518144
#define SHM_4M_RAW_TX_BUFF_SZ	1048576
#define SHM_4M_RAW_RX_BUFF_SZ	2097152
#else
#define SHM_4M_RESERVED_SZ	4056
#define SHM_4M_FMT_TX_BUFF_SZ	4096
#define SHM_4M_FMT_RX_BUFF_SZ	4096
#define SHM_4M_RAW_TX_BUFF_SZ	2084864
#define SHM_4M_RAW_RX_BUFF_SZ	2097152
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
#define SHM_4M_RESERVED_SZ_FIRST	984
#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
#define SHM_4M_RESERVED_SZ_SECOND	2960  /* 68 bytes for ulpath table */
#else
#define SHM_4M_RESERVED_SZ_SECOND	3028
#endif

#define DOORBELL_INT_ADD		0x10000
#define MODEM_INT_NUM			16
#endif

#define SHM_UL_USAGE_LIMIT	SZ_32K	/* Uplink burst limit */

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
#define RMNET_COUNT 8
#define SHM_2CP_UL_PATH_CTL_MAGIC	0x4B5B

struct __packed shmem_ulpath_ctl {
	u32 status;
	u32 path;
};

struct __packed shmem_ulpath_table {
	u32 magic;
	struct shmem_ulpath_ctl ctl[RMNET_COUNT];
};
#endif

struct __packed shmem_4mb_phys_map {
	u32 magic;
	u32 access;

	u32 fmt_tx_head;
	u32 fmt_tx_tail;

	u32 fmt_rx_head;
	u32 fmt_rx_tail;

#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	u32 raw_hprio_tx_head;
	u32 raw_hprio_tx_tail;

	u32 raw_hprio_rx_head;
	u32 raw_hprio_rx_tail;
#endif

	u32 raw_tx_head;
	u32 raw_tx_tail;

	u32 raw_rx_head;
	u32 raw_rx_tail;

#ifdef CONFIG_LINK_DEVICE_PCIE
	u8 reserved[SHM_4M_RESERVED_SZ_FIRST];
	u32 ap2cp_msg;
	u32 cp2ap_msg;
	u32 ap2cp_united_status;
	u32 cp2ap_united_status;
	u32 ap2cp_mif_freq;
	u32 cp2ap_dvfsreq_cpu;
	u32 cp2ap_dvfsreq_mif;
	u32 cp2ap_dvfsreq_int;
	u32 reserved_empty;
	u32 ap2cp_kerneltime;
	u32 cp2ap_int_state;
#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
	struct shmem_ulpath_table ulpath;		/* 68 bytes */
#endif
	u8 reserved_second[SHM_4M_RESERVED_SZ_SECOND];
#else
        char reserved[SHM_4M_RESERVED_SZ];
#endif

	char fmt_tx_buff[SHM_4M_FMT_TX_BUFF_SZ];
	char fmt_rx_buff[SHM_4M_FMT_RX_BUFF_SZ];

#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	char raw_hprio_tx_buff[SHM_4M_RAW_HPRIO_TX_BUFF_SZ];
	char raw_hprio_rx_buff[SHM_4M_RAW_HPRIO_RX_BUFF_SZ];
#endif

	char raw_tx_buff[SHM_4M_RAW_TX_BUFF_SZ];
	char raw_rx_buff[SHM_4M_RAW_RX_BUFF_SZ];
};

#endif

#ifdef GROUP_MEM_LINK_INTERRUPT

#define MASK_INT_VALID		0x0080
#define MASK_TX_FLOWCTL_SUSPEND	0x0010
#define MASK_TX_FLOWCTL_RESUME	0x0000

#define MASK_CMD_VALID		0x0040
#define MASK_CMD_FIELD		0x003F

#define MASK_REQ_ACK_FMT	0x0020
#define MASK_REQ_ACK_RAW	0x0010
#define MASK_RES_ACK_FMT	0x0008
#define MASK_RES_ACK_RAW	0x0004
#define MASK_SEND_FMT		0x0002
#define MASK_SEND_RAW		0x0001
#define MASK_SEND_DATA		0x0001

#define CMD_INIT_START		0x0001
#define CMD_INIT_END		0x0002
#define CMD_REQ_ACTIVE		0x0003
#define CMD_RES_ACTIVE		0x0004
#define CMD_REQ_TIME_SYNC	0x0005
#define CMD_KERNEL_PANIC	0x0006
#define CMD_CRASH_RESET		0x0007
#define CMD_PHONE_START		0x0008
#define CMD_CRASH_EXIT		0x0009
#define CMD_CP_DEEP_SLEEP	0x000A
#define CMD_NV_REBUILDING	0x000B
#define CMD_EMER_DOWN		0x000C
#define CMD_PIF_INIT_DONE	0x000D
#define CMD_SILENT_NV_REBUILD	0x000E
#define CMD_NORMAL_POWER_OFF	0x000F

#endif

#ifdef GROUP_MEM_IPC_DEVICE

struct mem_ipc_device {
	enum legacy_ipc_map id;
	char name[16];

	struct circ_queue txq;
	struct circ_queue rxq;

	u16 msg_mask;
	u16 req_ack_mask;
	u16 res_ack_mask;

	struct sk_buff_head *skb_txq;
	struct sk_buff_head *skb_rxq;

	unsigned int req_ack_cnt[MAX_DIR];

	spinlock_t tx_lock;
};

#endif

#define DATALLOC_PERIOD_MS		2	/* 2 ms */

#ifdef GROUP_MEM_FLOW_CONTROL
#define MAX_SKB_TXQ_DEPTH		1024
#define TX_PERIOD_MS			1	/* 1 ms */
#define MAX_TX_BUSY_COUNT		1024
#define BUSY_COUNT_MASK			0xF

#define RES_ACK_WAIT_TIMEOUT		10	/* 10 ms */

#define TXQ_STOP_MASK			(0x1<<0)
#define TX_SUSPEND_MASK			(0x1<<1)
#define SHM_FLOWCTL_BIT			BIT(2)
#endif

#ifdef GROUP_MEM_CP_CRASH

#define FORCE_CRASH_ACK_TIMEOUT		(10 * HZ)

#endif

#ifdef GROUP_MEM_LINK_SNAPSHOT

struct __packed mem_snapshot {
	/* Timestamp */
	struct timespec ts;

	/* Direction (TX or RX) */
	enum direction dir;

	/* The status of memory interface at the time */
	unsigned int magic;
	unsigned int access;

	unsigned int head[MAX_SIPC_MAP][MAX_DIR];
	unsigned int tail[MAX_SIPC_MAP][MAX_DIR];

	u16 int2ap;
	u16 int2cp;
};

struct mst_buff {
	/* These two members must be first. */
	struct mst_buff *next;
	struct mst_buff *prev;

	struct mem_snapshot snapshot;
};

struct mst_buff_head {
	/* These two members must be first. */
	struct mst_buff *next;
	struct mst_buff	*prev;

	u32 qlen;
	spinlock_t lock;
};

#endif

#ifdef GROUP_MEM_LINK_DEVICE

enum mem_ipc_mode {
	MEM_LEGACY_IPC,
	MEM_SBD_IPC,
};

#define CRASH_REASON_SIZE	512
enum crash_type {
	CRASH_REASON_CP_ACT_CRASH = 0,
	CRASH_REASON_RIL_MNR,
	CRASH_REASON_RIL_REQ_FULL,
	CRASH_REASON_RIL_PHONE_DIE,
	CRASH_REASON_RIL_RSV_MAX,
	CRASH_REASON_USER = 5,
	CRASH_REASON_MIF_TX_ERR = 6,
	CRASH_REASON_MIF_RIL_BAD_CH,
	CRASH_REASON_MIF_RX_BAD_DATA,
	CRASH_REASON_MIF_ZMC,
	CRASH_REASON_MIF_MDM_CTRL,
	CRASH_REASON_MIF_RSV_MAX = 11,
	CRASH_REASON_CP_SRST,
	CRASH_REASON_CP_ABNORMAL_START,
	CRASH_REASON_CP_NOT_WORK,
	CRASH_REASON_CP_PCI_LINK_FAIL = 15,
	CRASH_REASON_CP_RSV_MAX = 15,
};

struct crash_reason {
	u32 owner;
	char string[CRASH_REASON_SIZE];
};

#define FREQ_MAX_LV (40)

struct freq_table {
    int num_of_table;
    u32 freq[FREQ_MAX_LV];
};

struct mem_link_device {
	/**
	 * COMMON and MANDATORY to all link devices
	 */
	struct link_device link_dev;

	/**
	 * MEMORY type
	 */
	enum mem_iface_type type;

	/**
	 * Attributes
	 */
	unsigned long attrs;		/* Set of link_attr_bit flags	*/

	/**
	 * Flags
	 */
	bool dpram_magic;		/* DPRAM-style magic code	*/
	bool iosm;			/* IOSM message			*/

	size_t shm_size;
	/**
	 * {physical address, size, virtual address} for BOOT region
	 */
	phys_addr_t boot_start;
	size_t boot_size;
	struct page **boot_pages;	/* pointer to the page table for vmap */
	u8 __iomem *boot_base;

	/**
	 * {physical address, size, virtual address} for IPC region
	 */
	phys_addr_t start;
	size_t size;
	struct page **pages;		/* pointer to the page table for vmap */
	u8 __iomem *base;

	/**
	 * vss region for dump
	 */
	u8 __iomem *vss_base;

	/**
	 * acpm region for dump
	 */
	u8 __iomem *acpm_base;
	int acpm_size;

	/**
	 * CP Binary size for CRC checking
	 */
	u32 cp_binary_size;

	/**
	 * (u32 *) syscp_alive[0] = Magic Code, Version
	 * (u32 *) syscp_alive[1] = CP Reserved Size
	 * (u32 *) syscp_alive[2] = Shared Mem Size
	 */
	struct resource *syscp_info;

	/**
	 * Actual logical IPC devices (for IPC_MAP_FMT, IPC_MAP_NORM_RAW...)
	 */
	struct mem_ipc_device ipc_dev[MAX_SIPC_MAP];

	/**
	 * Pointers (aliases) to IPC device map
	 */
	u32 __iomem *magic;
	u32 __iomem *access;
	u32 __iomem *clk_table;
	struct mem_ipc_device *dev[MAX_SIPC_MAP];

	struct sbd_link_device sbd_link_dev;
	struct work_struct iosm_w;

	/**
	 * GPIO#, MBOX#, IRQ# for IPC
	 */
	unsigned int mbx_cp2ap_msg;	/* MBOX# for IPC RX */
	unsigned int irq_cp2ap_msg;	/* IRQ# for IPC RX  */

	unsigned int sbi_cp2ap_wakelock_mask;
	unsigned int sbi_cp2ap_wakelock_pos;

	unsigned int mbx_ap2cp_msg;	/* MBOX# for IPC TX */
	unsigned int int_ap2cp_msg;	/* INTR# for IPC TX */

	unsigned int sbi_cp_status_mask;
	unsigned int sbi_cp_status_pos;

	unsigned int mbx_perf_req;
	unsigned int mbx_perf_req_cpu;
	unsigned int mbx_perf_req_mif;
	unsigned int mbx_perf_req_int;
	unsigned int irq_perf_req;
	unsigned int irq_perf_req_cpu;
	unsigned int irq_perf_req_mif;
	unsigned int irq_perf_req_int;
	struct work_struct pm_qos_work;
	struct work_struct pm_qos_work_cpu;
	struct work_struct pm_qos_work_mif;
	struct work_struct pm_qos_work_int;

	struct freq_table cl0_table;
	struct freq_table cl1_table;
	struct freq_table mif_table;
	struct freq_table int_table;

	unsigned int irq_cp2ap_wakelock;	/* INTR# for wakelock */

	unsigned int sbi_cp_rat_mode_mask;		/* MBOX# for pcie */
	unsigned int sbi_cp_rat_mode_pos;		/* MBOX# for pcie */
	unsigned int irq_cp2ap_rat_mode;		/* INTR# for pcie */

	unsigned int irq_cp2ap_change_ul_path;

	unsigned int *ap_clk_table;
	unsigned int ap_clk_cnt;

	unsigned int *mif_clk_table;
	unsigned int mif_clk_cnt;

	unsigned int *int_clk_table;
	unsigned int int_clk_cnt;

	int current_mif_val;

	unsigned int mbx_cp2ap_status;	/* MBOX# for TX FLOWCTL */
	unsigned int irq_cp2ap_status;	/* INTR# for TX FLOWCTL */
	unsigned int tx_flowctrl_cmd;

	struct wake_lock cp_wakelock;	/* Requested by CP */

	/**
	 * Member variables for TX & RX
	 */
	struct mst_buff_head msb_rxq;
	struct mst_buff_head msb_log;

	struct tasklet_struct rx_tsk;

	struct hrtimer tx_timer;
	struct hrtimer sbd_tx_timer;
	struct hrtimer sbd_print_timer;

	struct work_struct page_reclaim_work;

	/**
	 * Member variables for CP booting and crash dump
	 */
	struct delayed_work udl_rx_dwork;
	struct std_dload_info img_info;	/* Information of each binary image */
	atomic_t cp_boot_done;

	/**
	 * Mandatory methods for the common memory-type interface framework
	 */
	void (*send_ap2cp_irq)(struct mem_link_device *mld, u16 mask);

	/**
	 * Optional methods for some kind of memory-type interface media
	 */
	u16 (*recv_cp2ap_irq)(struct mem_link_device *mld);
	u16 (*read_ap2cp_irq)(struct mem_link_device *mld);
	u16 (*recv_cp2ap_status)(struct mem_link_device *mld);
	void (*finalize_cp_start)(struct mem_link_device *mld);
	void (*unmap_region)(void *rgn);
	void (*debug_info)(void);
	void (*cmd_handler)(struct mem_link_device *mld, u16 cmd);

#ifdef DEBUG_MODEM_IF
	/* for logging MEMORY dump */
	struct work_struct dump_work;
	char dump_path[MIF_MAX_PATH_LEN];
#endif

#ifdef CONFIG_LINK_POWER_MANAGEMENT
#ifdef CONFIG_LINK_POWER_MANAGEMENT_WITH_FSM
	unsigned int gpio_ap_wakeup;		/* CP-to-AP wakeup GPIO */
	unsigned int gpio_cp_wakeup;		/* AP-to-CP wakeup GPIO */
	unsigned int gpio_cp_status;		/* CP-to-AP status GPIO */
	unsigned int gpio_ap_status;		/* AP-to-CP status GPIO */
#else
	unsigned int gpio_ap_wakeup;		/* CP-to-AP wakeup GPIO */
	struct modem_irq irq_ap_wakeup;		/* CP-to-AP wakeup IRQ  */

	unsigned int gpio_cp_wakeup;		/* AP-to-CP wakeup GPIO */

	unsigned int gpio_cp_status;		/* CP-to-AP status GPIO */
	struct modem_irq irq_cp_status;		/* CP-to-AP status IRQ  */

	unsigned int gpio_ap_status;		/* AP-to-CP status GPIO */

	struct wake_lock ap_wlock;		/* locked by ap_wakeup */
	struct wake_lock cp_wlock;		/* locked by cp_status */

	struct workqueue_struct *pm_wq;
	struct delayed_work cp_sleep_dwork;	/* to hold ap2cp_wakeup */

	spinlock_t pm_lock;

	unsigned long long last_cp2ap_intr;
#endif
	atomic_t ref_cnt;

	unsigned int gpio_ipc_int2cp;		/* AP-to-CP send signal GPIO */
	spinlock_t sig_lock;

	void (*start_pm)(struct mem_link_device *mld);
	void (*stop_pm)(struct mem_link_device *mld);
	void (*forbid_cp_sleep)(struct mem_link_device *mld);
	void (*permit_cp_sleep)(struct mem_link_device *mld);
	bool (*link_active)(struct mem_link_device *mld);
#endif

#ifdef DEBUG_MODEM_IF
	struct dentry *dbgfs_dir;
	struct debugfs_blob_wrapper mem_dump_blob;
	struct dentry *dbgfs_frame;
#endif
	unsigned int tx_period_ms;
	unsigned int force_use_memcpy;
	unsigned int memcpy_packet_count;
	unsigned int zeromemcpy_packet_count;

	atomic_t forced_cp_crash;
	struct timer_list crash_ack_timer;
	struct timer_list cp_not_work;
	unsigned int not_work_time;

	spinlock_t state_lock;
	enum link_state state;

	struct pktlog_data *pktlog;

	struct crash_reason crash_reason;

	struct wake_lock tx_timer_wlock;
	struct wake_lock sbd_tx_timer_wlock;

#ifdef CONFIG_LINK_DEVICE_NAPI
	struct net_device dummy_net;
	struct napi_struct mld_napi;
	unsigned int rx_int_enable;
	unsigned int rx_int_count;
	unsigned int rx_poll_count;
	unsigned long long rx_int_disabled_time;
#endif /* CONFIG_LINK_DEVICE_NAPI */
#ifdef CONFIG_MODEM_IF_NET_GRO
	struct timespec flush_time;
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
	/* Doorbell interrupt value to separate interrupt */
	unsigned int intval_ap2cp_msg;
	unsigned int intval_ap2cp_status;
	unsigned int intval_ap2cp_active;

	/* Location for arguments in shared memory */
	u32 __iomem *ap2cp_msg;
	u32 __iomem *cp2ap_msg;
	u32 __iomem *ap2cp_united_status;
	u32 __iomem *cp2ap_united_status;
	u32 __iomem *ap2cp_mif_freq;
	u32 __iomem *cp2ap_dvfsreq_cpu;
	u32 __iomem *cp2ap_dvfsreq_mif;
	u32 __iomem *cp2ap_dvfsreq_int;
	u32 __iomem *ap2cp_kerneltime;
	u32 __iomem *cp2ap_int_state;

	u32 __iomem *doorbell_addr;
	struct pci_dev *s5100_pdev;

	int msi_irq_base;
	int msi_irq_base_enabled;

#if defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
	struct shmem_ulpath_table ulpath;
#endif
#endif

	int (*pass_skb_to_net)(struct mem_link_device *mld, struct sk_buff *skb);

	struct dit_adaptor dit;
	struct pktproc_adaptor pktproc;
};

#define to_mem_link_device(ld) \
		container_of(ld, struct mem_link_device, link_dev)
#define ld_to_mem_link_device(ld) \
		container_of(ld, struct mem_link_device, link_dev)
#define sbd_to_mem_link_device(sl) \
		container_of(sl, struct mem_link_device, sbd_link_dev)

#define MEM_IPC_MAGIC		0xAA
#define MEM_CRASH_MAGIC		0xDEADDEAD
#define MEM_BOOT_MAGIC		0x424F4F54
#define MEM_DUMP_MAGIC		0x44554D50

#define MAX_TABLE_COUNT 8

struct clock_table_info {
	char table_name[4];
	u32 table_count;
};

struct clock_table {
	char parser_version[4];
	u32 total_table_count;
	struct clock_table_info table_info[MAX_TABLE_COUNT];
};

#endif

#ifdef GROUP_MEM_TYPE

static inline bool mem_type_shmem(enum mem_iface_type type)
{
	return (type & MEM_SHMEM_TYPE_MASK) ? true : false;
}

#endif

#ifdef GROUP_MEM_LINK_INTERRUPT

static inline bool int_valid(u16 x)
{
	return (x & MASK_INT_VALID) ? true : false;
}

static inline u16 mask2int(u16 mask)
{
	return mask | MASK_INT_VALID;
}

/**
@remark		This must be invoked after validation with int_valid().
*/
static inline bool cmd_valid(u16 x)
{
	return (x & MASK_CMD_VALID) ? true : false;

}

static inline bool chk_same_cmd(struct mem_link_device *mld, u16 x)
{
	if (mld->tx_flowctrl_cmd == x) {
		return true;
	} else {
		mld->tx_flowctrl_cmd = x;
		return false;
	}
}

static inline u16 int2cmd(u16 x)
{
	return x & MASK_CMD_FIELD;
}

static inline u16 cmd2int(u16 cmd)
{
	return mask2int(cmd | MASK_CMD_VALID);
}

#endif

#ifdef GROUP_MEM_IPC_DEVICE

static inline struct circ_queue *cq(struct mem_ipc_device *dev,
				    enum direction dir)
{
	return (dir == TX) ? &dev->txq : &dev->rxq;
}

static inline unsigned int get_txq_head(struct mem_ipc_device *dev)
{
	return get_head(&dev->txq);
}

static inline void set_txq_head(struct mem_ipc_device *dev, unsigned int in)
{
	set_head(&dev->txq, in);
}

static inline unsigned int get_txq_tail(struct mem_ipc_device *dev)
{
	return get_tail(&dev->txq);
}

static inline void set_txq_tail(struct mem_ipc_device *dev, unsigned int out)
{
	set_tail(&dev->txq, out);
}

static inline char *get_txq_buff(struct mem_ipc_device *dev)
{
	return get_buff(&dev->txq);
}

static inline unsigned int get_txq_buff_size(struct mem_ipc_device *dev)
{
	return get_size(&dev->txq);
}

static inline unsigned int get_rxq_head(struct mem_ipc_device *dev)
{
	return get_head(&dev->rxq);
}

static inline void set_rxq_head(struct mem_ipc_device *dev, unsigned int in)
{
	set_head(&dev->rxq, in);
}

static inline unsigned int get_rxq_tail(struct mem_ipc_device *dev)
{
	return get_tail(&dev->rxq);
}

static inline void set_rxq_tail(struct mem_ipc_device *dev, unsigned int out)
{
	set_tail(&dev->rxq, out);
}

static inline char *get_rxq_buff(struct mem_ipc_device *dev)
{
	return get_buff(&dev->rxq);
}

static inline unsigned int get_rxq_buff_size(struct mem_ipc_device *dev)
{
	return get_size(&dev->rxq);
}

static inline u16 msg_mask(struct mem_ipc_device *dev)
{
	return dev->msg_mask;
}

static inline u16 req_ack_mask(struct mem_ipc_device *dev)
{
	return dev->req_ack_mask;
}

static inline u16 res_ack_mask(struct mem_ipc_device *dev)
{
	return dev->res_ack_mask;
}

static inline bool req_ack_valid(struct mem_ipc_device *dev, u16 val)
{
	if (!cmd_valid(val) && (val & req_ack_mask(dev)))
		return true;
	else
		return false;
}

static inline bool res_ack_valid(struct mem_ipc_device *dev, u16 val)
{
	if (!cmd_valid(val) && (val & res_ack_mask(dev)))
		return true;
	else
		return false;
}

static inline bool rxq_empty(struct mem_ipc_device *dev)
{
	u32 head;
	u32 tail;
	unsigned long flags;

	spin_lock_irqsave(&dev->rxq.lock, flags);

	head = get_rxq_head(dev);
	tail = get_rxq_tail(dev);

	spin_unlock_irqrestore(&dev->rxq.lock, flags);

	return circ_empty(head, tail);
}

static inline bool txq_empty(struct mem_ipc_device *dev)
{
	u32 head;
	u32 tail;
	unsigned long flags;

	spin_lock_irqsave(&dev->txq.lock, flags);

	head = get_txq_head(dev);
	tail = get_txq_tail(dev);

	spin_unlock_irqrestore(&dev->txq.lock, flags);

	return circ_empty(head, tail);
}

static inline enum dev_format dev_id(enum sipc_ch_id ch)
{
	return sipc5_fmt_ch(ch) ? IPC_FMT : IPC_RAW;
}

static inline enum legacy_ipc_map get_mmap_idx(enum sipc_ch_id ch,
		struct sk_buff *skb)
{
	if (sipc5_fmt_ch(ch))
		return IPC_MAP_FMT;
#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	return (skb->queue_mapping == 1) ?
		IPC_MAP_HPRIO_RAW : IPC_MAP_NORM_RAW;
#else
	return IPC_MAP_NORM_RAW;
#endif
}

#endif

#ifdef GROUP_MEM_LINK_DEVICE

static inline unsigned int get_magic(struct mem_link_device *mld)
{
	return ioread32(mld->magic);
}

static inline unsigned int get_access(struct mem_link_device *mld)
{
	return ioread32(mld->access);
}

static inline void set_magic(struct mem_link_device *mld, unsigned int val)
{
	iowrite32(val, mld->magic);
}

static inline void set_access(struct mem_link_device *mld, unsigned int val)
{
	iowrite32(val, mld->access);
}

void shmem_unlock_mif_freq(struct mem_link_device *mld);
void shmem_restore_mif_freq(struct mem_link_device *mld);

#endif

#ifdef GROUP_MEM_LINK_SNAPSHOT

int msb_init(void);

struct mst_buff *msb_alloc(void);
void msb_free(struct mst_buff *msb);

void msb_queue_head_init(struct mst_buff_head *list);
void msb_queue_tail(struct mst_buff_head *list, struct mst_buff *msb);
void msb_queue_head(struct mst_buff_head *list, struct mst_buff *msb);

struct mst_buff *msb_dequeue(struct mst_buff_head *list);

void msb_queue_purge(struct mst_buff_head *list);

struct mst_buff *mem_take_snapshot(struct mem_link_device *mld,
				   enum direction dir);

#endif

#ifdef GROUP_MEM_LINK_INTERRUPT

static inline void send_ipc_irq(struct mem_link_device *mld, u16 val)
{
	if (likely(mld->send_ap2cp_irq))
		mld->send_ap2cp_irq(mld, val);
}

void mem_irq_handler(struct mem_link_device *mld, struct mst_buff *msb);

#endif

#ifdef GROUP_MEM_LINK_SETUP

void __iomem *mem_vmap(phys_addr_t pa, size_t size, struct page *pages[]);
void mem_vunmap(void *va);

int mem_register_boot_rgn(struct mem_link_device *mld, phys_addr_t start,
			  size_t size);
void mem_unregister_boot_rgn(struct mem_link_device *mld);
int mem_setup_boot_map(struct mem_link_device *mld);

int mem_register_ipc_rgn(struct mem_link_device *mld, phys_addr_t start,
			 size_t size);
void mem_unregister_ipc_rgn(struct mem_link_device *mld);
void mem_setup_ipc_map(struct mem_link_device *mld);

struct mem_link_device *mem_create_link_device(enum mem_iface_type type,
					       struct modem_data *modem);

#endif

#ifdef GROUP_MEM_LINK_COMMAND

int mem_reset_ipc_link(struct mem_link_device *mld);
void mem_cmd_handler(struct mem_link_device *mld, u16 cmd);

#endif

#ifdef GROUP_MEM_FLOW_CONTROL

void sbd_txq_stop(struct sbd_ring_buffer *rb);
void sbd_txq_start(struct sbd_ring_buffer *rb);

int sbd_under_tx_flow_ctrl(struct sbd_ring_buffer *rb);
int sbd_check_tx_flow_ctrl(struct sbd_ring_buffer *rb);

void tx_flowctrl_suspend(struct mem_link_device *mld);
void tx_flowctrl_resume(struct mem_link_device *mld);
void txq_stop(struct mem_link_device *mld, struct mem_ipc_device *dev);
void txq_start(struct mem_link_device *mld, struct mem_ipc_device *dev);

int under_tx_flow_ctrl(struct mem_link_device *mld, struct mem_ipc_device *dev);
int check_tx_flow_ctrl(struct mem_link_device *mld, struct mem_ipc_device *dev);

void send_req_ack(struct mem_link_device *mld, struct mem_ipc_device *dev);
void recv_res_ack(struct mem_link_device *mld, struct mem_ipc_device *dev,
		  struct mem_snapshot *mst);

void recv_req_ack(struct mem_link_device *mld, struct mem_ipc_device *dev,
		  struct mem_snapshot *mst);
void send_res_ack(struct mem_link_device *mld, struct mem_ipc_device *dev);

#endif

#ifdef GROUP_MEM_CP_CRASH

void mem_handle_cp_crash(struct mem_link_device *mld, enum modem_state state);
void mem_forced_cp_crash(struct mem_link_device *mld);

#endif

#ifdef GROUP_MEM_LINK_DEBUG

void print_pm_status(struct mem_link_device *mld);

void print_req_ack(struct mem_link_device *mld, struct mem_snapshot *mst,
		   struct mem_ipc_device *dev, enum direction dir);
void print_res_ack(struct mem_link_device *mld, struct mem_snapshot *mst,
		   struct mem_ipc_device *dev, enum direction dir);

void print_mem_snapshot(struct mem_link_device *mld, struct mem_snapshot *mst);
void print_dev_snapshot(struct mem_link_device *mld, struct mem_snapshot *mst,
			struct mem_ipc_device *dev);

#endif

static inline struct sk_buff *mem_alloc_skb(unsigned int len)
{
	gfp_t priority;
	struct sk_buff *skb;

	priority = in_interrupt() ? GFP_ATOMIC : GFP_KERNEL;

	skb = alloc_skb(len + NET_SKB_PAD, priority);
	if (!skb) {
		mif_err("ERR! alloc_skb(len:%d + pad:%d, gfp:0x%x) fail\n",
			len, NET_SKB_PAD, priority);
#ifdef CONFIG_SEC_DEBUG_MIF_OOM
		show_mem(SHOW_MEM_FILTER_NODES);
#endif
		return NULL;
	}

	skb_reserve(skb, NET_SKB_PAD);
	return skb;
}

#ifdef GROUP_MEM_LINK_IOSM_MESSAGE

/* direction: CP -> AP */
#define IOSM_C2A_MDM_READY	0x80
#define IOSM_C2A_CONF_CH_RSP	0xA3	/* answer of flow control msg */
#define IOSM_C2A_STOP_TX_CH	0xB0
#define IOSM_C2A_START_TX_CH	0xB1
#define IOSM_C2A_ACK		0xE0
#define IOSM_C2A_NACK		0xE1

/* direction: AP -> CP */
#define IOSM_A2C_AP_READY	0x00
#define IOSM_A2C_CONF_CH_REQ	0x22	/* flow control on/off */
#define IOSM_A2C_OPEN_CH	0x24
#define IOSM_A2C_CLOSE_CH	0x25
#define IOSM_A2C_STOP_TX_CH	0x30
#define IOSM_A2C_START_TX_CH	0x30
#define IOSM_A2C_ACK		0x60
#define IOSM_A2C_NACK		0x61

#define IOSM_TRANS_ID_MAX	255
#define IOSM_MSG_AREA_SIZE	(CTRL_RGN_SIZE / 2)
#define IOSM_MSG_TX_OFFSET	CMD_RGN_OFFSET
#define IOSM_MSG_RX_OFFSET	(CMD_RGN_OFFSET + IOSM_MSG_AREA_SIZE)
#define IOSM_MSG_DESC_OFFSET	(CMD_RGN_OFFSET + CMD_RGN_SIZE)

void tx_iosm_message(struct mem_link_device *, u8, u32 *);
void iosm_event_work(struct work_struct *work);
void iosm_event_bh(struct mem_link_device *mld, u16 cmd);

#define NET_HEADROOM (NET_SKB_PAD + NET_IP_ALIGN)

#endif

#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
extern int is_rndis_use(void);
#endif

#endif
