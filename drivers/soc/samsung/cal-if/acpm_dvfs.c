#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/sched/clock.h>

#include <soc/samsung/acpm_ipc_ctrl.h>
#include <soc/samsung/exynos-devfreq.h>
#include <soc/samsung/tmu.h>

#include "acpm_dvfs.h"
#include "cmucal.h"

#ifndef CONFIG_ARM_EXYNOS_DEVFREQ
#define PM_QOS_BUS_THROUGHPUT (11)
#endif

static struct acpm_dvfs acpm_dvfs;
static struct acpm_dvfs acpm_noti_mif;
static struct pm_qos_request mif_request_from_acpm;

int exynos_acpm_set_rate(unsigned int id, unsigned long rate)
{
	struct ipc_config config;
	unsigned int cmd[4];
	unsigned long long before, after, latency;
	int ret;

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[0] = id;
	config.cmd[1] = (unsigned int)rate;
	config.cmd[2] = FREQ_REQ;
	config.cmd[3] = 0;

	before = sched_clock();
	ret = acpm_ipc_send_data_lazy(acpm_dvfs.ch_num, &config);
	after = sched_clock();
	latency = after - before;
	if (ret)
		pr_err("%s:[%d] latency = %llu ret = %d",
			__func__, id, latency, ret);

	return ret;
}

int exynos_acpm_set_init_freq(unsigned int dfs_id, unsigned long freq)
{
	struct ipc_config config;
	unsigned int cmd[4];
	unsigned long long before, after, latency;
	int ret, id;

	id = GET_IDX(dfs_id);

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[0] = id;
	config.cmd[1] = (unsigned int)freq;
	config.cmd[2] = DATA_INIT;
	config.cmd[3] = SET_INIT_FREQ;

	before = sched_clock();
	ret = acpm_ipc_send_data_lazy(acpm_dvfs.ch_num, &config);
	after = sched_clock();
	latency = after - before;
	if (ret)
		pr_err("%s:[%d] latency = %llu ret = %d",
			__func__, id, latency, ret);

	return ret;
}

unsigned long exynos_acpm_get_rate(unsigned int id)
{
	struct ipc_config config;
	unsigned int cmd[4];
	unsigned long long before, after, latency;
	int ret;

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[0] = id;
	config.cmd[1] = 0;
	config.cmd[2] = FREQ_GET;
	config.cmd[3] = 0;

	before = sched_clock();
	ret = acpm_ipc_send_data_lazy(acpm_dvfs.ch_num, &config);
	after = sched_clock();
	latency = after - before;
	if (ret)
		pr_err("%s:[%d] latency = %llu ret = %d", __func__,
			id, latency, ret);

	return config.cmd[1];
}

char margin_list[MAX_MARGIN_ID][10] = {"MIF", "INT", "BIG", "MID", "LIT", "G3D",
		"INTCAM", "CAM", "DISP", "G3DM",
		"CP", "FSYS0", "AUD", "IVA", "SCORE", "NPU", "MFC"};

int exynos_acpm_set_volt_margin(unsigned int id, int volt)
{
	struct ipc_config config;
	unsigned int cmd[4];
	unsigned long long before, after, latency;
	int ret;
	struct vclk *vclk;

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[0] = id;
	config.cmd[1] = volt;
	config.cmd[2] = MARGIN_REQ;
	config.cmd[3] = 0;

	before = sched_clock();
	ret = acpm_ipc_send_data(acpm_dvfs.ch_num, &config);
	after = sched_clock();
	latency = after - before;
	if (ret)
		pr_err("%s:[%d] latency = %llu ret = %d",
			__func__, id, latency, ret);

	vclk = cmucal_get_node(id);
	if (!vclk)
		pr_err("%s:[%d] can't find cmucal node ",
			__func__, id);
	else
		pr_auto(ASL5, "%s: [%s] +margin %d uV\n", __func__, margin_list[vclk->margin_id], volt);

	return ret;
}

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
int exynos_acpm_set_cold_temp(unsigned int id, bool is_cold_temp)
{
	struct ipc_config config;
	unsigned int cmd[4];
	unsigned long long before, after, latency;
	int ret;

	config.cmd = cmd;
	config.response = true;
	config.indirection = false;
	config.cmd[0] = id;
	config.cmd[1] = is_cold_temp;
	config.cmd[2] = COLDTEMP_REQ;
	config.cmd[3] = 0;

	before = sched_clock();
	ret = acpm_ipc_send_data(acpm_dvfs.ch_num, &config);
	after = sched_clock();
	latency = after - before;
	if (ret)
		pr_err("%s:[%d] latency = %llu ret = %d",
			__func__, id, latency, ret);

	return ret;
}
#endif

