/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/delay.h>

#include <linux/exynos_iovmm.h>
#include <linux/pm_runtime.h>

#include "npu-config.h"
#include "vs4l.h"
#include "npu-device.h"
#include "vision-dev.h"
#include "vision-ioctl.h"
#include "npu-log.h"
#include "npu-debug.h"
#include "npu-protodrv.h"
#include "npu-profile.h"
#include "npu-util-memdump.h"

#ifdef CONFIG_EXYNOS_NPU_PUBLISH_NPU_BUILD_VER
#include "npu-ver-info.h"
#endif

#ifdef CONFIG_NPU_LOOPBACK
#include "npu-mailbox-mgr-mock.h"
#endif

extern struct system_pwr sysPwr;
extern int npu_system_save_result(struct npu_session *session, struct nw_result nw_result);

static int __npu_device_start(struct npu_device *device)
{
	int ret = 0;

	if (test_bit(NPU_DEVICE_STATE_START, &device->state)) {
		npu_err("already started\n");
		ret = -EINVAL;
		goto ErrorExit;
	}

	ret = npu_system_start(&device->system);
	if (ret) {
		npu_err("fail(%d) in npu_system_start\n", ret);
		goto ErrorExit;
	}

	ret = npu_debug_start(device);
	if (ret) {
		npu_err("fail(%d) in npu_debug_start\n", ret);
		goto ErrorExit;
	}

	ret = npu_log_start(device);
	if (ret) {
		npu_err("fail(%d) in npu_log_start\n", ret);
		goto ErrorExit;
	}

	ret = proto_drv_start(device);
	if (ret) {
		npu_err("fail(%d) in proto_drv_start\n", ret);
		goto ErrorExit;
	}

#ifdef CONFIG_NPU_LOOPBACK
	ret = mailbox_mgr_mock_start(device);
	if (ret) {
		npu_err("fail(%d) in mailbox_mgr_mock_start\n", ret);
		goto ErrorExit;
	}
#endif

	set_bit(NPU_DEVICE_STATE_START, &device->state);
ErrorExit:
	return ret;
}

static int __npu_device_stop(struct npu_device *device)
{
	int ret = 0;

	if (!test_bit(NPU_DEVICE_STATE_START, &device->state))
		goto ErrorExit;

#ifdef CONFIG_NPU_LOOPBACK
	ret = mailbox_mgr_mock_stop(device);
	if (ret)
		npu_err("fail(%d) in mailbox_mgr_mock_stop\n", ret);
#endif
	ret = proto_drv_stop(device);
	if (ret)
		npu_err("fail(%d) in proto_drv_stop\n", ret);

	ret = npu_log_stop(device);
	if (ret)
		npu_err("fail(%d) in npu_log_stop\n", ret);

	ret = npu_debug_stop(device);
	if (ret)
		npu_err("fail(%d) in npu_debug_stop\n", ret);

	ret = npu_system_stop(&device->system);
	if (ret)
		npu_err("fail(%d) in npu_system_stop\n", ret);

	clear_bit(NPU_DEVICE_STATE_START, &device->state);

ErrorExit:
	return ret;
}

static int __npu_device_power_on(struct npu_device *device)
{
	int ret = 0;

	ret = pm_runtime_get_sync(device->dev);
	if (ret)
		npu_err("fail(%d) in runtime resume\n", ret);

	if (!test_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state)) {
		if (ret != -ETIMEDOUT)
			BUG_ON(ret < 0);
		else
			npu_err("EMERGENCY_RECOVERY during %s.\n", __func__);
	}
	return ret;
}

static int __npu_device_power_off(struct npu_device *device)
{
	int ret = 0;

	if (test_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state)) {
		npu_info("EMERGENCY_RECOVERY - %ums delay insearted before power down.\n",
			POWER_DOWN_DELAY_ON_EMERGENCY);
		msleep(POWER_DOWN_DELAY_ON_EMERGENCY);
	}

	ret = pm_runtime_put_sync(device->dev);
	if (ret)
		npu_err("fail(%d) in runtime suspend\n", ret);
	BUG_ON(ret < 0);

	return ret;
}

/*
 * Called after all the open procedure is completed.
 * NPU H/W is fully working at this stage
 */
static int __npu_device_late_open(struct npu_device *device)
{
	int ret;

	npu_trace("starting\n");

	ret = npu_profile_open(&device->system);
	if (ret) {
		npu_err("fail(%d) in npu_profile_open\n", ret);
		goto err_exit;
	}

	ret = 0;

err_exit:
	if (ret)
		npu_err("error occurred: ret = %d\n", ret);
	else
		npu_trace("completed\n");
	return ret;
}

