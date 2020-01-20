/*
 * cs40l2x.c -- CS40L20/CS40L25/CS40L25A/CS40L25B Haptics Driver
 *
 * Copyright 2018 Cirrus Logic, Inc.
 *
 * Author: Jeff LaBundy <jeff.labundy@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt) "[VIB] " fmt
#define DEBUG_VIB 0
#define DEBUG_PRINT_PLAYBACK_QUEUE 40

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/regulator/consumer.h>
#include <linux/regmap.h>
#include <linux/sysfs.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/platform_data/cs40l2x.h>

#include "cs40l2x.h"

#ifdef CONFIG_ANDROID_TIMED_OUTPUT
#include "timed_output.h"
#else
#include <linux/leds.h>
#endif /* CONFIG_ANDROID_TIMED_OUTPUT */

struct cs40l2x_private {
	struct device *dev;
	struct regmap *regmap;
	struct regulator_bulk_data supplies[2];
	unsigned int num_supplies;
	unsigned int devid;
	unsigned int revid;
	struct work_struct vibe_start_work;
	struct work_struct vibe_pbq_work;
	struct work_struct vibe_stop_work;
	struct workqueue_struct *vibe_workqueue;
	struct mutex lock;
	unsigned int cp_trigger_index;
	unsigned int cp_trailer_index;
	unsigned int num_waves;
	unsigned int fw_id_match;
	unsigned int fw_rev_min;
	unsigned int vibegen_id;
	unsigned int vibegen_rev;
	unsigned int wt_limit_xm;
	unsigned int wt_limit_ym;
	bool vibe_init_success;
	struct gpio_desc *reset_gpio;
	struct cs40l2x_platform_data pdata;
	struct list_head coeff_desc_head;
	unsigned char diag_state;
	unsigned int diag_dig_scale;
	unsigned int f0_measured;
	unsigned int redc_measured;
	unsigned int intensity;
	unsigned int force_touch_intensity;
	struct cs40l2x_pbq_pair pbq_pairs[CS40L2X_PBQ_DEPTH_MAX];
	struct hrtimer pbq_timer;
	unsigned int pbq_depth;
	unsigned int pbq_index;
	unsigned int pbq_state;
	unsigned int pbq_cp_dig_scale;
	int pbq_repeat;
	int pbq_remain;
	struct cs40l2x_wseq_pair wseq_table[CS40L2X_WSEQ_LENGTH_MAX];
	unsigned int wseq_length;
	unsigned int event_control;
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
	struct timed_output_dev timed_dev;
	struct hrtimer vibe_timer;
	int vibe_timeout;
#else
	struct led_classdev led_dev;
#endif /* CONFIG_ANDROID_TIMED_OUTPUT */
};

static const char * const cs40l2x_supplies[] = {
	"VA",
	"VP",
};

static const char * const cs40l2x_part_nums[] = {
	"CS40L20",
	"CS40L25",
	"CS40L25A",
	"CS40L25B",
};

static const char * const cs40l2x_event_regs[] = {
	"GPIO1EVENT",
	"GPIO2EVENT",
	"GPIO3EVENT",
	"GPIO4EVENT",
	"GPIOPLAYBACKEVENT",
	"TRIGGERPLAYBACKEVENT",
	"RXREADYEVENT",
	"HARDWAREEVENT",
};

static const unsigned int cs40l2x_event_masks[] = {
	CS40L2X_EVENT_GPIO1_ENABLED,
	CS40L2X_EVENT_GPIO2_ENABLED,
	CS40L2X_EVENT_GPIO3_ENABLED,
	CS40L2X_EVENT_GPIO4_ENABLED,
	CS40L2X_EVENT_START_ENABLED | CS40L2X_EVENT_END_ENABLED,
	CS40L2X_EVENT_START_ENABLED | CS40L2X_EVENT_END_ENABLED,
	CS40L2X_EVENT_READY_ENABLED,
	CS40L2X_EVENT_HARDWARE_ENABLED,
};

static struct cs40l2x_private *cs40l2x_get_private(struct device *dev)
{
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
	/* timed output device does not register under a parent device */
	return container_of(dev_get_drvdata(dev),
			struct cs40l2x_private, timed_dev);
#else
	return dev_get_drvdata(dev);
#endif /* CONFIG_ANDROID_TIMED_OUTPUT */
}

static ssize_t cs40l2x_cp_trigger_index_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", cs40l2x->cp_trigger_index);
}

static ssize_t cs40l2x_cp_trigger_index_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int index;

	ret = kstrtou32(buf, 10, &index);
	if (ret)
		return -EINVAL;

#if 1 //DEBUG_VIB
	pr_info("%s index:%u (num_waves:%u)\n", __func__, index, cs40l2x->num_waves);	
#endif

	if ((index & CS40L2X_INDEX_MASK) >= cs40l2x->num_waves
				&& index != CS40L2X_INDEX_PBQ
				&& index != CS40L2X_INDEX_DIAG)
		return -EINVAL;

	cs40l2x->cp_trigger_index = index;

	return count;
}

static ssize_t cs40l2x_cp_trigger_queue_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	ssize_t len = 0;
	int i;

	if (cs40l2x->pbq_depth == 0)
		return -ENODATA;

	mutex_lock(&cs40l2x->lock);

	for (i = 0; i < cs40l2x->pbq_depth; i++) {
		switch (cs40l2x->pbq_pairs[i].tag) {
		case CS40L2X_PBQ_TAG_SILENCE:
			len += snprintf(buf + len, PAGE_SIZE - len, "%d",
					cs40l2x->pbq_pairs[i].mag);
			break;
		case CS40L2X_PBQ_TAG_START:
			len += snprintf(buf + len, PAGE_SIZE - len, "!!");
			break;
		case CS40L2X_PBQ_TAG_STOP:
			len += snprintf(buf + len, PAGE_SIZE - len, "%d!!",
					cs40l2x->pbq_pairs[i].repeat);
			break;
		default:
			len += snprintf(buf + len, PAGE_SIZE - len, "%d.%d",
					cs40l2x->pbq_pairs[i].tag,
					cs40l2x->pbq_pairs[i].mag);
		}

		if (i < (cs40l2x->pbq_depth - 1))
			len += snprintf(buf + len, PAGE_SIZE - len, ", ");
	}

	switch (cs40l2x->pbq_repeat) {
	case -1:
		len += snprintf(buf + len, PAGE_SIZE - len, ", ~\n");
		break;
	case 0:
		len += snprintf(buf + len, PAGE_SIZE - len, "\n");
		break;
	default:
		len += snprintf(buf + len, PAGE_SIZE - len, ", %d!\n",
				cs40l2x->pbq_repeat);
	}

	mutex_unlock(&cs40l2x->lock);

	return len;
}

static ssize_t cs40l2x_cp_trigger_queue_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	char *pbq_str_alloc, *pbq_str, *pbq_str_tok;
	char *pbq_seg_alloc, *pbq_seg, *pbq_seg_tok;
	size_t pbq_seg_len;
	unsigned int pbq_depth = 0;
	unsigned int val, num_empty;
	int pbq_marker = -1;
	int ret;

	pbq_str_alloc = kzalloc(count, GFP_KERNEL);
	if (!pbq_str_alloc)
		return -ENOMEM;

	pbq_seg_alloc = kzalloc(CS40L2X_PBQ_SEG_LEN_MAX + 1, GFP_KERNEL);
	if (!pbq_seg_alloc) {
		kfree(pbq_str_alloc);
		return -ENOMEM;
	}

	mutex_lock(&cs40l2x->lock);

	cs40l2x->pbq_depth = 0;
	cs40l2x->pbq_repeat = 0;

	pbq_str = pbq_str_alloc;
	strlcpy(pbq_str, buf, count);

#if DEBUG_VIB
	pr_info("%s pbq_str=%s\n", __func__, pbq_str);
#endif

	pbq_str_tok = strsep(&pbq_str, ",");

	while (pbq_str_tok) {
		pbq_seg = pbq_seg_alloc;
		pbq_seg_len = strlcpy(pbq_seg, strim(pbq_str_tok),
				CS40L2X_PBQ_SEG_LEN_MAX + 1);
		if (pbq_seg_len > CS40L2X_PBQ_SEG_LEN_MAX) {
			ret = -E2BIG;
			goto err_mutex;
		}

		/* waveform specifier */
		if (strnchr(pbq_seg, CS40L2X_PBQ_SEG_LEN_MAX, '.')) {
			/* index */
			pbq_seg_tok = strsep(&pbq_seg, ".");

			ret = kstrtou32(pbq_seg_tok, 10, &val);
			if (ret) {
				ret = -EINVAL;
				goto err_mutex;
			}
			if (val == 0 || val >= cs40l2x->num_waves) {
				ret = -EINVAL;
				goto err_mutex;
			}
			cs40l2x->pbq_pairs[pbq_depth].tag = val;

			/* scale */
			pbq_seg_tok = strsep(&pbq_seg, ".");

			ret = kstrtou32(pbq_seg_tok, 10, &val);
			if (ret) {
				ret = -EINVAL;
				goto err_mutex;
			}
			if (val == 0 || val > CS40L2X_PBQ_SCALE_MAX) {
				ret = -EINVAL;
				goto err_mutex;
			}
			cs40l2x->pbq_pairs[pbq_depth++].mag = val;

		/* repetition specifier */
		} else if (strnchr(pbq_seg, CS40L2X_PBQ_SEG_LEN_MAX, '!')) {
			val = 0;
			num_empty = 0;

			pbq_seg_tok = strsep(&pbq_seg, "!");

			while (pbq_seg_tok) {
				if (strnlen(pbq_seg_tok,
						CS40L2X_PBQ_SEG_LEN_MAX)) {
					ret = kstrtou32(pbq_seg_tok, 10, &val);
					if (ret) {
						ret = -EINVAL;
						goto err_mutex;
					}
					if (val > CS40L2X_PBQ_REPEAT_MAX) {
						ret = -EINVAL;
						goto err_mutex;
					}
				} else {
					num_empty++;
				}

				pbq_seg_tok = strsep(&pbq_seg, "!");
			}

			/* number of empty tokens reveals specifier type */
			switch (num_empty) {
			case 1:	/* outer loop: "n!" or "!n" */
				if (cs40l2x->pbq_repeat) {
					ret = -EINVAL;
					goto err_mutex;
				}
				cs40l2x->pbq_repeat = val;
				break;

			case 2:	/* inner loop stop: "n!!" or "!!n" */
				if (pbq_marker < 0) {
					ret = -EINVAL;
					goto err_mutex;
				}

				cs40l2x->pbq_pairs[pbq_depth].tag =
						CS40L2X_PBQ_TAG_STOP;
				cs40l2x->pbq_pairs[pbq_depth].mag = pbq_marker;
				cs40l2x->pbq_pairs[pbq_depth++].repeat = val;
				pbq_marker = -1;
				break;

			case 3:	/* inner loop start: "!!" */
				if (pbq_marker >= 0) {
					ret = -EINVAL;
					goto err_mutex;
				}

				cs40l2x->pbq_pairs[pbq_depth].tag =
						CS40L2X_PBQ_TAG_START;
				pbq_marker = pbq_depth++;
				break;

			default:
				ret = -EINVAL;
				goto err_mutex;
			}

		/* loop specifier */
		} else if (strnchr(pbq_seg, CS40L2X_PBQ_SEG_LEN_MAX, '~')) {
			if (cs40l2x->pbq_repeat) {
				ret = -EINVAL;
				goto err_mutex;
			}
			cs40l2x->pbq_repeat = -1;

		/* duration specifier */
		} else {
			cs40l2x->pbq_pairs[pbq_depth].tag =
					CS40L2X_PBQ_TAG_SILENCE;

			ret = kstrtou32(pbq_seg, 10, &val);
			if (ret) {
				ret = -EINVAL;
				goto err_mutex;
			}
			if (val > CS40L2X_PBQ_DELAY_MAX) {
				ret = -EINVAL;
				goto err_mutex;
			}
			cs40l2x->pbq_pairs[pbq_depth++].mag = val;
		}

		if (pbq_depth == CS40L2X_PBQ_DEPTH_MAX) {
			ret = -E2BIG;
			goto err_mutex;
		}

		pbq_str_tok = strsep(&pbq_str, ",");
	}

#if 1//DEBUG_VIB
	if(count > DEBUG_PRINT_PLAYBACK_QUEUE) {
		strlcpy(pbq_str_alloc, buf, DEBUG_PRINT_PLAYBACK_QUEUE);
		pr_info("%s pbq_str=%s... (depth:%d)\n", __func__, pbq_str_alloc, pbq_depth);
	} else {
		strlcpy(pbq_str_alloc, buf, count);
		pr_info("%s pbq_str=%s (depth:%d)\n", __func__, pbq_str_alloc, pbq_depth);
	}
#endif
	cs40l2x->pbq_depth = pbq_depth;
	ret = count;

err_mutex:
	mutex_unlock(&cs40l2x->lock);

	kfree(pbq_str_alloc);
	kfree(pbq_seg_alloc);

	return ret;
}

static unsigned int cs40l2x_dsp_reg(struct cs40l2x_private *cs40l2x,
			const char *coeff_name, const unsigned char block_type)
{
	struct cs40l2x_coeff_desc *coeff_desc;

	list_for_each_entry(coeff_desc, &cs40l2x->coeff_desc_head, list) {
		if (strcmp(coeff_desc->name, coeff_name))
			continue;
		if (coeff_desc->block_type != block_type)
			continue;

		return coeff_desc->reg;
	}

	/* return an identifiable register that is known to be read-only */
	return CS40L2X_DEVID;
}

static int cs40l2x_wseq_add_reg(struct cs40l2x_private *cs40l2x,
			unsigned int reg, unsigned int val)
{
	if (cs40l2x->wseq_length == CS40L2X_WSEQ_LENGTH_MAX)
		return -E2BIG;

	cs40l2x->wseq_table[cs40l2x->wseq_length].reg = reg;
	cs40l2x->wseq_table[cs40l2x->wseq_length++].val = val;

	return 0;
}

static int cs40l2x_wseq_add_seq(struct cs40l2x_private *cs40l2x,
			const struct reg_sequence *seq, unsigned int len)
{
	int ret, i;

	for (i = 0; i < len; i++) {
		ret = cs40l2x_wseq_add_reg(cs40l2x, seq[i].reg, seq[i].def);
		if (ret)
			return ret;
	}

	return 0;
}

