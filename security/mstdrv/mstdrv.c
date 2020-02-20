/*
 * MST drv Support
 *
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/smc.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#if defined(CONFIG_MST_LPM_CONTROL)
#include <linux/exynos-pci-ctrl.h>
#endif
//#include <mach/exynos-pm.h>
#include "mstdrv.h"

#if defined(CONFIG_MST_TEEGRIS)
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "../../drivers/misc/tzdev/include/tzdev/tee_client_api.h"
#endif

#define MST_LDO3_0 "MST_LEVEL_3.0V"
#define MST_NOT_SUPPORT		(0x1 << 3)

static int mst_pwr_en;
static struct class *mst_drv_class;
struct device *mst_drv_dev;
static int escape_loop = 1;
//static int rt;
static int nfc_state;
static struct wake_lock mst_wakelock;
#if defined(CONFIG_MFC_CHARGER)
static int wpc_det;
#endif
#if defined(CONFIG_MST_TEEGRIS)
/* swd_ta_mstdrv */
/*
   static TEEC_UUID uuid_driver = {
   0,0,0,{0,0,0,0x4d,0x53,0x54,0x44,0x56}
   };
   */
/* swd_ta_mst */
static TEEC_UUID uuid_ta = {
	0,0,0,{0,0,0x6d,0x73,0x74,0x5f,0x54,0x41}
};

enum cmd_drv_client
{
	/* ta cmd */
	CMD_OPEN,
	CMD_CLOSE,
	CMD_IOCTL,
	CMD_TRACK1,
	CMD_TRACK2,
	CMD_MAX
};
#endif

EXPORT_SYMBOL_GPL(mst_drv_dev);

#if defined(CONFIG_MFC_CHARGER)
#define MST_MODE_ON		1
#define MST_MODE_OFF		0
static inline struct power_supply *get_power_supply_by_name(char *name)
{
	if (!name)
		return (struct power_supply *)NULL;
	else
		return power_supply_get_by_name(name);
}

#define psy_do_property(name, function, property, value) \
{    \
	struct power_supply *psy;    \
	int ret;    \
	psy = get_power_supply_by_name((name));    \
	if (!psy) {    \
		pr_err("%s: Fail to "#function" psy (%s)\n",    \
				__func__, (name));    \
		value.intval = 0;    \
	} else {    \
		if (psy->desc->function##_property != NULL) { \
			ret = psy->desc->function##_property(psy, (property), &(value)); \
			if (ret < 0) {    \
				pr_err("%s: Fail to %s "#function" (%d=>%d)\n", \
						__func__, name, (property), ret);    \
				value.intval = 0;    \
			}    \
		}    \
	}    \
}
#endif

extern void mst_ctrl_of_mst_hw_onoff(bool on)
{
#if defined(CONFIG_MFC_CHARGER)
	union power_supply_propval value;	/* power_supply prop */
#endif

	printk("[MST] mst-drv : mst_ctrl : mst_power_onoff : %d\n", on);

	if (on) {
		printk("[MST] %s : nfc_status gets back to 0(unlock)\n",
				__func__);
		nfc_state = 0;
	} else {
		gpio_set_value(mst_pwr_en, 0);
		printk("%s : mst_pwr_en gets the LOW\n", __func__);

		usleep_range(800, 1000);
		printk("%s : msleep(1)\n", __func__);

#if defined(CONFIG_MFC_CHARGER)
		value.intval = MST_MODE_OFF;
		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		printk("%s : MST_MODE_OFF notify : %d\n", __func__,
				value.intval);
#endif
#if defined(CONFIG_MST_LPM_CONTROL)
		/* PCIe LPM Enable */
		printk("%s : l1ss enable, id : %d\n", __func__, PCIE_L1SS_CTRL_MST);
		exynos_pcie_l1ss_ctrl(1, PCIE_L1SS_CTRL_MST);
#endif

		printk("%s : nfc_status gets 1(lock)\n", __func__);
		nfc_state = 1;
	}
}

