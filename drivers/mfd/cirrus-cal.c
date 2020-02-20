/*
 * Calibration support for Cirrus Logic CS35L41 codec
 *
 * Copyright 2017 Cirrus Logic
 *
 * Author:	David Rhodes	<david.rhodes@cirrus.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <asm/io.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/power_supply.h>
#include <linux/fs.h>

#include <linux/mfd/cs35l41/core.h>
#include <linux/mfd/cs35l41/registers.h>
#include <linux/mfd/cs35l41/calibration.h>
#include <linux/mfd/cs35l41/wmfw.h>

#define CIRRUS_CAL_VERSION "5.00.6"

#define CIRRUS_CAL_DIR_NAME			"cirrus_cal"
#define CIRRUS_CAL_CONFIG_FILENAME_SUFFIX	"-dsp1-spk-prot-calib.bin"
#define CIRRUS_CAL_PLAYBACK_FILENAME_SUFFIX	"-dsp1-spk-prot.bin"
#define CIRRUS_CAL_RDC_LEFT_SAVE_LOCATION	"/efs/cirrus/rdc_cal"
#define CIRRUS_CAL_RDC_RIGHT_SAVE_LOCATION	"/efs/cirrus/rdc_cal_r"
#define CIRRUS_CAL_TEMP_SAVE_LOCATION		"/efs/cirrus/temp_cal"

#define CS35L41_CAL_COMPLETE_DELAY_MS	1250
#define CS34L40_CAL_AMBIENT_DEFAULT	23
#define CS34L40_CAL_RDC_DEFAULT		8580

struct cirrus_cal_t {
	struct class *cal_class;
	struct device *dev;
	struct regmap *regmap_right;
	struct regmap *regmap_left;
	const char *dsp_part_name_left;
	const char *dsp_part_name_right;
	bool cal_running;
	struct mutex lock;
	struct delayed_work cal_complete_work;
	int efs_cache_read_left;
	int efs_cache_read_right;
	unsigned int efs_cache_rdc_left;
	unsigned int efs_cache_rdc_right;
	unsigned int efs_cache_temp;
	unsigned int v_validation_left;
	unsigned int v_validation_right;
	unsigned int dsp_input1_cache_left;
	unsigned int dsp_input2_cache_left;
	unsigned int dsp_input1_cache_right;
	unsigned int dsp_input2_cache_right;
};

static struct cirrus_cal_t *cirrus_cal;

struct cs35l41_dsp_buf {
	struct list_head list;
	void *buf;
};


int cirrus_cal_amp_add(struct regmap *regmap_new, bool right_channel_amp,
					const char *dsp_part_name)
{
	if (cirrus_cal){
		if (right_channel_amp) {
			cirrus_cal->regmap_right = regmap_new;
			cirrus_cal->dsp_part_name_right = dsp_part_name;
		} else {
			cirrus_cal->regmap_left = regmap_new;
			cirrus_cal->dsp_part_name_left = dsp_part_name;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static struct cs35l41_dsp_buf *cs35l41_dsp_buf_alloc(const void *src, size_t len,
					     struct list_head *list)
{
	struct cs35l41_dsp_buf *buf = kzalloc(sizeof(*buf), GFP_KERNEL);

	if (buf == NULL)
		return NULL;

	buf->buf = vmalloc(len);
	if (!buf->buf) {
		kfree(buf);
		return NULL;
	}
	memcpy(buf->buf, src, len);

	if (list)
		list_add_tail(&buf->list, list);

	return buf;
}

static void cs35l41_dsp_buf_free(struct list_head *list)
{
	while (!list_empty(list)) {
		struct cs35l41_dsp_buf *buf = list_first_entry(list,
							   struct cs35l41_dsp_buf,
							   list);
		list_del(&buf->list);
		vfree(buf->buf);
		kfree(buf);
	}
}

static unsigned long long int cs35l41_rdc_to_ohms(unsigned long int rdc)
{
	return ((rdc * CS35L41_CAL_AMP_CONSTANT_NUM) /
		CS35L41_CAL_AMP_CONSTANT_DENOM);
}

static unsigned int cirrus_cal_vpk_to_mv(unsigned int vpk)
{
	return (vpk * CIRRUS_CAL_VFS_MV) >> 19;
}

static unsigned int cirrus_cal_ipk_to_ma(unsigned int ipk)
{
	return (ipk * CIRRUS_CAL_IFS_MA) >> 19;
}

static void cirrus_cal_unmute_dsp_inputs(void)
{
	regmap_write(cirrus_cal->regmap_left, CS35L41_DSP1_RX1_SRC,
			cirrus_cal->dsp_input1_cache_left);
	regmap_write(cirrus_cal->regmap_left, CS35L41_DSP1_RX2_SRC,
			cirrus_cal->dsp_input2_cache_left);
	regmap_write(cirrus_cal->regmap_right, CS35L41_DSP1_RX1_SRC,
			cirrus_cal->dsp_input1_cache_right);
	regmap_write(cirrus_cal->regmap_right, CS35L41_DSP1_RX2_SRC,
			cirrus_cal->dsp_input2_cache_right);
}

static void cirrus_cal_mute_dsp_inputs(void)
{
	unsigned int left1, left2, right1, right2;

	regmap_read(cirrus_cal->regmap_left, CS35L41_DSP1_RX1_SRC, &left1);
	if (left1) cirrus_cal->dsp_input1_cache_left = left1;
	regmap_read(cirrus_cal->regmap_left, CS35L41_DSP1_RX2_SRC, &left2);
	if (left2) cirrus_cal->dsp_input2_cache_left = left2;
	regmap_read(cirrus_cal->regmap_right, CS35L41_DSP1_RX1_SRC, &right1);
	if (right1) cirrus_cal->dsp_input1_cache_right = right1;
	regmap_read(cirrus_cal->regmap_right, CS35L41_DSP1_RX2_SRC, &right2);
	if (right2) cirrus_cal->dsp_input2_cache_right = right2;

	regmap_write(cirrus_cal->regmap_left, CS35L41_DSP1_RX1_SRC, 0);
	regmap_write(cirrus_cal->regmap_left, CS35L41_DSP1_RX2_SRC, 0);
	regmap_write(cirrus_cal->regmap_right, CS35L41_DSP1_RX1_SRC, 0);
	regmap_write(cirrus_cal->regmap_right, CS35L41_DSP1_RX2_SRC, 0);
}

static int cs35l41_load_config(const char *file, struct regmap *regmap)
{
	LIST_HEAD(buf_list);
	struct wmfw_coeff_hdr *hdr;
	struct wmfw_coeff_item *blk;
	const struct firmware *firmware;
	const char *region_name;
	int ret, pos, blocks, type, offset, reg;
	struct cs35l41_dsp_buf *buf;

	ret = request_firmware(&firmware, file, cirrus_cal->dev);

	if (ret != 0) {
		dev_err(cirrus_cal->dev, "Failed to request '%s'\n", file);
		ret = 0;
		goto out;
	}
	ret = -EINVAL;

	if (sizeof(*hdr) >= firmware->size) {
		dev_err(cirrus_cal->dev, "%s: file too short, %zu bytes\n",
			file, firmware->size);
		goto out_fw;
	}

	hdr = (void *)&firmware->data[0];
	if (memcmp(hdr->magic, "WMDR", 4) != 0) {
		dev_err(cirrus_cal->dev, "%s: invalid magic\n", file);
		goto out_fw;
	}

	switch (be32_to_cpu(hdr->rev) & 0xff) {
	case 1:
		break;
	default:
		dev_err(cirrus_cal->dev, "%s: Unsupported coefficient file format %d\n",
			 file, be32_to_cpu(hdr->rev) & 0xff);
		ret = -EINVAL;
		goto out_fw;
	}

	dev_dbg(cirrus_cal->dev, "%s: v%d.%d.%d\n", file,
		(le32_to_cpu(hdr->ver) >> 16) & 0xff,
		(le32_to_cpu(hdr->ver) >>  8) & 0xff,
		le32_to_cpu(hdr->ver) & 0xff);

	pos = le32_to_cpu(hdr->len);

	blocks = 0;
	while (pos < firmware->size &&
	       pos - firmware->size > sizeof(*blk)) {
		blk = (void *)(&firmware->data[pos]);

		type = le16_to_cpu(blk->type);
		offset = le16_to_cpu(blk->offset);

		dev_dbg(cirrus_cal->dev, "%s.%d: %x v%d.%d.%d\n",
			 file, blocks, le32_to_cpu(blk->id),
			 (le32_to_cpu(blk->ver) >> 16) & 0xff,
			 (le32_to_cpu(blk->ver) >>  8) & 0xff,
			 le32_to_cpu(blk->ver) & 0xff);
		dev_dbg(cirrus_cal->dev, "%s.%d: %d bytes at 0x%x in %x\n",
			 file, blocks, le32_to_cpu(blk->len), offset, type);

		reg = 0;
		region_name = "Unknown";
		switch (type) {
		case WMFW_ADSP2_YM:
			dev_dbg(cirrus_cal->dev, "%s.%d: %d bytes in %x for %x\n",
				 file, blocks, le32_to_cpu(blk->len),
				 type, le32_to_cpu(blk->id));

			if (le32_to_cpu(blk->id) == 0xcd) {
				reg = CS35L41_YM_CONFIG_ADDR;
				reg += offset - 0x8;
			}
			break;

		case WMFW_HALO_YM_PACKED:
			dev_dbg(cirrus_cal->dev, "%s.%d: %d bytes in %x for %x\n",
				 file, blocks, le32_to_cpu(blk->len),
				 type, le32_to_cpu(blk->id));

			if (le32_to_cpu(blk->id) == 0xcd) {
				/*     config addr packed + 1        */
				/* config size (config[0]) is not at 24bit packed boundary */
				/* so that fist word gets written by itself to unpacked mem */
				/* then the rest of it starts here */
				/* offset = 3 (groups of 4 24bit words) * 3 (packed words) * 4 bytes */
				reg = CS35L41_DSP1_YMEM_PACK_0 + 3 * 4 * 3;
			}
			break;

		default:
			dev_dbg(cirrus_cal->dev, "%s.%d: region type %x at %d\n",
				 file, blocks, type, pos);
			break;
		}

		if (reg) {
			if ((pos + le32_to_cpu(blk->len) + sizeof(*blk)) >
			    firmware->size) {
				dev_err(cirrus_cal->dev,
					 "%s.%d: %s region len %d bytes exceeds file length %zu\n",
					 file, blocks, region_name,
					 le32_to_cpu(blk->len),
					 firmware->size);
				ret = -EINVAL;
				goto out_fw;
			}

			buf = cs35l41_dsp_buf_alloc(blk->data,
						le32_to_cpu(blk->len),
						&buf_list);
			if (!buf) {
				dev_err(cirrus_cal->dev, "Out of memory\n");
				ret = -ENOMEM;
				goto out_fw;
			}

			dev_dbg(cirrus_cal->dev, "%s.%d: Writing %d bytes at %x\n",
				 file, blocks, le32_to_cpu(blk->len),
				 reg);
			ret = regmap_raw_write_async(regmap, reg, buf->buf,
						     le32_to_cpu(blk->len));
			if (ret != 0) {
				dev_err(cirrus_cal->dev,
					"%s.%d: Failed to write to %x in %s: %d\n",
					file, blocks, reg, region_name, ret);
			}
		}

		pos += (le32_to_cpu(blk->len) + sizeof(*blk) + 3) & ~0x03;
		blocks++;
	}

	ret = regmap_async_complete(regmap);
	if (ret != 0)
		dev_err(cirrus_cal->dev, "Failed to complete async write: %d\n", ret);

	if (pos > firmware->size)
		dev_err(cirrus_cal->dev, "%s.%d: %zu bytes at end of file\n",
			  file, blocks, pos - firmware->size);

	dev_info(cirrus_cal->dev, "%s load complete\n", file);