static int cs40l2x_wseq_write(struct cs40l2x_private *cs40l2x, unsigned int pos,
			unsigned int reg, unsigned int val)
{
	unsigned int wseq_base = cs40l2x_dsp_reg(cs40l2x, "POWERONSEQUENCE",
			CS40L2X_XM_UNPACKED_TYPE);
	int ret;

	/* upper half */
	ret = regmap_write(cs40l2x->regmap,
			wseq_base + pos * CS40L2X_WSEQ_STRIDE,
			((reg & CS40L2X_WSEQ_REG_MASK1)
				<< CS40L2X_WSEQ_REG_SHIFTUP) |
					((val & CS40L2X_WSEQ_VAL_MASK1)
						>> CS40L2X_WSEQ_VAL_SHIFTDN));
	if (ret)
		return ret;

	/* lower half */
	return regmap_write(cs40l2x->regmap,
			wseq_base + pos * CS40L2X_WSEQ_STRIDE + 4,
			val & CS40L2X_WSEQ_VAL_MASK2);
}

static int cs40l2x_wseq_init(struct cs40l2x_private *cs40l2x)
{
	unsigned int wseq_base = cs40l2x_dsp_reg(cs40l2x, "POWERONSEQUENCE",
			CS40L2X_XM_UNPACKED_TYPE);
	int ret, i;

	for (i = 0; i < cs40l2x->wseq_length; i++) {
		ret = cs40l2x_wseq_write(cs40l2x, i,
				cs40l2x->wseq_table[i].reg,
				cs40l2x->wseq_table[i].val);
		if (ret)
			return ret;
	}

	return regmap_write(cs40l2x->regmap,
			wseq_base + cs40l2x->wseq_length * CS40L2X_WSEQ_STRIDE,
			CS40L2X_WSEQ_LIST_TERM);
}

static int cs40l2x_wseq_replace(struct cs40l2x_private *cs40l2x,
			unsigned int reg, unsigned int val)
{
	int ret, i;

	/* write sequencer does not exist in A0/B0 firmware */
	if (cs40l2x->revid < CS40L2X_REVID_B1)
		return 0;

	for (i = 0; i < cs40l2x->wseq_length; i++)
		if (cs40l2x->wseq_table[i].reg == reg)
			break;

	if (i == cs40l2x->wseq_length)
		return -EINVAL;

	ret = cs40l2x_wseq_write(cs40l2x, i, reg, val);
	if (ret)
		return ret;

	cs40l2x->wseq_table[i].val = val;

	return 0;
}

static int cs40l2x_hiber_cmd_send(struct cs40l2x_private *cs40l2x,
			unsigned int hiber_cmd)
{
	int ret, i;
	unsigned int val;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	switch (hiber_cmd) {
	case CS40L2X_POWERCONTROL_NONE:
	case CS40L2X_POWERCONTROL_HIBERNATE:
		return regmap_write(cs40l2x->regmap,
				CS40L2X_MBOX_POWERCONTROL,
				hiber_cmd);

	case CS40L2X_POWERCONTROL_WAKEUP:
		for (i = 0; i < CS40L2X_WAKEUP_RETRIES; i++) {
			/*
			 * the first several transactions are expected to be
			 * NAK'd, so retry multiple times in rapid succession
			 */
			ret = regmap_write(cs40l2x->regmap,
					CS40L2X_MBOX_POWERCONTROL,
					hiber_cmd);
			if (!ret)
				break;

			usleep_range(1000, 1100);
		}
		if (i == CS40L2X_WAKEUP_RETRIES)
			return -EIO;

		for (i = 0; i < CS40L2X_WAKEUP_RETRIES; i++) {
			usleep_range(5000, 5100);

			ret = regmap_read(cs40l2x->regmap,
					cs40l2x_dsp_reg(cs40l2x, "POWERSTATE",
						CS40L2X_XM_UNPACKED_TYPE),
					&val);
			if (ret)
				return ret;

			if (val != CS40L2X_POWERSTATE_HIBERNATE)
				return 0;
		}
		return -ETIME;

	default:
		return -EINVAL;
	}
}

static ssize_t cs40l2x_hiber_cmd_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int hiber_cmd;

	/*
	 * hibernation is supported by revision B1 firmware only, and
	 * entry/exit is exposed to user space only for test purposes
	 */
	if (cs40l2x->revid < CS40L2X_REVID_B1)
		return -EPERM;

	ret = kstrtou32(buf, 10, &hiber_cmd);
	if (ret)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_hiber_cmd_send(cs40l2x, hiber_cmd);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to send hibernation command\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_hiber_timeout_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "FALSEI2CTIMEOUT",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read hibernation timeout\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_hiber_timeout_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val < CS40L2X_FALSEI2CTIMEOUT_MIN)
		return -EINVAL;

	if (val > CS40L2X_FALSEI2CTIMEOUT_MAX)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "FALSEI2CTIMEOUT",
				CS40L2X_XM_UNPACKED_TYPE), val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write hibernation timeout\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_gpio1_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GPIO_ENABLE",
					CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read GPIO1 configuration\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_gpio1_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GPIO_ENABLE",
					CS40L2X_XM_UNPACKED_TYPE),
			val ? CS40L2X_GPIO1_ENABLED : CS40L2X_GPIO1_DISABLED);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write GPIO1 configuration\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_gpio1_rise_index_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "INDEXBUTTONPRESS",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read index\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_gpio1_rise_index_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int index;

	ret = kstrtou32(buf, 10, &index);
	if (ret)
		return -EINVAL;

	if (index > (cs40l2x->num_waves - 1))
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s index:%u (num_waves:%u)\n", __func__, index, cs40l2x->num_waves);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "INDEXBUTTONPRESS",
				CS40L2X_XM_UNPACKED_TYPE), index);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write index\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_gpio1_fall_index_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "INDEXBUTTONRELEASE",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read index\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_gpio1_fall_index_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int index;

	ret = kstrtou32(buf, 10, &index);
	if (ret)
		return -EINVAL;

	if (index > (cs40l2x->num_waves - 1))
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s index:%u (num_waves:%u)\n", __func__, index, cs40l2x->num_waves);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "INDEXBUTTONRELEASE",
				CS40L2X_XM_UNPACKED_TYPE), index);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write index\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_gpio1_fall_timeout_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "PRESS_RELEASE_TIMEOUT",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read GPIO1 falling-edge timeout\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_gpio1_fall_timeout_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val > CS40L2X_PR_TIMEOUT_MAX)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "PRESS_RELEASE_TIMEOUT",
				CS40L2X_XM_UNPACKED_TYPE), val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write GPIO1 falling-edge timeout\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_standby_timeout_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "EVENT_TIMEOUT",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read standby timeout\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_standby_timeout_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val > CS40L2X_EVENT_TIMEOUT_MAX)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "EVENT_TIMEOUT",
				CS40L2X_XM_UNPACKED_TYPE), val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write standby timeout\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_f0_measured_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	unsigned char diag_state;
	unsigned int f0_measured;

	mutex_lock(&cs40l2x->lock);

	diag_state = cs40l2x->diag_state;
	f0_measured = cs40l2x->f0_measured;

	mutex_unlock(&cs40l2x->lock);

	if (diag_state != CS40L2X_DIAG_STATE_DONE)
		return -ENODATA;

	return snprintf(buf, PAGE_SIZE, "%d\n", f0_measured);
}

static ssize_t cs40l2x_f0_stored_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "F0_STORED",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read stored f0\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_f0_stored_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (cs40l2x->pdata.f0_min > 0 && val < cs40l2x->pdata.f0_min)
		return -EINVAL;

	if (cs40l2x->pdata.f0_max > 0 && val > cs40l2x->pdata.f0_max)
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s val:%u\n", __func__, val);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "F0_STORED",
				CS40L2X_XM_UNPACKED_TYPE), val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to store f0\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_redc_measured_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	unsigned char diag_state;
	unsigned int redc_measured;

	mutex_lock(&cs40l2x->lock);

	diag_state = cs40l2x->diag_state;
	redc_measured = cs40l2x->redc_measured;

	mutex_unlock(&cs40l2x->lock);

	if (diag_state != CS40L2X_DIAG_STATE_DONE)
		return -ENODATA;

	return snprintf(buf, PAGE_SIZE, "%d\n", redc_measured);
}

static ssize_t cs40l2x_redc_stored_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "REDC_STORED",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read stored ReDC\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_redc_stored_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (cs40l2x->pdata.redc_min > 0 && val < cs40l2x->pdata.redc_min)
		return -EINVAL;

	if (cs40l2x->pdata.redc_max > 0 && val > cs40l2x->pdata.redc_max)
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s val:%u\n", __func__, val);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "REDC_STORED",
				CS40L2X_XM_UNPACKED_TYPE), val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to store ReDC\n");
		return ret;
	}

	return count;
}

static ssize_t cs40l2x_comp_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "COMPENSATION_ENABLE",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read compensation state\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_comp_enable_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	ret = kstrtou32(buf, 10, &val);
	if (ret)
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s val:%u\n", __func__, val);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = regmap_write(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "COMPENSATION_ENABLE",
					CS40L2X_XM_UNPACKED_TYPE),
			val ? CS40L2X_COMP_ENABLED : CS40L2X_COMP_DISABLED);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write compensation state\n");
		return ret;
	}

	return count;
}

static int cs40l2x_dig_scale_get(struct cs40l2x_private *cs40l2x,
			unsigned int *dig_scale)
{
	int ret;
	unsigned int val;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	ret = regmap_read(cs40l2x->regmap, CS40L2X_AMP_DIG_VOL_CTRL, &val);
	if (ret)
		return ret;

	*dig_scale = (CS40L2X_DIG_SCALE_ZERO - ((val & CS40L2X_AMP_VOL_PCM_MASK)
			>> CS40L2X_AMP_VOL_PCM_SHIFT)) & CS40L2X_DIG_SCALE_MASK;

	return 0;
}

static int cs40l2x_dig_scale_set(struct cs40l2x_private *cs40l2x,
			unsigned int dig_scale)
{
	int ret;
	unsigned int val;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	if (dig_scale == CS40L2X_DIG_SCALE_RESET)
		return -EINVAL;

	ret = regmap_read(cs40l2x->regmap, CS40L2X_AMP_DIG_VOL_CTRL, &val);
	if (ret)
		return ret;

	val &= ~CS40L2X_AMP_VOL_PCM_MASK;
	val |= CS40L2X_AMP_VOL_PCM_MASK &
			(((CS40L2X_DIG_SCALE_ZERO - dig_scale)
			& CS40L2X_DIG_SCALE_MASK) << CS40L2X_AMP_VOL_PCM_SHIFT);

	ret = regmap_write(cs40l2x->regmap, CS40L2X_AMP_DIG_VOL_CTRL, val);
	if (ret)
		return ret;

	return cs40l2x_wseq_replace(cs40l2x, CS40L2X_AMP_DIG_VOL_CTRL, val);
}

#ifdef CIRRUS_VIB_DIG_SCALE_SUPPORT
static ssize_t cs40l2x_dig_scale_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int dig_scale;

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_dig_scale_get(cs40l2x, &dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read digital scale\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", dig_scale);
}

static ssize_t cs40l2x_dig_scale_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int dig_scale;

	ret = kstrtou32(buf, 10, &dig_scale);
	if (ret)
		return -EINVAL;

	if (dig_scale > CS40L2X_DIG_SCALE_MAX)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_dig_scale_set(cs40l2x, dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write digital scale\n");
		return ret;
	}

	return count;
}

static int cs40l2x_gpio1_dig_scale_get(struct cs40l2x_private *cs40l2x,
			unsigned int *dig_scale)
{
	int ret;
	unsigned int val;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GAIN_CONTROL",
					CS40L2X_XM_UNPACKED_TYPE), &val);
	if (ret)
		return ret;

	*dig_scale = (val & CS40L2X_GAIN_CTRL_GPIO_MASK)
			>> CS40L2X_GAIN_CTRL_GPIO_SHIFT;

	return 0;
}

static int cs40l2x_gpio1_dig_scale_set(struct cs40l2x_private *cs40l2x,
			unsigned int dig_scale)
{
	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	if (dig_scale == CS40L2X_DIG_SCALE_RESET)
		return -EINVAL;

	return regmap_update_bits(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GAIN_CONTROL",
					CS40L2X_XM_UNPACKED_TYPE),
			CS40L2X_GAIN_CTRL_GPIO_MASK,
			dig_scale << CS40L2X_GAIN_CTRL_GPIO_SHIFT);
}

static ssize_t cs40l2x_gpio1_dig_scale_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int dig_scale;

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_gpio1_dig_scale_get(cs40l2x, &dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read digital scale\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", dig_scale);
}

static ssize_t cs40l2x_gpio1_dig_scale_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int dig_scale;

	ret = kstrtou32(buf, 10, &dig_scale);
	if (ret)
		return -EINVAL;

	if (dig_scale > CS40L2X_DIG_SCALE_MAX)
		return -EINVAL;

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_gpio1_dig_scale_set(cs40l2x, dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write digital scale\n");
		return ret;
	}

	return count;
}
#endif

static int cs40l2x_cp_dig_scale_get(struct cs40l2x_private *cs40l2x,
			unsigned int *dig_scale)
{
	int ret;
	unsigned int val;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GAIN_CONTROL",
					CS40L2X_XM_UNPACKED_TYPE), &val);
	if (ret)
		return ret;

	*dig_scale = (val & CS40L2X_GAIN_CTRL_TRIG_MASK)
			>> CS40L2X_GAIN_CTRL_TRIG_SHIFT;

	return 0;
}

static int cs40l2x_cp_dig_scale_set(struct cs40l2x_private *cs40l2x,
			unsigned int dig_scale)
{
	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	if (dig_scale == CS40L2X_DIG_SCALE_RESET)
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s %u\n", __func__, dig_scale);
#endif

	return regmap_update_bits(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GAIN_CONTROL",
					CS40L2X_XM_UNPACKED_TYPE),
			CS40L2X_GAIN_CTRL_TRIG_MASK,
			dig_scale << CS40L2X_GAIN_CTRL_TRIG_SHIFT);
}

#ifdef CIRRUS_VIB_DIG_SCALE_SUPPORT
static ssize_t cs40l2x_cp_dig_scale_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int dig_scale;

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_cp_dig_scale_get(cs40l2x, &dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read digital scale\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", dig_scale);
}

static ssize_t cs40l2x_cp_dig_scale_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int dig_scale;

	ret = kstrtou32(buf, 10, &dig_scale);
	if (ret)
		return -EINVAL;

	if (dig_scale > CS40L2X_DIG_SCALE_MAX)
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s %u\n", __func__, dig_scale);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_cp_dig_scale_set(cs40l2x, dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write digital scale\n");
		return ret;
	}

	return count;
}
#endif

static ssize_t cs40l2x_heartbeat_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret;
	unsigned int val;

	mutex_lock(&cs40l2x->lock);
	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "HALO_HEARTBEAT",
				CS40L2X_XM_UNPACKED_TYPE), &val);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to read heartbeat\n");
		return ret;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t cs40l2x_num_waves_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);

	return snprintf(buf, PAGE_SIZE, "%d\n", cs40l2x->num_waves);
}

static ssize_t cs40l2x_intensity_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{

	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	return snprintf(buf, 20, "intensity: %u\n", cs40l2x->intensity);
}

