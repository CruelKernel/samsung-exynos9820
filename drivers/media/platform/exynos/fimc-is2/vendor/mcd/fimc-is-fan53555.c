#include <linux/types.h>
#include <linux/input.h>
#include <linux/errno.h>
#include <linux/err.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#include "fimc-is-fan53555.h"

/**
 * FAN53555 Vout Tables
 */
static const struct fan53555_vout vout_tables[] = {
	{0, FAN53555_VOUT_1P00, "1.00"}, /* defualt voltage */
	{1, FAN53555_VOUT_0P88, "0.88"},
	{2, FAN53555_VOUT_0P90, "0.90"},
	{3, FAN53555_VOUT_0P93, "0.93"},
	{4, FAN53555_VOUT_0P95, "0.95"},
	{5, FAN53555_VOUT_0P98, "0.98"},
	{6, FAN53555_VOUT_1P00, "1.00"},
};

/**
 * fan53555_get_default_vout_val: get i2c register value to set vout of dcdc regulator.
 */
static int fan53555_get_default_vout_val(int *vout_val, const char **vout_str)
{
	if (!vout_val || !vout_str) {
		err("invalid parameter");
		return -EINVAL;
	}

	*vout_val = vout_tables[0].val;
	*vout_str = vout_tables[0].vout;

	return 0;
}

/**
 * fan53555_get_vout_val: get i2c register value to set vout of dcdc regulator.
 */
static int fan53555_get_vout_val(int sel)
{
	int i, vout = vout_tables[0].val;

	if (sel < 0)
		pr_err("%s: error, invalid sel %d\n", __func__, sel);

	for (i = 0; ARRAY_SIZE(vout_tables); i++) {
		if (vout_tables[i].sel == sel) {
			return vout_tables[i].val;
		}
	}

	pr_err("%s: warning, default voltage selected. sel %d\n", __func__, sel);

	return vout;
}

/**
 * fan53555_get_vout_name: get voltage name of vout as string.
 */
static const char *fan53555_get_vout_str(int sel)
{
	const char *vout = vout_tables[0].vout;
	int i;

	if (sel < 0)
		pr_err("%s: error, invalid sel %d\n", __func__, sel);

	for (i = 0; ARRAY_SIZE(vout_tables); i++) {
		if (vout_tables[i].sel == sel) {
			return vout_tables[i].vout;
		}
	}

	pr_err("%s: warning, default voltage selected. sel %d\n", __func__, sel);

	return vout;
}

#ifdef FAN_DEBUG
static int fan53555_enable_vsel0(struct i2c_client *client, int on_off)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, REG_VSEL0);
	if(ret < 0){
		pr_err("%s: read error = %d , try again", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_VSEL0);
		if (ret < 0)
			pr_err("%s: read 2nd error = %d", __func__, ret);
	}

	ret &= (~VSEL0_BUCK_EN0);
	ret |= (on_off << VSEL0_BUCK_EN0_SHIFT);

	ret = i2c_smbus_write_byte_data(client, REG_VSEL0, (BYTE)ret);
	if (ret < 0){
		pr_err("%s: write error = %d , try again", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_VSEL0, (BYTE)ret);
		if (ret < 0)
			pr_err("%s: write 2nd error = %d", __func__, ret);
	}
	return ret;
}
#endif

/**
 * fan53555_set_vsel0_vout: set dcdc vout with i2c register value.
 */
static int fan53555_set_vsel0_vout(struct i2c_client *client, int vout)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, REG_VSEL0);
	if(ret < 0){
		pr_err("%s: read error = %d , try again", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_VSEL0);
		if (ret < 0)
			pr_err("%s: read 2nd error = %d", __func__, ret);
	}

	ret &= (~VSEL0_NSEL0);
	ret |= vout;

	ret = i2c_smbus_write_byte_data(client, REG_VSEL0, (BYTE)ret);
	if (ret < 0){
		pr_err("%s: write error = %d , try again", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_VSEL0, (BYTE)ret);
		if (ret < 0)
			pr_err("%s: write 2nd error = %d", __func__, ret);
	}
	return ret;
}

/**
 * fan53555_init: init dcdc_power structure.
 */
static void fan53555_init(struct dcdc_power *dcdc, struct i2c_client *client)
{
	static char *dcdc_name = "FAN53555";

	dcdc->client = client;
	dcdc->type = DCDC_VENDOR_FAN53555;
	dcdc->name = dcdc_name;
	dcdc->get_default_vout_val = fan53555_get_default_vout_val;
	dcdc->get_vout_val = fan53555_get_vout_val;
	dcdc->get_vout_str = fan53555_get_vout_str;
	dcdc->set_vout = fan53555_set_vsel0_vout;
}

