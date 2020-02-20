/*
 * Synopsys DesignWare Multimedia Card Interface driver
 *  (Based on NXP driver for lpc 31xx)
 *
 * Copyright (C) 2009 NXP Semiconductors
 * Copyright (C) 2009, 2010 Imagination Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DW_MMC_H_
#define _DW_MMC_H_

#include <linux/scatterlist.h>
#include <linux/mmc/core.h>
#include <linux/dmaengine.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/pm_qos.h>

enum dw_mci_state {
	STATE_IDLE = 0,
	STATE_SENDING_CMD,
	STATE_SENDING_DATA,
	STATE_DATA_BUSY,
	STATE_SENDING_STOP,
	STATE_DATA_ERROR,
	STATE_SENDING_CMD11,
	STATE_WAITING_CMD11_DONE,
};

enum {
	EVENT_CMD_COMPLETE = 0,
	EVENT_XFER_COMPLETE,
	EVENT_DATA_COMPLETE,
	EVENT_DATA_ERROR,
};

enum dw_mci_cookie {
	COOKIE_UNMAPPED,
	COOKIE_PRE_MAPPED,	/* mapped by pre_req() of dwmmc */
	COOKIE_MAPPED,		/* mapped by prepare_data() of dwmmc */
};

struct mmc_data;

enum {
	TRANS_MODE_PIO = 0,
	TRANS_MODE_IDMAC,
	TRANS_MODE_EDMAC
};

struct dw_mci_dma_slave {
	struct dma_chan *ch;
	enum dma_transfer_direction direction;
};

/**
 * struct dw_mci - MMC controller state shared between all slots
 * @lock: Spinlock protecting the queue and associated data.
 * @irq_lock: Spinlock protecting the INTMASK setting.
 * @regs: Pointer to MMIO registers.
 * @fifo_reg: Pointer to MMIO registers for data FIFO
 * @sg: Scatterlist entry currently being processed by PIO code, if any.
 * @sg_miter: PIO mapping scatterlist iterator.
 * @cur_slot: The slot which is currently using the controller.
 * @mrq: The request currently being processed on @cur_slot,
 *	or NULL if the controller is idle.
 * @cmd: The command currently being sent to the card, or NULL.
 * @data: The data currently being transferred, or NULL if no data
 *	transfer is in progress.
 * @stop_abort: The command currently prepared for stoping transfer.
 * @prev_blksz: The former transfer blksz record.
 * @timing: Record of current ios timing.
 * @use_dma: Whether DMA channel is initialized or not.
 * @using_dma: Whether DMA is in use for the current transfer.
 * @dma_64bit_address: Whether DMA supports 64-bit address mode or not.
 * @sg_dma: Bus address of DMA buffer.
 * @sg_cpu: Virtual address of DMA buffer.
 * @dma_ops: Pointer to platform-specific DMA callbacks.
 * @cmd_status: Snapshot of SR taken upon completion of the current
 * @ring_size: Buffer size for idma descriptors.
 *	command. Only valid when EVENT_CMD_COMPLETE is pending.
 * @dms: structure of slave-dma private data.
 * @phy_regs: physical address of controller's register map
 * @data_status: Snapshot of SR taken upon completion of the current
 *	data transfer. Only valid when EVENT_DATA_COMPLETE or
 *	EVENT_DATA_ERROR is pending.
 * @stop_cmdr: Value to be loaded into CMDR when the stop command is
 *	to be sent.
 * @dir_status: Direction of current transfer.
 * @tasklet: Tasklet running the request state machine.
 * @pending_events: Bitmask of events flagged by the interrupt handler
 *	to be processed by the tasklet.
 * @completed_events: Bitmask of events which the state machine has
 *	processed.
 * @state: Tasklet state.
 * @queue: List of slots waiting for access to the controller.
 * @bus_hz: The rate of @mck in Hz. This forms the basis for MMC bus
 *	rate and timeout calculations.
 * @current_speed: Configured rate of the controller.
 * @num_slots: Number of slots available.
 * @fifoth_val: The value of FIFOTH register.
 * @verid: Denote Version ID.
 * @dev: Device associated with the MMC controller.
 * @pdata: Platform data associated with the MMC controller.
 * @drv_data: Driver specific data for identified variant of the controller
 * @priv: Implementation defined private data.
 * @biu_clk: Pointer to bus interface unit clock instance.
 * @ciu_clk: Pointer to card interface unit clock instance.
 * @slot: Slots sharing this MMC controller.
 * @fifo_depth: depth of FIFO.
 * @data_addr_override: override fifo reg offset with this value.
 * @wm_aligned: force fifo watermark equal with data length in PIO mode.
 *	Set as true if alignment is needed.
 * @data_shift: log2 of FIFO item size.
 * @part_buf_start: Start index in part_buf.
 * @part_buf_count: Bytes of partial data in part_buf.
 * @part_buf: Simple buffer for partial fifo reads/writes.
 * @push_data: Pointer to FIFO push function.
 * @pull_data: Pointer to FIFO pull function.
 * @vqmmc_enabled: Status of vqmmc, should be true or false.
 * @irq_flags: The flags to be passed to request_irq.
 * @irq: The irq value to be passed to request_irq.
 * @sdio_id0: Number of slot0 in the SDIO interrupt registers.
 * @cmd11_timer: Timer for SD3.0 voltage switch over scheme.
 * @cto_timer: Timer for broken command transfer over scheme.
 * @dto_timer: Timer for broken data transfer over scheme.
 *
 * Locking
 * =======
 *
 * @lock is a softirq-safe spinlock protecting @queue as well as
 * at the same time while holding @lock.
 *
 * @irq_lock is an irq-safe spinlock protecting the INTMASK register
 * to allow the interrupt handler to modify it directly.  Held for only long
 * enough to read-modify-write INTMASK and no other locks are grabbed when
 * holding this one.
 *
 * The @mrq field of struct dw_mci_slot is also protected by @lock,
 * and must always be written at the same time as the slot is added to
 * @queue.
 *
 * @pending_events and @completed_events are accessed using atomic bit
 * operations, so they don't need any locking.
 *
 * None of the fields touched by the interrupt handler need any
 * locking. However, ordering is important: Before EVENT_DATA_ERROR or
 * EVENT_DATA_COMPLETE is set in @pending_events, all data-related
 * interrupts must be disabled and @data_status updated with a
 * snapshot of SR. Similarly, before EVENT_CMD_COMPLETE is set, the
 * CMDRDY interrupt must be disabled and @cmd_status updated with a
 * snapshot of SR, and before EVENT_XFER_COMPLETE can be set, the
 * bytes_xfered field of @data must be written. This is ensured by
 * using barriers.
 */