out_fw:
	regmap_async_complete(regmap);
	release_firmware(firmware);
	cs35l41_dsp_buf_free(&buf_list);
out:
	return ret;
}

static void cirrus_cal_complete_work(struct work_struct *work)
{
	int rdc_left, status_left, checksum_left, temp;
	int rdc_right, status_right, checksum_right;
	unsigned long long int ohms_left, ohms_right;
	unsigned int cal_state_left, cal_state_right;
	char *playback_config_filename_left;
	char *playback_config_filename_right;
	int timeout = 100;

	playback_config_filename_left = kzalloc(PAGE_SIZE, GFP_KERNEL);
	playback_config_filename_right = kzalloc(PAGE_SIZE, GFP_KERNEL);
	snprintf(playback_config_filename_left,
		PAGE_SIZE, "%s%s",
		cirrus_cal->dsp_part_name_left,
		CIRRUS_CAL_PLAYBACK_FILENAME_SUFFIX);
	snprintf(playback_config_filename_right,
		PAGE_SIZE, "%s%s",
		cirrus_cal->dsp_part_name_right,
		CIRRUS_CAL_PLAYBACK_FILENAME_SUFFIX);

	mutex_lock(&cirrus_cal->lock);

	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_STATUS, &status_left);
	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_RDC, &rdc_left);
	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, &temp);
	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_CHECKSUM, &checksum_left);
	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_STATUS, &status_right);
	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_RDC, &rdc_right);
	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_CHECKSUM, &checksum_right);

	ohms_left = cs35l41_rdc_to_ohms((unsigned long int)rdc_left);
	ohms_right = cs35l41_rdc_to_ohms((unsigned long int)rdc_right);

	regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE, &cal_state_left);
	if (cal_state_left == CS35L41_CSPL_STATE_ERROR) {
		dev_err(cirrus_cal->dev,
			"Error during calibration left, invalidating results\n");
		rdc_left = status_left = checksum_left = 0;
	}

	regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE, &cal_state_right);
	if (cal_state_right == CS35L41_CSPL_STATE_ERROR) {
		dev_err(cirrus_cal->dev,
			"Error during calibration right, invalidating results\n");
		rdc_right = status_right = checksum_right = 0;
	}

	dev_info(cirrus_cal->dev, "Calibration finished\n");
	dev_info(cirrus_cal->dev, "Duration:\t%d ms\n",
				    CS35L41_CAL_COMPLETE_DELAY_MS);
	dev_info(cirrus_cal->dev, "Status Left:\t%d\n", status_left);
	if (status_left == CS35L41_CSPL_STATUS_OUT_OF_RANGE)
		dev_err(cirrus_cal->dev, "Calibration left out of range\n");
	if (status_left == CS35L41_CSPL_STATUS_INCOMPLETE)
		dev_err(cirrus_cal->dev, "Calibration left incomplete\n");
	dev_info(cirrus_cal->dev, "Status Right:\t%d\n", status_right);
	if (status_right == CS35L41_CSPL_STATUS_OUT_OF_RANGE)
		dev_err(cirrus_cal->dev, "Calibration right out of range\n");
	if (status_right == CS35L41_CSPL_STATUS_INCOMPLETE)
		dev_err(cirrus_cal->dev, "Calibration right incomplete\n");
	dev_info(cirrus_cal->dev, "R Left:\t\t%d (%llu.%04llu Ohms)\n",
			rdc_left, ohms_left >> CS35L41_CAL_RDC_RADIX,
			    (ohms_left & (((1 << CS35L41_CAL_RDC_RADIX) - 1))) *
			    10000 / (1 << CS35L41_CAL_RDC_RADIX));
	dev_info(cirrus_cal->dev, "R Right:\t\t%d (%llu.%04llu Ohms)\n",
			rdc_right, ohms_right >> CS35L41_CAL_RDC_RADIX,
			    (ohms_right & (((1 << CS35L41_CAL_RDC_RADIX) - 1))) *
			    10000 / (1 << CS35L41_CAL_RDC_RADIX));
	dev_info(cirrus_cal->dev, "Checksum Left:\t%d\n", checksum_left);
	dev_info(cirrus_cal->dev, "Checksum Right:\t%d\n", checksum_right);
	dev_info(cirrus_cal->dev, "Ambient:\t%d\n", temp);

	usleep_range(5000, 5500);

	/* Send STOP_PRE_REINIT command and poll for response */
	regmap_write(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_STOP_PRE_REINIT);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_STOP_PRE_REINIT);
	timeout = 100;
	do {
		dev_info(cirrus_cal->dev, "waiting for REINIT ready...\n");
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_STS,
				&cal_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_STS,
				&cal_state_right);
	} while (((cal_state_left != CSPL_MBOX_STS_RDY_FOR_REINIT) ||
			(cal_state_right != CSPL_MBOX_STS_RDY_FOR_REINIT)) &&
			--timeout > 0);

	usleep_range(5000, 5500);

	cs35l41_load_config(playback_config_filename_left,
				cirrus_cal->regmap_left);
	cs35l41_load_config(playback_config_filename_right,
				cirrus_cal->regmap_right);

	regmap_update_bits(cirrus_cal->regmap_left,
		CS35L41_MIXER_NGATE_CH1_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	regmap_update_bits(cirrus_cal->regmap_left,
		CS35L41_MIXER_NGATE_CH2_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	regmap_update_bits(cirrus_cal->regmap_right,
		CS35L41_MIXER_NGATE_CH1_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	regmap_update_bits(cirrus_cal->regmap_right,
		CS35L41_MIXER_NGATE_CH2_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	dev_dbg(cirrus_cal->dev, "NOISE GATE ENABLE\n");

	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_STATUS, status_left);
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_RDC, rdc_left);
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, temp);
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_CHECKSUM, checksum_left);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_STATUS, status_right);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_RDC, rdc_right);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_AMBIENT, temp);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_CHECKSUM, checksum_right);


	/* Send REINIT command and poll for response */
	regmap_write(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_REINIT);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_REINIT);
	timeout = 100;
	do {
		dev_info(cirrus_cal->dev, "waiting for REINIT done...\n");
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_STS,
				&cal_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_STS,
				&cal_state_right);
	} while (((cal_state_left != CSPL_MBOX_STS_RUNNING) ||
			(cal_state_right != CSPL_MBOX_STS_RUNNING)) &&
			--timeout > 0);

	regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE, &cal_state_left);
	if (cal_state_left == CS35L41_CSPL_STATE_ERROR)
		dev_err(cirrus_cal->dev,
			"Playback config load error left\n");

	regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE, &cal_state_right);
	if (cal_state_right == CS35L41_CSPL_STATE_ERROR)
		dev_err(cirrus_cal->dev,
			"Playback config load error right\n");

	cirrus_cal_unmute_dsp_inputs();
	dev_dbg(cirrus_cal->dev, "DSP Inputs unmuted\n");

	dev_dbg(cirrus_cal->dev, "Calibration complete\n");
	cirrus_cal->cal_running = 0;
	cirrus_cal->efs_cache_read_left = 0;
	cirrus_cal->efs_cache_read_right = 0;
	mutex_unlock(&cirrus_cal->lock);
	kfree(playback_config_filename_left);
	kfree(playback_config_filename_right);
}

