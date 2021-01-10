/* linux/drivers/iommu/exynos_iommu.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/of.h>
#include <linux/of_iommu.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/slab.h>
#include <linux/smc.h>
#include <linux/swap.h>
#include <linux/swapops.h>

#include <asm/cacheflush.h>
#include <asm/pgtable.h>

#include <dt-bindings/sysmmu/sysmmu.h>

#include "exynos-iommu.h"
#include "exynos-iommu-reg.h"

/* Default IOVA region: [0x1000000, 0xD0000000) */
#define IOVA_START		0x10000000
#define IOVA_END		0xD0000000
#define IOVA_OVFL(addr, size)	((((addr) + (size)) > 0xFFFFFFFF) ||	\
				((addr) + (size) < (addr)))

static struct kmem_cache *lv2table_kmem_cache;

static struct sysmmu_drvdata *sysmmu_drvdata_list;
static struct exynos_iommu_owner *sysmmu_owner_list;

struct sysmmu_list_data {
	struct device *sysmmu;
	struct list_head node;
};

struct exynos_client {
	struct list_head list;
	struct device_node *master_np;
	struct exynos_iovmm *vmm_data;
};
static LIST_HEAD(exynos_client_list);
static DEFINE_SPINLOCK(exynos_client_lock);

struct owner_fault_info {
	struct device *master;
	struct notifier_block nb;
};

static struct dentry *exynos_sysmmu_debugfs_root;

int exynos_client_add(struct device_node *np, struct exynos_iovmm *vmm_data)
{
	struct exynos_client *client = kzalloc(sizeof(*client), GFP_KERNEL);

	if (!client)
		return -ENOMEM;

	INIT_LIST_HEAD(&client->list);
	client->master_np = np;
	client->vmm_data = vmm_data;
	spin_lock(&exynos_client_lock);
	list_add_tail(&client->list, &exynos_client_list);
	spin_unlock(&exynos_client_lock);

	return 0;
}

static int iova_from_sent(sysmmu_pte_t *base, sysmmu_pte_t *sent)
{
	return ((unsigned long)sent - (unsigned long)base) *
				(SECT_SIZE / sizeof(sysmmu_pte_t));
}

#define has_sysmmu(dev)		((dev)->archdata.iommu != NULL)

/* For ARM64 only */
static inline void pgtable_flush(void *vastart, void *vaend)
{
	__dma_flush_area(vastart, vaend - vastart);
}

void exynos_sysmmu_tlb_invalidate(struct iommu_domain *iommu_domain,
					dma_addr_t d_start, size_t size)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	struct exynos_iommu_owner *owner;
	struct sysmmu_list_data *list;
	sysmmu_iova_t start = (sysmmu_iova_t)d_start;
	unsigned long flags;

	spin_lock_irqsave(&domain->lock, flags);
	list_for_each_entry(owner, &domain->clients_list, client) {
		list_for_each_entry(list, &owner->sysmmu_list, node) {
			struct sysmmu_drvdata *drvdata = dev_get_drvdata(list->sysmmu);

			spin_lock(&drvdata->lock);
			if (!is_runtime_active_or_enabled(drvdata) ||
					!is_sysmmu_active(drvdata)) {
				spin_unlock(&drvdata->lock);
				dev_dbg(drvdata->sysmmu,
					"Skip TLB invalidation %#zx@%#x\n",
							size, start);
				continue;
			}

			dev_dbg(drvdata->sysmmu,
				"TLB invalidation %#zx@%#x\n", size, start);

			__sysmmu_tlb_invalidate(drvdata, start, size);

			spin_unlock(&drvdata->lock);
		}
	}
	spin_unlock_irqrestore(&domain->lock, flags);
}

static void sysmmu_get_interrupt_info(struct sysmmu_drvdata *data,
			int *flags, unsigned long *addr, bool is_secure)
{
	unsigned long itype;
	u32 info;

	itype =  __ffs(__sysmmu_get_intr_status(data, is_secure));
	if (WARN_ON(!(itype < SYSMMU_FAULT_UNKNOWN)))
		itype = SYSMMU_FAULT_UNKNOWN;
	else
		*addr = __sysmmu_get_fault_address(data, is_secure);

	info = __sysmmu_get_fault_trans_info(data, is_secure);
	*flags = MMU_IS_READ_FAULT(info) ?
		IOMMU_FAULT_READ : IOMMU_FAULT_WRITE;
	*flags |= SYSMMU_FAULT_FLAG(itype);
}

irqreturn_t exynos_sysmmu_irq_secure(int irq, void *dev_id)
{
	struct sysmmu_drvdata *drvdata = dev_id;
	unsigned long addr = -1;
	int flags = 0;

	dev_err(drvdata->sysmmu, "Secure irq occured!\n");
	if (!drvdata->securebase) {
		dev_err(drvdata->sysmmu, "Unknown interrupt occurred\n");
		BUG();
	} else {
		dev_err(drvdata->sysmmu, "Secure base = %#lx\n",
				(unsigned long)drvdata->securebase);
	}

	sysmmu_get_interrupt_info(drvdata, &flags, &addr, true);
	show_secure_fault_information(drvdata, flags, addr);
	atomic_notifier_call_chain(&drvdata->fault_notifiers, addr, &flags);

	BUG();

	return IRQ_HANDLED;
}

static irqreturn_t exynos_sysmmu_irq(int irq, void *dev_id)
{
	struct sysmmu_drvdata *drvdata = dev_id;
	unsigned long addr = -1;
	int flags = 0, ret;

	dev_info(drvdata->sysmmu, "%s:%d: irq(%d) happened\n", __func__, __LINE__, irq);

	WARN(!is_sysmmu_active(drvdata),
		"Fault occurred while System MMU %s is not enabled!\n",
		dev_name(drvdata->sysmmu));

	sysmmu_get_interrupt_info(drvdata, &flags, &addr, false);
	ret = show_fault_information(drvdata, flags, addr);
	if (ret == -EAGAIN)
		return IRQ_HANDLED;
	atomic_notifier_call_chain(&drvdata->fault_notifiers, addr, &flags);

	panic("Unrecoverable System MMU Fault!!");

	return IRQ_HANDLED;
}

static int sysmmu_get_hw_info(struct sysmmu_drvdata *data)
{
	struct tlb_props *tlb_props = &data->tlb_props;

	data->version = __sysmmu_get_hw_version(data);

	/*
	 * If CAPA1 doesn't exist, sysmmu uses TLB way dedication.
	 * If CAPA1[31:28] is zero, sysmmu uses TLB port dedication.
	 */
	if (!__sysmmu_has_capa1(data))
		tlb_props->flags |= TLB_TYPE_WAY;
	else if (__sysmmu_get_capa_type(data) == 0)
		tlb_props->flags |= TLB_TYPE_PORT;

	return 0;
}

static int __init __sysmmu_secure_irq_init(struct device *sysmmu,
				     struct sysmmu_drvdata *drvdata)
{
	struct platform_device *pdev = to_platform_device(sysmmu);
	u32 secure_reg;
	int ret;

	ret = platform_get_irq(pdev, 1);
	if (ret <= 0) {
		dev_err(sysmmu, "Unable to find secure IRQ resource\n");
		return -EINVAL;
	}
	dev_info(sysmmu, "Registering secure irq %d\n", ret);

	ret = devm_request_irq(sysmmu, ret, exynos_sysmmu_irq_secure, 0,
			dev_name(sysmmu), drvdata);
	if (ret) {
		dev_err(sysmmu, "Failed to register secure irq handler\n");
		return ret;
	}

