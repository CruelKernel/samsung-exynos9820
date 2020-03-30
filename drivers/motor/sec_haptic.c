/*
 * sec haptic driver for common code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) "[VIB] " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/sec_class.h>
#include <linux/sec_haptic.h>
#if defined(CONFIG_SSP_MOTOR_CALLBACK)
#include <linux/ssp_motorcallback.h>
#endif

static struct sec_haptic_drvdata *pddata;

static int sec_haptic_set_frequency(struct sec_haptic_drvdata *ddata,
	int num)
{
	if (num >= 0 && num < ddata->multi_frequency) {
		ddata->period = ddata->multi_freq_period[num];
		ddata->duty = ddata->max_duty = ddata->multi_freq_duty[num];
		ddata->intensity = MAX_INTENSITY;
		ddata->freq_num = num;
	} else if (num >= HAPTIC_ENGINE_FREQ_MIN && num <= HAPTIC_ENGINE_FREQ_MAX) {
		ddata->period = MULTI_FREQ_DIVIDER / num;
		ddata->duty = ddata->max_duty = (ddata->period * ddata->ratio) / 100;
		ddata->intensity = MAX_INTENSITY;
		ddata->freq_num = num;
	} else {
		pr_err("%s out of range %d\n", __func__, num);
		return -EINVAL;
	}

	return 0;
}

static int sec_haptic_set_intensity(struct sec_haptic_drvdata *ddata,
	int intensity)
{
	int duty = ddata->period >> 1;

	if (intensity < -(MAX_INTENSITY) || MAX_INTENSITY < intensity) {
		pr_err("%s out of range %d\n", __func__, intensity);
		return -EINVAL;
	}

	if (intensity == MAX_INTENSITY)
		duty = ddata->max_duty;
	else if (intensity == -(MAX_INTENSITY))
		duty = ddata->period - ddata->max_duty;
	else if (intensity != 0)
		duty += (ddata->max_duty - duty) * intensity / MAX_INTENSITY;
	else if (intensity == 0)
		return ddata->set_intensity(ddata->data, duty);

	ddata->intensity = intensity;
	ddata->duty = duty;

	return ddata->set_intensity(ddata->data, duty);
}

static int sec_haptic_enable(struct sec_haptic_drvdata *ddata, bool en)
{
	int ret = ddata->enable(ddata->data, en);

#if defined(CONFIG_SSP_MOTOR_CALLBACK)
	if (en && ddata->intensity > 0)
		setSensorCallback(true, ddata->timeout);
	else
		setSensorCallback(false, 0);
#endif

	return ret;
}

static void sec_haptic_set_overdrive(struct sec_haptic_drvdata *ddata, bool en)
{
	ddata->ratio = en ? ddata->overdrive_ratio : ddata->normal_ratio;
}

static int sec_haptic_boost(struct sec_haptic_drvdata *ddata, bool en)
{
	return ddata->boost(ddata->data, en);
}

static void sec_haptic_engine_run_packet(struct sec_haptic_drvdata *ddata,
		struct vib_packet packet)
{
	int frequency = packet.freq;
	int intensity = packet.intensity;
	int overdrive = packet.overdrive;

	if (!ddata->f_packet_en) {
		pr_err("haptic packet is empty\n");
		return;
	}

	sec_haptic_set_overdrive(ddata, overdrive);
	sec_haptic_set_frequency(ddata, frequency);
	if (intensity) {
		sec_haptic_set_intensity(ddata, intensity);
		if (!ddata->packet_running) {
			pr_info("[haptic engine] motor run\n");
			sec_haptic_enable(ddata, true);
		}
		ddata->packet_running = true;
	} else {
		if (ddata->packet_running) {
			pr_info("[haptic engine] motor stop\n");
			sec_haptic_enable(ddata, false);
		}
		ddata->packet_running = false;
		sec_haptic_set_intensity(ddata, intensity);
	}

	pr_info("%s [%d] freq:%d, intensity:%d, time:%d ratio: %d\n", __func__,
		ddata->packet_cnt, frequency, intensity, ddata->timeout, ddata->ratio);
}

static void timed_output_enable(struct sec_haptic_drvdata *ddata, unsigned int value)
{
	struct hrtimer *timer = &ddata->timer;
	int ret = 0;

	kthread_flush_worker(&ddata->kworker);
	ret = hrtimer_cancel(timer);

	value = min_t(int, value, (int)ddata->max_timeout);

	if (value) {
		mutex_lock(&ddata->mutex);
		if (ddata->f_packet_en) {
			ddata->packet_running = false;
			ddata->timeout = ddata->test_pac[0].time;
			sec_haptic_engine_run_packet(ddata, ddata->test_pac[0]);
		} else {
			ddata->timeout = value;

			ret = sec_haptic_set_intensity(ddata, ddata->intensity);
			if (ret)
				pr_err("%s: error to enable pwm %d\n", __func__, ret);

			ret = sec_haptic_enable(ddata, true);
			if (ret)
				pr_err("%s: error to enable i2c reg %d\n", __func__, ret);

			if (ddata->multi_frequency)
				pr_info("%d %u %ums\n", ddata->freq_num, ddata->duty, ddata->timeout);
			else
				pr_info("%u %ums\n", ddata->duty, ddata->timeout);
		}

		mutex_unlock(&ddata->mutex);
		hrtimer_start(timer,
			ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC),
			HRTIMER_MODE_REL);
	} else {
		mutex_lock(&ddata->mutex);
		ddata->f_packet_en = false;
		ddata->packet_cnt = 0;
		ddata->packet_size = 0;
		sec_haptic_enable(ddata, false);
		sec_haptic_set_intensity(ddata, 0);

		pr_info("off\n");
		mutex_unlock(&ddata->mutex);
	}
}

static enum hrtimer_restart haptic_timer_func(struct hrtimer *timer)
{
	struct sec_haptic_drvdata *ddata
		= container_of(timer, struct sec_haptic_drvdata, timer);

	pr_info("%s\n", __func__);
	kthread_queue_work(&ddata->kworker, &ddata->kwork);
	return HRTIMER_NORESTART;
}

static void sec_haptic_work(struct kthread_work *work)
{
	struct sec_haptic_drvdata *ddata
		= container_of(work, struct sec_haptic_drvdata, kwork);
	struct hrtimer *timer = &ddata->timer;

	mutex_lock(&ddata->mutex);

	if (ddata->f_packet_en) {
		ddata->packet_cnt++;
		if (ddata->packet_cnt < ddata->packet_size) {
			sec_haptic_engine_run_packet(ddata, ddata->test_pac[ddata->packet_cnt]);
			ddata->timeout = ddata->test_pac[ddata->packet_cnt].time;
			hrtimer_start(timer, ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC),
					HRTIMER_MODE_REL);

			goto unlock_without_vib_off;
		} else {
			ddata->f_packet_en = false;
			ddata->packet_cnt = 0;
			ddata->packet_size = 0;
		}
	}

	sec_haptic_enable(ddata, false);
	sec_haptic_set_intensity(ddata, 0);

	pr_info("timer off\n");

unlock_without_vib_off:
	mutex_unlock(&ddata->mutex);
}

static ssize_t duty_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	int ret;
	u16 duty;

	ret = kstrtou16(buf, 0, &duty);
	if (ret != 0) {
		dev_err(dev, "fail to get duty.\n");
		return count;
	}
	ddata->duty = ddata->max_duty = duty;
	if (ddata->multi_frequency)
		ddata->multi_freq_duty[0] = duty;
	ddata->intensity = MAX_INTENSITY;

	return count;
}

static ssize_t duty_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"duty: %u\n", ddata->duty);
}

static ssize_t period_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	int ret;
	u16 period;

	ret = kstrtou16(buf, 0, &period);
	if (ret != 0) {
		dev_err(dev, "fail to get period.\n");
		return count;
	}
	ddata->period = period;
	if (ddata->multi_frequency)
		ddata->multi_freq_period[0] = period;

	return count;
}

static ssize_t period_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"period: %u\n", ddata->period);
}

/* below nodes is SAMSUNG specific nodes */
static DEVICE_ATTR_RW(duty);
static DEVICE_ATTR_RW(period);