/*
 * Called before the close procedure is started.
 * NPU H/W is fully working at this stage
 */
static int __npu_device_early_close(struct npu_device *device)
{
	int ret;

	npu_trace("Starting.\n");

	ret = npu_profile_close(&device->system);
	if (ret) {
		npu_err("fail(%d) in npu_profile_close\n", ret);
		goto err_exit;
	}

	ret = 0;

err_exit:
	if (ret)
		npu_err("error occurred : ret = %d\n", ret);
	else
		npu_trace("completed\n");
	return ret;
}

static int npu_iommu_fault_handler(struct iommu_domain *domain,
		struct device *dev, unsigned long fault_addr,
		int fault_flags, void *token)
{
	struct npu_device	*device = token;

	dev_err(dev, "%s() is called with fault addr(0x%lx), mode(%pK), state(%pK))\n",
			__func__, fault_addr, &device->mode, &device->state);

	npu_util_dump_handle_error_k(device);

	return 0;
}

static int npu_device_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct npu_device *device;

	BUG_ON(!pdev);

	dev = &pdev->dev;

#ifdef CONFIG_EXYNOS_NPU_PUBLISH_NPU_BUILD_VER
	probe_info("NPU Device driver build info :\n%s\n%s\n",
		npu_build_info, npu_git_log_str);
#endif
	device = devm_kzalloc(dev, sizeof(*device), GFP_KERNEL);
	if (!device) {
		probe_err("fail in devm_kzalloc\n");
		ret = -ENOMEM;
		goto err_exit;
	}
	device->dev = dev;

	ret = npu_system_probe(&device->system, pdev);
	if (ret) {
		probe_err("fail(%d) in npu_system_probe\n", ret);
		ret = -EINVAL;
		goto err_exit;
	}

	/* Probe sub modules */
	ret = npu_debug_probe(device);
	if (ret) {
	    probe_err("fail(%d) in npu_debug_probe\n", ret);
		goto err_exit;
	}

	ret = npu_log_probe(device);
	if (ret) {
		probe_err("fail(%d) in npu_log_probe\n", ret);
		goto err_exit;
	}

	ret = npu_vertex_probe(&device->vertex, dev);
	if (ret) {
		probe_err("fail(%d) in npu_vertex_probe\n", ret);
		goto err_exit;
	}

	ret = proto_drv_probe(device);
	if (ret) {
		probe_err("fail(%d) in proto_drv_probe\n", ret);
		goto err_exit;
	}

	ret = npu_sessionmgr_probe(&device->sessionmgr);

#ifdef CONFIG_NPU_GOLDEN_MATCH
	ret = register_golden_matcher(dev);
	if (ret) {
		probe_err("fail(%d) in register_golden_matcher\n", ret);
		goto err_exit;
	}
#endif

#ifdef CONFIG_NPU_LOOPBACK
	ret = mailbox_mgr_mock_probe(device);
	if (ret) {
		probe_err("fail(%d) in mailbox_mgr_mock_probe\n", ret);
		goto err_exit;
	}
#endif
	ret = npu_profile_probe(&device->system);
	if (ret) {
		probe_err("fail(%d) in npu_profile_probe\n", ret);
		goto err_exit;
	}

	ret = iovmm_activate(dev);
	if (ret < 0)
		return ret;

	iovmm_set_fault_handler(dev, npu_iommu_fault_handler, device);

	dev_set_drvdata(dev, device);

	ret = 0;
	probe_info("complete in %s\n", __func__);

	goto ok_exit;

err_exit:
	probe_err("error on %s ret(%d)\n", __func__, ret);
ok_exit:
	return ret;

}

int npu_device_open(struct npu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	ret = npu_debug_open(device);
	if (ret) {
		npu_err("fail(%d) in npu_debug_open\n", ret);
		goto ErrorExit;
	}

	ret = npu_log_open(device);
	if (ret) {
		npu_err("fail(%d) in npu_log_open\n", ret);
		goto ErrorExit;
	}

	ret = npu_system_open(&device->system);
	if (ret) {
		npu_err("fail(%d) in npu_system_open\n", ret);
		goto ErrorExit;
	}

	ret = __npu_device_power_on(device);
	if (ret) {
		npu_err("fail(%d) in __npu_device_power_on\n", ret);
		goto ErrorExit;
	}
	if (test_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state)) {
		ret = -ELIBACC;
		goto ErrorExit;
	}

	ret = proto_drv_open(device);
	if (ret) {
		npu_err("fail(%d) in proto_drv_open\n", ret);
		goto ErrorExit;
	}

	ret = __npu_device_late_open(device);
	if (ret) {
		npu_err("fail(%d) in __npu_device_late_open\n", ret);
		goto ErrorExit;
	}
	/* clear npu_devcice_err_state to no_error for emergency recover */
	clear_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);

	set_bit(NPU_DEVICE_STATE_OPEN, &device->state);