	ret = of_property_read_u32(sysmmu->of_node,
				"sysmmu,secure_base", &secure_reg);
	if (!ret) {
		drvdata->securebase = secure_reg;
		dev_info(sysmmu, "Secure base = %#x\n", drvdata->securebase);
	} else {
		dev_err(sysmmu, "Failed to get secure register\n");
		return ret;
	}

	return ret;
}

static int __init sysmmu_parse_tlb_way_dt(struct device *sysmmu,
				struct sysmmu_drvdata *drvdata)
{
	const char *props_name = "sysmmu,tlb_property";
	struct tlb_props *tlb_props = &drvdata->tlb_props;
	struct tlb_priv_id *priv_id_cfg = NULL;
	struct tlb_priv_addr *priv_addr_cfg = NULL;
	int i, cnt, priv_id_cnt = 0, priv_addr_cnt = 0;
	unsigned int priv_id_idx = 0, priv_addr_idx = 0;
	unsigned int prop;
	int ret;

	/* Parsing TLB way properties */
	cnt = of_property_count_u32_elems(sysmmu->of_node, props_name);
	for (i = 0; i < cnt; i+=2) {
		ret = of_property_read_u32_index(sysmmu->of_node,
			props_name, i, &prop);
		if (ret) {
			dev_err(sysmmu, "failed to get property."
				       "cnt = %d, ret = %d\n", i, ret);
			return -EINVAL;
		}

		switch (prop & WAY_TYPE_MASK) {
		case _PRIVATE_WAY_ID:
			priv_id_cnt++;
			tlb_props->flags |= TLB_WAY_PRIVATE_ID;
			break;
		case _PRIVATE_WAY_ADDR:
			priv_addr_cnt++;
			tlb_props->flags |= TLB_WAY_PRIVATE_ADDR;
			break;
		case _PUBLIC_WAY:
			tlb_props->flags |= TLB_WAY_PUBLIC;
			tlb_props->way_props.public_cfg = (prop & ~WAY_TYPE_MASK);
			break;
		default:
			dev_err(sysmmu, "Undefined properties!: %#x\n", prop);
			break;
		}
	}

	if (priv_id_cnt) {
		priv_id_cfg = kzalloc(sizeof(*priv_id_cfg) * priv_id_cnt,
								GFP_KERNEL);
		if (!priv_id_cfg)
			return -ENOMEM;
	}

	if (priv_addr_cnt) {
		priv_addr_cfg = kzalloc(sizeof(*priv_addr_cfg) * priv_addr_cnt,
								GFP_KERNEL);
		if (!priv_addr_cfg) {
			ret = -ENOMEM;
			goto err_priv_id;
		}
	}

	for (i = 0; i < cnt; i+=2) {
		ret = of_property_read_u32_index(sysmmu->of_node,
			props_name, i, &prop);
		if (ret) {
			dev_err(sysmmu, "failed to get property again? "
				       "cnt = %d, ret = %d\n", i, ret);
			ret = -EINVAL;
			goto err_priv_addr;
		}

		switch (prop & WAY_TYPE_MASK) {
		case _PRIVATE_WAY_ID:
			BUG_ON(!priv_id_cfg || priv_id_idx >= priv_id_cnt);

			priv_id_cfg[priv_id_idx].cfg = prop & ~WAY_TYPE_MASK;
			ret = of_property_read_u32_index(sysmmu->of_node,
				props_name, i+1, &priv_id_cfg[priv_id_idx].id);
			if (ret) {
				dev_err(sysmmu, "failed to get id property"
						"cnt = %d, ret = %d\n", i, ret);
				goto err_priv_addr;
			}
			priv_id_idx++;
			break;
		case _PRIVATE_WAY_ADDR:
			BUG_ON(!priv_addr_cfg || priv_addr_idx >= priv_addr_cnt);

			priv_addr_cfg[priv_addr_idx].cfg = prop & ~WAY_TYPE_MASK;
			priv_addr_idx++;
			break;
		case _PUBLIC_WAY:
			break;
		}
	}

	tlb_props->way_props.priv_id_cfg = priv_id_cfg;
	tlb_props->way_props.priv_id_cnt = priv_id_cnt;

	tlb_props->way_props.priv_addr_cfg = priv_addr_cfg;
	tlb_props->way_props.priv_addr_cnt = priv_addr_cnt;

	return 0;

err_priv_addr:
	if (priv_addr_cfg)
		kfree(priv_addr_cfg);
err_priv_id:
	if (priv_id_cfg)
		kfree(priv_id_cfg);

	return ret;
}

static int __init sysmmu_parse_tlb_port_dt(struct device *sysmmu,
				struct sysmmu_drvdata *drvdata)
{
	const char *props_name = "sysmmu,tlb_property";
	const char *slot_props_name = "sysmmu,slot_property";
	struct tlb_props *tlb_props = &drvdata->tlb_props;
	struct tlb_port_cfg *port_cfg = NULL;
	unsigned int *slot_cfg = NULL;
	int i, cnt, ret;
	int port_id_cnt = 0;

	cnt = of_property_count_u32_elems(sysmmu->of_node, slot_props_name);
	if (cnt > 0) {
		slot_cfg = kzalloc(sizeof(*slot_cfg) * cnt, GFP_KERNEL);
		if (!slot_cfg)
			return -ENOMEM;

		for (i = 0; i < cnt; i++) {
			ret = of_property_read_u32_index(sysmmu->of_node,
					slot_props_name, i, &slot_cfg[i]);
			if (ret) {
				dev_err(sysmmu, "failed to get slot property."
						"cnt = %d, ret = %d\n", i, ret);
				ret = -EINVAL;
				goto err_slot_prop;
			}
		}

		tlb_props->port_props.slot_cnt = cnt;
		tlb_props->port_props.slot_cfg = slot_cfg;
	}

	cnt = of_property_count_u32_elems(sysmmu->of_node, props_name);
	if (!cnt || cnt < 0) {
		dev_info(sysmmu, "No TLB port propeties found.\n");
		return 0;
	}

	port_cfg = kzalloc(sizeof(*port_cfg) * (cnt/2), GFP_KERNEL);
	if (!port_cfg) {
		ret = -ENOMEM;
		goto err_slot_prop;
	}

	for (i = 0; i < cnt; i+=2) {
		ret = of_property_read_u32_index(sysmmu->of_node,
			props_name, i, &port_cfg[port_id_cnt].cfg);
		if (ret) {
			dev_err(sysmmu, "failed to get cfg property."
				       "cnt = %d, ret = %d\n", i, ret);
			ret = -EINVAL;
			goto err_port_prop;
		}

		ret = of_property_read_u32_index(sysmmu->of_node,
			props_name, i+1, &port_cfg[port_id_cnt].id);
		if (ret) {
			dev_err(sysmmu, "failed to get id property."
				       "cnt = %d, ret = %d\n", i, ret);
			ret = -EINVAL;
			goto err_port_prop;
		}
		port_id_cnt++;
	}

	tlb_props->port_props.port_id_cnt = port_id_cnt;
	tlb_props->port_props.port_cfg = port_cfg;

	return 0;

err_port_prop:
	kfree(port_cfg);

err_slot_prop:
	kfree(slot_cfg);

	return ret;
}

static int __init sysmmu_parse_dt(struct device *sysmmu,
				struct sysmmu_drvdata *drvdata)
{
	unsigned int qos = DEFAULT_QOS_VALUE;
	int ret;

	/* Parsing QoS */
	ret = of_property_read_u32_index(sysmmu->of_node, "qos", 0, &qos);
	if (!ret && (qos > 15)) {
		dev_err(sysmmu, "Invalid QoS value %d, use default.\n", qos);
		qos = DEFAULT_QOS_VALUE;
	}
	drvdata->qos = qos;

