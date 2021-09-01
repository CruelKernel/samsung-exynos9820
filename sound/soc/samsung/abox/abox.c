/* sound/soc/samsung/abox/abox.c
 *
 * ALSA SoC Audio Layer - Samsung Abox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/* #define DEBUG */
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pm_runtime.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/exynos_iovmm.h>
#include <linux/workqueue.h>
#include <linux/smc.h>
#include <linux/delay.h>
#include <linux/suspend.h>
#include <linux/sched/clock.h>
#include <linux/shm_ipc.h>
#include <linux/modem_notifier.h>

#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/samsung/abox.h>
#include <sound/samsung/vts.h>

#include <soc/samsung/exynos-pmu.h>
#ifdef CONFIG_EXYNOS_ITMON
#include <soc/samsung/exynos-itmon.h>
#endif
#include "../../../../drivers/iommu/exynos-iommu.h"

#include "abox_util.h"
#include "abox_dbg.h"
#include "abox_log.h"
#include "abox_dump.h"
#include "abox_gic.h"
#include "abox_failsafe.h"
#include "abox_if.h"
#include "abox_vdma.h"
#include "abox_effect.h"
#include "abox_cmpnt.h"
#include "abox_core.h"
#include "abox_ipc.h"
#include "abox.h"

#undef EMULATOR
#ifdef EMULATOR
static void __iomem *pmu_alive;
static void update_mask_value(void __iomem *sfr,
		unsigned int mask, unsigned int value)
{
	unsigned int sfr_value = readl(sfr);

	set_mask_value(sfr_value, mask, value);
	writel(sfr_value, sfr);
}
#endif

#ifdef CONFIG_SOC_EXYNOS8895
#define ABOX_PAD_NORMAL				(0x3048)
#define ABOX_PAD_NORMAL_MASK			(0x10000000)
#elif defined(CONFIG_SOC_EXYNOS9810)
#define ABOX_PAD_NORMAL				(0x4170)
#define ABOX_PAD_NORMAL_MASK			(0x10000000)
#elif defined(CONFIG_SOC_EXYNOS9820)
#define ABOX_PAD_NORMAL				(0x1920)
#define ABOX_PAD_NORMAL_MASK			(0x00000800)
#endif

#define DEFAULT_CPU_GEAR_ID		(0xAB0CDEFA)
#define TEST_CPU_GEAR_ID		(DEFAULT_CPU_GEAR_ID + 1)
#define CALLIOPE_ENABLE_TIMEOUT_MS	(1000)
#define IPC_TIMEOUT_US			(10000)
#define BOOT_DONE_TIMEOUT_MS		(10000)
#define DRAM_TIMEOUT_NS			(10000000)
#define IPC_RETRY			(10)

#define ERAP(wname, wcontrols, event_fn, wparams) \
{	.id = snd_soc_dapm_dai_link, .name = wname, \
	.reg = SND_SOC_NOPM, .event = event_fn, \
	.kcontrol_news = wcontrols, .num_kcontrols = 1, \
	.params = wparams, .num_params = ARRAY_SIZE(wparams), \
	.event_flags = SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_WILL_PMD }

#ifdef CONFIG_EXYNOS_ITMON
void __weak itmon_notifier_chain_register(struct notifier_block *n)
{}
#endif

/* For only external static functions */
static struct abox_data *p_abox_data;

struct abox_data *abox_get_abox_data(void)
{
	return p_abox_data;
}

static int abox_iommu_fault_handler(
		struct iommu_domain *domain, struct device *dev,
		unsigned long fault_addr, int fault_flags, void *token)
{
	struct abox_data *data = token;

	abox_dbg_print_gpr(&data->pdev->dev, data);

	return 0;
}

static void abox_cpu_power(bool on);
static int abox_cpu_enable(bool enable);
static int abox_cpu_pm_ipc(struct abox_data *data, bool resume);
static void abox_boot_done(struct device *dev, unsigned int version);

static void exynos_abox_panic_handler(void)
{
	static bool has_run;
	struct abox_data *data = p_abox_data;
	struct device *dev = data ? (data->pdev ? &data->pdev->dev : NULL) :
			NULL;

	dev_dbg(dev, "%s\n", __func__);

	if (abox_is_on() && dev) {
		if (has_run) {
			dev_info(dev, "already dumped\n");
			return;
		}
		has_run = true;

		abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_KERNEL, "panic");
		writel(0x504E4943, data->sram_base + data->sram_size -
				sizeof(u32));
		abox_cpu_enable(false);
		abox_cpu_power(false);
		abox_cpu_power(true);
		abox_cpu_enable(true);
		mdelay(100);
		abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_KERNEL, "panic");
	} else {
		dev_info(dev, "%s: dump is skipped due to no power\n",
				__func__);
	}
}

static int abox_panic_handler(struct notifier_block *nb,
			       unsigned long action, void *data)
{
	exynos_abox_panic_handler();
	return NOTIFY_OK;
}

static struct notifier_block abox_panic_notifier = {
	.notifier_call	= abox_panic_handler,
	.next		= NULL,
	.priority	= 0	/* priority: INT_MAX >= x >= 0 */
};

static void abox_wdt_work_func(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct abox_data *data = container_of(dwork, struct abox_data,
			wdt_work);
	struct device *dev_abox = &data->pdev->dev;

	abox_dbg_print_gpr(dev_abox, data);
	abox_dbg_dump_mem(dev_abox, data, ABOX_DBG_DUMP_KERNEL, "watchdog");
	abox_failsafe_report(dev_abox);
}

static DECLARE_DELAYED_WORK(abox_wdt_work, abox_wdt_work_func);

static irqreturn_t abox_wdt_handler(int irq, void *dev_id)
{
	struct abox_data *data = dev_id;
	struct device *dev = &data->pdev->dev;

	dev_err(dev, "abox watchdog timeout\n");

	if (abox_is_on()) {
		abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_KERNEL, "watchdog");
		writel(0x504E4943, data->sram_base + data->sram_size -
				sizeof(u32));
		abox_cpu_enable(false);
		abox_cpu_power(false);
		abox_cpu_power(true);
		abox_cpu_enable(true);
		schedule_delayed_work(&data->wdt_work, msecs_to_jiffies(100));
	} else {
		dev_info(dev, "%s: no power\n", __func__);
	}

	return IRQ_HANDLED;
}

void abox_enable_wdt(struct abox_data *data)
{
	abox_gic_target_ap(data->dev_gic, IRQ_WDT);
	abox_gic_enable(data->dev_gic, IRQ_WDT, true);
}

static struct platform_driver samsung_abox_driver;

bool is_abox(struct device *dev)
{
	return (&samsung_abox_driver.driver) == dev->driver;
}

static void abox_probe_quirks(struct abox_data *data, struct device_node *np)
{
	#define QUIRKS "samsung,quirks"
	#define DEC_MAP(id) {ABOX_QUIRK_STR_##id, ABOX_QUIRK_BIT_##id}

	static const struct {
		const char *str;
		unsigned int bit;
	} map[] = {
		DEC_MAP(TRY_TO_ASRC_OFF),
		DEC_MAP(SHARE_VTS_SRAM),
	};

	int i, ret;

	for (i = 0; i < ARRAY_SIZE(map); i++) {
		ret = of_property_match_string(np, QUIRKS, map[i].str);
		if (ret >= 0)
			data->quirks |= map[i].bit;
	}
}

int abox_disable_qchannel(struct device *dev, struct abox_data *data,
		enum qchannel clk, int disable)
{
	return regmap_update_bits(data->regmap, ABOX_QCHANNEL_DISABLE,
			ABOX_QCHANNEL_DISABLE_MASK(clk),
			!!disable << ABOX_QCHANNEL_DISABLE_L(clk));
}

phys_addr_t abox_addr_to_phys_addr(struct abox_data *data, unsigned int addr)
{
	phys_addr_t ret;

	/* 0 is translated to 0 for convenience. */
	if (addr == 0)
		ret = 0;
	else if (addr < IOVA_DRAM_FIRMWARE)
		ret = data->sram_base_phys + addr;
	else
		ret = iommu_iova_to_phys(data->iommu_domain, addr);

	return ret;
}

static void *abox_dram_addr_to_kernel_addr(struct abox_data *data,
		unsigned long iova)
{
	struct abox_iommu_mapping *m;
	unsigned long flags;
	void *ret = 0;

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_for_each_entry(m, &data->iommu_maps, list) {
		if (m->iova <= iova && iova < m->iova + m->bytes) {
			ret = m->area + (iova - m->iova);
			break;
		}
	}
	spin_unlock_irqrestore(&data->iommu_lock, flags);

	return ret;
}

void *abox_addr_to_kernel_addr(struct abox_data *data, unsigned int addr)
{
	void *ret;

	/* 0 is translated to 0 for convenience. */
	if (addr == 0)
		ret = 0;
	else if (addr < IOVA_DRAM_FIRMWARE)
		ret = data->sram_base + addr;
	else
		ret = abox_dram_addr_to_kernel_addr(data, addr);

	return ret;
}

phys_addr_t abox_iova_to_phys(struct device *dev, unsigned long iova)
{
	return abox_addr_to_phys_addr(dev_get_drvdata(dev), iova);
}
EXPORT_SYMBOL(abox_iova_to_phys);

void *abox_iova_to_virt(struct device *dev, unsigned long iova)
{
	return abox_addr_to_kernel_addr(dev_get_drvdata(dev), iova);
}
EXPORT_SYMBOL(abox_iova_to_virt);

int abox_show_gpr_min(char *buf, int len)
{
	return abox_core_show_gpr_min(buf, len);
}
EXPORT_SYMBOL(abox_show_gpr_min);

u32 abox_read_gpr(int core_id, int gpr_id)
{
	return abox_core_read_gpr(core_id, gpr_id);
}
EXPORT_SYMBOL(abox_read_gpr);

static int abox_of_get_addr(struct abox_data *data, struct device_node *np,
		const char *name, void **addr, size_t *size)
{
	struct device *dev = &data->pdev->dev;
	unsigned int area[3];
	int ret;

	dev_dbg(dev, "%s(%s)\n", __func__, name);

	ret = of_property_read_u32_array(np, name, area, ARRAY_SIZE(area));
	if (ret < 0) {
		dev_warn(dev, "Failed to read %s: %d\n", name, ret);
		goto out;
	}

	switch (area[0]) {
	case 0:
		*addr = data->sram_base;
		break;
	default:
		dev_warn(dev, "%s: invalid area: %d\n", __func__, area[0]);
		/* fall through */
	case 1:
		*addr = data->dram_base;
		break;
	case 2:
		*addr = shm_get_vss_region();
		break;
	}
	*addr += area[1];
	if (size)
		*size = area[2];
out:
	return ret;
}

static bool __abox_ipc_queue_empty(struct abox_data *data)
{
	return (data->ipc_queue_end == data->ipc_queue_start);
}

static bool __abox_ipc_queue_full(struct abox_data *data)
{
	size_t length = ARRAY_SIZE(data->ipc_queue);

	return (((data->ipc_queue_end + 1) % length) == data->ipc_queue_start);
}