static ssize_t cs40l2x_intensity_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret = 0;
	unsigned int intensity, dig_scale;

	ret = kstrtou32(buf, 10, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	if(intensity > CS40L2X_INTENSITY_SCALE_MAX)
		return -EINVAL;

 	dig_scale = cs40l2x_pbq_dig_scale[intensity/100];
#if DEBUG_VIB
	pr_info("%s %u (dig scale: %u)\n", __func__, intensity, dig_scale);
#endif

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_cp_dig_scale_set(cs40l2x, dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write digital scale\n");
		return ret;
	}

	cs40l2x->intensity = intensity;
	return count;
}

static ssize_t cs40l2x_haptic_engine_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t cs40l2x_haptic_engine_store(struct device *dev,
	struct device_attribute *devattr, const char *buf, size_t count)
{
	int _data = 0;

	if (sscanf(buf, "%6d", &_data) == 1)
		pr_info("%s, packet size : [%d] \n", __func__, _data);
	else
		pr_err("fail to get haptic_engine\n");

	return count;
}

/*
static int cs40l2x_gpio1_dig_scale_get(struct cs40l2x_private *cs40l2x,
			unsigned int *dig_scale)
{
	int ret;
	unsigned int val;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	ret = regmap_read(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GAIN_CONTROL",
					CS40L2X_XM_UNPACKED_TYPE), &val);
	if (ret)
		return ret;

	*dig_scale = (val & CS40L2X_GAIN_CTRL_GPIO_MASK)
			>> CS40L2X_GAIN_CTRL_GPIO_SHIFT;

	return 0;
}
*/

static int cs40l2x_gpio1_dig_scale_set(struct cs40l2x_private *cs40l2x,
			unsigned int dig_scale)
{
	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	if (dig_scale == CS40L2X_DIG_SCALE_RESET)
		return -EINVAL;

#if DEBUG_VIB
	pr_info("%s %u\n", __func__, dig_scale);
#endif

	return regmap_update_bits(cs40l2x->regmap,
			cs40l2x_dsp_reg(cs40l2x, "GAIN_CONTROL",
					CS40L2X_XM_UNPACKED_TYPE),
			CS40L2X_GAIN_CTRL_GPIO_MASK,
			dig_scale << CS40L2X_GAIN_CTRL_GPIO_SHIFT);
}


static ssize_t cs40l2x_force_touch_intensity_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	int ret = 0;
	unsigned int intensity, dig_scale;

	ret = kstrtou32(buf, 10, &intensity);
	if (ret) {
		pr_err("fail to get intensity\n");
		return -EINVAL;
	}

	if(intensity > CS40L2X_INTENSITY_SCALE_MAX)
		return -EINVAL;

 	dig_scale = cs40l2x_pbq_dig_scale[intensity/100];
	pr_info("%s %u (dig scale: %u)\n", __func__, intensity, dig_scale);

	mutex_lock(&cs40l2x->lock);
	ret = cs40l2x_gpio1_dig_scale_set(cs40l2x, dig_scale);
	mutex_unlock(&cs40l2x->lock);

	if (ret) {
		pr_err("Failed to write digital scale\n");
		return ret;
	}

	cs40l2x->force_touch_intensity = intensity;
	return count;
}

static ssize_t cs40l2x_force_touch_intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cs40l2x_private *cs40l2x = cs40l2x_get_private(dev);
	return snprintf(buf, 30, "force_touch_intensity: %u\n", cs40l2x->force_touch_intensity);
}

static DEVICE_ATTR(cp_trigger_index, 0660, cs40l2x_cp_trigger_index_show,
		cs40l2x_cp_trigger_index_store);
static DEVICE_ATTR(cp_trigger_queue, 0660, cs40l2x_cp_trigger_queue_show,
		cs40l2x_cp_trigger_queue_store);
static DEVICE_ATTR(hiber_cmd, 0660, NULL, cs40l2x_hiber_cmd_store);
static DEVICE_ATTR(hiber_timeout, 0660, cs40l2x_hiber_timeout_show,
		cs40l2x_hiber_timeout_store);
static DEVICE_ATTR(gpio1_enable, 0660, cs40l2x_gpio1_enable_show,
		cs40l2x_gpio1_enable_store);
static DEVICE_ATTR(gpio1_rise_index, 0660, cs40l2x_gpio1_rise_index_show,
		cs40l2x_gpio1_rise_index_store);
static DEVICE_ATTR(gpio1_fall_index, 0660, cs40l2x_gpio1_fall_index_show,
		cs40l2x_gpio1_fall_index_store);
static DEVICE_ATTR(gpio1_fall_timeout, 0660, cs40l2x_gpio1_fall_timeout_show,
		cs40l2x_gpio1_fall_timeout_store);
static DEVICE_ATTR(standby_timeout, 0660, cs40l2x_standby_timeout_show,
		cs40l2x_standby_timeout_store);
static DEVICE_ATTR(f0_measured, 0660, cs40l2x_f0_measured_show, NULL);
static DEVICE_ATTR(f0_stored, 0660, cs40l2x_f0_stored_show,
		cs40l2x_f0_stored_store);
static DEVICE_ATTR(redc_measured, 0660, cs40l2x_redc_measured_show, NULL);
static DEVICE_ATTR(redc_stored, 0660, cs40l2x_redc_stored_show,
		cs40l2x_redc_stored_store);
static DEVICE_ATTR(comp_enable, 0660, cs40l2x_comp_enable_show,
		cs40l2x_comp_enable_store);
#ifdef CIRRUS_VIB_DIG_SCALE_SUPPORT
static DEVICE_ATTR(dig_scale, 0660, cs40l2x_dig_scale_show,
		cs40l2x_dig_scale_store);
static DEVICE_ATTR(gpio1_dig_scale, 0660, cs40l2x_gpio1_dig_scale_show,
		cs40l2x_gpio1_dig_scale_store);
static DEVICE_ATTR(cp_dig_scale, 0660, cs40l2x_cp_dig_scale_show,
		cs40l2x_cp_dig_scale_store);
#endif
static DEVICE_ATTR(heartbeat, 0660, cs40l2x_heartbeat_show, NULL);
static DEVICE_ATTR(num_waves, 0660, cs40l2x_num_waves_show, NULL);
static DEVICE_ATTR(intensity, 0660, cs40l2x_intensity_show, cs40l2x_intensity_store);
static DEVICE_ATTR(haptic_engine, 0660, cs40l2x_haptic_engine_show, cs40l2x_haptic_engine_store);
static DEVICE_ATTR(force_touch_intensity, 0660, cs40l2x_force_touch_intensity_show,
		cs40l2x_force_touch_intensity_store);

static struct attribute *cs40l2x_dev_attrs[] = {
	&dev_attr_cp_trigger_index.attr,
	&dev_attr_cp_trigger_queue.attr,
	&dev_attr_hiber_cmd.attr,
	&dev_attr_hiber_timeout.attr,
	&dev_attr_gpio1_enable.attr,
	&dev_attr_gpio1_rise_index.attr,
	&dev_attr_gpio1_fall_index.attr,
	&dev_attr_gpio1_fall_timeout.attr,
	&dev_attr_standby_timeout.attr,
	&dev_attr_f0_measured.attr,
	&dev_attr_f0_stored.attr,
	&dev_attr_redc_measured.attr,
	&dev_attr_redc_stored.attr,
	&dev_attr_comp_enable.attr,
#ifdef CIRRUS_VIB_DIG_SCALE_SUPPORT
	&dev_attr_dig_scale.attr,
	&dev_attr_gpio1_dig_scale.attr,
	&dev_attr_cp_dig_scale.attr,
#endif
	&dev_attr_heartbeat.attr,
	&dev_attr_num_waves.attr,
	&dev_attr_intensity.attr,
	&dev_attr_haptic_engine.attr,
	&dev_attr_force_touch_intensity.attr,
	NULL,
};

static struct attribute_group cs40l2x_dev_attr_group = {
	.attrs = cs40l2x_dev_attrs,
};

static int cs40l2x_stop_playback(struct cs40l2x_private *cs40l2x)
{
	int ret, i;

	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	for (i = 0; i < CS40L2X_ENDPLAYBACK_RETRIES; i++) {
		ret = regmap_write(cs40l2x->regmap,
				cs40l2x_dsp_reg(cs40l2x, "ENDPLAYBACK",
						CS40L2X_XM_UNPACKED_TYPE),
				CS40L2X_ENDPLAYBACK_REQ);
		if (!ret)
			return 0;

		usleep_range(10000, 10100);
	}

	dev_err(cs40l2x->dev, "Parking device in reset\n");
	gpiod_set_value_cansleep(cs40l2x->reset_gpio, 0);

	return -EIO;
}

static int cs40l2x_pbq_cancel(struct cs40l2x_private *cs40l2x)
{
	int ret;

	/* this function expects to be called from a locked worker function */
	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	hrtimer_cancel(&cs40l2x->pbq_timer);

	ret = cs40l2x_stop_playback(cs40l2x);
	if (ret)
		return ret;

	ret = cs40l2x_cp_dig_scale_set(cs40l2x, cs40l2x->pbq_cp_dig_scale);
	if (ret)
		return ret;

	cs40l2x->pbq_state = CS40L2X_PBQ_STATE_IDLE;

	return 0;
}

static int cs40l2x_pbq_pair_launch(struct cs40l2x_private *cs40l2x)
{
	unsigned int tag, mag, cp_dig_scale;
	int ret, i;

	/* this function expects to be called from a locked worker function */
	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	do {
		/* restart queue as necessary */
		if (cs40l2x->pbq_index == cs40l2x->pbq_depth) {
			cs40l2x->pbq_index = 0;
			for (i = 0; i < cs40l2x->pbq_depth; i++)
				cs40l2x->pbq_pairs[i].remain =
						cs40l2x->pbq_pairs[i].repeat;

			switch (cs40l2x->pbq_remain) {
			case -1:
				/* loop until stopped */
				break;
			case 0:
				/* queue is finished */
				cs40l2x->cp_trailer_index = 0;
				cs40l2x->pbq_state = CS40L2X_PBQ_STATE_IDLE;

				ret = cs40l2x_cp_dig_scale_set(cs40l2x,
						cs40l2x->pbq_cp_dig_scale);
				return ret;
			default:
				/* loop once more */
				cs40l2x->pbq_remain--;
			}
		}

		tag = cs40l2x->pbq_pairs[cs40l2x->pbq_index].tag;
		mag = cs40l2x->pbq_pairs[cs40l2x->pbq_index].mag;

		switch (tag) {
		case CS40L2X_PBQ_TAG_SILENCE:
			hrtimer_start(&cs40l2x->pbq_timer,
					ktime_set(mag / 1000,
							(mag % 1000) * 1000000),
					HRTIMER_MODE_REL);
			cs40l2x->pbq_state = CS40L2X_PBQ_STATE_SILENT;
			cs40l2x->pbq_index++;
			break;
		case CS40L2X_PBQ_TAG_START:
			cs40l2x->pbq_index++;
			break;
		case CS40L2X_PBQ_TAG_STOP:
			if (cs40l2x->pbq_pairs[cs40l2x->pbq_index].remain) {
				cs40l2x->pbq_pairs[cs40l2x->pbq_index].remain--;
				cs40l2x->pbq_index = mag;
			} else {
				cs40l2x->pbq_index++;
			}
			break;
		default:
			cp_dig_scale = cs40l2x->pbq_cp_dig_scale
					+ cs40l2x_pbq_dig_scale[mag];
			if (cp_dig_scale > CS40L2X_DIG_SCALE_MAX)
				cp_dig_scale = CS40L2X_DIG_SCALE_MAX;

			ret = cs40l2x_cp_dig_scale_set(cs40l2x, cp_dig_scale);
			if (ret)
				return ret;

			ret = regmap_write(cs40l2x->regmap,
					CS40L2X_MBOX_TRIGGERINDEX, tag);
			if (ret)
				return ret;

			hrtimer_start(&cs40l2x->pbq_timer,
					ktime_set(0, CS40L2X_PBQ_POLL_NS),
					HRTIMER_MODE_REL);

			cs40l2x->pbq_state = CS40L2X_PBQ_STATE_PLAYING;
			cs40l2x->pbq_index++;
		}

	} while (tag == CS40L2X_PBQ_TAG_START || tag == CS40L2X_PBQ_TAG_STOP);

	return 0;
}