static void cirrus_cal_v_val_complete(void)
{
	char *playback_config_filename_left;
	char *playback_config_filename_right;
	unsigned int cal_state_left, cal_state_right;
	int timeout = 100;

	playback_config_filename_left = kzalloc(PAGE_SIZE, GFP_KERNEL);
	playback_config_filename_right = kzalloc(PAGE_SIZE, GFP_KERNEL);
	snprintf(playback_config_filename_left,
		PAGE_SIZE, "%s%s",
		cirrus_cal->dsp_part_name_left,
		CIRRUS_CAL_PLAYBACK_FILENAME_SUFFIX);
	snprintf(playback_config_filename_right,
		PAGE_SIZE, "%s%s",
		cirrus_cal->dsp_part_name_right,
		CIRRUS_CAL_PLAYBACK_FILENAME_SUFFIX);

	/* Send STOP_PRE_REINIT command and poll for response */
	regmap_write(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_STOP_PRE_REINIT);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_STOP_PRE_REINIT);
	timeout = 100;
	do {
		dev_info(cirrus_cal->dev, "waiting for REINIT ready...\n");
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_STS,
				&cal_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_STS,
				&cal_state_right);
	} while (((cal_state_left != CSPL_MBOX_STS_RDY_FOR_REINIT) ||
			(cal_state_right != CSPL_MBOX_STS_RDY_FOR_REINIT)) &&
			--timeout > 0);

	usleep_range(5000, 5500);

	cs35l41_load_config(playback_config_filename_left,
				cirrus_cal->regmap_left);
	cs35l41_load_config(playback_config_filename_right,
				cirrus_cal->regmap_right);

	regmap_update_bits(cirrus_cal->regmap_left,
		CS35L41_MIXER_NGATE_CH1_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	regmap_update_bits(cirrus_cal->regmap_left,
		CS35L41_MIXER_NGATE_CH2_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	regmap_update_bits(cirrus_cal->regmap_right,
		CS35L41_MIXER_NGATE_CH1_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	regmap_update_bits(cirrus_cal->regmap_right,
		CS35L41_MIXER_NGATE_CH2_CFG,
		CS35L41_NG_ENABLE_MASK,
		CS35L41_NG_ENABLE_MASK);
	dev_dbg(cirrus_cal->dev, "NOISE GATE ENABLE\n");

	/* Send REINIT command and poll for response */
	regmap_write(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_REINIT);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_REINIT);
	timeout = 100;
	do {
		dev_info(cirrus_cal->dev, "waiting for REINIT done...\n");
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_STS,
				&cal_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_STS,
				&cal_state_right);
	} while (((cal_state_left != CSPL_MBOX_STS_RUNNING) ||
			(cal_state_right != CSPL_MBOX_STS_RUNNING)) &&
			--timeout > 0);

	cirrus_cal_unmute_dsp_inputs();
	dev_dbg(cirrus_cal->dev, "DSP Inputs unmuted\n");

	regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE, &cal_state_left);
	if (cal_state_left == CS35L41_CSPL_STATE_ERROR)
		dev_err(cirrus_cal->dev,
			"Playback config load error left\n");

	regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE, &cal_state_right);
	if (cal_state_right == CS35L41_CSPL_STATE_ERROR)
		dev_err(cirrus_cal->dev,
			"Playback config load error right\n");

	dev_info(cirrus_cal->dev, "V validation complete\n");
	kfree(playback_config_filename_left);
	kfree(playback_config_filename_right);
}