static struct attribute *sec_haptic_attributes[] = {
	&dev_attr_duty.attr,
	&dev_attr_period.attr,
	NULL,
};

static struct attribute_group sec_haptic_attr_group = {
	.attrs = sec_haptic_attributes,
};

static ssize_t intensity_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	int intensity = 0, ret = 0;

	ret = kstrtoint(buf, 0, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, intensity);
	
	if ((intensity < 1) || ( intensity > MAX_INTENSITY)) {
		pr_err("%s out of range %d\n", __func__, intensity);
		return -EINVAL;
	}

	ddata->intensity = intensity;

	return count;
}

static ssize_t intensity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"intensity: %u\n", ddata->intensity);
}


static ssize_t motor_type_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"%s\n", ddata->vib_type);
}

#if 0
static ssize_t force_touch_intensity_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	int intensity = 0, ret = 0;

	ret = kstrtoint(buf, 0, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, intensity);

	if ((intensity < 1) || ( intensity > MAX_INTENSITY)) {
		pr_err("%s out of range %d\n", __func__, intensity);
		return -EINVAL;
	}

	ddata->force_touch_intensity = intensity;

	return count;
}
static ssize_t force_touch_intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"force touch intensity: %u\n", ddata->force_touch_intensity);
}
#endif
static ssize_t multi_freq_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	int num, ret;

	ret = kstrtoint(buf, 0, &num);
	if (ret) {
		pr_err("fail to get frequency\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, num);

	if (num < 0 || num >= HOMEKEY_PRESS_FREQ)
		return -EINVAL;
	
	ret = sec_haptic_set_frequency(ddata, num);
	if (ret)
		return ret;

	return count;
}

static ssize_t multi_freq_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE,
		"frequency: %d\n", ddata->freq_num);
}

