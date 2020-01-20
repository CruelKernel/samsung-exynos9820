/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of_platform.h>
#include <linux/syscore_ops.h>
#include <linux/slab.h>

#include <soc/samsung/exynos-bcm_dbg.h>
#include <soc/samsung/exynos-bcm_dbg-dt.h>

static const char *list[BCM_PD_INFO_MAX];

static void print_bcm_dbg_data(struct exynos_bcm_dbg_data *data)
{
	int i, j;

	BCM_DBG("IPC node name: %s\n", data->ipc_node->name);
	BCM_DBG("\n");

	BCM_DBG("pd_size: %u, pd_sync_init: %d\n", data->pd_size, data->pd_sync_init);
	for (i = 0; i < data->pd_size; i++)
		BCM_DBG("pd_name: %s, pd_index: %u, pd_on: %d, cal_pdid: 0x%08x\n",
			data->pd_info[i]->pd_name, data->pd_info[i]->pd_index,
			data->pd_info[i]->on, data->pd_info[i]->cal_pdid);
	BCM_DBG("\n");

	for (i = 0; i < data->define_event_max; i++) {
		BCM_DBG("Pre-defined Event index: %u\n", data->define_event[i].index);
		for (j = 0; j < BCM_EVT_EVENT_MAX; j++)
			BCM_DBG(" Event[%d]: 0x%02x\n", j, data->define_event[i].event[j]);
	}
	BCM_DBG("\n");

	BCM_DBG("Default Pre-defined Event index: %u\n", data->default_define_event);
	BCM_DBG("Pre-defined Event Max NR: %u\n", data->define_event_max);
	BCM_DBG("\n");

	for (i = 0; i < data->define_event_max; i++) {
		BCM_DBG("Pre-defined Event index: %u\n", data->define_event[i].index);
		BCM_DBG(" Filter ID mask: 0x%08x\n", data->define_filter_id[i].sm_id_mask);
		BCM_DBG(" Filter ID value: 0x%08x\n", data->define_filter_id[i].sm_id_value);
		BCM_DBG(" Filter ID active\n");
		for (j = 0; j < BCM_EVT_EVENT_MAX; j++)
			BCM_DBG("  Event[%d]: %u\n", j, data->define_filter_id[i].sm_id_active[j]);
	}
	BCM_DBG("\n");

	for (i = 0; i < data->define_event_max; i++) {
		BCM_DBG("Pre-defined Event index: %u\n", data->define_event[i].index);
		for (j = 0; j < BCM_EVT_FLT_OTHR_MAX; j++) {
			BCM_DBG(" Filter others type[%d]: 0x%02x\n",
					j, data->define_filter_others[i].sm_other_type[j]);
			BCM_DBG(" Filter others mask[%d]: 0x%02x\n",
					j, data->define_filter_others[i].sm_other_mask[j]);
			BCM_DBG(" Filter others value[%d]: 0x%02x\n",
					j, data->define_filter_others[i].sm_other_value[j]);
		}
		BCM_DBG(" Filter others active\n");
		for (j = 0; j < BCM_EVT_EVENT_MAX; j++)
			BCM_DBG("  Event[%d]: %u\n",
					j, data->define_filter_others[i].sm_other_active[j]);
	}
	BCM_DBG("\n");

	for (i = 0; i < data->define_event_max; i++) {
		BCM_DBG("Pre-defined Event index: %u\n", data->define_event[i].index);
		BCM_DBG(" Sample ID peak mask: 0x%08x\n", data->define_sample_id[i].peak_mask);
		BCM_DBG(" Sample ID peak id: 0x%08x\n", data->define_sample_id[i].peak_id);
		BCM_DBG(" Sample ID active\n");
		for (j = 0; j < BCM_EVT_EVENT_MAX; j++)
			BCM_DBG("  Event[%d]: %u\n", j, data->define_sample_id[i].peak_enable[j]);
	}
	BCM_DBG("\n");

	BCM_DBG("Available stop owner:\n");
	for (i = 0; i < STOP_OWNER_MAX; i++)
		BCM_DBG(" stop owner[%d]: %s\n", i,
				data->available_stop_owner[i] ? "true" : "false");
	BCM_DBG("\n");

	BCM_DBG("Initial BCM run: %u\n", data->initial_bcm_run);
	BCM_DBG("Initial monitor period: %u\n", data->initial_period);
	BCM_DBG("Initial BCM mode: %u\n", data->initial_bcm_mode);
	BCM_DBG("BCM Buffer size: 0x%x\n", data->dump_addr.buff_size);

	BCM_DBG("Initial Run IPs\n");
	for (i = 0; i < data->bcm_ip_nr; i++)
		BCM_DBG("  BCM IP[%d]: %u\n", i, data->initial_run_ip[i]);
	BCM_DBG("\n");
}

