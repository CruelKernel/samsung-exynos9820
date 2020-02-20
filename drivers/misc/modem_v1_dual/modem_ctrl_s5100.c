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

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/smc.h>
#include <linux/modem_notifier.h>
#include <linux/sec_class.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <soc/samsung/acpm_mfd.h>
#include <linux/reboot.h>
#include <linux/suspend.h>

#ifdef CONFIG_MUIC_NOTIFIER
#include <linux/muic/muic.h>
#include <linux/muic/muic_notifier.h>
#endif

#include <linux/exynos-pci-ctrl.h>
#include <linux/shm_ipc.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "s5100_pcie.h"

#ifdef CONFIG_EXYNOS_BUSMONITOR
#include <linux/exynos-busmon.h>
#endif

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
#include <linux/modem_notifier.h>
#endif

#define MIF_INIT_TIMEOUT	(300 * HZ)

#define msecs_to_loops(t) (loops_per_jiffy / 1000 * HZ * t)
#define CP2AP_WAKEUP_WAIT_TIME	100	/* msec */

#define RUNTIME_PM_AFFINITY_CORE 2

static void __iomem *cmgp2pmu_ap_ioaddr;
void init_pinctl_cp2ap_wakeup(struct modem_ctl *mc)
{
	u32 temp;

	cmgp2pmu_ap_ioaddr = devm_ioremap(mc->dev, 0x15c70288, SZ_64);
	if (cmgp2pmu_ap_ioaddr == NULL) {
	       mif_err("Fail to ioremap for cmgp2pmu_ap_ioaddr\n");
	       return;
	}

	temp = __raw_readl(cmgp2pmu_ap_ioaddr);
	mif_err("BEFORE 0x15c70288: %08X\n", temp);
	temp = temp | (1<<26);

	__raw_writel(temp, cmgp2pmu_ap_ioaddr);

	temp = __raw_readl(cmgp2pmu_ap_ioaddr);
	mif_err("AFTER 0x15c70288: %08X\n", temp);

	devm_ioremap_release(mc->dev, cmgp2pmu_ap_ioaddr);
}

static DEFINE_MUTEX(ap_status_lock);
static struct modem_ctl *g_mc;

static int register_phone_active_interrupt(struct modem_ctl *mc);
static int register_cp2ap_wakeup_interrupt(struct modem_ctl *mc);

static int exynos_s5100_reboot_handler(struct notifier_block *nb,
				    unsigned long l, void *p)
{
	mif_info("Now is device rebooting..ignore runtime suspend\n");
	g_mc->device_reboot = true;

	return 0;
}

static struct notifier_block nb_reboot_block = {
	.notifier_call = exynos_s5100_reboot_handler
};


static void print_mc_state(struct modem_ctl *mc)
{
	int pwr  = gpio_get_value(mc->s5100_gpio_cp_pwr);
	int reset = gpio_get_value(mc->s5100_gpio_cp_reset);
	int pshold = gpio_get_value(mc->s5100_gpio_cp_ps_hold);

	int ap_wakeup = gpio_get_value(mc->s5100_gpio_ap_wakeup);
	int cp_wakeup = gpio_get_value(mc->s5100_gpio_cp_wakeup);

	int dump = gpio_get_value(mc->s5100_gpio_cp_dump_noti);
	int ap_status = gpio_get_value(mc->s5100_gpio_ap_status);
	int phone_active = gpio_get_value(mc->s5100_gpio_phone_active);

	mif_info("%s: %pf:GPIO - pwr:%d rst:%d phd:%d aw:%d cw:%d dmp:%d ap_status:%d phone_state:%d\n",
		mc->name, CALLER, pwr, reset, pshold, ap_wakeup, cp_wakeup,
		dump, ap_status, phone_active);
}

static ssize_t modem_ctrl_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct link_device *ld = get_current_link(mc->iod);
	struct platform_device *pdev = to_platform_device(mc->dev);

	ret = snprintf(buf, PAGE_SIZE, "Start S5100 PCIe\n");

	mdelay(100);

	exynos_pcie_host_v1_poweron(mc->pcie_ch_num);
	atomic_set(&mc->pcie_pwron, 1);
	s5100pcie_init(mc->pcie_ch_num);

	request_pcie_msi_int(ld, pdev);
	mc->s5100_pdev = s5100pcie_get_pcidev();

	exynos_pcie_host_v1_poweroff(mc->pcie_ch_num);
	atomic_set(&mc->pcie_pwron, 0);

	return ret;
}
static int s5100_off(struct modem_ctl *mc);

static ssize_t modem_ctrl_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef CONFIG_PM
	struct modem_ctl *mc = dev_get_drvdata(dev);
#endif
	long ops_num;
	int ret;

	ret = kstrtol(buf, 10, &ops_num);

	if (ret != 0)
		return count;

	switch (ops_num) {
#ifdef CONFIG_PM
	case 0:
		mif_err(">>>>Queuing DISLINK work(UsageCNT:%d)<<<<\n",
				(int)atomic_read(&dev->power.usage_count));
		queue_work(mc->wakeup_wq, &mc->dislink_work);
		break;
	case 1:
		mif_err(">>>>Queuing LINK work(UsageCNT:%d)<<<<\n",
				(int)atomic_read(&dev->power.usage_count));
		queue_work(mc->wakeup_wq, &mc->link_work);
		break;

	case 2:
		cp_runtime_dislink(mc, LINK_CP, 0);
		mif_err("Checking Usage Count(%d)!!!\n",
				(int)atomic_read(&dev->power.usage_count));
		break;

	case 3:
		cp_runtime_link(mc, LINK_CP, 0);
		mif_err("Checking Usage Count(%d)!!!\n",
				(int)atomic_read(&dev->power.usage_count));
		break;
	case 4:
		mif_err("Checking Usage Count(%d)!!!\n",
				(int)atomic_read(&dev->power.usage_count));
		break;
#endif

	default:
		mif_err("Wrong operation number\n");
		mif_err("0 - Queuing Dislink work.\n");
		mif_err("1 - Queuing Link work.\n");
		mif_err("2 - Run runtime link.\n");
		mif_err("3 - Run runtime dislink.\n");
		mif_err("4 - Check usage count.\n");
	}

	return count;
}

static DEVICE_ATTR(modem_ctrl, 0644, modem_ctrl_show, modem_ctrl_store);

static ssize_t detect_time_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	
	return sprintf(buf, "%d\n", mld->not_work_time);
}

