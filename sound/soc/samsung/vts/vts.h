/* sound/soc/samsung/vts/vts.h
 *
 * ALSA SoC - Samsung VTS driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_VTS_H
#define __SND_SOC_VTS_H

#include <sound/memalloc.h>
#include <linux/wakelock.h>

/* SYSREG_VTS */
#define VTS_DEBUG			(0x0404)
#define VTS_DMIC_CLK_CTRL		(0x0408)
#define VTS_HWACG_CM4_CLKREQ		(0x0428)
#define VTS_DMIC_CLK_CON		(0x0434)
#define VTS_SYSPOWER_CTRL		(0x0500)
#define VTS_SYSPOWER_STATUS		(0x0504)

/* VTS_DEBUG */
#define VTS_NMI_EN_BY_WDT_OFFSET	(0)
#define VTS_NMI_EN_BY_WDT_SIZE		(1)
/* VTS_DMIC_CLK_CTRL */
#define VTS_CG_STATUS_OFFSET            (5)
#define VTS_CG_STATUS_SIZE		(1)
#define VTS_CLK_ENABLE_OFFSET		(4)
#define VTS_CLK_ENABLE_SIZE		(1)
#define VTS_CLK_SEL_OFFSET		(0)
#define VTS_CLK_SEL_SIZE		(1)
/* VTS_HWACG_CM4_CLKREQ */
#define VTS_MASK_OFFSET			(0)
#define VTS_MASK_SIZE			(32)
/* VTS_DMIC_CLK_CON */
#define VTS_ENABLE_CLK_GEN_OFFSET       (0)
#define VTS_ENABLE_CLK_GEN_SIZE         (1)
#define VTS_SEL_EXT_DMIC_CLK_OFFSET     (1)
#define VTS_SEL_EXT_DMIC_CLK_SIZE       (1)
#define VTS_ENABLE_CLK_CLK_GEN_OFFSET   (14)
#define VTS_ENABLE_CLK_CLK_GEN_SIZE     (1)

/* VTS_SYSPOWER_CTRL */
#define VTS_SYSPOWER_CTRL_OFFSET	(0)
#define VTS_SYSPOWER_CTRL_SIZE		(1)
/* VTS_SYSPOWER_STATUS */
#define VTS_SYSPOWER_STATUS_OFFSET	(0)
#define VTS_SYSPOWER_STATUS_SIZE	(1)


#define VTS_DMIC_ENABLE_DMIC_IF		(0x0000)
#define VTS_DMIC_CONTROL_DMIC_IF	(0x0004)
/* VTS_DMIC_ENABLE_DMIC_IF */
#define VTS_DMIC_ENABLE_DMIC_IF_OFFSET	(31)
#define VTS_DMIC_ENABLE_DMIC_IF_SIZE	(1)
#define VTS_DMIC_PERIOD_DATA2REQ_OFFSET	(16)
#define VTS_DMIC_PERIOD_DATA2REQ_SIZE	(2)
/* VTS_DMIC_CONTROL_DMIC_IF */
#define VTS_DMIC_HPF_EN_OFFSET		(31)
#define VTS_DMIC_HPF_EN_SIZE		(1)
#define VTS_DMIC_HPF_SEL_OFFSET		(28)
#define VTS_DMIC_HPF_SEL_SIZE		(1)
#define VTS_DMIC_CPS_SEL_OFFSET		(27)
#define VTS_DMIC_CPS_SEL_SIZE		(1)
#define VTS_DMIC_GAIN_OFFSET		(24)
#define VTS_DMIC_1DB_GAIN_OFFSET	(21)
#define VTS_DMIC_GAIN_SIZE		(3)
#define VTS_DMIC_DMIC_SEL_OFFSET	(18)
#define VTS_DMIC_DMIC_SEL_SIZE		(1)
#define VTS_DMIC_RCH_EN_OFFSET		(17)
#define VTS_DMIC_RCH_EN_SIZE		(1)
#define VTS_DMIC_LCH_EN_OFFSET		(16)
#define VTS_DMIC_LCH_EN_SIZE		(1)
#define VTS_DMIC_SYS_SEL_OFFSET		(12)
#define VTS_DMIC_SYS_SEL_SIZE		(2)
#define VTS_DMIC_POLARITY_CLK_OFFSET	(10)
#define VTS_DMIC_POLARITY_CLK_SIZE	(1)
#define VTS_DMIC_POLARITY_OUTPUT_OFFSET	(9)
#define VTS_DMIC_POLARITY_OUTPUT_SIZE	(1)
#define VTS_DMIC_POLARITY_INPUT_OFFSET	(8)
#define VTS_DMIC_POLARITY_INPUT_SIZE	(1)
#define VTS_DMIC_OVFW_CTRL_OFFSET	(4)
#define VTS_DMIC_OVFW_CTRL_SIZE		(1)
#define VTS_DMIC_CIC_SEL_OFFSET		(0)
#define VTS_DMIC_CIC_SEL_SIZE		(1)