#ifdef CONFIG_OF
static int exynos_bcm_ipc_node_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	struct device_node *child_np;

	for_each_child_of_node(np, child_np) {
		const char *node_name;

		node_name = child_np->name;
		BCM_DBG("%s: child node name: %s\n", __func__, node_name);
		if (!strcmp(node_name, "ipc_bcm_event")) {
			data->ipc_node = child_np;
		} else {
			BCM_ERR("%s: No device node name: %s\n", __func__, node_name);
			return -ENODEV;
		}
	}

	return 0;
}

static int exynos_bcm_pd_info_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	unsigned int pd_index = 0;
	int size;

	size = of_property_count_strings(np, "pd-name");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of pd-name\n", __func__);
		return size;
	}
	data->pd_size = size;

	size = of_property_read_string_array(np, "pd-name", list, size);
	if (size < 0) {
		BCM_ERR("%s: Failed get pd-name\n", __func__);
		return size;
	}

	for (pd_index = 0; pd_index < size; pd_index++) {
		data->pd_info[pd_index] =
			kzalloc(sizeof(struct exynos_bcm_pd_info), GFP_KERNEL);
		data->pd_info[pd_index]->pd_name = (char *)list[pd_index];
		data->pd_info[pd_index]->pd_index = pd_index;
		data->pd_info[pd_index]->cal_pdid = pd_index;
		BCM_DBG("%s: get pd-name: %s(%u)\n", __func__,
				data->pd_info[pd_index]->pd_name,
				data->pd_info[pd_index]->pd_index);
	}

	return 0;
}

static int exynos_bcm_init_run_ip_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int size, ret, i;
	unsigned int run_ip;

	ret = of_property_read_u32(np, "bcm_ip_nr", &data->bcm_ip_nr);
	if (ret) {
		BCM_ERR("%s: Failed get bcm_ip_nr\n", __func__);
		return ret;
	}

	data->initial_run_ip =
		kzalloc(sizeof(unsigned int) * data->bcm_ip_nr, GFP_KERNEL);
	if (data->initial_run_ip == NULL) {
		BCM_ERR("%s: failed to allocate BCM IPs\n", __func__);
		return -ENOMEM;
	}

	size = of_property_count_u32_elems(np, "initial_run_bcm_ip");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of initial run BCM IPs\n",
				__func__);
		kfree(data->initial_run_ip);
		return size;
	}

	if (size > data->bcm_ip_nr) {
		BCM_ERR("%s: Invalid BCM IPs size, size(%d):ip_nr(%u)\n",
				__func__, size, data->bcm_ip_nr);
		kfree(data->initial_run_ip);
		return -EINVAL;
	}

	for (i = 0; i < size; i++) {
		ret = of_property_read_u32_index(np,
				"initial_run_bcm_ip", i, &run_ip);
		if (ret) {
			BCM_ERR("%s: Failed get initial run BCM IP(%d)\n",
					__func__, i);
			kfree(data->initial_run_ip);
			return ret;
		}

		data->initial_run_ip[run_ip] = BCM_IP_EN;
	}

	return 0;
}

