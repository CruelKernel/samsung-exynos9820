/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/debug-snapshot-helper.h>

#include <asm/map.h>

#include <soc/samsung/acpm_ipc_ctrl.h>
#include <soc/samsung/exynos-sci.h>

const char *exynos_sci_llc_region_name[LLC_REGION_MAX] = {
	[LLC_REGION_DISABLE]			= "LLC_REGION_DISABLE",
	[LLC_REGION_LIT_MID_ALL]		= "LLC_REGION_LIT_MID_ALL",
	[LLC_REGION_CPU_ALL]			= "LLC_REGION_CPU_ALL",
	[LLC_REGION_CPU_2_GPU_2]		= "LLC_REGION_CPU_2_GPU_2",
	[LLC_REGION_BIG_ALL]			= "LLC_REGION_BIG_ALL",
	[LLC_REGION_GPU_ALL]			= "LLC_REGION_GPU_ALL",
	[LLC_REGION_LIT_MID_GPU_SHARE]		= "LLC_REGION_LIT_MID_GPU_SHARE",
	[LLC_REGION_CPU_GPU_SHARE]		= "LLC_REGION_CPU_GPU_SHARE",
	[LLC_REGION_BIG_GPU_SHARE]		= "LLC_REGION_BIG_GPU_SHARE",
};

static struct exynos_sci_data *sci_data;
static void __iomem *dump_base;

static void print_sci_data(struct exynos_sci_data *data)
{
	SCI_DBG("IPC Channel Number: %u\n", data->ipc_ch_num);
	SCI_DBG("IPC Channel Size: %u\n", data->ipc_ch_size);
	SCI_DBG("Use Initial LLC Region: %s\n",
			data->use_init_llc_region ? "True" : "False");
	SCI_DBG("Initial LLC Region: %s (%u)\n",
		exynos_sci_llc_region_name[data->initial_llc_region],
		data->initial_llc_region);
	SCI_DBG("LLC Enable: %s\n",
			data->llc_enable ? "True" : "False");
}

#ifdef CONFIG_OF
static int exynos_sci_parse_dt(struct device_node *np,
				struct exynos_sci_data *data)
{
	int ret;

	if (!np)
		return -ENODEV;

	ret = of_property_read_u32(np, "use_init_llc_region",
					&data->use_init_llc_region);
	if (ret) {
		SCI_ERR("%s: Failed get initial_llc_region\n", __func__);
		return ret;
	}

	if (data->use_init_llc_region) {
		ret = of_property_read_u32(np, "initial_llc_region",
					&data->initial_llc_region);
		if (ret) {
			SCI_ERR("%s: Failed get initial_llc_region\n", __func__);
			return ret;
		}
	}

	ret = of_property_read_u32(np, "llc_enable",
					&data->llc_enable);
	if (ret) {
		SCI_ERR("%s: Failed get llc_enable\n", __func__);
		return ret;
	}

	return 0;
}
#else
static inline
int exynos_sci_parse_dt(struct device_node *np,
				struct exynos_sci_data *data)
{
	return -ENODEV;
}
#endif

static enum exynos_sci_err_code exynos_sci_ipc_err_handle(unsigned int cmd)
{
	enum exynos_sci_err_code err_code;

	err_code = SCI_CMD_GET(cmd, SCI_ERR_MASK, SCI_ERR_SHIFT);
	if (err_code)
		SCI_ERR("%s: SCI IPC error return(%u)\n", __func__, err_code);

	return err_code;
}

static int __exynos_sci_ipc_send_data(enum exynos_sci_cmd_index cmd_index,
				struct exynos_sci_data *data,
				unsigned int *cmd)
{
#ifdef CONFIG_EXYNOS_ACPM
	struct ipc_config config;
	unsigned int *sci_cmd;
#endif
	int ret = 0;

	if (cmd_index >= SCI_CMD_MAX) {
		SCI_ERR("%s: Invalid CMD Index: %u\n", __func__, cmd_index);
		ret = -EINVAL;
		goto out;
	}

#ifdef CONFIG_EXYNOS_ACPM
	sci_cmd = cmd;
	config.cmd = sci_cmd;
	config.response = true;
	config.indirection = false;

	ret = acpm_ipc_send_data(data->ipc_ch_num, &config);
	if (ret) {
		SCI_ERR("%s: Failed to send IPC(%d:%u) data\n",
			__func__, cmd_index, data->ipc_ch_num);
		goto out;
	}
#endif

out:
	return ret;
}