static int abox_ipc_queue_put(struct abox_data *data, struct device *dev,
		int hw_irq, const void *supplement, size_t size)
{
	spinlock_t *lock = &data->ipc_queue_lock;
	size_t length = ARRAY_SIZE(data->ipc_queue);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(lock, flags);
	if (!__abox_ipc_queue_full(data)) {
		struct abox_ipc *ipc;

		ipc = &data->ipc_queue[data->ipc_queue_end];
		ipc->dev = dev;
		ipc->hw_irq = hw_irq;
		ipc->put_time = sched_clock();
		ipc->get_time = 0;
		memcpy(&ipc->msg, supplement, size);
		ipc->size = size;
		data->ipc_queue_end = (data->ipc_queue_end + 1) % length;

		ret = 0;
	} else {
		ret = -EBUSY;
	}
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

static int abox_ipc_queue_get(struct abox_data *data, struct abox_ipc *ipc)
{
	spinlock_t *lock = &data->ipc_queue_lock;
	size_t length = ARRAY_SIZE(data->ipc_queue);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(lock, flags);
	if (!__abox_ipc_queue_empty(data)) {
		struct abox_ipc *tmp;

		tmp = &data->ipc_queue[data->ipc_queue_start];
		tmp->get_time = sched_clock();
		*ipc = *tmp;
		data->ipc_queue_start = (data->ipc_queue_start + 1) % length;

		ret = 0;
	} else {
		ret = -ENODATA;
	}
	spin_unlock_irqrestore(lock, flags);

	return ret;
}

static bool abox_can_calliope_ipc(struct device *dev,
		struct abox_data *data)
{
	bool ret = true;

	switch (data->calliope_state) {
	case CALLIOPE_DISABLING:
	case CALLIOPE_ENABLED:
		break;
	case CALLIOPE_ENABLING:
		wait_event_timeout(data->ipc_wait_queue,
				data->calliope_state == CALLIOPE_ENABLED,
				msecs_to_jiffies(CALLIOPE_ENABLE_TIMEOUT_MS));
		if (data->calliope_state == CALLIOPE_ENABLED)
			break;
		/* Fallthrough */
	case CALLIOPE_DISABLED:
	default:
		dev_warn(dev, "Invalid calliope state: %d\n",
				data->calliope_state);
		ret = false;
	}

	dev_dbg(dev, "%s: %d\n", __func__, ret);

	return ret;
}

static int __abox_process_ipc_legacy(struct device *dev, struct abox_data *data,
		int hw_irq, const ABOX_IPC_MSG *msg)
{
	struct device *dev_abox = &data->pdev->dev;
	static unsigned int tout_cnt;
	static DEFINE_SPINLOCK(lock);

	void __iomem *tx_base = data->sram_base + data->ipc_tx_offset;
	void __iomem *tx_ack = data->sram_base + data->ipc_tx_ack_offset;
	int ret = 0, i;

	dev_dbg(dev, "%s(%d, %d, %d)\n", __func__, hw_irq,
			msg->ipcid, msg->msg.system.msgtype);

	do {
		spin_lock(&lock);

		memcpy_toio(tx_base, msg, sizeof(*msg));
		writel(1, tx_ack);
		abox_gic_generate_interrupt(data->dev_gic, hw_irq);
		for (i = IPC_TIMEOUT_US; i && readl(tx_ack); i--)
			udelay(1);

		if (readl(tx_ack)) {
			tout_cnt++;
			dev_warn_ratelimited(dev, "Transaction timeout(%d)\n",
					tout_cnt);

			if (data->failsafe) {
				writel(0, tx_ack);
				goto unlock;
			}

			if (tout_cnt == 1)
				abox_dbg_dump_simple(dev_abox, data,
						"Transaction timeout");

			if ((tout_cnt % IPC_RETRY) == 0) {
				abox_failsafe_report(dev_abox);
				writel(0, tx_ack);
			}

			ret = -EIO;
		} else {
			tout_cnt = 0;
			ret = 0;
		}
unlock:
		spin_unlock(&lock);
	} while (readl(tx_ack));

	return ret;
}

static int __abox_process_ipc(struct device *dev, struct abox_data *data,
		int hw_irq, const ABOX_IPC_MSG *msg, size_t size)
{
	int ret;

	switch (data->ipc_version) {
	case 1:
		ret = __abox_process_ipc_legacy(dev, data, hw_irq, msg);
		break;
	default:
		ret = abox_ipc_send(dev, msg, size, NULL, 0);
		if (ret < 0)
			break;

		ret = abox_ipc_flush(dev);
		if (ret < 0) {
			abox_ipc_retry();
			ret = abox_ipc_flush(dev);
		}
		break;
	}

	return ret;
}

static void abox_process_ipc(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data, ipc_work);
	struct device *dev = &data->pdev->dev;
	struct abox_ipc ipc;
	int ret;

	dev_dbg(dev, "%s: %d %d\n", __func__, data->ipc_queue_start,
			data->ipc_queue_end);

	pm_runtime_get_sync(dev);

	if (abox_can_calliope_ipc(dev, data)) {
		while (abox_ipc_queue_get(data, &ipc) == 0) {
			struct device *dev = ipc.dev;
			int hw_irq = ipc.hw_irq;
			ABOX_IPC_MSG *msg = &ipc.msg;
			size_t size = ipc.size;

			ret = __abox_process_ipc(dev, data, hw_irq, msg, size);
			if (ret < 0)
				abox_failsafe_report(dev);

			/* giving time to ABOX for processing */
			usleep_range(10, 100);
		}
	}

	pm_runtime_mark_last_busy(dev);
	pm_runtime_put_autosuspend(dev);
}

static int abox_schedule_ipc(struct device *dev, struct abox_data *data,
		int hw_irq, const void *supplement, size_t size,
		bool atomic, bool sync)
{
	struct abox_ipc *ipc;
	int retry = 0;
	int ret;

	dev_dbg(dev, "%s(%d, %zu, %d, %d)\n", __func__, hw_irq,
			size, atomic, sync);

	if (unlikely(sizeof(ipc->msg) < size)) {
		dev_err(dev, "%s: too large supplement\n", __func__);
		return -EINVAL;
	}

	do {
		ret = abox_ipc_queue_put(data, dev, hw_irq, supplement, size);
		queue_work(data->ipc_workqueue, &data->ipc_work);
		if (!atomic && sync)
			flush_work(&data->ipc_work);
		if (ret >= 0)
			break;

		if (!atomic) {
			dev_info(dev, "%s: flush(%d)\n", __func__, retry);
			flush_work(&data->ipc_work);
		} else {
			dev_info(dev, "%s: delay(%d)\n", __func__, retry);
			mdelay(10);
		}
	} while (retry++ < IPC_RETRY);

	if (ret < 0) {
		dev_err(dev, "%s(%d): ipc queue overflow\n", __func__, hw_irq);
		abox_failsafe_report(dev);
	}

	return ret;
}

int abox_request_ipc(struct device *dev,
		int hw_irq, const void *supplement,
		size_t size, int atomic, int sync)
{
	struct abox_data *data = dev_get_drvdata(dev);
	int ret;

	if (atomic && sync) {
		ret = __abox_process_ipc(dev, data, hw_irq, supplement, size);
	} else {
		ret = abox_schedule_ipc(dev, data, hw_irq, supplement, size,
				!!atomic, !!sync);
	}

	return ret;
}
EXPORT_SYMBOL(abox_request_ipc);

bool abox_is_on(void)
{
	return p_abox_data && p_abox_data->enabled;
}
EXPORT_SYMBOL(abox_is_on);

static int abox_correct_pll_rate(struct device *dev,
		long long src_rate, int diff_ppb)
{
	const int unit_ppb = 1000000000;
	long long correction;

	correction = src_rate * (diff_ppb + unit_ppb);
	do_div(correction, unit_ppb);
	dev_dbg(dev, "correct AUD_PLL %dppb: %lldHz -> %lldHz\n",
			diff_ppb, src_rate, correction);

	return (unsigned long)correction;
}

int abox_register_bclk_usage(struct device *dev, struct abox_data *data,
		enum abox_dai dai_id, unsigned int rate, unsigned int channels,
		unsigned int width)
{
	static int correction;
	unsigned long target_pll, audif_rate;
	int id = dai_id - ABOX_UAIF0;
	int ret = 0;
	int i;

	dev_dbg(dev, "%s(%d, %d)\n", __func__, id, rate);

	if (id < 0 || id >= ABOX_DAI_COUNT) {
		dev_err(dev, "invalid dai_id: %d\n", dai_id);
		return -EINVAL;
	}

	if (rate == 0) {
		data->audif_rates[id] = 0;
		return 0;
	}

	target_pll = ((rate % 44100) == 0) ? AUD_PLL_RATE_HZ_FOR_44100 :
			AUD_PLL_RATE_HZ_FOR_48000;

	if (data->clk_diff_ppb) {
		/* run only when correction value is changed */
		if (correction != data->clk_diff_ppb) {
			target_pll = abox_correct_pll_rate(dev, target_pll,
					data->clk_diff_ppb);
			correction = data->clk_diff_ppb;
		} else {
			target_pll = clk_get_rate(data->clk_pll);
		}
	}

	if (target_pll != clk_get_rate(data->clk_pll)) {
		dev_info(dev, "Set AUD_PLL rate: %lu -> %lu\n",
			clk_get_rate(data->clk_pll), target_pll);
		ret = clk_set_rate(data->clk_pll, target_pll);
		if (ret < 0) {
			dev_err(dev, "AUD_PLL set error=%d\n", ret);
			return ret;
		}
	}

	if (data->uaif_max_div <= 32) {
		if ((rate % 44100) == 0)
			audif_rate = ((rate > 176400) ? 352800 : 176400) *
					width * 2;
		else
			audif_rate = ((rate > 192000) ? 384000 : 192000) *
					width * 2;

		while (audif_rate / rate / channels / width >
				data->uaif_max_div)
			audif_rate /= 2;
	} else {
		int clk_width = 96; /* LCM of 24 and 32 */
		int clk_channels = 2;

		if ((rate % 44100) == 0)
			audif_rate = 352800 * clk_width * clk_channels;
		else
			audif_rate = 384000 * clk_width * clk_channels;

		if (audif_rate < rate * width * channels)
			audif_rate = rate * width * channels;
	}

	data->audif_rates[id] = audif_rate;

	for (i = 0; i < ARRAY_SIZE(data->audif_rates); i++) {
		if (data->audif_rates[i] > 0 &&
				data->audif_rates[i] > audif_rate) {
			audif_rate = data->audif_rates[i];
		}
	}

	ret = clk_set_rate(data->clk_audif, audif_rate);
	if (ret < 0)
		dev_err(dev, "Failed to set audif clock: %d\n", ret);

	dev_info(dev, "audif clock: %lu\n", clk_get_rate(data->clk_audif));

	return ret;
}

static bool abox_timer_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_TIMER_CTRL0(0):
	case ABOX_TIMER_CTRL1(0):
	case ABOX_TIMER_PRESET_LSB(0):
	case ABOX_TIMER_CTRL0(1):
	case ABOX_TIMER_CTRL1(1):
	case ABOX_TIMER_PRESET_LSB(1):
	case ABOX_TIMER_CTRL0(2):
	case ABOX_TIMER_CTRL1(2):
	case ABOX_TIMER_PRESET_LSB(2):
	case ABOX_TIMER_CTRL0(3):
	case ABOX_TIMER_CTRL1(3):
	case ABOX_TIMER_PRESET_LSB(3):
		return true;
	default:
		return false;
	}
}

static bool abox_timer_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_TIMER_CTRL0(0):
	case ABOX_TIMER_CTRL1(0):
	case ABOX_TIMER_PRESET_LSB(0):
	case ABOX_TIMER_CTRL0(1):
	case ABOX_TIMER_CTRL1(1):
	case ABOX_TIMER_PRESET_LSB(1):
	case ABOX_TIMER_CTRL0(2):
	case ABOX_TIMER_CTRL1(2):
	case ABOX_TIMER_PRESET_LSB(2):
	case ABOX_TIMER_CTRL0(3):
	case ABOX_TIMER_CTRL1(3):
	case ABOX_TIMER_PRESET_LSB(3):
		return true;
	default:
		return false;
	}
}

static bool abox_timer_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case ABOX_TIMER_CTRL0(0):
	case ABOX_TIMER_CTRL1(0):
	case ABOX_TIMER_CTRL0(1):
	case ABOX_TIMER_CTRL1(1):
	case ABOX_TIMER_CTRL0(2):
	case ABOX_TIMER_CTRL1(2):
	case ABOX_TIMER_CTRL0(3):
	case ABOX_TIMER_CTRL1(3):
		return true;
	default:
		return false;
	}
}

