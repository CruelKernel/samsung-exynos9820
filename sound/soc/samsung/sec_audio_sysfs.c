/*
 *  sec_audio_sysfs.c
 *
 *  Copyright (c) 2017 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <sound/samsung/sec_audio_sysfs.h>

struct sec_audio_sysfs_data *audio_data;

#ifdef CONFIG_SND_SOC_SAMSUNG_SYSFS_EARJACK
int audio_register_jack_select_cb(int (*set_jack) (int))
{
	if (audio_data->set_jack_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->set_jack_state = set_jack;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_jack_select_cb);

static ssize_t audio_jack_select_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (audio_data->set_jack_state) {
		if ((!size) || (buf[0] != '1')) {
			dev_info(dev, "%s: Forced remove jack\n", __func__);
			audio_data->set_jack_state(0);
		} else {
			dev_info(dev, "%s: Forced detect jack\n", __func__);
			audio_data->set_jack_state(1);
		}
	} else {
		dev_info(dev, "%s: No callback registered\n", __func__);
	}

	return size;
}

static DEVICE_ATTR(select_jack, 0664,
			NULL, audio_jack_select_store);

int audio_register_jack_state_cb(int (*jack_state) (void))
{
	if (audio_data->get_jack_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_jack_state = jack_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_jack_state_cb);

static ssize_t audio_jack_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_jack_state)
		report = audio_data->get_jack_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(state, 0664,
			audio_jack_state_show, NULL);

int audio_register_key_state_cb(int (*key_state) (void))
{
	if (audio_data->get_key_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_key_state = key_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_key_state_cb);

static ssize_t audio_key_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_key_state)
		report = audio_data->get_key_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(key_state, 0664,
			audio_key_state_show, NULL);

int audio_register_mic_adc_cb(int (*mic_adc) (void))
{
	if (audio_data->get_mic_adc) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_mic_adc = mic_adc;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_mic_adc_cb);

static ssize_t audio_mic_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_mic_adc)
		report = audio_data->get_mic_adc();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 16, "%d\n", report);
}

static DEVICE_ATTR(mic_adc, 0664,
			audio_mic_adc_show, NULL);

static struct attribute *sec_audio_jack_attr[] = {
	&dev_attr_select_jack.attr,
	&dev_attr_state.attr,
	&dev_attr_key_state.attr,
	&dev_attr_mic_adc.attr,
	NULL,
};

static struct attribute_group sec_audio_jack_attr_group = {
	.attrs = sec_audio_jack_attr,
};
#endif

int audio_register_codec_id_state_cb(int (*codec_id_state) (void))
{
	if (audio_data->get_codec_id_state) {
		dev_err(audio_data->codec_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_codec_id_state = codec_id_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_codec_id_state_cb);

static ssize_t audio_check_codec_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_codec_id_state)
		report = audio_data->get_codec_id_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(check_codec_id, 0664,
			audio_check_codec_id_show, NULL);

static struct attribute *sec_audio_codec_attr[] = {
	&dev_attr_check_codec_id.attr,
	NULL,
};

static struct attribute_group sec_audio_codec_attr_group = {
	.attrs = sec_audio_codec_attr,
};

static int __init sec_audio_sysfs_init(void)
{
	dev_t dev_id;
	int ret;

	audio_data = kzalloc(sizeof(struct sec_audio_sysfs_data), GFP_KERNEL);
	if (audio_data == NULL)
		return -ENOMEM;

	audio_data->audio_class = class_create(THIS_MODULE, "audio");
	if (IS_ERR(audio_data->audio_class)) {
		ret = PTR_ERR(audio_data->audio_class);
		pr_err("%s: Failed to create audio class (%d)\n",
							__func__, ret);
		kfree(audio_data);
		audio_data = NULL;
		return ret;
	}

	dev_id = 0;

#ifdef CONFIG_SND_SOC_SAMSUNG_SYSFS_EARJACK
	audio_data->jack_dev = device_create(audio_data->audio_class, NULL,
						++dev_id, NULL, "earjack");
	if (IS_ERR(audio_data->jack_dev)) {
		ret = PTR_ERR(audio_data->jack_dev);
		pr_err("%s: Failed to create earjack device (%d)\n",
							__func__, ret);
		audio_data->jack_dev = NULL;
		--dev_id;
	} else {
		ret = sysfs_create_group(&audio_data->jack_dev->kobj,
					&sec_audio_jack_attr_group);
		if (ret) {
			pr_err("%s: Failed to create earjack sysfs (%d)\n",
								__func__, ret);
			device_destroy(audio_data->audio_class,
					audio_data->jack_dev_id);
			audio_data->jack_dev = NULL;
			--dev_id;
		} else {
			pr_info("%s: create earjack device id(%lu)\n",
							__func__, dev_id);
			audio_data->jack_dev_id = dev_id;
		}
	}
#endif

	audio_data->codec_dev = device_create(audio_data->audio_class, NULL,
						++dev_id, NULL, "codec");
	if (IS_ERR(audio_data->codec_dev)) {
		ret = PTR_ERR(audio_data->codec_dev);
		pr_err("%s: Failed to create codec device (%d)\n",
							__func__, ret);
		audio_data->codec_dev = NULL;
		--dev_id;
	} else {
		ret = sysfs_create_group(&audio_data->codec_dev->kobj,
					&sec_audio_codec_attr_group);
		if (ret) {
			pr_err("%s: Failed to create codec sysfs (%d)\n",
								__func__, ret);
			device_destroy(audio_data->audio_class,
					audio_data->codec_dev_id);
			audio_data->codec_dev = NULL;
			--dev_id;
		} else {
			pr_info("%s: create codec device id(%lu)\n",
							__func__, dev_id);
			audio_data->codec_dev_id = dev_id;
		}
	}

	return 0;
}
subsys_initcall(sec_audio_sysfs_init);

static void __exit sec_audio_sysfs_exit(void)
{
#ifdef CONFIG_SND_SOC_SAMSUNG_SYSFS_EARJACK
	if (audio_data->jack_dev) {
		sysfs_remove_group(&audio_data->jack_dev->kobj,
				&sec_audio_jack_attr_group);
		device_destroy(audio_data->audio_class,
				audio_data->jack_dev_id);
	}
#endif

	if (audio_data->codec_dev) {
		sysfs_remove_group(&audio_data->codec_dev->kobj,
				&sec_audio_codec_attr_group);
		device_destroy(audio_data->audio_class,
				audio_data->codec_dev_id);
	}

	if (audio_data->audio_class)
		class_destroy(audio_data->audio_class);

	kfree(audio_data);
}
module_exit(sec_audio_sysfs_exit);

MODULE_DESCRIPTION("Samsung Electronics Audio SYSFS driver");
MODULE_LICENSE("GPL");