static int exynos_sci_ipc_send_data(enum exynos_sci_cmd_index cmd_index,
				struct exynos_sci_data *data,
				unsigned int *cmd)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&data->lock, flags);
	ret = __exynos_sci_ipc_send_data(cmd_index, data, cmd);
	spin_unlock_irqrestore(&data->lock, flags);

	return ret;
}

static void exynos_sci_base_cmd(struct exynos_sci_cmd_info *cmd_info,
					unsigned int *cmd)
{
	cmd[0] |= SCI_CMD_SET(cmd_info->cmd_index,
				SCI_CMD_IDX_MASK, SCI_CMD_IDX_SHIFT);
	cmd[0] |= SCI_CMD_SET(cmd_info->direction,
				SCI_ONE_BIT_MASK, SCI_IPC_DIR_SHIFT);
	cmd[0] |= SCI_CMD_SET(cmd_info->data, SCI_DATA_MASK, SCI_DATA_SHIFT);
}

static int exynos_sci_llc_invalidate(struct exynos_sci_data *data)
{
	struct exynos_sci_cmd_info cmd_info;
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (data->llc_region_prio[LLC_REGION_DISABLE])
		goto out;

	cmd_info.cmd_index = SCI_LLC_INVAL;
	cmd_info.direction = 0;
	cmd_info.data = 0;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

out:
	return ret;
}

static int exynos_sci_llc_flush(struct exynos_sci_data *data)
{
	struct exynos_sci_cmd_info cmd_info;
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (data->llc_region_prio[LLC_REGION_DISABLE])
		goto out;

	cmd_info.cmd_index = SCI_LLC_FLUSH;
	cmd_info.direction = 0;
	cmd_info.data = 0;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

out:
	return ret;
}

static int exynos_sci_llc_region_alloc(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *region_index, bool on)
{
	struct exynos_sci_cmd_info cmd_info;
	unsigned int cmd[4] = {0, 0, 0, 0};
	unsigned int i;
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (direction == SCI_IPC_SET) {
		if (*region_index >= LLC_REGION_MAX) {
			SCI_ERR("%s: Invalid Region Index: %u\n", __func__, *region_index);
			ret = -EINVAL;
			goto out;
		}

		if (*region_index > LLC_REGION_DISABLE) {
			if (on)
				data->llc_region_prio[*region_index] = 1;
			else
				data->llc_region_prio[*region_index] = 0;

			for (i = LLC_REGION_DISABLE + 1; i < LLC_REGION_MAX; i++) {
				if (data->llc_region_prio[i])
					*region_index = i;
			}
		}

		if (data->llc_region_prio[LLC_REGION_DISABLE])
			goto out;
	}

	cmd_info.cmd_index = SCI_LLC_REGION_ALLOC;
	cmd_info.direction = direction;
	cmd_info.data = *region_index;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*region_index = SCI_CMD_GET(cmd[1], SCI_DATA_MASK, SCI_DATA_SHIFT);

out:
	return ret;
}

