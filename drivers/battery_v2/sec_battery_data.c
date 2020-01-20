/*
 *  sec_battery_data.c
 *  Samsung Mobile Battery Driver
 *
 *  Copyright (C) 2012 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/power_supply.h>

enum battery_data_type {
	BATTERY_DATA_TYPE_NONE = 0,
	BATTERY_DATA_TYPE_INFO,
	BATTERY_DATA_TYPE_VALUE,
	BATTERY_DATA_TYPE_STRING,
};

enum battery_data_flag {
	BATTERY_DATA_FLAG_NONE = 0,
	BATTERY_DATA_FLAG_ADD,
	BATTERY_DATA_FLAG_REMOVE,
	BATTERY_DATA_FLAG_EDIT,
};

#define CHECK_ERROR_DATA(logical_test, return_value, error_value, action) { \
	if (logical_test) { \
		return_value = error_value; \
		pr_err("%s: error!!! (ret = %d)\n", __func__, error_value); \
		action; \
	} else { \
		return_value = 0; \
	} \
}

#define NODE_NAME_SIZE		32
#define PROPERTY_SIZE		68
#define MAX_VALUE_SIZE		(5 * 1024)

struct battery_data_info {
	int version;
	int hw_rev;
	int prop_count;
	int value_length;
};

struct battery_data {
	unsigned int type;
	unsigned int flag;
	char node_name[NODE_NAME_SIZE];
	char property[PROPERTY_SIZE];
	unsigned int length;
};

struct battery_property {
	struct property *property;
	unsigned int type;
	unsigned int flag;
	char *new_value;
	char *old_value;
	int new_length;
	int old_length;
	struct battery_property *next;
};

struct battery_node {
	char name[NODE_NAME_SIZE];
	struct device_node *np;
	struct battery_node *next;
	struct battery_property *start_prop;
};

static struct battery_node *get_battery_node(
					struct battery_node **start_node, char *name)
{
	struct battery_node *batt_node = NULL;
	struct battery_node *temp_node = NULL;

	if (name == NULL) {
		pr_err("%s: name is invalid!!\n", __func__);
		return NULL;
	}

	batt_node = *start_node;
	while (batt_node) {
		if (!strcmp(batt_node->name, name)) {
			return batt_node;
		} else {
			temp_node = batt_node;
			batt_node = batt_node->next;
		}
	}

	batt_node = kzalloc(sizeof(struct battery_node), GFP_KERNEL);
	if (!batt_node) {
		pr_err("%s: nomem!!\n", __func__);
		return NULL;
	}
	batt_node->np = of_find_node_by_name(NULL, name);
	if (IS_ERR_OR_NULL(batt_node->np)) {
		pr_err("%s: failed to find node(name=%s)\n", __func__, name);
		kfree(batt_node);
		return NULL;
	}
	strcpy(batt_node->name, name);
	if (temp_node)
		temp_node->next = batt_node;
	batt_node->start_prop = NULL;
	if (*start_node == NULL) *start_node = batt_node;
	pr_info("%s: add battery_node(name = %s)\n", __func__, name);
	return batt_node;
}

static int add_battery_property(	struct battery_node *batt_node,
									struct battery_property *batt_prop)
{
	struct battery_property *start_prop = NULL;
	struct battery_property *temp_prop = NULL;

	if (!batt_node || !batt_prop) {
		return -ENODATA;
	}

	start_prop = batt_node->start_prop;
	while (start_prop) {
		if (!strcmp(start_prop->property->name, batt_prop->property->name)) {
			return 0;
		} else {
			temp_prop = start_prop;
			start_prop = start_prop->next;
		}
	}

	if (!temp_prop)
		batt_node->start_prop = batt_prop;
	else {
		temp_prop->next = batt_prop;
	}
	return 0;
}

static void change_battery_pdata(
				struct battery_node *start_node, bool is_valid)
{
	struct battery_node *temp_node = NULL;

	temp_node = start_node;
	pr_info("%s: start update(%d)\n", __func__, is_valid);
	while (is_valid && temp_node) {
		struct battery_property *batt_prop = NULL;
		struct power_supply *psy = NULL;
		union power_supply_propval value;
		
		batt_prop = temp_node->start_prop;
		while (batt_prop) {
			if (batt_prop->flag != BATTERY_DATA_FLAG_REMOVE) {
				batt_prop->property->value = batt_prop->new_value;
				batt_prop->property->length = batt_prop->new_length;
			}
			batt_prop = batt_prop->next;
		}

		psy = power_supply_get_by_name(start_node->name);
		if (psy) {
			value.intval = 0;
			psy->desc->set_property(psy, POWER_SUPPLY_PROP_POWER_DESIGN, &value);
			power_supply_put(psy);
		}

		temp_node = temp_node->next;
	}

	pr_info("%s: release battery pdata\n", __func__);
	while (start_node) {
		struct battery_property *temp_batt_prop = NULL;
		struct battery_property *batt_prop = NULL;

		batt_prop = start_node->start_prop;
		while (batt_prop) {
			switch (batt_prop->flag) {
			case BATTERY_DATA_FLAG_REMOVE:
				pr_debug("%s: re-set property(ret=%d, flag=%d, name=%s)\n",
					__func__, of_add_property(start_node->np, batt_prop->property),
					batt_prop->flag, batt_prop->property->name);
				break;
			case BATTERY_DATA_FLAG_EDIT:
				pr_debug("%s: re-set property(type=%d, flag=%d, name=%s)\n",
					__func__, batt_prop->type,
					batt_prop->flag, batt_prop->property->name);
				if (!is_valid) {
					kfree(batt_prop->new_value);
				} else {
					/* In normal case, String type should not free. */
					if (batt_prop->type != BATTERY_DATA_TYPE_STRING) {
						batt_prop->property->value = batt_prop->old_value;
						batt_prop->property->length = batt_prop->old_length;
						kfree(batt_prop->new_value);
					}
				}
				break;
			case BATTERY_DATA_FLAG_ADD:
				pr_debug("%s: re-set property(ret=%d, flag=%d, name=%s)\n",
					__func__, of_remove_property(start_node->np, batt_prop->property),
					batt_prop->flag, batt_prop->property->name);
				if (batt_prop->new_value &&	(!is_valid || batt_prop->type != BATTERY_DATA_TYPE_STRING)) {
					kfree(batt_prop->new_value);
				}
				kfree(batt_prop->property->name);
				kfree(batt_prop->property);
				break;
			}

			temp_batt_prop = batt_prop;
			batt_prop = batt_prop->next;
			kfree(temp_batt_prop);
		}

		temp_node = start_node;
		start_node = start_node->next;
		kfree(temp_node);
	}
}