static int exynos_bcm_define_event_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int size, ret;
	int event_cnt, nr_event;
	const unsigned int *event_addr;
	unsigned int i;
	unsigned int event_get_len, event_len;
	unsigned int *event_data;

	ret = of_property_read_u32(np, "max_define_event", &data->define_event_max);
	if (ret) {
		BCM_ERR("%s: Failed get max define event\n", __func__);
		return ret;
	}

	size = of_property_count_u32_elems(np, "define_events");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined events\n",
				__func__);
		return size;
	}

	event_len = BCM_EVT_EVENT_MAX + 1;

	/*
	 * Element number of define_event is (BCM_EVT_EVENT_MAX + 1).
	 * calculation number of array
	 */
	if (size % event_len) {
		BCM_ERR("%s: Invalid define event size, size(%d):event_len(%d)\n",
				__func__, size, event_len);
		return -EINVAL;
	}

	nr_event = size / event_len;

	if (nr_event != data->define_event_max) {
		BCM_ERR("%s: Invalid define event nr, nr_event(%d):nr_max(%u)\n",
				__func__, nr_event, data->define_event_max);
		return -EINVAL;
	}

	event_addr = of_get_property(np, "define_events", &event_get_len);
	if (event_addr == NULL) {
		BCM_ERR("%s: Failed get defined events length\n", __func__);
		return -ENODEV;
	}

	for (i = 0; i < nr_event; i++) {
		event_data = (unsigned int *)&event_addr[i * event_len];

		if ((size - (i * event_len)) <= 0) {
			BCM_ERR("%s: Invalid defined event range\n", __func__);
			return -EINVAL;
		}

		data->define_event[i].index = be32_to_cpu(event_data[0]);
		for (event_cnt = 0; event_cnt < BCM_EVT_EVENT_MAX; event_cnt++)
			data->define_event[i].event[event_cnt] =
					be32_to_cpu(event_data[event_cnt + 1]);
	}

	ret = of_property_read_u32(np, "default_define_event",
					&data->default_define_event);
	if (ret) {
		BCM_ERR("%s: Failed get default define event\n", __func__);
		data->default_define_event = LATENCY_FMT_EVT;
		BCM_INFO("%s: replaced default define event: %u\n",
				__func__, data->default_define_event);
	} else {
		if (data->default_define_event >= data->define_event_max) {
			BCM_ERR("%s: Invalid default define event(%u), max(%u)\n",
					__func__, data->default_define_event,
					data->define_event_max);
			data->default_define_event = LATENCY_FMT_EVT;
			BCM_INFO("%s: replaced default define event: %u\n",
					__func__, data->default_define_event);
		}
	}

	return 0;
}

static int exynos_bcm_filter_id_info_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int size;
	int active_cnt;
	const unsigned int *filter_id_addr;
	const unsigned int *filter_id_active_addr;
	unsigned int i;
	unsigned int filter_id_len, active_len;
	unsigned int filter_id_get_len, active_get_len;
	unsigned int *id_data;
	unsigned int *active_data;

	/* sm_id_mask and sm_id_value */
	size = of_property_count_u32_elems(np, "define_filter_id");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined filter_id\n",
				__func__);
		return size;
	}

	filter_id_addr = of_get_property(np, "define_filter_id", &filter_id_get_len);
	if (filter_id_addr == NULL) {
		BCM_ERR("%s: Failed get define filter id length\n", __func__);
		return -ENODEV;
	}

	filter_id_len = 3;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		id_data = (unsigned int *)&filter_id_addr[i * filter_id_len];

		if ((size - (i * filter_id_len)) <= 0) {
			BCM_ERR("%s: Invalid defined filter id range\n", __func__);
			return -EINVAL;
		}

		data->define_filter_id[i].sm_id_mask = be32_to_cpu(id_data[1]);
		data->define_filter_id[i].sm_id_value = be32_to_cpu(id_data[2]);
	}

	/* sm_id_active */
	size = of_property_count_u32_elems(np, "define_filter_id_active");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined filter_id_active\n",
				__func__);
		return size;
	}

	filter_id_active_addr = of_get_property(np, "define_filter_id_active",
							&active_get_len);
	if (filter_id_active_addr == NULL) {
		BCM_ERR("%s: Failed get define filter id active length\n", __func__);
		return -ENODEV;
	}

	active_len = BCM_EVT_EVENT_MAX + 1;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		active_data = (unsigned int *)&filter_id_active_addr[i * active_len];

		if ((size - (i * active_len)) <= 0) {
			BCM_ERR("%s: Invalid defined filter id active range\n", __func__);
			return -EINVAL;
		}

		for (active_cnt = 0; active_cnt < BCM_EVT_EVENT_MAX; active_cnt++)
			data->define_filter_id[i].sm_id_active[active_cnt] =
					be32_to_cpu(active_data[active_cnt + 1]);
	}

	return 0;
}

