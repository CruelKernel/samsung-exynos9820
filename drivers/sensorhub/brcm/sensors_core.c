/*
 *  Universal sensors core class
 *
 *  Author : Ryunkyun Park <ryun.park@samsung.com>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/input.h>

struct class *sensors_class;
EXPORT_SYMBOL_GPL(sensors_class);
struct class *sensors_event_class;
EXPORT_SYMBOL_GPL(sensors_event_class);
static atomic_t sensor_count;
static struct device *symlink_dev;
static struct device *sensor_dev;
static struct input_dev *meta_input_dev;

/*
 * Create sysfs interface
 */
static void set_sensor_attr(struct device *dev,
	struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		if ((device_create_file(dev, attributes[i])) < 0)
			pr_err("[SENSOR CORE] fail device_create_file(dev,attributes[%d])\n", i);
}

int sensors_create_symlink(struct input_dev *inputdev)
{
	int err = 0;

	if (symlink_dev == NULL) {
		pr_err("%s, symlink_dev is NULL!!!\n", __func__);
		return err;
	}

	err = sysfs_create_link(&symlink_dev->kobj, &inputdev->dev.kobj,
				inputdev->name);

	if (err < 0) {
		pr_err("%s, %s failed!(%d)\n", __func__, inputdev->name, err);
		return err;
	}

	return err;
}
EXPORT_SYMBOL_GPL(sensors_create_symlink);

void sensors_remove_symlink(struct input_dev *inputdev)
{

	if (symlink_dev == NULL) {
		pr_err("%s, symlink_dev is NULL!!!\n", __func__);
		return;
	}

	sysfs_delete_link(&symlink_dev->kobj, &inputdev->dev.kobj,
				inputdev->name);
}
EXPORT_SYMBOL_GPL(sensors_remove_symlink);


int sensors_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name)
{
	int ret = 0;

	if (!sensors_class) {
		sensors_class = class_create(THIS_MODULE, "sensors");
		if (IS_ERR(sensors_class))
			return PTR_ERR(sensors_class);
	}

	dev = device_create(sensors_class, NULL, 0, drvdata, "%s", name);

	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("[SENSORS CORE] device_create failed![%d]\n", ret);
		return ret;
	}

	set_sensor_attr(dev, attributes);
	atomic_inc(&sensor_count);

	return 0;
}
EXPORT_SYMBOL_GPL(sensors_register);

void sensors_unregister(struct device * const dev,
	struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++)
		device_remove_file(dev, attributes[i]);
}
EXPORT_SYMBOL_GPL(sensors_unregister);

void destroy_sensor_class(void)
{
	if (sensors_class) {
		device_destroy(sensors_class, sensor_dev->devt);
		class_destroy(sensors_class);
		sensor_dev = NULL;
		sensors_class = NULL;
	}

	if (sensors_event_class) {
		device_destroy(sensors_event_class, symlink_dev->devt);
		class_destroy(sensors_event_class);
		symlink_dev = NULL;
		sensors_event_class = NULL;
	}
}
EXPORT_SYMBOL_GPL(destroy_sensor_class);

static ssize_t set_flush(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	u8 sensor_type = 0;

	if (kstrtou8(buf, 10, &sensor_type) < 0)
		return -EINVAL;

	input_report_rel(meta_input_dev, REL_DIAL,
		1);	/*META_DATA_FLUSH_COMPLETE*/
	input_report_rel(meta_input_dev, REL_HWHEEL, sensor_type + 1);
	input_sync(meta_input_dev);

	pr_info("[SENSOR CORE] flush %d\n", sensor_type);
	return size;
}
static DEVICE_ATTR(flush, 0220, NULL, set_flush);
static struct device_attribute *ap_sensor_attr[] = {
	&dev_attr_flush,
	NULL,
};

int sensors_meta_input_init(void)
{
	int ret;

	/* Meta Input Event Initialization */
	meta_input_dev = input_allocate_device();
	if (!meta_input_dev) {
		pr_err("[SENSOR CORE] failed alloc meta dev\n");
		return -ENOMEM;
	}

	meta_input_dev->name = "meta_event";
	input_set_capability(meta_input_dev, EV_REL, REL_HWHEEL);
	input_set_capability(meta_input_dev, EV_REL, REL_DIAL);

	ret = input_register_device(meta_input_dev);
	if (ret < 0) {
		pr_err("[SENSOR CORE] failed register meta dev\n");
		input_free_device(meta_input_dev);
		return ret;
	}

	ret = sensors_create_symlink(meta_input_dev);
	if (ret < 0) {
		pr_err("[SENSOR CORE] failed create meta symlink\n");
		input_unregister_device(meta_input_dev);
		return ret;
	}

	return ret;
}

void sensors_meta_input_clean(void)
{
	sensors_remove_symlink(meta_input_dev);
	input_unregister_device(meta_input_dev);
}

static int __init sensors_class_init(void)
{
	pr_info("[SENSORS CORE] %s\n", __func__);
	sensors_class = class_create(THIS_MODULE, "sensors");
	if (IS_ERR(sensors_class)) {
		pr_err("%s, create sensors_class is failed.(err=%d)\n",
			__func__, IS_ERR(sensors_class));
		return PTR_ERR(sensors_class);
	}

	/* For symbolic link */
	sensors_event_class = class_create(THIS_MODULE, "sensor_event");
	if (IS_ERR(sensors_event_class)) {
		pr_err("%s, create sensors_class is failed.(err=%d)\n",
			__func__, IS_ERR(sensors_event_class));
		class_destroy(sensors_class);
		return PTR_ERR(sensors_event_class);
	}

	symlink_dev = device_create(sensors_event_class, NULL, 0, NULL,
		"%s", "symlink");
	if (IS_ERR(symlink_dev)) {
		pr_err("[SENSORS CORE] symlink_dev create failed![%d]\n",
				IS_ERR(symlink_dev));
		class_destroy(sensors_class);
		class_destroy(sensors_event_class);
		return PTR_ERR(symlink_dev);
	}

	sensor_dev = device_create(sensors_class, NULL, 0, NULL,
		"%s", "sensor_dev");
	if (IS_ERR(sensor_dev)) {
		pr_err("[SENSORS CORE] sensor_dev create failed![%d]\n",
			IS_ERR(sensor_dev));
	} else {
		if ((device_create_file(sensor_dev, *ap_sensor_attr)) < 0)
			pr_err("[SENSOR CORE] failed flush device_file\n");
	}

	atomic_set(&sensor_count, 0);
	sensors_class->dev_uevent = NULL;

	sensors_meta_input_init();

	pr_info("[SENSORS CORE] %s  succcess\n", __func__);

	return 0;
}

static void __exit sensors_class_exit(void)
{
	if (meta_input_dev)
		sensors_meta_input_clean();

	if (sensors_class || sensors_event_class) {
		class_destroy(sensors_class);
		sensors_class = NULL;
		class_destroy(sensors_event_class);
		sensors_event_class = NULL;
	}
}

subsys_initcall(sensors_class_init);
module_exit(sensors_class_exit);

MODULE_DESCRIPTION("Universal sensors core class");
MODULE_AUTHOR("Ryunkyun Park <ryun.park@samsung.com>");
MODULE_LICENSE("GPL");