static ssize_t detect_time_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct modem_ctl *mc = dev_get_drvdata(dev);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int value;

	sscanf(buf, "%d", &value);
	if (value < 0 || value > 300) {
		return -EINVAL;
	}

	mld->not_work_time = (unsigned int)value;
	mif_info("set detect_time to: %u sec\n", mld->not_work_time);

	return count;
}

static DEVICE_ATTR(detect_time, 0644, detect_time_show, detect_time_store);

static void runtime_link_cnt(struct modem_ctl *mc)
{
#define LINE_BUFF_SIZE 80

	int i;
	char str[MAX_STR_LEN] = {0, };
	char line[LINE_BUFF_SIZE] = {0, };

	mif_info("SEND:%u/%u CP:%u/%u SBD_TX:%u/%u TX:%u/%u REG_PCI:%u/%u\n",
			atomic_read(&mc->runtime_link_cnt[LINK_SEND]),
			atomic_read(&mc->runtime_dislink_cnt[LINK_SEND]),
			atomic_read(&mc->runtime_link_cnt[LINK_CP]),
			atomic_read(&mc->runtime_dislink_cnt[LINK_CP]),
			atomic_read(&mc->runtime_link_cnt[LINK_SBD_TX_TIMER]),
			atomic_read(&mc->runtime_dislink_cnt[LINK_SBD_TX_TIMER]),
			atomic_read(&mc->runtime_link_cnt[LINK_TX_TIMER]),
			atomic_read(&mc->runtime_dislink_cnt[LINK_TX_TIMER]),
			atomic_read(&mc->runtime_link_cnt[LINK_REGISTER_PCI]),
			atomic_read(&mc->runtime_dislink_cnt[LINK_REGISTER_PCI]));

	snprintf(line, LINE_BUFF_SIZE, "IOD - ");
	strncat(str, line, strlen(line));
	for (i = 0; i < SIPC5_CH_ID_MAX; i++)
		if (atomic_read(&mc->runtime_link_iod_cnt[i]) != 0) {
			snprintf(line, LINE_BUFF_SIZE, "%d:%u/%u ", i,
				atomic_read(&mc->runtime_link_iod_cnt[i]),
				atomic_read(&mc->runtime_dislink_iod_cnt[i]));
			strncat(str, line, strlen(line));
		}
	mif_info("%s\n", str);
}

static void pcie_link_work(struct work_struct *ws)
{
	struct modem_ctl *mc;
	int __maybe_unused usage_cnt;

	mc = container_of(ws, struct modem_ctl, link_work);
	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	mif_info("pm usage_cnt=%d boot_done_cnt=%d\n", usage_cnt, mc->boot_done_cnt);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	cp_runtime_link(mc, LINK_CP, 0);
}

static void pcie_dislink_work(struct work_struct *ws)
{
	struct modem_ctl *mc;
	int __maybe_unused usage_cnt;

	mc = container_of(ws, struct modem_ctl, dislink_work);
	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	mif_info("pm usage_cnt=%d boot_done_cnt=%d\n", usage_cnt, mc->boot_done_cnt);

	if (mc->phone_state == STATE_ONLINE)
		cp_runtime_dislink(mc, LINK_CP, 0);
	else {
		mif_info("phone_state is not ONLINE:%d\n", mc->phone_state);
		if (usage_cnt > 0) 
		{
			mif_info("Enter RPM suspend - no autosuspend\n");
			cp_runtime_dislink_no_autosuspend(mc, LINK_CP, 0);
		}
	}

	runtime_link_cnt(mc);
}

static void pcie_clean_dislink(struct modem_ctl *mc)
{
	int __maybe_unused usage_cnt;
	int i;

	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	mif_info("pm usage_cnt=%d boot_done_cnt=%d\n", usage_cnt,
			mc->boot_done_cnt);

	if (usage_cnt > 0) {
		mif_info("Enter RPM suspend - no autosuspend\n");
		cp_runtime_dislink_no_autosuspend(mc, LINK_REGISTER_PCI, 0);
	}

	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	if (usage_cnt)
		mif_info("WARNING - pm usage_cnt=%d boot_done_cnt=%d\n",
				usage_cnt, mc->boot_done_cnt);

	for (i = 0; i < 100; i++) {
		if (exynos_check_pcie_link_status(mc->pcie_ch_num) == 0)
			break;
		mif_info("wait pcie link down - %d\n", i);
		usleep_range(9000, 10000); /* 10ms */
	}

	if (i == 100) {
		mif_err("WARNING : Link is NOT disconnected-force dislink!!\n");

		if (mc->s5100_pdev != NULL && (mc->phone_state == STATE_ONLINE ||
				mc->phone_state == STATE_BOOTING)) {
			mif_info("save s5100_status() - phone_state:%d\n",
					mc->phone_state);
			save_s5100_status();
		} else
			mif_info("ignore save_s5100_status() - phone_state:%d\n",
					mc->phone_state);

		gpio_set_value(mc->s5100_gpio_cp_wakeup, 0);

		exynos_pcie_host_v1_poweroff(mc->pcie_ch_num);
	} else
		mif_err("Link is disconnected!!!\n");

	atomic_set(&mc->pcie_pwron, 0);
}

/* It means initial GPIO level. */
static int check_link_order = 1;

static void ap_wakeup_work(struct work_struct *ws)
{
	struct modem_ctl *mc = container_of(to_delayed_work(ws),
					    struct modem_ctl,
					    nr2ap_wakeup_work);
	int gpio_val;

	if (atomic_read(&mc->pcie_suspend)) {
		queue_delayed_work(mc->wakeup_wq,
				   &mc->nr2ap_wakeup_work,
				   HZ / 10);
		mif_info("pcie still suspended\n");

		return;
	}
	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

	gpio_val = gpio_get_value(mc->s5100_gpio_ap_wakeup);

	if (gpio_val != check_link_order)
		check_link_order = gpio_val;
	else
		goto _IRQ_HANDLED;

	mif_info("CP2AP_WAKEUP GPIO VAL : %d\n", gpio_val);

	if (gpio_val == 1) {
		mc->apwake_irq_chip->irq_set_type(
				irq_get_irq_data(mc->s5100_irq_ap_wakeup.num),
				IRQF_TRIGGER_LOW);

		if (mc->phone_state == STATE_CRASH_EXIT)
			goto _IRQ_HANDLED;

		if (!wake_lock_active(&mc->mc_wake_lock))
			wake_lock(&mc->mc_wake_lock);

		queue_work_on(RUNTIME_PM_AFFINITY_CORE, mc->wakeup_wq,
				&mc->link_work);
	} else {
		mc->apwake_irq_chip->irq_set_type(
				irq_get_irq_data(mc->s5100_irq_ap_wakeup.num),
				IRQF_TRIGGER_HIGH);

		if (mc->phone_state == STATE_CRASH_EXIT)
			goto _IRQ_HANDLED;

		queue_work_on(RUNTIME_PM_AFFINITY_CORE, mc->wakeup_wq,
				&mc->dislink_work);
	}

_IRQ_HANDLED:
	mif_enable_irq(&mc->s5100_irq_ap_wakeup);

	return;
}