struct dw_mci {
	spinlock_t lock;
	spinlock_t irq_lock;
	void __iomem *regs;
	void __iomem *fifo_reg;
	u32 data_addr_override;
	bool wm_aligned;

	struct scatterlist *sg;
	struct sg_mapping_iter sg_miter;

	struct mmc_request *mrq;
	struct mmc_command *cmd;
	struct mmc_data *data;
	struct mmc_command stop_abort;
	unsigned int prev_blksz;
	unsigned char timing;
	struct workqueue_struct *card_workqueue;

	/* DMA interface members */
	int use_dma;
	int using_dma;
	int dma_64bit_address;

	dma_addr_t sg_dma;
	void *sg_cpu;
	const struct dw_mci_dma_ops *dma_ops;
	/* For idmac */
	unsigned int ring_size;

	/* For edmac */
	struct dw_mci_dma_slave *dms;
	/* Registers's physical base address */
	resource_size_t phy_regs;

	unsigned int desc_sz;
	struct pm_qos_request pm_qos_lock;
	struct delayed_work qos_work;
	bool qos_cntrl;
	u32 cmd_status;
	u32 data_status;
	u32 stop_cmdr;
	u32 dir_status;
	struct tasklet_struct tasklet;
	u32 tasklet_state;
	struct work_struct card_work;
	u32			card_detect_cnt;
	unsigned long pending_events;
	unsigned long completed_events;
	enum dw_mci_state state;
	struct list_head queue;

	u32 bus_hz;
	u32 current_speed;
	u32 num_slots;
	u32 fifoth_val;
	u32 cd_rd_thr;
	u16 verid;
	u16 data_offset;
	struct device *dev;
	struct dw_mci_board *pdata;
	const struct dw_mci_drv_data *drv_data;
	void *priv;
	struct clk *biu_clk;
	struct clk *ciu_clk;
	atomic_t biu_clk_cnt;
	atomic_t ciu_clk_cnt;
	atomic_t biu_en_win;
	atomic_t ciu_en_win;
	struct dw_mci_slot *slot;

	/* FIFO push and pull */
	int fifo_depth;
	int data_shift;
	u8 part_buf_start;
	u8 part_buf_count;
	union {
		u16 part_buf16;
		u32 part_buf32;
		u64 part_buf;
	};
	void (*push_data) (struct dw_mci * host, void *buf, int cnt);
	void (*pull_data) (struct dw_mci * host, void *buf, int cnt);

	/* Workaround flags */
	u32 quirks;

	/* S/W reset timer */
	struct timer_list timer;
	bool vqmmc_enabled;
	unsigned long irq_flags;	/* IRQ flags */
	int irq;

	/* Save request status */
#define DW_MMC_REQ_IDLE		0
#define DW_MMC_REQ_BUSY		1
	unsigned int req_state;
	struct dw_mci_debug_info *debug_info;	/* debug info */

	/* HWACG q-active ctrl check */
	unsigned int qactive_check;

	/* Support system power mode */
	int idle_ip_index;

	/* For argos */
	unsigned int transferred_cnt;

	/* Sfr dump */
	struct dw_mci_sfr_ram_dump *sfr_dump;

	/* S/W Timeout check */
	bool sw_timeout_chk;

	/* Card Clock In */
	u32 cclk_in;

	int sdio_id0;

	struct timer_list cmd11_timer;
	struct timer_list cto_timer;
	struct timer_list dto_timer;

	/* channel id */
	u32 ch_id;
};

/* DMA ops for Internal/External DMAC interface */
struct dw_mci_dma_ops {
	/* DMA Ops */
	int (*init) (struct dw_mci * host);
	int (*start) (struct dw_mci * host, unsigned int sg_len);
	void (*complete) (void *host);
	void (*stop) (struct dw_mci * host);
	void (*reset) (struct dw_mci * host);
	void (*cleanup) (struct dw_mci * host);
	void (*exit) (struct dw_mci * host);
};