static int cirrus_cal_get_power_temp(void)
{
	union power_supply_propval value = {0};
	struct power_supply *psy;

	psy = power_supply_get_by_name("battery");
	if (!psy) {
		dev_warn(cirrus_cal->dev, "failed to get battery, assuming %d\n",
				CS34L40_CAL_AMBIENT_DEFAULT);
		return CS34L40_CAL_AMBIENT_DEFAULT;
	}

	power_supply_get_property(psy, POWER_SUPPLY_PROP_TEMP, &value);

	return DIV_ROUND_CLOSEST(value.intval, 10);
}

static void cirrus_cal_start(void)
{
	int ambient;
	unsigned int global_en;
	unsigned int halo_state_right, halo_state_left;
	char *cal_config_filename_left;
	char *cal_config_filename_right;
	int timeout = 50;

	cal_config_filename_left = kzalloc(PAGE_SIZE, GFP_KERNEL);
	cal_config_filename_right = kzalloc(PAGE_SIZE, GFP_KERNEL);
	snprintf(cal_config_filename_left,
		PAGE_SIZE, "%s%s",
		cirrus_cal->dsp_part_name_left,
		CIRRUS_CAL_CONFIG_FILENAME_SUFFIX);
	snprintf(cal_config_filename_right,
		PAGE_SIZE, "%s%s",
		cirrus_cal->dsp_part_name_right,
		CIRRUS_CAL_CONFIG_FILENAME_SUFFIX);

	dev_info(cirrus_cal->dev, "Calibration prepare start\n");

	regmap_read(cirrus_cal->regmap_left,
			CS35L41_PWR_CTRL1, &global_en);
	while ((global_en & 1) == 0) {
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left,
			CS35L41_PWR_CTRL1, &global_en);
	}

	regmap_read(cirrus_cal->regmap_right,
			CS35L41_PWR_CTRL1, &global_en);
	while ((global_en & 1) == 0) {
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_right,
			CS35L41_PWR_CTRL1, &global_en);
	}

	do {
		dev_info(cirrus_cal->dev, "waiting for HALO start...\n");
		usleep_range(10000, 15500);

		regmap_read(cirrus_cal->regmap_left, CS35L41_HALO_STATE,
			&halo_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_HALO_STATE,
			&halo_state_right);
		timeout--;
	} while ((halo_state_right == 0 || halo_state_left == 0) &&
							timeout > 0);

	if (timeout == 0) {
		dev_err(cirrus_cal->dev, "Failed to setup calibration\n");
		kfree(cal_config_filename_left);
		kfree(cal_config_filename_right);
		return;
	}

	/* Send STOP_PRE_REINIT command and poll for response */
	regmap_write(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_STOP_PRE_REINIT);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_STOP_PRE_REINIT);
	timeout = 100;
	do {
		dev_info(cirrus_cal->dev, "waiting for REINIT ready...\n");
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_STS,
				&halo_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_STS,
				&halo_state_right);
	} while (((halo_state_left != CSPL_MBOX_STS_RDY_FOR_REINIT) ||
			(halo_state_right != CSPL_MBOX_STS_RDY_FOR_REINIT)) &&
			--timeout > 0);

	dev_dbg(cirrus_cal->dev, "load left\n");
	cs35l41_load_config(cal_config_filename_left,
					cirrus_cal->regmap_left);
	dev_dbg(cirrus_cal->dev, "load right\n");
	cs35l41_load_config(cal_config_filename_right,
					cirrus_cal->regmap_right);

	cirrus_cal_mute_dsp_inputs();
	dev_dbg(cirrus_cal->dev, "DSP Inputs muted\n");

	regmap_update_bits(cirrus_cal->regmap_left,
		CS35L41_MIXER_NGATE_CH1_CFG,
		CS35L41_NG_ENABLE_MASK, 0);
	regmap_update_bits(cirrus_cal->regmap_left,
		CS35L41_MIXER_NGATE_CH2_CFG,
		CS35L41_NG_ENABLE_MASK, 0);
	regmap_update_bits(cirrus_cal->regmap_right,
		CS35L41_MIXER_NGATE_CH1_CFG,
		CS35L41_NG_ENABLE_MASK, 0);
	regmap_update_bits(cirrus_cal->regmap_right,
		CS35L41_MIXER_NGATE_CH2_CFG,
		CS35L41_NG_ENABLE_MASK, 0);
	dev_dbg(cirrus_cal->dev, "NOISE GATE DISABLE\n");

	ambient = cirrus_cal_get_power_temp();
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, ambient);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_AMBIENT, ambient);

	/* Send REINIT command and poll for response */
	regmap_write(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_REINIT);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_CMD_DRV,
			CSPL_MBOX_CMD_REINIT);
	timeout = 100;
	do {
		dev_info(cirrus_cal->dev, "waiting for REINIT done...\n");
		usleep_range(1000, 1500);
		regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_MBOX_STS,
				&halo_state_left);
		regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_MBOX_STS,
				&halo_state_right);
	} while (((halo_state_left != CSPL_MBOX_STS_RUNNING) ||
			(halo_state_right != CSPL_MBOX_STS_RUNNING)) &&
			--timeout > 0);

	kfree(cal_config_filename_left);
	kfree(cal_config_filename_right);
}

