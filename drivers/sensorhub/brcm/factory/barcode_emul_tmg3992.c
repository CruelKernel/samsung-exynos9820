#include "../ssp.h"
#include <linux/sec_class.h>

#define	VENDOR		"AMS"
#define	CHIP_ID		"TMG399x"

u8 hop_count;
u8 is_beaming;

static struct reg_index_table reg_id_table[15] = {
	{0x81, 0}, {0x88, 1}, {0x8F, 2}, {0x96, 3}, {0x9D, 4},
	{0xA4, 5}, {0xAB, 6}, {0xB2, 7}, {0xB9, 8}, {0xC0, 9},
	{0xC7, 10}, {0xCE, 11}, {0xD5, 12}, {0xDC, 13}, {0xE3, 14}
};

#define BEAMING_ON	1
#define BEAMING_OFF	0
#define STOP_BEAMING	0

#define COUNT_TEST	48 /* 0 */
#define REGISTER_TEST	49 /* 1 */
#define DATA_TEST	50 /* 2 */

#define OFFSET_REGISTER_SET	 2
enum {
	dataset = 0,
	registerset,
	countset,
	start,
};

enum {
	reg,
	index,
};

void mobeam_write(struct ssp_data *data, int type, u8 *u_buf)
{
	int iRet = 0;
	u8 command = -1;
	u8 data_length = 0;

	struct ssp_msg *msg;

	if (!(data->uSensorState & (1 << PROXIMITY_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!!, proximity sensor is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return;
	}

	pr_info("[SSP] %s start, command_type = %d\n", __func__, type);
	switch (type) {
	case dataset:
		command = MSG2SSP_AP_MOBEAM_DATA_SET;
		data_length = 128;
		break;
	case registerset:
		command = MSG2SSP_AP_MOBEAM_REGISTER_SET;
		data_length = 6;
		break;
	case countset:
		command = MSG2SSP_AP_MOBEAM_COUNT_SET;
		data_length = 1;
		break;
	case start:
		command = MSG2SSP_AP_MOBEAM_START;
		data_length = 1;
		is_beaming = BEAMING_ON;
		break;
	default:
		pr_info("[SSP] %s - unknown cmd type\n", __func__);
		break;
	}

	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = command;
	msg->length = data_length;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(data_length, GFP_KERNEL);
	if ((msg->buffer) == NULL) {
		pr_err("[SSP]: %s - failed to allocate memory\n", __func__);
		kfree(msg);
		return;
	}
	msg->free_buffer = 1;

	memcpy(msg->buffer, u_buf, data_length);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - MOBEAM CMD fail %d\n", __func__, iRet);
		return;
	}

	pr_info("[SSP] %s command = 0x%X\n", __func__, command);
}

void mobeam_stop_set(struct ssp_data *data)
{
	int iRet, iReties = 0;
	struct ssp_msg *msg;
	u8 buffer = 0;

retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_MOBEAM_STOP;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
	} else {
		is_beaming = BEAMING_OFF;
		pr_info("[SSP] %s - success(%u)\n", __func__, is_beaming);
	}
}

static ssize_t mobeam_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t mobeam_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t barcode_emul_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 send_buf[128] = { 0, };
	int i, len;

	if(size <= 1) {
		pr_info("[SSP] %s - not enough size of input data(%d)", __func__, (int)size);
		return -EINVAL;
	}

	memset(send_buf, 0xFF, 128);
	if (buf[0] == 0xFF && buf[1] != STOP_BEAMING) {
		pr_info("[SSP] %s - START BEAMING(0x%X, 0x%X)\n", __func__,
			buf[0], buf[1]);
		send_buf[1] = buf[1];
		mobeam_write(data, start, &send_buf[1]);
	} else if (buf[0] == 0xFF && buf[1] == STOP_BEAMING) {
		pr_info("[SSP] %s - STOP BEAMING(0x%X, 0x%X)\n", __func__,
			buf[0], buf[1]);
		if (is_beaming == BEAMING_ON)
			mobeam_stop_set(data);
		else
			pr_info("[SSP] %s - skip stop command\n", __func__);
	} else if (buf[0] == 0x00) {
		pr_info("[SSP] %s - DATA SET\n", __func__);
		len = ((int)size - OFFSET_REGISTER_SET > 128) ? 128 : ((int)size-OFFSET_REGISTER_SET);
		memcpy(send_buf, &buf[OFFSET_REGISTER_SET], len);

		pr_info("[SSP] %s - %u %u %u %u %u %u\n", __func__,
			send_buf[0], send_buf[1], send_buf[2],
			send_buf[3], send_buf[4], send_buf[5]);
		mobeam_write(data, dataset, send_buf);
	} else if (buf[0] == 0x80) {
		pr_info("[SSP] %s - HOP COUNT SET(0x%X)\n", __func__, buf[1]);
		hop_count = buf[1];
		mobeam_write(data, countset, &hop_count);
	} else {
		pr_info("[SSP] %s - REGISTER SET(0x%X)\n", __func__, buf[0]);
		if(size < 8) {
			pr_info("[SSP] %s - not enough size of input data(%d)", __func__, (int)size);
			return -EINVAL;
		}
		for (i = 0; i < 15; i++) {
			if (reg_id_table[i].reg == buf[0])
				send_buf[0] = reg_id_table[i].index;
		}
		send_buf[1] = buf[1];
		send_buf[2] = buf[2];
		send_buf[3] = buf[4];
		send_buf[4] = buf[5];
		send_buf[5] = buf[7];
		mobeam_write(data, registerset, send_buf);
	}
	return size;
}

