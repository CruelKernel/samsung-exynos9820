/*
 * Copyright (C) 2018 Samsung Electronics.
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

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/mcu_ipc.h>
#include <linux/shm_ipc.h>
#include <linux/smc.h>
#include <linux/modem_notifier.h>
#include <soc/samsung/cal-if.h>
#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"

#define MIF_INIT_TIMEOUT	(15 * HZ)

#ifdef CONFIG_EXYNOS_DECON_LCD
#define DEFAULT_DSP_TYPE	0x01	// disconnected + C type
extern int get_lcd_info(char *arg);
#endif

/*
 * CP_WDT interrupt handler
 */
static irqreturn_t cp_wdt_handler(int irq, void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct io_device *iod;
	enum modem_state new_state;

	mif_disable_irq(&mc->irq_cp_wdt);
	mif_info("%s: CP_WDT occurred\n", mc->name);

	if (mc->phone_state == STATE_ONLINE)
		modem_notify_event(MODEM_EVENT_WATCHDOG);

	mif_set_snapshot(false);

	new_state = STATE_CRASH_WATCHDOG;

	mif_info("new_state:%s\n", cp_state_str(new_state));

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, new_state);
	}

	return IRQ_HANDLED;
}

/*
 * ACTIVE mailbox interrupt handler
 */
static void cp_active_handler(void *arg)
{
	struct modem_ctl *mc = (struct modem_ctl *)arg;
	struct io_device *iod;
	int cp_on = cal_cp_status();
	int cp_active = mbox_extract_value(MCU_CP, mc->mbx_cp_status,
					mc->sbi_lte_active_mask, mc->sbi_lte_active_pos);
	enum modem_state old_state = mc->phone_state;
	enum modem_state new_state = mc->phone_state;

	mif_info("old_state:%s cp_on:%d cp_active:%d\n",
		cp_state_str(old_state), cp_on, cp_active);

	if (!cp_active) {
		if (cp_on > 0) {
			new_state = STATE_OFFLINE;
			complete_all(&mc->off_cmpl);
		} else {
			mif_info("don't care!!!\n");
		}
	}

	if (old_state != new_state) {
		mif_info("new_state = %s\n", cp_state_str(new_state));

		if (old_state == STATE_ONLINE)
			modem_notify_event(MODEM_EVENT_RESET);

		list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
			if (iod && atomic_read(&iod->opened) > 0)
				iod->modem_state_changed(iod, new_state);
		}
	}
}

static int hw_rev;
#ifdef CONFIG_HW_REV_DETECT
static int __init console_setup(char *str)
{
	get_option(&str, &hw_rev);
	mif_info("hw_rev:%d\n", hw_rev);

	return 0;
}
__setup("androidboot.revision=", console_setup);
#else
static int get_system_rev(struct device_node *np)
{
	int value, cnt, gpio_cnt;
	unsigned int gpio_hw_rev, hw_rev = 0;

	gpio_cnt = of_gpio_count(np);
	if (gpio_cnt < 0) {
		mif_err("failed to get gpio_count from DT(%d)\n", gpio_cnt);
		return gpio_cnt;
	}

	for (cnt = 0; cnt < gpio_cnt; cnt++) {
		gpio_hw_rev = of_get_gpio(np, cnt);
		if (!gpio_is_valid(gpio_hw_rev)) {
			mif_err("gpio_hw_rev%d: Invalied gpio\n", cnt);
			return -EINVAL;
		}

		value = gpio_get_value(gpio_hw_rev);
		hw_rev |= (value & 0x1) << cnt;
	}

	return hw_rev;
}
#endif

#ifdef CONFIG_GPIO_DS_DETECT
static int get_ds_detect(struct device_node *np)
{
	unsigned gpio_ds_det;

	gpio_ds_det = of_get_named_gpio(np, "mif,gpio_ds_det", 0);
	if (!gpio_is_valid(gpio_ds_det)) {
		mif_err("gpio_ds_det: Invalid gpio\n");
		return 0;
	}

	return gpio_get_value(gpio_ds_det);
}
#else
static int ds_detect = 1;
module_param(ds_detect, int, S_IRUGO | S_IWUSR | S_IWGRP | S_IRGRP);
MODULE_PARM_DESC(ds_detect, "Dual SIM detect");

static ssize_t ds_detect_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", ds_detect);
}

static ssize_t ds_detect_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret;
	int value;

	ret = sscanf(buf, "%d", &value);
	if (ret != 1 || value > 2 || value < 0) {
		mif_err("invalid value:%d with %d\n", value, ret);
		return -EINVAL;
	}

	ds_detect = value;
	mif_info("set ds_detect: %d\n", ds_detect);

	return count;
}
static DEVICE_ATTR_RW(ds_detect);