static struct regmap_config abox_timer_regmap_config = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.max_register = ABOX_MAX_REGISTERS,
	.volatile_reg = abox_timer_volatile_reg,
	.readable_reg = abox_timer_readable_reg,
	.writeable_reg = abox_timer_writeable_reg,
	.cache_type = REGCACHE_RBTREE,
	.fast_io = true,
};

int abox_hw_params_fixup_helper(struct snd_soc_pcm_runtime *rtd,
		struct snd_pcm_hw_params *params, int stream)
{
	return abox_cmpnt_hw_params_fixup_helper(rtd, params, stream);
}
EXPORT_SYMBOL(abox_hw_params_fixup_helper);

unsigned int abox_get_requiring_int_freq_in_khz(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return abox_qos_get_target(&data->pdev->dev, ABOX_QOS_INT);
}
EXPORT_SYMBOL(abox_get_requiring_int_freq_in_khz);

unsigned int abox_get_requiring_aud_freq_in_khz(void)
{
	struct abox_data *data = p_abox_data;

	if (data == NULL)
		return 0;

	return abox_qos_get_target(&data->pdev->dev, ABOX_QOS_AUD);
}
EXPORT_SYMBOL(abox_get_requiring_aud_freq_in_khz);

bool abox_cpu_gear_idle(struct device *dev, unsigned int id)
{
	return !abox_qos_is_active(dev, ABOX_QOS_AUD, id);
}

static bool abox_is_clearable(struct device *dev, struct abox_data *data)
{
	return abox_cpu_gear_idle(dev, ABOX_CPU_GEAR_ABSOLUTE) &&
			data->audio_mode != MODE_IN_CALL;
}

static void abox_check_cpu_request(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int old_val, unsigned int val)
{
	struct device *dev_abox = &data->pdev->dev;

	if (id != ABOX_CPU_GEAR_BOOT)
		return;

	if (data->calliope_state == CALLIOPE_ENABLING)
		abox_boot_done(dev_abox, data->calliope_version);

	if (!old_val && val) {
		dev_dbg(dev, "%s(%#x): on\n", __func__, id);
		pm_wakeup_event(dev_abox, BOOT_DONE_TIMEOUT_MS);
	} else if (old_val && !val) {
		dev_dbg(dev, "%s(%#x): off\n", __func__, id);
		pm_relax(dev_abox);
	}
}

static void abox_notify_cpu_gear(struct abox_data *data, unsigned int freq)
{
	struct device *dev = &data->pdev->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	unsigned long long time = sched_clock();
	unsigned long rem = do_div(time, NSEC_PER_SEC);

	switch (data->calliope_state) {
	case CALLIOPE_ENABLING:
	case CALLIOPE_ENABLED:
		dev_dbg(dev, "%s\n", __func__);

		msg.ipcid = IPC_SYSTEM;
		system_msg->msgtype = ABOX_CHANGED_GEAR;
		system_msg->param1 = (int)freq;
		system_msg->param2 = (int)time; /* SEC */
		system_msg->param3 = (int)rem; /* NSEC */
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
		break;
	case CALLIOPE_DISABLING:
	case CALLIOPE_DISABLED:
	default:
		/* notification to passing by context is not needed */
		break;
	}
}

int abox_request_cpu_gear(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int level, const char *name)
{
	unsigned int old_val, val;
	int ret;

	if (level < 1)
		val = 0;
	else if (level <= ARRAY_SIZE(data->pm_qos_aud))
		val = data->pm_qos_aud[level - 1];
	else if (level <= ABOX_CPU_GEAR_MIN)
		val = 0;
	else
		val = level;

	old_val = abox_qos_get_value(dev, ABOX_QOS_AUD, id);
	ret = abox_qos_request_aud(dev, id, val, name);
	abox_check_cpu_request(dev, data, id, old_val, val);

	return ret;
}

void abox_cpu_gear_barrier(void)
{
	abox_qos_complete();
}

int abox_request_cpu_gear_sync(struct device *dev, struct abox_data *data,
		unsigned int id, unsigned int level, const char *name)
{
	int ret = abox_request_cpu_gear(dev, data, id, level, name);

	abox_cpu_gear_barrier();
	return ret;
}

void abox_clear_cpu_gear_requests(struct device *dev)
{
	abox_qos_clear(dev, ABOX_QOS_AUD);
}

static int abox_check_dram_status(struct device *dev, struct abox_data *data,
		bool on)
{
	struct regmap *regmap = data->regmap;
	u64 timeout = local_clock() + DRAM_TIMEOUT_NS;
	unsigned int val = 0;
	int ret;

	dev_dbg(dev, "%s(%d)\n", __func__, on);

	do {
		ret = regmap_read(regmap, ABOX_SYSPOWER_STATUS, &val);
		val &= ABOX_SYSPOWER_STATUS_MASK;
		if (local_clock() > timeout) {
			dev_warn(dev, "syspower status timeout: %u\n", val);
			abox_dbg_dump_simple(dev, data,
					"syspower status timeout");
			ret = -EPERM;
			break;
		}
	} while ((ret < 0) || ((!!val) != on));

	return ret;
}

void abox_request_dram_on(struct device *dev, unsigned int id, bool on)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->regmap;
	struct abox_dram_request *request;
	size_t len = ARRAY_SIZE(data->dram_requests);
	unsigned int val = 0;
	int ret;

	dev_dbg(dev, "%s(%#x, %d)\n", __func__, id, on);

	for (request = data->dram_requests; request - data->dram_requests <
			len && request->id && request->id != id; request++) {
	}

	if (request - data->dram_requests >= len) {
		dev_err(dev, "too many dram requests: %#x, %d\n", id, on);
		return;
	}

	request->on = on;
	request->id = id;
	request->updated = local_clock();

	for (request = data->dram_requests; request - data->dram_requests <
			len && request->id; request++) {
		if (request->on) {
			val = ABOX_SYSPOWER_CTRL_MASK;
			break;
		}
	}

	ret = regmap_write(regmap, ABOX_SYSPOWER_CTRL, val);
	if (ret < 0) {
		dev_err(dev, "syspower write failed\n", ret);
		return;
	}

	abox_check_dram_status(dev, data, on);
}
EXPORT_SYMBOL(abox_request_dram_on);

int abox_iommu_map(struct device *dev, unsigned long iova,
		phys_addr_t addr, size_t bytes, void *area)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_iommu_mapping *mapping;
	unsigned long flags;
	int ret;

	dev_dbg(dev, "%s(%#lx, %#zx)\n", __func__, iova, bytes);

	if (!area) {
		dev_warn(dev, "%s(%#lx): no virtual address\n", __func__, iova);
		area = phys_to_virt(addr);
	}

	mapping = devm_kmalloc(dev, sizeof(*mapping), GFP_KERNEL);
	if (!mapping) {
		ret = -ENOMEM;
		goto out;
	}

	ret = iommu_map(data->iommu_domain, iova, addr, bytes, 0);
	if (ret < 0) {
		devm_kfree(dev, mapping);
		dev_err(dev, "iommu_map(%#lx) fail: %d\n", iova, ret);
		goto out;
	}

	mapping->iova = iova;
	mapping->addr = addr;
	mapping->area = area;
	mapping->bytes = bytes;

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_add(&mapping->list, &data->iommu_maps);
	spin_unlock_irqrestore(&data->iommu_lock, flags);
out:
	return ret;
}
EXPORT_SYMBOL(abox_iommu_map);

int abox_iommu_map_sg(struct device *dev, unsigned long iova,
		struct scatterlist *sg, unsigned int nents,
		int prot, size_t bytes, void *area)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_iommu_mapping *mapping;
	unsigned long flags;
	int ret;

	dev_dbg(dev, "%s(%#lx, %#zx)\n", __func__, iova, bytes);

	if (!area)
		dev_warn(dev, "%s(%#lx): no virtual address\n", __func__, iova);

	mapping = devm_kmalloc(dev, sizeof(*mapping), GFP_KERNEL);
	if (!mapping) {
		ret = -ENOMEM;
		goto out;
	}

	ret = iommu_map_sg(data->iommu_domain, iova, sg, nents, prot);
	if (ret < 0) {
		devm_kfree(dev, mapping);
		dev_err(dev, "iommu_map_sg(%#lx) fail: %d\n", iova, ret);
		goto out;
	}

	mapping->iova = iova;
	mapping->addr = 0;
	mapping->area = area;
	mapping->bytes = bytes;

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_add(&mapping->list, &data->iommu_maps);
	spin_unlock_irqrestore(&data->iommu_lock, flags);
out:
	return ret;
}
EXPORT_SYMBOL(abox_iommu_map_sg);

int abox_iommu_unmap(struct device *dev, unsigned long iova)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct iommu_domain *domain = data->iommu_domain;
	struct abox_iommu_mapping *mapping;
	unsigned long flags;
	int ret = 0;

	dev_dbg(dev, "%s(%#lx)\n", __func__, iova);

	spin_lock_irqsave(&data->iommu_lock, flags);
	list_for_each_entry(mapping, &data->iommu_maps, list) {
		if (iova != mapping->iova)
			continue;

		list_del(&mapping->list);

		ret = iommu_unmap(domain, iova, mapping->bytes);
		if (ret < 0)
			dev_err(dev, "iommu_unmap(%#lx) fail: %d\n", iova, ret);

		exynos_sysmmu_tlb_invalidate(domain, iova, mapping->bytes);
		devm_kfree(dev, mapping);
		break;
	}
	spin_unlock_irqrestore(&data->iommu_lock, flags);

	return ret;
}
EXPORT_SYMBOL(abox_iommu_unmap);

int abox_register_ipc_handler(struct device *dev, int ipc_id,
		abox_ipc_handler_t ipc_handler, void *dev_id)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_ipc_action *action = NULL;
	bool new_handler = true;

	if (ipc_id >= IPC_ID_COUNT)
		return -EINVAL;

	list_for_each_entry(action, &data->ipc_actions, list) {
		if (action->ipc_id == ipc_id && action->data == dev_id) {
			new_handler = false;
			break;
		}
	}

	if (new_handler) {
		action = devm_kzalloc(dev, sizeof(*action), GFP_KERNEL);
		if (IS_ERR_OR_NULL(action)) {
			dev_err(dev, "%s: kmalloc fail\n", __func__);
			return -ENOMEM;
		}
		action->dev = dev;
		action->ipc_id = ipc_id;
		action->data = dev_id;
		list_add_tail(&action->list, &data->ipc_actions);
	}

	action->handler = ipc_handler;

	return 0;
}
EXPORT_SYMBOL(abox_register_ipc_handler);

static int abox_component_control_info(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_info *uinfo)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	struct ABOX_COMPONENT_CONTROL *control = value->control;
	int i;
	const char *c;

	dev_dbg(dev, "%s(%s)\n", __func__, kcontrol->id.name);

	switch (control->type) {
	case ABOX_CONTROL_INT:
		uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
		uinfo->count = control->count;
		uinfo->value.integer.min = control->min;
		uinfo->value.integer.max = control->max;
		break;
	case ABOX_CONTROL_ENUM:
		uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
		uinfo->count = control->count;
		uinfo->value.enumerated.items = control->max;
		if (uinfo->value.enumerated.item >= control->max)
			uinfo->value.enumerated.item = control->max - 1;
		for (c = control->texts.addr, i = 0; i <
				uinfo->value.enumerated.item; ++i, ++c) {
			for (; *c != '\0'; ++c)
				;
		}
		strlcpy(uinfo->value.enumerated.name, c,
			sizeof(uinfo->value.enumerated.name));
		break;
	case ABOX_CONTROL_BYTE:
		uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
		uinfo->count = control->count;
		break;
	default:
		dev_err(dev, "%s(%s): invalid type:%d\n", __func__,
				kcontrol->id.name, control->type);
		return -EINVAL;
	}

	return 0;
}