static int cirrus_cal_read_file(char *filename, int *value)
{
	struct file *cal_filp;
	mm_segment_t old_fs = get_fs();
	char str[12] = {0};
	int ret;

	set_fs(get_ds());

	cal_filp = filp_open(filename, O_RDONLY, 0660);
	if (IS_ERR(cal_filp)) {
		ret = PTR_ERR(cal_filp);
		dev_err(cirrus_cal->dev, "Failed to open calibration file %s: %d\n",
			filename, ret);
		goto err_open;
	}

	ret = vfs_read(cal_filp, (char __user *)str, sizeof(str),
						&cal_filp->f_pos);
	if (ret != sizeof(str)) {
		dev_err(cirrus_cal->dev, "Failed to read calibration file %s\n",
			filename);
		ret = -EIO;
		goto err_read;
	}

	ret = 0;

	if (kstrtoint(str, 0, value)) {
		dev_err(cirrus_cal->dev, "Failed to parse calibration.\n");
		ret = -EINVAL;
	}

err_read:
	filp_close(cal_filp, current->files);
err_open:
	set_fs(old_fs);
	return ret;
}

int cirrus_cal_apply_left(void)
{
	unsigned int temp, rdc_left, status, checksum_left;
	int ret1 = 0;
	int ret2 = 0;

	if (cirrus_cal->efs_cache_read_left == 1) {
		rdc_left = cirrus_cal->efs_cache_rdc_left;
		temp = cirrus_cal->efs_cache_temp;
	} else {
		ret1 = cirrus_cal_read_file(CIRRUS_CAL_RDC_LEFT_SAVE_LOCATION,
						&rdc_left);
		ret2 = cirrus_cal_read_file(CIRRUS_CAL_TEMP_SAVE_LOCATION,
						&temp);

		if (ret1 < 0 || ret2 < 0) {
			dev_err(cirrus_cal->dev,
				"No saved calibration, writing defaults\n");
			rdc_left = CS34L40_CAL_RDC_DEFAULT;
			temp = CS34L40_CAL_AMBIENT_DEFAULT;
		}

		cirrus_cal->efs_cache_rdc_left = rdc_left;
		cirrus_cal->efs_cache_temp = temp;
		cirrus_cal->efs_cache_read_left = 1;
	}

	status = 1;
	checksum_left = status + rdc_left;

	dev_info(cirrus_cal->dev, "Writing calibration: \n");
	dev_info(cirrus_cal->dev,
		"RDC Left = %d, Temp = %d, Status = %d Checksum Left= %d\n",
		rdc_left, temp, status, checksum_left);

	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_RDC, rdc_left);
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, temp);
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_STATUS, status);
	regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_CHECKSUM,
						checksum_left);

	return ret1 | ret2;
}
EXPORT_SYMBOL_GPL(cirrus_cal_apply_left);