static ssize_t barcode_emul_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return strlen(buf);
}

static ssize_t barcode_led_status_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", is_beaming);
}

static ssize_t barcode_ver_check_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", 15);
}

static ssize_t barcode_emul_test_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	u8 barcode_data[14] = {0xFF, 0xAC, 0xDB, 0x36, 0x42, 0x85,
			0x0A, 0xA8, 0xD1, 0xA3, 0x46, 0xC5, 0xDA, 0xFF};
	u8 test_data[128] = { 0, };

	memset(test_data, 0xFF, 128);
	if (buf[0] == COUNT_TEST) {
		test_data[0] = 0x80;
		test_data[1] = 1;
		pr_info("[SSP] %s, COUNT_TEST - 0x%X, %u\n", __func__,
			test_data[0], test_data[1]);
		mobeam_write(data, countset, &test_data[1]);
	} else if (buf[0] == REGISTER_TEST) {
		test_data[0] = 0;
		test_data[1] = 10;
		test_data[2] = 20;
		test_data[3] = 30;
		test_data[4] = 40;
		test_data[5] = 50;
		pr_info("[SSP] %s, REGISTER_TEST - %u: %u %u %u %u %u\n",
			__func__, test_data[0], test_data[1], test_data[2],
			test_data[3], test_data[4], test_data[5]);
		mobeam_write(data, registerset, test_data);
	} else if (buf[0] == DATA_TEST) {
		memcpy(test_data, &barcode_data[1], 13);
		pr_info("[SSP] %s, DATA_TEST - 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X\n",
			__func__, test_data[0], test_data[1], test_data[2],
			test_data[3], test_data[4], test_data[5]);
		mobeam_write(data, dataset, test_data);
	}
	return size;
}
static DEVICE_ATTR(vendor, 0444, mobeam_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, mobeam_name_show, NULL);
static DEVICE_ATTR(barcode_send, 0664,
	barcode_emul_show, barcode_emul_store);
static DEVICE_ATTR(barcode_led_status, 0444,	barcode_led_status_show, NULL);
static DEVICE_ATTR(barcode_ver_check, 0444, barcode_ver_check_show, NULL);
static DEVICE_ATTR(barcode_test_send, 0220,
	NULL, barcode_emul_test_store);

void initialize_mobeam(struct ssp_data *data)
{
	pr_info("[SSP] %s\n", __func__);
	data->mobeam_device = sec_device_create(data, "sec_barcode_emul");

	if (IS_ERR(data->mobeam_device))
		pr_err("[SSP] Failed to create mobeam_dev device\n");

	if (device_create_file(data->mobeam_device, &dev_attr_vendor) < 0)
		pr_err("[SSP] Failed to create device file(%s)!\n",
				dev_attr_vendor.attr.name);

	if (device_create_file(data->mobeam_device, &dev_attr_name) < 0)
		pr_err("[SSP] Failed to create device file(%s)!\n",
				dev_attr_name.attr.name);

	if (device_create_file(data->mobeam_device, &dev_attr_barcode_send) < 0)
		pr_err("[SSP] Failed to create device file(%s)!\n",
				dev_attr_barcode_send.attr.name);

	if (device_create_file(data->mobeam_device, &dev_attr_barcode_led_status) < 0)
		pr_err("[SSP] Failed to create device file(%s)!\n",
				dev_attr_barcode_led_status.attr.name);

	if (device_create_file(data->mobeam_device, &dev_attr_barcode_ver_check) < 0)
		pr_err("[SSP] Failed to create device file(%s)!\n",
				dev_attr_barcode_ver_check.attr.name);

	if (device_create_file(data->mobeam_device, &dev_attr_barcode_test_send) < 0)
		pr_err("[SSP] Failed to create device file(%s)!\n",
				dev_attr_barcode_test_send.attr.name);
	is_beaming = BEAMING_OFF;
}

void remove_mobeam(struct ssp_data *data)
{
	pr_info("[SSP] %s\n", __func__);

	device_remove_file(data->mobeam_device, &dev_attr_barcode_test_send);
	device_remove_file(data->mobeam_device, &dev_attr_barcode_ver_check);
	device_remove_file(data->mobeam_device, &dev_attr_barcode_led_status);
	device_remove_file(data->mobeam_device, &dev_attr_barcode_send);
	device_remove_file(data->mobeam_device, &dev_attr_name);
	device_remove_file(data->mobeam_device, &dev_attr_vendor);
}