ErrorExit:
	npu_info("%s():%d\n", __func__, ret);
	return ret;
}

int npu_system_put_req_nw(nw_cmd_e nw_cmd)
{
	int ret = 0;
	struct npu_nw req = {};

	//req.uid = session->uid,
	req.npu_req_id = 0,
	req.result_code = 0,
	//req.session = session,
	req.cmd = nw_cmd,
	//req.ncp_addr = session->ncp_info.ncp_addr,
	//req.magic_tail = NPU_NW_MAGIC_TAIL,
	req.notify_func = npu_system_save_result;

	ret = npu_ncp_mgmt_put(&req);
	if (!ret) {
		npu_err("fail in npu_ncp_mgmt_put");
		return ret;
	}
	return ret;
}

int npu_system_NW_CMD_POWER_DOWN(void)
{
	int ret = 0;
	nw_cmd_e nw_cmd;

	npu_info("start power_down\n");
	nw_cmd = NPU_NW_CMD_POWER_DOWN;

	sysPwr.system_result.result_code = NPU_SYSTEM_JUST_STARTED;
	npu_system_put_req_nw(nw_cmd);
	wait_event(sysPwr.wq, sysPwr.system_result.result_code != NPU_SYSTEM_JUST_STARTED);
	if (sysPwr.system_result.result_code != NPU_ERR_NO_ERROR) {
		ret = -EINVAL;
		goto ErrorExit;
	}
	npu_info("complete in powerdown(%d)\n", ret);
ErrorExit:
	return ret;
}


int npu_device_recovery_close(struct npu_device *device)
{

	int ret = 0;

	ret += npu_system_close(&device->system);
	ret += __npu_device_power_off(device);
	ret += npu_log_close(device);
	ret += npu_debug_close(device);
	if (ret)
		goto err_coma;
	clear_bit(NPU_DEVICE_STATE_OPEN, &device->state);
	clear_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);

	npu_info("%s():%d\n", __func__, ret);
	return ret;
err_coma:
	BUG_ON(1);
}
int npu_device_close(struct npu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	if (!test_bit(NPU_DEVICE_STATE_OPEN, &device->state)) {
		npu_err("already closed of device\n");
		ret = -EINVAL;
		goto ErrorExit;
	}

	ret = __npu_device_early_close(device);
	if (ret)
		npu_err("fail(%d) in __npu_device_early_close\n", ret);

	if (!npu_device_is_emergency_err(device)) {
		ret = npu_system_NW_CMD_POWER_DOWN();
		if (ret) {
			npu_err("fail(%d) in __npu_powerdown\n", ret);
		}
	} else {
		npu_warn("NPU DEVICE IS EMERGENCY ERROR!\n");
	}
#if 0 // 0521_CLEAN_CODE
	ret = __npu_device_stop(device);
	if (ret)
		npu_err("fail(%d) in __npu_device_stop\n", ret);
#endif

#ifdef CONFIG_NPU_LOOPBACK
	ret = mailbox_mgr_mock_close(device);
	if (ret)
		npu_err("fail(%d) in mailbox_mgr_mock_close\n", ret);
#endif
	ret = proto_drv_close(device);
	if (ret)
		npu_err("fail(%d) in proto_drv_close\n", ret);

	ret = npu_system_close(&device->system);
	if (ret)
		npu_err("fail(%d) in npu_system_close\n", ret);

	ret = __npu_device_power_off(device);
	if (ret)
		npu_err("fail(%d) in __npu_device_power_off\n", ret);

	ret = npu_log_close(device);
	if (ret)
		npu_err("fail(%d) in npu_log_close)\n", ret);

	ret = npu_debug_close(device);
	if (ret)
		npu_err("fail(%d) in npu_debug_close\n", ret);

	clear_bit(NPU_DEVICE_STATE_OPEN, &device->state);

	if (test_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state)) {
		npu_dbg("%s() : NPU_DEVICE_ERR_STATE_EMERGENCY bit was set before\n", __func__);
		clear_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);
		npu_dbg("%s() : NPU_DEVICE_ERR_STATE_EMERGENCY bit is clear now", __func__);
		npu_dbg(" (%d) \n", test_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state));
	}

ErrorExit:
	npu_info("%s():%d\n", __func__, ret);
	return ret;
}