static int sec_battery_check_info(struct file *fp,
						struct battery_data_info *batt_info)
{
	struct device_node *np;
	struct battery_data batt_data;
	int dt_version, hw_rev, hw_rev_end;
	int ret = 0, read_size;

	read_size = fp->f_op->read(fp, (char*)&batt_data, sizeof(struct battery_data), &fp->f_pos);
	CHECK_ERROR_DATA((read_size <= 0), ret, (-ENODATA), goto finish_check_info);
	CHECK_ERROR_DATA((batt_data.type != BATTERY_DATA_TYPE_INFO), ret, (-EINVAL), goto finish_check_info);

	read_size = fp->f_op->read(fp, (char*)batt_info, sizeof(struct battery_data_info), &fp->f_pos);
	CHECK_ERROR_DATA((read_size <= 0 || read_size != batt_data.length), ret, (-ENODATA), goto finish_check_info);
	CHECK_ERROR_DATA(
		(batt_info->version < 0 || batt_info->hw_rev < 0 || batt_info->prop_count <= 0) ||
		(batt_info->value_length < 0 || batt_info->value_length > MAX_VALUE_SIZE),
		ret, (-EINVAL), goto finish_check_info);

	np = of_find_node_by_name(NULL, batt_data.node_name);
	ret = of_property_read_u32(np, "battery,batt_data_version", &dt_version);
	if (ret) {
		pr_info("%s : batt_data_version is Empty\n", __func__);
		dt_version = 0;
	}