static void cs40l2x_vibe_pbq_worker(struct work_struct *work)
{
	struct cs40l2x_private *cs40l2x =
		container_of(work, struct cs40l2x_private, vibe_pbq_work);
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	unsigned int val;
	int ret;

	mutex_lock(&cs40l2x->lock);

	switch (cs40l2x->pbq_state) {
	case CS40L2X_PBQ_STATE_IDLE:
		/* queue may have been canceled */
		break;

	case CS40L2X_PBQ_STATE_PLAYING:
		ret = regmap_read(regmap, cs40l2x_dsp_reg(cs40l2x, "STATUS",
				CS40L2X_XM_UNPACKED_TYPE), &val);
		if (ret) {
			dev_err(dev, "Failed to capture playback status\n");
			goto err_mutex;
		}

		if (val != CS40L2X_STATUS_IDLE) {
			hrtimer_start(&cs40l2x->pbq_timer,
					ktime_set(0, CS40L2X_PBQ_POLL_NS),
					HRTIMER_MODE_REL);
			goto err_mutex;
		}

		ret = cs40l2x_pbq_pair_launch(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to continue playback queue\n");
		break;

	case CS40L2X_PBQ_STATE_SILENT:
		ret = cs40l2x_pbq_pair_launch(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to continue playback queue\n");
		break;

	default:
		dev_err(dev, "Unexpected playback queue state: %d\n",
				cs40l2x->pbq_state);
	}

err_mutex:
	mutex_unlock(&cs40l2x->lock);
}

static enum hrtimer_restart cs40l2x_pbq_timer(struct hrtimer *timer)
{
	struct cs40l2x_private *cs40l2x =
		container_of(timer, struct cs40l2x_private, pbq_timer);

	queue_work(cs40l2x->vibe_workqueue, &cs40l2x->vibe_pbq_work);

	return HRTIMER_NORESTART;
}

static int cs40l2x_diag_capture(struct cs40l2x_private *cs40l2x)
{
	struct regmap *regmap = cs40l2x->regmap;
	int ret;

	/* this function expects to be called from a locked worker function */
	if (!mutex_is_locked(&cs40l2x->lock))
		return -EACCES;

	if (cs40l2x->diag_state != CS40L2X_DIAG_STATE_RUN)
		return -ENODATA;

	ret = regmap_read(regmap, cs40l2x_dsp_reg(cs40l2x, "F0",
			CS40L2X_XM_UNPACKED_TYPE), &cs40l2x->f0_measured);
	if (ret)
		return ret;

	ret = regmap_read(regmap, cs40l2x_dsp_reg(cs40l2x, "REDC",
			CS40L2X_XM_UNPACKED_TYPE), &cs40l2x->redc_measured);
	if (ret)
		return ret;

	cs40l2x->diag_state = CS40L2X_DIAG_STATE_DONE;

	return 0;
}

static void cs40l2x_vibe_start_worker(struct work_struct *work)
{
	struct cs40l2x_private *cs40l2x =
		container_of(work, struct cs40l2x_private, vibe_start_work);
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	int ret, i;

	mutex_lock(&cs40l2x->lock);

	/* handle interruption of special cases */
	switch (cs40l2x->cp_trailer_index) {
	case CS40L2X_INDEX_PBQ:
		ret = cs40l2x_pbq_cancel(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to cancel playback queue\n");
		break;

	case CS40L2X_INDEX_DIAG:
		cs40l2x->diag_state = CS40L2X_DIAG_STATE_INIT;
		goto err_mutex;
	}

	cs40l2x->cp_trailer_index = cs40l2x->cp_trigger_index;

	switch (cs40l2x->cp_trailer_index) {
	case CS40L2X_INDEX_PBQ:
		pr_info("%s: Playing playback queue\n", __func__);
		break;
		
	case CS40L2X_INDEX_VIBE:
	case CS40L2X_INDEX_CONT_MIN ... CS40L2X_INDEX_CONT_MAX:
	case CS40L2X_INDEX_DIAG:
		pm_stay_awake(dev);
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
		hrtimer_start(&cs40l2x->vibe_timer,
				ktime_set(cs40l2x->vibe_timeout / 1000,
						(cs40l2x->vibe_timeout % 1000)
						* 1000000),
				HRTIMER_MODE_REL);
#endif /* CONFIG_ANDROID_TIMED_OUTPUT */
	default :
		pr_info("%s: Playing index %u (intensity: %u)\n",
			__func__, cs40l2x->cp_trailer_index, cs40l2x->intensity);
	}

	switch (cs40l2x->cp_trailer_index) {
	case CS40L2X_INDEX_VIBE:
	case CS40L2X_INDEX_CONT_MIN ... CS40L2X_INDEX_CONT_MAX:
		ret = regmap_write(regmap, CS40L2X_MBOX_TRIGGER_MS,
				cs40l2x->cp_trailer_index & CS40L2X_INDEX_MASK);
		if (ret)
			dev_err(dev, "Failed to start playback\n");
		break;

	case CS40L2X_INDEX_CLICK_MIN ... CS40L2X_INDEX_CLICK_MAX:
		ret = regmap_write(regmap, CS40L2X_MBOX_TRIGGERINDEX,
				cs40l2x->cp_trailer_index);
		if (ret)
			dev_err(dev, "Failed to start playback\n");
		break;

	case CS40L2X_INDEX_PBQ:
		cs40l2x->pbq_cp_dig_scale = CS40L2X_DIG_SCALE_RESET;

		ret = cs40l2x_cp_dig_scale_get(cs40l2x,
				&cs40l2x->pbq_cp_dig_scale);
		if (ret) {
			dev_err(dev, "Failed to read digital scale\n");
			goto err_mutex;
		}

		cs40l2x->pbq_index = 0;
		cs40l2x->pbq_remain = cs40l2x->pbq_repeat;

		for (i = 0; i < cs40l2x->pbq_depth; i++)
			cs40l2x->pbq_pairs[i].remain =
					cs40l2x->pbq_pairs[i].repeat;

		ret = cs40l2x_pbq_pair_launch(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to launch playback queue\n");
		break;

	case CS40L2X_INDEX_DIAG:
		cs40l2x->diag_state = CS40L2X_DIAG_STATE_INIT;
		cs40l2x->diag_dig_scale = CS40L2X_DIG_SCALE_RESET;

		ret = cs40l2x_dig_scale_get(cs40l2x, &cs40l2x->diag_dig_scale);
		if (ret) {
			dev_err(dev, "Failed to read digital scale\n");
			goto err_mutex;
		}

		ret = cs40l2x_dig_scale_set(cs40l2x, 0);
		if (ret) {
			dev_err(dev, "Failed to reset digital scale\n");
			goto err_mutex;
		}

		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "CLOSED_LOOP",
					CS40L2X_XM_UNPACKED_TYPE), 0);
		if (ret) {
			dev_err(dev, "Failed to disable closed-loop mode\n");
			goto err_mutex;
		}

		ret = regmap_write(regmap, CS40L2X_MBOX_STIMULUS_MODE, 1);
		if (ret) {
			dev_err(dev, "Failed to enable stimulus mode\n");
			goto err_mutex;
		}

		msleep(CS40L2X_DIAG_STATE_DELAY_MS);

		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "CLOSED_LOOP",
					CS40L2X_XM_UNPACKED_TYPE), 1);
		if (ret) {
			dev_err(dev, "Failed to enable closed-loop mode\n");
			goto err_mutex;
		}
		cs40l2x->diag_state = CS40L2X_DIAG_STATE_RUN;

		break;

	default:
		dev_err(dev, "Invalid wavetable index\n");
	}

err_mutex:
	mutex_unlock(&cs40l2x->lock);
}

static void cs40l2x_vibe_stop_worker(struct work_struct *work)
{
	struct cs40l2x_private *cs40l2x =
		container_of(work, struct cs40l2x_private, vibe_stop_work);
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	int ret;

	mutex_lock(&cs40l2x->lock);

	switch (cs40l2x->cp_trailer_index) {
	case CS40L2X_INDEX_VIBE:
	case CS40L2X_INDEX_CONT_MIN ... CS40L2X_INDEX_CONT_MAX:
		ret = cs40l2x_stop_playback(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to stop playback\n");
		pm_relax(dev);
		break;

	case CS40L2X_INDEX_CLICK_MIN ... CS40L2X_INDEX_CLICK_MAX:
		ret = cs40l2x_stop_playback(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to stop playback\n");
		break;

	case CS40L2X_INDEX_PBQ:
		ret = cs40l2x_pbq_cancel(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to cancel playback queue\n");
		break;

	case CS40L2X_INDEX_DIAG:
		ret = cs40l2x_diag_capture(cs40l2x);
		if (ret)
			dev_err(dev, "Failed to capture f0 and ReDC\n");

		ret = regmap_write(regmap, CS40L2X_MBOX_STIMULUS_MODE, 0);
		if (ret)
			dev_err(dev, "Failed to disable stimulus mode\n");

		ret = cs40l2x_dig_scale_set(cs40l2x, cs40l2x->diag_dig_scale);
		if (ret)
			dev_err(dev, "Failed to restore digital scale\n");
		pm_relax(dev);
		break;

	default:
		dev_err(dev, "Invalid wavetable index\n");
	}

	cs40l2x->cp_trailer_index = 0;

	mutex_unlock(&cs40l2x->lock);
}

#ifdef CONFIG_ANDROID_TIMED_OUTPUT
/* vibration callback for timed output device */
static void cs40l2x_vibe_enable(struct timed_output_dev *sdev, int timeout)
{
	struct cs40l2x_private *cs40l2x =
		container_of(sdev, struct cs40l2x_private, timed_dev);

	pr_info("%s %dms\n", __func__, timeout);

	if (cs40l2x->vibe_init_success) {
		if (timeout > 0) {
			cs40l2x->vibe_timeout = timeout;
			queue_work(cs40l2x->vibe_workqueue, &cs40l2x->vibe_start_work);
		} else {
			hrtimer_cancel(&cs40l2x->vibe_timer);
			queue_work(cs40l2x->vibe_workqueue, &cs40l2x->vibe_stop_work);
		}
	} else {
		pr_info("%s vibe init state? %d\n", __func__, cs40l2x->vibe_init_success);
	}

}

static int cs40l2x_vibe_get_time(struct timed_output_dev *sdev)
{
	struct cs40l2x_private *cs40l2x =
		container_of(sdev, struct cs40l2x_private, timed_dev);
	int ret = 0;

	if (hrtimer_active(&cs40l2x->vibe_timer))
		ret = ktime_to_ms(hrtimer_get_remaining(&cs40l2x->vibe_timer));

	return ret;
}

static enum hrtimer_restart cs40l2x_vibe_timer(struct hrtimer *timer)
{
	struct cs40l2x_private *cs40l2x =
		container_of(timer, struct cs40l2x_private, vibe_timer);

	queue_work(cs40l2x->vibe_workqueue, &cs40l2x->vibe_stop_work);

	return HRTIMER_NORESTART;
}

static void cs40l2x_create_timed_output(struct cs40l2x_private *cs40l2x)
{
	int ret;
	struct timed_output_dev *timed_dev = &cs40l2x->timed_dev;
	struct hrtimer *vibe_timer = &cs40l2x->vibe_timer;
	struct device *dev = cs40l2x->dev;

	timed_dev->name = CS40L2X_DEVICE_NAME;
	timed_dev->enable = cs40l2x_vibe_enable;
	timed_dev->get_time = cs40l2x_vibe_get_time;

	ret = timed_output_dev_register(timed_dev);
	if (ret) {
		dev_err(dev, "Failed to register timed output device: %d\n",
			ret);
		return;
	}

	hrtimer_init(vibe_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vibe_timer->function = cs40l2x_vibe_timer;

	ret = sysfs_create_group(&cs40l2x->timed_dev.dev->kobj,
			&cs40l2x_dev_attr_group);
	if (ret) {
		dev_err(dev, "Failed to create sysfs group: %d\n", ret);
		return;
	}
}
#else
/* vibration callback for LED device */
static void cs40l2x_vibe_brightness_set(struct led_classdev *led_cdev,
		enum led_brightness brightness)
{
	struct cs40l2x_private *cs40l2x =
		container_of(led_cdev, struct cs40l2x_private, led_dev);

	switch (brightness) {
	case LED_OFF:
		queue_work(cs40l2x->vibe_workqueue, &cs40l2x->vibe_stop_work);
		break;
	default:
		queue_work(cs40l2x->vibe_workqueue, &cs40l2x->vibe_start_work);
	}
}

static void cs40l2x_create_led(struct cs40l2x_private *cs40l2x)
{
	int ret;
	struct led_classdev *led_dev = &cs40l2x->led_dev;
	struct device *dev = cs40l2x->dev;

	led_dev->name = CS40L2X_DEVICE_NAME;
	led_dev->max_brightness = LED_FULL;
	led_dev->brightness_set = cs40l2x_vibe_brightness_set;
	led_dev->default_trigger = "transient";

	ret = led_classdev_register(dev, led_dev);
	if (ret) {
		dev_err(dev, "Failed to register LED device: %d\n", ret);
		return;
	}

	ret = sysfs_create_group(&cs40l2x->dev->kobj, &cs40l2x_dev_attr_group);
	if (ret) {
		dev_err(dev, "Failed to create sysfs group: %d\n", ret);
		return;
	}
}
#endif /* CONFIG_ANDROID_TIMED_OUTPUT */

static void cs40l2x_vibe_init(struct cs40l2x_private *cs40l2x)
{
	struct hrtimer *pbq_timer = &cs40l2x->pbq_timer;
	int ret;

	cs40l2x->vibe_workqueue =
		alloc_ordered_workqueue("vibe_workqueue", WQ_HIGHPRI);
	if (!cs40l2x->vibe_workqueue) {
		dev_err(cs40l2x->dev, "Failed to allocate workqueue\n");
		return;
	}

	INIT_WORK(&cs40l2x->vibe_start_work, cs40l2x_vibe_start_worker);
	INIT_WORK(&cs40l2x->vibe_pbq_work, cs40l2x_vibe_pbq_worker);
	INIT_WORK(&cs40l2x->vibe_stop_work, cs40l2x_vibe_stop_worker);

	hrtimer_init(pbq_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pbq_timer->function = cs40l2x_pbq_timer;

	ret = device_init_wakeup(cs40l2x->dev, true);
	if (ret) {
		dev_err(cs40l2x->dev, "Failed to initialize wakeup source\n");
		return;
	}

	cs40l2x->vibe_init_success = true;
}

static void cs40l2x_dev_node_init(struct cs40l2x_private *cs40l2x)
{
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
	cs40l2x_create_timed_output(cs40l2x);
#else
	cs40l2x_create_led(cs40l2x);
#endif
}

static void cs40l2x_dev_node_remove(struct cs40l2x_private *cs40l2x)
{
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
		sysfs_remove_group(&cs40l2x->timed_dev.dev->kobj,
				&cs40l2x_dev_attr_group);

		timed_output_dev_unregister(&cs40l2x->timed_dev);
#else
		sysfs_remove_group(&cs40l2x->dev->kobj,
				&cs40l2x_dev_attr_group);

		led_classdev_unregister(&cs40l2x->led_dev);
#endif
}

static int cs40l2x_coeff_init(struct cs40l2x_private *cs40l2x)
{
	int ret, i;
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	struct cs40l2x_coeff_desc *coeff_desc;
	unsigned int val, num_algos, algo_id, algo_rev;
	unsigned int xm_base, xm_size, ym_base, ym_size;
	unsigned int reg = CS40L2X_XM_FW_ID;

	ret = regmap_read(regmap, CS40L2X_XM_NUM_ALGOS, &num_algos);
	if (ret) {
		dev_err(dev, "Failed to read number of algorithms\n");
		return ret;
	}

	if (num_algos > CS40L2X_NUM_ALGOS_MAX) {
		dev_err(dev, "Invalid number of algorithms\n");
		return -EINVAL;
	}

	/* add one extra iteration to account for system algorithm */
	for (i = 0; i < (num_algos + 1); i++) {
		ret = regmap_read(regmap,
				reg + CS40L2X_ALGO_ID_OFFSET, &algo_id);
		if (ret) {
			dev_err(dev, "Failed to read algo. %d ID\n", i);
			return ret;
		}

		/* discern firmware ID from system algorithm */
		if (i == 0 && algo_id != cs40l2x->fw_id_match) {
			dev_err(dev, "Invalid firmware ID: 0x%06X\n", algo_id);
			return -EINVAL;
		}

		ret = regmap_read(regmap,
				reg + CS40L2X_ALGO_REV_OFFSET, &algo_rev);
		if (ret) {
			dev_err(dev, "Failed to read algo. %d revision\n", i);
			return ret;
		}

		/* discern firmware revision from system algorithm */
		if (i == 0) {
			if (algo_rev < cs40l2x->fw_rev_min) {
				dev_err(dev,
					"Invalid firmware revision: %d.%d.%d\n",
					(algo_rev & 0xFF0000) >> 16,
					(algo_rev & 0xFF00) >> 8,
					algo_rev & 0xFF);
				return -EINVAL;
			}
			dev_info(dev, "Firmware revision %d.%d.%d\n",
					(algo_rev & 0xFF0000) >> 16,
					(algo_rev & 0xFF00) >> 8,
					algo_rev & 0xFF);
		}

		ret = regmap_read(regmap,
				reg + CS40L2X_ALGO_XM_BASE_OFFSET, &xm_base);
		if (ret) {
			dev_err(dev, "Failed to read algo. %d XM_BASE\n", i);
			return ret;
		}

		ret = regmap_read(regmap,
				reg + CS40L2X_ALGO_XM_SIZE_OFFSET, &xm_size);
		if (ret) {
			dev_err(dev, "Failed to read algo. %d XM_SIZE\n", i);
			return ret;
		}

		ret = regmap_read(regmap,
				reg + CS40L2X_ALGO_YM_BASE_OFFSET, &ym_base);
		if (ret) {
			dev_err(dev, "Failed to read algo. %d YM_BASE\n", i);
			return ret;
		}

		ret = regmap_read(regmap,
				reg + CS40L2X_ALGO_YM_SIZE_OFFSET, &ym_size);
		if (ret) {
			dev_err(dev, "Failed to read algo. %d YM_SIZE\n", i);
			return ret;
		}

		list_for_each_entry(coeff_desc,
			&cs40l2x->coeff_desc_head, list) {

			if (coeff_desc->parent_id != algo_id)
				continue;

			switch (coeff_desc->block_type) {
			case CS40L2X_XM_UNPACKED_TYPE:
				coeff_desc->reg = CS40L2X_DSP1_XMEM_UNPACK24_0
					+ xm_base * 4
					+ coeff_desc->block_offset * 4;
				if (!strcmp(coeff_desc->name, "WAVETABLE")) {
					cs40l2x->wt_limit_xm = (xm_size
						- coeff_desc->block_offset) * 4;
					cs40l2x->vibegen_id = algo_id;
					cs40l2x->vibegen_rev = algo_rev;
				}
				break;
			case CS40L2X_YM_UNPACKED_TYPE:
				coeff_desc->reg = CS40L2X_DSP1_YMEM_UNPACK24_0
					+ ym_base * 4
					+ coeff_desc->block_offset * 4;
				if (!strcmp(coeff_desc->name, "WAVETABLEYM"))
					cs40l2x->wt_limit_ym = (ym_size
						- coeff_desc->block_offset) * 4;
				break;
			}

			dev_dbg(dev, "Found control %s at 0x%08X\n",
				coeff_desc->name, coeff_desc->reg);
		}

		/* system algo. contains one extra register (num. algos.) */
		if (i)
			reg += CS40L2X_ALGO_ENTRY_SIZE;
		else
			reg += (CS40L2X_ALGO_ENTRY_SIZE + 4);
	}

	ret = regmap_read(regmap, reg, &val);
	if (ret) {
		dev_err(dev, "Failed to read list terminator\n");
		return ret;
	}

	if (val != CS40L2X_ALGO_LIST_TERM) {
		dev_err(dev, "Invalid list terminator: 0x%X\n", val);
		return -EINVAL;
	}

	dev_info(dev, "Maximum wavetable size: %d bytes (XM), %d bytes (YM)\n",
			cs40l2x->wt_limit_xm / 4 * 3,
			cs40l2x->wt_limit_ym / 4 * 3);

	return 0;
}

static void cs40l2x_dsp_start(struct cs40l2x_private *cs40l2x)
{
	int ret;
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	unsigned int val;
	int dsp_timeout = CS40L2X_DSP_TIMEOUT_COUNT;

	if (cs40l2x->pdata.gpio1_mode != CS40L2X_GPIO1_MODE_DEF_ON) {
		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "GPIO_ENABLE",
						CS40L2X_XM_UNPACKED_TYPE),
				CS40L2X_GPIO1_DISABLED);
		if (ret) {
			dev_err(dev, "Failed to pre-configure GPIO1\n");
			return;
		}
	}

	switch (cs40l2x->revid) {
	case CS40L2X_REVID_A0:
		ret = regmap_update_bits(regmap, CS40L2X_PWR_CTRL1,
				CS40L2X_GLOBAL_EN_MASK,
				1 << CS40L2X_GLOBAL_EN_SHIFT);
		if (ret) {
			dev_err(dev, "Failed to enable device\n");
			return;
		}

		ret = regmap_update_bits(regmap, CS40L2X_DSP1_CCM_CORE_CTRL,
				CS40L2X_DSP1_RESET_MASK |
				CS40L2X_DSP1_EN_MASK,
				(1 << CS40L2X_DSP1_RESET_SHIFT) |
				(1 << CS40L2X_DSP1_EN_SHIFT));
		if (ret) {
			dev_err(dev, "Failed to start DSP\n");
			return;
		}
		break;

	case CS40L2X_REVID_B1:
		ret = cs40l2x_wseq_init(cs40l2x);
		if (ret) {
			dev_err(dev, "Failed to initialize write sequencer\n");
			return;
		}

		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "EVENTCONTROL",
						CS40L2X_XM_UNPACKED_TYPE),
				cs40l2x->event_control);
		if (ret) {
			dev_err(dev, "Failed to configure event controls\n");
			return;
		}
		/* intentionally fall through */

	default:
		ret = regmap_update_bits(regmap, CS40L2X_PWRMGT_CTL,
				CS40L2X_MEM_RDY_MASK,
				1 << CS40L2X_MEM_RDY_SHIFT);
		if (ret) {
			dev_err(dev, "Failed to set memory ready flag\n");
			return;
		}

		ret = regmap_update_bits(regmap, CS40L2X_DSP1_CCM_CORE_CTRL,
				CS40L2X_DSP1_RESET_MASK,
				1 << CS40L2X_DSP1_RESET_SHIFT);
		if (ret) {
			dev_err(dev, "Failed to restart DSP\n");
			return;
		}
	}

	while (dsp_timeout > 0) {
		usleep_range(10000, 10100);

		ret = regmap_read(regmap, cs40l2x_dsp_reg(cs40l2x, "HALO_STATE",
				CS40L2X_XM_UNPACKED_TYPE), &val);
		if (ret) {
			dev_err(dev, "Failed to read DSP status\n");
			return;
		}

		if (val == CS40L2X_HALO_STATE_RUNNING)
			break;

		dsp_timeout--;
	}

	if (dsp_timeout == 0) {
		dev_err(dev, "Timed out with DSP status = %d\n", val);
		return;
	}

	ret = regmap_write(regmap, cs40l2x_dsp_reg(cs40l2x, "TIMEOUT_MS",
			CS40L2X_XM_UNPACKED_TYPE), CS40L2X_TIMEOUT_MS_MAX);
	if (ret) {
		dev_err(dev, "Failed to extend playback timeout\n");
		return;
	}

	ret = regmap_read(regmap, cs40l2x_dsp_reg(cs40l2x, "NUMBEROFWAVES",
			CS40L2X_XM_UNPACKED_TYPE), &cs40l2x->num_waves);
	if (ret) {
		dev_err(dev, "Failed to count wavetable entries\n");
		return;
	}

	if (cs40l2x->num_waves == 0) {
		dev_err(dev, "Wavetable is empty\n");
		return;
	}

	if (cs40l2x->pdata.f0_default) {
		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "F0_STORED",
						CS40L2X_XM_UNPACKED_TYPE),
				cs40l2x->pdata.f0_default);
		if (ret) {
			dev_err(dev, "Failed to write default f0\n");
			return;
		}
	}

	if (cs40l2x->pdata.redc_default) {
		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "REDC_STORED",
						CS40L2X_XM_UNPACKED_TYPE),
				cs40l2x->pdata.redc_default);
		if (ret) {
			dev_err(dev, "Failed to write default ReDC\n");
			return;
		}
	}

	if (cs40l2x->pdata.gpio1_rise_index > 0
			&& cs40l2x->pdata.gpio1_rise_index
				< cs40l2x->num_waves) {
		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "INDEXBUTTONPRESS",
						CS40L2X_XM_UNPACKED_TYPE),
				cs40l2x->pdata.gpio1_rise_index);
		if (ret) {
			dev_err(dev,
				"Failed to write default gpio1_rise_index\n");
			return;
		}
	} else if (cs40l2x->pdata.gpio1_rise_index >= cs40l2x->num_waves) {
		dev_warn(dev, "Ignored default gpio1_rise_index\n");
	}

	if (cs40l2x->pdata.gpio1_fall_index > 0
			&& cs40l2x->pdata.gpio1_fall_index
				< cs40l2x->num_waves) {
		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x, "INDEXBUTTONRELEASE",
						CS40L2X_XM_UNPACKED_TYPE),
				cs40l2x->pdata.gpio1_fall_index);
		if (ret) {
			dev_err(dev,
				"Failed to write default gpio1_fall_index\n");
			return;
		}
	} else if (cs40l2x->pdata.gpio1_fall_index >= cs40l2x->num_waves) {
		dev_warn(dev, "Ignored default gpio1_fall_index\n");
	}

	if (cs40l2x->pdata.gpio1_fall_timeout > 0
			&& (cs40l2x->pdata.gpio1_fall_timeout
				& CS40L2X_PDATA_MASK)
					<= CS40L2X_PR_TIMEOUT_MAX) {
		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x,
						"PRESS_RELEASE_TIMEOUT",
						CS40L2X_XM_UNPACKED_TYPE),
				cs40l2x->pdata.gpio1_fall_timeout
					& CS40L2X_PDATA_MASK);
		if (ret) {
			dev_err(dev,
				"Failed to write default gpio1_fall_timeout\n");
			return;
		}
	} else if ((cs40l2x->pdata.gpio1_fall_timeout
			& CS40L2X_PDATA_MASK) > CS40L2X_PR_TIMEOUT_MAX) {
		dev_warn(dev, "Ignored default gpio1_fall_timeout\n");
	}

	dev_info(dev, "Normal-mode haptics successfully started\n");

	cs40l2x_vibe_init(cs40l2x);
}