/* IP Quirks/flags. */
/* High Speed Capable - Supports HS cards (up to 50MHz) */
#define DW_MCI_QUIRK_HIGHSPEED                  BIT(0)
/* Unreliable card detection */
#define DW_MCI_QUIRK_BROKEN_CARD_DETECTION      BIT(1)
/* No write protect */
#define DW_MCI_QUIRK_NO_WRITE_PROTECT           BIT(2)
/* No detect end bit during read */
#define DW_MCI_QUIRK_NO_DETECT_EBIT             BIT(3)
/* Use fixed IO voltage */
#define DW_MMC_QUIRK_FIXED_VOLTAGE              BIT(4)
/* Card init W/A HWACG ctrl */
#define DW_MCI_QUIRK_HWACG_CTRL                 BIT(5)
/* Enables ultra low power mode */
#define DW_MCI_QUIRK_ENABLE_ULP                 BIT(6)
/* Use the security management unit */
#define DW_MCI_QUIRK_USE_SMU                    BIT(7)
/* Spread Spectrum Clock Cntrl */
#define DW_MCI_QUIRK_USE_SSC			BIT(8)
/* Timer for broken data transfer over scheme */
#define DW_MCI_QUIRK_BROKEN_DTO			BIT(9)

/* Slot level quirks */
/* This slot has no write protect */
#define DW_MCI_SLOT_QUIRK_NO_WRITE_PROTECT	BIT(0)
enum dw_mci_cd_types {
	DW_MCI_CD_INTERNAL = 1,	/* use mmc internal CD line */
	DW_MCI_CD_EXTERNAL,	/* use external callback */
	DW_MCI_CD_GPIO,		/* use external gpio pin for CD line */
	DW_MCI_CD_NONE,		/* no CD line, use polling to detect card */
	DW_MCI_CD_PERMANENT,	/* no CD line, card permanently wired to host */
};
struct dma_pdata;

/* Board platform data */
struct dw_mci_board {
	u32 num_slots;

	u32 quirks;		/* Workaround / Quirk flags */
	unsigned int bus_hz;	/* Clock speed at the cclk_in pad */

	u32 caps;		/* Capabilities */
	u32 caps2;		/* More capabilities */
	u32 pm_caps;		/* PM capabilities */
	/*
	 * Override fifo depth. If 0, autodetect it from the FIFOTH register,
	 * but note that this may not be reliable after a bootloader has used
	 * it.
	 */
	unsigned int fifo_depth;

	/* delay in mS before detecting cards after interrupt */
	u32 detect_delay_ms;
	u8 clk_smpl;
	bool is_fine_tuned;
	bool tuned;
	bool extra_tuning;
	bool only_once_tune;

	/* INT QOS khz */
	unsigned int qos_dvfs_level;
	unsigned char io_mode;

	/* SSC RATE */
	unsigned int ssc_rate;

	enum dw_mci_cd_types cd_type;
	struct reset_control *rstc;
	struct dw_mci_dma_ops *dma_ops;
	struct dma_pdata *data;
	struct block_settings *blk_settings;
	unsigned int sw_timeout;

	/* DATA_TIMEOUT[31:11] of TMOUT */
	u32 data_timeout;
	u32 hto_timeout;
	bool use_gate_clock;
	bool use_biu_gate_clock;
	bool use_gpio_invert;
	bool enable_cclk_on_suspend;
	bool on_suspend;

	/* Broke DRTO */
	bool sw_drto;

	/* Number of descriptors */
	unsigned int desc_sz;
};

#ifdef CONFIG_MMC_DW_IDMAC
#define IDMAC_INT_CLR		(SDMMC_IDMAC_INT_AI | SDMMC_IDMAC_INT_NI | \
				 SDMMC_IDMAC_INT_CES | SDMMC_IDMAC_INT_DU | \
				 SDMMC_IDMAC_INT_FBE | SDMMC_IDMAC_INT_RI | \
				 SDMMC_IDMAC_INT_TI)

