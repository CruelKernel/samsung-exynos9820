/*
 * Copyright (C) 2012 Samsung Electronics.
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

#ifndef __MODEM_V1_H__
#define __MODEM_V1_H__

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/types.h>

#ifdef CONFIG_LINK_DEVICE_SHMEM
#include <linux/shm_ipc.h>
#endif

#define MAX_STR_LEN		256
#define MAX_NAME_LEN		64
#define MAX_DUMP_LEN		20

#define SMC_ID		0x82000700
#define SMC_ID_CLK	0x82001011
#define SSS_CLK_ENABLE	0
#define SSS_CLK_DISABLE	1

enum modem_t {
	IMC_XMM6260,
	IMC_XMM6262,
	VIA_CBP71,
	VIA_CBP72,
	VIA_CBP82,
	SEC_CMC220,
	SEC_CMC221,
	SEC_SS222,
	SEC_SS300,
	SEC_SH222AP,
	SEC_SS310AP,
	SEC_S5000AP,
	SEC_S5100,
	QC_MDM6600,
	QC_ESC6270,
	QC_QSC6085,
	SPRD_SC8803,
	IMC_XMM7260,
	DUMMY,
	MAX_MODEM_TYPE
};

enum dev_format {
	IPC_FMT,
	IPC_RAW,
	IPC_RFS,
	IPC_MULTI_RAW,
	IPC_BOOT,
	IPC_DUMP,
	IPC_CMD,
	IPC_DEBUG,
	MAX_DEV_FORMAT,
};

enum legacy_ipc_map {
	IPC_MAP_FMT = 0,
#ifdef CONFIG_MODEM_IF_LEGACY_QOS
	IPC_MAP_HPRIO_RAW,
#endif
	IPC_MAP_NORM_RAW,
	MAX_SIPC_MAP,
};

#define MAX_SIPC_CHANNELS	256	/* 2 ^ 8		*/
#define MAX_LINK_CHANNELS	32	/* up to 32 channels	*/

enum modem_io {
	IODEV_MISC,
	IODEV_NET,
	IODEV_DUMMY,
};

/* Caution!!
 * Please make sure that below sequence is exactly matched with cbd.
 * If you want one more link type, append it to the end of list.
 */
enum modem_link {
	LINKDEV_UNDEFINED,
	LINKDEV_MIPI,
	LINKDEV_DPRAM,
	LINKDEV_SPI,
	LINKDEV_USB,
	LINKDEV_HSIC,
	LINKDEV_C2C,
	LINKDEV_UART,
	LINKDEV_PLD,
	LINKDEV_SHMEM,
	LINKDEV_LLI,
	LINKDEV_PCIE,
	LINKDEV_MAX
};
#define LINKTYPE(modem_link) (1u << (modem_link))

enum modem_network {
	UMTS_NETWORK,
	CDMA_NETWORK,
	TDSCDMA_NETWORK,
	LTE_NETWORK,
	MAX_MODEM_NETWORK
};

enum sipc_ver {
	NO_SIPC_VER = 0,
	SIPC_VER_40 = 40,
	SIPC_VER_41 = 41,
	SIPC_VER_42 = 42,
	SIPC_VER_50 = 50,
	MAX_SIPC_VER
};

enum sipc_ch_id {
	SIPC_CH_ID_RAW_0 = 0,	/*reserved*/
	SIPC_CH_ID_CS_VT_DATA,
	SIPC_CH_ID_CS_VT_CONTROL,
	SIPC_CH_ID_CS_VT_AUDIO,
	SIPC_CH_ID_CS_VT_VIDEO,
	SIPC_CH_ID_RAW_5,	/*reserved*/
	SIPC_CH_ID_RAW_6,	/*reserved*/
	SIPC_CH_ID_CDMA_DATA,
	SIPC_CH_ID_PCM_DATA,
	SIPC_CH_ID_TRANSFER_SCREEN,