	/* Secure IRQ */
	if (of_find_property(sysmmu->of_node, "sysmmu,secure-irq", NULL)) {
		ret = __sysmmu_secure_irq_init(sysmmu, drvdata);
		if (ret) {
			dev_err(sysmmu, "Failed to init secure irq\n");
			return ret;
		}
	}

	if (of_property_read_bool(sysmmu->of_node, "sysmmu,no-suspend"))
		dev_pm_syscore_device(sysmmu, true);

	if (of_property_read_bool(sysmmu->of_node, "sysmmu,hold-rpm-on-boot"))
		drvdata->hold_rpm_on_boot = true;

	if (of_property_read_bool(sysmmu->of_node, "sysmmu,no-rpm-control"))
		drvdata->no_rpm_control = SYSMMU_STATE_DISABLED;

	if (IS_TLB_WAY_TYPE(drvdata)) {
		ret = sysmmu_parse_tlb_way_dt(sysmmu, drvdata);
		if (ret)
			dev_err(sysmmu, "Failed to parse TLB way property\n");
	} else if (IS_TLB_PORT_TYPE(drvdata)) {
		ret = sysmmu_parse_tlb_port_dt(sysmmu, drvdata);
		if (ret)
			dev_err(sysmmu, "Failed to parse TLB port property\n");
	};

	return ret;
}

static struct iommu_ops exynos_iommu_ops;
static int __init exynos_sysmmu_probe(struct platform_device *pdev)
{
	int irq, ret;
	struct device *dev = &pdev->dev;
	struct sysmmu_drvdata *data;
	struct resource *res;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Failed to get resource info\n");
		return -ENOENT;
	}

	data->sfrbase = devm_ioremap_resource(dev, res);
	if (IS_ERR(data->sfrbase))
		return PTR_ERR(data->sfrbase);

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(dev, "Unable to find IRQ resource\n");
		return irq;
	}

	ret = devm_request_irq(dev, irq, exynos_sysmmu_irq, 0,
				dev_name(dev), data);
	if (ret) {
		dev_err(dev, "Unabled to register handler of irq %d\n", irq);
		return ret;
	}

	data->clk = devm_clk_get(dev, "aclk");
	if (IS_ERR(data->clk)) {
		dev_err(dev, "Failed to get clock!\n");
		return PTR_ERR(data->clk);
	} else  {
		ret = clk_prepare(data->clk);
		if (ret) {
			dev_err(dev, "Failed to prepare clk\n");
			return ret;
		}
	}

	data->sysmmu = dev;
	spin_lock_init(&data->lock);
	ATOMIC_INIT_NOTIFIER_HEAD(&data->fault_notifiers);

	platform_set_drvdata(pdev, data);

	ret = exynos_iommu_init_event_log(SYSMMU_DRVDATA_TO_LOG(data),
				SYSMMU_LOG_LEN);
	if (!ret)
		sysmmu_add_log_to_debugfs(exynos_sysmmu_debugfs_root,
				SYSMMU_DRVDATA_TO_LOG(data), dev_name(dev));
	else
		return ret;

	ret = sysmmu_get_hw_info(data);
	if (ret) {
		dev_err(dev, "Failed to get h/w info\n");
		return ret;
	}

	ret = sysmmu_parse_dt(data->sysmmu, data);
	if (ret) {
		dev_err(dev, "Failed to parse DT\n");
		return ret;
	}

	if (!sysmmu_drvdata_list) {
		sysmmu_drvdata_list = data;
	} else {
		data->next = sysmmu_drvdata_list->next;
		sysmmu_drvdata_list->next = data;
	}

	iommu_device_set_ops(&data->iommu, &exynos_iommu_ops);
	iommu_device_set_fwnode(&data->iommu, &dev->of_node->fwnode);

	ret = iommu_device_register(&data->iommu);
	if (ret) {
		dev_err(dev, "Failed to register device\n");
		return ret;
	}

	pm_runtime_enable(dev);

	if (data->hold_rpm_on_boot)
		pm_runtime_get_sync(dev);

	dev_info(data->sysmmu, "is probed. Version %d.%d.%d\n",
			MMU_MAJ_VER(data->version),
			MMU_MIN_VER(data->version),
			MMU_REV_VER(data->version));

	return 0;
}

static bool __sysmmu_disable(struct sysmmu_drvdata *drvdata)
{
	bool disabled;
	unsigned long flags;

	spin_lock_irqsave(&drvdata->lock, flags);

	disabled = set_sysmmu_inactive(drvdata);

	if (disabled) {
		drvdata->pgtable = 0;

		if (is_sysmmu_runtime_active(drvdata))
			__sysmmu_disable_nocount(drvdata);

		dev_dbg(drvdata->sysmmu, "Disabled\n");
	} else  {
		dev_dbg(drvdata->sysmmu, "%d times left to disable\n",
					drvdata->activations);
	}

	spin_unlock_irqrestore(&drvdata->lock, flags);

	return disabled;
}

static void sysmmu_disable_from_master(struct device *master)
{
	unsigned long flags;
	struct exynos_iommu_owner *owner = master->archdata.iommu;
	struct sysmmu_list_data *list;
	struct sysmmu_drvdata *drvdata;

	BUG_ON(!has_sysmmu(master));

	spin_lock_irqsave(&owner->lock, flags);
	list_for_each_entry(list, &owner->sysmmu_list, node) {
		drvdata = dev_get_drvdata(list->sysmmu);
		__sysmmu_disable(drvdata);
	}
	spin_unlock_irqrestore(&owner->lock, flags);
}

static int __sysmmu_enable(struct sysmmu_drvdata *drvdata, phys_addr_t pgtable)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&drvdata->lock, flags);
	if (set_sysmmu_active(drvdata)) {
		drvdata->pgtable = pgtable;

		if (is_sysmmu_runtime_active(drvdata))
			__sysmmu_enable_nocount(drvdata);

		dev_dbg(drvdata->sysmmu, "Enabled\n");
	} else {
		ret = (pgtable == drvdata->pgtable) ? 1 : -EBUSY;

		dev_dbg(drvdata->sysmmu, "already enabled\n");
	}

	if (WARN_ON(ret < 0))
		set_sysmmu_inactive(drvdata); /* decrement count */

	spin_unlock_irqrestore(&drvdata->lock, flags);

	return ret;
}

static int sysmmu_enable_from_master(struct device *master,
				struct exynos_iommu_domain *domain)
{
	int ret = 0;
	unsigned long flags;
	struct exynos_iommu_owner *owner = master->archdata.iommu;
	struct sysmmu_list_data *list;
	struct sysmmu_drvdata *drvdata;
	phys_addr_t pgtable = virt_to_phys(domain->pgtable);

	spin_lock_irqsave(&owner->lock, flags);
	list_for_each_entry(list, &owner->sysmmu_list, node) {
		drvdata = dev_get_drvdata(list->sysmmu);
		ret = __sysmmu_enable(drvdata, pgtable);

		/* rollback if enable is failed */
		if (ret < 0) {
			list_for_each_entry_continue_reverse(list,
						&owner->sysmmu_list, node) {
				drvdata = dev_get_drvdata(list->sysmmu);
				__sysmmu_disable(drvdata);
			}
			break;
		}
		if (drvdata->hold_rpm_on_boot) {
			pm_runtime_put(drvdata->sysmmu);
			drvdata->hold_rpm_on_boot = false;
		}
	}
	spin_unlock_irqrestore(&owner->lock, flags);

	return ret;
}