static ssize_t haptic_engine_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	int i = 0, _data = 0, tmp = 0;

	if (sscanf(buf, "%6d", &_data) != 1)
		return -EINVAL;

	if (_data > PACKET_MAX_SIZE * VIB_PACKET_MAX) {
		pr_info("%s, [%d] packet size over\n", __func__, _data);
		return -EINVAL;
	} else {
		ddata->packet_size = _data / VIB_PACKET_MAX;
		ddata->packet_cnt = 0;
		ddata->f_packet_en = true;

		buf = strstr(buf, " ");

		for (i = 0; i < ddata->packet_size; i++) {
			for (tmp = 0; tmp < VIB_PACKET_MAX; tmp++) {
				if (buf == NULL) {
					pr_err("%s, buf is NULL, Please check packet data again\n",
							__func__);
					ddata->f_packet_en = false;
					return -EINVAL;
				}

				if (sscanf(buf++, "%6d", &_data) != 1) {
					pr_err("%s, packet data error, Please check packet data again\n",
							__func__);
					ddata->f_packet_en = false;
					return -EINVAL;
				}

				switch (tmp) {
				case VIB_PACKET_TIME:
					ddata->test_pac[i].time = _data;
					break;
				case VIB_PACKET_INTENSITY:
					ddata->test_pac[i].intensity = _data;
					break;
				case VIB_PACKET_FREQUENCY:
					ddata->test_pac[i].freq = _data;
					break;
				case VIB_PACKET_OVERDRIVE:
					ddata->test_pac[i].overdrive = _data;
					break;
				}
				buf = strstr(buf, " ");
			}
		}
	}

	return count;
}

static ssize_t haptic_engine_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	int i = 0;
	size_t size = 0;

	for (i = 0; i < ddata->packet_size && ddata->f_packet_en &&
			((4 * VIB_BUFSIZE + size) < PAGE_SIZE); i++) {
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->test_pac[i].time);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->test_pac[i].intensity);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->test_pac[i].freq);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->test_pac[i].overdrive);
	}

	return size;
}

static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	struct hrtimer *timer = &ddata->timer;
	int remaining = 0;

	if (hrtimer_active(timer)) {
		ktime_t remain = hrtimer_get_remaining(timer);
		struct timeval t = ktime_to_timeval(remain);

		remaining = t.tv_sec * 1000 + t.tv_usec / 1000;
	}
	return sprintf(buf, "%d\n", remaining);
}

static ssize_t enable_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	struct sec_haptic_drvdata *ddata = dev_get_drvdata(dev);
	int value;
	int ret;

	ret = kstrtoint(buf, 0, &value);
	if (ret != 0)
		return -EINVAL;

	timed_output_enable(ddata, value);
	return size;
}

static DEVICE_ATTR_RW(haptic_engine);
static DEVICE_ATTR_RW(multi_freq);
static DEVICE_ATTR_RW(intensity);
//static DEVICE_ATTR_RW(force_touch_intensity);
static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR(motor_type, S_IWUSR | S_IRUGO, motor_type_show, NULL);

static struct attribute *timed_output_attributes[] = {
	&dev_attr_intensity.attr,
	&dev_attr_enable.attr,
	&dev_attr_motor_type.attr,
	NULL,
};

static struct attribute_group timed_output_attr_group = {
	.attrs = timed_output_attributes,
};

static struct attribute *multi_freq_attributes[] = {
	&dev_attr_haptic_engine.attr,
	&dev_attr_multi_freq.attr,
//	&dev_attr_force_touch_intensity.attr,
	NULL,
};

static struct attribute_group multi_freq_attr_group = {
	.attrs = multi_freq_attributes,
};