static struct attribute *sim_attrs[] = {
	&dev_attr_ds_detect.attr,
	NULL,
};

static const struct attribute_group sim_group = {	\
	.attrs = sim_attrs,				\
	.name = "sim",
};

static int get_ds_detect(struct device_node *np)
{
	mif_info("Dual SIM detect = %d\n", ds_detect);
	return ds_detect - 1;
}
#endif

static int init_mailbox_regs(struct modem_ctl *mc)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	struct device_node *np = pdev->dev.of_node;
	unsigned int mbx_ap_status;
	unsigned int sbi_ds_det_mask, sbi_ds_det_pos;
	unsigned int sbi_sys_rev_mask, sbi_sys_rev_pos;
	int ds_det, i;
#ifdef CONFIG_CP_RAM_LOGGING
	unsigned int sbi_ext_backtrace_mask, sbi_ext_backtrace_pos;
#endif

	for (i = 0; i < MAX_MBOX_NUM; i++)
		mbox_set_value(MCU_CP, i, 0);

	if (!np) {
		mif_err("non-DT project, can't set mailbox regs\n");
		return -1;
	}

	mif_dt_read_u32(np, "mbx_ap2cp_united_status", mbx_ap_status);

#ifdef CONFIG_CP_RAM_LOGGING
	mif_dt_read_u32(np, "sbi_ext_backtrace_mask", sbi_ext_backtrace_mask);
	mif_dt_read_u32(np, "sbi_ext_backtrace_pos", sbi_ext_backtrace_pos);
	mbox_update_value(MCU_CP, mbx_ap_status, shm_get_cplog_flag(),
		sbi_ext_backtrace_mask, sbi_ext_backtrace_pos);
#endif

	mif_dt_read_u32(np, "sbi_sys_rev_mask", sbi_sys_rev_mask);
	mif_dt_read_u32(np, "sbi_sys_rev_pos", sbi_sys_rev_pos);
	mif_dt_read_u32(np, "sbi_ds_det_mask", sbi_ds_det_mask);
	mif_dt_read_u32(np, "sbi_ds_det_pos", sbi_ds_det_pos);

	ds_det = get_ds_detect(np);
	if (ds_det < 0) {
		mif_err("ds_det error:%d\n", ds_det);
		return -EINVAL;
	}
	mbox_update_value(MCU_CP, mbx_ap_status, ds_det,
				sbi_ds_det_mask, sbi_ds_det_pos);
	mif_info("ds_det:%d\n", ds_det);

#ifndef CONFIG_HW_REV_DETECT
	hw_rev = get_system_rev(np);
#endif
	if (hw_rev < 0) {
		mif_err("hw_rev error:%d\n", hw_rev);
		return -EINVAL;
	}
	mbox_update_value(MCU_CP, mbx_ap_status, hw_rev,
				sbi_sys_rev_mask, sbi_sys_rev_pos);
	mif_info("hw_rev:%d\n", hw_rev);

#ifdef CONFIG_EXYNOS_DECON_LCD
	{//////* Detect and deliver device type to CP */
	unsigned int sbi_device_type_mask, sbi_device_type_pos;
	unsigned int value = 0;
	int dsp_connected = 0;
	int dsp_type = 0;

	mif_dt_read_u32(np, "sbi_device_type_mask", sbi_device_type_mask);
	mif_dt_read_u32(np, "sbi_device_type_pos", sbi_device_type_pos);

	dsp_connected = get_lcd_info("connected");
	if (dsp_connected < 0) {
		mif_err("Failed to get dsp_info(%d)\n", dsp_connected);
		value = DEFAULT_DSP_TYPE;
	} else {
		/* 1: dsp_connect, 0: dsp_disconnect */
		if (dsp_connected) {
			dsp_type = get_lcd_info("id");
			if (dsp_type < 0) {
				mif_err("Failed to get dsp_type(%d)\n", dsp_type);
				value = DEFAULT_DSP_TYPE;
			} else {
				/* [3:0] display type, [4] connected or not */
				value |= ((dsp_connected & 0x1) << 4);
				/* DSP ID1 value is located in [19:16] */
				value |= ((dsp_type >> 16) & 0xf);
			}
		} else {
			mif_err("display disconnected, set default value\n");
			value = DEFAULT_DSP_TYPE;
		}
	}

	mbox_update_value(MCU_CP, mbx_ap_status, value,
		sbi_device_type_mask, sbi_device_type_pos);

	mif_info("dsp_type:0x%x, conn:0x%x, get_val: 0x%x\n",
		dsp_type, dsp_connected,
		mbox_get_value(MCU_CP, mbx_ap_status));
	}/* Detect and deliver device type to CP *//////