static irqreturn_t ap_wakeup_handler(int irq, void *data)
{
	struct modem_ctl *mc = (struct modem_ctl *)data;
	int qdelay = 0;

	mif_disable_irq(&mc->s5100_irq_ap_wakeup);

	if (mc->device_reboot == true) {
		mif_info("skip : device is rebooting..!!!\n");
		/* do not enable irq */
		return IRQ_HANDLED;
	}

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	if (atomic_read(&mc->pcie_suspend)) {
		qdelay = HZ / 10;
	}

	queue_delayed_work(mc->wakeup_wq,
			   &mc->nr2ap_wakeup_work,
			   qdelay);

	return IRQ_HANDLED;
}

static irqreturn_t cp_active_handler(int irq, void *data)
{
	struct modem_ctl *mc = (struct modem_ctl *)data;
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct io_device *iod;
	int cp_active;
	enum modem_state old_state;
	enum modem_state new_state;

	if (mc == NULL) {
		mif_err_limited("modem_ctl is NOT initialized - IGNORING interrupt\n");
		goto irq_done;
	}

	if (mc->s5100_pdev == NULL) {
		mif_err_limited("S5100 is NOT initialized - IGNORING interrupt\n");
		goto irq_done;
	}

	if (mc->phone_state != STATE_ONLINE) {
		mif_err_limited("Phone_state is NOT ONLINE - IGNORING interrupt\n");
		goto irq_done;
	}

	cp_active = gpio_get_value(mc->s5100_gpio_phone_active);
	mif_err("[PHONE_ACTIVE Handler] state:%s cp_active:%d\n",
			cp_state_str(mc->phone_state), cp_active);

	if (cp_active == 1) {
		mif_err("ERROR - phone_start is not low, state:%s cp_active:%d\n",
				cp_state_str(mc->phone_state), cp_active);
		return IRQ_HANDLED;
	}

	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);

	old_state = mc->phone_state;
	new_state = STATE_CRASH_EXIT;
	cp_active = gpio_get_value(mc->s5100_gpio_phone_active);

	mif_info("Set s5100_cp_reset_required to false\n");
	mc->s5100_cp_reset_required = false;

	if (old_state != new_state) {
		mif_err("new_state = %s\n", cp_state_str(new_state));
		set_cp_crash_link(ld->link_type);

		if (old_state == STATE_ONLINE)
			modem_notify_event(MODEM_EVENT_EXIT, mc);

		list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
			if (iod && atomic_read(&iod->opened) > 0)
				iod->modem_state_changed(iod, new_state);
		}
	}

	atomic_set(&mld->forced_cp_crash, 0);

irq_done:
	mif_disable_irq(&mc->s5100_irq_phone_active);

	return IRQ_HANDLED;
}

static int register_phone_active_interrupt(struct modem_ctl *mc)
{
	int ret;

	if (mc == NULL)
		return -EINVAL;

	if (mc->s5100_irq_phone_active.registered == true)
		return 0;

	mif_info("Register PHONE ACTIVE interrupt.\n");
	mif_init_irq(&mc->s5100_irq_phone_active, mc->s5100_irq_phone_active.num,
			"phone_active", IRQF_TRIGGER_LOW);

	ret = mif_request_irq(&mc->s5100_irq_phone_active, cp_active_handler, mc);
	if (ret) {
		mif_err("%s: ERR! request_irq(%s#%d) fail (%d)\n",
			mc->name, mc->s5100_irq_phone_active.name,
			mc->s5100_irq_phone_active.num, ret);
		mif_err("xxx\n");
		return ret;
	}

	return ret;
}

static int register_cp2ap_wakeup_interrupt(struct modem_ctl *mc)
{
	int ret;

	if (mc == NULL)
		return -EINVAL;

	if (mc->s5100_irq_ap_wakeup.registered == true) {
		mif_info("Set IRQF_TRIGGER_LOW to cp2ap_wakeup gpio\n");
		check_link_order = 1;
		ret = mc->apwake_irq_chip->irq_set_type(
				irq_get_irq_data(mc->s5100_irq_ap_wakeup.num),
				IRQF_TRIGGER_LOW);
		return ret;
	}

	mif_info("Register CP2AP WAKEUP interrupt.\n");
	mif_init_irq(&mc->s5100_irq_ap_wakeup, mc->s5100_irq_ap_wakeup.num, "cp2ap_wakeup",
			IRQF_TRIGGER_LOW);

	ret = mif_request_irq(&mc->s5100_irq_ap_wakeup, ap_wakeup_handler, mc);
	if (ret) {
		mif_err("%s: ERR! request_irq(%s#%d) fail (%d)\n",
			mc->name, mc->s5100_irq_ap_wakeup.name,
			mc->s5100_irq_ap_wakeup.num, ret);
		mif_err("xxx\n");
		return ret;
	}

	return ret;
}

static int s5100_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->iod);
	struct modem_data __maybe_unused *modem = mc->mdm_data;
	struct mem_link_device *mld = to_mem_link_device(ld);
	struct shmem_4mb_phys_map *map = (struct shmem_4mb_phys_map *)mld->base;

	mif_info("%s: %s: +++\n", mc->name, __func__);

	mif_disable_irq(&mc->s5100_irq_phone_active);
	mif_disable_irq(&mc->s5100_irq_ap_wakeup);

	print_mc_state(mc);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	mc->phone_state = STATE_OFFLINE;
	pcie_clean_dislink(mc);

	mc->pcie_registered = false;

	mif_err("Set s5100_gpio_ap_status to 1\n");
	gpio_set_value(mc->s5100_gpio_ap_status, 1);

	mif_err("Set s5100_gpio_cp_dump_noti to 0\n");
	gpio_set_value(mc->s5100_gpio_cp_dump_noti, 0);

	/* Clear shared memory */
	map->ap2cp_msg = 0;
	map->cp2ap_msg = 0;
	map->cp2ap_int_state = 0;

	print_mc_state(mc);

	mif_info("SET GPIO_CP_RESET 0\n");
	gpio_set_value(mc->s5100_gpio_cp_reset, 0);

	mif_info("SET GPIO_CP_PWR 1\n");
	gpio_set_value(mc->s5100_gpio_cp_pwr, 1);

	mif_info("Wait 50ms\n");
	msleep(50);

	mif_info("SET GPIO_CP_RESET 1\n");
	gpio_set_value(mc->s5100_gpio_cp_reset, 1);

	mif_info("GPIO status after S5100 Power on\n");

	print_mc_state(mc);
	mif_info("---\n");

	return 0;
}