	SIPC_CH_ID_PDP_0 = 10,	/*ID:10*/
	SIPC_CH_ID_PDP_1,
	SIPC_CH_ID_PDP_2,
	SIPC_CH_ID_PDP_3,
	SIPC_CH_ID_PDP_4,
	SIPC_CH_ID_PDP_5,
	SIPC_CH_ID_PDP_6,
	SIPC_CH_ID_PDP_7,
	SIPC_CH_ID_PDP_8,
	SIPC_CH_ID_PDP_9,
	SIPC_CH_ID_PDP_10,
	SIPC_CH_ID_PDP_11,
	SIPC_CH_ID_PDP_12,
	SIPC_CH_ID_PDP_13,
	SIPC_CH_ID_PDP_14,
	SIPC_CH_ID_BT_DUN,	/*ID:25*/
	SIPC_CH_ID_CIQ_DATA,
	SIPC_CH_ID_PDP_17,	/*reserved*/
	SIPC_CH_ID_CPLOG1,	/*ID:28*/
	SIPC_CH_ID_CPLOG2,	/*ID:29*/
	SIPC_CH_ID_LOOPBACK1,	/*ID:30*/
	SIPC_CH_ID_LOOPBACK2,	/*ID:31*/

	SIPC_CH_ID_CASS = 35,

	/* 32 ~ 214 are reserved */

	SIPC5_CH_ID_BOOT_0 = 215,
	SIPC5_CH_ID_BOOT_1,
	SIPC5_CH_ID_BOOT_2,
	SIPC5_CH_ID_BOOT_3,
	SIPC5_CH_ID_BOOT_4,
	SIPC5_CH_ID_BOOT_5,
	SIPC5_CH_ID_BOOT_6,
	SIPC5_CH_ID_BOOT_7,
	SIPC5_CH_ID_BOOT_8,
	SIPC5_CH_ID_BOOT_9,

	SIPC5_CH_ID_DUMP_0 = 225,
	SIPC5_CH_ID_DUMP_1,
	SIPC5_CH_ID_DUMP_2,
	SIPC5_CH_ID_DUMP_3,
	SIPC5_CH_ID_DUMP_4,
	SIPC5_CH_ID_DUMP_5,
	SIPC5_CH_ID_DUMP_6,
	SIPC5_CH_ID_DUMP_7,
	SIPC5_CH_ID_DUMP_8,
	SIPC5_CH_ID_DUMP_9,

	SIPC5_CH_ID_FMT_0 = 235,
	SIPC5_CH_ID_FMT_1,
	SIPC5_CH_ID_FMT_2,
	SIPC5_CH_ID_FMT_3,
	SIPC5_CH_ID_FMT_4,
	SIPC5_CH_ID_FMT_5,
	SIPC5_CH_ID_FMT_6,
	SIPC5_CH_ID_FMT_7,
	SIPC5_CH_ID_FMT_8,
	SIPC5_CH_ID_FMT_9,

	SIPC5_CH_ID_RFS_0 = 245,
	SIPC5_CH_ID_RFS_1,
	SIPC5_CH_ID_RFS_2,
	SIPC5_CH_ID_RFS_3,
	SIPC5_CH_ID_RFS_4,
	SIPC5_CH_ID_RFS_5,
	SIPC5_CH_ID_RFS_6,
	SIPC5_CH_ID_RFS_7,
	SIPC5_CH_ID_RFS_8,
	SIPC5_CH_ID_RFS_9,

	SIPC5_CH_ID_MAX = 255
};

struct __packed multi_frame_control {
	u8 id:7,
	   more:1;
};

enum io_mode {
	PIO,
	DMA
};

enum direction {
	TX = 0,
	UL = 0,
	AP2CP = 0,
	RX = 1,
	DL = 1,
	CP2AP = 1,
	TXRX = 2,
	ULDL = 2,
	MAX_DIR = 2
};

enum read_write {
	RD = 0,
	WR = 1,
	RDWR = 2
};

#define STR_CP_FAIL	"cp_fail"
#define STR_CP_WDT	"cp_wdt"	/* CP watchdog timer */

/* You can define modem specific attribute here.
 * It could be all the different behaviour between many modem vendor.
 */