static int exynos_sci_llc_enable(struct exynos_sci_data *data,
					enum exynos_sci_ipc_dir direction,
					unsigned int *enable)
{
	struct exynos_sci_cmd_info cmd_info;
	unsigned int cmd[4] = {0, 0, 0, 0};
	int ret = 0;
	enum exynos_sci_err_code ipc_err;

	if (direction == SCI_IPC_SET) {
		if (*enable > 1) {
			SCI_ERR("%s: Invalid Control Index: %u\n", __func__, *enable);
			ret = -EINVAL;
			goto out;
		}

		if (*enable)
			data->llc_region_prio[LLC_REGION_DISABLE] = 0;
		else
			data->llc_region_prio[LLC_REGION_DISABLE] = 1;
	}

	cmd_info.cmd_index = SCI_LLC_EN;
	cmd_info.direction = direction;
	cmd_info.data = *enable;

	exynos_sci_base_cmd(&cmd_info, cmd);

	/* send command for SCI */
	ret = exynos_sci_ipc_send_data(cmd_info.cmd_index, data, cmd);
	if (ret) {
		SCI_ERR("%s: Failed send data\n", __func__);
		goto out;
	}

	ipc_err = exynos_sci_ipc_err_handle(cmd[1]);
	if (ipc_err) {
		ret = -EBADMSG;
		goto out;
	}

	if (direction == SCI_IPC_GET)
		*enable = SCI_CMD_GET(cmd[1], SCI_DATA_MASK, SCI_DATA_SHIFT);

out:
	return ret;
}

static void exynos_sci_llc_dump_to_memory(struct exynos_sci_data *data,
				struct exynos_llc_dump_fmt *llc_dump_fmt)
{
	u32 *dump_data = (u32 *)llc_dump_fmt;
	u32 entry_size = sizeof(struct exynos_llc_dump_fmt);
	u32 write_count = entry_size / 4;
	int i;

	for (i = 0; i < write_count; i++)
		__raw_writel(dump_data[i], dump_base + (i * 0x4));

	dump_base += entry_size;
}

static int exynos_sci_llc_dump(struct exynos_sci_data *data)
{
	struct exynos_llc_dump_fmt llc_dump_fmt;
	u32 clk_gating_save, disable_llc_save;
	u32 tmp;
	u16 slice, bank, set, way, qword;
	u32 arr_dbg_rd = 1, arr_dbg_sel = 1, llc_arr_dbg_sel;
	u32 arr_dbg_cntl;
	char *dump_sign = "LLCD";
	u32 *char_data = (u32 *)dump_sign;

	/* disable LLC clock gating */
	tmp = __raw_readl(data->sci_base + PM_SCI_CTL);
	clk_gating_save = (tmp >> LLC_En_Bit) & 0x1;
	tmp = tmp & ~(0x1 << LLC_En_Bit);
	__raw_writel(tmp, data->sci_base + PM_SCI_CTL);

	/* disable LLC */
	tmp = __raw_readl(data->sci_base + CCMControl1);
	disable_llc_save = (tmp >> DisableLlc_Bit) & 0x1;
	tmp = tmp | (0x1 << DisableLlc_Bit);
	__raw_writel(tmp, data->sci_base + CCMControl1);

	__raw_writel(char_data[0], dump_base);
	dump_base += 0x4;

	for (slice = 0; slice <= LLC_SLICE_END; slice++) {
		llc_dump_fmt.slice = slice;
		for (bank = 0; bank <= LLC_BANK_END; bank++) {
			llc_dump_fmt.bank = bank;
			for (set = 0; set <= LLC_SET_END; set++) {
				llc_dump_fmt.set = set;
				for (way = 0; way <= LLC_WAY_END; way++) {
					llc_dump_fmt.way = way;
					llc_arr_dbg_sel = 1;
					arr_dbg_cntl = 0;
					arr_dbg_cntl = ((arr_dbg_rd << 30) | (arr_dbg_sel << 27) |
							(llc_arr_dbg_sel << 23) | (slice << 20) |
							(bank << 19) | (set << 9) | (way << 4));
					__raw_writel(arr_dbg_cntl, data->sci_base + ArrDbgCntl);

					do {
						arr_dbg_cntl = __raw_readl(data->sci_base + ArrDbgCntl);
					} while (((arr_dbg_cntl >> 30) & 0x1) == 0x1);

					llc_dump_fmt.tag.data_lo =
						__raw_readl(data->sci_base + ArrDbgRDataLo);
					llc_dump_fmt.tag.data_mi =
						__raw_readl(data->sci_base + ArrDbgRDataMi);
					llc_dump_fmt.tag.data_hi =
						__raw_readl(data->sci_base + ArrDbgRDataHi);

					for (qword = 0; qword <= LLC_QWORD_END; qword++) {
						llc_arr_dbg_sel = 4;
						arr_dbg_cntl = 0;
						arr_dbg_cntl = ((arr_dbg_rd << 30) | (arr_dbg_sel << 27) |
								(llc_arr_dbg_sel << 23) | (slice << 20) |
								(bank << 19) | (set << 9) | (way << 4) |
								(qword << 0));
						__raw_writel(arr_dbg_cntl, data->sci_base + ArrDbgCntl);

						do {
							arr_dbg_cntl = __raw_readl(data->sci_base + ArrDbgCntl);
						} while (((arr_dbg_cntl >> 30) & 0x1) == 0x1);

						llc_dump_fmt.data_q[qword].data_lo =
							__raw_readl(data->sci_base + ArrDbgRDataLo);
						llc_dump_fmt.data_q[qword].data_mi =
							__raw_readl(data->sci_base + ArrDbgRDataMi);
						llc_dump_fmt.data_q[qword].data_hi =
							__raw_readl(data->sci_base + ArrDbgRDataHi);
					}

					exynos_sci_llc_dump_to_memory(data, &llc_dump_fmt);
				}
			}
		}
	}

	dump_base = data->llc_dump_addr.v_addr;

	/* restore LLC */
	tmp = __raw_readl(data->sci_base + CCMControl1);
	tmp = tmp & ~(0x1 << DisableLlc_Bit);
	tmp = tmp | (disable_llc_save << DisableLlc_Bit);
	__raw_writel(tmp, data->sci_base + CCMControl1);

	/* restore LLC clock gating */
	tmp = __raw_readl(data->sci_base + PM_SCI_CTL);
	tmp = tmp & ~(0x1 << LLC_En_Bit);
	tmp = tmp | (clk_gating_save << LLC_En_Bit);
	__raw_writel(tmp, data->sci_base + PM_SCI_CTL);

	return 0;
}