#if defined(CONFIG_MMC_DW_EXYNOS_FMP)
struct idmac_desc_64addr {
	u32 des0;		/* Control Descriptor */
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_CES	BIT(30)
#define IDMAC_DES0_OWN	BIT(31)
	u32 des1;		/* Reserved */
#define IDMAC_64ADDR_SET_BUFFER1_SIZE(d, s) \
	((d)->des2 = ((d)->des2 & cpu_to_le32(0x03ffe000)) | \
	 ((cpu_to_le32(s)) & cpu_to_le32(0x1fff)))
	u32 des2;		/*Buffer sizes */
	u32 des3;		/* Reserved */
	u32 des4;		/* Lower 32-bits of Buffer Address Pointer 1 */
	u32 des5;		/* Upper 32-bits of Buffer Address Pointer 1 */
	u32 des6;		/* Lower 32-bits of Next Descriptor Address */
	u32 des7;		/* Upper 32-bits of Next Descriptor Address */
	u32 des8;		/* File IV 0 */
	u32 des9;		/* File IV 1 */
	u32 des10;		/* File IV 2 */
	u32 des11;		/* File IV 3 */
	u32 des12;		/* File EncKey 0 */
	u32 des13;		/* File EncKey 1 */
	u32 des14;		/* File EncKey 2 */
	u32 des15;		/* File EncKey 3 */
	u32 des16;		/* File EncKey 4 */
	u32 des17;		/* File EncKey 5 */
	u32 des18;		/* File EncKey 6 */
	u32 des19;		/* File EncKey 7 */
	u32 des20;		/* File TwKey 0 */
	u32 des21;		/* File TwKey 1 */
	u32 des22;		/* File TwKey 2 */
	u32 des23;		/* File TwKey 3 */
	u32 des24;		/* File TwKey 4 */
	u32 des25;		/* File TwKey 5 */
	u32 des26;		/* File TwKey 6 */
	u32 des27;		/* File TwKey 7 */
	u32 des28;		/* Disk IV 0 */
	u32 des29;		/* Disk IV 1 */
	u32 des30;		/* Disk IV 2 */
	u32 des31;		/* Disk IV 3 */
#if defined(CONFIG_EXYNOS_FMP_DUAL_FILE_ENCRYPTION)
	u32 des32;		/* File2 EncKey 0 */
	u32 des33;		/* File2 EncKey 1 */
	u32 des34;		/* File2 EncKey 2 */
	u32 des35;		/* File2 EncKey 3 */
	u32 des36;		/* File2 EncKey 4 */
	u32 des37;		/* File2 EncKey 5 */
	u32 des38;		/* File2 EncKey 6 */
	u32 des39;		/* File2 EncKey 7 */
	u32 des40;		/* File TwKey 0 */
	u32 des41;		/* File TwKey 1 */
	u32 des42;		/* File TwKey 2 */
	u32 des43;		/* File TwKey 3 */
	u32 des44;		/* File TwKey 4 */
	u32 des45;		/* File TwKey 5 */
	u32 des46;		/* File TwKey 6 */
	u32 des47;		/* File TwKey 7 */
	u32 des48;		/* Reserved */
	u32 des49;		/* Reserved */
	u32 des50;		/* Reserved */
	u32 des51;		/* Reserved */
	u32 des52;		/* Reserved */
	u32 des53;		/* Reserved */
	u32 des54;		/* Reserved */
	u32 des55;		/* Reserved */
	u32 des56;		/* Reserved */
	u32 des57;		/* Reserved */
	u32 des58;		/* Reserved */
	u32 des59;		/* Reserved */
	u32 des60;		/* Reserved */
	u32 des61;		/* Reserved */
	u32 des62;		/* Reserved */
	u32 des63;		/* Reserved */
#endif				/* CONFIG_EXYNOS_FMP_DUAL_FILE_ENCRYPTION */
#define IDMAC_64ADDR_SET_DESC_CLEAR(d) \
do {			\
	(d)->des1 = 0;	\
	(d)->des2 = 0;	\
	(d)->des3 = 0;	\
} while(0)
#define IDMAC_64ADDR_SET_DESC_ADDR(d, a) \
do {			\
	(d)->des6 = ((u32)(a) & 0xffffffff); \
	(d)->des7 = ((u32)((a) >> 32));	\
} while(0)
};
#else
struct idmac_desc_64addr {
	u32 des0;		/* Control Descriptor */
#define IDMAC_OWN_CLR64(x) \
	!((x) & cpu_to_le32(IDMAC_DES0_OWN))

	u32 des1;		/* Reserved */

	u32 des2;		/*Buffer sizes */
#define IDMAC_64ADDR_SET_BUFFER1_SIZE(d, s) \
	((d)->des2 = ((d)->des2 & 0x03ffe000) | ((s) & 0x1fff))

	u32 des3;		/* Reserved */

	u32 des4;		/* Lower 32-bits of Buffer Address Pointer 1 */
	u32 des5;		/* Upper 32-bits of Buffer Address Pointer 1 */

	u32 des6;		/* Lower 32-bits of Next Descriptor Address */
	u32 des7;		/* Upper 32-bits of Next Descriptor Address */
#define IDMAC_64ADDR_SET_DESC_CLEAR(d) \
do {			\
	(d)->des1 = 0;	\
	(d)->des2 = 0;	\
	(d)->des3 = 0;	\
} while(0)
#define IDMAC_64ADDR_SET_DESC_ADDR(d, a) \
do {			\
	(d)->des6 = ((u32)(a) & 0xffffffff); \
	(d)->des7 = ((u32)((a) >> 32));	\
} while(0)
};
#endif

struct idmac_desc {
	u32 des0;		/* Control Descriptor */
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_CES	BIT(30)
#define IDMAC_DES0_OWN	BIT(31)

	u32 des1;		/* Buffer sizes */
#define IDMAC_SET_BUFFER1_SIZE(d, s) \
	((d)->des1 = ((d)->des1 & 0x03ffe000) | ((s) & 0x1fff))

	u32 des2;		/* buffer 1 physical address */

	u32 des3;		/* buffer 2 physical address */
#define IDMAC_SET_DESC_ADDR(d, a) \
do {	\
	(d)->des3 = (u32)(a);	\
} while(0)
};
#endif				/* CONFIG_MMC_DW_IDMAC */

/* FMP bypass/encrypt mode */
#define CLEAR		0
#define AES_CBC		1
#define AES_XTS		2

#define DW_MMC_MAX_TRANSFER_SIZE	4096
#define DW_MMC_SECTOR_SIZE		512
#define MMC_DW_IDMAC_MULTIPLIER		8

#define DW_MMC_240A		0x240a
#define DW_MMC_260A		0x260a
#define DW_MMC_280A		0x280a

