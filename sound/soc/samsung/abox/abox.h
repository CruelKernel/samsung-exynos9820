/* sound/soc/samsung/abox/abox.h
 *
 * ALSA SoC - Samsung Abox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_H
#define __SND_SOC_ABOX_H

#include <sound/samsung/abox.h>
#include <linux/miscdevice.h>
#include <linux/dma-direction.h>
#include <linux/completion.h>
#include "abox_qos.h"
#include "abox_soc.h"

#define DEFAULT_CPU_GEAR_ID		(0xAB0CDEFA)
#define TEST_CPU_GEAR_ID		(DEFAULT_CPU_GEAR_ID + 1)
#define DEFAULT_LIT_FREQ_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_BIG_FREQ_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_HMP_BOOST_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_INT_FREQ_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_MIF_FREQ_ID		DEFAULT_CPU_GEAR_ID
#define DEFAULT_SYS_POWER_ID		DEFAULT_CPU_GEAR_ID

#define BUFFER_BYTES_MIN		(SZ_64K)
#define BUFFER_BYTES_MAX		(SZ_1M)
#define PERIOD_BYTES_MIN		(SZ_16)
#define PERIOD_BYTES_MAX		(BUFFER_BYTES_MAX / 2)

#define DRAM_FIRMWARE_SIZE		CONFIG_SND_SOC_SAMSUNG_ABOX_DRAM_SIZE
#define IOVA_DRAM_FIRMWARE		(0x80000000)
#define IOVA_RDMA_BUFFER_BASE		(0x91000000)
#define IOVA_RDMA_BUFFER(x)		(IOVA_RDMA_BUFFER_BASE + (SZ_1M * x))
#define IOVA_WDMA_BUFFER_BASE		(0x92000000)
#define IOVA_WDMA_BUFFER(x)		(IOVA_WDMA_BUFFER_BASE + (SZ_1M * x))
#define IOVA_COMPR_BUFFER_BASE		(0x93000000)
#define IOVA_COMPR_BUFFER(x)		(IOVA_COMPR_BUFFER_BASE + (SZ_1M * x))
#define IOVA_VDMA_BUFFER_BASE		(0x94000000)
#define IOVA_VDMA_BUFFER(x)		(IOVA_VDMA_BUFFER_BASE + (SZ_1M * x))
#define IOVA_VSS_FIRMWARE		(0xA0000000)
#define IOVA_VSS_PARAMETER		(0xA1000000)
#define IOVA_DUMP_BUFFER		(0xD0000000)
#define IOVA_PRIVATE			(0xE0000000)
#define PRIVATE_SIZE			(SZ_8M)
#define PHSY_VSS_FIRMWARE		(0xFEE00000)
#define PHSY_VSS_SIZE			(SZ_4M + SZ_2M)

#define AUD_PLL_RATE_HZ_FOR_48000	(1179648040)
#define AUD_PLL_RATE_HZ_FOR_44100	(1083801600)

#define LIMIT_IN_JIFFIES		(msecs_to_jiffies(1000))

#define ABOX_CPU_GEAR_CALL_VSS		(0xCA11)
#define ABOX_CPU_GEAR_CALL_KERNEL	(0xCA12)
#define ABOX_CPU_GEAR_CALL		ABOX_CPU_GEAR_CALL_VSS
#define ABOX_CPU_GEAR_ABSOLUTE		(0xABC0ABC0)
#define ABOX_CPU_GEAR_BOOT		(0xB00D)
#define ABOX_CPU_GEAR_MAX		(1)
#define ABOX_CPU_GEAR_MIN		(12)
#define ABOX_CPU_GEAR_DAI		0xDA100000

#define ABOX_DMA_TIMEOUT_NS		(40000000)

#define ABOX_SAMPLING_RATES (SNDRV_PCM_RATE_KNOT)
#define ABOX_SAMPLE_FORMATS (SNDRV_PCM_FMTBIT_S16\
		| SNDRV_PCM_FMTBIT_S24\
		| SNDRV_PCM_FMTBIT_S32)
#define ABOX_WDMA_SAMPLE_FORMATS (SNDRV_PCM_FMTBIT_S16\
		| SNDRV_PCM_FMTBIT_S24\
		| SNDRV_PCM_FMTBIT_S32)

#define ABOX_SUPPLEMENT_SIZE (SZ_128)
#define ABOX_IPC_QUEUE_SIZE (SZ_128)

#define CALLIOPE_VERSION(class, year, month, minor) \
		((class << 24) | \
		((year - 1 + 'A') << 16) | \
		((month - 1 + 'A') << 8) | \
		((minor + '0') << 0))

enum abox_dai {
	ABOX_RDMA0 = 0x10,
	ABOX_RDMA1,
	ABOX_RDMA2,
	ABOX_RDMA3,
	ABOX_RDMA4,
	ABOX_RDMA5,
	ABOX_RDMA6,
	ABOX_RDMA7,
	ABOX_RDMA8,
	ABOX_RDMA9,
	ABOX_RDMA10,
	ABOX_RDMA11,
	ABOX_WDMA0 = 0x20,
	ABOX_WDMA1,
	ABOX_WDMA2,
	ABOX_WDMA3,
	ABOX_WDMA4,
	ABOX_WDMA5,
	ABOX_WDMA6,
	ABOX_WDMA7,
	ABOX_UAIF0 = 0x30,
	ABOX_UAIF1,
	ABOX_UAIF2,
	ABOX_UAIF3,
	ABOX_UAIF4,
	ABOX_DSIF,
	ABOX_SIFS0 = 0x40, /* Virtual DAI */
	ABOX_SIFS1, /* Virtual DAI */
	ABOX_SIFS2, /* Virtual DAI */
	ABOX_SIFS3, /* Virtual DAI */
	ABOX_SIFS4, /* Virtual DAI */
	ABOX_RSRC0 = 0x50, /* Virtual DAI */
	ABOX_RSRC1, /* Virtual DAI */
	ABOX_NSRC0, /* Virtual DAI */
	ABOX_NSRC1, /* Virtual DAI */
	ABOX_NSRC2, /* Virtual DAI */
	ABOX_NSRC3, /* Virtual DAI */
	ABOX_NSRC4, /* Virtual DAI */
	ABOX_NSRC5, /* Virtual DAI */
	ABOX_NSRC6, /* Virtual DAI */
};