static void of_mst_hw_onoff(bool on)
{
#if defined(CONFIG_MFC_CHARGER)
	union power_supply_propval value;	/* power_supply prop */
	int retry_cnt = 8;
#endif

	printk("mst-drv : mst_power_onoff : %d\n", on);

	if (nfc_state == 1) {
		printk("[MST] %s : nfc_state is on!!!\n", __func__);
		return;
	}

	if (on) {
#if defined(CONFIG_MFC_CHARGER)
		printk("%s : MST_MODE_ON notify start\n", __func__);
		value.intval = MST_MODE_ON;

		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		printk("%s : MST_MODE_ON notified : %d\n", __func__,
				value.intval);
#endif
		gpio_set_value(mst_pwr_en, 1);
		printk("%s : mst_pwr_en gets the HIGH\n", __func__);

#if defined(CONFIG_MST_LPM_CONTROL)
		/* PCIe LPM Disable */
		printk("%s : l1ss disable, id : %d\n", __func__, PCIE_L1SS_CTRL_MST);
		exynos_pcie_l1ss_ctrl(0, PCIE_L1SS_CTRL_MST);
#endif
		mdelay(40);

#if defined(CONFIG_MFC_CHARGER)
		while (--retry_cnt) {
			psy_do_property("mfc-charger", get,
					POWER_SUPPLY_PROP_TECHNOLOGY, value);
			//printk("%s : check mst mode status : %d\n", __func__, value.intval);
			if (value.intval == 0x02) {
				printk("%s : mst mode set!!! : %d\n", __func__,
						value.intval);
				retry_cnt = 1;
				break;
			}
			usleep_range(3600, 4000);
		}

		if (!retry_cnt) {
			printk("%s : timeout !!! : %d\n", __func__,
					value.intval);
		}
#endif
	} else {
		gpio_set_value(mst_pwr_en, 0);
		printk("%s : mst_pwr_en gets the LOW\n", __func__);

		usleep_range(800, 1000);
		printk("%s : msleep(1)\n", __func__);

#if defined(CONFIG_MFC_CHARGER)
		value.intval = MST_MODE_OFF;
		psy_do_property("mfc-charger", set,
				POWER_SUPPLY_PROP_TECHNOLOGY, value);
		printk("%s : MST_MODE_OFF notify : %d\n", __func__,
				value.intval);
#endif

#if defined(CONFIG_MST_LPM_CONTROL)
		/* PCIe LPM Enable */
		printk("%s : l1ss enable, id : %d\n", __func__, PCIE_L1SS_CTRL_MST);
		exynos_pcie_l1ss_ctrl(1, PCIE_L1SS_CTRL_MST);
#endif
	}
}

#if defined(CONFIG_MST_TEEGRIS)
uint32_t transmit_mst_teegris(uint32_t cmd)
{
	TEEC_Context context;
	TEEC_Session session_ta;
	//TEEC_Session session_driver;
	TEEC_Operation operation;
	TEEC_Result result;

	uint32_t origin;

	origin = 0x0;
	operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	printk("TEST: TEEC_InitializeContext\n");
	result = TEEC_InitializeContext(NULL, &context);
	if (result != TEEC_SUCCESS) {
		printk("TEST: TEEC_InitializeContext failed, %d\n", result);
		goto exit;
	}
	/*
	   printk("TEST: TEEC_OpenSession (driver)\n");
	   result = TEEC_OpenSession(&context, &session_driver, &uuid_driver, 0,
	   NULL, &operation, &origin);
	   if (result != TEEC_SUCCESS) {
	   printk("TEST: TEEC_OpenSession(driver) failed, %d\n", result);
	   goto finalize_context;
	   }
	   */
	printk("TEST: TEEC_OpenSession (ta)\n");
	result = TEEC_OpenSession(&context, &session_ta, &uuid_ta, 0,
			NULL, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		printk("TEST: TEEC_OpenSession(ta) failed, %d\n", result);
		goto finalize_context;
	}

	printk("TEST: TEEC_InvokeCommand (CMD_OPEN)\n");
	result = TEEC_InvokeCommand(&session_ta, CMD_OPEN, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		printk("TEST: TEEC_InvokeCommand(OPEN) failed, %d\n", result);
		goto ta_close_session;
	}

	/* MST IOCTL - transmit track1 */
	printk("TEST: TEEC_InvokeCommand (CMD_TRACK1 or CMD_TRACK2)\n");
	result = TEEC_InvokeCommand(&session_ta, cmd, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		printk("TEST: TEEC_InvokeCommand(CMD_TRACK1 or CMD_TRACK2) failed, %d\n", result);
		goto ta_close_session;
	}

	printk("TEST: TEEC_InvokeCommand (CMD_CLOSE)\n");
	result = TEEC_InvokeCommand(&session_ta, CMD_CLOSE, &operation, &origin);
	if (result != TEEC_SUCCESS) {
		printk("TEST: TEEC_InvokeCommand(CLOSE) failed, %d\n", result);
		goto ta_close_session;
	}

ta_close_session:
	TEEC_CloseSession(&session_ta);
	//driver_close_session:
	//TEEC_CloseSession(&session_driver);
finalize_context:
	TEEC_FinalizeContext(&context);
exit:
	return result;
}

