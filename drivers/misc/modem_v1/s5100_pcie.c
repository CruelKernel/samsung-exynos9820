/* linux/drivers/modem/modem.c
 *
 * Copyright (C) 2010 Google, Inc.
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
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/if_arp.h>
#include <linux/version.h>

#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#endif
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>

#include <linux/exynos-pci-ctrl.h>
#include <linux/exynos-pci-noti.h>

#include "modem_prj.h"
#include "modem_utils.h"
#include "link_device_memory.h"
#include "s5100_pcie.h"

struct s5100pcie s5100pcie;
void quirk_s350_set_atu1(struct pci_dev *dev);

static int s5100pcie_read_procmem(struct seq_file *m, void *v)
{
	pr_err("Procmem READ!\n");

	return 0;
}
static int s5100pcie_proc_open(struct inode *inode, struct  file *file) {
	return single_open(file, s5100pcie_read_procmem, NULL);
}

static const struct file_operations s5100pcie_proc_fops = {
	.owner = THIS_MODULE,
	.open = s5100pcie_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

inline int s5100pcie_send_doorbell_int(int int_num)
{
	u32 reg, count = 0;
	int cnt = 0;
	u16 cmd;

	s5100_check_doorbell_ready();

	if (s5100pcie.link_status == 0) {
		mif_err_limited("Can't send Interrupt(not enabled)!!!\n");
		return -EAGAIN;
	}

	if (exynos_check_pcie_link_status(s5100pcie.pcie_channel_num) == 0) {
		mif_err_limited("Can't send Interrupt(not linked)!!!\n");
		return -EAGAIN;
	}

	pci_read_config_word(s5100pcie.s5100_pdev, PCI_COMMAND, &cmd);
	if (((cmd & PCI_COMMAND_MASTER) == 0) || (cmd == 0xffff)) {
		mif_err_limited("Can't send Interrupt(not setted bme_en, 0x%04x)!!!\n", cmd);

		do {
			cnt++;

			pci_set_master(s5100pcie.s5100_pdev);

			pci_read_config_word(s5100pcie.s5100_pdev, PCI_COMMAND, &cmd);
			mif_info("cmd reg = 0x%04x \n", cmd);

			if ((cmd & PCI_COMMAND_MASTER) && (cmd != 0xffff))
				break;
		} while (cnt < 10);

		if (cnt >= 10) {
			mif_err_limited("BME is not set(cnt=%d)\n", cnt);
			return -EAGAIN;
		}
	}

send_doorbell_again:
	iowrite32(int_num, s5100pcie.doorbell_addr);

	reg = ioread32(s5100pcie.doorbell_addr);

	/* debugging: */
	mif_debug("DBG: s5100pcie.doorbell_addr = 0x%x - written(int_num=0x%x) read(reg=0x%x)\n", \
		s5100pcie.doorbell_addr, int_num, reg);

	if (reg == 0xffffffff) {
		count++;
		if (count < 100) {
			if (!in_interrupt())
				udelay(1000); /* 1ms */
			else {
				mif_err_limited("Can't send doorbell in interrupt mode (0x%08X)\n"
						, reg);
				return 0;
			}

#ifdef CONFIG_LTE_MODEM_S5100
			mif_err("retry set atu cnt = %d\n", count);
			quirk_s350_set_atu1(s5100pcie.s5100_pdev);
#endif

			goto send_doorbell_again;
		}
		mif_err("[Need to CHECK] Can't send doorbell int (0x%x)\n", reg);
		exynos_pcie_host_v1_register_dump(s5100pcie.pcie_channel_num);

		return -EAGAIN;
	}

	return 0;
}

struct pci_dev *s5100pcie_get_pcidev(void)
{
	if (s5100pcie.s5100_pdev != NULL) {
		return s5100pcie.s5100_pdev;
	} else {
		pr_err("PCI device is NOT initialzed!!!\n");
		return NULL;
	}

}

void s5100pcie_set_cp_wake_gpio(int cp_wakeup)
{
	s5100pcie.gpio_cp_wakeup = cp_wakeup;
}

void save_s5100_status()
{
	if (exynos_check_pcie_link_status(s5100pcie.pcie_channel_num) == 0) {
		mif_err("It's not Linked - Ignore saving the s5100\n");
		return;
	}

	/* pci_pme_active(s5100pcie.s5100_pdev, 0); */

	/* Disable L1.2 before PCIe power off */
	/* exynos_pcie_host_v1_l1ss_ctrl(0, PCIE_L1SS_CTRL_MODEM_IF); */

	pci_clear_master(s5100pcie.s5100_pdev);

	pci_save_state(s5100pcie.s5100_pdev);

	s5100pcie.pci_saved_configs = pci_store_saved_state(s5100pcie.s5100_pdev);

	disable_msi_int();

	/* pci_enable_wake(s5100pcie.s5100_pdev, PCI_D0, 0); */

	pci_disable_device(s5100pcie.s5100_pdev);

	pci_wake_from_d3(s5100pcie.s5100_pdev, false);
	if (pci_set_power_state(s5100pcie.s5100_pdev, PCI_D3hot)) {
		mif_err("Can't set D3 state!!!!\n");
	}

}