int sec_haptic_register(struct sec_haptic_drvdata *ddata)
{
	struct task_struct *kworker_task;
	int ret = 0;

	mutex_init(&ddata->mutex);
	kthread_init_worker(&ddata->kworker);
	kworker_task = kthread_run(kthread_worker_fn,
		   &ddata->kworker, "sec_haptic");
	if (IS_ERR(kworker_task)) {
		pr_err("Failed to create message pump task\n");
		ret = -ENOMEM;
		goto err_kthread;
	}

	kthread_init_work(&ddata->kwork, sec_haptic_work);
	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = haptic_timer_func;

	ddata->to_class = class_create(THIS_MODULE, "timed_output");
	if (IS_ERR(ddata->to_class)) {
		ret = PTR_ERR(ddata->to_class);
		goto err_class_create;
	}
	ddata->to_dev = device_create(ddata->to_class, NULL,
		MKDEV(0, 0), ddata, "vibrator");
	if (IS_ERR(ddata->dev)) {
		ret = PTR_ERR(ddata->dev);
		goto err_device_create;
	}
	ret = sysfs_create_group(&ddata->to_dev->kobj, &timed_output_attr_group);
	if (ret) {
		ret = -ENODEV;
		pr_err("Failed to create sysfs %d\n", ret);
		goto err_sysfs;
	}
	if (ddata->multi_frequency) {
		ret = sysfs_create_group(&ddata->to_dev->kobj, &multi_freq_attr_group);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs %d\n", ret);
			goto err_sysfs2;
		}
	}
	ddata->dev = sec_device_create(ddata, "motor");
	if (IS_ERR(ddata->dev)) {
		ret = -ENODEV;
		pr_err("Failed to create device %d\n", ret);
		goto exit_sec_devices;
	}
	ret = sysfs_create_group(&ddata->dev->kobj, &sec_haptic_attr_group);
	if (ret) {
		ret = -ENODEV;
		pr_err("Failed to create sysfs %d\n", ret);
		goto err_sysfs3;
	}

	pddata = ddata;

	return ret;

err_sysfs3:
	sysfs_remove_group(&ddata->dev->kobj, &sec_haptic_attr_group);
exit_sec_devices:
	sysfs_remove_group(&ddata->to_dev->kobj, &multi_freq_attr_group);
err_sysfs2:
	sysfs_remove_group(&ddata->to_dev->kobj, &timed_output_attr_group);
err_sysfs:
	sec_device_destroy(ddata->dev->devt);
err_device_create:
err_class_create:
err_kthread:
	return ret;
}

int sec_haptic_unregister(struct sec_haptic_drvdata *ddata)
{
	sec_haptic_boost(ddata, BOOST_OFF);
	sysfs_remove_group(&ddata->dev->kobj, &sec_haptic_attr_group);
	sysfs_remove_group(&ddata->to_dev->kobj, &multi_freq_attr_group);
	sysfs_remove_group(&ddata->to_dev->kobj, &timed_output_attr_group);
	sec_device_destroy(ddata->dev->devt);
	sec_haptic_enable(ddata, false);
	device_destroy(ddata->to_class, MKDEV(0, 0));
	class_destroy(ddata->to_class);
	pddata = NULL;
	kfree(ddata);
	return 0;
}
#if 0
extern int haptic_homekey_press(void)
{
	struct sec_haptic_drvdata *ddata = pddata;
	struct hrtimer *timer;

	if (ddata == NULL)
		return -1;
	if (!ddata->multi_frequency)
		return -1;
	timer = &ddata->timer;

	sec_haptic_boost(ddata, BOOST_ON);

	mutex_lock(&ddata->mutex);
	ddata->timeout = HOMEKEY_DURATION;
	sec_haptic_set_frequency(ddata, HOMEKEY_PRESS_FREQ);
	sec_haptic_set_intensity(ddata, ddata->force_touch_intensity);
	sec_haptic_enable(ddata, true);

	pr_info("%s freq:%d, intensity:%d, time:%d\n", __func__,
			HOMEKEY_PRESS_FREQ, ddata->force_touch_intensity, ddata->timeout);
	mutex_unlock(&ddata->mutex);

	hrtimer_start(timer,
		ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC),
		HRTIMER_MODE_REL);

	return 0;
}

extern int haptic_homekey_release(void)
{
	struct sec_haptic_drvdata *ddata = pddata;
	struct hrtimer *timer;

	if (ddata == NULL)
		return -1;
	if (!ddata->multi_frequency)
		return -1;
	timer = &ddata->timer;

	mutex_lock(&ddata->mutex);
	ddata->timeout = HOMEKEY_DURATION;
	sec_haptic_set_frequency(ddata, HOMEKEY_RELEASE_FREQ);
	sec_haptic_set_intensity(ddata, ddata->force_touch_intensity);
	sec_haptic_enable(ddata, true);

	pr_info("%s freq:%d, intensity:%d, time:%d\n", __func__,
			HOMEKEY_RELEASE_FREQ, ddata->force_touch_intensity, ddata->timeout);
	mutex_unlock(&ddata->mutex);

	hrtimer_start(timer,
		ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC),
		HRTIMER_MODE_REL);

	return 0;
}
#endif
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sec haptic driver");