static ABOX_IPC_MSG abox_component_control_get_msg;

static int abox_component_control_get_cache(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	int i;

	for (i = 0; i < value->control->count; i++) {
		switch (value->control->type) {
		case ABOX_CONTROL_INT:
			ucontrol->value.integer.value[i] = value->cache[i];
			break;
		case ABOX_CONTROL_ENUM:
			ucontrol->value.enumerated.item[i] = value->cache[i];
			break;
		case ABOX_CONTROL_BYTE:
			ucontrol->value.bytes.data[i] = value->cache[i];
			break;
		default:
			dev_err(dev, "%s(%s): invalid type:%d\n", __func__,
					kcontrol->id.name,
					value->control->type);
			break;
		}
	}

	return 0;
}

static int abox_component_control_get(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	ABOX_IPC_MSG *msg = &abox_component_control_get_msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg->msg.system;
	int i, ret;

	dev_dbg(dev, "%s\n", __func__);

	if (value->cache_only || !value->control->is_volatile)
		return abox_component_control_get_cache(kcontrol, ucontrol);

	msg->ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_COMPONENT_CONTROL;
	system_msg->param1 = value->desc->id;
	system_msg->param2 = value->control->id;
	ret = abox_request_ipc(dev, msg->ipcid, msg, sizeof(*msg), 0, 0);
	if (ret < 0)
		return ret;

	ret = wait_event_timeout(data->ipc_wait_queue,
			system_msg->msgtype == ABOX_REPORT_COMPONENT_CONTROL,
			msecs_to_jiffies(1000));
	if (system_msg->msgtype != ABOX_REPORT_COMPONENT_CONTROL)
		return -ETIME;

	for (i = 0; i < value->control->count; i++) {
		switch (value->control->type) {
		case ABOX_CONTROL_INT:
			ucontrol->value.integer.value[i] =
					system_msg->bundle.param_s32[i];
			break;
		case ABOX_CONTROL_ENUM:
			ucontrol->value.enumerated.item[i] =
					system_msg->bundle.param_s32[i];
			break;
		case ABOX_CONTROL_BYTE:
			ucontrol->value.bytes.data[i] =
					system_msg->bundle.param_bundle[i];
			break;
		default:
			dev_err(dev, "%s(%s): invalid type:%d\n", __func__,
					kcontrol->id.name,
					value->control->type);
			break;
		}
	}

	return 0;
}

static int abox_component_control_put_ipc(struct device *dev,
		struct abox_component_kcontrol_value *value)
{
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	int i;

	dev_dbg(dev, "%s\n", __func__);

	for (i = 0; i < value->control->count; i++) {
		int val = value->cache[i];
		char *name = value->control->name;

		switch (value->control->type) {
		case ABOX_CONTROL_INT:
			system_msg->bundle.param_s32[i] = val;
			dev_dbg(dev, "%s: %s[%d] = %d", __func__, name, i, val);
			break;
		case ABOX_CONTROL_ENUM:
			system_msg->bundle.param_s32[i] = val;
			dev_dbg(dev, "%s: %s[%d] = %d", __func__, name, i, val);
			break;
		case ABOX_CONTROL_BYTE:
			system_msg->bundle.param_bundle[i] = val;
			dev_dbg(dev, "%s: %s[%d] = %d", __func__, name, i, val);
			break;
		default:
			dev_err(dev, "%s(%s): invalid type:%d\n", __func__,
					name, value->control->type);
			break;
		}
	}

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_UPDATE_COMPONENT_CONTROL;
	system_msg->param1 = value->desc->id;
	system_msg->param2 = value->control->id;

	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

static int abox_component_control_put(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *cmpnt = snd_soc_kcontrol_component(kcontrol);
	struct device *dev = cmpnt->dev;
	struct abox_component_kcontrol_value *value =
			(void *)kcontrol->private_value;
	int i;

	dev_dbg(dev, "%s\n", __func__);

	for (i = 0; i < value->control->count; i++) {
		int val = (int)ucontrol->value.integer.value[i];
		char *name = kcontrol->id.name;

		value->cache[i] = val;
		dev_dbg(dev, "%s: %s[%d] <= %d", __func__, name, i, val);
	}

	return abox_component_control_put_ipc(dev, value);
}

#define ABOX_COMPONENT_KCONTROL(xname, xdesc, xcontrol)	\
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
	.access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
	.info = abox_component_control_info, \
	.get = abox_component_control_get, \
	.put = abox_component_control_put, \
	.private_value = \
		(unsigned long)&(struct abox_component_kcontrol_value) \
		{.desc = xdesc, .control = xcontrol} }

struct snd_kcontrol_new abox_component_kcontrols[] = {
	ABOX_COMPONENT_KCONTROL(NULL, NULL, NULL),
};

static int abox_register_control(struct device *dev,
		struct abox_data *data, struct abox_component *com,
		struct ABOX_COMPONENT_CONTROL *control)
{
	struct ABOX_COMPONENT_DESCRIPTIOR *desc = com->desc;
	struct abox_component_kcontrol_value *value;
	int len, ret;

	value = devm_kzalloc(dev, sizeof(*value) + (control->count *
			sizeof(value->cache[0])), GFP_KERNEL);
	if (!value)
		return -ENOMEM;

	if (control->texts.aaddr) {
		char *texts, *addr;
		size_t size;

		texts = abox_addr_to_kernel_addr(data, control->texts.aaddr);
		size = strlen(texts) + 1;
		addr = devm_kmalloc(dev, size + 1, GFP_KERNEL);
		texts = memcpy(addr, texts, size);
		texts[size] = '\0';
		for (len = 0; strsep(&addr, ","); len++)
			;
		if (control->max > len) {
			dev_dbg(dev, "%s: incomplete enumeration. expected=%d, received=%d\n",
					control->name, control->max, len);
			control->max = len;
		}
		control->texts.addr = texts;
	}

	value->desc = desc;
	value->control = control;

	if (strlen(desc->name))
		abox_component_kcontrols[0].name = kasprintf(GFP_KERNEL,
				"%s %s", desc->name, control->name);
	else
		abox_component_kcontrols[0].name = kasprintf(GFP_KERNEL,
				"%s", control->name);
	abox_component_kcontrols[0].private_value = (unsigned long)value;
	if (data->cmpnt && data->cmpnt->card) {
		ret = snd_soc_add_component_controls(data->cmpnt,
				abox_component_kcontrols, 1);
		if (ret >= 0)
			list_add_tail(&value->list, &com->value_list);
		else
			devm_kfree(dev, value);
	} else {
		ret = 0;
		devm_kfree(dev, value);
	}
	kfree_const(abox_component_kcontrols[0].name);

	return ret;
}

static int abox_register_void_component(struct abox_data *data,
		const void *void_component)
{
	static const unsigned int void_id = 0xAB0C;
	struct device *dev = &data->pdev->dev;
	const struct ABOX_COMPONENT_CONTROL *ctrls;
	struct abox_component *com;
	struct ABOX_COMPONENT_DESCRIPTIOR *desc;
	struct ABOX_COMPONENT_CONTROL *ctrl;
	size_t len;
	int ret;

	len = ARRAY_SIZE(data->components);
	for (com = data->components; com - data->components < len; com++) {
		if (com->desc && com->desc->id == void_id)
			return 0;

		if (!com->desc && !com->registered) {
			WRITE_ONCE(com->registered, true);
			INIT_LIST_HEAD(&com->value_list);
			break;
		}
	}

	dev_dbg(dev, "%s\n", __func__);

	for (ctrls = void_component, len = 0; ctrls->id; ctrls++)
		len++;

	desc = devm_kzalloc(dev, sizeof(*desc) + (sizeof(*ctrl) * len),
			GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	com->desc = desc;

	desc->id = void_id;
	desc->control_count = (unsigned int)len;
	memcpy(desc->controls, void_component, sizeof(*ctrl) * len);

	for (ctrl = desc->controls; ctrl->id; ctrl++) {
		dev_dbg(dev, "%s: %d, %s\n", __func__, ctrl->id, ctrl->name);

		ret = abox_register_control(dev, data, com, ctrl);
		if (ret < 0)
			dev_err(dev, "%s: %s register failed (%d)\n",
					__func__, ctrl->name, ret);
	}

	return ret;
}

static void abox_register_component_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			register_component_work);
	struct device *dev = &data->pdev->dev;
	struct abox_component *com;
	size_t len = ARRAY_SIZE(data->components);
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	for (com = data->components; com - data->components < len; com++) {
		struct ABOX_COMPONENT_DESCRIPTIOR *desc = com->desc;
		struct ABOX_COMPONENT_CONTROL *ctrl, *first;
		size_t ctrl_len, size;

		if (!desc || com->registered)
			continue;

		size = sizeof(*desc) + (sizeof(desc->controls[0]) *
				desc->control_count);
		com->desc = devm_kmemdup(dev, desc, size, GFP_KERNEL);
		if (!com->desc) {
			dev_err(dev, "%s: %s: alloc fail\n", __func__,
					desc->name);
			com->desc = desc;
			continue;
		}
		devm_kfree(dev, desc);
		desc = com->desc;
		first = desc->controls;
		ctrl_len = desc->control_count;

		for (ctrl = first; ctrl - first < ctrl_len; ctrl++) {
			ret = abox_register_control(dev, data, com, ctrl);
			if (ret < 0)
				dev_err(dev, "%s: %s register failed (%d)\n",
						__func__, ctrl->name, ret);
		}

		WRITE_ONCE(com->registered, true);
		dev_info(dev, "%s: %s registered\n", __func__, desc->name);
	}
}

static int abox_register_component(struct device *dev,
		struct ABOX_COMPONENT_DESCRIPTIOR *desc)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct abox_component *com;
	size_t len = ARRAY_SIZE(data->components);
	int ret;

	dev_dbg(dev, "%s(%d, %s)\n", __func__, desc->id, desc->name);

	for (com = data->components; com - data->components < len; com++) {
		struct ABOX_COMPONENT_DESCRIPTIOR *temp;
		size_t size;

		if (com->desc && !strcmp(com->desc->name, desc->name))
			break;

		if (com->desc)
			continue;

		size = sizeof(*desc) + (sizeof(desc->controls[0]) *
				desc->control_count);
		temp = devm_kmemdup(dev, desc, size, GFP_ATOMIC);
		if (!temp) {
			dev_err(dev, "%s: %s register fail\n", __func__,
					desc->name);
			ret = -ENOMEM;
		}
		INIT_LIST_HEAD(&com->value_list);
		WRITE_ONCE(com->desc, temp);
		schedule_work(&data->register_component_work);
		break;
	}

	return ret;
}

static void abox_restore_component_control(struct device *dev,
		struct abox_component_kcontrol_value *value)
{
	int count = value->control->count;
	int *val;

	for (val = value->cache; val - value->cache < count; val++) {
		if (*val) {
			abox_component_control_put_ipc(dev, value);
			break;
		}
	}
	value->cache_only = false;
}

static void abox_restore_components(struct device *dev, struct abox_data *data)
{
	struct abox_component *com;
	struct abox_component_kcontrol_value *value;
	size_t len = ARRAY_SIZE(data->components);

	dev_dbg(dev, "%s\n", __func__);

	for (com = data->components; com - data->components < len; com++) {
		if (!com->registered)
			break;
		list_for_each_entry(value, &com->value_list, list) {
			abox_restore_component_control(dev, value);
		}
	}
}