static int cs40l2x_raw_write(struct cs40l2x_private *cs40l2x, unsigned int reg,
		const void *val, size_t val_len, size_t limit)
{
	int ret, i;

	/* split "val" into smaller writes not to exceed "limit" in length */
	for (i = 0; i < val_len; i += limit) {
		ret = regmap_raw_write(cs40l2x->regmap, (reg + i), (val + i),
				(val_len - i) > limit ? limit : (val_len - i));
		if (ret)
			break;
	}

	return ret;
}

static void cs40l2x_waveform_load(const struct firmware *fw, void *context)
{
	int ret;
	struct cs40l2x_private *cs40l2x = (struct cs40l2x_private *)context;
	struct device *dev = cs40l2x->dev;
	unsigned int pos = CS40L2X_WT_FILE_HEADER_SIZE;
	unsigned int block_type, block_length;
	unsigned int algo_id, algo_rev;

	if (!fw)
		goto skip_loading;

	if (memcmp(fw->data, "WMDR", 4)) {
		dev_err(dev, "Failed to recognize waveform file\n");
		goto err_rls_fw;
	}

	while (pos < fw->size) {

		/* block offset is not used here */
		pos += CS40L2X_WT_DBLK_OFFSET_SIZE;

		block_type = fw->data[pos]
				+ (fw->data[pos + 1] << 8);
		pos += CS40L2X_WT_DBLK_TYPE_SIZE;

		algo_id = fw->data[pos]
				+ (fw->data[pos + 1] << 8)
				+ (fw->data[pos + 2] << 16)
				+ (fw->data[pos + 3] << 24);
		pos += CS40L2X_WT_ALGO_ID_SIZE;

		algo_rev = fw->data[pos]
				+ (fw->data[pos + 1] << 8)
				+ (fw->data[pos + 2] << 16)
				+ (fw->data[pos + 3] << 24);
		pos += CS40L2X_WT_ALGO_REV_SIZE;

		/* sample rate is not used here */
		pos += CS40L2X_WT_SAMPLE_RATE_SIZE;

		block_length = fw->data[pos]
				+ (fw->data[pos + 1] << 8)
				+ (fw->data[pos + 2] << 16)
				+ (fw->data[pos + 3] << 24);
		pos += CS40L2X_WT_DBLK_LENGTH_SIZE;

		switch (block_type) {
		case CS40L2X_XM_UNPACKED_TYPE:
			if (algo_id != cs40l2x->vibegen_id) {
				dev_err(dev, "Invalid algo. ID: 0x%06X\n",
					algo_id);
				goto err_rls_fw;
			}

			if (((algo_rev >> 8) & CS40L2X_ALGO_REV_MASK)
					!= (cs40l2x->vibegen_rev
						& CS40L2X_ALGO_REV_MASK)) {
				dev_err(dev, "Invalid algo. rev.: %d.%d.%d\n",
					(algo_rev & 0xFF000000) >> 24,
					(algo_rev & 0xFF0000) >> 16,
					(algo_rev & 0xFF00) >> 8);
				goto err_rls_fw;
			}

			if (block_length > cs40l2x->wt_limit_xm) {
				dev_err(dev,
					"Wavetable too large: %d bytes (XM)\n",
					block_length / 4 * 3);
				goto err_rls_fw;
			}

			ret = cs40l2x_raw_write(cs40l2x,
					cs40l2x_dsp_reg(cs40l2x, "WAVETABLE",
						CS40L2X_XM_UNPACKED_TYPE),
					&fw->data[pos], block_length,
					CS40L2X_MAX_WLEN);
			if (ret) {
				dev_err(dev,
					"Failed to write XM_UNPACKED memory\n");
				goto err_rls_fw;
			}
			break;
		case CS40L2X_YM_UNPACKED_TYPE:
			if (algo_id != cs40l2x->vibegen_id) {
				dev_err(dev, "Invalid algo. ID: 0x%06X\n",
					algo_id);
				goto err_rls_fw;
			}

			if (((algo_rev >> 8) & CS40L2X_ALGO_REV_MASK)
					!= (cs40l2x->vibegen_rev
						& CS40L2X_ALGO_REV_MASK)) {
				dev_err(dev, "Invalid algo. rev.: %d.%d.%d\n",
					(algo_rev & 0xFF000000) >> 24,
					(algo_rev & 0xFF0000) >> 16,
					(algo_rev & 0xFF00) >> 8);
				goto err_rls_fw;
			}

			if (block_length > cs40l2x->wt_limit_ym) {
				dev_err(dev,
					"Wavetable too large: %d bytes (YM)\n",
					block_length / 4 * 3);
				goto err_rls_fw;
			}

			ret = cs40l2x_raw_write(cs40l2x,
					cs40l2x_dsp_reg(cs40l2x, "WAVETABLEYM",
						CS40L2X_YM_UNPACKED_TYPE),
					&fw->data[pos], block_length,
					CS40L2X_MAX_WLEN);
			if (ret) {
				dev_err(dev,
					"Failed to write YM_UNPACKED memory\n");
				goto err_rls_fw;
			}
			break;
		}

		pos += block_length;
	}

skip_loading:
	cs40l2x_dsp_start(cs40l2x);
err_rls_fw:
	release_firmware(fw);
}