enum link_attr_bit {
	LINK_ATTR_SBD_IPC,	/* IPC over SBD (from MIPI-LLI)		*/
	LINK_ATTR_IPC_ALIGNED,	/* IPC with 4-bytes alignment		*/
	LINK_ATTR_IOSM_MESSAGE,	/* IOSM message				*/
	LINK_ATTR_DPRAM_MAGIC,	/* DPRAM-style magic code validation	*/
	/*--------------------------------------------------------------*/
	LINK_ATTR_SBD_BOOT,	/* BOOT over SBD			*/
	LINK_ATTR_SBD_DUMP,	/* DUMP over SBD			*/
	LINK_ATTR_MEM_BOOT,	/* BOOT over legacy memory-type I/F	*/
	LINK_ATTR_MEM_DUMP,	/* DUMP over legacy memory-type I/F	*/
	LINK_ATTR_BOOT_ALIGNED,	/* BOOT with 4-bytes alignment		*/
	LINK_ATTR_DUMP_ALIGNED,	/* DUMP with 4-bytes alignment		*/
	/*--------------------------------------------------------------*/
	LINK_ATTR_XMIT_BTDLR,	/* Used to download CP bootloader	*/
};
#define LINK_ATTR(b)	(0x1 << b)

enum iodev_attr_bit {
	ATTR_SIPC4,
	ATTR_SIPC5,
	ATTR_CDC_NCM,
	ATTR_MULTIFMT,
	ATTR_HANDOVER,
	ATTR_LEGACY_RFS,
	ATTR_RX_FRAGMENT,
	ATTR_SBD_IPC,		/* IPC using SBD designed from MIPI-LLI	*/
	ATTR_NO_LINK_HEADER,	/* Link-layer header is not needed	*/
	ATTR_NO_CHECK_MAXQ,     /* no need to check rxq overflow condition */
	ATTR_DUALSIM,		/* support Dual SIM */
	ATTR_OPTION_REGION,	/* region & operator info */
	ATTR_ZEROCOPY,		/* support Zerocopy : 0x1 << 12*/
	ATTR_DIT,		/* Support DIT */
};
#define IODEV_ATTR(b)	(0x1 << b)

/**
 * struct modem_io_t - declaration for io_device
 * @name:	device name
 * @id:		for SIPC4, contains format & channel information
 *		(id & 11100000b)>>5 = format  (eg, 0=FMT, 1=RAW, 2=RFS)
 *		(id & 00011111b)    = channel (valid only if format is RAW)
 *		for SIPC5, contains only 8-bit channel ID
 * @format:	device format
 * @io_type:	type of this io_device
 * @links:	list of link_devices to use this io_device
 *		for example, if you want to use DPRAM and USB in an io_device.
 *		.links = LINKTYPE(LINKDEV_DPRAM) | LINKTYPE(LINKDEV_USB)
 * @tx_link:	when you use 2+ link_devices, set the link for TX.
 *		If define multiple link_devices in @links,
 *		you can receive data from them. But, cannot send data to all.
 *		TX is only one link_device.
 * @app:	the name of the application that will use this IO device
 *
 */
struct modem_io_t {
	char *name;
	u32 id;
	enum dev_format format;
	enum modem_io io_type;
	u32 links;
	enum modem_link tx_link;
	u32 attrs;
	char *app;
	char *option_region;
#if 1/*defined(CONFIG_LINK_DEVICE_MEMORY_SBD)*/
	unsigned int ul_num_buffers;
	unsigned int ul_buffer_size;
	unsigned int dl_num_buffers;
	unsigned int dl_buffer_size;
#endif
};

struct modemlink_pm_data {
	char *name;
	struct device *dev;
	/* link power contol 2 types : pin & regulator control */
	int (*link_ldo_enable)(bool);
	unsigned int gpio_link_enable;
	unsigned int gpio_link_active;
	unsigned int gpio_link_hostwake;
	unsigned int gpio_link_slavewake;
	int (*link_reconnect)(void);

	/* usb hub only */
	int (*port_enable)(int, int);
	int (*hub_standby)(void *);
	void *hub_pm_data;
	bool has_usbhub;

	/* cpu/bus frequency lock */
	atomic_t freqlock;
	int (*freq_lock)(struct device *dev);
	int (*freq_unlock)(struct device *dev);

	int autosuspend_delay_ms; /* if zero, the default value is used */
	void (*ehci_reg_dump)(struct device *);
};