static int s5100_off(struct modem_ctl *mc)
{
	mif_info("%s: %s: +++\n", mc->name, __func__);

	if (mc->phone_state == STATE_OFFLINE)
		goto exit;

	mc->phone_state = STATE_OFFLINE;
	mc->bootd->modem_state_changed(mc->iod, STATE_OFFLINE);
	mc->iod->modem_state_changed(mc->iod, STATE_OFFLINE);

	pcie_clean_dislink(mc);

	mif_info("SET GPIO_CP_RESET 0\n");
	gpio_set_value(mc->s5100_gpio_cp_reset, 0);

	mif_info("SET GPIO_CP_PWR 0\n");
	gpio_set_value(mc->s5100_gpio_cp_pwr, 0);

	print_mc_state(mc);

exit:
	mif_info("---\n");

	return 0;
}

static int s5100_shutdown(struct modem_ctl *mc)
{
	int i;

	mif_err("+++\n");

	if (mc->phone_state == STATE_OFFLINE)
		goto exit;

	mif_disable_irq(&mc->s5100_irq_phone_active);
	mif_disable_irq(&mc->s5100_irq_ap_wakeup);

	/* wait for cp_active for 3 seconds */
	for (i = 0; i < 150; i++) {
		if (gpio_get_value(mc->s5100_gpio_phone_active)) {
			mif_err("PHONE_ACTIVE pin is HIGH...\n");
			break;
		}
		msleep(20);
	}

	mif_info("SET GPIO_CP_RESET 0\n");
	gpio_set_value(mc->s5100_gpio_cp_reset, 0);

	mif_info("SET GPIO_CP_PWR 0\n");
	gpio_set_value(mc->s5100_gpio_cp_pwr, 0);

	print_mc_state(mc);

	pcie_clean_dislink(mc);

exit:
	mif_err("---\n");
	return 0;
}

static int s5100_dump_reset(struct modem_ctl *mc)
{
	int __maybe_unused usage_cnt;

	mif_info("%s: %s: +++\n", mc->name, __func__);

	mc->phone_state = STATE_OFFLINE;
	pcie_clean_dislink(mc);
	mif_disable_irq(&mc->s5100_irq_phone_active);
	mif_disable_irq(&mc->s5100_irq_ap_wakeup);

	if (!wake_lock_active(&mc->mc_wake_lock))
		wake_lock(&mc->mc_wake_lock);

	/* Check PCIe linke usage count */
	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	mif_info("pm usage_cnt=%d boot_done_cnt=%d\n", usage_cnt,
			mc->boot_done_cnt);

	mutex_lock(&ap_status_lock);
	if (s5100pcie.link_status == 1) {
		/* save_s5100_status(); */
		mif_err("link_satus:%d\n", s5100pcie.link_status);
		pcie_clean_dislink(mc);
	}
	mutex_unlock(&ap_status_lock);

	mif_err("Set gpio_cp_dump_noti to 1\n");
	gpio_set_value(mc->s5100_gpio_cp_dump_noti, 1);

	mif_info("s5100_cp_reset_required:%d\n", mc->s5100_cp_reset_required);
	if (mc->s5100_cp_reset_required == true) {
		mif_info("SET GPIO_CP_RESET 0\n");
		gpio_set_value(mc->s5100_gpio_cp_reset, 0);
		print_mc_state(mc);

		mif_info("Wait 50ms\n");
		msleep(50);

		mif_info("SET GPIO_CP_RESET 1\n");
		gpio_set_value(mc->s5100_gpio_cp_reset, 1);
		print_mc_state(mc);
	}

	mif_info("Set GPIO ap2cp_status.\n");
	gpio_set_value(mc->s5100_gpio_ap_status, 1);

	mif_err("---\n");

	return 0;
}

static int s5100_reset(struct modem_ctl *mc)
{
	int __maybe_unused usage_cnt;

	mif_info("%s: %s: +++\n", mc->name, __func__);

	mc->phone_state = STATE_OFFLINE;
	pcie_clean_dislink(mc);

	/* Check PCIe linke usage count */
	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	mif_info("pm usage_cnt=%d boot_done_cnt=%d\n", usage_cnt,
			mc->boot_done_cnt);

	mutex_lock(&ap_status_lock);
	if (s5100pcie.link_status == 1) {
		/* save_s5100_status(); */
		mif_err("link_satus:%d\n", s5100pcie.link_status);
		pcie_clean_dislink(mc);
	}
	mutex_unlock(&ap_status_lock);

	mif_info("SET GPIO_CP_RESET 0\n");
	gpio_set_value(mc->s5100_gpio_cp_reset, 0);
	print_mc_state(mc);

	mif_info("Wait 50ms\n");
	msleep(50);

	mif_info("SET GPIO_CP_RESET 1\n");
	gpio_set_value(mc->s5100_gpio_cp_reset, 1);
	print_mc_state(mc);

	mif_err("---\n");

	return 0;
}

static int s5100_boot_on(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	struct io_device *iod;
	struct mem_link_device *mld = to_mem_link_device(ld);

	mif_info("+++\n");

	/* 2cp dump WA */
	if (timer_pending(&mld->crash_ack_timer))
		del_timer(&mld->crash_ack_timer);
	atomic_set(&mld->forced_cp_crash, 0);

	mif_info("Set link mode to LINK_MODE_BOOT.\n");

	if (ld->boot_on)
		ld->boot_on(ld, mc->bootd);

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0)
			iod->modem_state_changed(iod, STATE_BOOTING);
	}

	mif_info("Disable phone actvie interrupt.\n");
	mif_disable_irq(&mc->s5100_irq_phone_active);

	mif_info("Set GPIO ap2cp_status.\n");
	gpio_set_value(mc->s5100_gpio_ap_status, 1);
	mc->phone_state = STATE_BOOTING;

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
	reset_cp_upload_cnt();
#endif

	del_timer(&mld->cp_not_work);

	mif_info("---\n");
	return 0;
}