static int exynos_bcm_filter_others_info_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int size;
	int active_cnt;
	const unsigned int *filter_other_0_addr;
	const unsigned int *filter_other_1_addr;
	const unsigned int *filter_other_active_addr;
	unsigned int i;
	unsigned int filter_other_0_len, filter_other_1_len, active_len;
	unsigned int filter_other_0_get_len, filter_other_1_get_len, active_get_len;
	unsigned int *other0_data;
	unsigned int *other1_data;
	unsigned int *active_data;

	/* sm_other_type, sm_other_mask and sm_other_value */
	size = of_property_count_u32_elems(np, "define_filter_other_0");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined filter_other_0\n",
				__func__);
		return size;
	}

	filter_other_0_addr = of_get_property(np, "define_filter_other_0",
							&filter_other_0_get_len);
	if (filter_other_0_addr == NULL) {
		BCM_ERR("%s: Failed get define filter_other_0 length\n", __func__);
		return -ENODEV;
	}

	filter_other_0_len = 4;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		other0_data = (unsigned int *)&filter_other_0_addr[i * filter_other_0_len];

		if ((size - (i * filter_other_0_len)) <= 0) {
			BCM_ERR("%s: Invalid defined filter_other_0 range\n", __func__);
			return -EINVAL;
		}

		data->define_filter_others[i].sm_other_type[0] =
						be32_to_cpu(other0_data[1]);
		data->define_filter_others[i].sm_other_mask[0] =
						be32_to_cpu(other0_data[2]);
		data->define_filter_others[i].sm_other_value[0] =
						be32_to_cpu(other0_data[3]);
	}

	size = of_property_count_u32_elems(np, "define_filter_other_1");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined filter_other_1\n",
				__func__);
		return size;
	}

	filter_other_1_addr = of_get_property(np, "define_filter_other_1",
							&filter_other_1_get_len);
	if (filter_other_1_addr == NULL) {
		BCM_ERR("%s: Failed get define filter_other_1 length\n", __func__);
		return -ENODEV;
	}

	filter_other_1_len = 4;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		other1_data = (unsigned int *)&filter_other_1_addr[i * filter_other_1_len];

		if ((size - (i * filter_other_1_len)) <= 0) {
			BCM_ERR("%s: Invalid defined filter_other_1 range\n", __func__);
			return -EINVAL;
		}

		data->define_filter_others[i].sm_other_type[1] =
						be32_to_cpu(other1_data[1]);
		data->define_filter_others[i].sm_other_mask[1] =
						be32_to_cpu(other1_data[2]);
		data->define_filter_others[i].sm_other_value[1] =
						be32_to_cpu(other1_data[3]);
	}

	/* sm_other_active */
	size = of_property_count_u32_elems(np, "define_filter_other_active");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined filter_other_active\n",
				__func__);
		return size;
	}

	filter_other_active_addr = of_get_property(np, "define_filter_other_active",
							&active_get_len);
	if (filter_other_active_addr == NULL) {
		BCM_ERR("%s: Failed get define filter_other_active length\n", __func__);
		return -ENODEV;
	}

	active_len = BCM_EVT_EVENT_MAX + 1;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		active_data = (unsigned int *)&filter_other_active_addr[i * active_len];

		if ((size - (i * active_len)) <= 0) {
			BCM_ERR("%s: Invalid defined filter_other_active range\n", __func__);
			return -EINVAL;
		}

		for (active_cnt = 0; active_cnt < BCM_EVT_EVENT_MAX; active_cnt++)
			data->define_filter_others[i].sm_other_active[active_cnt] =
					be32_to_cpu(active_data[active_cnt + 1]);
	}

	return 0;
}

static int exynos_bcm_sample_id_info_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int size;
	int active_cnt;
	const unsigned int *sample_id_addr;
	const unsigned int *sample_active_addr;
	unsigned int i;
	unsigned int sample_id_len, active_len;
	unsigned int sample_id_get_len, active_get_len;
	unsigned int *id_data;
	unsigned int *active_data;

	/* peak_mask and peak_id */
	size = of_property_count_u32_elems(np, "define_sample_id");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined sample_id\n",
				__func__);
		return size;
	}

	sample_id_addr = of_get_property(np, "define_sample_id", &sample_id_get_len);
	if (sample_id_addr == NULL) {
		BCM_ERR("%s: Failed get define_sample_id length\n", __func__);
		return -ENODEV;
	}

	sample_id_len = 3;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		id_data = (unsigned int *)&sample_id_addr[i * sample_id_len];

		if ((size - (i * sample_id_len)) <= 0) {
			BCM_ERR("%s: Invalid defined sample id range\n", __func__);
			return -EINVAL;
		}

		data->define_sample_id[i].peak_mask = be32_to_cpu(id_data[1]);
		data->define_sample_id[i].peak_id = be32_to_cpu(id_data[2]);
	}

	/* peak_enable */
	size = of_property_count_u32_elems(np, "define_sample_id_enable");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of defined sample_id_enable\n",
				__func__);
		return size;
	}

	sample_active_addr = of_get_property(np, "define_sample_id_enable",
							&active_get_len);
	if (sample_active_addr == NULL) {
		BCM_ERR("%s: Failed get define sample_id_enable length\n", __func__);
		return -ENODEV;
	}

	active_len = BCM_EVT_EVENT_MAX + 1;

	for (i = 0; i < PRE_DEFINE_EVT_MAX; i++) {
		active_data = (unsigned int *)&sample_active_addr[i * active_len];

		if ((size - (i * active_len)) <= 0) {
			BCM_ERR("%s: Invalid defined sample_id_enable range\n", __func__);
			return -EINVAL;
		}

		for (active_cnt = 0; active_cnt < BCM_EVT_EVENT_MAX; active_cnt++)
			data->define_sample_id[i].peak_enable[active_cnt] =
					be32_to_cpu(active_data[active_cnt + 1]);
	}

	return 0;
}