	np = of_find_all_nodes(NULL);
	ret = of_property_read_u32(np, "model_info-hw_rev", &hw_rev);
	if (ret) {
		pr_info("%s: model_info-hw_rev is Empty\n", __func__);
		hw_rev = 0;
	}
	ret = of_property_read_u32(np, "model_info-hw_rev_end", &hw_rev_end);
	if (ret) {
		pr_info("%s: model_info-hw_rev_end is Empty\n", __func__);
		hw_rev_end = 99;
	}

	ret = (batt_info->version < dt_version) ? -1 :
		((batt_info->hw_rev > hw_rev_end || batt_info->hw_rev < hw_rev) ? -2 : 0);

	pr_info("%s: check info(ret=%d), version(%d <-> %d), hw_rev(%d ~ %d <-> %d), prop_count(%d), value_length(%d)\n",
		__func__, ret,
		dt_version, batt_info->version,
		hw_rev, hw_rev_end, batt_info->hw_rev,
		batt_info->prop_count, batt_info->value_length);

finish_check_info:
	return ret;
}

static int sec_battery_check_none(struct file *fp)
{
	struct battery_data batt_data;
	int ret = 0, read_size;

	read_size = fp->f_op->read(fp, (char*)&batt_data, sizeof(struct battery_data), &fp->f_pos);
	CHECK_ERROR_DATA((read_size <= 0), ret, (-ENODATA), goto finish_check_none);

	if (batt_data.type != BATTERY_DATA_TYPE_NONE) {
		pr_info("%s: invalid type(%d)\n", __func__, batt_data.type);
		ret = -EINVAL;
	}

finish_check_none:
	return ret;
}

static char *sec_battery_check_value(struct file *fp,
			struct battery_data_info *batt_info, struct battery_data *batt_data)
{
	char *temp_buf = NULL;
	int read_size;

	if (batt_data->length < 0 || batt_data->length > batt_info->value_length) {
		pr_info("%s: length(%d) of data is invalid\n", __func__, batt_data->length);
		return NULL;
	} else if (batt_data->length == 0) {
		pr_info("%s: skip alloc buffer(length=%d)\n", __func__, batt_data->length);
		return NULL;
	}

	temp_buf = kzalloc(batt_data->length, GFP_KERNEL);
	if (temp_buf) {
		read_size = fp->f_op->read(fp, temp_buf, batt_data->length, &fp->f_pos);
		if (read_size <= 0) {
			pr_info("%s: failed to read value\n", __func__);
			kfree(temp_buf);
			temp_buf = NULL;
		}
	}

	return temp_buf;
}

