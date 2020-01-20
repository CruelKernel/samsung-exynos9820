#include "../ssp.h"

#define	VENDOR		"AMS"
#define	CHIP_ID		"TMG399x"

//ssp.h have to add.
//#define MSG2SSP_AP_IRDATA_SEND		0x38

static ssize_t irled_vendor_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", VENDOR);
}

static ssize_t irled_name_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", CHIP_ID);
}

static ssize_t irled_send_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	struct ssp_msg *msg;
	unsigned int buf_len = 0;
	unsigned int iRet;

	if (!(data->uSensorState & (1 << PROXIMITY_SENSOR))) {
		pr_info("[SSP]: %s - Skip this function!!! irled_remote is not connected(0x%llx)\n",
			__func__, data->uSensorState);
		return FAIL;
	}

	buf_len = (unsigned int)(strlen(buf)+1);
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	if (msg == NULL) {
		pr_err("[SSP]: %s - failed to allocate memory\n", __func__);
		return FAIL;
	}
	msg->cmd = MSG2SSP_AP_IRDATA_SEND;
	msg->length = buf_len;
	msg->options = AP2HUB_WRITE;
	msg->buffer = kzalloc(buf_len, GFP_KERNEL);
	msg->free_buffer = 1;

	memcpy(msg->buffer, buf, buf_len);

	iRet = ssp_spi_async(data, msg);

	if (iRet != SUCCESS) {
		pr_err("[SSP]: %s - IRLED SEND fail %d\n", __func__, iRet);
		return iRet;
	}

	pr_info("[SSP] %s IRLED SEND Success %d\n", __func__, iRet);
	return size;
}

static ssize_t irled_send_show(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return strlen(buf);
}

static ssize_t irled_send_result_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct ssp_data *data = dev_get_drvdata(dev);
	int iRet, iReties = 0;
	struct ssp_msg *msg;
	u8 buffer = 0;
	bool success_fail = FAIL;

retries:
	msg = kzalloc(sizeof(*msg), GFP_KERNEL);
	msg->cmd = MSG2SSP_AP_IRDATA_SEND_RESULT;
	msg->length = 1;
	msg->options = AP2HUB_READ;
	msg->buffer = &buffer;
	msg->free_buffer = 0;

	iRet = ssp_spi_sync(data, msg, 1000);
	success_fail = buffer;
	if (iRet != SUCCESS) {
		pr_err("[SSP] %s fail %d\n", __func__, iRet);

		if (iReties++ < 2) {
			pr_err("[SSP] %s fail, retry\n", __func__);
			mdelay(5);
			goto retries;
		}
		return FAIL;
	}
	if (success_fail == SUCCESS) {
		pr_info("[SSP] %s - SUCCESS(%u)\n", __func__, success_fail);
		return snprintf(buf, PAGE_SIZE, "%d\n", success_fail);
	}

	pr_info("[SSP] %s - FAIL(%u)\n", __func__, success_fail);
	return snprintf(buf, PAGE_SIZE, "%d\n", success_fail);
}


static DEVICE_ATTR(vendor, 0444, irled_vendor_show, NULL);
static DEVICE_ATTR(name, 0444, irled_name_show, NULL);
static DEVICE_ATTR(irled_send_result, 0444, irled_send_result_show, NULL);
static DEVICE_ATTR(irled_send, 0664,
	irled_send_show, irled_send_store);


static struct device_attribute *irled_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_irled_send,
	&dev_attr_irled_send_result,
	NULL,
};

void initialize_irled_factorytest(struct ssp_data *data)
{
	sensors_register(data->irled_device, data, irled_attrs, "irled_remote");
}

void remove_irled_factorytest(struct ssp_data *data)
{
	sensors_unregister(data->irled_device, irled_attrs);
}
