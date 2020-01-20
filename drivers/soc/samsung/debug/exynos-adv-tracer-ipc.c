/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/debug-snapshot.h>
#include <linux/ktime.h>

#include <soc/samsung/exynos-adv-tracer.h>
#include <soc/samsung/exynos-adv-tracer-ipc.h>
#include <soc/samsung/exynos-pmu.h>

#define EXYNOS_DBGCORE_CPU_CONFIGURATION	(0x2e80)
#define EXYNOS_DBGCORE_CPU_STATUS		(0x2e84)

static struct adv_tracer_ipc_main *adv_tracer_ipc;

static void adv_tracer_ipc_read_buffer(void *dest, const void *src, unsigned int len)
{
	const unsigned int *sp = src;
	unsigned int *dp = dest;
	int i;

	for (i = 0; i < len; i++)
		*dp++ = readl(sp++);
}

static void adv_tracer_ipc_write_buffer(void *dest, const void *src, unsigned int len)
{
	const unsigned int *sp = src;
	unsigned int *dp = dest;
	int i;

	for (i = 0; i < len; i++)
		writel(*sp++, dp++);
}

static void adv_tracer_ipc_interrupt_clear(int id)
{
	__raw_writel((1 << INTR_FLAG_OFFSET) << id, adv_tracer_ipc->mailbox_base + AP_INTCR);
}

static irqreturn_t adv_tracer_ipc_irq_handler(int irq, void *data)
{
	struct adv_tracer_ipc_main *ipc = data;
	struct adv_tracer_ipc_cmd *cmd;
	struct adv_tracer_ipc_ch *channel;
	unsigned int status;
	int i;

	/* ADV_TRACER IPC INTERRUPT STATUS REGISTER */
	status = __raw_readl(adv_tracer_ipc->mailbox_base + AP_INTSR);
	status = status >> INTR_FLAG_OFFSET;

	for (i = 0; i < adv_tracer_ipc->num_channels; i++) {
		channel = &adv_tracer_ipc->channel[i];
		if (status & (0x1 << channel->id)) {
			cmd = channel->cmd;
			adv_tracer_ipc_read_buffer(cmd->buffer, channel->buff_regs, channel->len);
			/* ADV_TRACER IPC INTERRUPT PENDING CLEAR */
			adv_tracer_ipc_interrupt_clear(channel->id);
		}
	}
	ipc->mailbox_status = status;
	return IRQ_WAKE_THREAD;
}

static irqreturn_t adv_tracer_ipc_irq_handler_thread(int irq, void *data)
{
	struct adv_tracer_ipc_main *ipc = data;
	struct adv_tracer_ipc_cmd *cmd;
	struct adv_tracer_ipc_ch *channel;
	unsigned int status;
	int i;

	status = ipc->mailbox_status;

	for (i = 0; i < adv_tracer_ipc->num_channels; i++) {
		channel = &adv_tracer_ipc->channel[i];
		if (status & (1 << i)) {
			cmd = channel->cmd;
			if (cmd->cmd_raw.response)
				complete(&channel->wait);

			if (cmd->cmd_raw.one_way || cmd->cmd_raw.response) {
				/* HANDLE to Plugin handler */
				if (channel->ipc_callback)
					channel->ipc_callback(cmd, channel->len);
			}
		}
	}
	return IRQ_HANDLED;
}

static void adv_tracer_interrupt_gen(unsigned int id)
{
	/* APM NVIC INTERRUPT GENERATE */
	writel(1 << id, adv_tracer_ipc->mailbox_base + INTGR_AP_TO_DBGC);
}

int adv_tracer_ipc_send_data(unsigned int id, struct adv_tracer_ipc_cmd *cmd)
{
	struct adv_tracer_ipc_ch *channel = NULL;
	int ret;

	if (id >= EAT_MAX_CHANNEL && IS_ERR(cmd))
		return -EIO;

	channel = &adv_tracer_ipc->channel[id];
	cmd->cmd_raw.manual_polling = 0;
	cmd->cmd_raw.one_way = 0;
	cmd->cmd_raw.response = 0;

	spin_lock(&channel->ch_lock);
	memcpy(channel->cmd, cmd, sizeof(unsigned int) * channel->len);
	adv_tracer_ipc_write_buffer(channel->buff_regs, channel->cmd, channel->len);
	adv_tracer_interrupt_gen(channel->id);
	spin_unlock(&channel->ch_lock);

	ret = wait_for_completion_interruptible_timeout(&channel->wait, msecs_to_jiffies(50));
	if (!ret) {
		pr_err("%s: %d channel timeout error\n", __func__, id);
		return -EBUSY;
	}
	memcpy(cmd, channel->cmd, sizeof(unsigned int) * channel->len);
	return 0;
}