void exynos_sysmmu_control(struct device *master, bool enable)
{
	unsigned long flags;
	struct exynos_iommu_owner *owner = master->archdata.iommu;
	struct sysmmu_list_data *list;
	struct sysmmu_drvdata *drvdata;

	BUG_ON(!has_sysmmu(master));

	spin_lock_irqsave(&owner->lock, flags);
	list_for_each_entry(list, &owner->sysmmu_list, node) {
		drvdata = dev_get_drvdata(list->sysmmu);
		spin_lock(&drvdata->lock);
		if (!drvdata->no_rpm_control) {
			spin_unlock(&drvdata->lock);
			continue;
		}
		if (enable) {
			__sysmmu_enable_nocount(drvdata);
			drvdata->no_rpm_control = SYSMMU_STATE_ENABLED;
		} else {
			__sysmmu_disable_nocount(drvdata);
			drvdata->no_rpm_control = SYSMMU_STATE_DISABLED;
		}
		spin_unlock(&drvdata->lock);
	}
	spin_unlock_irqrestore(&owner->lock, flags);
}

#ifdef CONFIG_PM_SLEEP
static int exynos_sysmmu_suspend(struct device *dev)
{
	unsigned long flags;
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);

	spin_lock_irqsave(&drvdata->lock, flags);
	if (is_sysmmu_active(drvdata) &&
			is_sysmmu_runtime_active(drvdata)) {
		__sysmmu_disable_nocount(drvdata);
		drvdata->is_suspended = true;
	}
	spin_unlock_irqrestore(&drvdata->lock, flags);

	return 0;
}

static int exynos_sysmmu_resume(struct device *dev)
{
	unsigned long flags;
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(dev);

	spin_lock_irqsave(&drvdata->lock, flags);
	if (drvdata->is_suspended && !drvdata->no_rpm_control) {
		__sysmmu_enable_nocount(drvdata);
		drvdata->is_suspended = false;
	}
	spin_unlock_irqrestore(&drvdata->lock, flags);

	return 0;
}
#endif

int exynos_iommu_runtime_suspend(struct device *sysmmu)
{
	unsigned long flags;
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(sysmmu);

	SYSMMU_EVENT_LOG_POWEROFF(SYSMMU_DRVDATA_TO_LOG(drvdata));
	spin_lock_irqsave(&drvdata->lock, flags);
	if (put_sysmmu_runtime_active(drvdata) && is_sysmmu_active(drvdata))
		__sysmmu_disable_nocount(drvdata);
	spin_unlock_irqrestore(&drvdata->lock, flags);

	return 0;
}

int exynos_iommu_runtime_resume(struct device *sysmmu)
{
	unsigned long flags;
	struct sysmmu_drvdata *drvdata = dev_get_drvdata(sysmmu);

	SYSMMU_EVENT_LOG_POWERON(SYSMMU_DRVDATA_TO_LOG(drvdata));
	spin_lock_irqsave(&drvdata->lock, flags);
	if (get_sysmmu_runtime_active(drvdata) && is_sysmmu_active(drvdata))
		__sysmmu_enable_nocount(drvdata);
	spin_unlock_irqrestore(&drvdata->lock, flags);

	return 0;
}

static const struct dev_pm_ops sysmmu_pm_ops = {
	SET_RUNTIME_PM_OPS(exynos_iommu_runtime_suspend,
				exynos_iommu_runtime_resume, NULL)
	SET_LATE_SYSTEM_SLEEP_PM_OPS(exynos_sysmmu_suspend, exynos_sysmmu_resume)
};

static const struct of_device_id sysmmu_of_match[] = {
	{ .compatible	= "samsung,exynos-sysmmu", },
	{ },
};

static struct platform_driver exynos_sysmmu_driver __refdata = {
	.probe	= exynos_sysmmu_probe,
	.driver	= {
		.name		= "exynos-sysmmu",
		.of_match_table	= sysmmu_of_match,
		.pm		= &sysmmu_pm_ops,
	}
};

static struct iommu_domain *exynos_iommu_domain_alloc(unsigned type)
{
	struct exynos_iommu_domain *domain;

	if (type != IOMMU_DOMAIN_UNMANAGED)
		return NULL;

	domain = kzalloc(sizeof(*domain), GFP_KERNEL);
	if (!domain)
		return NULL;

	domain->pgtable = (sysmmu_pte_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 2);
	if (!domain->pgtable)
		goto err_pgtable;

	domain->lv2entcnt = (atomic_t *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, 2);
	if (!domain->lv2entcnt)
		goto err_counter;

	if (exynos_iommu_init_event_log(IOMMU_PRIV_TO_LOG(domain), IOMMU_LOG_LEN))
		goto err_init_event_log;

	pgtable_flush(domain->pgtable, domain->pgtable + NUM_LV1ENTRIES);

	spin_lock_init(&domain->lock);
	spin_lock_init(&domain->pgtablelock);
	INIT_LIST_HEAD(&domain->clients_list);

	/* TODO: get geometry from device tree */
	domain->domain.geometry.aperture_start = 0;
	domain->domain.geometry.aperture_end   = ~0UL;
	domain->domain.geometry.force_aperture = true;

	return &domain->domain;

err_init_event_log:
	free_pages((unsigned long)domain->lv2entcnt, 2);
err_counter:
	free_pages((unsigned long)domain->pgtable, 2);
err_pgtable:
	kfree(domain);
	return NULL;
}

static void exynos_iommu_domain_free(struct iommu_domain *iommu_domain)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	struct exynos_iommu_owner *owner;
	unsigned long flags;
	int i;

	WARN_ON(!list_empty(&domain->clients_list));

	spin_lock_irqsave(&domain->lock, flags);

	list_for_each_entry(owner, &domain->clients_list, client)
		sysmmu_disable_from_master(owner->master);

	while (!list_empty(&domain->clients_list))
		list_del_init(domain->clients_list.next);

	spin_unlock_irqrestore(&domain->lock, flags);

	for (i = 0; i < NUM_LV1ENTRIES; i++)
		if (lv1ent_page(domain->pgtable + i))
			kmem_cache_free(lv2table_kmem_cache,
				phys_to_virt(lv2table_base(domain->pgtable + i)));

	free_pages((unsigned long)domain->pgtable, 2);
	free_pages((unsigned long)domain->lv2entcnt, 2);
	kfree(domain);
}

static int exynos_iommu_attach_device(struct iommu_domain *iommu_domain,
				   struct device *master)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	struct exynos_iommu_owner *owner;
	phys_addr_t pagetable = virt_to_phys(domain->pgtable);
	unsigned long flags;
	int ret = -ENODEV;

	if (!has_sysmmu(master)) {
		dev_err(master, "has no sysmmu device.\n");
		return -ENODEV;
	}

	spin_lock_irqsave(&domain->lock, flags);
	list_for_each_entry(owner, &domain->clients_list, client) {
		if (owner->master == master) {
			dev_err(master, "is already attached!\n");
			spin_unlock_irqrestore(&domain->lock, flags);
			return -EEXIST;
		}
	}
	/* owner is under domain. */
	owner = master->archdata.iommu;
	list_add_tail(&owner->client, &domain->clients_list);
	spin_unlock_irqrestore(&domain->lock, flags);

	ret = sysmmu_enable_from_master(master, domain);
	if (ret < 0) {
		dev_err(master, "%s: Failed to attach IOMMU with pgtable %pa\n",
					__func__, &pagetable);
		return ret;
	}

	dev_dbg(master, "%s: Attached IOMMU with pgtable %pa %s\n",
		__func__, &pagetable, (ret == 0) ? "" : ", again");
	SYSMMU_EVENT_LOG_IOMMU_ATTACH(IOMMU_PRIV_TO_LOG(domain), master);

	return 0;
}