static int s5100_boot_off(struct modem_ctl *mc)
{
	int err = 0;
	unsigned long remain;
	int cp_active;

	mif_info("+++\n");

	reinit_completion(&mc->init_cmpl);
	remain = wait_for_completion_timeout(&mc->init_cmpl, MIF_INIT_TIMEOUT);
	if (remain == 0) {
		mif_err("T-I-M-E-O-U-T\n");
		err = -EAGAIN;
		goto exit;
	}

	/* Read cp_active before enabling irq */
	cp_active = gpio_get_value(mc->s5100_gpio_phone_active);
	mif_info("cp_active: %d\n", cp_active);

	err = register_phone_active_interrupt(mc);
	if (err)
		mif_err("Err: register_phone_active_interrupt:%d\n", err);
	mif_enable_irq(&mc->s5100_irq_phone_active);

	err = register_cp2ap_wakeup_interrupt(mc);
	if (err)
		mif_err("Err: register_cp2ap_wakeup_interrupt:%d\n", err);
	mif_enable_irq(&mc->s5100_irq_ap_wakeup);

	print_mc_state(mc);

	mif_info("---\n");

exit:
	return err;
}

static int s5100_boot_done(struct modem_ctl *mc)
{
	int usage_cnt;
	struct io_device *iod;

	mif_info("+++\n");

	mc->device_reboot = false;
	s5100pcie.suspend_try = false;

	mc->boot_done_cnt++;

	/* Check PCIe linke usage count */
	usage_cnt = atomic_read(&mc->dev->power.usage_count);
	mif_info("pm usage_cnt=%d boot_done_cnt=%d\n", usage_cnt,
			mc->boot_done_cnt);

	if (usage_cnt != 1) {
		mif_err("ERR! pm usage_cnt=%d\n", usage_cnt);

		if (usage_cnt != 1) {
			atomic_set(&mc->dev->power.usage_count, 1);
			mif_err("new pm usage_cnt=%d\n",
					atomic_read(&mc->dev->power.usage_count));
		}
	}

	list_for_each_entry(iod, &mc->modem_state_notify_list, list) {
		if (iod && atomic_read(&iod->opened) > 0) {
			iod->modem_state_changed(iod, STATE_ONLINE);
		}
	}

	print_mc_state(mc);

	mif_info("enbable irq\n");
	mif_enable_irq(&mc->s5100_irq_phone_active);
	mif_enable_irq(&mc->s5100_irq_ap_wakeup);

	mif_info("---\n");
	return 0;
}

static int s5100_force_crash_exit(struct modem_ctl *mc)
{
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	if (gpio_get_value(mc->s5100_gpio_phone_active)) {
		mif_err("Set s5100_gpio_cp_dump_noti to 1\n");
		gpio_set_value(mc->s5100_gpio_cp_dump_noti, 1);
	} else {
		mif_err("s5100_gpio_phone_active is already 0\n");
	}

	/* Make DUMP start */
	ld->force_dump(ld, mc->bootd);

	mif_err("---\n");
	return 0;
}

int s5100_force_crash_exit_ext(u32 owner, char *reason)
{
	if (g_mc) {
		struct link_device *ld = get_current_link(g_mc->bootd);
		struct mem_link_device *mld = to_mem_link_device(ld);

		mld->crash_reason.owner = owner;
		sprintf(mld->crash_reason.string, "%s", reason);

		s5100_force_crash_exit(g_mc);
	}

	return 0;
}

#if !defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
int modem_force_crash_exit_ext(void)
{
	return s5100_force_crash_exit_ext(CRASH_REASON_MIF_MDM_CTRL, 
					  "forced crash by external");
}
EXPORT_SYMBOL(modem_force_crash_exit_ext);
#endif

int s5100_send_panic_noti_ext(void)
{
	struct modem_data *modem;

	if (g_mc) {
		modem = g_mc->mdm_data;
		if (modem->mld) {
			mif_err("Send CMD_KERNEL_PANIC message to CP\n");
			send_ipc_irq(modem->mld, cmd2int(CMD_KERNEL_PANIC));
		}
	}

	return 0;
}

#if !defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
int modem_send_panic_noti_ext(void)
{
	return s5100_send_panic_noti_ext();
}
EXPORT_SYMBOL(modem_send_panic_noti_ext);
#endif

static int s5100_dump_start(struct modem_ctl *mc)
{
	int err;
	struct link_device *ld = get_current_link(mc->bootd);
	mif_err("+++\n");

	/* Change phone state to CRASH_EXIT */
	mc->phone_state = STATE_CRASH_EXIT;

	if (!ld->dump_start) {
		mif_err("ERR! %s->dump_start not exist\n", ld->name);
		return -EFAULT;
	}

	err = ld->dump_start(ld, mc->bootd);
	if (err)
		return err;

	mif_err("---\n");
	return err;
}

static void s5100_modem_boot_confirm(struct modem_ctl *mc)
{
	/* Do nothing now!! */
}

static int s5100_runtime_suspend(struct modem_ctl *mc)
{
	int wait_time;
	int __maybe_unused usage_cnt = atomic_read(&mc->dev->power.usage_count);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret = 0;
	mif_info("[S5100] Runtime SUSPEND(%d)\n", usage_cnt);

	mutex_lock(&ap_status_lock);
	if (mld->msi_irq_base_enabled == 1) {
		disable_irq(mld->msi_irq_base);
		mld->msi_irq_base_enabled = 0;
	}

	s5100pcie.suspend_try = true;

	if (mc->device_reboot == true) {
		mif_info("skip runtime_suspend : device is rebooting..!!!\n");
		goto done;
	}

	mif_info("set s5100_gpio_cp_wakeup to 0\n");
	gpio_set_value(mc->s5100_gpio_cp_wakeup, 0);
	print_mc_state(mc);

	wait_time = CP2AP_WAKEUP_WAIT_TIME;
	do {
		if (!gpio_get_value(mc->s5100_gpio_ap_wakeup))
			break;

		if ((wait_time % 10) == 0)
			mif_err_limited("Waiting for CP2AP_WAKEUP Low - %dms\n",
				(CP2AP_WAKEUP_WAIT_TIME - wait_time));
		msleep(1);
	} while (--wait_time);

	if (wait_time <= 0) {
		mif_err("CP2AP wakeup is NOT Low!!!(wakeup_gpio:%d)\n",
				gpio_get_value(mc->s5100_gpio_ap_wakeup));
		mutex_unlock(&ap_status_lock);
		pm_runtime_mark_last_busy(mc->dev);

		mld->msi_irq_base_enabled = 1;
		enable_irq(mld->msi_irq_base);
		
		return -EBUSY;
	}

	if (mc->s5100_pdev != NULL && (mc->phone_state == STATE_ONLINE ||
				mc->phone_state == STATE_BOOTING)) {
		mif_info("save s5100_status - phone_state:%d\n",
				mc->phone_state);
		save_s5100_status();
	} else
		mif_info("ignore save_s5100_status - phone_state:%d\n",
				mc->phone_state);

	exynos_pcie_host_v1_poweroff(mc->pcie_ch_num);
	atomic_set(&mc->pcie_pwron, 0);

	s5100pcie.suspend_try = false;
done:
	mutex_unlock(&ap_status_lock);

	if (wake_lock_active(&mc->mc_wake_lock))
		wake_unlock(&mc->mc_wake_lock);

	del_timer(&mld->cp_not_work);

	return ret;
}