int sec_battery_update_data(const char* file_path)
{
	struct battery_node *batt_node = NULL;
	struct battery_node *temp_node = NULL;
	struct property *batt_property = NULL;
	struct battery_data_info batt_info;
	struct battery_property *batt_prop;
	struct battery_data batt_data;
	struct file* fp = NULL;
	char *temp_buf = NULL;
	int length, read_size;
	int ret = 0;

	fp = filp_open(file_path, O_RDONLY, 0);
	CHECK_ERROR_DATA(IS_ERR(fp), ret, (int)(PTR_ERR(fp)), goto err_filp_open);

	ret = sec_battery_check_info(fp, &batt_info);
	CHECK_ERROR_DATA((ret), ret, (-EINVAL), goto skip_check_data);

	while (batt_info.prop_count-- > 0) {
		read_size = fp->f_op->read(fp, (char*)&batt_data, sizeof(struct battery_data), &fp->f_pos);
		CHECK_ERROR_DATA((read_size <= 0), ret, (-ENODATA), goto finish_update_data);
		pr_debug("%s: read batt_data(type=%d, flag=%d, node_name=%s, property=%s, length=%d)\n",
			__func__, batt_data.type, batt_data.flag, batt_data.node_name, batt_data.property, batt_data.length);

		temp_node = get_battery_node(&batt_node, batt_data.node_name);
		CHECK_ERROR_DATA((!temp_node), ret, (-ENODEV), goto finish_update_data);

		batt_prop = kzalloc(sizeof(struct battery_property), GFP_KERNEL);
		CHECK_ERROR_DATA((!batt_prop), ret, (-ENOMEM), goto finish_update_data);

		batt_property = of_find_property(temp_node->np, batt_data.property, &length);
		switch (batt_data.flag) {
		case BATTERY_DATA_FLAG_ADD:
			if (IS_ERR_OR_NULL(batt_property)) {
				temp_buf = sec_battery_check_value(fp, &batt_info, &batt_data);
				CHECK_ERROR_DATA((!temp_buf && batt_data.length != 0), ret, (-ENOMEM),
					{kfree(batt_prop); goto finish_update_data;});

				batt_property = kzalloc(sizeof(struct property), GFP_KERNEL);
				CHECK_ERROR_DATA((!batt_property), ret, (-ENOMEM),
					{kfree(batt_prop); kfree(temp_buf); goto finish_update_data;});

				batt_property->name = kzalloc(PROPERTY_SIZE, GFP_KERNEL);
				CHECK_ERROR_DATA((!batt_property->name), ret, (-ENOMEM),
					{kfree(batt_prop); kfree(temp_buf); kfree(batt_property); goto finish_update_data;});

				memcpy(batt_property->name, batt_data.property, PROPERTY_SIZE);
				ret = of_add_property(temp_node->np, batt_property);
				CHECK_ERROR_DATA((ret), ret, ret,
					{kfree(batt_prop); kfree(temp_buf); kfree(batt_property->name); kfree(batt_property); goto finish_update_data;});
			} else {
				pr_info("%s: invalid data(name=%s, property=%s, flag=%d)\n",
					__func__, batt_data.node_name, batt_data.property, batt_data.flag);
				ret = -EINVAL;
				kfree(batt_prop);
				goto finish_update_data;
			}
			break;
		case BATTERY_DATA_FLAG_EDIT:
			if (!IS_ERR_OR_NULL(batt_property)) {
				temp_buf = sec_battery_check_value(fp, &batt_info, &batt_data);
				CHECK_ERROR_DATA((!temp_buf), ret, (-ENOMEM),
					{kfree(batt_prop); goto finish_update_data;});
			} else {
				pr_info("%s: invalid data(name=%s, property=%s, flag=%d)\n",
					__func__, batt_data.node_name, batt_data.property, batt_data.flag);
				ret = -EINVAL;
				kfree(batt_prop);
				goto finish_update_data;
			}
			break;
		case BATTERY_DATA_FLAG_REMOVE:
			if (!IS_ERR_OR_NULL(batt_property)) {
				temp_buf = NULL;

				ret = of_remove_property(temp_node->np, batt_property);
				CHECK_ERROR_DATA((ret), ret, ret, {kfree(batt_prop); goto finish_update_data;});
			} else {
				pr_info("%s: invalid data(name=%s, property=%s, flag=%d)\n",
					__func__, batt_data.node_name, batt_data.property, batt_data.flag);
				ret = -EINVAL;
				kfree(batt_prop);
				goto finish_update_data;
			}
			break;
		default:
			pr_info("%s: invalid flag(%d)\n", __func__, batt_data.flag);
			ret = -EINVAL;
			kfree(batt_prop);
			goto finish_update_data;
		}

		batt_prop->property = batt_property;
		batt_prop->type = batt_data.type;
		batt_prop->flag = batt_data.flag;
		batt_prop->new_value = temp_buf;
		batt_prop->old_value = batt_property->value;
		batt_prop->new_length = batt_data.length;
		batt_prop->old_length = batt_property->length;
		add_battery_property(temp_node, batt_prop);
	}
	ret = sec_battery_check_none(fp);

finish_update_data:
	change_battery_pdata(batt_node, (ret == 0));
skip_check_data:
	filp_close(fp, NULL);
err_filp_open:
	return ret;
}