int adv_tracer_ipc_send_data_polling_timeout(unsigned int id, struct adv_tracer_ipc_cmd *cmd,
					unsigned long timeout_ns)
{
	struct adv_tracer_ipc_ch *channel = NULL;
	ktime_t timeout, now;
	struct adv_tracer_ipc_cmd _cmd;

	if (id >= EAT_MAX_CHANNEL && IS_ERR(cmd))
		return -EIO;

	channel = &adv_tracer_ipc->channel[id];
	cmd->cmd_raw.manual_polling = 1;
	cmd->cmd_raw.one_way = 0;
	cmd->cmd_raw.response = 0;

	spin_lock(&channel->ch_lock);
	memcpy(channel->cmd, cmd, sizeof(unsigned int) * channel->len);
	adv_tracer_ipc_write_buffer(channel->buff_regs, channel->cmd, channel->len);
	adv_tracer_interrupt_gen(channel->id);
	spin_unlock(&channel->ch_lock);

	timeout = ktime_add_ns(ktime_get(), timeout_ns);
	do {
		now = ktime_get();
		adv_tracer_ipc_read_buffer(&_cmd.buffer[0], channel->buff_regs, 1);
	} while (!(_cmd.cmd_raw.response || ktime_after(now, timeout)));

	if (!_cmd.cmd_raw.response) {
		pr_err("%s: %d channel timeout error\n", __func__, id);
		return -EBUSY;
	}
	adv_tracer_ipc_read_buffer(cmd->buffer, channel->buff_regs, channel->len);
	return 0;
}

int adv_tracer_ipc_send_data_polling(unsigned int id, struct adv_tracer_ipc_cmd *cmd)
{
	return adv_tracer_ipc_send_data_polling_timeout(id, cmd, EAT_IPC_TIMEOUT);
}

int adv_tracer_ipc_send_data_async(unsigned int id, struct adv_tracer_ipc_cmd *cmd)
{
	struct adv_tracer_ipc_ch *channel = NULL;

	if (id >= EAT_MAX_CHANNEL && IS_ERR(cmd))
		return -EIO;

	channel = &adv_tracer_ipc->channel[id];
	cmd->cmd_raw.manual_polling = 0;
	cmd->cmd_raw.one_way = 1;
	cmd->cmd_raw.response = 0;

	spin_lock(&channel->ch_lock);
	memcpy(channel->cmd, cmd, sizeof(unsigned int) * channel->len);
	adv_tracer_ipc_write_buffer(channel->buff_regs, channel->cmd, channel->len);
	adv_tracer_interrupt_gen(channel->id);
	spin_unlock(&channel->ch_lock);

	return 0;
}

struct adv_tracer_ipc_cmd *adv_tracer_ipc_get_channel_cmd(unsigned int id)
{
	struct adv_tracer_ipc_ch *channel = NULL;
	struct adv_tracer_ipc_cmd *cmd = NULL;

	if (IS_ERR(adv_tracer_ipc))
		goto out;

	channel = &adv_tracer_ipc->channel[id];
	if (IS_ERR(channel))
		goto out;

	if (!channel->used)
		goto out;

	cmd = channel->cmd;
out:
	return cmd;
}

struct adv_tracer_ipc_ch *adv_tracer_ipc_get_channel(unsigned int id)
{
	struct adv_tracer_ipc_ch *ipc_channel = NULL;

	if (IS_ERR(adv_tracer_ipc))
		goto out;

	ipc_channel = &adv_tracer_ipc->channel[id];
	if (IS_ERR(ipc_channel)) {
		pr_err("%s: %d channel is not allocated\n", __func__, id);
		ipc_channel = NULL;
	}
out:
	return ipc_channel;
}

