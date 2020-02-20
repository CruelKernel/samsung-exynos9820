/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *	      http://www.samsung.com/
 *
 * EXYNOS - Per-Page Memory Protection Unit(PPMPU) fail detector
 * Author: Junho Choi <junhosj.choi@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/smc.h>

#include <soc/samsung/exynos-ppmpu.h>


static irqreturn_t exynos_ppmpu_irq_handler(int irq, void *dev_id)
{
	struct ppmpu_info_data *data = dev_id;
	uint32_t dir = 0;
	uint32_t irq_idx;

	/*
	 *	IRQ_Index	PPMPU_INTR
	 *	    0		 PPMPU0_R
	 *	    1		 PPMPU0_W
	 *	    2		 PPMPU1_R
	 *	    3		 PPMPU1_W
	 *	    4		 PPMPU2_R
	 *	    5		 PPMPU2_W
	 *	    6		 PPMPU3_R
	 *	    7		 PPMPU3_W
	 */
	for (irq_idx = 0; irq_idx < data->irqcnt; irq_idx++) {
		if (irq == data->irq[irq_idx])
			break;
	}

#ifdef CONFIG_EXYNOS_PPMPU_SUPPORT_SMC
	if (irq_idx % 2)
		dir = PPMPU_ILLEGAL_ACCESS_WRITE;
	else
		dir = PPMPU_ILLEGAL_ACCESS_READ;
#endif

	/*
	 * Interrupt status register in PPMPU will clear
	 * in this SMC handler
	 */
	data->need_log = exynos_smc(SMC_CMD_GET_PPMPU_FAIL_INFO,
				    data->fail_info_pa,
				    (dir << PPMPU_DIRECTION_SHIFT) |
				    (irq_idx / 2),
				    data->info_flag);
	if ((data->need_log != PPMPU_NEED_FAIL_INFO_LOGGING) &&
		(data->need_log != PPMPU_SKIP_FAIL_INFO_LOGGING))
		pr_err("%s:ppmpu_fail_info buffer is invalid! ret(%#x)\n",
			__func__, data->need_log);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t exynos_ppmpu_irq_handler_thread(int irq, void *dev_id)
{
	struct ppmpu_info_data *data = dev_id;
	unsigned int intr_stat, addr_low;
	unsigned long addr_high;
	int i;

	if (data->need_log == PPMPU_SKIP_FAIL_INFO_LOGGING) {
		pr_debug("PPMPU_FAIL_DETECTOR: Ignore PPMPU illegal reads\n");
		return IRQ_HANDLED;
	}

	pr_info("===============[PPMPU FAIL DETECTION]===============\n");

	/* Parse fail register information */
	for (i = 0; i < data->ch_num; i++) {
		pr_info("[Channel %d]\n", i);

		intr_stat = data->fail_info[i].ppmpu_intr_stat;

		if (!(intr_stat &
			(PPMPU_WRITE_INTR_STATUS |
			 PPMPU_READ_INTR_STATUS))) {
			pr_info("NO access failure in this Channel\n\n");
			continue;
		}

		if (intr_stat & PPMPU_READ_INTR_STATUS) {
			addr_low = data->fail_info[i].ppmpu_illegal_read_addr_low;
			addr_high = data->fail_info[i].ppmpu_illegal_read_addr_high &
					PPMPU_ILLEGAL_ADDR_HIGH_MASK;

			pr_info("- [READ] Illegal Adddress : %#lx\n",
				addr_high ?
				(addr_high << 32) | addr_low :
				addr_low);

			pr_info("- [READ] Fail VC : %#x\n",
				data->fail_info[i].ppmpu_illigal_read_field &
				PPMPU_ILLEGAL_FIELD_FAIL_VC_MASK);

			pr_info("- [READ] Fail ID : %#x\n",
				data->fail_info[i].ppmpu_illigal_read_field &
				PPMPU_ILLEGAL_FIELD_FAIL_ID_MASK);

			pr_info("- [READ] Interrupt Status : %s\n",
				(data->fail_info[i].ppmpu_intr_stat &
				PPMPU_READ_INTR_STATUS) ?
				"Interrupt is asserted" :
				"Interrupt is not asserted");

			pr_info("- [READ] Interrupt Overrun : %s\n",
				(data->fail_info[i].ppmpu_intr_stat &
				PPMPU_READ_INTR_STATUS_OVERRUN) ?
				"Two or more failures occurred" :
				"Only one failure occurred");

			pr_info("\n");

		}

		if (intr_stat & PPMPU_WRITE_INTR_STATUS) {
			addr_low = data->fail_info[i].ppmpu_illegal_write_addr_low;
			addr_high = data->fail_info[i].ppmpu_illegal_write_addr_high &
					PPMPU_ILLEGAL_ADDR_HIGH_MASK;

			pr_info("- [WRITE] Illegal Adddress : %#lx\n",
				addr_high ?
				(addr_high << 32) | addr_low :
				addr_low);

			pr_info("- [WRITE] Fail VC : %#x\n",
				data->fail_info[i].ppmpu_illigal_write_field &
				PPMPU_ILLEGAL_FIELD_FAIL_VC_MASK);

			pr_info("- [WRITE] Fail ID : %#x\n",
				data->fail_info[i].ppmpu_illigal_write_field &
				PPMPU_ILLEGAL_FIELD_FAIL_ID_MASK);

			pr_info("- [WRITE] Interrupt Status : %s\n",
				(data->fail_info[i].ppmpu_intr_stat &
				PPMPU_WRITE_INTR_STATUS) ?
				"Interrupt is asserted" :
				"Interrupt is not asserted");

			pr_info("- [WRITE] Interrupt Overrun : %s\n",
				(data->fail_info[i].ppmpu_intr_stat &
				PPMPU_WRITE_INTR_STATUS_OVERRUN) ?
				"Two or more failures occurred" :
				"Only one failure occurred");

			pr_info("\n");

		}

		pr_info("- SFR values\n");
		pr_info("INT_STATUS : %#x\n",
			data->fail_info[i].ppmpu_intr_stat);
		pr_info("ILLEGAL_READ_ADDRESS_LOW : %#x\n",
			data->fail_info[i].ppmpu_illegal_read_addr_low);
		pr_info("ILLEGAL_READ_ADDRESS_HIGH : %#x\n",
			data->fail_info[i].ppmpu_illegal_read_addr_high);
		pr_info("ILLEGAL_READ_FIELD : %#x\n",
			data->fail_info[i].ppmpu_illigal_read_field);
		pr_info("ILLEGAL_WRITE_ADDRESS_LOW : %#x\n",
			data->fail_info[i].ppmpu_illegal_write_addr_low);
		pr_info("ILLEGAL_WRITE_ADDRESS_HIGH : %#x\n",
			data->fail_info[i].ppmpu_illegal_write_addr_high);
		pr_info("ILLEGAL_WRITE_FIELD : %#x\n",
			data->fail_info[i].ppmpu_illigal_write_field);

		pr_info("\n");
	}

	pr_info("====================================================\n");

#ifdef CONFIG_EXYNOS_PPMPU_ILLEGAL_ACCESS_PANIC
	BUG();
#endif
	return IRQ_HANDLED;
}

static int exynos_ppmpu_probe(struct platform_device *pdev)
{
	struct ppmpu_info_data *data;
	int ret, i;

	data = devm_kzalloc(&pdev->dev, sizeof(struct ppmpu_info_data), GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "Fail to allocate memory(ppmpu_info_data)\n");
		ret = -ENOMEM;
		goto out;
	}

	platform_set_drvdata(pdev, data);
	data->dev = &pdev->dev;

	ret = of_property_read_u32(data->dev->of_node, "channel", &data->ch_num);
	if (ret) {
		dev_err(data->dev,
			"Fail to get PPMPU channel number(%d) from dt\n",
			data->ch_num);
		goto out;
	}

	ret = exynos_smc(SMC_CMD_CHECK_PPMPU_CH_NUM,
				data->ch_num,
				sizeof(struct ppmpu_fail_info),
				0);
	if (ret) {
		switch (ret) {
		case PPMPU_ERROR_INVALID_CH_NUM:
			dev_err(data->dev,
				"The channel number(%d) defined in DT is invalid\n",
				data->ch_num);
			break;
		case PPMPU_ERROR_INVALID_FAIL_INFO_SIZE:
			dev_err(data->dev,
				"The size of struct ppmpu_fail_info(%#x) is invalid\n",
				sizeof(struct ppmpu_fail_info));
			break;
		case SMC_CMD_CHECK_PPMPU_CH_NUM:
			dev_err(data->dev,
				"DO NOT support this SMC(%#x)\n",
				SMC_CMD_CHECK_PPMPU_CH_NUM);
			break;
		default:
			dev_err(data->dev,
				"Unknown error from SMC. ret[%#x]\n",
				ret);
			break;
		}

		ret = -EINVAL;
		goto out;
	}

	dev_dbg(data->dev,
		"PPMPU channel number : %d\n",
		data->ch_num);
	dev_info(data->dev,
		"PPMPU channel number is valid\n");

	/*
	 * Allocate PPMPU fail information buffers as the channel number
	 *
	 * DRM LDFW has mapped Kernel region with non-cacheable,
	 * so Kernel allocates it by dma_alloc_coherent
	 */
	data->fail_info = dma_alloc_coherent(data->dev,
						sizeof(struct ppmpu_fail_info) *
						data->ch_num,
						&data->fail_info_pa,
						GFP_KERNEL);
	if (!data->fail_info) {
		dev_err(data->dev, "Fail to allocate memory(ppmpu_fail_info)\n");
		ret = -ENOMEM;
		goto out;
	}

	dev_dbg(data->dev,
		"VA of ppmpu_fail_info : %lx\n",
		(unsigned long)data->fail_info);
	dev_dbg(data->dev,
		"PA of ppmpu_fail_info : %lx\n",
		data->fail_info_pa);

	ret = of_property_read_u32(data->dev->of_node, "irqcnt", &data->irqcnt);
	if (ret) {
		dev_err(data->dev,
			"Fail to get irqcnt(%d) from dt\n",
			data->irqcnt);
		goto out_with_dma_free;
	}

	dev_dbg(data->dev,
		"The number of PPMPU interrupt : %d\n",
		data->irqcnt);

	for (i = 0; i < data->irqcnt; i++) {
		data->irq[i] = irq_of_parse_and_map(data->dev->of_node, i);
		if (!data->irq[i]) {
			dev_err(data->dev,
				"Fail to get irq(%d) from dt\n",
				data->irq[i]);
			ret = -EINVAL;
			goto out_with_dma_free;
		}

		ret = devm_request_threaded_irq(data->dev,
						data->irq[i],
						exynos_ppmpu_irq_handler,
						exynos_ppmpu_irq_handler_thread,
						IRQF_ONESHOT,
						pdev->name,
						data);
		if (ret) {
			dev_err(data->dev,
				"Fail to request IRQ handler. ret(%d) irq(%d)\n",
				ret, data->irq[i]);
			goto out_with_dma_free;
		}
	}

#ifdef CONFIG_EXYNOS_PPMPU_ILLEGAL_READ_LOGGING
	data->info_flag = STR_INFO_FLAG;
#endif

	dev_info(data->dev, "Exynos PPMPU driver probe done!\n");

	return 0;

out_with_dma_free:
	platform_set_drvdata(pdev, NULL);

	dma_free_coherent(data->dev,
			  sizeof(struct ppmpu_fail_info) *
			  data->ch_num,
			  data->fail_info,
			  data->fail_info_pa);
	data->fail_info = NULL;
	data->fail_info_pa = 0;

out:
	return ret;
}