static int s5100_runtime_resume(struct modem_ctl *mc)
{
	int wait_time;
	int __maybe_unused usage_cnt = atomic_read(&mc->dev->power.usage_count);
	struct link_device *ld = get_current_link(mc->iod);
	struct mem_link_device *mld = to_mem_link_device(ld);
	int ret;

	if (mc == NULL) {
		mif_info("Skip runtime_resume : mc is NULL\n");
		return 0;
	}

	if (mc->phone_state == STATE_OFFLINE) {
		mif_info("Skip runtime_resume : phone_state is OFFLINE\n");
		return 0;
	}

	mutex_lock(&ap_status_lock);
	mif_info("set s5100_gpio_cp_wakeup to 1\n");
	gpio_set_value(mc->s5100_gpio_cp_wakeup, 1);
	print_mc_state(mc);

	mif_info("[S5100] Runtime RESUME(%d)\n", usage_cnt);

	wait_time = CP2AP_WAKEUP_WAIT_TIME;
	do {
		if (gpio_get_value(mc->s5100_gpio_ap_wakeup))
			break;

		if ((wait_time % 10) == 0)
			mif_err_limited("Waiting for CP2AP_WAKEUP - %dms\n",
				(CP2AP_WAKEUP_WAIT_TIME - wait_time));
		msleep(1);
	} while (--wait_time);

	if (wait_time <= 0) {
		mif_err("CP2AP wakeup is NOT asserted!!!(wakeup_gpio:%d)\n",
				gpio_get_value(mc->s5100_gpio_ap_wakeup));
		mif_err("Make CP crash\n");
		mutex_unlock(&ap_status_lock);

		mld->crash_reason.owner = CRASH_REASON_CP_PCI_LINK_FAIL;
		sprintf(mld->crash_reason.string, "CP2AP wakeup is not asserted");
		s5100_force_crash_exit(mc);
		return 0;
	}

	if (exynos_pcie_host_v1_poweron(mc->pcie_ch_num) != 0) {
		mif_err("PCIE Link powerup failed!!!\n");
		mutex_unlock(&ap_status_lock);

		return 0;
	}
	atomic_set(&mc->pcie_pwron, 1);

	if (mc->s5100_iommu_map_enabled == false) {
		mc->s5100_iommu_map_enabled = true;

		ret = pcie_iommu_map(mc->pcie_ch_num,
			shm_get_s5100_ipc_base(),
			shm_get_s5100_ipc_base(),
			shm_get_s5100_ipc_size(), 7);
		mif_info("pcie_iommu_map ipc - addr:0x%08lx size:0x%08x ret:%d\n",
			shm_get_s5100_ipc_base(),
			shm_get_s5100_ipc_size(), ret);

#ifdef CONFIG_CP_PKTPROC_V2
		ret = pcie_iommu_map(mc->pcie_ch_num,
			shm_get_s5100_ipc_base() + shm_get_s5100_ipc_size(),
			shm_get_s5100_ipc_base() + shm_get_s5100_ipc_size(),
			shm_get_pktproc_v2_size(), 7);
		mif_info("pcie_iommu_map pktproc - addr:0x%08lx size:0x%08x ret:%d\n",
			shm_get_s5100_ipc_base() + shm_get_s5100_ipc_size(),
			shm_get_pktproc_v2_size(), ret);
#endif

		ret = pcie_iommu_map(mc->pcie_ch_num,
			shm_get_s5100_cp2cp_base(),
			shm_get_s5100_cp2cp_base(),
			shm_get_s5100_cp2cp_size(), 7);
		mif_info("pcie_iommu_map cp2cp - addr:0x%08lx size:0x%X ret:%d\n",
			shm_get_s5100_cp2cp_base(),
			shm_get_s5100_cp2cp_size(), ret);
	}

	//DBG: if (mc->s5100_pdev != NULL)
	//DBG: 	restore_s5100_state();

	if (mc->s5100_pdev != NULL) {
		restore_s5100_state();

		/* DBG: check MSI sfr setting values */
		print_msi_register();
	} else {
		mif_err("DBG: MSI sfr not set up, yet(s5100_pdev is NULL)");
	}

	if (mld->msi_irq_base_enabled == 0) {
		enable_irq(mld->msi_irq_base);
		mld->msi_irq_base_enabled = 1;
	}

	if(mc->pcie_registered == true) {

		/* DBG */
		mif_err("DBG: doorbell: mc->pcie_registered = %d\n", mc->pcie_registered);

		if (s5100pcie_send_doorbell_int(mc->int_pcie_link_ack) != 0) {
			/* DBG */
			mif_err("DBG: s5100pcie_send_doorbell_int() func. is failed !!!\n");
			s5100_force_crash_exit_ext(CRASH_REASON_MIF_MDM_CTRL,
						   "fail to send doorbell [2]");
		}
	}

	if (mc->reserve_doorbell_int == true) {
		mif_err("doorbell_int was reserved\n");

		mc->reserve_doorbell_int = false;
		if (s5100pcie_send_doorbell_int(mld->intval_ap2cp_msg) != 0)
			s5100_force_crash_exit_ext(CRASH_REASON_MIF_MDM_CTRL,
						   "fail to send doorbell [3]");
	}

	mutex_unlock(&ap_status_lock);

	// using PCIE 1 lane at usual tput
	if (exynos_pcie_host_v1_lanechange(1, 1)) {
		mif_info("fail to change PCI lane 1\n");
	} else {
		mif_info("change to PCI lane 1\n");
	}

	mod_timer(&mld->cp_not_work, jiffies + mld->not_work_time * HZ);
	return 0;
}