/* Export Functions */
void llc_invalidate(void)
{
	int ret;

	ret = exynos_sci_llc_invalidate(sci_data);
	if (ret)
		SCI_ERR("%s: Failed llc invalidate\n", __func__);

	return;
}
EXPORT_SYMBOL(llc_invalidate);

void llc_flush(void)
{
	int ret;

	ret = exynos_sci_llc_flush(sci_data);
	if (ret)
		SCI_ERR("%s: Failed llc flush\n", __func__);

	return;
}
EXPORT_SYMBOL(llc_flush);

void llc_region_alloc(unsigned int region_index, bool on)
{
	int ret;

	ret = exynos_sci_llc_region_alloc(sci_data, SCI_IPC_SET, &region_index, on);
	if (ret)
		SCI_ERR("%s: Failed llc region allocate\n", __func__);

	return;
}
EXPORT_SYMBOL(llc_region_alloc);

void llc_dump(void)
{
	SCI_INFO("%s: llc_dump start!!\n", __func__);

	exynos_sci_llc_dump(sci_data);

	SCI_INFO("%s: llc_dump end!!\n", __func__);

	return;
}
EXPORT_SYMBOL(llc_dump);

#define SCI_ErrStatHi	0x0114
#define SCI_ErrStatLo	0x0118
#define SCI_ErrCnt	0x0108
#define SCI_ErrAddrHi	0x010C
#define SCI_ErrAddrLo	0x0110

/* for SCI_ErrCnt */
static struct err_variant sci_err_type_1[] = {
	ERR_VAR("MaxOtherCount", 31, 24),
	ERR_VAR("MaxCount", 23, 16),
	ERR_VAR("ClrOtherCount", 15, 15),
	ERR_VAR("ClrCount", 14, 14),
	ERR_VAR("RspErrEn", 13, 13),
	ERR_VAR("ProtErrEn", 12, 12),
	ERR_VAR("MemArrayErrEn", 11, 11),
	ERR_VAR("SfArrayErrEn", 10, 10),
	ERR_VAR("LlcArrayErrEn", 9, 9),
	ERR_VAR("FatalUnContErrEn", 8, 8),
	ERR_VAR("FatalUcErrEn", 7, 7),
	ERR_VAR("CountCorrErrEn", 6, 6),
	ERR_VAR("CountUnCorrErrEn", 5, 5),
	ERR_VAR("IntOtherCountErrEn", 4, 4),
	ERR_VAR("IntCountErrEn", 3, 3),
	ERR_VAR("IntCorrErrEn", 2, 2),
	ERR_VAR("IntUnCorrErrEn", 1, 1),
	ERR_VAR("LogErrEn", 0, 0),
	ERR_VAR("END", 64, 64),
};