static void exynos_iommu_detach_device(struct iommu_domain *iommu_domain,
				    struct device *master)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	phys_addr_t pagetable = virt_to_phys(domain->pgtable);
	struct exynos_iommu_owner *owner, *tmp_owner;
	unsigned long flags;
	bool found = false;

	if (!has_sysmmu(master))
		return;

	spin_lock_irqsave(&domain->lock, flags);
	list_for_each_entry_safe(owner, tmp_owner,
				&domain->clients_list, client) {
		if (owner->master == master) {
			sysmmu_disable_from_master(master);
			list_del_init(&owner->client);
			found = true;
		}
	}
	spin_unlock_irqrestore(&domain->lock, flags);

	if (found) {
		dev_dbg(master, "%s: Detached IOMMU with pgtable %pa\n",
					__func__, &pagetable);
		SYSMMU_EVENT_LOG_IOMMU_DETACH(IOMMU_PRIV_TO_LOG(domain), master);
	} else {
		dev_err(master, "%s: No IOMMU is attached\n", __func__);
	}
}

static sysmmu_pte_t *alloc_lv2entry(struct exynos_iommu_domain *domain,
				sysmmu_pte_t *sent, sysmmu_iova_t iova,
				atomic_t *pgcounter, gfp_t gfpmask)
{
	if (lv1ent_section(sent)) {
		WARN(1, "Trying mapping on %#08x mapped with 1MiB page", iova);
		return ERR_PTR(-EADDRINUSE);
	}

	if (lv1ent_fault(sent)) {
		unsigned long flags;
		sysmmu_pte_t *pent = NULL;

		if (!(gfpmask & __GFP_ATOMIC)) {
			pent = kmem_cache_zalloc(lv2table_kmem_cache, gfpmask);
			if (!pent)
				return ERR_PTR(-ENOMEM);
		}

		spin_lock_irqsave(&domain->pgtablelock, flags);
		if (lv1ent_fault(sent)) {
			if (!pent) {
				pent = kmem_cache_zalloc(lv2table_kmem_cache, gfpmask);
				if (!pent) {
					spin_unlock_irqrestore(&domain->pgtablelock, flags);
					return ERR_PTR(-ENOMEM);
				}
			}

			*sent = mk_lv1ent_page(virt_to_phys(pent));
			kmemleak_ignore(pent);
			atomic_set(pgcounter, NUM_LV2ENTRIES);
			pgtable_flush(pent, pent + NUM_LV2ENTRIES);
			pgtable_flush(sent, sent + 1);
			SYSMMU_EVENT_LOG_IOMMU_ALLOCSLPD(IOMMU_PRIV_TO_LOG(domain),
					iova & SECT_MASK, *sent);
		} else {
			/* Pre-allocated entry is not used, so free it. */
			kmem_cache_free(lv2table_kmem_cache, pent);
		}
		spin_unlock_irqrestore(&domain->pgtablelock, flags);
	}

	return page_entry(sent, iova);
}
static void clear_lv2_page_table(sysmmu_pte_t *ent, int n)
{
	if (n > 0)
		memset(ent, 0, sizeof(*ent) * n);
}

static int lv1set_section(struct exynos_iommu_domain *domain,
			  sysmmu_pte_t *sent, sysmmu_iova_t iova,
			  phys_addr_t paddr, int prot, atomic_t *pgcnt)
{
	bool shareable = !!(prot & IOMMU_CACHE);

	if (lv1ent_section(sent)) {
		WARN(1, "Trying mapping on 1MiB@%#08x that is mapped",
			iova);
		return -EADDRINUSE;
	}

	if (lv1ent_page(sent)) {
		if (WARN_ON(atomic_read(pgcnt) != NUM_LV2ENTRIES)) {
			WARN(1, "Trying mapping on 1MiB@%#08x that is mapped",
				iova);
			return -EADDRINUSE;
		}
		/* TODO: for v7, free lv2 page table */
	}

	*sent = mk_lv1ent_sect(paddr);
	if (shareable)
		set_lv1ent_shareable(sent);
	pgtable_flush(sent, sent + 1);

	return 0;
}

static int lv2set_page(sysmmu_pte_t *pent, phys_addr_t paddr, size_t size,
						int prot, atomic_t *pgcnt)
{
	bool shareable = !!(prot & IOMMU_CACHE);

	if (size == SPAGE_SIZE) {
		if (WARN_ON(!lv2ent_fault(pent)))
			return -EADDRINUSE;

		*pent = mk_lv2ent_spage(paddr);
		if (shareable)
			set_lv2ent_shareable(pent);
		pgtable_flush(pent, pent + 1);
		atomic_dec(pgcnt);
	} else { /* size == LPAGE_SIZE */
		int i;

		for (i = 0; i < SPAGES_PER_LPAGE; i++, pent++) {
			if (WARN_ON(!lv2ent_fault(pent))) {
				clear_lv2_page_table(pent - i, i);
				return -EADDRINUSE;
			}

			*pent = mk_lv2ent_lpage(paddr);
			if (shareable)
				set_lv2ent_shareable(pent);
		}
		pgtable_flush(pent - SPAGES_PER_LPAGE, pent);
		atomic_sub(SPAGES_PER_LPAGE, pgcnt);
	}

	return 0;
}
static int exynos_iommu_map(struct iommu_domain *iommu_domain,
			    unsigned long l_iova, phys_addr_t paddr, size_t size,
			    int prot)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	sysmmu_pte_t *entry;
	sysmmu_iova_t iova = (sysmmu_iova_t)l_iova;
	int ret = -ENOMEM;

	BUG_ON(domain->pgtable == NULL);

	entry = section_entry(domain->pgtable, iova);

	if (size == SECT_SIZE) {
		ret = lv1set_section(domain, entry, iova, paddr, prot,
				     &domain->lv2entcnt[lv1ent_offset(iova)]);
	} else {
		sysmmu_pte_t *pent;

		pent = alloc_lv2entry(domain, entry, iova,
			      &domain->lv2entcnt[lv1ent_offset(iova)], GFP_KERNEL);

		if (IS_ERR(pent))
			ret = PTR_ERR(pent);
		else
			ret = lv2set_page(pent, paddr, size, prot,
				       &domain->lv2entcnt[lv1ent_offset(iova)]);
	}

	if (ret)
		pr_err("%s: Failed(%d) to map %#zx bytes @ %#x\n",
			__func__, ret, size, iova);

	return ret;
}

static size_t exynos_iommu_unmap(struct iommu_domain *iommu_domain,
				 unsigned long l_iova, size_t size)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	sysmmu_iova_t iova = (sysmmu_iova_t)l_iova;
	sysmmu_pte_t *sent, *pent;
	size_t err_pgsize;
	atomic_t *lv2entcnt = &domain->lv2entcnt[lv1ent_offset(iova)];

	BUG_ON(domain->pgtable == NULL);

	sent = section_entry(domain->pgtable, iova);

	if (lv1ent_section(sent)) {
		if (WARN_ON(size < SECT_SIZE)) {
			err_pgsize = SECT_SIZE;
			goto err;
		}

		*sent = 0;
		pgtable_flush(sent, sent + 1);
		size = SECT_SIZE;
		goto done;
	}

	if (unlikely(lv1ent_fault(sent))) {
		if (size > SECT_SIZE)
			size = SECT_SIZE;
		goto done;
	}

	/* lv1ent_page(sent) == true here */

	pent = page_entry(sent, iova);

	if (unlikely(lv2ent_fault(pent))) {
		size = SPAGE_SIZE;
		goto done;
	}

	if (lv2ent_small(pent)) {
		*pent = 0;
		size = SPAGE_SIZE;
		pgtable_flush(pent, pent + 1);
		atomic_inc(lv2entcnt);
		goto unmap_flpd;
	}

	/* lv1ent_large(pent) == true here */
	if (WARN_ON(size < LPAGE_SIZE)) {
		err_pgsize = LPAGE_SIZE;
		goto err;
	}

	clear_lv2_page_table(pent, SPAGES_PER_LPAGE);
	pgtable_flush(pent, pent + SPAGES_PER_LPAGE);
	size = LPAGE_SIZE;
	atomic_add(SPAGES_PER_LPAGE, lv2entcnt);