#endif
	return 0;
}

static int s5000ap_on(struct modem_ctl *mc)
{
	int cp_active = mbox_extract_value(MCU_CP, mc->mbx_cp_status,
						mc->sbi_lte_active_mask,
						mc->sbi_lte_active_pos);
	int cp_status = mbox_extract_value(MCU_CP, mc->mbx_cp_status,
						mc->sbi_cp_status_mask,
						mc->sbi_cp_status_pos);
	int ret;

	mif_info("+++\n");
	mif_info("cp_active:%d cp_status:%d\n", cp_active, cp_status);

	mc->receive_first_ipc = 0;

#ifndef CONFIG_CP_SECURE_BOOT
	exynos_cp_init();
#endif

	/* Enable debug Snapshot */
	mif_set_snapshot(true);

	mc->phone_state = STATE_OFFLINE;

	if (init_mailbox_regs(mc))
		mif_err("Failed to initialize mbox regs\n");

	mbox_update_value(MCU_CP, mc->mbx_ap_status, 1,
			mc->sbi_pda_active_mask, mc->sbi_pda_active_pos);
	mif_info("ap_status:0x%08x\n", mbox_get_value(MCU_CP, mc->mbx_ap_status));

	if (mc->ap2cp_cfg_ioaddr) {
		mif_info("Before setting AP2CP_CFG:0x%08x\n",
					__raw_readl(mc->ap2cp_cfg_ioaddr));
		__raw_writel(1, mc->ap2cp_cfg_ioaddr);
		ret = __raw_readl(mc->ap2cp_cfg_ioaddr);
		if (ret != 1) {
			mif_err("AP2CP_CFG setting is not correct:%d\n", ret);
			return -1;
		}
		mif_info("AP2CP_CFG is ok:0x%08x\n", ret);
	} else {
		if (cal_cp_status()) {
			mif_info("CP aleady Init, Just reset release!\n");
			cal_cp_reset_release();
		} else {
			mif_info("CP first Init!\n");
			cal_cp_init();
		}
	}

	mif_info("---\n");
	return 0;
}

static int s5000ap_off(struct modem_ctl *mc)
{
	mif_info("+++\n");

	mbox_set_interrupt(MCU_CP, mc->int_cp_wakeup);
	usleep_range(5000, 10000);

	cal_cp_enable_dump_pc_no_pg();
	cal_cp_reset_assert();

	mif_info("---\n");
	return 0;
}

static int s5000ap_shutdown(struct modem_ctl *mc)
{
	struct io_device *iod;
	unsigned long timeout = msecs_to_jiffies(3000);
	unsigned long remain;

	mif_info("+++\n");

	if (mc->phone_state == STATE_OFFLINE || cal_cp_status() == 0)
		goto exit;

	reinit_completion(&mc->off_cmpl);
	remain = wait_for_completion_timeout(&mc->off_cmpl, timeout);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		mc->phone_state = STATE_OFFLINE;
		list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
			if (iod && atomic_read(&iod->opened) > 0)
				iod->modem_state_changed(iod, STATE_OFFLINE);
		}
	}

exit:
	mbox_set_interrupt(MCU_CP, mc->int_cp_wakeup);
	usleep_range(5000, 10000);

	cal_cp_enable_dump_pc_no_pg();
	cal_cp_reset_assert();

	mif_info("---\n");
	return 0;
}

static int s5000ap_reset(struct modem_ctl *mc)
{
	void __iomem *base = shm_get_ipc_region();

	mif_info("+++\n");

	mc->receive_first_ipc = 0;

	/* mc->phone_state = STATE_OFFLINE; */
	if (mc->phone_state == STATE_OFFLINE)
		return 0;

	/* FIXME: For CP debug */
	if (base) {
		if (*(unsigned int *)(base + 0xF80) == 0xDEB)
			return 0;
	}

	if (mc->phone_state == STATE_ONLINE)
		modem_notify_event(MODEM_EVENT_RESET);

	/* Change phone state to OFFLINE */
	mc->phone_state = STATE_OFFLINE;

	if (cal_cp_status()) {
		mif_info("CP aleady Init, try reset\n");
		mbox_set_interrupt(MCU_CP, mc->int_cp_wakeup);
		usleep_range(5000, 10000);

		cal_cp_enable_dump_pc_no_pg();
		cal_cp_reset_assert();
		usleep_range(5000, 10000);
		cal_cp_reset_release();

		mbox_sw_reset(MCU_CP);
	}

	mif_info("---\n");
	return 0;
}