/*
   uint32_t mst_driver_test_teegris(uint32_t cmd)
   {
   TEEC_Context context;
   TEEC_Session session_driver;
   TEEC_Operation operation;
   TEEC_Result result;

   uint32_t origin;

   printk("TEST: Begin mst_driver_test_teegris() +\n");

   origin = 0x0;
   operation.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);

   printk("TEST: TEEC_InitializeContext\n");
   result = TEEC_InitializeContext(NULL, &context);
   if (result != TEEC_SUCCESS) {
   printk("TEST: TEEC_InitializeContext failed, %d\n", result);
   goto exit;
   }

   printk("TEST: TEEC_OpenSession (driver)\n");
   result = TEEC_OpenSession(&context, &session_driver, &uuid_driver, 0,
   NULL, &operation, &origin);
   if (result != TEEC_SUCCESS) {
   printk("TEST: TEEC_OpenSession(driver) failed, %d\n", result);
   goto finalize_context;
   }

   printk("TEST: TEEC_InvokeCommand\n");
   result = TEEC_InvokeCommand(&session_driver, cmd, &operation, &origin);
   if (result != TEEC_SUCCESS) {
   printk("TEST: TEEC_InvokeCommand(CLOSE) failed, %d\n", result);
   goto driver_close_session;
   }

driver_close_session:
TEEC_CloseSession(&session_driver);
finalize_context:
TEEC_FinalizeContext(&context);
exit:
printk("TEST: Begin mst_ta_driver_test_teegris() -, %d\n", result);

return result;
}
*/
#endif

static ssize_t show_mst_drv(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!dev)
		return -ENODEV;
	// todo
	if (escape_loop == 0) {
		return sprintf(buf, "%s\n", "activating");
	} else {
		return sprintf(buf, "%s\n", "waiting");
	}
}