static void abox_cache_components(struct device *dev, struct abox_data *data)
{
	struct abox_component *com;
	struct abox_component_kcontrol_value *value;
	size_t len = ARRAY_SIZE(data->components);

	dev_dbg(dev, "%s\n", __func__);

	for (com = data->components; com - data->components < len; com++) {
		if (!com->registered)
			break;
		list_for_each_entry(value, &com->value_list, list) {
			value->cache_only = true;
		}
	}
}

static bool abox_is_calliope_incompatible(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	memcpy(&msg, data->sram_base + 0x30040, 0x3C);

	return ((system_msg->param3 >> 24) == 'A');
}

static int abox_set_profiling_ipc(struct abox_data *data)
{
	struct device *dev = &data->pdev->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	bool en = !data->no_profiling;

	dev_dbg(dev, "%s profiling\n", en ? "enable" : "disable");

	/* Profiler is enabled by default. */
	if (en)
		return 0;

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_DEBUG;
	system_msg->param1 = en;
	return abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
}

int abox_set_profiling(int en)
{
	struct abox_data *data = p_abox_data;

	if (!data)
		return -EAGAIN;

	data->no_profiling = !en;
	return abox_set_profiling_ipc(data);
}
EXPORT_SYMBOL(abox_set_profiling);

static void abox_restore_data(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);

	dev_dbg(dev, "%s\n", __func__);

	abox_restore_components(dev, data);
	abox_cmpnt_restore(dev);
	abox_effect_restore();
	data->restored = true;
	wake_up_all(&data->wait_queue);
}

void abox_wait_restored(struct abox_data *data)
{
	wait_event_timeout(data->wait_queue, data->restored == true, HZ);
}

static void abox_restore_data_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			restore_data_work);
	struct device *dev = &data->pdev->dev;

	abox_restore_data(dev);
	abox_set_profiling_ipc(data);
}

static void abox_boot_done_work_func(struct work_struct *work)
{
	struct abox_data *data = container_of(work, struct abox_data,
			boot_done_work);
	struct device *dev = &data->pdev->dev;

	dev_dbg(dev, "%s\n", __func__);

	abox_cpu_pm_ipc(data, true);
	abox_request_cpu_gear(dev, data, DEFAULT_CPU_GEAR_ID,
			ABOX_CPU_GEAR_MIN, "boot_done");
}

static void abox_boot_done(struct device *dev, unsigned int version)
{
	struct abox_data *data = dev_get_drvdata(dev);
	char ver_char[4];

	dev_dbg(dev, "%s\n", __func__);

	data->calliope_version = version;
	memcpy(ver_char, &version, sizeof(ver_char));
	dev_info(dev, "Calliope is ready to sing (version:%c%c%c%c)\n",
			ver_char[3], ver_char[2], ver_char[1], ver_char[0]);
	data->calliope_state = CALLIOPE_ENABLED;
	schedule_work(&data->boot_done_work);

	wake_up(&data->ipc_wait_queue);
}

static irqreturn_t abox_dma_irq_handler(int irq, struct abox_data *data)
{
	struct device *dev = &data->pdev->dev;
	int id;
	struct platform_device **pdev_dma;
	struct abox_platform_data *platform_data;

	dev_dbg(dev, "%s(%d)\n", __func__, irq);

	switch (irq) {
	case RDMA0_BUF_EMPTY:
		id = 0;
		pdev_dma = data->pdev_rdma;
		break;
	case RDMA1_BUF_EMPTY:
		id = 1;
		pdev_dma = data->pdev_rdma;
		break;
	case RDMA2_BUF_EMPTY:
		id = 2;
		pdev_dma = data->pdev_rdma;
		break;
	case RDMA3_BUF_EMPTY:
		id = 3;
		pdev_dma = data->pdev_rdma;
		break;
	case WDMA0_BUF_FULL:
		id = 0;
		pdev_dma = data->pdev_wdma;
		break;
	case WDMA1_BUF_FULL:
		id = 1;
		pdev_dma = data->pdev_wdma;
		break;
	default:
		return IRQ_NONE;
	}

	if (unlikely(!pdev_dma[id])) {
		dev_err(dev, "spurious dma irq: irq=%d id=%d\n", irq, id);
		return IRQ_HANDLED;
	}

	platform_data = platform_get_drvdata(pdev_dma[id]);
	if (unlikely(!platform_data)) {
		dev_err(dev, "dma irq with null data: irq=%d id=%d\n", irq, id);
		return IRQ_HANDLED;
	}

	platform_data->pointer = 0;
	snd_pcm_period_elapsed(platform_data->substream);

	return IRQ_HANDLED;
}

static irqreturn_t abox_registered_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg, bool broadcast)
{
	struct abox_ipc_action *action;
	int ipc_id = msg->ipcid;
	irqreturn_t ret = IRQ_NONE;

	dev_dbg(dev, "%s: ipc_id=%d\n", __func__, ipc_id);

	list_for_each_entry(action, &data->ipc_actions, list) {
		if (action->ipc_id != ipc_id)
			continue;

		ret = action->handler(ipc_id, action->data, msg);
		if (!broadcast && ret == IRQ_HANDLED)
			break;
	}

	return ret;
}

static void abox_system_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_SYSTEM_MSG *system_msg = &msg->msg.system;
	phys_addr_t addr;
	void *area;
	int ret;

	dev_dbg(dev, "msgtype=%d\n", system_msg->msgtype);

	switch (system_msg->msgtype) {
	case ABOX_BOOT_DONE:
		if (abox_is_calliope_incompatible(dev))
			dev_err(dev, "Calliope is not compatible with the driver\n");

		abox_boot_done(dev, system_msg->param3);
		abox_registered_ipc_handler(dev, data, msg, true);
		break;
	case ABOX_CHANGE_GEAR:
		abox_request_cpu_gear(dev, data, system_msg->param2,
				system_msg->param1, "calliope");
		break;
	case ABOX_REQUEST_SYSCLK:
		switch (system_msg->param2) {
		default:
			/* fall through */
		case 0:
			abox_qos_request_mif(dev, system_msg->param3,
					system_msg->param1, "calliope");
			break;
		case 1:
			abox_qos_request_int(dev, system_msg->param3,
					system_msg->param1, "calliope");
			break;
		}
		break;
	case ABOX_REPORT_LOG:
		area = abox_addr_to_kernel_addr(data, system_msg->param2);
		ret = abox_log_register_buffer(dev, system_msg->param1, area);
		if (ret < 0) {
			dev_err(dev, "log buffer registration failed: %u, %u\n",
					system_msg->param1, system_msg->param2);
		}
		break;
	case ABOX_FLUSH_LOG:
		break;
	case ABOX_REPORT_DUMP:
		area = abox_addr_to_kernel_addr(data, system_msg->param2);
		addr = abox_addr_to_phys_addr(data, system_msg->param2);
		ret = abox_dump_register_buffer(dev, system_msg->param1,
				system_msg->bundle.param_bundle, area, addr,
				system_msg->param3);
		if (ret < 0) {
			dev_err(dev, "dump buffer registration failed: %u, %u\n",
					system_msg->param1, system_msg->param2);
		}
		break;
	case ABOX_TRANSFER_DUMP:
		area = abox_addr_to_kernel_addr(data, system_msg->param2);
		abox_dump_transfer(system_msg->param1, area,
				system_msg->param3);
		break;
	case ABOX_FLUSH_DUMP:
		abox_dump_period_elapsed(system_msg->param1,
				system_msg->param2);
		break;
	case ABOX_REPORT_COMPONENT:
		area = abox_addr_to_kernel_addr(data, system_msg->param1);
		abox_register_component(dev, area);
		break;
	case ABOX_REPORT_COMPONENT_CONTROL:
		abox_component_control_get_msg = *msg;
		wake_up(&data->ipc_wait_queue);
		break;
	case ABOX_REPORT_FAULT:
	{
		const char *type;

		switch (system_msg->param1) {
		case 1:
			type = "data abort";
			break;
		case 2:
			type = "prefetch abort";
			break;
		case 3:
			type = "os error";
			break;
		case 4:
			type = "vss error";
			break;
		case 5:
			type = "undefined exception";
			break;
		default:
			type = "unknown error";
			break;
		}

		switch (system_msg->param1) {
		case 1:
		case 2:
			dev_err(dev, "%s(%#x, %#x, %#x, %#x) is reported from calliope\n",
					type, system_msg->param1,
					system_msg->param2, system_msg->param3,
					system_msg->bundle.param_s32[1]);
			area = abox_addr_to_kernel_addr(data,
					system_msg->bundle.param_s32[0]);
			abox_dbg_print_gpr_from_addr(dev, data, area);
			abox_dbg_dump_gpr_from_addr(dev, area,
					ABOX_DBG_DUMP_FIRMWARE, type);
			abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_FIRMWARE,
					type);
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
			abox_debug_string_update(system_msg->param1, 
					abox_addr_to_kernel_addr(data, system_msg->bundle.param_s32[0]));
#endif
			break;
		default:
			dev_err(dev, "%s(%#x, %#x, %#x) is reported from calliope\n",
					type, system_msg->param1,
					system_msg->param2, system_msg->param3);
			abox_dbg_print_gpr(dev, data);
			abox_dbg_dump_gpr(dev, data, ABOX_DBG_DUMP_FIRMWARE,
					type);
			abox_dbg_dump_mem(dev, data, ABOX_DBG_DUMP_FIRMWARE,
					type);
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
			if (system_msg->param1 > TYPE_ABOX_UNDEFEXCEPTION)
				abox_debug_string_update(TYPE_ABOX_UNDEFEXCEPTION, NULL);
			else
				abox_debug_string_update(system_msg->param1, NULL);
#endif
			break;
		}
		abox_failsafe_report(dev);
		break;
	}
	case ABOX_REPORT_CLK_DIFF_PPB:
		data->clk_diff_ppb = system_msg->param1;
		break;
	default:
		dev_warn(dev, "Redundant system message: %d(%d, %d, %d)\n",
				system_msg->msgtype, system_msg->param1,
				system_msg->param2, system_msg->param3);
		break;
	}
}

static void abox_pcm_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_PCMTASK_MSG *pcmtask_msg = &msg->msg.pcmtask;
	irqreturn_t ret;

	dev_dbg(dev, "msgtype=%d\n", pcmtask_msg->msgtype);

	ret = abox_registered_ipc_handler(dev, data, msg, false);
	if (ret != IRQ_HANDLED)
		dev_err(dev, "not handled pcm ipc: %d, %d, %d\n", msg->ipcid,
				pcmtask_msg->channel_id, pcmtask_msg->msgtype);
}

static void abox_offload_ipc_handler(struct device *dev,
		struct abox_data *data, ABOX_IPC_MSG *msg)
{
	struct IPC_OFFLOADTASK_MSG *offloadtask_msg = &msg->msg.offload;
	int id = offloadtask_msg->channel_id;
	struct abox_platform_data *platform_data;

	if (id != 5) {
		dev_warn(dev, "%s: unknown channel id(%d)\n", __func__, id);
		id = 5;
	}
	platform_data = platform_get_drvdata(data->pdev_rdma[id]);

	if (platform_data->compr_data.isr_handler)
		platform_data->compr_data.isr_handler(data->pdev_rdma[id]);
	else
		dev_warn(dev, "Redundant offload message on rdma[%d]", id);
}

static irqreturn_t abox_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);
	ABOX_IPC_MSG msg;

	if (abox_dma_irq_handler(irq, data) == IRQ_HANDLED)
		goto out;

	memcpy(&msg, data->sram_base + data->ipc_rx_offset, sizeof(msg));
	writel(0, data->sram_base + data->ipc_rx_ack_offset);

	dev_dbg(dev, "%s: irq=%d, ipcid=%d\n", __func__, irq, msg.ipcid);

	switch (msg.ipcid) {
	case IPC_SYSTEM:
		abox_system_ipc_handler(dev, data, &msg);
		break;
	case IPC_PCMPLAYBACK:
	case IPC_PCMCAPTURE:
		abox_pcm_ipc_handler(dev, data, &msg);
		break;
	case IPC_OFFLOAD:
		abox_offload_ipc_handler(dev, data, &msg);
		break;
	default:
		abox_registered_ipc_handler(dev, data, &msg, false);
		break;
	}