#define ABOX_DAI_COUNT (ABOX_RSRC0 - ABOX_UAIF0)

enum calliope_state {
	CALLIOPE_DISABLED,
	CALLIOPE_DISABLING,
	CALLIOPE_ENABLING,
	CALLIOPE_ENABLED,
	CALLIOPE_STATE_COUNT,
};

enum audio_mode {
	MODE_NORMAL,
	MODE_RINGTONE,
	MODE_IN_CALL,
	MODE_IN_COMMUNICATION,
	MODE_IN_VIDEOCALL,
};

enum sound_type {
	SOUND_TYPE_VOICE,
	SOUND_TYPE_SPEAKER,
	SOUND_TYPE_HEADSET,
	SOUND_TYPE_BTVOICE,
	SOUND_TYPE_USB,
	SOUND_TYPE_CALLFWD,
};

enum qchannel {
	ABOX_CCLK_CA7,
	ABOX_ACLK,
	ABOX_BCLK_UAIF0,
	ABOX_BCLK_UAIF1,
	ABOX_BCLK_UAIF2,
	ABOX_BCLK_UAIF3,
	ABOX_BCLK_DSIF,
	ABOX_CCLK_ATB,
	ABOX_CCLK_ASB,
};

#define ABOX_QUIRK_BIT_TRY_TO_ASRC_OFF	(1 << 0)
#define ABOX_QUIRK_BIT_SHARE_VTS_SRAM	(1 << 1)
#define ABOX_QUIRK_STR_TRY_TO_ASRC_OFF	"try to asrc off"
#define ABOX_QUIRK_STR_SHARE_VTS_SRAM	"share vts sram"