static ssize_t store_mst_drv(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	u64 r0 = 0, r1 = 0, r2 = 0, r3 = 0;
	char test_result[256] = { 0, };
	int result = 0;
#if defined(CONFIG_MFC_CHARGER)
	struct device_node *np;
	enum of_gpio_flags irq_gpio_flags;
#endif

	sscanf(buf, "%20s\n", test_result);
	printk(KERN_ERR "MST Store test result : %s\n", test_result);

#if defined(CONFIG_MFC_CHARGER)
	if (wpc_det < 0) {
		np = of_find_node_by_name(NULL, "battery");
		if (!np) {
			pr_err("%s np NULL\n", __func__);
		} else {
			/* wpc_det */
			wpc_det = of_get_named_gpio_flags(np, "battery,wpc_det",
					0, &irq_gpio_flags);
			if (wpc_det < 0) {
				dev_err(dev, "%s : can't get wpc_det = %d\n", __FUNCTION__, wpc_det);
			}
		}
	}

	if (wpc_det && (gpio_get_value(wpc_det) == 1)) {
		printk("[MST] Wireless charging is ongoing, do not proceed MST work\n");
		return count;
	}
#endif

	switch (test_result[0]) {
		case '1':
			of_mst_hw_onoff(1);
			break;

		case '0':
			of_mst_hw_onoff(0);
			break;

		case '2':
#if !defined(CONFIG_MFC_CHARGER)
			of_mst_hw_onoff(1);
#endif
#if defined(CONFIG_MST_TEEGRIS)
			printk("MST_LDO_DRV]]] Call to Blowfish ta_mst for TRACK1\n");
			result = transmit_mst_teegris(CMD_TRACK1);
			printk(KERN_INFO "MST_LDO_DRV]]] Track1 data sent : %d\n", result);
#else
			printk(KERN_INFO "%s\n", __func__);
			printk(KERN_INFO "MST_LDO_DRV]]] Track1 data transmit\n");
			//Will Add here
			r0 = (0x8300000f);
			r1 = 1;
			result = exynos_smc(r0, r1, r2, r3);
			printk(KERN_INFO "MST_LDO_DRV]]] Track1 data sent : %d\n", result);
#endif
#if !defined(CONFIG_MFC_CHARGER)
			of_mst_hw_onoff(0);
#endif
			break;

		case '3':
#if !defined(CONFIG_MFC_CHARGER)
			of_mst_hw_onoff(1);
#endif
#if defined(CONFIG_MST_TEEGRIS)
			printk("MST_LDO_DRV]]] Call to Blowfish ta_mst for TRACK2\n");
			result = transmit_mst_teegris(CMD_TRACK2);
			printk(KERN_INFO "MST_LDO_DRV]]] Track2 data sent : %d\n", result);
#else
			printk(KERN_INFO "%s\n", __func__);
			printk(KERN_INFO "MST_LDO_DRV]]] Track2 data transmit\n");
			//Will Add here
			r0 = (0x8300000f);
			r1 = 2;
			result = exynos_smc(r0, r1, r2, r3);
			printk(KERN_INFO "MST_LDO_DRV]]] Track2 data sent : %d\n", result);
#endif
#if !defined(CONFIG_MFC_CHARGER)
			of_mst_hw_onoff(0);
#endif
			break;

		case '4':
			if (escape_loop) {
				wake_lock_init(&mst_wakelock, WAKE_LOCK_SUSPEND,
						"mst_wakelock");
				wake_lock(&mst_wakelock);
			}
			escape_loop = 0;
			while (1) {
				if (escape_loop == 1)
					break;
#if !defined(CONFIG_MFC_CHARGER)
				of_mst_hw_onoff(1);
#endif
				mdelay(10);
				printk("MST_LDO_DRV]]] Continuous Track2 data transmit\n");
				r0 = (0x8300000f);
				r1 = 2;
				result = exynos_smc(r0, r1, r2, r3);
				printk(KERN_INFO
						"MST_LDO_DRV]]] Continuous Track2 data transmit after smc : %d\n",
						result);
#if !defined(CONFIG_MFC_CHARGER)
				of_mst_hw_onoff(0);
#endif
				mdelay(1000);
			}
			break;

		case '5':
			if (!escape_loop)
				wake_lock_destroy(&mst_wakelock);
			escape_loop = 1;
			printk("MST escape_loop value = 1\n");
			break;

		default:
			printk(KERN_ERR "MST invalid value : %s\n", test_result);
			break;
	}
	return count;
}

static DEVICE_ATTR(transmit, 0770, show_mst_drv, store_mst_drv);

#if defined(CONFIG_MFC_CHARGER)
static ssize_t show_mfc(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if (!dev)
		return -ENODEV;

	return sprintf(buf, "%s\n", "mfc_charger");
}

static ssize_t store_mfc(struct device *dev,
		struct device_attribute *attr, const char *buf,
		size_t count)
{
	return count;
}

static DEVICE_ATTR(mfc, 0770, show_mfc, store_mfc);
#endif

static int sec_mst_gpio_init(struct device *dev)
{
	int ret = 0;
#if defined(CONFIG_MFC_CHARGER)
	struct device_node *np;
	enum of_gpio_flags irq_gpio_flags;

	/* get wireless chraging check gpio */
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s np NULL\n", __func__);
	} else {
		/* wpc_det */
		wpc_det = of_get_named_gpio_flags(np, "battery,wpc_det",
				0, &irq_gpio_flags);
		if (wpc_det < 0) {
			dev_err(dev, "%s : can't get wpc_det = %d\n", __FUNCTION__, wpc_det);
		}
	}
#endif

	mst_pwr_en = of_get_named_gpio(dev->of_node, "sec-mst,mst-pwr-gpio", 0);

	printk("[MST] Data Value : %d\n", mst_pwr_en);

	/* check if gpio pin is inited */
	if (mst_pwr_en < 0) {
		printk(KERN_ERR "%s : Cannot create the gpio\n", __func__);
		return 1;
	}
	printk(KERN_ERR "MST_DRV]]] gpio pwr en inited\n");

	/* gpio request */
	ret = gpio_request(mst_pwr_en, "sec-mst,mst-pwr-gpio");
	if (ret) {
		printk(KERN_ERR "[MST] failed to get en gpio : %d, %d\n", ret,
				mst_pwr_en);
	}

	/* set gpio direction */
	if (!(ret < 0) && (mst_pwr_en > 0)) {
		gpio_direction_output(mst_pwr_en, 0);
		printk(KERN_ERR "%s : Send Output\n", __func__);
	}

	return 0;
}