unmap_flpd:
	/* TODO: for v7, remove all */
	if (atomic_read(lv2entcnt) == NUM_LV2ENTRIES) {
		unsigned long flags;
		spin_lock_irqsave(&domain->pgtablelock, flags);
		if (atomic_read(lv2entcnt) == NUM_LV2ENTRIES) {
			kmem_cache_free(lv2table_kmem_cache,
					page_entry(sent, 0));
			atomic_set(lv2entcnt, 0);

			SYSMMU_EVENT_LOG_IOMMU_FREESLPD(
				IOMMU_PRIV_TO_LOG(domain),
				iova_from_sent(domain->pgtable, sent), *sent);

			*sent = 0;
		}
		spin_unlock_irqrestore(&domain->pgtablelock, flags);
	}

done:
	/* TLB invalidation is performed by IOVMM */

	return size;
err:
	pr_err("%s: Failed: size(%#zx) @ %#x is smaller than page size %#zx\n",
		__func__, size, iova, err_pgsize);

	return 0;
}

static phys_addr_t exynos_iommu_iova_to_phys(struct iommu_domain *iommu_domain,
					  dma_addr_t d_iova)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(iommu_domain);
	sysmmu_iova_t iova = (sysmmu_iova_t)d_iova;
	sysmmu_pte_t *entry;
	phys_addr_t phys = 0;

	entry = section_entry(domain->pgtable, iova);

	if (lv1ent_section(entry)) {
		phys = section_phys(entry) + section_offs(iova);
	} else if (lv1ent_page(entry)) {
		entry = page_entry(entry, iova);

		if (lv2ent_large(entry))
			phys = lpage_phys(entry) + lpage_offs(iova);
		else if (lv2ent_small(entry))
			phys = spage_phys(entry) + spage_offs(iova);
	}

	return phys;
}

static int exynos_iommu_of_xlate(struct device *master,
				 struct of_phandle_args *spec)
{
	struct exynos_iommu_owner *owner = master->archdata.iommu;
	struct platform_device *sysmmu_pdev = of_find_device_by_node(spec->np);
	struct sysmmu_drvdata *data;
	struct device *sysmmu;
	struct exynos_client *client, *buf_client;
	struct sysmmu_list_data *list_data;

	if (!sysmmu_pdev)
		return -ENODEV;

	data = platform_get_drvdata(sysmmu_pdev);
	if (!data)
		return -ENODEV;

	sysmmu = data->sysmmu;
	if (!owner) {
		owner = kzalloc(sizeof(*owner), GFP_KERNEL);
		if (!owner)
			return -ENOMEM;

		INIT_LIST_HEAD(&owner->sysmmu_list);
		INIT_LIST_HEAD(&owner->client);
		master->archdata.iommu = owner;
		owner->master = master;
		spin_lock_init(&owner->lock);

		list_for_each_entry_safe(client, buf_client,
					&exynos_client_list, list) {
			if (client->master_np == master->of_node) {
				owner->domain = client->vmm_data->domain;
				owner->vmm_data = client->vmm_data;
				list_del(&client->list);
				kfree(client);
			}
		}

		/* HACK: Make relationship between group and master */
		master->iommu_group = owner->vmm_data->group;

		if (!sysmmu_owner_list) {
			sysmmu_owner_list = owner;
		} else {
			owner->next = sysmmu_owner_list->next;
			sysmmu_owner_list->next = owner;
		}
	}

	list_for_each_entry(list_data, &owner->sysmmu_list, node)
		if (list_data->sysmmu == sysmmu)
			return 0;

	list_data = devm_kzalloc(sysmmu, sizeof(*list_data), GFP_KERNEL);
	if (!list_data)
		return -ENOMEM;

	INIT_LIST_HEAD(&list_data->node);
	list_data->sysmmu = sysmmu;

	/*
	 * Use device link to make relationship between SysMMU and master.
	 * SysMMU device is supplier, and master device is consumer.
	 * This relationship guarantees that supplier is enabled before
	 * consumer, and it is disabled after consumer.
	 */
	device_link_add(master, sysmmu, DL_FLAG_PM_RUNTIME);

	/*
	 * System MMUs are attached in the order of the presence
	 * in device tree
	 */
	list_add_tail(&list_data->node, &owner->sysmmu_list);
	dev_info(master, "is owner of %s\n", dev_name(sysmmu));

	return 0;
}

static struct iommu_ops exynos_iommu_ops = {
	.domain_alloc = exynos_iommu_domain_alloc,
	.domain_free = exynos_iommu_domain_free,
	.attach_dev = exynos_iommu_attach_device,
	.detach_dev = exynos_iommu_detach_device,
	.map = exynos_iommu_map,
	.unmap = exynos_iommu_unmap,
	.map_sg = default_iommu_map_sg,
	.iova_to_phys = exynos_iommu_iova_to_phys,
	.pgsize_bitmap = SECT_SIZE | LPAGE_SIZE | SPAGE_SIZE,
	.of_xlate = exynos_iommu_of_xlate,
};

void exynos_sysmmu_show_status(struct device *master)
{
	struct exynos_iommu_owner *owner;
	struct sysmmu_list_data *list;

	if (!has_sysmmu(master)) {
		dev_err(master, "has no sysmmu!\n");
		return;
	}

	owner = master->archdata.iommu;
	list_for_each_entry(list, &owner->sysmmu_list, node) {
		unsigned long flags;
		struct sysmmu_drvdata *drvdata;

		drvdata = dev_get_drvdata(list->sysmmu);

		spin_lock_irqsave(&drvdata->lock, flags);
		if (!is_sysmmu_active(drvdata) ||
				!is_runtime_active_or_enabled(drvdata)) {
			dev_info(drvdata->sysmmu,
				"%s: SysMMU is not active\n", __func__);
			spin_unlock_irqrestore(&drvdata->lock, flags);
			continue;
		}

		dev_info(drvdata->sysmmu, "Dumping status.\n");
		dump_sysmmu_status(drvdata, 0);

		spin_unlock_irqrestore(&drvdata->lock, flags);
	}
}

static int sysmmu_fault_notifier(struct notifier_block *nb,
				     unsigned long fault_addr, void *data)
{
	struct owner_fault_info *info;
	struct exynos_iommu_owner *owner = NULL;

	info = container_of(nb, struct owner_fault_info, nb);
	owner = info->master->archdata.iommu;

	if (owner && owner->fault_handler)
		owner->fault_handler(owner->domain, owner->master,
			fault_addr, *(int *)data, owner->token);

	return 0;
}

int exynos_iommu_add_fault_handler(struct device *master,
			     iommu_fault_handler_t handler, void *token)
{
	struct exynos_iommu_owner *owner = master->archdata.iommu;
	struct sysmmu_list_data *list;
	struct sysmmu_drvdata *drvdata;
	struct owner_fault_info *info;
	unsigned long flags;

	if (!has_sysmmu(master)) {
		dev_info(master, "%s doesn't have sysmmu\n", dev_name(master));
		return -ENODEV;
	}

	spin_lock_irqsave(&owner->lock, flags);

	owner->fault_handler = handler;
	owner->token = token;

	list_for_each_entry(list, &owner->sysmmu_list, node) {
		info = kzalloc(sizeof(*info), GFP_ATOMIC);
		if (!info) {
			spin_unlock_irqrestore(&owner->lock, flags);
			return -ENOMEM;
		}
		info->master = master;
		info->nb.notifier_call = sysmmu_fault_notifier;

		drvdata = dev_get_drvdata(list->sysmmu);

		atomic_notifier_chain_register(
				&drvdata->fault_notifiers, &info->nb);
		dev_info(master, "Fault handler is registered for %s\n",
						dev_name(list->sysmmu));
	}

	spin_unlock_irqrestore(&owner->lock, flags);

	return 0;
}