static int adv_tracer_ipc_channel_init(unsigned int id,
				       unsigned int offset,
				       unsigned int len,
				       ipc_callback handler,
				       char *name)
{
	struct adv_tracer_ipc_ch *channel = &adv_tracer_ipc->channel[id];

	if (!channel->used) {
		channel->id = id;
		channel->offset = offset;
		channel->len = len;
		strncpy(channel->id_name, name, sizeof(unsigned int) - 1);

		/* channel->buff_regs -> shared buffer by owns */
		channel->buff_regs = (void __iomem *)(adv_tracer_ipc->mailbox_base + offset);
		channel->cmd = devm_kzalloc(adv_tracer_ipc->dev,
						sizeof(struct adv_tracer_ipc_cmd), GFP_KERNEL);
		channel->ipc_callback = handler;

		if (IS_ERR(channel->cmd))
			return PTR_ERR(channel->cmd);

		channel->used = true;
	} else {
		pr_err("[EAT] %d channel is already reserved\n", id);
		return -1;
	}
	return 0;
}

int adv_tracer_ipc_release_channel(unsigned int id)
{
	struct adv_tracer_ipc_cmd *cmd;
	int ret;

	if (!adv_tracer_ipc->channel[id].used) {
		pr_err("%s: %d channel is unsed\n", __func__, id);
		return -1;
	}

	cmd = adv_tracer_ipc_get_channel_cmd(EAT_FRM_CHANNEL);
	if (!cmd) {
		pr_err("%s: %d channel is failed to release\n", __func__, id);
		return -EIO;
	}

	cmd->cmd_raw.cmd = EAT_IPC_CMD_CH_RELEASE;
	cmd->buffer[1] = id;

	ret = adv_tracer_ipc_send_data(EAT_FRM_CHANNEL, cmd);
	if (ret < 0) {
		pr_err("%s: %d channel is failed to release\n", __func__, id);
		return ret;
	}

	return 0;
}

void adv_tracer_ipc_release_channel_by_name(const char *name)
{
	struct adv_tracer_ipc_ch *ipc_ch = adv_tracer_ipc->channel;
	int i;

	for (i = 0; i < adv_tracer_ipc->num_channels; i++) {
		if (ipc_ch[i].used) {
			if (!strncmp(name, ipc_ch[i].id_name, strlen(name))) {
				ipc_ch[i].used = 0;
				break;
			}
		}
	}
}

int adv_tracer_ipc_request_channel(struct device_node *np,
				   ipc_callback handler,
				   unsigned int *id,
				   unsigned int *len)
{
	const __be32 *prop;
	const char *plugin_name;
	unsigned int plugin_len, offset;
	struct adv_tracer_ipc_cmd cmd;
	int ret;

	if (!np)
		return -ENODEV;

	prop = of_get_property(np, "plugin-len", &plugin_len);
	if (!prop)
		return -ENOENT;

	plugin_len = be32_to_cpup(prop);

	plugin_name = of_get_property(np, "plugin-name", NULL);
	if (!plugin_name)
		return -ENOENT;

	cmd.cmd_raw.cmd = EAT_IPC_CMD_CH_INIT;
	cmd.buffer[1] = plugin_len;
	memcpy(&cmd.buffer[2], plugin_name, sizeof(unsigned int));

	ret = adv_tracer_ipc_send_data_polling(EAT_FRM_CHANNEL, &cmd);
	if (ret) {
		pr_err("%s: %d channel is failed to request\n", __func__, EAT_FRM_CHANNEL);
		return -ENODEV;
	}

	*id = cmd.buffer[3];
	offset = SR(cmd.buffer[2]);
	*len = cmd.buffer[1];

	ret = adv_tracer_ipc_channel_init(*id, offset, *len, handler, (char *)plugin_name);
	if (ret) {
		pr_err("%s: %d channel is failed to init\n", __func__, *id);
		return -ENODEV;
	}

	return 0;
}

static void adv_tracer_ipc_channel_clear(void)
{
	struct adv_tracer_ipc_ch *channel;

	channel = &adv_tracer_ipc->channel[EAT_FRM_CHANNEL];
	channel->cmd->cmd_raw.cmd = EAT_IPC_CMD_CH_CLEAR;
	adv_tracer_ipc_write_buffer(channel->buff_regs, channel->cmd, 1);
	adv_tracer_interrupt_gen(EAT_FRM_CHANNEL);
}