static int s5000ap_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct io_device *iod;
	int cnt = 100;

	mif_info("+++\n");

	if (ld->boot_on)
		ld->boot_on(ld, mc->bootd);

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, STATE_BOOTING);
	}

	while (mbox_extract_value(MCU_CP, mc->mbx_cp_status,
				mc->sbi_cp_status_mask, mc->sbi_cp_status_pos) == 0) {
		if (--cnt > 0)
			usleep_range(10000, 20000);
		else
			return -EACCES;
	}

	mif_disable_irq(&mc->irq_cp_wdt);

	mif_info("cp_status=%u\n",
			mbox_extract_value(MCU_CP, mc->mbx_cp_status,
					mc->sbi_cp_status_mask, mc->sbi_cp_status_pos));

	mif_info("---\n");
	return 0;
}

static int s5000ap_boot_off(struct modem_ctl *mc)
{
	struct io_device *iod;
	unsigned long remain;
	int err = 0;
	mif_info("+++\n");

	cal_cp_disable_dump_pc_no_pg();

	reinit_completion(&mc->init_cmpl);
	remain = wait_for_completion_timeout(&mc->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		err = -EAGAIN;
		goto exit;
	}

	mif_enable_irq(&mc->irq_cp_wdt);

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, STATE_ONLINE);
	}

	mif_info("---\n");

exit:
	return err;
}

static int s5000ap_boot_done(struct modem_ctl *mc)
{
	mif_info("+++\n");
	mif_info("---\n");
	return 0;
}

static int s5000ap_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_info("+++\n");

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_info("---\n");
	return 0;
}

/*
 * Notify AP crash status to CP
 */
static struct modem_ctl *g_mc;
int modem_force_crash_exit_ext(void)
{
	if (!g_mc) {
		mif_err("g_mc is null\n");
		return -1;
	}

	mif_info("Make forced crash exit\n");
	s5000ap_force_crash_exit(g_mc);

	return 0;
}
EXPORT_SYMBOL(modem_force_crash_exit_ext);

int modem_send_panic_noti_ext(void)
{
	struct modem_data *modem;

	if (!g_mc) {
		mif_err("g_mc is null\n");
		return -1;
	}

	modem = g_mc->mdm_data;
	if (!modem->mld) {
		mif_err("modem->mld is null\n");
		return -1;
	}

	mif_info("Send CMD_KERNEL_PANIC message to CP\n");
	send_ipc_irq(modem->mld, cmd2int(CMD_KERNEL_PANIC));

	return 0;
}
EXPORT_SYMBOL(modem_send_panic_noti_ext);

static int s5000ap_dump_start(struct modem_ctl *mc)
{
	int err, ret;
	struct link_device *ld = get_current_link(mc->bootd);

	mif_info("+++\n");

	if (!ld->dump_start) {
		mif_err("ERR! %s->dump_start not exist\n", ld->name);
		return -EFAULT;
	}

	/* Change phone state to CRASH_EXIT */
	mc->phone_state = STATE_CRASH_EXIT;

	err = ld->dump_start(ld, mc->bootd);
	if (err)
		return err;

	if (mc->ap2cp_cfg_ioaddr) {
		mif_info("Before setting AP2CP_CFG:0x%08x\n",
					__raw_readl(mc->ap2cp_cfg_ioaddr));
		__raw_writel(1, mc->ap2cp_cfg_ioaddr);
		ret = __raw_readl(mc->ap2cp_cfg_ioaddr);
		if (ret != 1) {
			mif_err("AP2CP_CFG setting is not correct:%d\n", ret);
			return -1;
		}
		mif_info("AP2CP_CFG is ok:0x%08x\n", ret);
	} else
		cal_cp_reset_release();

	mbox_update_value(MCU_CP, mc->mbx_ap_status, 1,
			mc->sbi_ap_status_mask, mc->sbi_ap_status_pos);

	mif_info("---\n");
	return err;
}

static void s5000ap_modem_boot_confirm(struct modem_ctl *mc)
{
	mbox_update_value(MCU_CP,mc->mbx_ap_status, 1,
			mc->sbi_ap_status_mask, mc->sbi_ap_status_pos);
	mif_info("ap_status=%u\n",
			mbox_extract_value(MCU_CP, mc->mbx_ap_status,
			mc->sbi_ap_status_mask, mc->sbi_ap_status_pos));
}