struct modemlink_pm_link_activectl {
	int gpio_initialized;
	int gpio_request_host_active;
};

#if defined(CONFIG_LINK_DEVICE_SHMEM) || defined(CONFIG_LINK_DEVICE_PCIE)
enum shmem_type {
	REAL_SHMEM,
	C2C_SHMEM,
	MAX_SHMEM_TYPE
};

struct modem_mbox {
	unsigned int mbx_ap2cp_msg;
	unsigned int mbx_cp2ap_msg;
	unsigned int mbx_ap2cp_wakeup;	/* CP_WAKEUP	*/
	unsigned int mbx_cp2ap_wakeup;	/* AP_WAKEUP	*/
	unsigned int mbx_ap2cp_status;	/* AP_STATUS	*/
	unsigned int mbx_cp2ap_status;	/* CP_STATUS	*/
	unsigned int mbx_cp2ap_wakelock; /* Wakelock for VoLTE */
	unsigned int mbx_cp2ap_ratmode; /* Wakelock for pcie */
	unsigned int mbx_ap2cp_kerneltime; /* Kernel time */

	unsigned int int_ap2cp_msg;
	unsigned int int_ap2cp_active;
	unsigned int int_ap2cp_wakeup;
	unsigned int int_ap2cp_status;
	unsigned int int_ap2cp_uart_noti;

	unsigned int irq_cp2ap_msg;
	unsigned int irq_cp2ap_active;
	unsigned int irq_cp2ap_wakeup;
	unsigned int irq_cp2ap_status;
	unsigned int irq_cp2ap_wakelock;
	unsigned int irq_cp2ap_rat_mode;

	/* Performance request */
	unsigned int mbx_ap2cp_perf_req;
	unsigned int mbx_cp2ap_perf_req;
	unsigned int mbx_cp2ap_perf_req_cpu;
	unsigned int mbx_cp2ap_perf_req_mif;
	unsigned int mbx_cp2ap_perf_req_int;

	unsigned int int_ap2cp_perf_req;
	unsigned int irq_cp2ap_perf_req;
	unsigned int irq_cp2ap_perf_req_cpu;
	unsigned int irq_cp2ap_perf_req_mif;
	unsigned int irq_cp2ap_perf_req_int;

	unsigned int mbx_ap2cp_lock_value;

	/* Status Bit Info */
	unsigned int sbi_lte_active_mask;
	unsigned int sbi_lte_active_pos;
	unsigned int sbi_cp_status_mask;
	unsigned int sbi_cp_status_pos;
	unsigned int sbi_cp2ap_wakelock_mask;
	unsigned int sbi_cp2ap_wakelock_pos;
	unsigned int sbi_cp2ap_rat_mode_mask;
	unsigned int sbi_cp2ap_rat_mode_pos;

	unsigned int sbi_pda_active_mask;
	unsigned int sbi_pda_active_pos;
	unsigned int sbi_ap_status_mask;
	unsigned int sbi_ap_status_pos;

	unsigned int sbi_ap2cp_kerneltime_sec_mask;
	unsigned int sbi_ap2cp_kerneltime_sec_pos;
	unsigned int sbi_ap2cp_kerneltime_usec_mask;
	unsigned int sbi_ap2cp_kerneltime_usec_pos;

	unsigned int sbi_uart_noti_mask;
	unsigned int sbi_uart_noti_pos;

	unsigned int sbi_crash_type_mask;
	unsigned int sbi_crash_type_pos;

	/* Clk table Info */
	unsigned int *ap_clk_table;
	unsigned int ap_clk_cnt;

	unsigned int *mif_clk_table;
	unsigned int mif_clk_cnt;

	unsigned int *int_clk_table;
	unsigned int int_clk_cnt;
};
#endif

/* platform data */
struct modem_data {
	char *name;

	unsigned int gpio_cp_on;
	unsigned int gpio_cp_off;
	unsigned int gpio_reset_req_n;
	unsigned int gpio_cp_reset;

	/* for broadcasting AP's PM state (active or sleep) */
	unsigned int gpio_pda_active;

	/* for checking aliveness of CP */
	unsigned int gpio_phone_active;
	unsigned int irq_phone_active;