static int exynos_ppmpu_remove(struct platform_device *pdev)
{
	struct ppmpu_info_data *data = platform_get_drvdata(pdev);
	int i;

	platform_set_drvdata(pdev, NULL);

	if (data->fail_info) {
		dma_free_coherent(data->dev,
				  sizeof(struct ppmpu_fail_info) *
				  data->ch_num,
				  data->fail_info,
				  data->fail_info_pa);

		data->fail_info = NULL;
		data->fail_info_pa = 0;
	}

	for (i = 0; i < data->ch_num; i++)
		data->irq[i] = 0;

	data->ch_num = 0;
	data->irqcnt = 0;
	data->info_flag = 0;

	return 0;
}


static const struct of_device_id exynos_ppmpu_of_match_table[] = {
	{ .compatible = "samsung,exynos-ppmpu", },
	{ },
};

static struct platform_driver exynos_ppmpu_driver = {
	.probe = exynos_ppmpu_probe,
	.remove = exynos_ppmpu_remove,
	.driver = {
		.name = "exynos-ppmpu",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(exynos_ppmpu_of_match_table),
	}
};

static int __init exynos_ppmpu_init(void)
{
	return platform_driver_register(&exynos_ppmpu_driver);
}

static void __exit exynos_ppmpu_exit(void)
{
	platform_driver_unregister(&exynos_ppmpu_driver);
}

module_init(exynos_ppmpu_init);
module_exit(exynos_ppmpu_exit);

MODULE_DESCRIPTION("Exynos Per-Page Memory Protection Unit(PPMPU) driver");
MODULE_AUTHOR("<junhosj.choi@samsung.com>");
MODULE_LICENSE("GPL");