static void s5000ap_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = s5000ap_on;
	mc->ops.modem_off = s5000ap_off;
	mc->ops.modem_shutdown = s5000ap_shutdown;
	mc->ops.modem_reset = s5000ap_reset;
	mc->ops.modem_boot_on = s5000ap_boot_on;
	mc->ops.modem_boot_off = s5000ap_boot_off;
	mc->ops.modem_boot_done = s5000ap_boot_done;
	mc->ops.modem_force_crash_exit = s5000ap_force_crash_exit;
	mc->ops.modem_dump_start = s5000ap_dump_start;
	mc->ops.modem_boot_confirm = s5000ap_modem_boot_confirm;
}

static void s5000ap_get_pdata(struct modem_ctl *mc, struct modem_data *modem)
{
	struct modem_mbox *mbx = modem->mbx;

	mc->int_pda_active = mbx->int_ap2cp_active;

	mc->int_cp_wakeup = mbx->int_ap2cp_wakeup;

	mc->irq_phone_active = mbx->irq_cp2ap_active;

	mc->mbx_ap_status = mbx->mbx_ap2cp_status;
	mc->mbx_cp_status = mbx->mbx_cp2ap_status;

	mc->mbx_perf_req = mbx->mbx_cp2ap_perf_req;
	mc->irq_perf_req = mbx->irq_cp2ap_perf_req;

	mc->int_uart_noti = mbx->int_ap2cp_uart_noti;

	mc->sbi_lte_active_mask = mbx->sbi_lte_active_mask;
	mc->sbi_lte_active_pos = mbx->sbi_lte_active_pos;
	mc->sbi_cp_status_mask = mbx->sbi_cp_status_mask;
	mc->sbi_cp_status_pos = mbx->sbi_cp_status_pos;

	mc->sbi_pda_active_mask = mbx->sbi_pda_active_mask;
	mc->sbi_pda_active_pos = mbx->sbi_pda_active_pos;
	mc->sbi_ap_status_mask = mbx->sbi_ap_status_mask;
	mc->sbi_ap_status_pos = mbx->sbi_ap_status_pos;

	mc->sbi_uart_noti_mask = mbx->sbi_uart_noti_mask;
	mc->sbi_uart_noti_pos = mbx->sbi_uart_noti_pos;

	mc->sbi_crash_type_mask = mbx->sbi_crash_type_mask;
	mc->sbi_crash_type_pos = mbx->sbi_crash_type_pos;
}

int s5000ap_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	unsigned int irq_num;
	unsigned long flags = IRQF_NO_SUSPEND | IRQF_NO_THREAD;

	mif_info("+++\n");

	/* To notify AP crash status to CP */
	g_mc = mc;

	s5000ap_get_ops(mc);
	s5000ap_get_pdata(mc, pdata);
	dev_set_drvdata(mc->dev, mc);

	/* Register CP_WDT */
	irq_num = platform_get_irq(pdev, 0);
	mif_init_irq(&mc->irq_cp_wdt, irq_num, "cp_wdt", flags);
	ret = mif_request_irq(&mc->irq_cp_wdt, cp_wdt_handler, mc);
	if (ret) {
		mif_err("Failed to request_irq with(%d)", ret);
		return ret;
	}
	/* CP_WDT interrupt must be enabled only after CP booting */
	mif_disable_irq(&mc->irq_cp_wdt);

	/* Register LTE_ACTIVE mailbox interrupt */
	ret = mbox_request_irq(MCU_CP, mc->irq_phone_active, cp_active_handler, mc);
	if (ret) {
		mif_err("Failed to mbox_request_irq %u with(%d)",
				mc->irq_phone_active, ret);
		return ret;
	}

	init_completion(&mc->init_cmpl);
	init_completion(&mc->off_cmpl);

	/* AP2CP_CFG */
	mif_dt_read_u32_noerr(np, "ap2cp_cfg_addr", mc->ap2cp_cfg_addr);
	if (mc->ap2cp_cfg_addr) {
		mif_info("AP2CP_CFG:0x%08x\n", mc->ap2cp_cfg_addr);
		mc->ap2cp_cfg_ioaddr = devm_ioremap(mc->dev, mc->ap2cp_cfg_addr, SZ_64);
		if (mc->ap2cp_cfg_ioaddr == NULL) {
			mif_err("%s: AP2CP_CFG ioremap failed.\n", __func__);
			return -EACCES;
		}
	}

#ifndef CONFIG_GPIO_DS_DETECT
	if (sysfs_create_group(&pdev->dev.kobj, &sim_group))
		mif_err("failed to create sysfs node related sim\n");
#endif
	mif_info("---\n");
	return 0;
}