static int mst_ldo_device_probe(struct platform_device *pdev)
{
	int retval = 0;

	printk("%s init start\n", __func__);

	if (sec_mst_gpio_init(&pdev->dev))
		return -1;

	printk(KERN_ALERT "%s\n", __func__);

	mst_drv_class = class_create(THIS_MODULE, "mstldo");
	if (IS_ERR(mst_drv_class)) {
		retval = PTR_ERR(mst_drv_class);
		goto error;
	}

	mst_drv_dev = device_create(mst_drv_class,
			NULL /* parent */, 0 /* dev_t */,
			NULL /* drvdata */,
			MST_DRV_DEV);
	if (IS_ERR(mst_drv_dev)) {
		retval = PTR_ERR(mst_drv_dev);
		goto error_destroy;
	}

	/* register this mst device with the driver core */
	retval = device_create_file(mst_drv_dev, &dev_attr_transmit);
	if (retval)
		goto error_destroy;

#if defined(CONFIG_MFC_CHARGER)
	retval = device_create_file(mst_drv_dev, &dev_attr_mfc);
	if (retval)
		goto error_destroy;
#endif

	printk(KERN_DEBUG "MST drv driver (%s) is initialized.\n", MST_DRV_DEV);
	return 0;

error_destroy:
	kfree(mst_drv_dev);
	device_destroy(mst_drv_class, 0);
error:
	printk(KERN_ERR "%s: MST drv driver initialization failed\n", __FILE__);
	return retval;
}

static struct of_device_id mst_match_ldo_table[] = {
	{.compatible = "sec-mst",},
	{},
};

static int mst_ldo_device_suspend(struct platform_device *dev, pm_message_t state)
{
	uint8_t is_mst_pwr_on;

	is_mst_pwr_on = gpio_get_value(mst_pwr_en);
	if (is_mst_pwr_on == 1) {
		pr_info("%s: mst power is on, is_mst_pwr_on = %d\n", __func__, is_mst_pwr_on);
		pr_info("%s: mst power off\n", __func__);
		of_mst_hw_onoff(0);
	} else {
		pr_info("%s: mst power is off, is_mst_pwr_on = %d\n", __func__, is_mst_pwr_on);
	}

	return 0;
}

#if 0
static int mst_ldo_device_resume(struct platform_device *dev)
{
	u64 r0 = 0, r1 = 0, r2 = 0, r3 = 0;
	int result = 0;

	printk(KERN_INFO "%s\n", __func__);
	printk(KERN_INFO "MST_LDO_DRV]]] resume");
	//Will Add here
	r0 = (0x8300000d);
	result = exynos_smc(r0, r1, r2, r3);
	if (result == MST_NOT_SUPPORT) {
		printk(KERN_INFO
				"MST_LDO_DRV]]] resume do nothing after smc : %x\n",
				result);
	} else {
		printk(KERN_INFO
				"MST_LDO_DRV]]] resume success after smc : %x\n",
				result);
	}

	rt = result;
	return 0;
}
#endif

static struct platform_driver sec_mst_ldo_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "mstldo",
		.of_match_table = mst_match_ldo_table,
	},
	.probe = mst_ldo_device_probe,
	.suspend = mst_ldo_device_suspend,
	//.resume = mst_ldo_device_resume,
};

static int __init mst_drv_init(void)
{
	int ret = 0;
	printk(KERN_ERR "%s\n", __func__);

	ret = platform_driver_register(&sec_mst_ldo_driver);
	printk(KERN_ERR "MST_LDO_DRV]]] init , ret val : %d\n", ret);
	return ret;
}

static void __exit mst_drv_exit(void)
{
	class_destroy(mst_drv_class);
	printk(KERN_ALERT "%s\n", __func__);
}

MODULE_AUTHOR("JASON KANG, j_seok.kang@samsung.com");
MODULE_DESCRIPTION("MST drv driver");
MODULE_VERSION("0.1");
module_init(mst_drv_init);
module_exit(mst_drv_exit);