out:
	abox_log_schedule_flush_all(dev);

	dev_dbg(dev, "%s: exit\n", __func__);
	return IRQ_HANDLED;
}

static irqreturn_t abox_ipc_handler(int ipc, void *dev_id, ABOX_IPC_MSG *msg)
{
	struct platform_device *pdev = dev_id;
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	dev_dbg(dev, "%s: ipc=%d, ipcid=%d\n", __func__, ipc, msg->ipcid);

	switch (ipc) {
	case IPC_SYSTEM:
		abox_system_ipc_handler(dev, data, msg);
		break;
	case IPC_PCMPLAYBACK:
	case IPC_PCMCAPTURE:
		abox_pcm_ipc_handler(dev, data, msg);
		break;
	case IPC_OFFLOAD:
		abox_offload_ipc_handler(dev, data, msg);
		break;
	default:
		abox_registered_ipc_handler(dev, data, msg, false);
		break;
	}

	abox_log_schedule_flush_all(dev);

	dev_dbg(dev, "%s: exit\n", __func__);
	return IRQ_HANDLED;
}

int abox_register_extra_sound_card(struct device *dev,
		struct snd_soc_card *card, unsigned int idx)
{
	static DEFINE_MUTEX(lock);
	static struct snd_soc_card *cards[SNDRV_CARDS];
	int i;

	if (idx >= ARRAY_SIZE(cards))
		return -EINVAL;

	mutex_lock(&lock);
	dev_dbg(dev, "%s(%s, %u)\n", __func__, card->name, idx);

	cards[idx] = card;

	for (i = ARRAY_SIZE(cards) - 1; i >= idx; i--)
		if (cards[i])
			snd_soc_unregister_card(cards[i]);

	for (i = idx; i < ARRAY_SIZE(cards); i++)
		if (cards[i])
			snd_soc_register_card(cards[i]);

	mutex_unlock(&lock);
	return 0;
}

static int abox_cpu_suspend_complete(struct device *dev)
{
	static const u64 timeout = NSEC_PER_MSEC * 100;
	struct abox_data *data = dev_get_drvdata(dev);
	struct regmap *regmap = data->timer_regmap;
	unsigned int value = 1;
	u64 limit;

	dev_dbg(dev, "%s\n", __func__);

	limit = local_clock() + timeout;

	while (regmap_read(regmap, ABOX_TIMER_PRESET_LSB(1), &value) >= 0) {
		if (value == 0)
			break;

		if (local_clock() > limit) {
			dev_err(dev, "%s: timeout\n", __func__);
			break;
		}
	}

	return value ? -ETIME : 0;
}

static int abox_cpu_pm_ipc(struct abox_data *data, bool resume)
{
	const unsigned long long TIMER_RATE = 26000000;
	struct device *dev = &data->pdev->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system = &msg.msg.system;
	unsigned long long ktime, atime;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	ktime = sched_clock();
	atime = readq_relaxed(data->timer_base + ABOX_TIMER_CURVALUD_LSB(1));
	/* clock to ns */
	atime *= 500;
	do_div(atime, TIMER_RATE / 2000000);

	msg.ipcid = IPC_SYSTEM;
	system->msgtype = resume ? ABOX_RESUME : ABOX_SUSPEND;
	system->bundle.param_u64[0] = ktime;
	system->bundle.param_u64[1] = atime;
	ret = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 1);
	if (!resume) {
		abox_cpu_suspend_complete(dev);
		abox_core_standby();
	}

	return ret;
}

static void abox_pad_retention(bool retention)
{
	if (!retention) {
#ifndef EMULATOR
		exynos_pmu_update(ABOX_PAD_NORMAL,
				ABOX_PAD_NORMAL_MASK, ABOX_PAD_NORMAL_MASK);
#else
		update_mask_value(pmu_alive + ABOX_PAD_NORMAL,
				ABOX_PAD_NORMAL_MASK, ABOX_PAD_NORMAL_MASK);
#endif
	}
}

static void abox_cpu_power(bool on)
{
	abox_core_power(on);
}

static int abox_cpu_enable(bool enable)
{
	/* reset core only if disable */
	if (!enable)
		abox_core_enable(enable);

	return 0;
}

static void abox_save_register(struct abox_data *data)
{
	regcache_cache_only(data->regmap, true);
	regcache_mark_dirty(data->regmap);
}

static void abox_restore_register(struct abox_data *data)
{
	regcache_cache_only(data->regmap, false);
	regcache_sync(data->regmap);
}

static int abox_request_firmware(struct device *dev,
		const struct firmware **fw, const char *name)
{
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	release_firmware(*fw);
	ret = request_firmware_direct(fw, name, dev);
	if (ret == -ENOENT)
		dev_warn(dev, "%s: %s file does not exist(%d)\n", __func__,
				name, ret);
	else if (ret < 0)
		dev_err(dev, "%s: %s request failed(%d)\n", __func__,
				name, ret);
	else
		dev_info(dev, "%s is loaded\n", name);

	return ret;
}

static struct abox_extra_firmware *abox_get_extra_firmware(
		struct abox_data *data, const char *name)
{
	struct abox_extra_firmware *efw;

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (strcmp(efw->name, name) == 0)
			return efw;
	}

	return NULL;
}

static void abox_reload_extra_firmware(struct abox_data *data, const char *name)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	struct abox_extra_firmware *efw;

	dev_dbg(dev, "%s(%s)\n", __func__, name);

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (!efw->name || strcmp(efw->name, name))
			continue;

		abox_request_firmware(dev, &efw->firmware, efw->name);
	}
}

static void abox_request_extra_firmware(struct abox_data *data)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child_np;
	struct abox_extra_firmware *efw;
	const char *name;
	u32 area, offset;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	for_each_available_child_of_node(np, child_np) {
		ret = of_property_read_string(child_np, "samsung,name", &name);
		if (ret < 0)
			continue;

		ret = of_property_read_u32(child_np, "samsung,area", &area);
		if (ret < 0)
			continue;

		ret = of_property_read_u32(child_np, "samsung,offset", &offset);
		if (ret < 0)
			continue;

		efw = abox_get_extra_firmware(data, name);
		if (!efw) {
			dev_info(dev, "new extra firmware %s\n", name);
			efw = devm_kzalloc(dev, sizeof(*efw), GFP_KERNEL);
			list_add_tail(&efw->list, &data->firmware_extra);
			efw->name = name;
			efw->area = area;
			efw->offset = offset;
		}
		if (!efw) {
			dev_err(dev, "%s: no memory %s\n", __func__, name);
			continue;
		}

		dev_dbg(dev, "%s: name=%s, area=%u, offset=%u\n", __func__,
				efw->name, efw->area, efw->offset);

		if (!efw->firmware) {
			dev_dbg(dev, "%s: request %s\n", __func__, efw->name);
			abox_request_firmware(dev, &efw->firmware, efw->name);
			efw->kcontrol = !!of_find_property(child_np,
					"samsung,mixer-control", NULL);
		}
	}
}

static void abox_download_extra_firmware(struct abox_data *data)
{
	struct device *dev = &data->pdev->dev;
	struct abox_extra_firmware *efw;
	void __iomem *base;
	size_t size;

	dev_dbg(dev, "%s\n", __func__);

	list_for_each_entry(efw, &data->firmware_extra, list) {
		if (!efw->firmware)
			continue;

		switch (efw->area) {
		case 0:
			base = data->sram_base;
			size = data->sram_size;
			break;
		case 1:
			base = data->dram_base;
			size = DRAM_FIRMWARE_SIZE;
			break;
		case 2:
			base = shm_get_vss_region();
			size = shm_get_vss_size();
			break;
		default:
			dev_err(dev, "%s: invalied area %s, %u, %u\n",
					__func__, efw->name, efw->area,
					efw->offset);
			continue;
		}

		if (efw->offset + efw->firmware->size > size) {
			dev_err(dev, "%s: too large firmware %s, %u, %u\n",
					__func__, efw->name, efw->area,
					efw->offset);
			continue;
		}

		memcpy(base + efw->offset, efw->firmware->data,
				efw->firmware->size);
		dev_dbg(dev, "%s is downloaded %u, %#x\n", efw->name,
				efw->area, efw->offset);
		if (efw->kcontrol)
			abox_register_void_component(data, efw->firmware->data);
	}
}

static int abox_download_firmware(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	int ret;

	ret = abox_core_download_firmware();
	if (ret)
		return ret;

	abox_request_extra_firmware(data);
	abox_download_extra_firmware(data);

	return 0;
}

static void abox_cfg_gpio(struct device *dev, const char *name)
{
	struct abox_data *data = dev_get_drvdata(dev);
	struct pinctrl_state *pin_state;
	int ret;

	dev_dbg(dev, "%s(%s)\n", __func__, name);

	if (!data->pinctrl)
		return;

	pin_state = pinctrl_lookup_state(data->pinctrl, name);
	if (IS_ERR(pin_state)) {
		dev_err(dev, "Couldn't find pinctrl %s\n", name);
	} else {
		ret = pinctrl_select_state(data->pinctrl, pin_state);
		if (ret < 0)
			dev_err(dev, "Unable to configure pinctrl %s\n", name);
	}
}

#undef MANUAL_SECURITY_CHANGE
#ifdef MANUAL_SECURITY_CHANGE
static void work_temp_function(struct work_struct *work)
{
	exynos_smc(0x82000701, 0, 0, 0);
	pr_err("%s: ABOX_CA7 security changed!!!\n", __func__);
}
static DECLARE_DELAYED_WORK(work_temp, work_temp_function);
#endif

static bool abox_is_timer_set(struct abox_data *data)
{
	unsigned int val;
	int ret;

	ret = regmap_read(data->timer_regmap, ABOX_TIMER_CTRL1(0), &val);
	if (ret < 0)
		val = 0;

	return !!val;
}

static void abox_start_timer(struct abox_data *data)
{
	struct regmap *regmap = data->timer_regmap;

	regmap_write(regmap, ABOX_TIMER_CTRL0(0), 1 << ABOX_TIMER_START_L);
	regmap_write(regmap, ABOX_TIMER_CTRL0(2), 1 << ABOX_TIMER_START_L);
}

static void abox_update_suspend_wait_flag(struct abox_data *data, bool suspend)
{
	const unsigned int HOST_SUSPEND_HOLDING_FLAG = 0x74736F68; /* host */
	const unsigned int HOST_SUSPEND_RELEASE_FLAG = 0x656B6177; /* wake */
	unsigned int flag;

	flag = suspend ? HOST_SUSPEND_HOLDING_FLAG : HOST_SUSPEND_RELEASE_FLAG;
	WRITE_ONCE(data->hndshk_tag->suspend_wait_flag, flag);
}