static int s5100_pm_notifier(struct notifier_block *notifier,
				       unsigned long pm_event, void *v)
{
	struct modem_ctl *mc = container_of(notifier, struct modem_ctl,
			pm_notifier);

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		mif_info("Suspend prepare\n");
		atomic_set(&mc->pcie_suspend, 1);
		break;

	case PM_POST_SUSPEND:
		mif_info("Resume done\n");
		atomic_set(&mc->pcie_suspend, 0);
		break;

	default:
		mif_info("pm_event %d\n", pm_event);
		break;
	}

	return NOTIFY_OK;
}

static int s5100_link_down(struct modem_ctl *mc)
{
	pcie_clean_dislink(mc);

	return 0;
}

static int s5100_suspend(struct modem_ctl *mc)
{
	if (gpio_get_value(mc->s5100_gpio_ap_wakeup)) {
		mif_err("s5100_gpio_ap_wakeup asserted !!!\n");
		return -EBUSY;
	}

	if (mc && mc->s5100_gpio_ap_status) {
		gpio_set_value(mc->s5100_gpio_ap_status, 0);
		mif_info("set gpio_ap_status to %d\n", gpio_get_value(mc->s5100_gpio_ap_status));
	}
	return 0;
}

static int s5100_resume(struct modem_ctl *mc)
{
	if (mc && mc->s5100_gpio_ap_status) {
		gpio_set_value(mc->s5100_gpio_ap_status, 1);
		mif_info("set gpio_ap_status to %d\n", gpio_get_value(mc->s5100_gpio_ap_status));
	}
	return 0;
}

int s5100_set_gpio_2cp_uart_sel(struct modem_ctl *mc, int value)
{
	if (mc->s5100_gpio_2cp_uart_sel < 0) {
		mif_err("ERR No s5100_gpio_2cp_uart_sel\n");
		return -EINVAL;
	} else {
		mif_info("Set s5100_gpio_2cp_uart_sel to %d\n", value);
		gpio_set_value(mc->s5100_gpio_2cp_uart_sel, value);
		return 0;
	}
}

int s5100_get_gpio_2cp_uart_sel(struct modem_ctl *mc)
{
	int value;

	if (mc->s5100_gpio_2cp_uart_sel < 0) {
		mif_err("ERR No s5100_gpio_2cp_uart_sel\n");
		return -EINVAL;
	} else {
		value = gpio_get_value(mc->s5100_gpio_2cp_uart_sel);
		mif_info("Get s5100_gpio_2cp_uart_sel to %d\n", value);
		return value;
	}
}

static void s5100_get_ops(struct modem_ctl *mc)
{
	mc->ops.modem_on = s5100_on;
	mc->ops.modem_off = s5100_off;
	mc->ops.modem_shutdown = s5100_shutdown;
	mc->ops.modem_reset = s5100_reset;
	mc->ops.modem_dump_reset = s5100_dump_reset;
	mc->ops.modem_boot_on = s5100_boot_on;
	mc->ops.modem_boot_off = s5100_boot_off;
	mc->ops.modem_boot_done = s5100_boot_done;
	mc->ops.modem_force_crash_exit = s5100_force_crash_exit;
	mc->ops.modem_dump_start = s5100_dump_start;
	mc->ops.modem_boot_confirm = s5100_modem_boot_confirm;
	mc->ops.modem_runtime_suspend = s5100_runtime_suspend;
	mc->ops.modem_runtime_resume = s5100_runtime_resume;
	mc->ops.modem_link_down = s5100_link_down;
	mc->ops.modem_suspend = s5100_suspend;
	mc->ops.modem_resume = s5100_resume;
}