static int cs40l2x_algo_parse(struct cs40l2x_private *cs40l2x,
		const unsigned char *data)
{
	struct cs40l2x_coeff_desc *coeff_desc;
	unsigned int pos = 0;
	unsigned int algo_id, algo_desc_length, coeff_count;
	unsigned int block_offset, block_type, block_length;
	unsigned char algo_name_length;
	int i;

	/* record algorithm ID */
	algo_id = *(data + pos)
			+ (*(data + pos + 1) << 8)
			+ (*(data + pos + 2) << 16)
			+ (*(data + pos + 3) << 24);
	pos += CS40L2X_ALGO_ID_SIZE;

	/* skip past algorithm name */
	algo_name_length = *(data + pos);
	pos += ((algo_name_length / 4) * 4) + 4;

	/* skip past algorithm description */
	algo_desc_length = *(data + pos)
			+ (*(data + pos + 1) << 8);
	pos += ((algo_desc_length / 4) * 4) + 4;

	/* record coefficient count */
	coeff_count = *(data + pos)
			+ (*(data + pos + 1) << 8)
			+ (*(data + pos + 2) << 16)
			+ (*(data + pos + 3) << 24);
	pos += CS40L2X_COEFF_COUNT_SIZE;

	for (i = 0; i < coeff_count; i++) {
		block_offset = *(data + pos)
				+ (*(data + pos + 1) << 8);
		pos += CS40L2X_COEFF_OFFSET_SIZE;

		block_type = *(data + pos)
				+ (*(data + pos + 1) << 8);
		pos += CS40L2X_COEFF_TYPE_SIZE;

		block_length = *(data + pos)
				+ (*(data + pos + 1) << 8)
				+ (*(data + pos + 2) << 16)
				+ (*(data + pos + 3) << 24);
		pos += CS40L2X_COEFF_LENGTH_SIZE;

		coeff_desc = devm_kzalloc(cs40l2x->dev, sizeof(*coeff_desc),
				GFP_KERNEL);
		if (!coeff_desc)
			return -ENOMEM;

		coeff_desc->parent_id = algo_id;
		coeff_desc->block_offset = block_offset;
		coeff_desc->block_type = block_type;

		memcpy(coeff_desc->name, data + pos + 1, *(data + pos));
		coeff_desc->name[*(data + pos)] = '\0';

		list_add(&coeff_desc->list, &cs40l2x->coeff_desc_head);

		pos += block_length;
	}

	return 0;
}

static void cs40l2x_firmware_load(const struct firmware *fw, void *context)
{
	int ret;
	struct cs40l2x_private *cs40l2x = (struct cs40l2x_private *)context;
	struct device *dev = cs40l2x->dev;
	unsigned int pos = CS40L2X_FW_FILE_HEADER_SIZE;
	unsigned int block_offset, block_length;
	unsigned char block_type;

	if (!fw) {
		dev_err(dev, "Failed to request firmware file\n");
		return;
	}

	if (memcmp(fw->data, "WMFW", 4)) {
		dev_err(dev, "Failed to recognize firmware file\n");
		goto err_rls_fw;
	}

	while (pos < fw->size) {

		block_offset = fw->data[pos]
				+ (fw->data[pos + 1] << 8)
				+ (fw->data[pos + 2] << 16);
		pos += CS40L2X_FW_DBLK_OFFSET_SIZE;

		block_type = fw->data[pos];
		pos += CS40L2X_FW_DBLK_TYPE_SIZE;

		block_length = fw->data[pos]
				+ (fw->data[pos + 1] << 8)
				+ (fw->data[pos + 2] << 16)
				+ (fw->data[pos + 3] << 24);
		pos += CS40L2X_FW_DBLK_LENGTH_SIZE;

		switch (block_type) {
		case CS40L2X_PM_PACKED_TYPE:
			ret = cs40l2x_raw_write(cs40l2x,
					CS40L2X_DSP1_PMEM_0
						+ block_offset * 5,
					&fw->data[pos], block_length,
					CS40L2X_MAX_WLEN);
			if (ret) {
				dev_err(dev,
					"Failed to write PM_PACKED memory\n");
				goto err_rls_fw;
			}
			break;
		case CS40L2X_XM_PACKED_TYPE:
			ret = cs40l2x_raw_write(cs40l2x,
					CS40L2X_DSP1_XMEM_PACK_0
						+ block_offset * 3,
					&fw->data[pos], block_length,
					CS40L2X_MAX_WLEN);
			if (ret) {
				dev_err(dev,
					"Failed to write XM_PACKED memory\n");
				goto err_rls_fw;
			}
			break;
		case CS40L2X_YM_PACKED_TYPE:
			ret = cs40l2x_raw_write(cs40l2x,
					CS40L2X_DSP1_YMEM_PACK_0
						+ block_offset * 3,
					&fw->data[pos], block_length,
					CS40L2X_MAX_WLEN);
			if (ret) {
				dev_err(dev,
					"Failed to write YM_PACKED memory\n");
				goto err_rls_fw;
			}
			break;
		case CS40L2X_ALGO_INFO_TYPE:
			ret = cs40l2x_algo_parse(cs40l2x, &fw->data[pos]);
			if (ret) {
				dev_err(dev,
					"Failed to parse algorithm: %d\n", ret);
				goto err_rls_fw;
			}
			break;
		}

		pos += block_length;
	}

	ret = cs40l2x_coeff_init(cs40l2x);
	if (ret)
		goto err_rls_fw;

	request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG, CS40L2X_WT_NAME,
			dev, GFP_KERNEL, cs40l2x, cs40l2x_waveform_load);
err_rls_fw:
	release_firmware(fw);
}

static int cs40l2x_boost_config(struct cs40l2x_private *cs40l2x,
		int boost_ind, int boost_cap, int boost_ipk)
{
	int ret;
	unsigned char bst_lbst_val, bst_cbst_range, bst_ipk_scaled;
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;

	switch (boost_ind) {
	case 1000:	/* 1.0 uH */
		bst_lbst_val = 0;
		break;
	case 1200:	/* 1.2 uH */
		bst_lbst_val = 1;
		break;
	case 1500:	/* 1.5 uH */
		bst_lbst_val = 2;
		break;
	case 2200:	/* 2.2 uH */
		bst_lbst_val = 3;
		break;
	default:
		dev_err(dev, "Invalid boost inductor value: %d nH\n",
				boost_ind);
		return -EINVAL;
	}

	switch (boost_cap) {
	case 0 ... 19:
		bst_cbst_range = 0;
		break;
	case 20 ... 50:
		bst_cbst_range = 1;
		break;
	case 51 ... 100:
		bst_cbst_range = 2;
		break;
	case 101 ... 200:
		bst_cbst_range = 3;
		break;
	default:	/* 201 uF and greater */
		bst_cbst_range = 4;
	}

	ret = regmap_update_bits(regmap, CS40L2X_BSTCVRT_COEFF,
			CS40L2X_BST_K1_MASK,
			cs40l2x_bst_k1_table[bst_lbst_val][bst_cbst_range]
				<< CS40L2X_BST_K1_SHIFT);
	if (ret) {
		dev_err(dev, "Failed to write boost K1 coefficient\n");
		return ret;
	}

	ret = regmap_update_bits(regmap, CS40L2X_BSTCVRT_COEFF,
			CS40L2X_BST_K2_MASK,
			cs40l2x_bst_k2_table[bst_lbst_val][bst_cbst_range]
				<< CS40L2X_BST_K2_SHIFT);
	if (ret) {
		dev_err(dev, "Failed to write boost K2 coefficient\n");
		return ret;
	}

	ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_BSTCVRT_COEFF,
			(cs40l2x_bst_k2_table[bst_lbst_val][bst_cbst_range]
				<< CS40L2X_BST_K2_SHIFT) |
			(cs40l2x_bst_k1_table[bst_lbst_val][bst_cbst_range]
				<< CS40L2X_BST_K1_SHIFT));
	if (ret) {
		dev_err(dev, "Failed to sequence boost K1/K2 coefficients\n");
		return ret;
	}

	ret = regmap_update_bits(regmap, CS40L2X_BSTCVRT_SLOPE_LBST,
			CS40L2X_BST_SLOPE_MASK,
			cs40l2x_bst_slope_table[bst_lbst_val]
				<< CS40L2X_BST_SLOPE_SHIFT);
	if (ret) {
		dev_err(dev, "Failed to write boost slope coefficient\n");
		return ret;
	}

	ret = regmap_update_bits(regmap, CS40L2X_BSTCVRT_SLOPE_LBST,
			CS40L2X_BST_LBST_VAL_MASK,
			bst_lbst_val << CS40L2X_BST_LBST_VAL_SHIFT);
	if (ret) {
		dev_err(dev, "Failed to write boost inductor value\n");
		return ret;
	}

	ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_BSTCVRT_SLOPE_LBST,
			(cs40l2x_bst_slope_table[bst_lbst_val]
				<< CS40L2X_BST_SLOPE_SHIFT) |
			(bst_lbst_val << CS40L2X_BST_LBST_VAL_SHIFT));
	if (ret) {
		dev_err(dev, "Failed to sequence boost inductor value\n");
		return ret;
	}

	if ((boost_ipk < 1600) || (boost_ipk > 4500)) {
		dev_err(dev, "Invalid boost inductor peak current: %d mA\n",
				boost_ipk);
		return -EINVAL;
	}
	bst_ipk_scaled = ((boost_ipk - 1600) / 50) + 0x10;

	ret = regmap_update_bits(regmap, CS40L2X_BSTCVRT_PEAK_CUR,
			CS40L2X_BST_IPK_MASK,
			bst_ipk_scaled << CS40L2X_BST_IPK_SHIFT);
	if (ret) {
		dev_err(dev, "Failed to write boost inductor peak current\n");
		return ret;
	}

	ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_BSTCVRT_PEAK_CUR,
			bst_ipk_scaled << CS40L2X_BST_IPK_SHIFT);
	if (ret) {
		dev_err(dev,
			"Failed to sequence boost inductor peak current\n");
		return ret;
	}

	return 0;
}

static const struct reg_sequence cs40l2x_mpu_config[] = {
	{CS40L2X_DSP1_MPU_LOCK_CONFIG,	CS40L2X_MPU_UNLOCK_CODE1},
	{CS40L2X_DSP1_MPU_LOCK_CONFIG,	CS40L2X_MPU_UNLOCK_CODE2},
	{CS40L2X_DSP1_MPU_XM_ACCESS0,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_YM_ACCESS0,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_WNDW_ACCESS0,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_XREG_ACCESS0,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_YREG_ACCESS0,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_WNDW_ACCESS1,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_XREG_ACCESS1,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_YREG_ACCESS1,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_WNDW_ACCESS2,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_XREG_ACCESS2,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_YREG_ACCESS2,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_WNDW_ACCESS3,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_XREG_ACCESS3,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_YREG_ACCESS3,	0xFFFFFFFF},
	{CS40L2X_DSP1_MPU_LOCK_CONFIG,	0x00000000}
};

static int cs40l2x_dsp_load(struct cs40l2x_private *cs40l2x)
{
	int ret;
	unsigned int revid = cs40l2x->revid;
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;

	switch (revid) {
	case CS40L2X_REVID_A0:
	case CS40L2X_REVID_B0:
		ret = regmap_multi_reg_write(regmap, cs40l2x_mpu_config,
				ARRAY_SIZE(cs40l2x_mpu_config));
		if (ret) {
			dev_err(dev, "Failed to configure MPU\n");
			return ret;
		}

		cs40l2x->fw_id_match = CS40L2X_FW_ID_MATCH_A0;
		cs40l2x->fw_rev_min = CS40L2X_FW_REV_MIN_A0;

		request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
				CS40L2X_FW_NAME_A0, dev, GFP_KERNEL, cs40l2x,
				cs40l2x_firmware_load);
		return 0;
	case CS40L2X_REVID_B1:
		cs40l2x->fw_id_match = CS40L2X_FW_ID_MATCH_B1;
		cs40l2x->fw_rev_min = CS40L2X_FW_REV_MIN_B1;

		request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG,
				CS40L2X_FW_NAME_B1, dev, GFP_KERNEL, cs40l2x,
				cs40l2x_firmware_load);
		return 0;
	default:
		dev_err(dev, "No firmware defined for revision %02X\n", revid);
		return -EINVAL;
	}
}

static const struct reg_sequence cs40l2x_pcm_routing[] = {
	{CS40L2X_DAC_PCM1_SRC,		CS40L2X_DAC_PCM1_SRC_DSP1TX1},
	{CS40L2X_DSP1_RX2_SRC,		CS40L2X_DSP1_RXn_SRC_VMON},
	{CS40L2X_DSP1_RX3_SRC,		CS40L2X_DSP1_RXn_SRC_IMON},
	{CS40L2X_DSP1_RX4_SRC,		CS40L2X_DSP1_RXn_SRC_VPMON},
};

static int cs40l2x_init(struct cs40l2x_private *cs40l2x)
{
	int ret;
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	unsigned int wksrc_en = CS40L2X_WKSRC_EN_SDA | CS40L2X_WKSRC_EN_GPIO1;

	/* REFCLK configuration is handled by revision B1 ROM */
	if (cs40l2x->pdata.refclk_gpio2 &&
			(cs40l2x->revid < CS40L2X_REVID_B1)) {
		ret = regmap_update_bits(regmap, CS40L2X_GPIO_PAD_CONTROL,
				CS40L2X_GP2_CTRL_MASK,
				CS40L2X_GPx_CTRL_MCLK
					<< CS40L2X_GP2_CTRL_SHIFT);
		if (ret) {
			dev_err(dev, "Failed to select GPIO2 function\n");
			return ret;
		}

		ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_GPIO_PAD_CONTROL,
				((CS40L2X_GPx_CTRL_MCLK
					<< CS40L2X_GP2_CTRL_SHIFT)
					& CS40L2X_GP2_CTRL_MASK) |
				((CS40L2X_GPx_CTRL_GPIO
					<< CS40L2X_GP1_CTRL_SHIFT)
					& CS40L2X_GP1_CTRL_MASK));
		if (ret) {
			dev_err(dev,
				"Failed to sequence GPIO1/2 configuration\n");
			return ret;
		}

		ret = regmap_update_bits(regmap, CS40L2X_PLL_CLK_CTRL,
				CS40L2X_PLL_REFCLK_SEL_MASK,
				CS40L2X_PLL_REFCLK_SEL_MCLK
					<< CS40L2X_PLL_REFCLK_SEL_SHIFT);
		if (ret) {
			dev_err(dev, "Failed to select clock source\n");
			return ret;
		}

		ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_PLL_CLK_CTRL,
				((1 << CS40L2X_PLL_REFCLK_EN_SHIFT)
					& CS40L2X_PLL_REFCLK_EN_MASK) |
				((CS40L2X_PLL_REFCLK_SEL_MCLK
					<< CS40L2X_PLL_REFCLK_SEL_SHIFT)
					& CS40L2X_PLL_REFCLK_SEL_MASK));
		if (ret) {
			dev_err(dev,
				"Failed to sequence PLL configuration\n");
			return ret;
		}
	}

	ret = cs40l2x_boost_config(cs40l2x, cs40l2x->pdata.boost_ind,
			cs40l2x->pdata.boost_cap, cs40l2x->pdata.boost_ipk);
	if (ret)
		return ret;

	ret = regmap_multi_reg_write(regmap, cs40l2x_pcm_routing,
			ARRAY_SIZE(cs40l2x_pcm_routing));
	if (ret) {
		dev_err(dev, "Failed to configure PCM channel routing\n");
		return ret;
	}

	ret = cs40l2x_wseq_add_seq(cs40l2x, cs40l2x_pcm_routing,
			ARRAY_SIZE(cs40l2x_pcm_routing));
	if (ret) {
		dev_err(dev, "Failed to sequence PCM channel routing\n");
		return ret;
	}

	ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_AMP_DIG_VOL_CTRL,
			(1 << CS40L2X_AMP_HPF_PCM_EN_SHIFT)
				& CS40L2X_AMP_HPF_PCM_EN_MASK);
	if (ret) {
		dev_err(dev, "Failed to sequence amplifier volume control\n");
		return ret;
	}

	/* hibernation is supported by revision B1 firmware only */
	if (cs40l2x->revid == CS40L2X_REVID_B1) {
		if (cs40l2x->devid == CS40L2X_DEVID_L25B)
			wksrc_en |= CS40L2X_WKSRC_EN_GPIO2;

		ret = regmap_update_bits(regmap,
				CS40L2X_WAKESRC_CTL,
				CS40L2X_WKSRC_EN_MASK,
				wksrc_en << CS40L2X_WKSRC_EN_SHIFT);
		if (ret) {
			dev_err(dev, "Failed to enable wake sources\n");
			return ret;
		}

		ret = cs40l2x_wseq_add_reg(cs40l2x, CS40L2X_WAKESRC_CTL,
				((wksrc_en << CS40L2X_WKSRC_EN_SHIFT)
					& CS40L2X_WKSRC_EN_MASK) |
						((CS40L2X_WKSRC_POL_SDA
						<< CS40L2X_WKSRC_POL_SHIFT)
						& CS40L2X_WKSRC_POL_MASK));
		if (ret) {
			dev_err(dev, "Failed to sequence wake sources\n");
			return ret;
		}
	}

	return 0;
}