static void acpm_noti_mif_callback(unsigned int *cmd, unsigned int size)
{
	pr_info("%s : req %d KHz\n", __func__, cmd[1]);
	pm_qos_update_request(&mif_request_from_acpm, cmd[1]);
}

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
static int acpm_cpu_tmu_notifier(struct notifier_block *nb, unsigned long event, void *v)
{
	unsigned int *is_cold_temp = v;
	int i;
	int ret = NOTIFY_OK;

	if (event != TMU_COLD)
		return ret;

	if (!acpm_dvfs.cpu_coldtemp)
		return ret;

	for (i = 0; i < acpm_dvfs.cpu_len; i++)
		exynos_acpm_set_cold_temp(GET_IDX(acpm_dvfs.cpu_coldtemp[i]),
					*is_cold_temp);

	return NOTIFY_OK;
}
#endif

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
static int acpm_gpu_tmu_notifier(struct notifier_block *nb, unsigned long event, void *v)
{
	unsigned int *is_cold_temp = v;
	int i;
	int ret = NOTIFY_OK;

	if (event != GPU_COLD && event != GPU_NORMAL)
		return ret;

	if (!acpm_dvfs.gpu_coldtemp)
		return ret;

	for (i = 0; i < acpm_dvfs.gpu_len; i++)
		exynos_acpm_set_cold_temp(GET_IDX(acpm_dvfs.gpu_coldtemp[i]),
					*is_cold_temp);

	return ret;
}
#endif

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
static void acpm_dvfs_get_cpu_cold_temp_list(struct device *dev)
{
	struct device_node *node = dev->of_node;
	int proplen;

	proplen = of_property_count_u32_elems(node, "cpu_cold_temp_list");

	if (proplen <= 0)
		return;

	acpm_dvfs.cpu_coldtemp = kcalloc(proplen, sizeof(u32), GFP_KERNEL);

	if (!acpm_dvfs.cpu_coldtemp) {
		pr_err("acpm_dvfs memory allocation fail\n");
		return;
	}

	of_property_read_u32_array(node, "cpu_cold_temp_list",
					acpm_dvfs.cpu_coldtemp, proplen);

	acpm_dvfs.cpu_len = proplen;

	acpm_dvfs.cpu_tmu_notifier.notifier_call = acpm_cpu_tmu_notifier;
	if (exynos_tmu_add_notifier(&acpm_dvfs.cpu_tmu_notifier))
		dev_err(dev, "failed register cpu tmu notifier\n");

}
#endif

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
static void acpm_dvfs_get_gpu_cold_temp_list(struct device *dev)
{
	struct device_node *node = dev->of_node;
	int proplen;

	proplen = of_property_count_u32_elems(node, "gpu_cold_temp_list");

	if (proplen <= 0)
		return;

	acpm_dvfs.gpu_coldtemp = kcalloc(proplen, sizeof(u32), GFP_KERNEL);
	if (!acpm_dvfs.gpu_coldtemp) {
		pr_err("acpm_dvfs memory allocation fail\n");
		return;
	}

	of_property_read_u32_array(node, "gpu_cold_temp_list",
					acpm_dvfs.gpu_coldtemp, proplen);

	acpm_dvfs.gpu_len = proplen;

	acpm_dvfs.gpu_tmu_notifier.notifier_call = acpm_gpu_tmu_notifier;
	if (exynos_gpu_add_notifier(&acpm_dvfs.gpu_tmu_notifier))
		dev_err(dev, "failed register gpu tmu notifier\n");
}
#endif

static int acpm_dvfs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	acpm_noti_mif.dev = dev->of_node;

	ret = acpm_ipc_request_channel(acpm_noti_mif.dev,
				 acpm_noti_mif_callback,
				 &acpm_noti_mif.ch_num,
				 &acpm_noti_mif.size);

	if (ret < 0)
		return ret;

	pm_qos_add_request(&mif_request_from_acpm, PM_QOS_BUS_THROUGHPUT, 0);

#ifndef CONFIG_EXYNOS_ACPM_THERMAL
	acpm_dvfs_get_cpu_cold_temp_list(dev);
	acpm_dvfs_get_gpu_cold_temp_list(dev);
#endif

	return ret;
}

static int acpm_dvfs_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id acpm_dvfs_match[] = {
	{ .compatible = "samsung,exynos-acpm-dvfs" },
	{},
};

static struct platform_driver samsung_acpm_dvfs_driver = {
	.probe	= acpm_dvfs_probe,
	.remove	= acpm_dvfs_remove,
	.driver	= {
		.name = "exynos-acpm-dvfs",
		.owner	= THIS_MODULE,
		.of_match_table	= acpm_dvfs_match,
	},
};

static int __init exynos_acpm_dvfs_init(void)
{
	int ret;

	ret = acpm_ipc_request_channel(acpm_dvfs.dev,
				 NULL,
				 &acpm_dvfs.ch_num,
				 &acpm_dvfs.size);

	if (ret < 0)
		pr_err("acpm_dvfs_init fail ret = %d\n", ret);

	return platform_driver_register(&samsung_acpm_dvfs_driver);
}
fs_initcall(exynos_acpm_dvfs_init);

void __init exynos_acpm_set_device(void *dev)
{
	acpm_dvfs.dev = dev;
}