/* CM4 */
#define VTS_CM4_R(x)			(0x0010 + (x * 0x4))
#define VTS_CM4_PC			(0x0004)

#if defined(CONFIG_SOC_EXYNOS9820)
#define VTS_IRQ_VTS_ERROR               (0)
#define VTS_IRQ_VTS_BOOT_COMPLETED      (1)
#define VTS_IRQ_VTS_IPC_RECEIVED        (2)
#define VTS_IRQ_VTS_VOICE_TRIGGERED     (3)
#define VTS_IRQ_VTS_PERIOD_ELAPSED      (4)
#define VTS_IRQ_VTS_REC_PERIOD_ELAPSED  (5)
#define VTS_IRQ_VTS_DBGLOG_BUFZERO      (6)
#define VTS_IRQ_VTS_DBGLOG_BUFONE       (7)
#define VTS_IRQ_VTS_AUDIO_DUMP          (8)
#define VTS_IRQ_VTS_LOG_DUMP            (9)
#define VTS_IRQ_COUNT                   (10)

#define VTS_IRQ_AP_IPC_RECEIVED         (16)
#define VTS_IRQ_AP_SET_DRAM_BUFFER      (17)
#define VTS_IRQ_AP_START_RECOGNITION    (18)
#define VTS_IRQ_AP_STOP_RECOGNITION     (19)
#define VTS_IRQ_AP_START_COPY           (20)
#define VTS_IRQ_AP_STOP_COPY            (21)
#define VTS_IRQ_AP_SET_MODE             (22)
#define VTS_IRQ_AP_POWER_DOWN           (23)
#define VTS_IRQ_AP_TARGET_SIZE          (24)
#define VTS_IRQ_AP_SET_REC_BUFFER       (25)
#define VTS_IRQ_AP_START_REC            (26)
#define VTS_IRQ_AP_STOP_REC             (27)
#define VTS_IRQ_AP_RESTART_RECOGNITION  (29)
#define VTS_IRQ_AP_TEST_COMMAND         (31)

#else
#define VTS_IRQ_VTS_ERROR               (16)
#define VTS_IRQ_VTS_BOOT_COMPLETED      (17)
#define VTS_IRQ_VTS_IPC_RECEIVED        (18)
#define VTS_IRQ_VTS_VOICE_TRIGGERED     (19)
#define VTS_IRQ_VTS_PERIOD_ELAPSED      (20)
#define VTS_IRQ_VTS_REC_PERIOD_ELAPSED  (21)
#define VTS_IRQ_VTS_DBGLOG_BUFZERO      (22)
#define VTS_IRQ_VTS_DBGLOG_BUFONE       (23)
#define VTS_IRQ_VTS_AUDIO_DUMP          (24)
#define VTS_IRQ_VTS_LOG_DUMP            (25)
#define VTS_IRQ_COUNT                   (26)

#define VTS_IRQ_AP_IPC_RECEIVED		(0)
#define VTS_IRQ_AP_SET_DRAM_BUFFER	(1)
#define VTS_IRQ_AP_START_RECOGNITION	(2)
#define VTS_IRQ_AP_STOP_RECOGNITION	(3)
#define VTS_IRQ_AP_START_COPY		(4)
#define VTS_IRQ_AP_STOP_COPY		(5)
#define VTS_IRQ_AP_SET_MODE		(6)
#define VTS_IRQ_AP_POWER_DOWN		(7)
#define VTS_IRQ_AP_TARGET_SIZE		(8)
#define VTS_IRQ_AP_SET_REC_BUFFER	(9)
#define VTS_IRQ_AP_START_REC		(10)
#define VTS_IRQ_AP_STOP_REC		(11)
#define VTS_IRQ_AP_RESTART_RECOGNITION	(13)
#define VTS_IRQ_AP_TEST_COMMAND		(15)
#endif

#define VTS_IRQ_LIMIT			(32)

#define VTS_BAAW_BASE			(0x60000000)
#define VTS_BAAW_SRC_START_ADDRESS	(0x10000)
#define VTS_BAAW_SRC_END_ADDRESS	(0x10004)
#define VTS_BAAW_REMAPPED_ADDRESS	(0x10008)
#define VTS_BAAW_INIT_DONE		(0x1000C)