void restore_s5100_state()
{
	int ret;

	if (exynos_check_pcie_link_status(s5100pcie.pcie_channel_num) == 0) {
		mif_err("It's not Linked - Ignore restoring the s5100_status!\n");
		return;
	}

	if (pci_set_power_state(s5100pcie.s5100_pdev, PCI_D0)) {
		mif_err("Can't set D0 state!!!!\n");
	}

	pci_load_saved_state(s5100pcie.s5100_pdev, s5100pcie.pci_saved_configs);
	pci_restore_state(s5100pcie.s5100_pdev);

	pci_enable_wake(s5100pcie.s5100_pdev, PCI_D0, 0);
	/* pci_enable_wake(s5100pcie.s5100_pdev, PCI_D3hot, 0); */

	ret = pci_enable_device(s5100pcie.s5100_pdev);

	if (ret) {
		pr_err("Can't enable PCIe Device after linkup!\n");
	}
	pci_set_master(s5100pcie.s5100_pdev);

#ifdef CONFIG_DISABLE_PCIE_CP_L1_2
	/* Disable L1.2 after PCIe power on */
	exynos_pcie_host_v1_l1ss_ctrl(0, PCIE_L1SS_CTRL_MODEM_IF);
#else
	/* Enable L1.2 after PCIe power on */
	exynos_pcie_host_v1_l1ss_ctrl(1, PCIE_L1SS_CTRL_MODEM_IF);
#endif

	s5100pcie.link_status = 1;
	/* pci_pme_active(s5100pcie.s5100_pdev, 1); */
}

void disable_msi_int()
{
	s5100pcie.link_status = 0;
	/* It's not needed now...
	 * pci_disable_msi(s5100pcie.s5100_pdev);
	 * pci_config_pm_runtime_put(&s5100pcie.s5100_pdev->dev);
	 */

}

int s5100pcie_request_msi_int(int int_num)
{
	int err = -EFAULT;

	pr_err("Enable MSI interrupts : %d\n", int_num);

	if (int_num > MAX_MSI_NUM) {
		pr_err("Too many MSI interrupts are requested(<=16)!!!\n");
		return -EFAULT;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,18,0))
	/* used 'pci_enable_msi_range()' before kernel 4.14 ver.:
	err = __pci_enable_msi_range(s5100pcie.s5100_pdev, int_num, int_num); */
	err = pci_alloc_irq_vectors_affinity(s5100pcie.s5100_pdev, int_num, int_num, PCI_IRQ_MSI, NULL);
#else
	err = pci_enable_msi_block(s5100pcie.s5100_pdev, int_num);
#endif

	if (err <= 0) {
		printk("Can't get msi IRQ!!!!!\n");
		return -EFAULT;
	}

	return s5100pcie.s5100_pdev->irq;
}

extern int s5100_force_crash_exit_ext(void);

static void s5100pcie_linkdown_cb(struct exynos_pcie_notify *noti)
{
	struct pci_dev __maybe_unused *pdev = (struct pci_dev *)noti->user;

	pr_err("S5100 Link-Down notification callback function!!!\n");

	s5100_force_crash_exit_ext();
}

static int s5100pcie_probe(struct pci_dev *pdev,
			const struct pci_device_id *ent)
{
	int ret;
	int __maybe_unused i;

	pr_info("S5100 EP driver Probe(%s)\n", __func__);
	s5100pcie.s5100_pdev = pdev;
	s5100pcie.irq_num_base = pdev->irq;
	s5100pcie.link_status = 1;

	pr_err("Disable BAR resources.\n");
	for (i = 0; i < 6; i++) {
		pdev->resource[i].start = 0x0;
		pdev->resource[i].end = 0x0;
		pdev->resource[i].flags = 0x82000000;
		pci_update_resource(pdev, i);
	}

	/* EP(S5100) BAR setup: BAR0 0x12d0_0000~0x12d0_0fff (4kB) */
	pdev->resource[0].start = 0x12d00000;
	pdev->resource[0].end = (0x12d00000 + 0xfff);
	pdev->resource[0].flags = 0x82000000;
	pci_update_resource(pdev, 0);

	pr_err("Set Doorbell register addres.\n");
#if defined(CONFIG_SOC_EXYNOS8890)
	if (s5100pcie.pcie_channel_num == 0)
		s5100pcie.doorbell_addr = devm_ioremap_wc(&pdev->dev,
					0x1c002c20, SZ_4);
	else
		s5100pcie.doorbell_addr = devm_ioremap_wc(&pdev->dev,
					0x1e002c20, SZ_4);
#elif defined(CONFIG_SOC_EXYNOS8895)
	if (s5100pcie.pcie_channel_num == 0)
		s5100pcie.doorbell_addr = devm_ioremap_wc(&pdev->dev,
					0x11802c20, SZ_4);
	else
		s5100pcie.doorbell_addr = devm_ioremap_wc(&pdev->dev,
					0x11c02c20, SZ_4);
#elif defined(CONFIG_SOC_EXYNOS9820)
	/* doorbell target address: (RC)0x1100_0d20 ->(EP)0x12d0_0d20 */
	if (s5100pcie.pcie_channel_num == 0)
		s5100pcie.doorbell_addr = devm_ioremap_wc(&pdev->dev,
					0x0, SZ_4); /* ch0: TBD */
	else
		s5100pcie.doorbell_addr = devm_ioremap_wc(&pdev->dev,
					0x11000d20, SZ_4);

	pr_info("s5100pcie.doorbell_addr = 0x%x (CONFIG_SOC_EXYNOS9820: 0x11000d20)\n", \
		s5100pcie.doorbell_addr);
#else
	#error "Can't set Doorbell interrupt register!"
#endif

	if (s5100pcie.doorbell_addr == NULL)
		pr_err("Can't ioremap doorbell address!!!\n");


	pr_info("Register PCIE notification event...\n");
	s5100pcie.pcie_event.events = EXYNOS_PCIE_EVENT_LINKDOWN;
	s5100pcie.pcie_event.user = pdev;
	s5100pcie.pcie_event.mode = EXYNOS_PCIE_TRIGGER_CALLBACK;
	s5100pcie.pcie_event.callback = s5100pcie_linkdown_cb;
	exynos_pcie_host_v1_register_event(&s5100pcie.pcie_event);

	pr_err("Enable PCI device...\n");
	ret = pci_enable_device(pdev);

	pci_set_master(pdev);

	pci_set_drvdata(pdev, &s5100pcie);

	return 0;
}

