/*
 *  Copyright (C) 2016, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include "ssp_dump.h"

int get_size_dumpregister(int sensor_type)
{
	int size = 0;

	switch (sensor_type) {
	case ACCELEROMETER_SENSOR:
	case GYROSCOPE_SENSOR:
		size = DUMPREGISTER_SIZE_ACCELEROMTER;
		break;
	case GEOMAGNETIC_UNCALIB_SENSOR:
		size = DUMPREGISTER_SIZE_GEOMAGNETIC_FIELD;
		break;
	case PRESSURE_SENSOR:
		size = DUMPREGISTER_SIZE_PRESSURE;
		break;
	case PROXIMITY_SENSOR:
	case LIGHT_SENSOR:
		size = DUMPREGISTER_SIZE_PROXIMITY;
		break;
	}
	return size;
}

int store_sensor_dump(struct ssp_data *data, int sensor_type, u16 length, u8 *buf)
{
#ifdef SENSOR_DUMP_FILE_STORE
	mm_segment_t old_fs;
	struct file *dump_filp = NULL;
	char file_name[SENSOR_DUMP_FILE_LENGTH] = {0,};
#endif

	char *contents;
	int contents_length, ret = SUCCESS;
	int i = 0;
	int len = 0;
	int string_len = sensor_dump_length(length);
	char tmp_ch;

	pr_info("[SSP] %s - type %d, length %d\n", __func__, sensor_type, length);

	/*make file contents*/
	contents = kzalloc(sensor_dump_length(length) + 1, GFP_KERNEL);

	for (i = 0, len = 0; i < length; i++) {
		tmp_ch = ((i % NUM_LINE_ITEM == NUM_LINE_ITEM - 1) || (i - 1 == length)) ? '\n' : ' ';
		len += snprintf(contents + len, string_len - len, "%02x%c", buf[i], tmp_ch);
	}
	contents[string_len] = '\0';
	contents_length = string_len + 1;

	if (data->sensor_dump[sensor_type] != NULL) {
		kfree(data->sensor_dump[sensor_type]);
		data->sensor_dump[sensor_type] = NULL;
	}

	data->sensor_dump[sensor_type] = kzalloc(contents_length, GFP_KERNEL);
	memcpy(data->sensor_dump[sensor_type], contents, contents_length);
	kfree(contents);

#ifdef SENSOR_DUMP_FILE_STORE
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	/*make file name*/
	sprintf(file_name, "%s%d.txt", SENSOR_DUMP_PATH, sensor_type);

	dump_filp = filp_open(file_name,
			O_CREAT | O_TRUNC | O_WRONLY | O_SYNC, 0640);

	if (IS_ERR(dump_filp)) {
		pr_err("[SSP] %s - Can't open dump file %d\n", __func__, sensor_type);
		set_fs(old_fs);
		ret = PTR_ERR(dump_filp);
		return ret;
	}

	ret = vfs_write(dump_filp, data->sensor_dump[sensor_type], contents_length, &dump_filp->f_pos);
	if (ret < 0) {
		pr_err("[SSP] %s - Can't write the dump data to file\n", __func__);
		ret = -EIO;
	}

	filp_close(dump_filp, current->files);
	set_fs(old_fs);
#endif

	return ret;
}


int send_sensor_dump_command(struct ssp_data *data, u8 sensor_type)
{
	int ret = SUCCESS, size;
	struct ssp_msg *msg;
	char *buf;

	if (sensor_type >= SENSOR_MAX) {
		pr_err("[SSP] %s - invalid sensor type %d\n", __func__, sensor_type);
		return -EINVAL;
	} else if (!(data->uSensorState & (1ULL << sensor_type))) {
		pr_err("[SSP] %s - %u is not connected(0x%llx)\n",
			 __func__, sensor_type, data->uSensorState);
		return -EINVAL;
	}

	size = get_size_dumpregister(sensor_type);
	if (size <= 0) {
		pr_err("[SSP] %s - unsupported sensor type %u\n", __func__, sensor_type);
		return -EINVAL;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	buf = kzalloc(size, GFP_KERNEL);

	msg->cmd = MSG2SSP_AP_REGISTER_DUMP;
	msg->length = size;
	msg->options = AP2HUB_READ;
	msg->data = sensor_type;
	msg->buffer = buf;
	msg->free_buffer = 0;

	ret = ssp_spi_sync(data, msg, 1000);

	pr_info("[SSP] %s - (%u)\n", __func__, sensor_type);

	if (ret != SUCCESS) {
		pr_err("[SSP] MSG2SSP_AP_REGISTER_DUMP CMD Fail %d", ret);
		return -EIO;
	}

	ret = store_sensor_dump(data, sensor_type, size, buf);

	kfree(buf);

	return ret;
}

int send_all_sensor_dump_command(struct ssp_data *data)
{
	int types[] = SENSOR_DUMP_SENSOR_LIST;
	int i, ret = SUCCESS;

	for (i = 0; i < ARRAY_SIZE(types); i++) {
		int temp = send_sensor_dump_command(data, types[i]);

		if (temp != SUCCESS)
			ret = temp;
	}

	return ret;
}