struct abox_ipc {
	struct device *dev;
	int hw_irq;
	unsigned long long put_time;
	unsigned long long get_time;
	ABOX_IPC_MSG msg;
	size_t size;
};

struct abox_ipc_action {
	struct list_head list;
	const struct device *dev;
	int ipc_id;
	abox_ipc_handler_t handler;
	void *data;
};

struct abox_iommu_mapping {
	struct list_head list;
	unsigned long iova;	/* IO virtual address */
	unsigned char *area;	/* virtual pointer */
	dma_addr_t addr;	/* physical address */
	size_t bytes;		/* buffer size in bytes */
};

struct abox_dram_request {
	unsigned int id;
	bool on;
	unsigned long long updated;
};

struct abox_extra_firmware {
	struct list_head list;
	const struct firmware *firmware;
	const char *name;
	u32 area;
	u32 offset;
	int kcontrol;
};

struct abox_component {
	struct ABOX_COMPONENT_DESCRIPTIOR *desc;
	bool registered;
	struct list_head value_list;
};

struct abox_component_kcontrol_value {
	struct ABOX_COMPONENT_DESCRIPTIOR *desc;
	struct ABOX_COMPONENT_CONTROL *control;
	struct list_head list;
	bool cache_only;
	int cache[];
};

struct abox_data {
	struct platform_device *pdev;
	struct snd_soc_component *cmpnt;
	struct regmap *regmap;
	struct regmap *timer_regmap;
	void __iomem *sfr_base;
	void __iomem *sysreg_base;
	void __iomem *sram_base;
	void __iomem *timer_base;
	phys_addr_t sram_base_phys;
	size_t sram_size;
	void *dram_base;
	dma_addr_t dram_base_phys;
	void *dump_base;
	phys_addr_t dump_base_phys;
	void *priv_base;
	phys_addr_t priv_base_phys;
	struct iommu_domain *iommu_domain;
	unsigned int ipc_tx_offset;
	unsigned int ipc_rx_offset;
	unsigned int ipc_tx_ack_offset;
	unsigned int ipc_rx_ack_offset;
	void *ipc_tx_addr;
	size_t ipc_tx_size;
	void *ipc_rx_addr;
	size_t ipc_rx_size;
	struct abox2host_hndshk_tag *hndshk_tag;
	int clk_diff_ppb;
	int ipc_version;
	unsigned int if_count;
	unsigned int rdma_count;
	unsigned int wdma_count;
	unsigned int calliope_version;
	struct list_head firmware_extra;
	struct device *dev_gic;
	struct platform_device *pdev_if[8];
	struct platform_device *pdev_rdma[16];
	struct platform_device *pdev_wdma[16];
	struct platform_device *pdev_vts;
	struct workqueue_struct *ipc_workqueue;
	struct work_struct ipc_work;
	struct abox_ipc ipc_queue[ABOX_IPC_QUEUE_SIZE];
	int ipc_queue_start;
	int ipc_queue_end;
	spinlock_t ipc_queue_lock;
	wait_queue_head_t ipc_wait_queue;
	wait_queue_head_t wait_queue;
	struct clk *clk_pll;
	struct clk *clk_pll1;
	struct clk *clk_audif;
	struct clk *clk_cpu;
	struct clk *clk_dmic;
	struct clk *clk_bus;
	struct clk *clk_cnt;
	unsigned int uaif_max_div;
	struct pinctrl *pinctrl;
	unsigned long quirks;
	unsigned int cpu_gear;
	unsigned int cpu_gear_min;
	struct abox_dram_request dram_requests[16];
	unsigned long audif_rates[ABOX_DAI_COUNT];
	unsigned int sif_rate[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	snd_pcm_format_t sif_format[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	unsigned int sif_channels[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	unsigned int sif_rate_min[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	snd_pcm_format_t sif_format_min[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	unsigned int sif_channels_min[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	bool sif_auto_config[SET_SIFS0_FORMAT - SET_SIFS0_RATE];
	int apf_coef[2][16];
	struct work_struct register_component_work;
	struct abox_component components[16];
	struct list_head ipc_actions;
	struct list_head iommu_maps;
	spinlock_t iommu_lock;
	bool enabled;
	bool restored;
	bool no_profiling;
	enum calliope_state calliope_state;
	bool failsafe;
	struct notifier_block qos_nb;
	struct notifier_block pm_nb;
	struct notifier_block modem_nb;
	struct notifier_block itmon_nb;
	int pm_qos_int[5];
	int pm_qos_aud[5];
	struct work_struct restore_data_work;
	struct work_struct boot_done_work;
	struct delayed_work wdt_work;
	unsigned long long audio_mode_time;
	enum audio_mode audio_mode;
	enum sound_type sound_type;
	struct wakeup_source ws;
};

struct abox_compr_data {
	/* compress offload */
	struct snd_compr_stream *cstream;

	void *dma_area;
	size_t dma_size;
	dma_addr_t dma_addr;

	unsigned int block_num;
	unsigned int handle_id;
	unsigned int codec_id;
	unsigned int channels;
	unsigned int sample_rate;

	unsigned int byte_offset;
	u64 copied_total;
	u64 received_total;

	bool start;
	bool eos;
	bool created;
	bool bespoke_start;
	bool dirty;

	bool effect_on;

	wait_queue_head_t flush_wait;
	wait_queue_head_t exit_wait;
	wait_queue_head_t ipc_wait;

	uint32_t stop_ack;
	uint32_t exit_ack;

	spinlock_t lock;
	struct mutex cmd_lock;

	int (*isr_handler)(void *data);

	struct snd_compr_params codec_param;

	/* effect offload */
	unsigned int out_sample_rate;
};

enum abox_platform_type {
	PLATFORM_NORMAL,
	PLATFORM_CALL,
	PLATFORM_COMPRESS,
	PLATFORM_REALTIME,
	PLATFORM_VI_SENSING,
	PLATFORM_SYNC,
};

enum abox_buffer_type {
	BUFFER_TYPE_DMA,
	BUFFER_TYPE_ION,
};

enum abox_rate {
	RATE_SUHQA,
	RATE_UHQA,
	RATE_NORMAL,
	RATE_COUNT,
};

/**
 * Test quirk
 * @param[in]	data	pointer to abox_data structure
 * @param[in]	quirk	quirk bit
 * @return	true or false
 */
static inline bool abox_test_quirk(struct abox_data *data, unsigned long quirk)
{
	return !!(data->quirks & quirk);
}

/**
 * Get sampling rate type
 * @param[in]	rate		sampling rate in Hz
 * @return	rate type in enum abox_rate
 */
static inline enum abox_rate abox_get_rate_type(unsigned int rate)
{
	if (rate < 176400)
		return RATE_NORMAL;
	else if (rate >= 176400 && rate <= 192000)
		return RATE_UHQA;
	else
		return RATE_SUHQA;
}

/**
 * Get SFR of sample format
 * @param[in]	width		count of bit in sample
 * @param[in]	channel		count of channel
 * @return	SFR of sample format
 */
static inline u32 abox_get_format(u32 width, u32 channels)
{
	u32 ret = (channels - 1);

	switch (width) {
	case 16:
		ret |= 1 << 3;
		break;
	case 24:
		ret |= 2 << 3;
		break;
	case 32:
		ret |= 3 << 3;
		break;
	default:
		break;
	}

	pr_debug("%s(%u, %u): %u\n", __func__, width, channels, ret);

	return ret;
}

/**
 * Get enum IPC_ID from SNDRV_PCM_STREAM_*
 * @param[in]	stream	SNDRV_PCM_STREAM_*
 * @return	IPC_PCMPLAYBACK or IPC_PCMCAPTURE
 */
static inline enum IPC_ID abox_stream_to_ipcid(int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return IPC_PCMPLAYBACK;
	else if (stream == SNDRV_PCM_STREAM_CAPTURE)
		return IPC_PCMCAPTURE;
	else
		return -EINVAL;
}

/**
 * Get SNDRV_PCM_STREAM_* from enum IPC_ID
 * @param[in]	ipcid	IPC_PCMPLAYBACK or IPC_PCMCAPTURE
 * @return	SNDRV_PCM_STREAM_*
 */
static inline int abox_ipcid_to_stream(enum IPC_ID ipcid)
{
	if (ipcid == IPC_PCMPLAYBACK)
		return SNDRV_PCM_STREAM_PLAYBACK;
	else if (ipcid == IPC_PCMCAPTURE)
		return SNDRV_PCM_STREAM_CAPTURE;
	else
		return -EINVAL;
}

struct abox_dma_of_data {
	enum abox_dai (*get_dai_id)(int id);
	const char *(*get_dai_name)(struct device *dev, int id);
	const char *(*get_str_name)(struct device *dev, int id, int stream);
	struct snd_soc_dai_driver *base_dai_drv;
};

struct abox_ion_buf {
	size_t size;
	size_t align;
	void *ctx;
	void *kvaddr;
	void *kva;
	dma_addr_t iova;
	struct sg_table *sgt;

	struct dma_buf *dma_buf;
	struct dma_buf_attachment *attachment;
	enum dma_data_direction direction;
	int fd;

	void *priv;
};

struct abox_platform_data {
	struct platform_device *pdev;
	void __iomem *sfr_base;
	void __iomem *mailbox_base;
	unsigned int id;
	unsigned int pointer;
	int pm_qos_cl0[RATE_COUNT];
	int pm_qos_cl1[RATE_COUNT];
	int pm_qos_cl2[RATE_COUNT];
	struct device *dev_abox;
	struct abox_data *abox_data;
	struct snd_pcm_substream *substream;
	enum abox_platform_type type;
	struct snd_dma_buffer dmab;
	struct abox_ion_buf ion_buf;
	struct snd_hwdep *hwdep;
	bool mmap_fd_state;
	enum abox_buffer_type buf_type;
	bool ack_enabled;
	struct abox_compr_data compr_data;
	struct regmap *mailbox;
	struct snd_soc_component *cmpnt;
	struct snd_soc_dai_driver *dai_drv;
	const struct abox_dma_of_data *of_data;
	struct miscdevice misc_dev;
	struct completion closed;
};

/**
 * test given device is abox or not
 * @param[in]
 * @return	true or false
 */
extern bool is_abox(struct device *dev);

/**
 * get pointer to abox_data (internal use only)
 * @return		pointer to abox_data
 */
extern struct abox_data *abox_get_abox_data(void);

/**
 * get physical address from abox virtual address
 * @param[in]	data	pointer to abox_data structure
 * @param[in]	addr	abox virtual address
 * @return	physical address
 */
extern phys_addr_t abox_addr_to_phys_addr(struct abox_data *data,
		unsigned int addr);

/**
 * get kernel address from abox virtual address
 * @param[in]	data	pointer to abox_data structure
 * @param[in]	addr	abox virtual address
 * @return	kernel address
 */
extern void *abox_addr_to_kernel_addr(struct abox_data *data,
		unsigned int addr);

/**
 * Check specific cpu gear request is idle
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	id		key which is used as unique handle
 * @return	true if it is idle or not has been requested, false on otherwise
 */
extern bool abox_cpu_gear_idle(struct device *dev, unsigned int id);

/**
 * Request abox cpu clock level
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	level		gear level or frequency in kHz
 * @param[in]	name		cookie for logging
 * @return	error code if any
 */
extern int abox_request_cpu_gear(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int level, const char *name);

/**
 * Wait for pending cpu gear change
 */
extern void abox_cpu_gear_barrier(void);

/**
 * Request abox cpu clock level synchronously
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		key which is used as unique handle
 * @param[in]	level		gear level or frequency in kHz
 * @param[in]	name		cookie for logging
 * @return	error code if any
 */
extern int abox_request_cpu_gear_sync(struct device *dev,
		struct abox_data *data, unsigned int id, unsigned int level,
		const char *name);

/**
 * Clear abox cpu clock requests
 * @param[in]	dev		pointer to struct dev which invokes this API
 */
extern void abox_clear_cpu_gear_requests(struct device *dev);

/**
 * Request abox cpu clock level with dai
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	dai		DAI which is used as unique handle
 * @param[in]	level		gear level or frequency in kHz
 * @return	error code if any
 */
static inline int abox_request_cpu_gear_dai(struct device *dev,
		struct abox_data *data, struct snd_soc_dai *dai,
		unsigned int level)
{
	unsigned int id = ABOX_CPU_GEAR_DAI | dai->id;

	return abox_request_cpu_gear(dev, data, id, level, dai->name);
}

/**
 * Request cluster 0 clock level with DAI
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	dai		DAI which is used as unique handle
 * @param[in]	freq		frequency in kHz
 * @return	error code if any
 */
static inline int abox_request_cl0_freq_dai(struct device *dev,
		struct snd_soc_dai *dai, unsigned int freq)
{
	unsigned int id = ABOX_CPU_GEAR_DAI | dai->id;

	return abox_qos_request_cl0(dev, id, freq, dai->name);
}

/**
 * Request cluster 1 clock level with DAI
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	dai		DAI which is used as unique handle
 * @param[in]	freq		frequency in kHz
 * @return	error code if any
 */
static inline int abox_request_cl1_freq_dai(struct device *dev,
		struct snd_soc_dai *dai, unsigned int freq)
{
	unsigned int id = ABOX_CPU_GEAR_DAI | dai->id;

	return abox_qos_request_cl1(dev, id, freq, dai->name);
}

/**
 * Request cluster 2 clock level with DAI
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	dai		DAI which is used as unique handle
 * @param[in]	freq		frequency in kHz
 * @return	error code if any
 */
static inline int abox_request_cl2_freq_dai(struct device *dev,
		struct snd_soc_dai *dai, unsigned int freq)
{
	unsigned int id = ABOX_CPU_GEAR_DAI | dai->id;

	return abox_qos_request_cl2(dev, id, freq, dai->name);
}

/**
 * Register uaif to abox
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	id		id of the uaif
 * @param[in]	rate		sampling rate
 * @param[in]	channels	number of channels
 * @param[in]	width		number of bit in sample
 * @return	error code if any
 */
extern int abox_register_bclk_usage(struct device *dev, struct abox_data *data,
		enum abox_dai id, unsigned int rate, unsigned int channels,
		unsigned int width);

/**
 * disable or enable qchannel of a clock
 * @param[in]	dev		pointer to struct dev which invokes this API
 * @param[in]	data		pointer to abox_data structure
 * @param[in]	clk		clock id
 * @param[in]	disable		disable or enable
 */
extern int abox_disable_qchannel(struct device *dev, struct abox_data *data,
		enum qchannel clk, int disable);

/**
 * wait for restoring abox from suspend
 * @param[in]	data		pointer to abox_data structure
 */
extern void abox_wait_restored(struct abox_data *data);

/**
 * register sound card with specific order
 * @param[in]	dev		calling device
 * @param[in]	card		sound card to register
 * @param[in]	idx		order of the sound card
 */
extern int abox_register_extra_sound_card(struct device *dev,
		struct snd_soc_card *card, unsigned int idx);
#endif /* __SND_SOC_ABOX_H */