static int abox_enable(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	bool has_reset;
	int ret = 0;

	dev_info(dev, "%s\n", __func__);

	abox_gic_enable_irq(data->dev_gic);
	abox_enable_wdt(data);

	abox_request_cpu_gear_sync(dev, data, DEFAULT_CPU_GEAR_ID,
			ABOX_CPU_GEAR_MAX, "enable");

	writel(0x1, data->sysreg_base + ABOX_SYSREG_MISC_CON);
	if (abox_test_quirk(data, ABOX_QUIRK_BIT_SHARE_VTS_SRAM))
		writel(0x1, data->sysreg_base + ABOX_SYSREG_L2_CACHE_CON);

	if (data->clk_cpu) {
		ret = clk_enable(data->clk_cpu);
		if (ret < 0) {
			dev_err(dev, "Failed to enable cpu clock: %d\n", ret);
			goto error;
		}
	}

	ret = clk_enable(data->clk_audif);
	if (ret < 0) {
		dev_err(dev, "Failed to enable audif clock: %d\n", ret);
		goto error;
	}

	abox_cfg_gpio(dev, "default");
	abox_pad_retention(false);

	abox_restore_register(data);
	has_reset = !abox_is_timer_set(data);
	if (!has_reset) {
		dev_info(dev, "wakeup from WFI\n");
		abox_update_suspend_wait_flag(data, false);
		abox_start_timer(data);
	} else {
		abox_gic_init_gic(data->dev_gic);

		ret = abox_download_firmware(dev);
		if (ret < 0) {
			if (ret != -EAGAIN)
				dev_err(dev, "Failed to download firmware\n");
			else
				ret = 0;

			abox_request_cpu_gear(dev, data, DEFAULT_CPU_GEAR_ID,
					ABOX_CPU_GEAR_MIN, "enable");
			goto error;
		}
	}

	abox_request_dram_on(dev, DEFAULT_SYS_POWER_ID, true);
	data->calliope_state = CALLIOPE_ENABLING;
	if (has_reset) {
		abox_cpu_power(true);
		abox_cpu_enable(true);
	}

#ifdef MANUAL_SECURITY_CHANGE
	schedule_delayed_work(&work_temp, msecs_to_jiffies(3000));
#endif

	data->enabled = true;
	if (has_reset)
		pm_wakeup_event(dev, BOOT_DONE_TIMEOUT_MS);
	else
		abox_boot_done(dev, data->calliope_version);

	schedule_work(&data->restore_data_work);
error:
	return ret;
}

static int abox_disable(struct device *dev)
{
	struct abox_data *data = dev_get_drvdata(dev);
	enum calliope_state state = data->calliope_state;

	dev_info(dev, "%s\n", __func__);

	abox_update_suspend_wait_flag(data, true);
	data->calliope_state = CALLIOPE_DISABLING;
	abox_cache_components(dev, data);
	flush_work(&data->boot_done_work);
	if (state != CALLIOPE_DISABLED)
		abox_cpu_pm_ipc(data, false);
	data->calliope_state = CALLIOPE_DISABLED;
	abox_log_drain_all(dev);
	abox_request_dram_on(dev, DEFAULT_SYS_POWER_ID, false);
	abox_save_register(data);
	abox_cfg_gpio(dev, "idle");
	abox_pad_retention(true);
	data->enabled = false;
	data->restored = false;
	if (data->clk_cpu)
		clk_disable(data->clk_cpu);
	abox_gic_disable_irq(data->dev_gic);
	abox_failsafe_report_reset(dev);
	abox_dbg_dump_suspend(dev, data);
	return 0;
}

static int abox_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	return abox_disable(dev);
}

static int abox_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);

	return abox_enable(dev);
}

static int abox_suspend(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
	/* nothing to do */
	return 0;
}

static int abox_resume(struct device *dev)
{
	dev_dbg(dev, "%s\n", __func__);
	/* nothing to do */
	return 0;
}

static int abox_qos_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, qos_nb);
	struct device *dev = &data->pdev->dev;
	long value = (long)action;

	if (!pm_runtime_active(dev))
		return NOTIFY_DONE;

	dev_info(dev, "pm qos aud: %ldkHz\n", value);
	abox_notify_cpu_gear(data, value * 1000);
	abox_cmpnt_update_cnt_val(dev);
	abox_cmpnt_update_asrc_tick(dev);

	return NOTIFY_DONE;
}

static int abox_print_power_usage(struct device *dev, void *data)
{
	dev_dbg(dev, "%s\n", __func__);

	if (pm_runtime_enabled(dev) && pm_runtime_active(dev)) {
		dev_info(dev, "usage_count:%d\n",
				atomic_read(&dev->power.usage_count));
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
		sec_audio_pmlog(6, dev, "usage_count:%d\n",
				atomic_read(&dev->power.usage_count));
#endif
		device_for_each_child(dev, data, abox_print_power_usage);
	}

	return 0;
}

static int abox_pm_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, pm_nb);
	struct device *dev = &data->pdev->dev;
	int ret;

	dev_dbg(dev, "%s(%lu)\n", __func__, action);

	switch (action) {
	case PM_SUSPEND_PREPARE:
		pm_runtime_barrier(dev);
		abox_cpu_gear_barrier();
		flush_workqueue(data->ipc_workqueue);
		if (abox_is_clearable(dev, data)) {
			if (atomic_read(&dev->power.child_count) < 1)
				abox_qos_print(dev, ABOX_QOS_AUD);

			ret = pm_runtime_suspend(dev);
			if (ret < 0) {
				dev_info(dev, "runtime suspend: %d\n", ret);
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
				sec_audio_pmlog(6, dev, "runtime suspend: %d\n", ret);
#endif
				abox_print_power_usage(dev, NULL);
				return NOTIFY_BAD;
			}
		}
		break;
	default:
		/* Nothing to do */
		break;
	}
	return NOTIFY_DONE;
}

static int abox_modem_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, modem_nb);
	struct device *dev = &data->pdev->dev;
	ABOX_IPC_MSG msg;
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;

	if (!abox_is_on())
		return NOTIFY_DONE;

	dev_info(&data->pdev->dev, "%s(%lu)\n", __func__, action);

	msg.ipcid = IPC_SYSTEM;
	switch (action) {
	case MODEM_EVENT_RESET:
		system_msg->msgtype = ABOX_RESET_VSS;
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
		abox_debug_string_update(TYPE_MODEM_CPCRASH, NULL);
#endif
		break;
	case MODEM_EVENT_EXIT:
		system_msg->msgtype = ABOX_STOP_VSS;
		abox_dbg_print_gpr(dev, data);
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
		abox_debug_string_update(TYPE_MODEM_CPCRASH, NULL);
#endif
		break;
	case MODEM_EVENT_ONLINE:
		system_msg->msgtype = ABOX_START_VSS;
		break;
	case MODEM_EVENT_WATCHDOG:
		system_msg->msgtype = ABOX_WATCHDOG_VSS;
#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
		abox_debug_string_update(TYPE_MODEM_CPCRASH, NULL);
#endif
		break;
	default:
		system_msg->msgtype = 0;
		break;
	}

	if (system_msg->msgtype)
		abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 1, 0);

	return NOTIFY_DONE;
}

#ifdef CONFIG_EXYNOS_ITMON
static int abox_itmon_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct abox_data *data = container_of(nb, struct abox_data, itmon_nb);
	struct device *dev = &data->pdev->dev;
	struct itmon_notifier *itmon_data = nb_data;

	if (itmon_data && itmon_data->dest && (strncmp("ABOX", itmon_data->dest,
			sizeof("ABOX") - 1) == 0)) {
		dev_info(dev, "%s(%lu)\n", __func__, action);
		data->enabled = false;
		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}
#endif

static ssize_t calliope_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct abox_data *data = dev_get_drvdata(dev);
	unsigned int version = be32_to_cpu(data->calliope_version);

	memcpy(buf, &version, sizeof(version));
	buf[4] = '\n';
	buf[5] = '\0';

	return 6;
}

static ssize_t calliope_debug_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	ABOX_IPC_MSG msg = {0,};
	struct IPC_SYSTEM_MSG *system_msg = &msg.msg.system;
	int ret;

	dev_dbg(dev, "%s\n", __func__);

	msg.ipcid = IPC_SYSTEM;
	system_msg->msgtype = ABOX_REQUEST_DEBUG;
	ret = sscanf(buf, "%10d,%10d,%10d,%739s", &system_msg->param1,
			&system_msg->param2, &system_msg->param3,
			system_msg->bundle.param_bundle);
	if (ret < 0)
		return ret;

	ret = abox_request_ipc(dev, msg.ipcid, &msg, sizeof(msg), 0, 0);
	if (ret < 0)
		return ret;

	return count;
}

static ssize_t calliope_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	static const char cmd_reload_ext_bin[] = "RELOAD EXT BIN";
	static const char cmd_failsafe[] = "FAILSAFE";
	static const char cmd_cpu_gear[] = "CPU GEAR";
	struct abox_data *data = dev_get_drvdata(dev);
	char name[80];

	dev_dbg(dev, "%s(%s)\n", __func__, buf);
	if (!strncmp(cmd_reload_ext_bin, buf, sizeof(cmd_reload_ext_bin) - 1)) {
		dev_dbg(dev, "reload ext bin\n");
		if (sscanf(buf, "RELOAD EXT BIN:%63s", name) == 1)
			abox_reload_extra_firmware(data, name);
	} else if (!strncmp(cmd_failsafe, buf, sizeof(cmd_failsafe) - 1)) {
		dev_dbg(dev, "failsafe\n");
		abox_failsafe_report(dev);
	} else if (!strncmp(cmd_cpu_gear, buf, sizeof(cmd_cpu_gear) - 1)) {
		unsigned int gear;
		int ret;

		dev_info(dev, "set clk\n");
		ret = kstrtouint(buf + sizeof(cmd_cpu_gear), 10, &gear);
		if (!ret) {
			dev_info(dev, "gear = %u\n", gear);
			pm_runtime_get_sync(dev);
			abox_request_cpu_gear(dev, data, TEST_CPU_GEAR_ID,
					gear, "calliope_cmd");
			pm_runtime_mark_last_busy(dev);
			pm_runtime_put_autosuspend(dev);
		}
	}

	return count;
}

static DEVICE_ATTR_RO(calliope_version);
static DEVICE_ATTR_WO(calliope_debug);
static DEVICE_ATTR_WO(calliope_cmd);