static int exynos_bcm_init_control_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int ret;
	int i, size;
	const unsigned int *stop_owner_addr;
	unsigned int stop_owner_get_len, owner_index;
	unsigned int *stop_owner_data;


	ret = of_property_read_u32(np, "initial_bcm_run", &data->initial_bcm_run);
	if (ret) {
		BCM_ERR("%s: Failed get initial BCM run state\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "initial_period", &data->initial_period);
	if (ret) {
		BCM_ERR("%s: Failed get initial sampling period\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "initial_bcm_mode", &data->initial_bcm_mode);
	if (ret) {
		BCM_ERR("%s: Failed get initial BCM measure mode\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "buff_size", &data->dump_addr.buff_size);
	if (ret) {
		BCM_ERR("%s: Failed get buffer size\n", __func__);
		return ret;
	}

	if (data->initial_bcm_mode >= BCM_MODE_MAX) {
		BCM_ERR("%s: Invalid initial BCM measure mode(%u), max(%u)\n",
					__func__, data->initial_bcm_mode,
					BCM_MODE_MAX);
		return -EINVAL;
	}

	size = of_property_count_u32_elems(np, "available_stop_owner");
	if (size < 0) {
		BCM_ERR("%s: Failed get number of available_stop_owner\n",
				__func__);
		return size;
	}

	if (size > STOP_OWNER_MAX) {
		BCM_ERR("%s: Invalid stop owner size (%u)\n", __func__, size);
		return -EINVAL;
	}

	stop_owner_addr = of_get_property(np, "available_stop_owner", &stop_owner_get_len);
	if (stop_owner_addr == NULL) {
		BCM_ERR("%s: Failed get define stop owner length\n", __func__);
		return -ENODEV;
	}

	stop_owner_data = (unsigned int *)stop_owner_addr;

	for (i = 0; i < size; i++) {
		owner_index = be32_to_cpu(stop_owner_data[i]);
		if (owner_index >= STOP_OWNER_MAX) {
			BCM_ERR("%s: Invalid stop owner (%d:%u)\n",
					__func__, i, owner_index);
			return -EINVAL;
		}
		data->available_stop_owner[owner_index] = true;
	}

	return 0;
}

int exynos_bcm_dbg_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	int ret = 0;

	if (!np)
		return -ENODEV;

	/* get IPC type */
	ret = exynos_bcm_ipc_node_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse IPC node\n", __func__);
		return ret;
	}

	/* get Local Power domain names and set power domain index */
	ret = exynos_bcm_pd_info_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse Power domain info\n", __func__);
		return ret;
	}

	/* Get initial run BCM IPs information */
	ret = exynos_bcm_init_run_ip_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse initial run BCM IPs info\n", __func__);
		return ret;
	}

	/* Get Pre-defined Event information */
	ret = exynos_bcm_define_event_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse Pre-defined Event info\n", __func__);
		return ret;
	}

	/* Get define Filter ID information */
	ret = exynos_bcm_filter_id_info_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse Filter ID info\n", __func__);
		return ret;
	}

	/* Get define Filter Others information */
	ret = exynos_bcm_filter_others_info_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse Filter Others info\n", __func__);
		return ret;
	}

	/* Get define Sample ID information */
	ret = exynos_bcm_sample_id_info_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse Sample ID info\n", __func__);
		return ret;
	}

	/* Get initial Control information */
	ret = exynos_bcm_init_control_parse_dt(np, data);
	if (ret) {
		BCM_ERR("%s: Failed parse Control info\n", __func__);
		return ret;
	}

	/* printing BCM data for debug */
	print_bcm_dbg_data(data);

	return ret;
}
#else
int exynos_bcm_dbg_parse_dt(struct device_node *np,
				struct exynos_bcm_dbg_data *data)
{
	return -ENODEV;
}
#endif