/* for SCI_ErrStatLo */
static struct err_variant sci_err_type_2[] = {
	ERR_VAR("SType", 31, 16),
	ERR_VAR("ErrType", 15, 12),
	ERR_VAR("MultiErrorValid", 5, 5),
	ERR_VAR("SynValid", 4, 4),
	ERR_VAR("AddrValid", 3, 3),
	ERR_VAR("UnContained", 2, 2),
	ERR_VAR("UC", 1, 1),
	ERR_VAR("Vaild", 0, 0),
	ERR_VAR("END", 64, 64),
};

/* for SCI_ErrStatHi */
static struct err_variant sci_err_type_3[] = {
	ERR_VAR("OtherCount", 31, 24),
	ERR_VAR("Count", 23, 16),
	ERR_VAR("ErrSyn", 8, 0),
	ERR_VAR("END", 64, 64),
};

enum {
	SCI_ERRCNT = 0,
	SCI_ERRSTATLO,
	SCI_ERRSTATHI
};

static struct err_variant_data exynos_sci_err_table[] = {
	ERR_REG(sci_err_type_1, 0, "SCI_ErrCnt"),
	ERR_REG(sci_err_type_2, 0, "SCI_ErrStatLo"),
	ERR_REG(sci_err_type_3, 0xff, "SCI_ErrStatHi"),
};

static void exynos_sci_err_parse(u32 reg_idx, u64 reg)
{
	if (reg_idx >= ARRAY_SIZE(exynos_sci_err_table)) {
		pr_err("%s: there is no parse data\n", __func__);
		return;
	}

	exynos_err_parse(reg_idx, reg, &exynos_sci_err_table[reg_idx]);
}

#define MSB_MASKING		(0x0000FF0000000000)
#define MSB_PADDING		(0xFFFFFF0000000000)
#define ERR_INJ_DONE		(1 << 31)
#define ERR_NS			(1 << 8)

extern void exynos_dump_common_cpu_reg(void);

void sci_error_dump(void)
{
	void __iomem *sci_base;
	unsigned int sci_reg, sci_ns, sci_err_inj;
	unsigned long sci_reg_addr, sci_reg_hi;

	sci_base = sci_data->sci_base;

	pr_info("============== SCI Error Logging Info=====================\n");
	pr_info("SCI_ErrCnt    : %08x\n", sci_reg = __raw_readl(sci_base + SCI_ErrCnt));
	exynos_sci_err_parse(SCI_ERRCNT, sci_reg);
	pr_info("SCI_ErrStatHi : %08x\n", sci_reg = __raw_readl(sci_base + SCI_ErrStatHi));
	exynos_sci_err_parse(SCI_ERRSTATHI, sci_reg);
	pr_info("SCI_ErrStatLo : %08x\n", sci_reg = __raw_readl(sci_base + SCI_ErrStatLo));
	exynos_sci_err_parse(SCI_ERRSTATLO, sci_reg);
	pr_info("SCI_ErrAddr(Hi,Lo): %08x %08x\n",
			sci_reg_hi = __raw_readl(sci_base + SCI_ErrAddrHi),
			sci_reg = __raw_readl(sci_base + SCI_ErrAddrLo));

	sci_reg_addr = sci_reg + (MSB_MASKING & (sci_reg_hi << 32L));
	sci_ns = (ERR_NS & sci_reg_hi) >> 8;
	sci_err_inj = (ERR_INJ_DONE & sci_reg_hi) >> 31;
	pr_info("SCI_ErrAddr : %016lx (NS:%d, ERR_INJ:%d)\n", sci_reg_addr, sci_err_inj);
	exynos_dump_common_cpu_reg();
	pr_info("============================================================\n");
}
EXPORT_SYMBOL(sci_error_dump);