void print_msi_register()
{
	u32 msi_val;

	pci_read_config_dword(s5100pcie.s5100_pdev, 0x50, &msi_val);
	mif_err("MSI Control Reg(0x50) : 0x%x\n", msi_val);
	pci_read_config_dword(s5100pcie.s5100_pdev, 0x54, &msi_val);
	mif_err("MSI Message Reg(0x54) : 0x%x\n", msi_val);
	pci_read_config_dword(s5100pcie.s5100_pdev, 0x58, &msi_val);
	mif_err("MSI MsgData Reg(0x58) : 0x%x\n", msi_val);

	if (msi_val == 0x0) {
		mif_err("MSI Message Reg == 0x0 - set MSI again!!!\n");
		pci_restore_msi_state(s5100pcie.s5100_pdev);

		mif_err("exynos_pcie_msi_init_ext is not implemented\n");
		/* exynos_pcie_msi_init_ext(s5100pcie.pcie_channel_num); */

		pci_read_config_dword(s5100pcie.s5100_pdev, 0x50, &msi_val);
		mif_err("Recheck - MSI Control Reg : 0x%x (0x50)\n", msi_val);
		pci_read_config_dword(s5100pcie.s5100_pdev, 0x54, &msi_val);
		mif_err("Recheck - MSI Message Reg : 0x%x (0x54)\n", msi_val);
		pci_read_config_dword(s5100pcie.s5100_pdev, 0x58, &msi_val);
		mif_err("Recheck - MSI MsgData Reg : 0x%x (0x58)\n", msi_val);
	}
}

static void s5100pcie_remove(struct pci_dev *pdev)
{
	pr_err("S5100 PCIe Remove!!!\n");

	pci_release_regions(pdev);
}

static int __init s5100_reserved_mem_setup(struct reserved_mem *rmem)
{
	pr_info("reserved memory: %s = %#lx @ %pK\n",
			rmem->name, (unsigned long) rmem->size, &rmem->base);

	return 0;
}
RESERVEDMEM_OF_DECLARE(s5100_pcie, "modem-s5100-shmem", s5100_reserved_mem_setup);

static int __init s5100_reserved_msi_mem(struct reserved_mem *rmem)
{
	pr_info("reserved memory: %s = %#lx @ %pK\n",
			rmem->name, (unsigned long) rmem->size, &rmem->base);

	return 0;
}
RESERVEDMEM_OF_DECLARE(s5100_pcie_msi, "s5100-msi", s5100_reserved_msi_mem);

/* For Test */
static struct pci_device_id s5100_pci_id_tbl[] =
{
	{ PCI_VENDOR_ID_SAMSUNG, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, },   // SC Basic
	{ }
};

MODULE_DEVICE_TABLE(pci, s5100_pci_id_tbl);

static struct pci_driver s5100_driver = {
	.name = "s5100",
	.id_table = s5100_pci_id_tbl,
	.probe = s5100pcie_probe,
	.remove = s5100pcie_remove,
};

/*
 * Initialize PCIe S5100 EP driver.
 */
int s5100pcie_init(int ch_num)
{
	int ret;

	pr_err("Register PCIE drvier for S5100.\n");
	s5100pcie.pcie_channel_num = ch_num;
	ret = pci_register_driver(&s5100_driver);

	/* Create PROC fs */
	proc_create("driver/s5100pcie_proc", 0, NULL, &s5100pcie_proc_fops);

	return 0;
}

static void __exit s5100pcie_exit(void)
{
	pci_unregister_driver(&s5100_driver);
}
module_exit(s5100pcie_exit);