int cirrus_cal_apply_right(void)
{
	unsigned int temp, rdc_right, status, checksum_right;
	int ret1 = 0;
	int ret2 = 0;

	if (cirrus_cal->efs_cache_read_right == 1) {
		rdc_right = cirrus_cal->efs_cache_rdc_right;
		temp = cirrus_cal->efs_cache_temp;
	} else {
		ret1 = cirrus_cal_read_file(CIRRUS_CAL_RDC_RIGHT_SAVE_LOCATION,
						&rdc_right);
		ret2 = cirrus_cal_read_file(CIRRUS_CAL_TEMP_SAVE_LOCATION,
						&temp);

		if (ret1 < 0 || ret2 < 0) {
			dev_err(cirrus_cal->dev,
				"No saved calibration, writing defaults\n");
			rdc_right = CS34L40_CAL_RDC_DEFAULT;
			temp = CS34L40_CAL_AMBIENT_DEFAULT;
		}

		cirrus_cal->efs_cache_rdc_right = rdc_right;
		cirrus_cal->efs_cache_temp = temp;
		cirrus_cal->efs_cache_read_right = 1;
	}

	status = 1;
	checksum_right = status + rdc_right;

	dev_info(cirrus_cal->dev, "Writing calibration: \n");
	dev_info(cirrus_cal->dev,
		"RDC Right = %d, Temp = %d, Status = %d, Checksum Right= %d\n",
		rdc_right, temp, status, checksum_right);

	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_RDC, rdc_right);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_AMBIENT, temp);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_STATUS, status);
	regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_CHECKSUM,
						checksum_right);

	return ret1 | ret2;
}
EXPORT_SYMBOL_GPL(cirrus_cal_apply_right);

/***** SYSFS Interfaces *****/

static ssize_t cirrus_cal_version_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, CIRRUS_CAL_VERSION "\n");
}

static ssize_t cirrus_cal_version_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n",
			cirrus_cal->cal_running ? "Enabled" : "Disabled");
}

static ssize_t cirrus_cal_status_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int prepare;
	int ret = kstrtos32(buf, 10, &prepare);
	int delay = msecs_to_jiffies(CS35L41_CAL_COMPLETE_DELAY_MS);
	int retries = 5;
	unsigned int cal_state_left, cal_state_right;

	if (cirrus_cal->cal_running) {
		dev_err(cirrus_cal->dev,
			"cirrus_cal measurement in progress\n");
		return size;
	}

	mutex_lock(&cirrus_cal->lock);

	if (ret == 0 && (cirrus_cal->regmap_right) && (cirrus_cal->regmap_left)) {
		if (prepare == 1) {

			cirrus_cal->cal_running = 1;

			regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_STATUS,0);
			regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_RDC, 0);
			regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, 0);
			regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_CHECKSUM,0);
			regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_STATUS, 0);
			regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_RDC, 0);
			regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_CHECKSUM, 0);

			cirrus_cal_start();
			usleep_range(80000, 90000);

			regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE,
				&cal_state_left);
			regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE,
				&cal_state_right);

			while ((cal_state_left == CS35L41_CSPL_STATE_ERROR ||
				cal_state_right == CS35L41_CSPL_STATE_ERROR) &&
				retries > 0) {
				if (cal_state_left == CS35L41_CSPL_STATE_ERROR){
					dev_err(cirrus_cal->dev,
						"Calibration config load error left\n");
				}
				if (cal_state_right == CS35L41_CSPL_STATE_ERROR)
					dev_err(cirrus_cal->dev,
						"Calibration config load error right\n");

				cirrus_cal_start();
				usleep_range(80000, 90000);
				regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE,
					&cal_state_left);
				regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE,
					&cal_state_right);
				retries--;
			}

			if (retries == 0) {
				dev_err(cirrus_cal->dev,
					"Calibration setup failed\n");
				mutex_unlock(&cirrus_cal->lock);
				cirrus_cal_unmute_dsp_inputs();
				cirrus_cal->cal_running = 0;
				return size;
			}

			dev_dbg(cirrus_cal->dev, "Calibration prepare complete\n");

			queue_delayed_work(system_unbound_wq,
						&cirrus_cal->cal_complete_work,
						delay);
		}
	}

	mutex_unlock(&cirrus_cal->lock);
	return size;
}

static ssize_t cirrus_cal_v_status_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%s\n",
			cirrus_cal->cal_running ? "Enabled" : "Disabled");
}