#define SDMMC_CTRL		0x000
#define SDMMC_PWREN		0x004
#define SDMMC_CLKDIV		0x008
#define SDMMC_CLKSRC		0x00c
#define SDMMC_CLKENA		0x010
#define SDMMC_TMOUT		0x014
#define SDMMC_CTYPE		0x018
#define SDMMC_BLKSIZ		0x01c
#define SDMMC_BYTCNT		0x020
#define SDMMC_INTMASK		0x024
#define SDMMC_CMDARG		0x028
#define SDMMC_CMD		0x02c
#define SDMMC_RESP0		0x030
#define SDMMC_RESP1		0x034
#define SDMMC_RESP2		0x038
#define SDMMC_RESP3		0x03c
#define SDMMC_MINTSTS		0x040
#define SDMMC_RINTSTS		0x044
#define SDMMC_STATUS		0x048
#define SDMMC_FIFOTH		0x04c
#define SDMMC_CDETECT		0x050
#define SDMMC_WRTPRT		0x054
#define SDMMC_GPIO		0x058
#define SDMMC_TCBCNT		0x05c
#define SDMMC_TBBCNT		0x060
#define SDMMC_DEBNCE		0x064
#define SDMMC_USRID		0x068
#define SDMMC_VERID		0x06c
#define SDMMC_HCON		0x070
#define SDMMC_UHS_REG		0x074
#define SDMMC_RST_N		0x078
#define SDMMC_BMOD		0x080
#define SDMMC_PLDMND		0x084
#define SDMMC_DBADDR		0x088
#define SDMMC_IDSTS		0x08c
#define SDMMC_IDINTEN		0x090
#define SDMMC_DSCADDR		0x094
#define SDMMC_BUFADDR		0x098
#define SDMMC_RESP_TAT		0x0AC
#define SDMMC_CDTHRCTL		0x100
#define SDMMC_UHS_REG_EXT	0x108
#define SDMMC_ENABLE_SHIFT	0x110
#define SDMMC_DATA(x)		(x)

/*
 * Data offset is difference according to Version
 * Lower than 2.40a : data register offest is 0x100
 */
#define DATA_OFFSET		0x100
#define DATA_240A_OFFSET	0x200

/* shift bit field */
#define _SBF(f, v)		((v) << (f))

/* Control register defines */
#define SDMMC_CTRL_USE_IDMAC		BIT(25)
#define SDMMC_CTRL_CEATA_INT_EN		BIT(11)
#define SDMMC_CTRL_SEND_AS_CCSD		BIT(10)
#define SDMMC_CTRL_SEND_CCSD		BIT(9)
#define SDMMC_CTRL_ABRT_READ_DATA	BIT(8)
#define SDMMC_CTRL_SEND_IRQ_RESP	BIT(7)
#define SDMMC_CTRL_READ_WAIT		BIT(6)
#define SDMMC_CTRL_DMA_ENABLE		BIT(5)
#define SDMMC_CTRL_INT_ENABLE		BIT(4)
#define SDMMC_CTRL_DMA_RESET		BIT(2)
#define SDMMC_CTRL_FIFO_RESET		BIT(1)
#define SDMMC_CTRL_RESET		BIT(0)
/* Clock Enable register defines */
#define SDMMC_CLKEN_LOW_PWR		BIT(16)
#define SDMMC_CLKEN_ENABLE		BIT(0)
/* time-out register defines */
#define SDMMC_TMOUT_DATA(n)		_SBF(8, (n))
#define SDMMC_TMOUT_DATA_MSK		0xFFFFFF00
#define SDMMC_TMOUT_RESP(n)		((n) & 0xFF)
#define SDMMC_TMOUT_RESP_MSK		0xFF
/* card-type register defines */
#define SDMMC_CTYPE_8BIT		BIT(16)
#define SDMMC_CTYPE_4BIT		BIT(0)
#define SDMMC_CTYPE_1BIT		0
/* Interrupt status & mask register defines */
#define SDMMC_INT_SDIO(n)		BIT(16 + (n))
#define SDMMC_INT_EBE			BIT(15)
#define SDMMC_INT_ACD			BIT(14)
#define SDMMC_INT_SBE			BIT(13)
#define SDMMC_INT_HLE			BIT(12)
#define SDMMC_INT_FRUN			BIT(11)
#define SDMMC_INT_HTO			BIT(10)
#define SDMMC_INT_VOLT_SWITCH		BIT(10)	/* overloads bit 10! */
#define SDMMC_INT_DRTO			BIT(9)
#define SDMMC_INT_RTO			BIT(8)
#define SDMMC_INT_DCRC			BIT(7)
#define SDMMC_INT_RCRC			BIT(6)
#define SDMMC_INT_RXDR			BIT(5)
#define SDMMC_INT_TXDR			BIT(4)
#define SDMMC_INT_DATA_OVER		BIT(3)
#define SDMMC_INT_CMD_DONE		BIT(2)
#define SDMMC_INT_RESP_ERR		BIT(1)
#define SDMMC_INT_CD			BIT(0)
#define SDMMC_INT_ERROR			0xbfc2
/* Command register defines */
#define SDMMC_CMD_START			BIT(31)
#define SDMMC_CMD_USE_HOLD_REG	BIT(29)
#define SDMMC_CMD_VOLT_SWITCH		BIT(28)
#define SDMMC_CMD_CCS_EXP		BIT(23)
#define SDMMC_CMD_CEATA_RD		BIT(22)
#define SDMMC_CMD_UPD_CLK		BIT(21)
#define SDMMC_CMD_INIT			BIT(15)
#define SDMMC_CMD_STOP			BIT(14)
#define SDMMC_CMD_PRV_DAT_WAIT		BIT(13)
#define SDMMC_CMD_SEND_STOP		BIT(12)
#define SDMMC_CMD_STRM_MODE		BIT(11)
#define SDMMC_CMD_DAT_WR		BIT(10)
#define SDMMC_CMD_DAT_EXP		BIT(9)
#define SDMMC_CMD_RESP_CRC		BIT(8)
#define SDMMC_CMD_RESP_LONG		BIT(7)
#define SDMMC_CMD_RESP_EXP		BIT(6)
#define SDMMC_CMD_INDX(n)		((n) & 0x1F)
/* Status register defines */
#define SDMMC_GET_FCNT(x)		(((x)>>17) & 0x1FFF)
#define SDMMC_STATUS_DMA_REQ		BIT(31)
#define SDMMC_STATUS_BUSY		BIT(9)
/* FIFOTH register defines */
#define SDMMC_SET_FIFOTH(m, r, t)	(((m) & 0x7) << 28 | \
					 ((r) & 0xFFF) << 16 | \
					 ((t) & 0xFFF))