#ifdef CONFIG_OF
static int fimc_is_fan53555_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct fimc_is_core *core;
	int ret = 0;
#ifdef CONFIG_SOC_EXYNOS5422
	struct regulator *regulator = NULL;
	const char power_name[] = "CAM_IO_1.8V_AP";
#endif
	struct device_node *np;
	int gpio_comp_en;

	BUG_ON(!fimc_is_dev);

	pr_info("%s start\n",__func__);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	np = of_find_compatible_node(NULL, NULL, "samsung,fimc_is_fan53555");
	if(np == NULL) {
		pr_err("compatible: fail to read, fan_parse_dt\n");
		return -ENODEV;
	}

	gpio_comp_en = of_get_named_gpio(np, "comp_en", 0);
	if (!gpio_is_valid(gpio_comp_en))
		pr_err("failed to get comp en gpio\n");

	ret = gpio_request(gpio_comp_en,"COMP_EN");
	if (ret)
		pr_err("gpio_request_error(%d)\n",ret);

	gpio_direction_output(gpio_comp_en,1);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		pr_err("%s: SMBUS Byte Data not Supported\n", __func__);
		ret = -EIO;
		goto err;
	}

	/* Init companion dcdc */
	fan53555_init(&core->companion_dcdc, client);

#ifdef CONFIG_SOC_EXYNOS5422
	regulator = regulator_get(NULL, power_name);
	if (IS_ERR(regulator)) {
		pr_err("%s : regulator_get(%s) fail\n", __func__, power_name);
		return PTR_ERR(regulator);
	}

	if (regulator_is_enabled(regulator)) {
		pr_info("%s regulator is already enabled\n", power_name);
	} else {
		ret = regulator_enable(regulator);
		if (unlikely(ret)) {
			pr_err("%s : regulator_enable(%s) fail\n", __func__, power_name);
			goto err;
		}
	}
	usleep_range(1000, 1000);
#endif

	ret = i2c_smbus_write_byte_data(client, REG_VSEL0, VSEL0_INIT_VAL);
	if (ret < 0) {
		pr_err("%s: write error = %d , try again\n", __func__, ret);
		ret = i2c_smbus_write_byte_data(client, REG_VSEL0, VSEL0_INIT_VAL);
		if (ret < 0)
			pr_err("%s: write 2nd error = %d\n", __func__, ret);
	}

	ret = i2c_smbus_read_byte_data(client, REG_VSEL0);
	if (ret < 0) {
		pr_err("%s: read error = %d , try again\n", __func__, ret);
		ret = i2c_smbus_read_byte_data(client, REG_VSEL0);
		if (ret < 0)
			pr_err("%s: read 2nd error = %d\n", __func__, ret);
	}
	pr_err("[%s::%d]fan53555 [Read :: %x ,%x]\n\n", __func__, __LINE__, ret,VSEL0_INIT_VAL);

#ifdef CONFIG_SOC_EXYNOS5422
	ret = regulator_disable(regulator);
	if (unlikely(ret)) {
		pr_err("%s: regulator_disable(%s) fail\n", __func__, power_name);
		goto err;
	}
	regulator_put(regulator);
#endif
	gpio_direction_output(gpio_comp_en,0);
	gpio_free(gpio_comp_en);

	pr_info(" %s end\n",__func__);

	return 0;

err:
	gpio_direction_output(gpio_comp_en, 0);
	gpio_free(gpio_comp_en);

#ifdef CONFIG_SOC_EXYNOS5422
	if (!IS_ERR_OR_NULL(regulator)) {
		ret = regulator_disable(regulator);
		if (unlikely(ret)) {
			pr_err("%s: regulator_disable(%s) fail\n", __func__, power_name);
		}
		regulator_put(regulator);
	}
#endif
        return ret;
}

static int fimc_is_fan53555_remove(struct i2c_client *client)
{
	return 0;
}

static struct of_device_id fan53555_dt_ids[] = {
        { .compatible = "samsung,fimc_is_fan53555",},
        {},
};
MODULE_DEVICE_TABLE(of, fan53555_dt_ids);

static const struct i2c_device_id fan53555_id[] = {
        {"fimc_is_fan53555", 0},
        {}
};
MODULE_DEVICE_TABLE(i2c, fan53555_id);

static struct i2c_driver fan53555_driver = {
        .driver = {
                .name = "fimc_is_fan53555",
                .owner  = THIS_MODULE,
                .of_match_table = fan53555_dt_ids,
        },
        .probe = fimc_is_fan53555_probe,
        .remove = fimc_is_fan53555_remove,
        .id_table = fan53555_id,
};
module_i2c_driver(fan53555_driver);

MODULE_DESCRIPTION("Exynos FIMC-IS FAN driver");
MODULE_LICENSE("GPL");
#endif