#include <linux/types.h>
#include <linux/input.h>
#include <linux/errno.h>
#include <linux/err.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#include "fimc-is-ncp6335b.h"

/**
 * NCP6335B Vout Tables
 */
static const struct ncp6335b_vout vout_tables[] = {
	{0, NCP6335B_VOUT_1P000, "1.000"}, /* defualt voltage */
	{1, NCP6335B_VOUT_0P875, "0.875"},
	{2, NCP6335B_VOUT_0P900, "0.900"},
	{3, NCP6335B_VOUT_0P925, "0.925"},
	{4, NCP6335B_VOUT_0P950, "0.950"},
	{5, NCP6335B_VOUT_0P975, "0.975"},
	{6, NCP6335B_VOUT_1P000, "1.000"},
};

/**
 * ncp6335_get_default_vout_val: get i2c register value to set vout of dcdc regulator.
 */
static int ncp6335_get_default_vout_val(int *vout_val, const char **vout_str)
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
 * ncp6335b_get_vout_val: get i2c register value to set vout of dcdc regulator.
 */
static int ncp6335b_get_vout_val(int sel)
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
 * ncp6335b_get_vout_name: get voltage name of vout as string.
 */
static const char *ncp6335b_get_vout_str(int sel)
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

/**
 * ncp6335b_set_voltage: set dcdc vout with i2c register value.
 */
static int ncp6335b_set_voltage(struct i2c_client *client, int vout)
{
	int ret = i2c_smbus_write_byte_data(client, 0x14, 0x00);
	if (ret < 0)
		pr_err("[%s::%d] Write Error [%d]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_write_byte_data(client, 0x10, vout); /* 1.05V -> 1V (0xC0) */
	if (ret < 0)
		pr_err("[%s::%d] Write Error [%d]. vout 0x%X\n", __func__, __LINE__, ret, vout);

	ret = i2c_smbus_write_byte_data(client, 0x11, vout); /* 1.05V -> 1V (0xC0) */
	if (ret < 0)
		pr_err("[%s::%d] Write Error [%d]. vout 0x%X\n", __func__, __LINE__, ret, vout);

	/*pr_info("%s: vout 0x%X\n", __func__, vout);*/

	return ret;
}

static int ncp6335b_read_voltage(struct i2c_client *client)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, 0x3);
	if (ret < 0)
		pr_err("[%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[%s::%d]NCP6335B PID[%x]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_read_byte_data(client, 0x10);
	if (ret < 0)
		pr_err("[%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[%s::%d]NCP6335B [0x10 Read :: %x]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_read_byte_data(client, 0x11);
	if (ret < 0)
		pr_err("[%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[%s::%d]NCP6335B [0x11 Read :: %x]\n", __func__, __LINE__, ret);

	ret = i2c_smbus_read_byte_data(client, 0x14);
	if (ret < 0)
		pr_err("[%s::%d]Read Error [%d]\n", __func__, __LINE__, ret);
	pr_err("[%s::%d]NCP6335B [0x14 Read :: %x]\n", __func__, __LINE__, ret);

	return ret;
}

/**
 * ncp6335b_init: init dcdc_power structure.
 */
static void ncp6335b_init(struct dcdc_power *dcdc, struct i2c_client *client)
{
	static char *dcdc_name = "NCP6335B";

	dcdc->client = client;
	dcdc->type = DCDC_VENDOR_NCP6335B;
	dcdc->name = dcdc_name;
	dcdc->get_default_vout_val = ncp6335_get_default_vout_val;
	dcdc->get_vout_val = ncp6335b_get_vout_val;
	dcdc->get_vout_str = ncp6335b_get_vout_str;
	dcdc->set_vout = ncp6335b_set_voltage;
}

#ifdef CONFIG_OF
static int fimc_is_ncp6335b_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	struct fimc_is_core *core;
	int ret = 0;

	struct device_node *np;
	int gpio_comp_en;

	BUG_ON(!fimc_is_dev);

	pr_info("%s start\n",__func__);

	core = (struct fimc_is_core *)dev_get_drvdata(fimc_is_dev);
	if (!core) {
		pr_err("core device is not yet probed");
		return -EPROBE_DEFER;
	}

	np = of_find_compatible_node(NULL, NULL, "samsung,fimc_is_ncp6335b");
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
	ncp6335b_init(&core->companion_dcdc, client);

	ret = ncp6335b_set_voltage(client, 0xC0);
	if (ret < 0) {
		pr_err("%s: error, fail to set voltage\n", __func__);
		goto err;
	}

	ret = ncp6335b_read_voltage(client);
	if (ret < 0) {
		pr_err("%s: error, fail to read voltage\n", __func__);
		goto err;
	}

	pr_info("%s %s: ncp6335b probed\n",
		dev_driver_string(&client->dev), dev_name(&client->dev));

err:
	gpio_direction_output(gpio_comp_en,0);
	gpio_free(gpio_comp_en);

	return ret;
}

static int fimc_is_ncp6335b_remove(struct i2c_client *client)
{
	return 0;
}

static struct of_device_id ncp6335b_dt_ids[] = {
        { .compatible = "samsung,fimc_is_ncp6335b",},
        {},
};
MODULE_DEVICE_TABLE(of, ncp6335b_dt_ids);

static const struct i2c_device_id ncp6335b_id[] = {
        {"fimc_is_ncp6335b", 0},
        {}
};
MODULE_DEVICE_TABLE(i2c, ncp6335b_id);

static struct i2c_driver ncp6335b_driver = {
        .driver = {
                .name = "fimc_is_ncp6335b",
                .owner  = THIS_MODULE,
                .of_match_table = ncp6335b_dt_ids,
        },
        .probe = fimc_is_ncp6335b_probe,
        .remove = fimc_is_ncp6335b_remove,
        .id_table = ncp6335b_id,
};
module_i2c_driver(ncp6335b_driver);

MODULE_DESCRIPTION("Exynos FIMC-IS NCP driver");
MODULE_LICENSE("GPL");
#endif