#define SDMMC_FIFOTH_DMA_MULTI_TRANS_SIZE	28
#define SDMMC_FIFOTH_RX_WMARK		16

/* HCON register defines */
#define DMA_INTERFACE_IDMA		(0x0)
#define DMA_INTERFACE_DWDMA		(0x1)
#define DMA_INTERFACE_GDMA		(0x2)
#define DMA_INTERFACE_NODMA		(0x3)
#define SDMMC_GET_TRANS_MODE(x)		(((x)>>16) & 0x3)
#define SDMMC_GET_SLOT_NUM(x)		((((x)>>1) & 0x1F) + 1)
#define SDMMC_GET_HDATA_WIDTH(x)	(((x)>>7) & 0x7)
#define SDMMC_GET_ADDR_CONFIG(x)	(((x)>>27) & 0x1)
/* Internal DMAC interrupt defines */
#define SDMMC_IDMAC_INT_AI		BIT(9)
#define SDMMC_IDMAC_INT_NI		BIT(8)
#define SDMMC_IDMAC_INT_CES		BIT(5)
#define SDMMC_IDMAC_INT_DU		BIT(4)
#define SDMMC_IDMAC_INT_FBE		BIT(2)
#define SDMMC_IDMAC_INT_RI		BIT(1)
#define SDMMC_IDMAC_INT_TI		BIT(0)
/* Internal DMAC bus mode bits */
#define SDMMC_IDMAC_ENABLE		BIT(7)
#define SDMMC_IDMAC_FB			BIT(1)
#define SDMMC_IDMAC_SWRESET		BIT(0)
/* H/W reset */
#define SDMMC_RST_HWACTIVE		0x1
/* Version ID register define */
#define SDMMC_GET_VERID(x)		((x) & 0xFFFF)
/* Card read threshold */
#define SDMMC_SET_THLD(v, x)		(((v) & 0xFFF) << 16 | (x))
#define SDMMC_CARD_WR_THR_EN		BIT(2)
#define SDMMC_CARD_RD_THR_EN		BIT(0)
/* UHS-1 register defines */
#define SDMMC_UHS_18V			BIT(0)
/* All ctrl reset bits */
#define SDMMC_CTRL_ALL_RESET_FLAGS \
	(SDMMC_CTRL_RESET | SDMMC_CTRL_FIFO_RESET | SDMMC_CTRL_DMA_RESET)

/* FIFO register access macros. These should not change the data endian-ness
 * as they are written to memory to be dealt with by the upper layers */
#define mci_fifo_readw(__reg)	__raw_readw(__reg)
#define mci_fifo_readl(__reg)	__raw_readl(__reg)
#ifdef CONFIG_MMC_DW_FORCE_32BIT_SFR_RW
#define mci_fifo_readq(__reg) ({\
		u64 __ret = 0;\
		u32* ptr = (u32*)&__ret;\
		*ptr++ = __raw_readl(__reg);\
		*ptr = __raw_readl(__reg + 0x4);\
		__ret;\
		})
#define mci_fifo_writeq(__reg, value) ({\
		u32 *ptr = (u32*)&(value);\
		__raw_writel(*ptr++, __reg);\
		__raw_writel(*ptr, __reg + 0x4);\
		})
#else
#define mci_fifo_readq(__reg)	__raw_readq(__reg)
#define mci_fifo_writeq(__value, __reg)	__raw_writeq(__reg, __value)
#endif				/* CONFIG_MMC_DW_FORCE_32BIT_SFR_RW */

#define mci_fifo_writew(__value, __reg)	__raw_writew(__reg, __value)
#define mci_fifo_writel(__value, __reg)	__raw_writel(__reg, __value)