static ssize_t cirrus_cal_v_status_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int prepare;
	int ret = kstrtos32(buf, 10, &prepare);
	int retries = 5;
	unsigned int cal_state_left, cal_state_right;
	int i, reg;
	unsigned int vmax_left = 0, vmax_right = 0;
	unsigned int vmin_left = INT_MAX, vmin_right = INT_MAX;
	unsigned int imax_left = 0, imax_right = 0;
	unsigned int imin_left = INT_MAX, imin_right = INT_MAX;

	if (cirrus_cal->cal_running) {
		dev_err(cirrus_cal->dev,
			"cirrus_cal measurement in progress\n");
		return size;
	}

	mutex_lock(&cirrus_cal->lock);

	if (ret == 0 && (cirrus_cal->regmap_right) && (cirrus_cal->regmap_left)) {
		if (prepare == 1) {

			cirrus_cal->cal_running = 1;

			regmap_write(cirrus_cal->regmap_right, CIRRUS_PWR_CSPL_V_PEAK, 0);
			regmap_write(cirrus_cal->regmap_left, CIRRUS_PWR_CSPL_V_PEAK, 0);
			regmap_write(cirrus_cal->regmap_right, CIRRUS_PWR_CSPL_I_PEAK, 0);
			regmap_write(cirrus_cal->regmap_left, CIRRUS_PWR_CSPL_I_PEAK, 0);

			cirrus_cal_start();
			usleep_range(80000, 90000);

			regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE,
				&cal_state_left);
			regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE,
				&cal_state_right);

			while ((cal_state_left == CS35L41_CSPL_STATE_ERROR ||
				cal_state_right == CS35L41_CSPL_STATE_ERROR) &&
				retries > 0) {
				if (cal_state_left == CS35L41_CSPL_STATE_ERROR){
					dev_err(cirrus_cal->dev,
						"Calibration config load error left\n");
				}
				if (cal_state_right == CS35L41_CSPL_STATE_ERROR)
					dev_err(cirrus_cal->dev,
						"Calibration config load error right\n");

				cirrus_cal_start();
				usleep_range(80000, 90000);
				regmap_read(cirrus_cal->regmap_left, CS35L41_CSPL_STATE,
					&cal_state_left);
				regmap_read(cirrus_cal->regmap_right, CS35L41_CSPL_STATE,
					&cal_state_right);
				retries--;
			}

			if (retries == 0) {
				dev_err(cirrus_cal->dev,
					"Calibration setup failed\n");
				mutex_unlock(&cirrus_cal->lock);
				cirrus_cal_unmute_dsp_inputs();
				cirrus_cal->cal_running = 0;
				return size;
			}

			dev_info(cirrus_cal->dev, "V validation prepare complete\n");

			for (i = 0; i < 400; i++)
			{
				regmap_read(cirrus_cal->regmap_left,
					CIRRUS_PWR_CSPL_V_PEAK, &reg);
				if (reg > vmax_left) vmax_left = reg;
				if (reg < vmin_left) vmin_left = reg;

				regmap_read(cirrus_cal->regmap_right,
					CIRRUS_PWR_CSPL_V_PEAK, &reg);
				if (reg > vmax_right) vmax_right = reg;
				if (reg < vmin_right) vmin_right = reg;

				regmap_read(cirrus_cal->regmap_left,
					CIRRUS_PWR_CSPL_I_PEAK, &reg);
				if (reg > imax_left) imax_left = reg;
				if (reg < imin_left) imin_left = reg;

				regmap_read(cirrus_cal->regmap_right,
					CIRRUS_PWR_CSPL_I_PEAK, &reg);
				if (reg > imax_right) imax_right = reg;
				if (reg < imin_right) imin_right = reg;

				usleep_range(50, 150);
			}

			dev_dbg(cirrus_cal->dev, "V Max Left: 0x%x\tV Max Right: 0x%x\n",
				vmax_left, vmax_right);
			vmax_left = cirrus_cal_vpk_to_mv(vmax_left);
			vmax_right = cirrus_cal_vpk_to_mv(vmax_right);
			dev_info(cirrus_cal->dev, "V Max Left: %d mV\tV Max Right: %d mV\n",
				vmax_left, vmax_right);

			dev_dbg(cirrus_cal->dev, "V Min Left: 0x%x\tV Min Right: 0x%x\n",
				vmin_left, vmin_right);
			vmin_left = cirrus_cal_vpk_to_mv(vmin_left);
			vmin_right = cirrus_cal_vpk_to_mv(vmin_right);
			dev_info(cirrus_cal->dev, "V Min Left: %d mV\tV Min Right: %d mV\n",
				vmin_left, vmin_right);

			dev_dbg(cirrus_cal->dev, "I Max Left: 0x%x\tI Max Right: 0x%x\n",
				imax_left, imax_right);
			imax_left = cirrus_cal_ipk_to_ma(imax_left);
			imax_right = cirrus_cal_ipk_to_ma(imax_right);
			dev_info(cirrus_cal->dev, "I Max Left: %d mA\tI Max Right: %d mA\n",
				imax_left, imax_right);

			dev_dbg(cirrus_cal->dev, "I Min Left: 0x%x\tI Min Right: 0x%x\n",
				imin_left, imin_right);
			imin_left = cirrus_cal_ipk_to_ma(imin_left);
			imin_right = cirrus_cal_ipk_to_ma(imin_right);
			dev_info(cirrus_cal->dev, "I Min Left: %d mA\tI Min Right: %d mA\n",
				imin_left, imin_right);

			if (vmax_left < CIRRUS_CAL_V_VAL_UB_MV &&
			    vmax_left > CIRRUS_CAL_V_VAL_LB_MV) {
				cirrus_cal->v_validation_left = 1;
				dev_info(cirrus_cal->dev,
					"V validation success Left\n");
			} else {
				cirrus_cal->v_validation_left = 0xCC;
				dev_err(cirrus_cal->dev,
					"V validation failed Left\n");
			}

			if (vmax_right < CIRRUS_CAL_V_VAL_UB_MV &&
			    vmax_right > CIRRUS_CAL_V_VAL_LB_MV) {
				cirrus_cal->v_validation_right = 1;
				dev_info(cirrus_cal->dev,
					"V validation success Right\n");
			} else {
				cirrus_cal->v_validation_right = 0xCC;
				dev_err(cirrus_cal->dev,
					"V validation failed Right\n");
			}

			cirrus_cal_v_val_complete();

			cirrus_cal->cal_running = 0;
		}
	}

	mutex_unlock(&cirrus_cal->lock);
	return size;
}

static ssize_t cirrus_cal_vval_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d", cirrus_cal->v_validation_left);
}

static ssize_t cirrus_cal_vval_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_vval_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	return sprintf(buf, "%d", cirrus_cal->v_validation_right);
}

static ssize_t cirrus_cal_vval_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_rdc_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int rdc;

	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_RDC, &rdc);

	return sprintf(buf, "%d", rdc);
}

static ssize_t cirrus_cal_rdc_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int rdc, ret;

	ret = kstrtos32(buf, 10, &rdc);
	if (ret == 0)
		regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_RDC, rdc);
	return size;
}

static ssize_t cirrus_cal_temp_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int temp;

	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, &temp);
	return sprintf(buf, "%d", temp);
}

static ssize_t cirrus_cal_temp_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int temp, ret;

	ret = kstrtos32(buf, 10, &temp);
	if (ret == 0)
		regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_AMBIENT, temp);
	return size;
}

static ssize_t cirrus_cal_checksum_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int checksum;

	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_CHECKSUM, &checksum);
	return sprintf(buf, "%d", checksum);
}

static ssize_t cirrus_cal_checksum_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int checksum, ret;

	ret = kstrtos32(buf, 10, &checksum);
	if (ret == 0)
		regmap_write(cirrus_cal->regmap_left, CS35L41_CAL_CHECKSUM, checksum);
	return size;
}

static ssize_t cirrus_cal_set_status_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int set_status;

	regmap_read(cirrus_cal->regmap_left, CS35L41_CAL_SET_STATUS, &set_status);
	return sprintf(buf, "%d", set_status);
}

static ssize_t cirrus_cal_set_status_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_rdc_stored_left_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int rdc = 0;

	cirrus_cal_read_file(CIRRUS_CAL_RDC_LEFT_SAVE_LOCATION, &rdc);
	return sprintf(buf, "%d", rdc);
}

static ssize_t cirrus_cal_rdc_stored_left_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_temp_stored_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int temp_stored = 0;

	cirrus_cal_read_file(CIRRUS_CAL_TEMP_SAVE_LOCATION, &temp_stored);
	return sprintf(buf, "%d", temp_stored);
}

static ssize_t cirrus_cal_temp_stored_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_rdc_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int rdc;

	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_RDC, &rdc);

	return sprintf(buf, "%d", rdc);
}