/* SYSFS Interface */
static ssize_t show_sci_data(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	int i;

	count += snprintf(buf + count, PAGE_SIZE, "IPC Channel Number: %u\n",
				data->ipc_ch_num);
	count += snprintf(buf + count, PAGE_SIZE, "IPC Channel Size: %u\n",
				data->ipc_ch_size);
	count += snprintf(buf + count, PAGE_SIZE, "Use Initial LLC Region: %s\n",
				data->use_init_llc_region ? "True" : "False");
	count += snprintf(buf + count, PAGE_SIZE, "Initial LLC Region: %s (%u)\n",
				exynos_sci_llc_region_name[data->initial_llc_region],
				data->initial_llc_region);
	count += snprintf(buf + count, PAGE_SIZE, "LLC Enable: %s\n",
				data->llc_enable ? "True" : "False");
	count += snprintf(buf + count, PAGE_SIZE, "Plugin Initial LLC Region: %s (%u)\n",
				exynos_sci_llc_region_name[data->plugin_init_llc_region],
				data->plugin_init_llc_region);
	count += snprintf(buf + count, PAGE_SIZE, "LLC Region Priority:\n");
	count += snprintf(buf + count, PAGE_SIZE, "prio   region                  on\n");
	for (i = LLC_REGION_DISABLE + 1; i < LLC_REGION_MAX; i++)
		count += snprintf(buf + count, PAGE_SIZE, "%2d     %s  %u\n",
				i, exynos_sci_llc_region_name[i], data->llc_region_prio[i]);

	return count;
}

static ssize_t store_llc_invalidate(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int invalidate;
	int ret;

	ret = sscanf(buf, "%u",	&invalidate);
	if (ret != 1)
		return -EINVAL;

	if (invalidate != 1) {
		SCI_ERR("%s: Invalid parameter: %u, should be set 1\n",
				__func__, invalidate);
		return -EINVAL;
	}

	ret = exynos_sci_llc_invalidate(data);
	if (ret) {
		SCI_ERR("%s: Failed llc invalidate\n", __func__);
		return ret;
	}

	return count;
}

static ssize_t store_llc_flush(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int flush;
	int ret;

	ret = sscanf(buf, "%u",	&flush);
	if (ret != 1)
		return -EINVAL;

	if (flush != 1) {
		SCI_ERR("%s: Invalid parameter: %u, should be set 1\n",
				__func__, flush);
		return -EINVAL;
	}

	ret = exynos_sci_llc_flush(data);
	if (ret) {
		SCI_ERR("%s: Failed llc flush\n", __func__);
		return ret;
	}

	return count;
}

static ssize_t show_llc_region_alloc(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int region_index;
	int ret;

	ret = exynos_sci_llc_region_alloc(data, SCI_IPC_GET, &region_index, 0);
	if (ret) {
		count += snprintf(buf + count, PAGE_SIZE,
				"Failed llc region allocate state\n");
		return count;
	}

	count += snprintf(buf + count, PAGE_SIZE, "LLC Region: %s (%u)\n",
			exynos_sci_llc_region_name[region_index], region_index);

	return count;
}

static ssize_t store_llc_region_alloc(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int region_index, on;
	int ret;

	ret = sscanf(buf, "%u %u", &region_index, &on);
	if (ret != 2) {
		SCI_ERR("%s: usage: echo [region_index] [on] > llc_region_alloc\n",
				__func__);
		return -EINVAL;
	}

	ret = exynos_sci_llc_region_alloc(data, SCI_IPC_SET, &region_index, (bool)on);
	if (ret) {
		SCI_ERR("%s: Failed llc region allocate\n", __func__);
		return ret;
	}

	return count;
}