#define BUFFER_BYTES_MAX (0xa0000)
#define PERIOD_BYTES_MIN (SZ_4)
#define PERIOD_BYTES_MAX (BUFFER_BYTES_MAX / 2)

#define SOUND_MODEL_SIZE_MAX (SZ_32K)
#define SOUND_MODEL_COUNT (3)

/* DRAM for copying VTS firmware logs */
#define LOG_BUFFER_BYTES_MAX	(0x2000)
#define VTS_SRAMLOG_MSGS_OFFSET (0x59000)

/* VTS firmware version information offset */
#define VTSFW_VERSION_OFFSET	(0x7c)
#define DETLIB_VERSION_OFFSET	(0x78)

/* svoice net(0x8000) & grammar(0x300) binary sizes defined in firmware */
#define SOUND_MODEL_SVOICE_SIZE_MAX (0x8000 + 0x300)

/* google binary size defined in firmware (It is same with VTSDRV_MISC_MODEL_BIN_MAXSZ) */
#define SOUND_MODEL_GOOGLE_SIZE_MAX (0xB500)

/* VTS Model Binary Max buffer sizes */
#define VTS_MODEL_SVOICE_BIN_MAXSZ     (SOUND_MODEL_SVOICE_SIZE_MAX)
#define VTS_MODEL_GOOGLE_BIN_MAXSZ     (SOUND_MODEL_GOOGLE_SIZE_MAX)

enum ipc_state {
	IDLE,
	SEND_MSG,
	SEND_MSG_OK,
	SEND_MSG_FAIL,
};

enum trigger {
	TRIGGER_NONE	= -1,
	TRIGGER_SVOICE	= 0,
	TRIGGER_GOOGLE	= 1,
	TRIGGER_SENSORY	= 2,
	TRIGGER_COUNT,
};

enum vts_platform_type {
	PLATFORM_VTS_NORMAL_RECORD,
	PLATFORM_VTS_TRIGGER_RECORD,
};

enum vts_dmic_sel {
	DPDM,
	APDM,
};

enum executionmode {
	//default is off
	VTS_OFF_MODE			= 0,
	//voice-trig-mode:Both LPSD & Trigger are enabled
	VTS_VOICE_TRIGGER_MODE		= 1,
	//sound-detect-mode: Low Power sound Detect
	VTS_SOUND_DETECT_MODE		= 2,
	//vt-always-mode: key phrase Detection only(Trigger)
	VTS_VT_ALWAYS_ON_MODE		= 3,
	//google-trigger: key phrase Detection only(Trigger)
	VTS_GOOGLE_TRIGGER_MODE		= 4,
	//sensory-trigger: key phrase Detection only(Trigger)
	VTS_SENSORY_TRIGGER_MODE	= 5,
	//off:voice-trig-mode:Both LPSD & Trigger are enabled
	VTS_VOICE_TRIGGER_MODE_OFF	= 6,
	//off:sound-detect-mode: Low Power sound Detect
	VTS_SOUND_DETECT_MODE_OFF	= 7,
	//off:vt-always-mode: key phrase Detection only(Trigger)
	VTS_VT_ALWAYS_ON_MODE_OFF	= 8,
	//off:google-trigger: key phrase Detection only(Trigger)
	VTS_GOOGLE_TRIGGER_MODE_OFF	= 9,
	//off:sensory-trigger: key phrase Detection only(Trigger)
	VTS_SENSORY_TRIGGER_MODE_OFF	= 10,
	VTS_MODE_COUNT,
};

enum vts_dump_type {
	KERNEL_PANIC_DUMP,
	VTS_FW_NOT_READY,
	VTS_IPC_TRANS_FAIL,
	RUNTIME_SUSPEND_DUMP,
	VTS_DUMP_LAST,
};

enum vts_test_command {
	VTS_DISABLE_LOGDUMP	= 0x01000000,
	VTS_ENABLE_LOGDUMP	= 0x02000000,
	VTS_DISABLE_AUDIODUMP	= 0x04000000,
	VTS_ENABLE_AUDIODUMP	= 0x08000000,
	VTS_DISABLE_DEBUGLOG	= 0x10000000,
	VTS_ENABLE_DEBUGLOG	= 0x20000000,
	VTS_ENABLE_SRAM_LOG	= 0x80000000,
};

struct vts_ipc_msg {
	int msg;
	u32 values[3];
};

enum vts_micconf_type {
	VTS_MICCONF_FOR_RECORD	= 0,
	VTS_MICCONF_FOR_TRIGGER	= 1,
	VTS_MICCONF_FOR_GOOGLE	= 2,
};

