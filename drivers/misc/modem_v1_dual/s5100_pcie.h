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

#ifndef __S5100_PCIE_H__
#define __S5100_PCIE_H__

#include <linux/exynos-pci-noti.h>

#define MAX_MSI_NUM	(16)

extern int s5100pcie_init(int ch_num);
extern void first_save_s5100_status(void);
extern int exynos_pcie_host_v1_register_event(struct exynos_pcie_register_event *reg);
extern void exynos_pcie_host_v1_register_dump(int ch_num);

struct s5100pcie {
	unsigned int busdev_num;
	int pcie_channel_num;
	struct pci_dev *s5100_pdev;
	int irq_num_base;
	u32 __iomem *doorbell_addr;
	u32 __iomem *reg_base;

	unsigned gpio_cp_wakeup;
	u32 link_status;
	bool suspend_try;

	struct exynos_pcie_register_event pcie_event;
	struct pci_saved_state *pci_saved_configs;
};

extern struct s5100pcie s5100pcie;

extern int exynos_pcie_host_v1_poweron(int ch_num);
extern int exynos_pcie_host_v1_poweroff(int ch_num);
extern int exynos_check_pcie_link_status(int ch_num);
extern int pci_alloc_irq_vectors_affinity(struct pci_dev *dev, unsigned int min_vecs, \
                                    unsigned int max_vecs, unsigned int flags, \
				    const struct irq_affinity *affd);
extern int exynos_pcie_host_v1_l1ss_ctrl(int enable, int id);
extern int pcie_iommu_map(int ch_num, unsigned long iova, phys_addr_t paddr,
				size_t size, int prot);

#define AUTOSUSPEND_TIMEOUT	50

int s5100pcie_request_msi_int(int int_num);
void __iomem *s5100pcie_get_doorbell_address(void);
struct pci_dev *s5100pcie_get_pcidev(void);
void s5100pcie_set_cp_wake_gpio(int cp_wakeup);
int s5100pcie_send_doorbell_int(int int_num);
void save_s5100_status(void);
void restore_s5100_state(void);
void disable_msi_int(void);
void print_msi_register(void);
int s5100_recover_pcie_link(bool start);
int request_pcie_msi_int(struct link_device *ld,
				struct platform_device *pdev);
int s5100_force_crash_exit_ext(u32 owner, char *reason);
int s5100_send_panic_noti_ext(void);
int s5100_set_gpio_2cp_uart_sel(struct modem_ctl *mc, int value);
int s5100_get_gpio_2cp_uart_sel(struct modem_ctl *mc);

// PCIE Dynamic lane change : from //drivers/pci
int exynos_pcie_host_v1_lanechange(int ch_num, int lane);

#endif /* __S5100_PCIE_H__ */