static ssize_t show_llc_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;
	unsigned int enable;
	int ret;

	ret = exynos_sci_llc_enable(data, SCI_IPC_GET, &enable);
	if (ret) {
		count += snprintf(buf + count, PAGE_SIZE,
				"Failed llc enable state\n");
		return count;
	}

	count += snprintf(buf + count, PAGE_SIZE, "LLC Enable: %s (%d)\n",
			enable ? "enable" : "disable", enable);

	return count;
}

static ssize_t store_llc_enable(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	unsigned int enable;
	int ret;

	ret = sscanf(buf, "%u",	&enable);
	if (ret != 1)
		return -EINVAL;

	ret = exynos_sci_llc_enable(data, SCI_IPC_SET, &enable);
	if (ret) {
		SCI_ERR("%s: Failed llc enable control\n", __func__);
		return ret;
	}

	return count;
}

static ssize_t store_llc_dump_test(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	unsigned int dump;
	int ret;

	ret = sscanf(buf, "%u",	&dump);
	if (ret != 1)
		return -EINVAL;

	if (dump == 0)
		return -EINVAL;

	panic("LLC Dump Test!!!!\n");

	return count;
}

static ssize_t show_llc_dump_addr_info(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev,
					struct platform_device, dev);
	struct exynos_sci_data *data = platform_get_drvdata(pdev);
	ssize_t count = 0;

	count += snprintf(buf + count, PAGE_SIZE, "\n= LLC dump address info =\n");
	count += snprintf(buf + count, PAGE_SIZE,
			"physical address = 0x%08x\n", data->llc_dump_addr.p_addr);
	count += snprintf(buf + count, PAGE_SIZE,
			"virtual address = 0x%p\n", data->llc_dump_addr.v_addr);
	count += snprintf(buf + count, PAGE_SIZE,
			"dump region size = 0x%08x\n", data->llc_dump_addr.p_size);
	count += snprintf(buf + count, PAGE_SIZE,
			"dump buffer size = 0x%08x\n", data->llc_dump_addr.buff_size);

	return count;
}

static DEVICE_ATTR(sci_data, 0440, show_sci_data, NULL);
static DEVICE_ATTR(llc_invalidate, 0640, NULL, store_llc_invalidate);
static DEVICE_ATTR(llc_flush, 0640, NULL, store_llc_flush);
static DEVICE_ATTR(llc_region_alloc, 0640, show_llc_region_alloc, store_llc_region_alloc);
static DEVICE_ATTR(llc_enable, 0640, show_llc_enable, store_llc_enable);
static DEVICE_ATTR(llc_dump_test, 0640, NULL, store_llc_dump_test);
static DEVICE_ATTR(llc_dump_addr_info, 0440, show_llc_dump_addr_info, NULL);

static struct attribute *exynos_sci_sysfs_entries[] = {
	&dev_attr_sci_data.attr,
	&dev_attr_llc_invalidate.attr,
	&dev_attr_llc_flush.attr,
	&dev_attr_llc_region_alloc.attr,
	&dev_attr_llc_enable.attr,
	&dev_attr_llc_dump_test.attr,
	&dev_attr_llc_dump_addr_info.attr,
	NULL,
};

static struct attribute_group exynos_sci_attr_group = {
	.name	= "sci_attr",
	.attrs	= exynos_sci_sysfs_entries,
};

static void exynos_sci_llc_dump_config(struct exynos_sci_data *data)
{
	data->llc_dump_addr.p_addr = dbg_snapshot_get_item_paddr(LLC_DSS_NAME);
	data->llc_dump_addr.p_size = dbg_snapshot_get_item_size(LLC_DSS_NAME);
	data->llc_dump_addr.v_addr =
		(void __iomem *)dbg_snapshot_get_item_vaddr(LLC_DSS_NAME);
	data->llc_dump_addr.buff_size = data->llc_dump_addr.p_size;
	dump_base = data->llc_dump_addr.v_addr;
}

static int __init exynos_sci_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct exynos_sci_data *data;

	data = kzalloc(sizeof(struct exynos_sci_data), GFP_KERNEL);
	if (data == NULL) {
		SCI_ERR("%s: failed to allocate SCI device\n", __func__);
		ret = -ENOMEM;
		goto err_data;
	}

	sci_data = data;
	data->dev = &pdev->dev;

	spin_lock_init(&data->lock);