static int cs40l2x_otp_unpack(struct cs40l2x_private *cs40l2x)
{
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	struct cs40l2x_trim trim;
	unsigned char row_offset, col_offset;
	unsigned int val, otp_map;
	unsigned int *otp_mem;
	int ret, i;

	otp_mem = kmalloc_array(CS40L2X_NUM_OTP_WORDS,
				sizeof(*otp_mem), GFP_KERNEL);
	if (!otp_mem)
		return -ENOMEM;

	ret = regmap_read(regmap, CS40L2X_OTPID, &val);
	if (ret) {
		dev_err(dev, "Failed to read OTP ID\n");
		goto err_otp_unpack;
	}

	/* hard matching against known OTP IDs */
	for (i = 0; i < CS40L2X_NUM_OTP_MAPS; i++) {
		if (cs40l2x_otp_map[i].id == val) {
			otp_map = i;
			break;
		}
	}

	/* reject unrecognized IDs, including untrimmed devices (OTP ID = 0) */
	if (i == CS40L2X_NUM_OTP_MAPS) {
		dev_err(dev, "Unrecognized OTP ID: 0x%01X\n", val);
		ret = -ENODEV;
		goto err_otp_unpack;
	}

	dev_dbg(dev, "Found OTP ID: 0x%01X\n", val);

	ret = regmap_bulk_read(regmap, CS40L2X_OTP_MEM0, otp_mem,
			CS40L2X_NUM_OTP_WORDS);
	if (ret) {
		dev_err(dev, "Failed to read OTP contents\n");
		goto err_otp_unpack;
	}

	ret = regmap_write(regmap, CS40L2X_TEST_KEY_CTL,
			CS40L2X_TEST_KEY_UNLOCK_CODE1);
	if (ret) {
		dev_err(dev, "Failed to unlock test space (step 1 of 2)\n");
		goto err_otp_unpack;
	}

	ret = regmap_write(regmap, CS40L2X_TEST_KEY_CTL,
			CS40L2X_TEST_KEY_UNLOCK_CODE2);
	if (ret) {
		dev_err(dev, "Failed to unlock test space (step 2 of 2)\n");
		goto err_otp_unpack;
	}

	row_offset = cs40l2x_otp_map[otp_map].row_start;
	col_offset = cs40l2x_otp_map[otp_map].col_start;

	for (i = 0; i < cs40l2x_otp_map[otp_map].num_trims; i++) {
		trim = cs40l2x_otp_map[otp_map].trim_table[i];

		if (col_offset + trim.size - 1 > 31) {
			/* trim straddles word boundary */
			val = (otp_mem[row_offset] &
					GENMASK(31, col_offset)) >> col_offset;
			val |= (otp_mem[row_offset + 1] &
					GENMASK(col_offset + trim.size - 33, 0))
					<< (32 - col_offset);
		} else {
			/* trim does not straddle word boundary */
			val = (otp_mem[row_offset] &
					GENMASK(col_offset + trim.size - 1,
						col_offset)) >> col_offset;
		}

		/* advance column marker and wrap if necessary */
		col_offset += trim.size;
		if (col_offset > 31) {
			col_offset -= 32;
			row_offset++;
		}

		/* skip blank trims */
		if (trim.reg == 0)
			continue;

		ret = regmap_update_bits(regmap, trim.reg,
				GENMASK(trim.shift + trim.size - 1, trim.shift),
				val << trim.shift);
		if (ret) {
			dev_err(dev, "Failed to write trim %d\n", i + 1);
			goto err_otp_unpack;
		}

		dev_dbg(dev, "Trim %d: wrote 0x%X to 0x%08X bits [%d:%d]\n",
				i + 1, val, trim.reg,
				trim.shift + trim.size - 1, trim.shift);
	}

	ret = regmap_write(regmap, CS40L2X_TEST_KEY_CTL,
			CS40L2X_TEST_KEY_RELOCK_CODE1);
	if (ret) {
		dev_err(dev, "Failed to lock test space (step 1 of 2)\n");
		goto err_otp_unpack;
	}

	ret = regmap_write(regmap, CS40L2X_TEST_KEY_CTL,
			CS40L2X_TEST_KEY_RELOCK_CODE2);
	if (ret) {
		dev_err(dev, "Failed to lock test space (step 2 of 2)\n");
		goto err_otp_unpack;
	}

	ret = 0;

err_otp_unpack:
	kfree(otp_mem);

	return ret;
}

static int cs40l2x_handle_of_data(struct i2c_client *i2c_client,
		struct cs40l2x_platform_data *pdata)
{
	struct device_node *np = i2c_client->dev.of_node;
	struct device *dev = &i2c_client->dev;
	int ret;
	unsigned int out_val;

	if (!np)
		return 0;

	ret = of_property_read_u32(np, "cirrus,boost-ind-nanohenry", &out_val);
	if (ret) {
		dev_err(dev, "Boost inductor value not specified\n");
		return -EINVAL;
	}
	pdata->boost_ind = out_val;

	ret = of_property_read_u32(np, "cirrus,boost-cap-microfarad", &out_val);
	if (ret) {
		dev_err(dev, "Boost capacitance not specified\n");
		return -EINVAL;
	}
	pdata->boost_cap = out_val;

	ret = of_property_read_u32(np, "cirrus,boost-ipk-milliamp", &out_val);
	if (ret) {
		dev_err(dev, "Boost inductor peak current not specified\n");
		return -EINVAL;
	}
	pdata->boost_ipk = out_val;

	pdata->refclk_gpio2 = of_property_read_bool(np, "cirrus,refclk-gpio2");

	ret = of_property_read_u32(np, "cirrus,f0-default", &out_val);
	if (!ret)
		pdata->f0_default = out_val;

	ret = of_property_read_u32(np, "cirrus,f0-min", &out_val);
	if (!ret)
		pdata->f0_min = out_val;

	ret = of_property_read_u32(np, "cirrus,f0-max", &out_val);
	if (!ret)
		pdata->f0_max = out_val;

	ret = of_property_read_u32(np, "cirrus,redc-default", &out_val);
	if (!ret)
		pdata->redc_default = out_val;

	ret = of_property_read_u32(np, "cirrus,redc-min", &out_val);
	if (!ret)
		pdata->redc_min = out_val;

	ret = of_property_read_u32(np, "cirrus,redc-max", &out_val);
	if (!ret)
		pdata->redc_max = out_val;

	ret = of_property_read_u32(np, "cirrus,gpio1-rise-index", &out_val);
	if (!ret)
		pdata->gpio1_rise_index = out_val;

	ret = of_property_read_u32(np, "cirrus,gpio1-fall-index", &out_val);
	if (!ret)
		pdata->gpio1_fall_index = out_val;

	ret = of_property_read_u32(np, "cirrus,gpio1-fall-timeout", &out_val);
	if (!ret)
		pdata->gpio1_fall_timeout = out_val | CS40L2X_PDATA_PRESENT;

	ret = of_property_read_u32(np, "cirrus,gpio1-mode", &out_val);
	if (!ret) {
		if (out_val > CS40L2X_GPIO1_MODE_MAX)
			dev_warn(dev, "Ignored default gpio1_mode\n");
		else
			pdata->gpio1_mode = out_val;
	}

	pdata->hiber_enable = of_property_read_bool(np, "cirrus,hiber-enable");

	return 0;
}

static int cs40l2x_basic_mode_exit(struct cs40l2x_private *cs40l2x)
{
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	unsigned int val;
	int ret, i;

	for (i = 0; i < CS40L2X_BASIC_TIMEOUT_COUNT; i++) {
		ret = regmap_read(regmap, CS40L2X_BASIC_AMP_STATUS, &val);
		if (ret) {
			dev_err(dev, "Failed to read basic-mode status\n");
			return ret;
		}

		if (val != CS40L2X_BASIC_NO_STATUS)
			break;

		usleep_range(10000, 10100);
	}

	if (val != CS40L2X_BASIC_BOOT_DONE) {
		dev_err(dev, "Unexpected basic-mode status: %02X\n", val);
		return -EIO;
	}

	ret = regmap_write(regmap, CS40L2X_BASIC_SHUTDOWNREQUEST, 1);
	if (ret) {
		dev_err(dev, "Failed to write shutdown request\n");
		return ret;
	}

	for (i = 0; i < CS40L2X_BASIC_TIMEOUT_COUNT; i++) {
		usleep_range(10000, 10100);

		ret = regmap_read(regmap, CS40L2X_BASIC_SHUTDOWNREQUEST, &val);
		if (ret) {
			dev_err(dev, "Failed to read shutdown request\n");
			return ret;
		}

		if (!val)
			break;
	}

	if (i == CS40L2X_BASIC_TIMEOUT_COUNT) {
		dev_err(dev, "Timed out waiting for basic-mode shutdown\n");
		return -ETIME;
	}

	ret = regmap_read(regmap, CS40L2X_BASIC_STATEMACHINE, &val);
	if (ret) {
		dev_err(dev, "Failed to read basic-mode state\n");
		return ret;
	}

	if (val != CS40L2X_BASIC_SHUTDOWN) {
		dev_err(dev, "Unexpected basic-mode state: %02X\n", val);
		return -EBUSY;
	}

	dev_info(dev, "Basic-mode haptics successfully stopped\n");

	return 0;
}

static const struct reg_sequence cs40l2x_rev_a0_errata[] = {
	{CS40L2X_OTP_TRIM_30,		0x9091A1C8},
	{CS40L2X_PLL_LOOP_PARAM,	0x000C1837},
	{CS40L2X_PLL_MISC_CTRL,		0x03008E0E},
	{CS40L2X_BSTCVRT_DCM_CTRL,	0x00000051},
	{CS40L2X_CTRL_ASYNC1,		0x00000004},
	{CS40L2X_IRQ1_DB3,		0x00000000},
	{CS40L2X_IRQ2_DB3,		0x00000000},
};

static const struct reg_sequence cs40l2x_rev_b0_errata[] = {
	{CS40L2X_PLL_LOOP_PARAM,	0x000C1837},
	{CS40L2X_PLL_MISC_CTRL,		0x03008E0E},
	{CS40L2X_TEST_KEY_CTL,		CS40L2X_TEST_KEY_UNLOCK_CODE1},
	{CS40L2X_TEST_KEY_CTL,		CS40L2X_TEST_KEY_UNLOCK_CODE2},
	{CS40L2X_OTP_TRIM_12,		0x002F0065},
	{CS40L2X_OTP_TRIM_13,		0x00002B4F},
	{CS40L2X_TEST_KEY_CTL,		CS40L2X_TEST_KEY_RELOCK_CODE1},
	{CS40L2X_TEST_KEY_CTL,		CS40L2X_TEST_KEY_RELOCK_CODE2},
};

static const struct reg_sequence cs40l2x_basic_mode_revert[] = {
	{CS40L2X_PWR_CTRL1,		0x00000000},
	{CS40L2X_PWR_CTRL2,		0x00003321},
	{CS40L2X_LRCK_PAD_CONTROL,	0x00000007},
	{CS40L2X_SDIN_PAD_CONTROL,	0x00000007},
	{CS40L2X_GPIO_PAD_CONTROL,	0x00000000},
	{CS40L2X_AMP_DIG_VOL_CTRL,	0x00008000},
	{CS40L2X_IRQ2_MASK1,		0xFFFFFFFF},
	{CS40L2X_IRQ2_MASK2,		0xFFFFFFFF},
};