enum vts_state_machine {
	VTS_STATE_NONE			= 0,	/* runtime_suspended state */
	VTS_STATE_VOICECALL		= 1,	/* sram L2Cache call state */
	VTS_STATE_RUNTIME_RESUMING	= 2,	/* runtime_resume started */
	VTS_STATE_RUNTIME_RESUMED	= 3,	/* runtime_resume done */
	VTS_STATE_RECOG_STARTED		= 4,	/* Recognization started */
	VTS_STATE_RECOG_TRIGGERED	= 5,	/* Recognize triggered */
	VTS_STATE_SEAMLESS_REC_STARTED	= 6,	/* seamless record started */
	VTS_STATE_SEAMLESS_REC_STOPPED	= 7,	/* seamless record stopped */
	VTS_STATE_RECOG_STOPPED		= 8,	/* Recognization stopped */
	VTS_STATE_RUNTIME_SUSPENDING	= 9,	/* runtime_suspend started */
	VTS_STATE_RUNTIME_SUSPENDED	= 10,	/* runtime_suspend done */
};

struct vts_model_bin_info {
	unsigned char *data;
	size_t	actual_sz;
	size_t	max_sz;
	bool loaded;
};

struct vts_dbg_dump {
	long long time;
	enum vts_dump_type dbg_type;
	unsigned int gpr[17];
	char *sram_log;
	char *sram_fw;
};

struct vts_log_buffer {
	char *addr;
	unsigned int size;
};

struct vts_data {
	struct platform_device *pdev;
	struct snd_soc_component *cmpnt;
	void __iomem *sfr_base;
	void __iomem *baaw_base;
	void __iomem *sram_base;
	void __iomem *dmic_base;
#if defined(CONFIG_SOC_EXYNOS9820)
	void __iomem *dmic_3rd_base;
#endif
	void __iomem *gpr_base;
	size_t sram_size;
	struct regmap *regmap_dmic;
	struct clk *clk_rco;
	struct clk *clk_dmic;
	struct clk *clk_dmic_if;
	struct clk *clk_dmic_sync;
	struct pinctrl *pinctrl;
	unsigned int vtsfw_version;
	unsigned int vtsdetectlib_version;
	const struct firmware *firmware;
	unsigned int vtsdma_count;
	unsigned long syssel_rate;
	struct platform_device *pdev_mailbox;
	struct platform_device *pdev_vtsdma[2];
	struct proc_dir_entry *proc_dir_entry;
	int irq[VTS_IRQ_LIMIT];
	volatile enum ipc_state ipc_state_ap;
	wait_queue_head_t ipc_wait_queue;
	spinlock_t ipc_spinlock;
	struct mutex ipc_mutex;
	u32 dma_area_vts;
	struct snd_dma_buffer dmab;
	struct snd_dma_buffer dmab_rec;
	struct snd_dma_buffer dmab_log;
	struct snd_dma_buffer dmab_model;
	u32 target_size;
	volatile enum trigger active_trigger;
	u32 voicerecog_start;
	enum executionmode exec_mode;
	bool vts_ready;
	volatile unsigned long sram_acquired;
	volatile bool enabled;
	volatile bool running;
	bool voicecall_enabled;
	struct snd_soc_card *card;
	int micclk_init_cnt;
	unsigned int mic_ready;
	enum vts_dmic_sel dmic_if;
	bool irq_state;
	u32 lpsdgain;
	u32 dmicgain;
	u32 amicgain;
	char *sramlog_baseaddr;
	u32 running_ipc;
	struct wake_lock wake_lock;
	unsigned int vts_state;
	u32 vtslog_enabled;
	bool audiodump_enabled;
	bool logdump_enabled;
	struct vts_model_bin_info svoice_info;
	struct vts_model_bin_info google_info;
	spinlock_t state_spinlock;
	struct vts_dbg_dump p_dump[VTS_DUMP_LAST];
};

struct vts_platform_data {
	unsigned int id;
	struct platform_device *pdev_vts;
	struct vts_data *vts_data;
	struct snd_pcm_substream *substream;
	enum vts_platform_type type;
	volatile unsigned int pointer;
};

extern int vts_start_ipc_transaction(struct device *dev, struct vts_data *data,
		int msg, u32 (*values)[3], int atomic, int sync);

extern int vts_send_ipc_ack(struct vts_data *data, u32 result);
extern void vts_register_dma(struct platform_device *pdev_vts,
		struct platform_device *pdev_vts_dma, unsigned int id);
extern int vts_set_dmicctrl(struct platform_device *pdev, int micconf_type, bool enable);
extern int vts_sound_machine_drv_register(void);
#endif /* __SND_SOC_VTS_H */