static void s5100_get_pdata(struct modem_ctl *mc, struct modem_data *pdata)
{
	struct platform_device *pdev = to_platform_device(mc->dev);
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	/* CP Power */
	mc->s5100_gpio_cp_pwr = of_get_named_gpio(np, "gpio_ap2cp_cp_pwr_on", 0);
	if (mc->s5100_gpio_cp_pwr < 0) {
		mif_err("Can't Get s5100_gpio_cp_pwr!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_cp_pwr,"gpio_ap2cp_cp_pwr_on");
	gpio_direction_output(mc->s5100_gpio_cp_pwr, 0);

	/* CP Reset */
	mc->s5100_gpio_cp_reset = of_get_named_gpio(np, "gpio_ap2cp_nreset_n", 0);
	if (mc->s5100_gpio_cp_reset < 0) {
		mif_err("Can't Get gpio_cp_nreset_n!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_cp_reset,"gpio_ap2cp_nreset_n");
	gpio_direction_output(mc->s5100_gpio_cp_reset, 0);

	/* CP PS HOLD */
	mc->s5100_gpio_cp_ps_hold = of_get_named_gpio(np, "gpio_cp2ap_cp_ps_hold", 0);
	if (mc->s5100_gpio_cp_ps_hold < 0) {
		mif_err("Can't Get s5100_gpio_cp_ps_hold!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_cp_ps_hold, "gpio_cp2ap_cp_ps_hold");
	gpio_direction_input(mc->s5100_gpio_cp_ps_hold);

	/* AP2CP WAKE UP */
	mc->s5100_gpio_cp_wakeup = of_get_named_gpio(np, "gpio_ap2cp_wake_up", 0);
	if (mc->s5100_gpio_cp_wakeup < 0) {
		mif_err("Can't Get s5100_gpio_cp_wakeup!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_cp_wakeup,"gpio_ap2cp_wake_up");
	gpio_direction_output(mc->s5100_gpio_cp_wakeup, 0);

	/* CP2AP WAKE UP */
	mc->s5100_gpio_ap_wakeup = of_get_named_gpio(np, "gpio_cp2ap_wake_up", 0);
	if (mc->s5100_gpio_ap_wakeup < 0) {
		mif_err("Can't Get gpio_cp2ap_wake_up!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_ap_wakeup,"gpio_cp2ap_wake_up");
	mc->s5100_irq_ap_wakeup.num = gpio_to_irq(mc->s5100_gpio_ap_wakeup);

	/* DUMP NOTI */
	mc->s5100_gpio_cp_dump_noti = of_get_named_gpio(np, "gpio_ap2cp_dump_noti", 0);
	if (mc->s5100_gpio_cp_dump_noti < 0) {
		mif_err("Can't Get gpio_ap2cp_dump_noti!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_cp_dump_noti,"s5100_ap2cp_dump_noti");
	gpio_direction_output(mc->s5100_gpio_cp_dump_noti, 0);

	/* PDA ACTIVE */
	mc->s5100_gpio_ap_status = of_get_named_gpio(np, "gpio_ap2cp_pda_active", 0);
	if (mc->s5100_gpio_ap_status < 0) {
		mif_err("Can't Get s5100_gpio_ap_status!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_ap_status,"gpio_ap2cp_pda_active");
	gpio_direction_output(mc->s5100_gpio_ap_status, 0);

#if defined(CONFIG_SEC_SIPC_DUAL_MODEM_IF)
	/* 2CP UART SEL */
	mc->s5100_gpio_2cp_uart_sel = of_get_named_gpio(np, "gpio_2cp_uart_sel", 0);
	if (mc->s5100_gpio_2cp_uart_sel < 0) {
		mif_err("Can't Get s5100_gpio_2cp_uart_sel!\n");
	} else {
		gpio_request(mc->s5100_gpio_2cp_uart_sel,"gpio_2cp_uart_sel");
		gpio_direction_output(mc->s5100_gpio_2cp_uart_sel, 0);
	}
#endif

	/* PHONE ACTIVE */
	mc->s5100_gpio_phone_active = of_get_named_gpio(np, "gpio_cp2ap_phone_active", 0);
	if (mc->s5100_gpio_phone_active < 0) {
		mif_err("Can't Get s5100_gpio_phone_active!\n");
		return;
	}
	gpio_request(mc->s5100_gpio_phone_active,"gpio_cp2ap_phone_active");
	mc->s5100_irq_phone_active.num = gpio_to_irq(mc->s5100_gpio_phone_active);

	ret = of_property_read_u32(np, "mif,int_ap2cp_pcie_link_ack",
				&mc->int_pcie_link_ack);
	if (ret) {
		mif_err("Can't Get PCIe Link ACK interrupt number!!!\n");
		return;
	}
	mc->int_pcie_link_ack += DOORBELL_INT_ADD;

	s5100pcie_set_cp_wake_gpio(mc->s5100_gpio_cp_wakeup);

	/* Get PCIe Channel Number */
	ret = of_property_read_u32(np, "pci_ch_num",
				&mc->pcie_ch_num);
	if (ret) {
		mif_err("Can't Get PCIe channel!!!\n");
		return;
	}
	mif_info("S5100 PCIe Chennel Number : %d\n", mc->pcie_ch_num);
}

int s5100_recover_pcie_link(bool start)
{
	if (g_mc == NULL)
		return -ENODEV;

	mif_err("s5100_recover_pcie_link:%d\n", start);

	g_mc->recover_pcie_link = start;

	return 0;
}

#ifdef CONFIG_EXYNOS_BUSMONITOR
static int s5100_busmon_notifier(struct notifier_block *nb,
						unsigned long event, void *data)
{
	struct busmon_notifier *info = (struct busmon_notifier *)data;
	char *init_desc = info->init_desc;

	if (init_desc != NULL &&
		(strncmp(init_desc, "CP", strlen(init_desc)) == 0 ||
		strncmp(init_desc, "APB_CORE_CP", strlen(init_desc)) == 0 ||
		strncmp(init_desc, "MIF_CP", strlen(init_desc)) == 0)) {
		struct modem_ctl *mc =
			container_of(nb, struct modem_ctl, busmon_nfb);

		s5100_force_crash_exit(mc);
	}
	return 0;
}
#endif

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
static int s5100_modem_notifier(struct notifier_block *nb,
		unsigned long action, void *nb_data)
{
	struct modem_ctl *mc = container_of(nb, struct modem_ctl, modem_nb);
	struct link_device *ld = get_current_link(mc->iod);
	struct modem_ctl *origin_mc = nb_data;

	mif_info("action:%lu\n", action);

	switch (action) {
	case MODEM_EVENT_EXIT:
	case MODEM_EVENT_WATCHDOG:
		if (origin_mc != mc) {
			if (ld->link_type == cp_crash_link)
				s5100_force_crash_exit(mc);
			else
				s5100_force_crash_exit_ext(CRASH_REASON_MIF_MDM_CTRL,
							   "triggered by another modem");
			break;
		}
		break;
	}

	return NOTIFY_DONE;
}
#endif

int s5100_init_modemctl_device(struct modem_ctl *mc, struct modem_data *pdata)
{
#ifdef CONFIG_PM
	struct platform_device *pdev = to_platform_device(mc->dev);
#endif
	int ret = 0;
	struct resource __maybe_unused *sysram_alive;

	mif_err("+++\n");

	g_mc = mc;

	s5100_get_ops(mc);
	s5100_get_pdata(mc, pdata);
	dev_set_drvdata(mc->dev, mc);

	wake_lock_init(&mc->mc_wake_lock, WAKE_LOCK_SUSPEND, "s5100_wake_lock");

	gpio_set_value(mc->s5100_gpio_cp_reset, 0);
	gpio_set_value(mc->gpio_cp_on, 0);

	mif_err("Register GPIO interrupts\n");
	mc->apwake_irq_chip = irq_get_chip(mc->s5100_irq_ap_wakeup.num);
	if (mc->apwake_irq_chip == NULL) {
		mif_err("Can't get irq_chip structure!!!!\n");
		return -EINVAL;
	}

	mc->wakeup_wq = create_singlethread_workqueue("cp2ap_wakeup_wq");
	if (!mc->wakeup_wq) {
		mif_err("%s: ERR! fail to create wakeup_wq\n", mc->name);
		return -EINVAL;
	}

	INIT_WORK(&mc->link_work, pcie_link_work);
	INIT_WORK(&mc->dislink_work, pcie_dislink_work);
	INIT_DELAYED_WORK(&mc->nr2ap_wakeup_work, ap_wakeup_work);

	ret = device_create_file(mc->dev, &dev_attr_modem_ctrl);
	ret |= device_create_file(mc->dev, &dev_attr_detect_time);
	if (ret)
		mif_err("can't create modem_ctrl!!!\n");

	/* For disable runtime pm */
	register_reboot_notifier(&nb_reboot_block);

	/* Register PM notifier_call */
	mc->pm_notifier.notifier_call = s5100_pm_notifier;
	ret = register_pm_notifier(&mc->pm_notifier);
	if (ret) {
		mif_err("failed to register PM notifier_call\n");
		return ret;
	}

	init_pinctl_cp2ap_wakeup(mc);

#ifdef CONFIG_SEC_SIPC_DUAL_MODEM_IF
	mc->modem_nb.notifier_call = s5100_modem_notifier;
	register_modem_event_notifier(&mc->modem_nb);
#endif

#ifdef CONFIG_PM
	pr_info("S5100: Runtime PM Initialization...\n");
	pm_runtime_set_autosuspend_delay(&pdev->dev, AUTOSUSPEND_TIMEOUT);
	pm_runtime_use_autosuspend(&pdev->dev);
	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);
#endif

	mif_err("---\n");

	return 0;
}