int npu_device_start(struct npu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	ret = __npu_device_start(device);
	if (ret)
		npu_err("fail(%d) in __npu_device_start\n", ret);

	npu_info("%s():%d\n", __func__, ret);
	return ret;
}

int npu_device_stop(struct npu_device *device)
{
	int ret = 0;

	BUG_ON(!device);

	ret = __npu_device_stop(device);
	if (ret)
		npu_err("fail(%d) in __npu_device_stop\n", ret);

	npu_info("%s():%d\n", __func__, ret);

	return ret;
}

#ifdef CONFIG_PM_SLEEP
static int npu_device_suspend(struct device *dev)
{
	npu_info("%s()\n", __func__);
	return 0;
}

static int npu_device_resume(struct device *dev)
{
	npu_info("%s()\n", __func__);
	return 0;
}
#endif

#ifdef CONFIG_PM
static int npu_device_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct npu_device *device;

	device = dev_get_drvdata(dev);

	ret = npu_system_suspend(&device->system);
	if (ret)
		npu_err("fail(%d) in npu_system_suspend\n", ret);

	npu_info("%s():%d\n", __func__, ret);
	return ret;
}

static int npu_device_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct npu_device *device;

	device = dev_get_drvdata(dev);
	ret = npu_system_resume(&device->system, device->mode);
	if (ret) {
		npu_err("fail(%d) in npu_system_resume\n", ret);
		goto ErrorExit;
	}

ErrorExit:
	npu_info("%s():%d\n", __func__, ret);
	return ret;
}
#endif

static int npu_device_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev;
	struct npu_device *device;

	BUG_ON(!pdev);

	dev = &pdev->dev;
	device = dev_get_drvdata(dev);

	/* Error will not be propagated to upper layer, but just logging */
	ret = npu_profile_release();
	if (ret)
		npu_err("fail(%d) in npu_profile_release\n", ret);

#ifdef CONFIG_NPU_LOOPBACK
	ret = mailbox_mgr_mock_release();
	if (ret) {
		npu_err("fail(%d) in mailbox_mgr_mock_release\n", ret);
	}
#endif

	ret = proto_drv_release();
	if (ret) {
		npu_err("fail(%d) in proto_drv_release\n", ret);
	}

	ret = npu_debug_release();
	if (ret) {
		npu_err("fail(%d) in npu_debug_release\n", ret);
	}

	ret = npu_log_release(device);
	if (ret) {
		npu_err("fail(%d) in npu_log_release\n", ret);
	}

	ret = npu_system_release(&device->system, pdev);
	if (ret) {
		npu_err("fail(%d) in npu_system_release\n", ret);
	}

	npu_info("completed in %s\n", __func__);
	return 0;
}
/*
int npu_device_emergency_recover(struct npu_device *device){

	npu_dbg("npu_device_emergency_recover!!\n");
	return 0;
}
*/
void npu_device_set_emergency_err(struct npu_device *device)
{
	npu_warn("Emergency flag set.\n");
	set_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);
}

int npu_device_is_emergency_err(struct npu_device *device)
{

	return test_bit(NPU_DEVICE_ERR_STATE_EMERGENCY, &device->err_state);
}

static const struct dev_pm_ops npu_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(npu_device_suspend, npu_device_resume)
	SET_RUNTIME_PM_OPS(npu_device_runtime_suspend, npu_device_runtime_resume, NULL)
};

#ifdef CONFIG_OF
static const struct of_device_id exynos_npu_match[] = {
	{
		.compatible = "samsung,exynos-npu"
	},
	{}
};
MODULE_DEVICE_TABLE(of, exynos_npu_match);
#endif

static struct platform_driver npu_driver = {
	.probe	= npu_device_probe,
	.remove = npu_device_remove,
	.driver = {
		.name	= "exynos-npu",
		.owner	= THIS_MODULE,
		.pm	= &npu_pm_ops,
		.of_match_table = of_match_ptr(exynos_npu_match),
	},
};

static int __init npu_device_init(void)
{
	int ret = platform_driver_register(&npu_driver);

	if (ret) {
		probe_err("error(%d) in platform_driver_register\n", ret);
		goto err_exit;
	}

	probe_info("success in %s\n", __func__);
	ret = 0;
	goto ok_exit;

err_exit:
	// necessary clean-up

ok_exit:
	return ret;
}

static void __exit npu_device_exit(void)
{
	platform_driver_unregister(&npu_driver);
	npu_info("success in %s\n", __func__);
}


late_initcall(npu_device_init);
module_exit(npu_device_exit);


MODULE_AUTHOR("Eungjoo Kim<ej7.kim@samsung.com>");
MODULE_DESCRIPTION("Exynos NPU driver");
MODULE_LICENSE("GPL");