static int samsung_abox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_tmp;
	struct platform_device *pdev_tmp;
	struct abox_data *data;
	phys_addr_t paddr;
	int ret, i;

	dev_info(dev, "%s\n", __func__);

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	platform_set_drvdata(pdev, data);
	data->pdev = pdev;
	p_abox_data = data;

	abox_probe_quirks(data, np);
	init_waitqueue_head(&data->ipc_wait_queue);
	init_waitqueue_head(&data->wait_queue);
	spin_lock_init(&data->ipc_queue_lock);
	spin_lock_init(&data->iommu_lock);
	device_init_wakeup(dev, true);
	data->cpu_gear = ABOX_CPU_GEAR_MIN;
	data->cpu_gear_min = 3; /* default value from kangchen */
	for (i = 0; i < ARRAY_SIZE(data->sif_rate); i++) {
		data->sif_rate_min[i] = data->sif_rate[i] = 48000;
		data->sif_format_min[i] = data->sif_format[i] =
				SNDRV_PCM_FORMAT_S16;
		data->sif_channels_min[i] = data->sif_channels[i] = 2;
	}
	INIT_WORK(&data->ipc_work, abox_process_ipc);
	INIT_WORK(&data->register_component_work,
			abox_register_component_work_func);
	INIT_WORK(&data->restore_data_work, abox_restore_data_work_func);
	INIT_WORK(&data->boot_done_work, abox_boot_done_work_func);
	INIT_DELAYED_WORK(&data->wdt_work, abox_wdt_work_func);
	INIT_LIST_HEAD(&data->firmware_extra);
	INIT_LIST_HEAD(&data->ipc_actions);
	INIT_LIST_HEAD(&data->iommu_maps);

	data->ipc_workqueue = alloc_ordered_workqueue("abox_ipc",
			WQ_MEM_RECLAIM);
	if (!data->ipc_workqueue) {
		dev_err(dev, "Couldn't create workqueue %s\n", "abox_ipc");
		return -ENOMEM;
	}

	data->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(data->pinctrl)) {
		dev_dbg(dev, "Couldn't get pins (%li)\n",
				PTR_ERR(data->pinctrl));
		data->pinctrl = NULL;
	}

	data->sfr_base = devm_get_request_ioremap(pdev, "sfr",
			NULL, NULL);
	if (IS_ERR(data->sfr_base))
		return PTR_ERR(data->sfr_base);

	data->sysreg_base = devm_get_request_ioremap(pdev, "sysreg",
			NULL, NULL);
	if (IS_ERR(data->sysreg_base))
		return PTR_ERR(data->sysreg_base);

	data->sram_base = devm_get_request_ioremap(pdev, "sram",
			&data->sram_base_phys, &data->sram_size);
	if (IS_ERR(data->sram_base))
		return PTR_ERR(data->sram_base);

	data->timer_base = devm_get_request_ioremap(pdev, "timer",
			NULL, NULL);
	if (IS_ERR(data->timer_base))
		return PTR_ERR(data->timer_base);


	data->iommu_domain = get_domain_from_dev(dev);
	if (IS_ERR(data->iommu_domain)) {
		dev_err(dev, "Unable to get iommu domain\n");
		return PTR_ERR(data->iommu_domain);
	}

	ret = iommu_attach_device(data->iommu_domain, dev);
	if (ret < 0) {
		dev_err(dev, "Unable to attach device to iommu (%d)\n", ret);
		return ret;
	}

	data->dram_base = dmam_alloc_coherent(dev, DRAM_FIRMWARE_SIZE,
			&data->dram_base_phys, GFP_KERNEL);
	if (!data->dram_base) {
		dev_err(dev, "%s: no memory\n", "dram firmware");
		return -ENOMEM;
	}
	dev_info(dev, "%s(%#x) alloc\n", "dram firmware", DRAM_FIRMWARE_SIZE);
	abox_iommu_map(dev, IOVA_DRAM_FIRMWARE, data->dram_base_phys,
			DRAM_FIRMWARE_SIZE, data->dram_base);

	data->priv_base = dmam_alloc_coherent(dev, PRIVATE_SIZE,
			&data->priv_base_phys, GFP_KERNEL);
	if (!data->priv_base) {
		dev_err(dev, "%s: no memory\n", "private");
		return -ENOMEM;
	}
	dev_info(dev, "%s(%#x) alloc\n", "private", PRIVATE_SIZE);
	abox_iommu_map(dev, IOVA_PRIVATE, data->priv_base_phys,
			PRIVATE_SIZE, data->priv_base);

	paddr = shm_get_vss_base();
	dev_info(dev, "%s(%#x) alloc\n", "vss firmware", shm_get_vss_size());
	abox_iommu_map(dev, IOVA_VSS_FIRMWARE, paddr, shm_get_vss_size(),
			shm_get_vss_region());

	paddr = shm_get_vparam_base();
	dev_info(dev, "%s(%#x) alloc\n", "vss parameter",
			shm_get_vparam_size());
	abox_iommu_map(dev, IOVA_VSS_PARAMETER, paddr, shm_get_vparam_size(),
			shm_get_vparam_region());

	abox_iommu_map(dev, 0x10000000, 0x10000000, PAGE_SIZE, 0);
	iovmm_set_fault_handler(dev, abox_iommu_fault_handler, data);

	data->clk_pll = devm_clk_get_and_prepare(pdev, "pll");
	if (IS_ERR(data->clk_pll))
		return PTR_ERR(data->clk_pll);

	data->clk_pll1 = devm_clk_get_and_prepare(pdev, "pll1");
	if (IS_ERR(data->clk_pll1))
		return PTR_ERR(data->clk_pll1);

	data->clk_audif = devm_clk_get_and_prepare(pdev, "audif");
	if (IS_ERR(data->clk_audif))
		return PTR_ERR(data->clk_audif);

	data->clk_cpu = devm_clk_get_and_prepare(pdev, "cpu");
	if (IS_ERR(data->clk_cpu)) {
		if (IS_ENABLED(CONFIG_SOC_EXYNOS8895))
			return PTR_ERR(data->clk_cpu);

		data->clk_cpu = NULL;
	}

	data->clk_dmic = devm_clk_get_and_prepare(pdev, "dmic");
	if (IS_ERR(data->clk_dmic))
		return PTR_ERR(data->clk_dmic);

	data->clk_bus = devm_clk_get_and_prepare(pdev, "bus");
	if (IS_ERR(data->clk_bus))
		return PTR_ERR(data->clk_bus);

	data->clk_cnt = devm_clk_get_and_prepare(pdev, "cnt");
	if (IS_ERR(data->clk_cnt))
		data->clk_cnt = NULL;

	ret = of_samsung_property_read_u32(dev, np, "uaif-max-div",
			&data->uaif_max_div);
	if (ret < 0)
		data->uaif_max_div = 32;

	ret = of_samsung_property_read_u32(dev, np, "ipc-tx-offset",
			&data->ipc_tx_offset);
	if (ret < 0)
		return ret;

	ret = of_samsung_property_read_u32(dev, np, "ipc-rx-offset",
			&data->ipc_rx_offset);
	if (ret < 0)
		return ret;

	ret = of_samsung_property_read_u32(dev, np, "ipc-tx-ack-offset",
			&data->ipc_tx_ack_offset);
	if (ret < 0)
		return ret;

	ret = of_samsung_property_read_u32(dev, np, "ipc-rx-ack-offset",
			&data->ipc_rx_ack_offset);
	if (ret < 0)
		return ret;

	of_samsung_property_read_u32_array(dev, np, "pm-qos-int",
			data->pm_qos_int, ARRAY_SIZE(data->pm_qos_int));

	ret = of_samsung_property_read_u32_array(dev, np, "pm-qos-aud",
			data->pm_qos_aud, ARRAY_SIZE(data->pm_qos_aud));
	if (ret >= 0) {
		for (i = 0; i < ARRAY_SIZE(data->pm_qos_aud); i++) {
			if (!data->pm_qos_aud[i]) {
				data->cpu_gear_min = i;
				break;
			}
		}
	}

	np_tmp = of_parse_phandle(np, "samsung,abox-gic", 0);
	if (!np_tmp) {
		dev_err(dev, "Failed to get abox-gic device node\n");
		return -EPROBE_DEFER;
	}
	pdev_tmp = of_find_device_by_node(np_tmp);
	if (!pdev_tmp) {
		dev_err(dev, "Failed to get abox-gic platform device\n");
		return -EPROBE_DEFER;
	}
	data->dev_gic = &pdev_tmp->dev;

	if (abox_test_quirk(data, ABOX_QUIRK_BIT_SHARE_VTS_SRAM)) {
		np_tmp = of_parse_phandle(np, "vts", 0);
		if (!np_tmp) {
			dev_err(dev, "Failed to get vts device node\n");
			return -EPROBE_DEFER;
		}
		data->pdev_vts = of_find_device_by_node(np_tmp);
		if (!data->pdev_vts) {
			dev_err(dev, "Failed to get vts platform device\n");
			return -EPROBE_DEFER;
		}
	}

#ifdef EMULATOR
	pmu_alive = ioremap(0x16480000, 0x10000);
#endif
	abox_gic_register_irq_handler(data->dev_gic, IRQ_WDT, abox_wdt_handler,
			data);
	for (i = 0; i < SGI_ABOX_MSG; i++)
		abox_gic_register_irq_handler(data->dev_gic, i,
				abox_irq_handler, pdev);

	abox_of_get_addr(data, np, "samsung,ipc-tx-area", &data->ipc_tx_addr,
			&data->ipc_tx_size);
	abox_of_get_addr(data, np, "samsung,ipc-rx-area", &data->ipc_rx_addr,
			&data->ipc_rx_size);
	abox_of_get_addr(data, np, "samsung,handshake-area",
			(void **)&data->hndshk_tag, NULL);
	ret = abox_ipc_init(dev, data->ipc_tx_addr, data->ipc_tx_size,
			data->ipc_rx_addr, data->ipc_rx_size);
	data->ipc_version = (ret < 0) ? 1 : 2;
	dev_info(dev, "ipc version=%d\n", data->ipc_version);
	for (i = 0; i < IPC_ID_COUNT; i++)
		abox_ipc_register_handler(dev, i, abox_ipc_handler, pdev);

	data->regmap = abox_soc_get_regmap(dev);

	data->timer_regmap = devm_regmap_init_mmio(dev,
			data->timer_base,
			&abox_timer_regmap_config);

	abox_qos_init(dev);

	pm_runtime_enable(dev);
	pm_runtime_set_autosuspend_delay(dev, 500);
	pm_runtime_use_autosuspend(dev);
	pm_runtime_get(dev);

	data->qos_nb.notifier_call = abox_qos_notifier;
	pm_qos_add_notifier(PM_QOS_AUD_THROUGHPUT, &data->qos_nb);

	data->pm_nb.notifier_call = abox_pm_notifier;
	register_pm_notifier(&data->pm_nb);

	data->modem_nb.notifier_call = abox_modem_notifier;
	register_modem_event_notifier(&data->modem_nb);

#ifdef CONFIG_EXYNOS_ITMON
	data->itmon_nb.notifier_call = abox_itmon_notifier;
	itmon_notifier_chain_register(&data->itmon_nb);
#endif

	abox_failsafe_init(dev);

	ret = device_create_file(dev, &dev_attr_calliope_version);
	if (ret < 0)
		dev_warn(dev, "Failed to create file: %s\n", "version");

	ret = device_create_file(dev, &dev_attr_calliope_debug);
	if (ret < 0)
		dev_warn(dev, "Failed to create file: %s\n", "debug");

	ret = device_create_file(dev, &dev_attr_calliope_cmd);
	if (ret < 0)
		dev_warn(dev, "Failed to create file: %s\n", "cmd");

	atomic_notifier_chain_register(&panic_notifier_list,
			&abox_panic_notifier);

	ret = abox_cmpnt_register(dev);
	if (ret < 0)
		dev_err(dev, "component register failed:%d\n", ret);

	dev_info(dev, "%s: probe complete\n", __func__);

	of_platform_populate(np, NULL, NULL, dev);

	return 0;
}

static int samsung_abox_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct abox_data *data = platform_get_drvdata(pdev);

	dev_info(dev, "%s\n", __func__);

	pm_runtime_disable(dev);
#ifndef CONFIG_PM
	abox_runtime_suspend(dev);
#endif
	device_init_wakeup(dev, false);
	destroy_workqueue(data->ipc_workqueue);
	snd_soc_unregister_component(dev);
	abox_iommu_unmap(dev, IOVA_DRAM_FIRMWARE);
#ifdef EMULATOR
	iounmap(pmu_alive);
#endif
	return 0;
}

static void samsung_abox_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dev_info(dev, "%s\n", __func__);
	pm_runtime_disable(dev);
}

static const struct of_device_id samsung_abox_match[] = {
	{
		.compatible = "samsung,abox",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_match);

static const struct dev_pm_ops samsung_abox_pm = {
	SET_SYSTEM_SLEEP_PM_OPS(abox_suspend, abox_resume)
	SET_RUNTIME_PM_OPS(abox_runtime_suspend, abox_runtime_resume, NULL)
};

static struct platform_driver samsung_abox_driver = {
	.probe  = samsung_abox_probe,
	.remove = samsung_abox_remove,
	.shutdown = samsung_abox_shutdown,
	.driver = {
		.name = "samsung-abox",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_match),
		.pm = &samsung_abox_pm,
	},
};

module_platform_driver(samsung_abox_driver);

static int __init samsung_abox_late_initcall(void)
{
	pr_info("%s\n", __func__);

	if (p_abox_data && p_abox_data->pdev)
		pm_runtime_put_sync_suspend(&p_abox_data->pdev->dev);
	else
		pr_err("%s: p_abox_data or pdev is null", __func__);

	return 0;
}
late_initcall(samsung_abox_late_initcall);

/* Module information */
MODULE_AUTHOR("Gyeongtaek Lee, <gt82.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung ASoC A-Box Driver");
MODULE_ALIAS("platform:samsung-abox");
MODULE_LICENSE("GPL");