#ifdef CONFIG_EXYNOS_ACPM
	/* acpm_ipc_request_channel */
	ret = acpm_ipc_request_channel(data->dev->of_node, NULL,
				&data->ipc_ch_num, &data->ipc_ch_size);
	if (ret) {
		SCI_ERR("%s: acpm request channel is failed, ipc_ch: %u, size: %u\n",
				__func__, data->ipc_ch_num, data->ipc_ch_size);
		goto err_acpm;
	}
#endif

	/* parsing dts data for SCI */
	ret = exynos_sci_parse_dt(data->dev->of_node, data);
	if (ret) {
		SCI_ERR("%s: failed to parse private data\n", __func__);
		goto err_parse_dt;
	}

	ret = exynos_sci_llc_region_alloc(data, SCI_IPC_GET,
					&data->plugin_init_llc_region, 0);
	if (ret) {
		SCI_ERR("%s: Falied get plugin initial llc region\n", __func__);
		goto err_plug_llc_region;
	}

	data->llc_region_prio[data->plugin_init_llc_region] = 1;

	if (data->use_init_llc_region) {
		ret = exynos_sci_llc_region_alloc(data, SCI_IPC_SET,
						&data->initial_llc_region, true);
		if (ret) {
			SCI_ERR("%s: Failed llc region allocate\n", __func__);
			goto err_llc_region;
		}
	}

	if (data->llc_enable) {
		ret = exynos_sci_llc_enable(data, SCI_IPC_SET, &data->llc_enable);
		if (ret) {
			SCI_ERR("%s: Failed llc enable control\n", __func__);
			goto err_llc_disable;
		}
	}

	data->sci_base = ioremap(SCI_BASE, SZ_4K);
	if (IS_ERR(data->sci_base)) {
		SCI_ERR("%s: Failed SCI base remap\n", __func__);
		goto err_ioremap;
	}

	exynos_sci_llc_dump_config(data);

	platform_set_drvdata(pdev, data);

	ret = sysfs_create_group(&data->dev->kobj, &exynos_sci_attr_group);
	if (ret)
		SCI_ERR("%s: failed creat sysfs for Exynos SCI\n", __func__);

	print_sci_data(data);

	SCI_INFO("%s: exynos sci is initialized!!\n", __func__);
 
	return 0;

err_ioremap:
err_llc_disable:
err_llc_region:
err_plug_llc_region:
err_parse_dt:
#ifdef CONFIG_EXYNOS_ACPM
	acpm_ipc_release_channel(data->dev->of_node, data->ipc_ch_num);
err_acpm:
#endif
	kfree(data);

err_data:
	return ret;
}

static int exynos_sci_remove(struct platform_device *pdev)
{
	struct exynos_sci_data *data = platform_get_drvdata(pdev);

	sysfs_remove_group(&data->dev->kobj, &exynos_sci_attr_group);
	platform_set_drvdata(pdev, NULL);
	iounmap(data->sci_base);
#ifdef CONFIG_EXYNOS_ACPM
	acpm_ipc_release_channel(data->dev->of_node, data->ipc_ch_num);
#endif
	kfree(data);

	SCI_INFO("%s: exynos sci is removed!!\n", __func__);

	return 0;
}

static struct platform_device_id exynos_sci_driver_ids[] = {
	{ .name = EXYNOS_SCI_MODULE_NAME, },
	{},
};
MODULE_DEVICE_TABLE(platform, exynos_sci_driver_ids);

static const struct of_device_id exynos_sci_match[] = {
	{ .compatible = "samsung,exynos-sci", },
	{},
};

static struct platform_driver exynos_sci_driver = {
	.remove = exynos_sci_remove,
	.id_table = exynos_sci_driver_ids,
	.driver = {
		.name = EXYNOS_SCI_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = exynos_sci_match,
	},
};

module_platform_driver_probe(exynos_sci_driver, exynos_sci_probe);

MODULE_AUTHOR("Taekki Kim <taekki.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung SCI Interface driver");
MODULE_LICENSE("GPL");