int sysmmu_set_prefetch_buffer_property(struct device *dev,
			unsigned int inplanes, unsigned int onplanes,
			unsigned int ipoption[], unsigned int opoption[])
{
	/* DUMMY */
	dev_info(dev, "Called prefetch buffer property\n");

	return 0;
}

static int __init exynos_iommu_create_domain(void)
{
	struct device_node *domain_np;
	int ret;

	for_each_compatible_node(domain_np, NULL, "samsung,exynos-iommu-bus") {
		struct device_node *np;
		struct exynos_iovmm *vmm = NULL;
		struct exynos_iommu_domain *domain;
		unsigned int start = IOVA_START, end = IOVA_END;
		dma_addr_t d_addr;
		size_t d_size;
		int i = 0;

		ret = of_get_dma_window(domain_np, NULL, 0, NULL, &d_addr, &d_size);
		if (!ret) {
			if (d_addr == 0 || IOVA_OVFL(d_addr, d_size)) {
				pr_err("Failed to get valid dma ranges,\n");
				pr_err("Domain %s, range %pad++%#zx]\n",
					domain_np->name, &d_addr, d_size);
				of_node_put(domain_np);
				return -EINVAL;
			}
			start = d_addr;
			end = d_addr + d_size;
		}
		pr_info("DMA ranges for domain %s. [%#x..%#x]\n",
					domain_np->name, start, end);

		while ((np = of_parse_phandle(domain_np, "domain-clients", i++))) {
			if (!vmm) {
				vmm = exynos_create_single_iovmm(np->name,
								start, end);
				if (IS_ERR(vmm)) {
					pr_err("%s: Failed to create IOVM space\
							of %s\n",
							__func__, np->name);
					of_node_put(np);
					of_node_put(domain_np);
					return -ENOMEM;
				}

				/* HACK: Make one group for one domain */
				domain = to_exynos_domain(vmm->domain);
				vmm->group = iommu_group_alloc();
				iommu_attach_group(vmm->domain, vmm->group);
			}
			/* Relationship between domain and client is added. */
			ret = exynos_client_add(np, vmm);
			if (ret) {
				pr_err("Failed to adding client[%s] to domain %s\n",
						np->name, domain_np->name);
				of_node_put(np);
				of_node_put(domain_np);
				return -ENOMEM;
			} else {
				pr_info("Added client.%d[%s] into domain %s\n",
						i, np->name, domain_np->name);
			}
			of_node_put(np);
		}
		of_node_put(domain_np);
	}

	return 0;
}

static int __init exynos_iommu_init(void)
{
	struct device_node *np;
	int ret;

	np = of_find_matching_node(NULL, sysmmu_of_match);
	if (!np)
		return 0;

	of_node_put(np);

	lv2table_kmem_cache = kmem_cache_create("exynos-iommu-lv2table",
				LV2TABLE_SIZE, LV2TABLE_SIZE, 0, NULL);
	if (!lv2table_kmem_cache) {
		pr_err("%s: Failed to create kmem cache\n", __func__);
		return -ENOMEM;
	}

	exynos_sysmmu_debugfs_root = debugfs_create_dir("sysmmu", NULL);
	if (!exynos_sysmmu_debugfs_root)
		pr_err("%s: Failed to create debugfs entry\n", __func__);

	ret = platform_driver_register(&exynos_sysmmu_driver);
	if (ret) {
		pr_err("%s: Failed to register driver\n", __func__);
		goto err_reg_driver;
	}

	ret = bus_set_iommu(&platform_bus_type, &exynos_iommu_ops);
	if (ret) {
		pr_err("%s: Failed to register exynos-iommu driver.\n",
								__func__);
		goto err_set_iommu;
	}
	ret = exynos_iommu_create_domain();
	if (ret) {
		pr_err("%s: Failed to create domain\n", __func__);
		goto err_create_domain;
	}

	return 0;

err_create_domain:
	bus_set_iommu(&platform_bus_type, NULL);
err_set_iommu:
	platform_driver_unregister(&exynos_sysmmu_driver);
err_reg_driver:
	kmem_cache_destroy(lv2table_kmem_cache);
	return ret;
}
subsys_initcall_sync(exynos_iommu_init);

IOMMU_OF_DECLARE(exynos_iommu_of, "samsung,exynos-sysmmu", NULL);

static int mm_fault_translate(int fault)
{
	if (fault & VM_FAULT_OOM)
		return -ENOMEM;
	else if (fault & (VM_FAULT_SIGBUS | VM_FAULT_SIGSEGV))
		return -EBUSY;
	else if (fault & (VM_FAULT_HWPOISON | VM_FAULT_HWPOISON_LARGE))
		return -EFAULT;
	else if (fault & VM_FAULT_FALLBACK)
		return -EAGAIN;

	return -EFAULT;
}

static sysmmu_pte_t *alloc_lv2entry_userptr(struct exynos_iommu_domain *domain,
						sysmmu_iova_t iova)
{
	return alloc_lv2entry(domain, section_entry(domain->pgtable, iova),
			iova, &domain->lv2entcnt[lv1ent_offset(iova)], GFP_ATOMIC);
}

static int sysmmu_map_pte(struct mm_struct *mm,
		pmd_t *pmd, unsigned long addr, unsigned long end,
		struct exynos_iommu_domain *domain, sysmmu_iova_t iova, int prot)
{
	pte_t *pte;
	int ret = 0;
	spinlock_t *ptl;
	bool write = !!(prot & IOMMU_WRITE);
	bool pfnmap = !!(prot & IOMMU_PFNMAP);
	bool shareable = !!(prot & IOMMU_CACHE);
	unsigned int fault_flag = write ? FAULT_FLAG_WRITE : 0;
	sysmmu_pte_t *ent, *ent_beg;

	pte = pte_alloc_map_lock(mm, pmd, addr, &ptl);
	if (!pte)
		return -ENOMEM;

	ent = alloc_lv2entry_userptr(domain, iova);
	if (IS_ERR(ent)) {
		ret = PTR_ERR(ent);
		goto err;
	}

	ent_beg = ent;

	do {
		if (pte_none(*pte) || !pte_present(*pte) ||
					(write && !pte_write(*pte))) {
			int cnt = 0;
			int maxcnt = 1;

			if (pfnmap) {
				ret = -EFAULT;
				goto err;
			}

			while (cnt++ < maxcnt) {
				spin_unlock(ptl);
				/* find_vma() always successes */
				ret = handle_mm_fault(find_vma(mm, addr),
						addr, fault_flag);
				spin_lock(ptl);
				if (ret & VM_FAULT_ERROR) {
					ret = mm_fault_translate(ret);
					goto err;
				} else {
					ret = 0;
				}
				/*
				 * the racing between handle_mm_fault() and the
				 * page reclamation may cause handle_mm_fault()
				 * to return 0 even though it failed to page in.
				 * This behavior expect the process to access
				 * the paged out entry again then give
				 * handle_mm_fault() a chance again to page in
				 * the entry.
				 */
				if (is_swap_pte(*pte)) {
					BUG_ON(maxcnt > 8);
					maxcnt++;
				}
			}
		}

		BUG_ON(!lv2ent_fault(ent));

		*ent = mk_lv2ent_spage(pte_pfn(*pte) << PAGE_SHIFT);

		if (!pfnmap)
			get_page(pte_page(*pte));
		else
			mk_lv2ent_pfnmap(ent);

		if (shareable)
			set_lv2ent_shareable(ent);

		ent++;
		iova += (sysmmu_iova_t)PAGE_SIZE;

		if ((iova & SECT_MASK) != ((iova - 1) & SECT_MASK)) {
			pgtable_flush(ent_beg, ent);

			ent = alloc_lv2entry_userptr(domain, iova);
			if (IS_ERR(ent)) {
				ret = PTR_ERR(ent);
				goto err;
			}
			ent_beg = ent;
		}
	} while (pte++, addr += PAGE_SIZE, addr != end);