static int cs40l2x_part_num_resolve(struct cs40l2x_private *cs40l2x)
{
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	unsigned int val, devid, revid;
	unsigned int part_num_index;
	int otp_timeout = CS40L2X_OTP_TIMEOUT_COUNT;
	int ret;

	while (otp_timeout > 0) {
		usleep_range(10000, 10100);

		ret = regmap_read(regmap, CS40L2X_IRQ1_STATUS4, &val);
		if (ret) {
			dev_err(dev, "Failed to read OTP boot status\n");
			return ret;
		}

		if (val & CS40L2X_OTP_BOOT_DONE)
			break;

		otp_timeout--;
	}

	if (otp_timeout == 0) {
		dev_err(dev, "Timed out waiting for OTP boot\n");
		return -ETIME;
	}

	ret = regmap_read(regmap, CS40L2X_IRQ1_STATUS3, &val);
	if (ret) {
		dev_err(dev, "Failed to read OTP error status\n");
		return ret;
	}

	if (val & CS40L2X_OTP_BOOT_ERR) {
		dev_err(dev, "Encountered fatal OTP error\n");
		return -EIO;
	}

	ret = regmap_read(regmap, CS40L2X_DEVID, &devid);
	if (ret) {
		dev_err(dev, "Failed to read device ID\n");
		return ret;
	}

	ret = regmap_read(regmap, CS40L2X_REVID, &revid);
	if (ret) {
		dev_err(dev, "Failed to read revision ID\n");
		return ret;
	}

	switch (devid) {
	case CS40L2X_DEVID_L20:
		part_num_index = 0;
		if (revid != CS40L2X_REVID_A0)
			goto err_revid;

		ret = regmap_register_patch(regmap, cs40l2x_rev_a0_errata,
				ARRAY_SIZE(cs40l2x_rev_a0_errata));
		if (ret) {
			dev_err(dev, "Failed to apply revision %02X errata\n",
					revid);
			return ret;
		}

		ret = cs40l2x_otp_unpack(cs40l2x);
		if (ret)
			return ret;
		break;
	case CS40L2X_DEVID_L25:
		part_num_index = 1;
		if (revid != CS40L2X_REVID_B0)
			goto err_revid;

		ret = regmap_register_patch(regmap, cs40l2x_rev_b0_errata,
				ARRAY_SIZE(cs40l2x_rev_b0_errata));
		if (ret) {
			dev_err(dev, "Failed to apply revision %02X errata\n",
					revid);
			return ret;
		}

		ret = cs40l2x_wseq_add_seq(cs40l2x, cs40l2x_rev_b0_errata,
				ARRAY_SIZE(cs40l2x_rev_b0_errata));
		if (ret) {
			dev_err(dev,
				"Failed to sequence revision %02X errata\n",
				revid);
			return ret;
		}
		break;
	case CS40L2X_DEVID_L25A:
	case CS40L2X_DEVID_L25B:
		part_num_index = devid - CS40L2X_DEVID_L25A + 2;
		if (revid < CS40L2X_REVID_B1)
			goto err_revid;

		ret = cs40l2x_basic_mode_exit(cs40l2x);
		if (ret)
			return ret;

		ret = regmap_multi_reg_write(regmap, cs40l2x_basic_mode_revert,
				ARRAY_SIZE(cs40l2x_basic_mode_revert));
		if (ret) {
			dev_err(dev, "Failed to revert basic-mode fields\n");
			return ret;
		}

		ret = regmap_register_patch(regmap, cs40l2x_rev_b0_errata,
				ARRAY_SIZE(cs40l2x_rev_b0_errata));
		if (ret) {
			dev_err(dev, "Failed to apply revision %02X errata\n",
					revid);
			return ret;
		}

		ret = cs40l2x_wseq_add_seq(cs40l2x, cs40l2x_rev_b0_errata,
				ARRAY_SIZE(cs40l2x_rev_b0_errata));
		if (ret) {
			dev_err(dev,
				"Failed to sequence revision %02X errata\n",
				revid);
			return ret;
		}
		break;
	default:
		dev_err(dev, "Unrecognized device ID: 0x%06X\n", devid);
		return -ENODEV;
	}

	dev_info(dev, "Cirrus Logic %s revision %02X\n",
			cs40l2x_part_nums[part_num_index], revid);
	cs40l2x->devid = devid;
	cs40l2x->revid = revid;

	return 0;
err_revid:
	dev_err(dev, "Unexpected revision ID for %s: %02X\n",
			cs40l2x_part_nums[part_num_index], revid);
	return -ENODEV;
}

static irqreturn_t cs40l2x_irq(int irq, void *data)
{
	struct cs40l2x_private *cs40l2x = (struct cs40l2x_private *)data;
	struct regmap *regmap = cs40l2x->regmap;
	struct device *dev = cs40l2x->dev;
	unsigned int val;
	int event_count = 0;
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(cs40l2x_event_regs); i++) {
		/* skip disabled event notifiers */
		if (!(cs40l2x->event_control & cs40l2x_event_masks[i]))
			continue;

		ret = regmap_read(regmap,
				cs40l2x_dsp_reg(cs40l2x,
						cs40l2x_event_regs[i],
						CS40L2X_XM_UNPACKED_TYPE),
				&val);
		if (ret) {
			dev_err(dev, "Failed to read %s\n",
					cs40l2x_event_regs[i]);
			continue;
		}

		/* any event handling goes here */
		switch (val) {
		case CS40L2X_EVENT_CTRL_NONE:
			continue;
		case CS40L2X_EVENT_CTRL_GPIO1_FALL
			... CS40L2X_EVENT_CTRL_HARDWARE:
			dev_dbg(dev, "Found notifier %d in %s\n",
					val, cs40l2x_event_regs[i]);
			break;
		default:
			dev_err(dev, "Unrecognized notifier %d in %s\n",
					val, cs40l2x_event_regs[i]);
		}

		ret = regmap_write(regmap,
				cs40l2x_dsp_reg(cs40l2x,
						cs40l2x_event_regs[i],
						CS40L2X_XM_UNPACKED_TYPE),
				CS40L2X_EVENT_CTRL_NONE);
		if (ret) {
			dev_err(dev, "Failed to acknowledge %s\n",
					cs40l2x_event_regs[i]);
			return IRQ_NONE;
		}

		ret = regmap_write(regmap, CS40L2X_MBOX_POWERCONTROL,
				CS40L2X_POWERCONTROL_WAKEUP);
		if (ret) {
			dev_err(dev, "Failed to free /ALERT output\n");
			return IRQ_NONE;
		}

		event_count++;
	}

	return event_count ? IRQ_HANDLED : IRQ_NONE;
}

static struct regmap_config cs40l2x_regmap = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
	.reg_format_endian = REGMAP_ENDIAN_BIG,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.max_register = CS40L2X_LASTREG,
	.precious_reg = cs40l2x_precious_reg,
	.readable_reg = cs40l2x_readable_reg,
	.cache_type = REGCACHE_NONE,
};

static int cs40l2x_i2c_probe(struct i2c_client *i2c_client,
				const struct i2c_device_id *id)
{
	int ret, i;
	struct cs40l2x_private *cs40l2x;
	struct device *dev = &i2c_client->dev;
	struct cs40l2x_platform_data *pdata = dev_get_platdata(dev);

	pr_info("%s\n", __func__ );
 
	cs40l2x = devm_kzalloc(dev, sizeof(struct cs40l2x_private), GFP_KERNEL);
	if (!cs40l2x)
		return -ENOMEM;

	cs40l2x->dev = dev;
	dev_set_drvdata(dev, cs40l2x);
	i2c_set_clientdata(i2c_client, cs40l2x);

	mutex_init(&cs40l2x->lock);

	INIT_LIST_HEAD(&cs40l2x->coeff_desc_head);

	cs40l2x->regmap = devm_regmap_init_i2c(i2c_client, &cs40l2x_regmap);
	if (IS_ERR(cs40l2x->regmap)) {
		ret = PTR_ERR(cs40l2x->regmap);
		dev_err(dev, "Failed to allocate register map: %d\n", ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(cs40l2x_supplies); i++)
		cs40l2x->supplies[i].supply = cs40l2x_supplies[i];

	cs40l2x->num_supplies = ARRAY_SIZE(cs40l2x_supplies);

	ret = devm_regulator_bulk_get(dev, cs40l2x->num_supplies,
			cs40l2x->supplies);
	if (ret) {
		dev_err(dev, "Failed to request core supplies: %d\n", ret);
		return ret;
	}

	if (pdata) {
		cs40l2x->pdata = *pdata;
	} else {
		pdata = devm_kzalloc(dev, sizeof(struct cs40l2x_platform_data),
				GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		if (i2c_client->dev.of_node) {
			ret = cs40l2x_handle_of_data(i2c_client, pdata);
			if (ret)
				return ret;

		}
		cs40l2x->pdata = *pdata;
	}

	ret = regulator_bulk_enable(cs40l2x->num_supplies, cs40l2x->supplies);
	if (ret) {
		dev_err(dev, "Failed to enable core supplies: %d\n", ret);
		return ret;
	}

	cs40l2x->reset_gpio = devm_gpiod_get_optional(dev, "reset",
			GPIOD_OUT_LOW);
	if (IS_ERR(cs40l2x->reset_gpio))
		return PTR_ERR(cs40l2x->reset_gpio);

	/* satisfy reset pulse width specification (with margin) */
	usleep_range(2000, 2100);

	gpiod_set_value_cansleep(cs40l2x->reset_gpio, 1);

	/* satisfy control port delay specification (with margin) */
	usleep_range(1000, 1100);

	ret = cs40l2x_part_num_resolve(cs40l2x);
	if (ret)
		goto err1;

	/* interrupts are supported by revision B1 firmware only */
	if (cs40l2x->revid == CS40L2X_REVID_B1) {
		if (i2c_client->irq) {
			ret = devm_request_threaded_irq(dev, i2c_client->irq,
					NULL, cs40l2x_irq,
					IRQF_ONESHOT | IRQF_TRIGGER_LOW
						| IRQF_SHARED,
					i2c_client->name, cs40l2x);
			if (ret) {
				dev_err(dev,
					"Failed to request IRQ: %d\n", ret);
				goto err1;
			}

			cs40l2x->event_control = CS40L2X_EVENT_GPIO1_ENABLED;
			if (cs40l2x->devid == CS40L2X_DEVID_L25B)
				cs40l2x->event_control |=
						CS40L2X_EVENT_GPIO2_ENABLED;
		} else {
			cs40l2x->event_control = CS40L2X_EVENT_DISABLED;
		}
	}

	cs40l2x_dev_node_init(cs40l2x);

	ret = cs40l2x_init(cs40l2x);
	if (ret)
		goto err2;

	ret = cs40l2x_dsp_load(cs40l2x);
	if (ret)
		goto err2;
	return 0;

err2:
	pr_info("%s err2\n", __func__ );
	cs40l2x_dev_node_remove(cs40l2x);
err1:
	pr_info("%s err1\n", __func__ );
	gpiod_set_value_cansleep(cs40l2x->reset_gpio, 0);
	regulator_bulk_disable(cs40l2x->num_supplies, cs40l2x->supplies);
	return ret;
}

static int cs40l2x_i2c_remove(struct i2c_client *i2c_client)
{
	struct cs40l2x_private *cs40l2x = i2c_get_clientdata(i2c_client);

	if (cs40l2x->vibe_init_success) {
#ifdef CONFIG_ANDROID_TIMED_OUTPUT
		hrtimer_cancel(&cs40l2x->vibe_timer);

		sysfs_remove_group(&cs40l2x->timed_dev.dev->kobj,
				&cs40l2x_dev_attr_group);

		timed_output_dev_unregister(&cs40l2x->timed_dev);
#else
		sysfs_remove_group(&cs40l2x->dev->kobj,
				&cs40l2x_dev_attr_group);

		led_classdev_unregister(&cs40l2x->led_dev);
#endif /* CONFIG_ANDROID_TIMED_OUTPUT */

		hrtimer_cancel(&cs40l2x->pbq_timer);

		cancel_work_sync(&cs40l2x->vibe_start_work);
		cancel_work_sync(&cs40l2x->vibe_pbq_work);
		cancel_work_sync(&cs40l2x->vibe_stop_work);

		destroy_workqueue(cs40l2x->vibe_workqueue);

		device_init_wakeup(cs40l2x->dev, false);
	}

	gpiod_set_value_cansleep(cs40l2x->reset_gpio, 0);

	regulator_bulk_disable(cs40l2x->num_supplies, cs40l2x->supplies);

	mutex_destroy(&cs40l2x->lock);

	return 0;
}

static int __maybe_unused cs40l2x_suspend(struct device *dev)
{
	struct cs40l2x_private *cs40l2x = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&cs40l2x->lock);

	if (cs40l2x->pdata.gpio1_mode == CS40L2X_GPIO1_MODE_AUTO) {
		ret = regmap_write(cs40l2x->regmap,
				cs40l2x_dsp_reg(cs40l2x, "GPIO_ENABLE",
						CS40L2X_XM_UNPACKED_TYPE),
				CS40L2X_GPIO1_ENABLED);
		if (ret) {
			dev_err(dev, "Failed to enable GPIO1 upon suspend\n");
			goto err_mutex;
		}
	}

	if (cs40l2x->pdata.hiber_enable && cs40l2x->revid == CS40L2X_REVID_B1) {
		ret = cs40l2x_hiber_cmd_send(cs40l2x,
				CS40L2X_POWERCONTROL_HIBERNATE);
		if (ret)
			dev_err(dev, "Failed to hibernate upon suspend\n");
	}

err_mutex:
	mutex_unlock(&cs40l2x->lock);

	return ret;
}

static int __maybe_unused cs40l2x_resume(struct device *dev)
{
	struct cs40l2x_private *cs40l2x = dev_get_drvdata(dev);
	int ret = 0;

	mutex_lock(&cs40l2x->lock);

	if (cs40l2x->pdata.gpio1_mode == CS40L2X_GPIO1_MODE_AUTO) {
		ret = regmap_write(cs40l2x->regmap,
				cs40l2x_dsp_reg(cs40l2x, "GPIO_ENABLE",
						CS40L2X_XM_UNPACKED_TYPE),
				CS40L2X_GPIO1_DISABLED);
		if (ret) {
			dev_err(dev, "Failed to disable GPIO1 upon resume\n");
			goto err_mutex;
		}
	}

	if (cs40l2x->pdata.hiber_enable && cs40l2x->revid == CS40L2X_REVID_B1) {
		ret = cs40l2x_hiber_cmd_send(cs40l2x,
				CS40L2X_POWERCONTROL_WAKEUP);
		if (ret)
			dev_err(dev, "Failed to wake up upon resume\n");
	}

err_mutex:
	mutex_unlock(&cs40l2x->lock);

	return ret;
}

static SIMPLE_DEV_PM_OPS(cs40l2x_pm_ops, cs40l2x_suspend, cs40l2x_resume);

static const struct of_device_id cs40l2x_of_match[] = {
	{ .compatible = "cirrus,cs40l20" },
	{ .compatible = "cirrus,cs40l25" },
	{ .compatible = "cirrus,cs40l25a" },
	{ .compatible = "cirrus,cs40l25b" },
	{ }
};

MODULE_DEVICE_TABLE(of, cs40l2x_of_match);

static const struct i2c_device_id cs40l2x_id[] = {
	{ "cs40l20", 0 },
	{ "cs40l25", 1 },
	{ "cs40l25a", 2 },
	{ "cs40l25b", 3 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, cs40l2x_id);

static struct i2c_driver cs40l2x_i2c_driver = {
	.driver = {
		.name = "cs40l2x",
		.of_match_table = cs40l2x_of_match,
		.pm = &cs40l2x_pm_ops,
	},
	.id_table = cs40l2x_id,
	.probe = cs40l2x_i2c_probe,
	.remove = cs40l2x_i2c_remove,
};

module_i2c_driver(cs40l2x_i2c_driver);

MODULE_DESCRIPTION("CS40L20/CS40L25/CS40L25A/CS40L25B Haptics Driver");
MODULE_AUTHOR("Jeff LaBundy, Cirrus Logic Inc, <jeff.labundy@cirrus.com>");
MODULE_LICENSE("GPL");
