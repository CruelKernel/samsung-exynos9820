#include <linux/types.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/module.h>

#include "fimc-is-pmic.h"

/**
 * PMIC Vout Tables
 */
#if 0
static const struct comp_pmic_vout evt0_vout_tables[] = {
	{0, 850000, "0.850"}, /* defualt voltage for EVT0 */
};
#endif

#if defined(CONFIG_COMPANION_C3_USE)
static const struct comp_pmic_vout vout_tables[] = {
	{0, 800000, "0.800"}, /* defualt voltage for EVT1 */
	{1, 700000, "0.700"},
	{2, 725000, "0.725"},
	{3, 750000, "0.750"},
	{4, 775000, "0.775"},
	{5, 800000, "0.800"},
	{6, 825000, "0.825"},
};
#else
static const struct comp_pmic_vout vout_tables[] = {
	{0, 800000, "0.800"}, /* defualt voltage for EVT1 */
	{1, 675000, "0.675"},
	{2, 700000, "0.700"},
	{3, 725000, "0.725"},
	{4, 750000, "0.750"},
	{5, 775000, "0.775"},
	{6, 800000, "0.800"},
	{7, 825000, "0.825"},
};
#endif


/**
 * comp_pmic_get_default_vout_val: get i2c register value to set vout of dcdc regulator.
 */
static int comp_pmic_get_default_vout_val(int *vout_val, const char **vout_str)
{
	if (!vout_val || !vout_str) {
		err("invalid parameter");
		return -EINVAL;
	}

	/* Return default vout for both EVT0 and EVT1, not in vout table */
	*vout_val = COMP_DEFAULT_VOUT_VAL;
	*vout_str = COMP_DEFAULT_VOUT_STR;

	return 0;
}

/**
 * comp_pmic_get_vout_val: get i2c register value to set vout of dcdc regulator.
 */
static int comp_pmic_get_vout_val(int sel)
{
	int i, vout = vout_tables[0].val;

	if (sel < 0)
		err("invalid sel %d", sel);

	for (i = 0; ARRAY_SIZE(vout_tables); i++) {
		if (vout_tables[i].sel == sel) {
			return vout_tables[i].val;
		}
	}

	err("default voltage selected. sel %d", sel);

	return vout;
}

/**
 * comp_pmic_get_vout_name: get voltage name of vout as string.
 */
static const char *comp_pmic_get_vout_str(int sel)
{
	const char *vout = vout_tables[0].vout;
	int i;

	if (sel < 0)
		err("invalid sel %d", sel);

	for (i = 0; ARRAY_SIZE(vout_tables); i++) {
		if (vout_tables[i].sel == sel) {
			return vout_tables[i].vout;
		}
	}

	err("default voltage selected. sel %d", sel);

	return vout;
}

/**
 * comp_pmic_set_vsel0_vout: set voltage of pmic's regulator.
 */
static int comp_pmic_set_vout(struct i2c_client *client, int vout)
{
	struct regulator *regulator = NULL;
	int ret;

	if (vout <= 0) {
		err("invalid vout (%d)", vout);
		return -EINVAL;
	}

	regulator = regulator_get(NULL, "VDDD_CORE_0.8V_COMP");
	if (IS_ERR_OR_NULL(regulator)) {
		err("regulator_get fail\n");
		return PTR_ERR(regulator);
	}

	/* info("%s: regulator_set_voltage(%d)\n", __func__, vout); */
	ret = regulator_set_voltage(regulator, vout, vout);
	if(ret) {
		err("regulator_set_voltage(%d) fail\n", ret);
	}

	regulator_put(regulator);
	return ret;
}

/**
 * comp_pmic_init: init dcdc_power structure.
 */
void comp_pmic_init(struct dcdc_power *dcdc, struct i2c_client *client)
{
	static char *dcdc_name = "PMIC";

	dcdc->client = client;
	dcdc->type = DCDC_VENDOR_PMIC;
	dcdc->name = dcdc_name;
	dcdc->get_default_vout_val = comp_pmic_get_default_vout_val;
	dcdc->get_vout_val = comp_pmic_get_vout_val;
	dcdc->get_vout_str = comp_pmic_get_vout_str;
	dcdc->set_vout = comp_pmic_set_vout;
}

MODULE_DESCRIPTION("Exynos FIMC-IS PMIC driver");
MODULE_LICENSE("GPL");