static ssize_t cirrus_cal_rdc_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int rdc, ret;

	ret = kstrtos32(buf, 10, &rdc);
	if (ret == 0)
		regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_RDC, rdc);
	return size;
}

static ssize_t cirrus_cal_temp_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int temp;

	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_AMBIENT, &temp);
	return sprintf(buf, "%d", temp);
}

static ssize_t cirrus_cal_temp_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int temp, ret;

	ret = kstrtos32(buf, 10, &temp);
	if (ret == 0)
		regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_AMBIENT, temp);
	return size;
}

static ssize_t cirrus_cal_checksum_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int checksum;

	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_CHECKSUM, &checksum);
	return sprintf(buf, "%d", checksum);
}

static ssize_t cirrus_cal_checksum_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int checksum, ret;

	ret = kstrtos32(buf, 10, &checksum);
	if (ret == 0)
		regmap_write(cirrus_cal->regmap_right, CS35L41_CAL_CHECKSUM, checksum);
	return size;
}

static ssize_t cirrus_cal_set_status_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int set_status;

	regmap_read(cirrus_cal->regmap_right, CS35L41_CAL_SET_STATUS, &set_status);
	return sprintf(buf, "%d", set_status);
}

static ssize_t cirrus_cal_set_status_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static ssize_t cirrus_cal_rdc_stored_right_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	unsigned int rdc = 0;

	cirrus_cal_read_file(CIRRUS_CAL_RDC_RIGHT_SAVE_LOCATION, &rdc);
	return sprintf(buf, "%d", rdc);
}

static ssize_t cirrus_cal_rdc_stored_right_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	return 0;
}

static DEVICE_ATTR(version, 0444, cirrus_cal_version_show,
				cirrus_cal_version_store);
static DEVICE_ATTR(status, 0664, cirrus_cal_status_show,
				cirrus_cal_status_store);
static DEVICE_ATTR(v_status, 0664, cirrus_cal_v_status_show,
				cirrus_cal_v_status_store);
static DEVICE_ATTR(v_validation, 0444, cirrus_cal_vval_left_show,
				cirrus_cal_vval_left_store);
static DEVICE_ATTR(v_validation_r, 0444, cirrus_cal_vval_right_show,
				cirrus_cal_vval_right_store);
static DEVICE_ATTR(rdc, 0664, cirrus_cal_rdc_left_show,
				cirrus_cal_rdc_left_store);
static DEVICE_ATTR(temp, 0664, cirrus_cal_temp_left_show,
				cirrus_cal_temp_left_store);
static DEVICE_ATTR(checksum, 0664, cirrus_cal_checksum_left_show,
				cirrus_cal_checksum_left_store);
static DEVICE_ATTR(set_status, 0444, cirrus_cal_set_status_left_show,
				cirrus_cal_set_status_left_store);
static DEVICE_ATTR(rdc_stored, 0444, cirrus_cal_rdc_stored_left_show,
				cirrus_cal_rdc_stored_left_store);
static DEVICE_ATTR(temp_stored, 0444, cirrus_cal_temp_stored_show,
				cirrus_cal_temp_stored_store);
static DEVICE_ATTR(rdc_r, 0664, cirrus_cal_rdc_right_show,
				cirrus_cal_rdc_right_store);
static DEVICE_ATTR(temp_r, 0664, cirrus_cal_temp_right_show,
				cirrus_cal_temp_right_store);
static DEVICE_ATTR(checksum_r, 0664, cirrus_cal_checksum_right_show,
				cirrus_cal_checksum_right_store);
static DEVICE_ATTR(set_status_r, 0444, cirrus_cal_set_status_right_show,
				cirrus_cal_set_status_right_store);
static DEVICE_ATTR(rdc_stored_r, 0444, cirrus_cal_rdc_stored_right_show,
				cirrus_cal_rdc_stored_right_store);

static struct attribute *cirrus_cal_attr[] = {
	&dev_attr_version.attr,
	&dev_attr_status.attr,
	&dev_attr_v_status.attr,
	&dev_attr_v_validation.attr,
	&dev_attr_v_validation_r.attr,
	&dev_attr_rdc.attr,
	&dev_attr_temp.attr,
	&dev_attr_checksum.attr,
	&dev_attr_set_status.attr,
	&dev_attr_rdc_stored.attr,
	&dev_attr_temp_stored.attr,
	&dev_attr_rdc_r.attr,
	&dev_attr_temp_r.attr,
	&dev_attr_checksum_r.attr,
	&dev_attr_set_status_r.attr,
	&dev_attr_rdc_stored_r.attr,
	NULL,
};

static struct attribute_group cirrus_cal_attr_grp = {
	.attrs = cirrus_cal_attr,
};

int cirrus_cal_init(struct class *cirrus_amp_class)
{
	int ret;

	cirrus_cal = kzalloc(sizeof(struct cirrus_cal_t), GFP_KERNEL);
	if (cirrus_cal == NULL)
		return -ENOMEM;

	cirrus_cal->cal_class = cirrus_amp_class;
	if (IS_ERR(cirrus_cal->cal_class)) {
		pr_err("Failed to register cirrus_cal\n");
		ret = -EINVAL;
		goto err;
	}

	cirrus_cal->dev = device_create(cirrus_cal->cal_class, NULL, 1, NULL,
						CIRRUS_CAL_DIR_NAME);
	if (IS_ERR(cirrus_cal->dev)) {
		ret = PTR_ERR(cirrus_cal->dev);
		pr_err("Failed to create cirrus_cal device\n");
		class_destroy(cirrus_cal->cal_class);
		goto err;
	}

	ret = sysfs_create_group(&cirrus_cal->dev->kobj, &cirrus_cal_attr_grp);
	if (ret) {
		dev_err(cirrus_cal->dev, "Failed to create sysfs group\n");
		goto err;
	}

	cirrus_cal->efs_cache_read_left = 0;
	cirrus_cal->efs_cache_read_right = 0;
	cirrus_cal->v_validation_left = 0;
	cirrus_cal->v_validation_right = 0;
	mutex_init(&cirrus_cal->lock);
	INIT_DELAYED_WORK(&cirrus_cal->cal_complete_work,
						cirrus_cal_complete_work);

	return 0;
err:
	kfree(cirrus_cal);
	return ret;
}

void cirrus_cal_exit(void)
{
	kfree(cirrus_cal);
}