/* Register access macros */
#define mci_readl(dev, reg)			\
	readl_relaxed((dev)->regs + SDMMC_##reg)
#define mci_writel(dev, reg, value)			\
	writel_relaxed((value), (dev)->regs + SDMMC_##reg)

/* timeout */
#define dw_mci_set_timeout(host, value)	mci_writel(host, TMOUT, value)

/* 16-bit FIFO access macros */
#define mci_readw(dev, reg)			\
	readw_relaxed((dev)->regs + SDMMC_##reg)
#define mci_writew(dev, reg, value)			\
	writew_relaxed((value), (dev)->regs + SDMMC_##reg)

/* 64-bit FIFO access macros */
#ifdef readq
#ifdef CONFIG_MMC_DW_FORCE_32BIT_SFR_RW
#define mci_readq(dev, reg) ({\
		u64 __ret = 0;\
		u32* ptr = (u32*)&__ret;\
		*ptr++ = __raw_readl((dev)->regs + SDMMC_##reg);\
		*ptr = __raw_readl((dev)->regs + SDMMC_##reg + 0x4);\
		__ret;\
	})
#define mci_writeq(dev, reg, value) ({\
		u32 *ptr = (u32*)&(value);\
		__raw_writel(*ptr++, (dev)->regs + SDMMC_##reg);\
		__raw_writel(*ptr, (dev)->regs + SDMMC_##reg + 0x4);\
	})
#else
#define mci_readq(dev, reg)			\
	readq_relaxed((dev)->regs + SDMMC_##reg)
#define mci_writeq(dev, reg, value)			\
	writeq_relaxed((value), (dev)->regs + SDMMC_##reg)
#endif				/* CONFIG_MMC_DW_FORCE_32BIT_SFR_RW */
#else
/*
 * Dummy readq implementation for architectures that don't define it.
 *
 * We would assume that none of these architectures would configure
 * the IP block with a 64bit FIFO width, so this code will never be
 * executed on those machines. Defining these macros here keeps the
 * rest of the code free from ifdefs.
 */
#define mci_readq(dev, reg)			\
	(*(volatile u64 __force *)((dev)->regs + SDMMC_##reg))
#define mci_writeq(dev, reg, value)			\
	(*(volatile u64 __force *)((dev)->regs + SDMMC_##reg) = (value))

#define __raw_writeq(__value, __reg) \
	(*(volatile u64 __force *)(__reg) = (__value))
#define __raw_readq(__reg) (*(volatile u64 __force *)(__reg))
#endif

/*
 * platform-dependent miscellaneous control
 *
 * Input arguments for platform-dependent control may be different
 * for each one, respectively. If we would add functions like them
 * whenever we need to do that, this common header file(dw_mmc.h)
 * will be modified so frequently.
 * The following enumeration type is to minimize an amount of changes
 * of common files.
 */

enum dw_mci_misc_control {
	CTRL_RESTORE_CLKSEL = 0,
	CTRL_REQUEST_EXT_IRQ,
	CTRL_CHECK_CD,
	CTRL_ADD_SYSFS,
};

#define SDMMC_DATA_TMOUT_SHIFT		11
#define SDMMC_RESP_TMOUT		0xFF
#define SDMMC_DATA_TMOUT_CRT		8
#define SDMMC_DATA_TMOUT_EXT		0x1
#define SDMMC_DATA_TMOUT_EXT_SHIFT	8
#define SDMMC_DATA_TMOUT_MAX_CNT	0x1FFFFF
#define SDMMC_DATA_TMOUT_MAX_EXT_CNT	0xFFFFFF
#define SDMMC_HTO_TMOUT_SHIFT		8

extern u32 dw_mci_calc_timeout(struct dw_mci *host);
extern int dw_mci_probe(struct dw_mci *host);
extern void dw_mci_remove(struct dw_mci *host);
#ifdef CONFIG_PM
extern int dw_mci_runtime_suspend(struct device *device);
extern int dw_mci_runtime_resume(struct device *device);
#endif

/**
 * struct dw_mci_slot - MMC slot state
 * @mmc: The mmc_host representing this slot.
 * @host: The MMC controller this slot is using.
 * @ctype: Card type for this slot.
 * @mrq: mmc_request currently being processed or waiting to be
 *	processed, or NULL when the slot is idle.
 * @queue_node: List node for placing this node in the @queue list of
 *	&struct dw_mci.
 * @clock: Clock rate configured by set_ios(). Protected by host->lock.
 * @__clk_old: The last clock value that was requested from core.
 *	Keeping track of this helps us to avoid spamming the console.
 * @flags: Random state bits associated with the slot.
 * @id: Number of this slot.
 * @sdio_id: Number of this slot in the SDIO interrupt registers.
 * @last_detect_state: Most recently observed card detect state.
 */
struct dw_mci_slot {
	struct mmc_host *mmc;
	struct dw_mci *host;

	int quirks;
	u32 ctype;

	struct mmc_request *mrq;
	struct list_head queue_node;

	unsigned int clock;
	unsigned int __clk_old;

	unsigned long flags;
#define DW_MMC_CARD_PRESENT	0
#define DW_MMC_CARD_NEED_INIT	1
#define DW_MMC_CARD_NO_LOW_PWR	2
#define DW_MMC_CARD_NO_USE_HOLD 3
#define DW_MMC_CARD_NEEDS_POLL	4
	int id;
	int sdio_id;
	int last_detect_state;
};

/**
 * struct dw_mci_debug_data - DwMMC debugging infomation
 * @host_count: a number of all hosts
 * @info_count: a number of set of debugging information
 * @info_index: index of debugging information for each host
 * @host: pointer of each dw_mci structure
 * @debug_info: debugging information structure
 */

struct dw_mci_cmd_log {
	u64 send_time;
	u64 done_time;
	u8 cmd;
	u32 arg;
	u8 data_size;
	/* no data CMD = CD, data CMD = DTO */
	/*
	 * 0b1000 0000  : new_cmd with without send_cmd
	 * 0b0000 1000  : error occurs
	 * 0b0000 0100  : data_done : DTO(Data Transfer Over)
	 * 0b0000 0010  : resp : CD(Command Done)
	 * 0b0000 0001  : send_cmd : set 1 only start_command
	 */
	u8 seq_status;		/* 0bxxxx xxxx : error data_done resp send */
#define DW_MCI_FLAG_SEND_CMD	BIT(0)
#define DW_MCI_FLAG_CD		BIT(1)
#define DW_MCI_FLAG_DTO		BIT(2)
#define DW_MCI_FLAG_ERROR	BIT(3)
#define DW_MCI_FLAG_NEW_CMD_ERR	BIT(7)

	u16 rint_sts;		/* RINTSTS value in case of error */
	u8 status_count;	/* TBD : It can be changed */
};

enum dw_mci_req_log_state {
	STATE_REQ_START = 0,
	STATE_REQ_CMD_PROCESS,
	STATE_REQ_DATA_PROCESS,
	STATE_REQ_END,
};

struct dw_mci_req_log {
	u64 timestamp;
	u32 info0;
	u32 info1;
	u32 info2;
	u32 info3;
	u32 pending_events;
	u32 completed_events;
	enum dw_mci_state state;
	enum dw_mci_state state_cmd;
	enum dw_mci_state state_dat;
	enum dw_mci_req_log_state log_state;
};

#define DWMCI_LOG_MAX		0x80
#define DWMCI_REQ_LOG_MAX	0x40
struct dw_mci_debug_info {
	struct dw_mci_cmd_log cmd_log[DWMCI_LOG_MAX];
	atomic_t cmd_log_count;
	struct dw_mci_req_log req_log[DWMCI_REQ_LOG_MAX];
	atomic_t req_log_count;
	unsigned char en_logging;
#define DW_MCI_DEBUG_ON_CMD	BIT(0)
#define DW_MCI_DEBUG_ON_REQ	BIT(1)
};

#define DWMCI_DBG_NUM_HOST	3

#define DWMCI_DBG_NUM_INFO	3	/* configurable */
#define DWMCI_DBG_MASK_INFO	(BIT(0) | BIT(1) | BIT(2))	/* configurable */
#define DWMCI_DBG_BIT_HOST(x)	BIT(x)

struct dw_mci_debug_data {
	unsigned char host_count;
	unsigned char info_count;
	unsigned char info_index[DWMCI_DBG_NUM_HOST];
	struct dw_mci *host[DWMCI_DBG_NUM_HOST];
	struct dw_mci_debug_info debug_info[DWMCI_DBG_NUM_INFO];
};

struct dw_mci_tuning_data {
	const u8 *blk_pattern;
	unsigned int blksz;
};

/**
 * dw_mci driver data - dw-mshc implementation specific driver data.
 * @caps: mmc subsystem specified capabilities of the controller(s).
 * @num_caps: number of capabilities specified by @caps.
 * @init: early implementation specific initialization.
 * @set_ios: handle bus specific extensions.
 * @parse_dt: parse implementation specific device tree properties.
 * @execute_tuning: implementation specific tuning procedure.
 *
 * Provide controller implementation specific extensions. The usage of this
 * data structure is fully optional and usage of each member in this structure
 * is optional as well.
 */
struct dw_mci_drv_data {
	unsigned long *caps;
	u32 num_caps;
	int (*init) (struct dw_mci * host);
	void (*set_ios) (struct dw_mci * host, struct mmc_ios * ios);
	int (*parse_dt) (struct dw_mci * host);
	int (*execute_tuning) (struct dw_mci_slot * slot, u32 opcode,
			       struct dw_mci_tuning_data * tuning_data);
	int (*prepare_hs400_tuning) (struct dw_mci * host, struct mmc_ios * ios);
	int (*switch_voltage) (struct mmc_host * mmc, struct mmc_ios * ios);
	void (*hwacg_control) (struct dw_mci * host, u32 flag);
	void (*pins_control) (struct dw_mci * host, int config);
	int (*misc_control) (struct dw_mci * host, enum dw_mci_misc_control control, void *priv);
	int (*crypto_engine_cfg) (struct dw_mci * host,
				  void *desc,
				  struct mmc_data * data,
				  struct page * page, int sector_offset, bool cmdq_enabled);
	int (*crypto_engine_clear) (struct dw_mci * host, void *desc, bool cmdq_enabled);
	int (*access_control_get_dev) (struct dw_mci * host);
	int (*access_control_sec_cfg) (struct dw_mci * host);
	int (*access_control_init) (struct dw_mci * host);
	int (*access_control_abort) (struct dw_mci * host);
	int (*access_control_resume) (struct dw_mci * host);
	void (*ssclk_control) (struct dw_mci * host, int enable);
};

struct dw_mci_sfr_ram_dump {
	u32 contrl;
	u32 pwren;
	u32 clkdiv;
	u32 clkena;
	u32 clksrc;
	u32 tmout;
	u32 ctype;
	u32 blksiz;
	u32 bytcnt;
	u32 intmask;
	u32 cmdarg;
	u32 cmd;
	u32 mintsts;
	u32 rintsts;
	u32 status;
	u32 fifoth;
	u32 tcbcnt;
	u32 tbbcnt;
	u32 debnce;
	u32 uhs_reg;
	u32 bmod;
	u32 pldmnd;
	u32 dbaddrl;
	u32 dbaddru;
	u32 dscaddrl;
	u32 dscaddru;
	u32 bufaddru;
	u32 dbaddr;
	u32 dscaddr;
	u32 bufaddr;
	u32 clksel;
	u32 idsts64;
	u32 idinten64;
	u32 force_clk_stop;
	u32 cdthrctl;
	u32 hs400_rdqs_en;
	u32 hs400_acync_fifo_ctrl;
	u32 hs400_dline_ctrl;
	u32 fmp_emmcp_base;
	u32 mpsecurity;
	u32 mpstat;
	u32 mpsbegin;
	u32 mpsend;
	u32 mpsctrl;
	u32 cmd_status;
	u32 data_status;
	unsigned long pending_events;
	unsigned long completed_events;
	u32 host_state;
	u32 cmd_index;
	u32 fifo_count;
	u32 data_busy;
	u32 data_3_state;
	u32 fifo_tx_watermark;
	u32 fifo_rx_watermark;
};
#endif				/* _DW_MMC_H_ */