static void adv_tracer_ipc_dbgc_panic(void)
{
	u32 val = 0;

	exynos_pmu_update(EXYNOS_DBGCORE_CPU_CONFIGURATION, 1, 0);
	dev_info(adv_tracer_ipc->dev, "DBGC power off for recovery.\n");
	exynos_pmu_update(EXYNOS_DBGCORE_CPU_CONFIGURATION, 1, 1);
	mdelay(10);
	exynos_pmu_read(EXYNOS_DBGCORE_CPU_STATUS, &val);
	if (!val) {
		dev_err(adv_tracer_ipc->dev, "DBGC abnormal state!\n");
		panic("DBGC abnormal state!");
	}
}

static void adv_tracer_ipc_callback(struct adv_tracer_ipc_cmd *cmd, unsigned int len)
{
	switch(cmd->cmd_raw.cmd) {
	case EAT_IPC_CMD_BOOT_DBGC:
		dev_info(adv_tracer_ipc->dev, "DBGC Boot!(cnt:%d)\n", cmd->buffer[1]);
		break;
	case EAT_IPC_CMD_EXCEPTION_DBGC:
		dev_err(adv_tracer_ipc->dev,
				"DBGC occurred exception! (SP: 0x%x, LR:0x%x, PC:0x%x)\n",
				cmd->buffer[1], cmd->buffer[2], cmd->buffer[3]);
		if (adv_tracer_ipc->recovery)
			panic("DBGC failed recovery!");
		adv_tracer_ipc_dbgc_panic();
		adv_tracer_ipc->recovery = true;
		break;
	default:
		break;
	}
}

static int adv_tracer_ipc_channel_probe(void)
{
	int i, ret;

	adv_tracer_ipc->num_channels = EAT_MAX_CHANNEL;
	adv_tracer_ipc->channel = devm_kzalloc(adv_tracer_ipc->dev,
			sizeof(struct adv_tracer_ipc_ch) * adv_tracer_ipc->num_channels, GFP_KERNEL);

	if (IS_ERR(adv_tracer_ipc->channel))
		return PTR_ERR(adv_tracer_ipc->channel);

	for (i = 0; i < adv_tracer_ipc->num_channels; i++) {
		init_completion(&adv_tracer_ipc->channel[i].wait);
		spin_lock_init(&adv_tracer_ipc->channel[i].ch_lock);
	}

	ret = adv_tracer_ipc_channel_init(EAT_FRM_CHANNEL, SR(16), 4,
			adv_tracer_ipc_callback, FRAMEWORK_NAME);
	if (ret) {
		pr_err("%s: failed to register Framework channel\n", __func__);
		return -EIO;
	}

	adv_tracer_ipc_channel_clear();
	return 0;
}

int adv_tracer_ipc_init(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node;
	struct resource *res;
	int ret = 0;

	if (!node) {
		dev_err(&pdev->dev, "driver doesnt support non-dt devices\n");
		return -ENODEV;
	}

	dev_info(&pdev->dev, "[EAT +] %s\n", __func__);

	adv_tracer_ipc = devm_kzalloc(&pdev->dev,
				sizeof(struct adv_tracer_ipc_main), GFP_KERNEL);

	if (IS_ERR(adv_tracer_ipc))
		return PTR_ERR(adv_tracer_ipc);

	/* Mailbox interrupt, AP -- Debug Core */
	adv_tracer_ipc->irq = irq_of_parse_and_map(node, 0);
	adv_tracer_ipc->recovery = false;

	/* Mailbox base register */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mailbox");
	adv_tracer_ipc->mailbox_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(adv_tracer_ipc->mailbox_base))
		return PTR_ERR(adv_tracer_ipc->mailbox_base);

	adv_tracer_ipc_interrupt_clear(EAT_FRM_CHANNEL);
	ret = devm_request_threaded_irq(&pdev->dev, adv_tracer_ipc->irq, adv_tracer_ipc_irq_handler,
			adv_tracer_ipc_irq_handler_thread,
			IRQF_ONESHOT,
			dev_name(&pdev->dev), adv_tracer_ipc);
	if (ret) {
		dev_err(&pdev->dev, "failed to register adv_tracer_ipc interrupt: %d\n", ret);
		return ret;
	}
	adv_tracer_ipc->dev = &pdev->dev;

	ret = adv_tracer_ipc_channel_probe();
	if (ret) {
		dev_err(&pdev->dev, "failed to register channel: %d\n", ret);
		return ret;
	}

	dev_info(&pdev->dev, "[EAT -] %s\n", __func__);
	return ret;
}