	/* for AP-CP IPC interrupt */
	unsigned int gpio_ipc_int2ap;
	unsigned int irq_ipc_int2ap;
	unsigned long irqf_ipc_int2ap;	/* IRQ flags */
	unsigned int gpio_ipc_int2cp;

	/* for AP-CP power management (PM) handshaking */
	unsigned int gpio_ap_wakeup;
	unsigned int irq_ap_wakeup;
	unsigned int gpio_ap_status;
	unsigned int gpio_cp_wakeup;
	unsigned int gpio_cp_status;
	unsigned int irq_cp_status;

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

#if defined(CONFIG_LINK_DEVICE_SHMEM) || defined(CONFIG_LINK_DEVICE_PCIE)
	struct modem_mbox *mbx;
	struct mem_link_device *mld;
#endif

	/* Switch with 2 links in a modem */
	unsigned int gpio_link_switch;

	/* Modem component */
	enum modem_network modem_net;
	enum modem_t modem_type;

	u32 link_types;
	char *link_name;
	unsigned long link_attrs;	/* Set of link_attr_bit flags	*/

	/* SIPC version */
	enum sipc_ver ipc_version;

	/* Information of IO devices */
	unsigned int num_iodevs;
	struct modem_io_t *iodevs;

	/* Modem link PM support */
	struct modemlink_pm_data *link_pm_data;

	/* Handover with 2+ modems */
	bool use_handover;

	/* SIM Detect polarity */
	bool sim_polarity;

	void (*gpio_revers_bias_clear)(void);
	void (*gpio_revers_bias_restore)(void);
};

struct modem_irq {
	spinlock_t lock;
	unsigned int num;
	char name[MAX_NAME_LEN];
	unsigned long flags;
	bool active;
	bool registered;
};

#define MODEM_BOOT_DEV_SPI "spi_boot_link"

struct modem_boot_spi_platform_data {
	const char *name;
	unsigned int gpio_cp_status;
};

struct modem_boot_spi {
	struct miscdevice misc_dev;
	struct spi_device *spi_dev;
	struct mutex lock;
	unsigned int gpio_cp_status;
};
#define to_modem_boot_spi(misc)	\
		container_of(misc, struct modem_boot_spi, misc_dev);

#ifdef CONFIG_OF
#define mif_dt_read_enum(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) { \
			mif_err("%s is not defined\n", prop); \
			return -EINVAL; \
		} \
		dest = (__typeof__(dest))(val); \
	} while (0)

#define mif_dt_read_bool(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) { \
			mif_err("%s is not defined\n", prop); \
			return -EINVAL; \
		} \
		dest = val ? true : false; \
	} while (0)

#define mif_dt_read_string(np, prop, dest) \
	do { \
		if (of_property_read_string(np, prop, \
				(const char **)&dest)) { \
			mif_err("%s is not defined\n", prop); \
			return -EINVAL; \
		} \
	} while (0)

#define mif_dt_read_u32(np, prop, dest) \
	do { \
		u32 val; \
		if (of_property_read_u32(np, prop, &val)) { \
			mif_err("%s is not defined\n", prop); \
			return -EINVAL; \
		} \
		dest = val; \
	} while (0)

#define mif_dt_read_u32_noerr(np, prop, dest) \
	do { \
		u32 val; \
		if (!of_property_read_u32(np, prop, &val)) \
			dest = val; \
	} while (0)
#endif

#define LOG_TAG	"mif: "
#define FUNC	(__func__)
#define CALLER	(__builtin_return_address(0))

#define mif_err(fmt, ...) \
	pr_err(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_debug(fmt, ...) \
	pr_debug(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_info(fmt, ...) \
	pr_info(LOG_TAG "%s: " pr_fmt(fmt), __func__, ##__VA_ARGS__)
#define mif_err_limited(fmt, ...) \
	net_ratelimited_function(mif_err, fmt, ##__VA_ARGS__)
#define mif_trace(fmt, ...) \
	printk(KERN_DEBUG "mif: %s: %d: called(%pF): " fmt, \
		__func__, __LINE__, __builtin_return_address(0), ##__VA_ARGS__)

#endif