	pgtable_flush(ent_beg, ent);
err:
	pte_unmap_unlock(pte - 1, ptl);
	return ret;
}

static inline int sysmmu_map_pmd(struct mm_struct *mm,
		pud_t *pud, unsigned long addr, unsigned long end,
		struct exynos_iommu_domain *domain, sysmmu_iova_t iova, int prot)
{
	pmd_t *pmd;
	unsigned long next;

	pmd = pmd_alloc(mm, pud, addr);
	if (!pmd)
		return -ENOMEM;

	do {
		next = pmd_addr_end(addr, end);
		if (sysmmu_map_pte(mm, pmd, addr, next, domain, iova, prot))
			return -ENOMEM;
		iova += (sysmmu_iova_t)(next - addr);
	} while (pmd++, addr = next, addr != end);
	return 0;
}
static inline int sysmmu_map_pud(struct mm_struct *mm,
		pgd_t *pgd, unsigned long addr, unsigned long end,
		struct exynos_iommu_domain *domain, sysmmu_iova_t iova, int prot)
{
	pud_t *pud;
	unsigned long next;

	pud = pud_alloc(mm, pgd, addr);
	if (!pud)
		return -ENOMEM;
	do {
		next = pud_addr_end(addr, end);
		if (sysmmu_map_pmd(mm, pud, addr, next, domain, iova, prot))
			return -ENOMEM;
		iova += (sysmmu_iova_t)(next - addr);
	} while (pud++, addr = next, addr != end);
	return 0;
}
int exynos_iommu_map_userptr(struct iommu_domain *dom, unsigned long addr,
			      dma_addr_t d_iova, size_t size, int prot)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(dom);
	struct mm_struct *mm = current->mm;
	unsigned long end = addr + size;
	dma_addr_t start = d_iova;
	sysmmu_iova_t iova = (sysmmu_iova_t)d_iova;
	unsigned long next;
	pgd_t *pgd;
	int ret;

	BUG_ON(!!((iova | addr | size) & ~PAGE_MASK));

	pgd = pgd_offset(mm, addr);

	do {
		next = pgd_addr_end(addr, end);
		ret = sysmmu_map_pud(mm, pgd, addr, next, domain, iova, prot);
		if (ret)
			goto err;
		iova += (sysmmu_iova_t)(next - addr);
	} while (pgd++, addr = next, addr != end);

	return 0;
err:
	/* unroll */
	exynos_iommu_unmap_userptr(dom, start, size);
	return ret;
}

#define sect_offset(iova)	((iova) & ~SECT_MASK)
#define lv2ents_within(iova)	((SECT_SIZE - sect_offset(iova)) >> SPAGE_ORDER)

void exynos_iommu_unmap_userptr(struct iommu_domain *dom,
				dma_addr_t d_iova, size_t size)
{
	struct exynos_iommu_domain *domain = to_exynos_domain(dom);
	sysmmu_iova_t iova = (sysmmu_iova_t)d_iova;
	sysmmu_pte_t *sent = section_entry(domain->pgtable, iova);
	unsigned int entries = (unsigned int)(size >> SPAGE_ORDER);
	dma_addr_t start = d_iova;

	while (entries > 0) {
		unsigned int lv2ents, i;
		sysmmu_pte_t *pent;

		/* ignore fault entries */
		if (lv1ent_fault(sent)) {
			lv2ents = min_t(unsigned int, entries, NUM_LV1ENTRIES);
			entries -= lv2ents;
			iova += lv2ents << SPAGE_ORDER;
			sent++;
			continue;
		}

		BUG_ON(!lv1ent_page(sent));

		lv2ents = min_t(unsigned int, lv2ents_within(iova), entries);

		pent = page_entry(sent, iova);
		for (i = 0; i < lv2ents; i++, pent++) {
			/* ignore fault entries */
			if (lv2ent_fault(pent))
				continue;

			BUG_ON(!lv2ent_small(pent));

			if (!lv2ent_pfnmap(pent))
				put_page(phys_to_page(spage_phys(pent)));

			*pent = 0;
		}

		pgtable_flush(pent - lv2ents, pent);

		entries -= lv2ents;
		iova += lv2ents << SPAGE_ORDER;
		sent++;
	}

	exynos_sysmmu_tlb_invalidate(dom, start, size);
}

typedef void (*syncop)(const void *, size_t, int);

static size_t sysmmu_dma_sync_page(phys_addr_t phys, off_t off,
				  size_t pgsize, size_t size,
				  syncop op, enum dma_data_direction dir)
{
	size_t len;
	size_t skip_pages = off >> PAGE_SHIFT;
	struct page *page;

	off = off & ~PAGE_MASK;
	page = phys_to_page(phys) + skip_pages;
	len = min(pgsize - off, size);
	size = len;

	while (len > 0) {
		size_t sz;

		sz = min(PAGE_SIZE, len + off) - off;
		op(kmap(page) + off, sz, dir);
		kunmap(page++);
		len -= sz;
		off = 0;
	}

	return size;
}

static void exynos_iommu_sync(sysmmu_pte_t *pgtable, dma_addr_t iova,
			size_t len, syncop op, enum dma_data_direction dir)
{
	while (len > 0) {
		sysmmu_pte_t *entry;
		size_t done;

		entry = section_entry(pgtable, iova);
		switch (*entry & FLPD_FLAG_MASK) {
		case SECT_FLAG:
			done = sysmmu_dma_sync_page(section_phys(entry),
					section_offs(iova), SECT_SIZE,
					len, op, dir);
			break;
		case SLPD_FLAG:
			entry = page_entry(entry, iova);
			switch (*entry & SLPD_FLAG_MASK) {
			case LPAGE_FLAG:
				done = sysmmu_dma_sync_page(lpage_phys(entry),
						lpage_offs(iova), LPAGE_SIZE,
						len, op, dir);
				break;
			case SPAGE_FLAG:
				done = sysmmu_dma_sync_page(spage_phys(entry),
						spage_offs(iova), SPAGE_SIZE,
						len, op, dir);
				break;
			default: /* fault */
				return;
			}
			break;
		default: /* fault */
			return;
		}

		iova += done;
		len -= done;
	}
}

static sysmmu_pte_t *sysmmu_get_pgtable(struct device *dev)
{
	struct exynos_iommu_owner *owner = dev->archdata.iommu;
	struct exynos_iommu_domain *domain = to_exynos_domain(owner->domain);

	return domain->pgtable;
}

void exynos_iommu_sync_for_device(struct device *dev, dma_addr_t iova,
				  size_t len, enum dma_data_direction dir)
{
	exynos_iommu_sync(sysmmu_get_pgtable(dev),
			iova, len, __dma_map_area, dir);
}

void exynos_iommu_sync_for_cpu(struct device *dev, dma_addr_t iova, size_t len,
				enum dma_data_direction dir)
{
	if (dir == DMA_TO_DEVICE)
		return;

	exynos_iommu_sync(sysmmu_get_pgtable(dev),
			iova, len, __dma_unmap_area, dir);
}
