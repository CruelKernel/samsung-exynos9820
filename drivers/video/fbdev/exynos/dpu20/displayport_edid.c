/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC DisplayPort EDID driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/fb.h>
#include "displayport.h"

#ifdef FEATURE_SUPPORT_DISPLAYID
#include <drm/drm_displayid.h>
#endif

#define EDID_SEGMENT_ADDR	(0x60 >> 1)
#define EDID_ADDR		(0xA0 >> 1)
#define EDID_SEGMENT_IGNORE	(2)
#define EDID_BLOCK_SIZE		128
#define EDID_SEGMENT(x)		((x) >> 1)
#define EDID_OFFSET(x)		(((x) & 1) * EDID_BLOCK_SIZE)
#define EDID_EXTENSION_FLAG	0x7E
#define EDID_NATIVE_FORMAT	0x83
#define EDID_BASIC_AUDIO	(1 << 6)
#define EDID_COLOR_DEPTH	0x14

#define DETAILED_TIMING_DESCRIPTIONS_START	0x36

int forced_resolution = -1;

videoformat ud_mode_h14b_vsdb[] = {
	V3840X2160P30,
	V3840X2160P25,
	V3840X2160P24,
	V4096X2160P24
};

static struct v4l2_dv_timings preferred_preset = V4L2_DV_BT_DMT_640X480P60;
static u32 edid_misc;
static int audio_channels;
static int audio_bit_rates;
static int audio_sample_rates;
static int audio_speaker_alloc;

struct fb_audio test_audio_info;

void edid_check_set_i2c_capabilities(void)
{
	u8 val[1];

	displayport_reg_dpcd_read(DPCD_ADD_I2C_SPEED_CONTROL_CAPABILITES, 1, val);
	displayport_info("DPCD_ADD_I2C_SPEED_CONTROL_CAPABILITES = 0x%x\n", val[0]);

	if (val[0] != 0) {
		if (val[0] & I2C_1Mbps)
			val[0] = I2C_1Mbps;
		else if (val[0] & I2C_400Kbps)
			val[0] = I2C_400Kbps;
		else if (val[0] & I2C_100Kbps)
			val[0] = I2C_100Kbps;
		else if (val[0] & I2C_10Kbps)
			val[0] = I2C_10Kbps;
		else if (val[0] & I2C_1Kbps)
			val[0] = I2C_1Kbps;
		else
			val[0] = I2C_400Kbps;

		displayport_reg_dpcd_write(DPCD_ADD_I2C_SPEED_CONTROL_STATUS, 1, val);
		displayport_dbg("DPCD_ADD_I2C_SPEED_CONTROL_STATUS = 0x%x\n", val[0]);
	}
}

static int edid_checksum(u8 *data, int block)
{
	int i;
	u8 sum = 0, all_null = 0;

	for (i = 0; i < EDID_BLOCK_SIZE; i++) {
		sum += data[i];
		all_null |= data[i];
	}

	if (sum || all_null == 0x0) {
		displayport_err("checksum error block = %d sum = %02x\n", block, sum);
		return -EPROTO;
	}

	return 0;
}

static int edid_read_block(struct displayport_device *hdev, int block, u8 *buf, size_t len)
{
	int ret;
	u8 offset = EDID_OFFSET(block);

	if (len < EDID_BLOCK_SIZE)
		return -EINVAL;

	edid_check_set_i2c_capabilities();
	ret = displayport_reg_edid_read(offset, EDID_BLOCK_SIZE, buf);
	if (ret)
		return ret;

	print_hex_dump(KERN_INFO, "EDID: ", DUMP_PREFIX_OFFSET, 16, 1,
					buf, 128, false);
#if defined(CONFIG_SEC_DISPLAYPORT_LOGGER)
	dp_print_hex_dump(buf, "EDID: ", 128);
#endif
	return 0;
}

int edid_read(struct displayport_device *hdev, u8 **data)
{
	u8 block0[EDID_BLOCK_SIZE];
	u8 *edid;
	int block = 0;
	int block_cnt, ret;

	ret = edid_read_block(hdev, 0, block0, sizeof(block0));
	if (ret)
		return ret;

	ret = edid_checksum(block0, block);
	if (ret)
		return ret;

	block_cnt = block0[EDID_EXTENSION_FLAG] + 1;
	displayport_info("block_cnt = %d\n", block_cnt);

	edid = kmalloc(block_cnt * EDID_BLOCK_SIZE, GFP_KERNEL);
	if (!edid)
		return -ENOMEM;

	memcpy(edid, block0, sizeof(block0));

	while (++block < block_cnt) {
		ret = edid_read_block(hdev, block,
			edid + (block * EDID_BLOCK_SIZE),
			EDID_BLOCK_SIZE);

		/* check error, extension tag and checksum */
		if (ret || *(edid + (block * EDID_BLOCK_SIZE)) != 0x02 ||
				edid_checksum(edid + (block * EDID_BLOCK_SIZE), block)) {
			displayport_info("block_cnt:%d/%d, ret: %d\n", block, block_cnt, ret);
			*data = edid;
			return block;
		}
	}

	*data = edid;

	return block_cnt;
}

static int get_ud_timing(struct fb_vendor *vsdb, int vic_idx)
{
	unsigned char val = 0;
	int idx = -EINVAL;

	val = vsdb->vic_data[vic_idx];
	switch (val) {
	case 0x01:
		idx = 0;
		break;
	case 0x02:
		idx = 1;
		break;
	case 0x03:
		idx = 2;
		break;
	case 0x04:
		idx = 3;
		break;
	}

	return idx;
}

bool edid_find_max_resolution(const struct v4l2_dv_timings *t1,
			const struct v4l2_dv_timings *t2)
{
	if ((t1->bt.width * t1->bt.height < t2->bt.width * t2->bt.height) ||
		((t1->bt.width * t1->bt.height == t2->bt.width * t2->bt.height) &&
		(t1->bt.pixelclock < t2->bt.pixelclock)))
		return true;

	return false;
}

static void edid_find_preset(const struct fb_videomode *mode)
{
	int i;
	struct displayport_device *displayport = get_displayport_drvdata();

	displayport_dbg("EDID: %ux%u@%u - %u(ps?), lm:%u, rm:%u, um:%u, lm:%u",
		mode->xres, mode->yres, mode->refresh, mode->pixclock,
		mode->left_margin, mode->right_margin, mode->upper_margin, mode->lower_margin);

	for (i = 0; i < supported_videos_pre_cnt; i++) {
		if ((mode->refresh == supported_videos[i].fps ||
			mode->refresh == supported_videos[i].fps - 1) &&
			mode->xres == supported_videos[i].dv_timings.bt.width &&
			mode->yres == supported_videos[i].dv_timings.bt.height &&
			mode->left_margin == supported_videos[i].dv_timings.bt.hbackporch &&
			mode->right_margin == supported_videos[i].dv_timings.bt.hfrontporch &&
			mode->upper_margin == supported_videos[i].dv_timings.bt.vbackporch &&
			mode->lower_margin == supported_videos[i].dv_timings.bt.vfrontporch) {
			if (supported_videos[i].edid_support_match == false) {
				displayport_info("EDID: found: %s, dex: %d\n",
					supported_videos[i].name, supported_videos[i].dex_support);
				supported_videos[i].edid_support_match = true;
				preferred_preset = supported_videos[i].dv_timings;
				if (displayport->best_video < i)
					displayport->best_video = i;
			}
		}
	}
}

static void edid_use_default_preset(void)
{
	int i;

	if (forced_resolution >= 0)
		preferred_preset = supported_videos[forced_resolution].dv_timings;
	else
		preferred_preset = supported_videos[EDID_DEFAULT_TIMINGS_IDX].dv_timings;

	for (i = 0; i < supported_videos_pre_cnt; i++) {
		supported_videos[i].edid_support_match =
			v4l2_match_dv_timings(&supported_videos[i].dv_timings,
					&preferred_preset, 0, 0);
	}

	if (!test_audio_info.channel_count)
		audio_channels = 2;
}

void edid_set_preferred_preset(int mode)
{
	int i;

	preferred_preset = supported_videos[mode].dv_timings;
	for (i = 0; i < supported_videos_pre_cnt; i++) {
		supported_videos[i].edid_support_match =
			v4l2_match_dv_timings(&supported_videos[i].dv_timings,
					&preferred_preset, 0, 0);
	}
}

int edid_find_resolution(u16 xres, u16 yres, u16 refresh)
{
	int i;
	int ret=0;

	for (i = 0; i < supported_videos_pre_cnt; i++) {
		if (refresh == supported_videos[i].fps &&
			xres == supported_videos[i].dv_timings.bt.width &&
			yres == supported_videos[i].dv_timings.bt.height) {
			return i;
		}
	}
	return ret;
}

void edid_parse_hdmi14_vsdb(unsigned char *edid_ext_blk,
	struct fb_vendor *vsdb, int block_cnt)
{
	int i, j;
	int hdmi_vic_len;
	int vsdb_offset_calc = VSDB_VIC_FIELD_OFFSET;

	for (i = 0; i < (block_cnt - 1) * EDID_BLOCK_SIZE; i++) {
		if ((edid_ext_blk[i] & DATA_BLOCK_TAG_CODE_MASK)
			== (VSDB_TAG_CODE << DATA_BLOCK_TAG_CODE_BIT_POSITION)
				&& edid_ext_blk[i + IEEE_OUI_0_BYTE_NUM] == HDMI14_IEEE_OUI_0
				&& edid_ext_blk[i + IEEE_OUI_1_BYTE_NUM] == HDMI14_IEEE_OUI_1
				&& edid_ext_blk[i + IEEE_OUI_2_BYTE_NUM] == HDMI14_IEEE_OUI_2) {
			displayport_dbg("EDID: find VSDB for HDMI 1.4\n");

			if (edid_ext_blk[i + 8] & VSDB_HDMI_VIDEO_PRESETNT_MASK) {
				displayport_dbg("EDID: Find HDMI_Video_present in VSDB\n");

				if (!(edid_ext_blk[i + 8] & VSDB_LATENCY_FILEDS_PRESETNT_MASK)) {
					vsdb_offset_calc = vsdb_offset_calc - 2;
					displayport_dbg("EDID: Not support LATENCY_FILEDS_PRESETNT in VSDB\n");
				}

				if (!(edid_ext_blk[i + 8] & VSDB_I_LATENCY_FILEDS_PRESETNT_MASK)) {
					vsdb_offset_calc = vsdb_offset_calc - 2;
					displayport_dbg("EDID: Not support I_LATENCY_FILEDS_PRESETNT in VSDB\n");
				}

				hdmi_vic_len = (edid_ext_blk[i + vsdb_offset_calc]
						& VSDB_VIC_LENGTH_MASK) >> VSDB_VIC_LENGTH_BIT_POSITION;

				if (hdmi_vic_len > 0) {
					vsdb->vic_len = hdmi_vic_len;

					for (j = 0; j < hdmi_vic_len; j++)
						vsdb->vic_data[j] = edid_ext_blk[i + vsdb_offset_calc + j + 1];

					break;
				} else {
					vsdb->vic_len = 0;
					displayport_dbg("EDID: No hdmi vic data in VSDB\n");
					break;
				}
			} else
				displayport_dbg("EDID: Not support HDMI_Video_present in VSDB\n");
		}
	}

	if (i >= (block_cnt - 1) * EDID_BLOCK_SIZE) {
		vsdb->vic_len = 0;
		displayport_dbg("EDID: can't find VSDB for HDMI 1.4 block\n");
	}
}

int static dv_timing_to_fb_video(videoformat video, struct fb_videomode *fb)
{
	struct displayport_supported_preset pre = supported_videos[video];

	fb->name = pre.name;
	fb->refresh = pre.fps;
	fb->xres = pre.dv_timings.bt.width;
	fb->yres = pre.dv_timings.bt.height;
	fb->pixclock = pre.dv_timings.bt.pixelclock;
	fb->left_margin = pre.dv_timings.bt.hbackporch;
	fb->right_margin = pre.dv_timings.bt.hfrontporch;
	fb->upper_margin = pre.dv_timings.bt.vbackporch;
	fb->lower_margin = pre.dv_timings.bt.vfrontporch;
	fb->hsync_len = pre.dv_timings.bt.hsync;
	fb->vsync_len = pre.dv_timings.bt.vsync;
	fb->sync = pre.v_sync_pol | pre.h_sync_pol;
	fb->vmode = FB_VMODE_NONINTERLACED;

	return 0;
}

void edid_find_hdmi14_vsdb_update(struct fb_vendor *vsdb)
{
	int udmode_idx, vic_idx;

	if (!vsdb)
		return;

	/* find UHD preset in HDMI 1.4 vsdb block*/
	if (vsdb->vic_len) {
		for (vic_idx = 0; vic_idx < vsdb->vic_len; vic_idx++) {
			udmode_idx = get_ud_timing(vsdb, vic_idx);

			displayport_dbg("EDID: udmode_idx = %d\n", udmode_idx);

			if (udmode_idx >= 0) {
				struct fb_videomode fb;

				dv_timing_to_fb_video(ud_mode_h14b_vsdb[udmode_idx], &fb);
				edid_find_preset(&fb);
			}
		}
	}
}

void edid_parse_hdmi20_vsdb(unsigned char *edid_ext_blk,
	struct fb_vendor *vsdb, int block_cnt)
{
	int i;
	struct displayport_device *displayport = get_displayport_drvdata();

	displayport->rx_edid_data.max_support_clk = 0;
	displayport->rx_edid_data.support_10bpc = 0;

	for (i = 0; i < (block_cnt - 1) * EDID_BLOCK_SIZE; i++) {
		if ((edid_ext_blk[i] & DATA_BLOCK_TAG_CODE_MASK)
			== (VSDB_TAG_CODE << DATA_BLOCK_TAG_CODE_BIT_POSITION)
				&& edid_ext_blk[i + IEEE_OUI_0_BYTE_NUM] == HDMI20_IEEE_OUI_0
				&& edid_ext_blk[i + IEEE_OUI_1_BYTE_NUM] == HDMI20_IEEE_OUI_1
				&& edid_ext_blk[i + IEEE_OUI_2_BYTE_NUM] == HDMI20_IEEE_OUI_2) {
			displayport_dbg("EDID: find VSDB for HDMI 2.0\n");

			/* Max_TMDS_Character_Rate * 5Mhz */
			displayport->rx_edid_data.max_support_clk =
				edid_ext_blk[i + MAX_TMDS_RATE_BYTE_NUM] * 5;
			displayport_info("EDID: Max_TMDS_Character_Rate = %d Mhz\n",
				displayport->rx_edid_data.max_support_clk);

			if (edid_ext_blk[i + DC_SUPPORT_BYTE_NUM] & DC_30BIT)
				displayport->rx_edid_data.support_10bpc = 1;
			else
				displayport->rx_edid_data.support_10bpc = 0;

			displayport_info("EDID: 10 bpc support = %d\n",
				displayport->rx_edid_data.support_10bpc);

			break;
		}
	}

	if (i >= (block_cnt - 1) * EDID_BLOCK_SIZE) {
		vsdb->vic_len = 0;
		displayport_dbg("EDID: can't find VSDB for HDMI 2.0 block\n");
	}
}

void edid_parse_hdr_metadata(unsigned char *edid_ext_blk,  int block_cnt)
{
	int i;
	struct displayport_device *displayport = get_displayport_drvdata();

	displayport->rx_edid_data.hdr_support = 0;
	displayport->rx_edid_data.eotf = 0;
	displayport->rx_edid_data.max_lumi_data = 0;
	displayport->rx_edid_data.max_average_lumi_data = 0;
	displayport->rx_edid_data.min_lumi_data = 0;

	for (i = 0; i < (block_cnt - 1) * EDID_BLOCK_SIZE; i++) {
		if ((edid_ext_blk[i] & DATA_BLOCK_TAG_CODE_MASK)
			== (USE_EXTENDED_TAG_CODE << DATA_BLOCK_TAG_CODE_BIT_POSITION)
				&& edid_ext_blk[i + EXTENDED_TAG_CODE_BYTE_NUM]
				== EXTENDED_HDR_TAG_CODE) {
			displayport_dbg("EDID: find HDR Metadata Data Block\n");

			displayport->rx_edid_data.eotf =
				edid_ext_blk[i + SUPPORTED_EOTF_BYTE_NUM];
			displayport_dbg("EDID: SUPPORTED_EOTF = 0x%x\n",
				displayport->rx_edid_data.eotf);

			if (displayport->rx_edid_data.eotf & SMPTE_ST_2084) {
				if (displayport->hdr_test == 1)
					displayport->rx_edid_data.hdr_support = 1;
				displayport_info("EDID: SMPTE_ST_2084 support(%d)\n",
							displayport->rx_edid_data.hdr_support);
			}

			displayport->rx_edid_data.max_lumi_data =
				edid_ext_blk[i + MAX_LUMI_BYTE_NUM];
			displayport_dbg("EDID: MAX_LUMI = 0x%x\n",
				displayport->rx_edid_data.max_lumi_data);

			displayport->rx_edid_data.max_average_lumi_data =
				edid_ext_blk[i + MAX_AVERAGE_LUMI_BYTE_NUM];
			displayport_dbg("EDID: MAX_AVERAGE_LUMI = 0x%x\n",
				displayport->rx_edid_data.max_average_lumi_data);

			displayport->rx_edid_data.min_lumi_data =
				edid_ext_blk[i + MIN_LUMI_BYTE_NUM];
			displayport_dbg("EDID: MIN_LUMI = 0x%x\n",
				displayport->rx_edid_data.min_lumi_data);

			if (displayport->hdr_test == 2) {
				displayport->rx_edid_data.hdr_support = 1;
				displayport->rx_edid_data.eotf = 0x5;
				displayport->rx_edid_data.max_lumi_data = 2;
				displayport->rx_edid_data.max_average_lumi_data = 58;
				displayport->rx_edid_data.min_lumi_data = 128;
				displayport_info("Dummy HDR Metadata Data for test\n");
			}

			displayport_info("HDR: EOTF(0x%X) ST2084(%u) GAMMA(%s|%s) LUMI(max:%u,avg:%u,min:%u)\n",
					displayport->rx_edid_data.eotf,
					displayport->rx_edid_data.hdr_support,
					displayport->rx_edid_data.eotf & 0x1 ? "SDR" : "",
					displayport->rx_edid_data.eotf & 0x2 ? "HDR" : "",
					displayport->rx_edid_data.max_lumi_data,
					displayport->rx_edid_data.max_average_lumi_data,
					displayport->rx_edid_data.min_lumi_data);
			break;
		}
	}

	if (i >= (block_cnt - 1) * EDID_BLOCK_SIZE)
		displayport_dbg("EDID: can't find HDR Metadata Data Block\n");
}

void edid_find_preset_in_video_data_block(u8 vic)
{
	int i;
	struct displayport_device *displayport = get_displayport_drvdata();

	for (i = 0; i < supported_videos_pre_cnt; i++) {
		if ((vic != 0) && (supported_videos[i].vic == vic) &&
				(supported_videos[i].edid_support_match == false)) {
			supported_videos[i].edid_support_match = true;
			if (displayport->best_video < i)
					displayport->best_video = i;
			displayport_info("EDID: found(VDB): %s, dex: %d\n",
					supported_videos[i].name, supported_videos[i].dex_support);
		}

	}
}

static int edid_parse_audio_video_db(unsigned char *edid, struct fb_audio *sad)
{
	int i;
	u8 pos = 4;

	if (!edid)
		return -EINVAL;

	if (edid[0] != 0x2 || edid[1] != 0x3 ||
	    edid[2] < 4 || edid[2] > 128 - DETAILED_TIMING_DESCRIPTION_SIZE)
		return -EINVAL;

	if (!sad)
		return -EINVAL;

	while (pos < edid[2]) {
		u8 len = edid[pos] & DATA_BLOCK_LENGTH_MASK;
		u8 type = (edid[pos] >> DATA_BLOCK_TAG_CODE_BIT_POSITION) & 7;
		displayport_dbg("Data block %u of %u bytes\n", type, len);

		if (len == 0)
			break;

		pos++;
		if (type == AUDIO_DATA_BLOCK) {
			for (i = pos; i < pos + len; i += 3) {
				if (((edid[i] >> 3) & 0xf) != 1)
					continue; /* skip non-lpcm */

				displayport_dbg("LPCM ch=%d\n", (edid[i] & 7) + 1);

				sad->channel_count |= 1 << (edid[i] & 0x7);
				sad->sample_rates |= (edid[i + 1] & 0x7F);
				sad->bit_rates |= (edid[i + 2] & 0x7);

				displayport_dbg("ch:0x%X, sample:0x%X, bitrate:0x%X\n",
					sad->channel_count, sad->sample_rates, sad->bit_rates);
			}
		} else if (type == VIDEO_DATA_BLOCK) {
			for (i = pos; i < pos + len; i++) {
				u8 vic = edid[i] & SVD_VIC_MASK;
				edid_find_preset_in_video_data_block(vic);
				displayport_dbg("EDID: Video data block vic:%d %s\n",
					vic, supported_videos[i].name);
			}
		} else if (type == SPEAKER_DATA_BLOCK) {
			sad->speaker |= edid[pos] & 0xff;
			displayport_dbg("EDID: speaker 0x%X\n", sad->speaker);
		}

		pos += len;
	}

	return 0;
}

void edid_check_detail_timing_desc1(struct fb_monspecs *specs, int modedb_len, u8 *edid)
{
	int i;
	struct fb_videomode *mode = NULL;
	u64 pixelclock = 0;
	struct displayport_device *displayport = get_displayport_drvdata();
	u8 *block = edid + DETAILED_TIMING_DESCRIPTIONS_START;

	for (i = 0; i < modedb_len; i++) {
		mode = &(specs->modedb[i]);
		if (mode->flag == (FB_MODE_IS_FIRST | FB_MODE_IS_DETAILED))
			break;
	}

	if (i >= modedb_len)
		return;

	mode = &(specs->modedb[i]);
	pixelclock = (u64)((u32)block[1] << 8 | (u32)block[0]) * 10000;

	displayport_info("detail_timing_desc1: %d*%d@%d (%lld, %dps)\n",
			mode->xres, mode->yres, mode->refresh, pixelclock, mode->pixclock);

	for (i = 0; i < supported_videos_pre_cnt; i++) {
		if (mode->vmode == FB_VMODE_NONINTERLACED &&
				(mode->refresh == supported_videos[i].fps ||
				 mode->refresh == supported_videos[i].fps - 1) &&
				mode->xres == supported_videos[i].dv_timings.bt.width &&
				mode->yres == supported_videos[i].dv_timings.bt.height) {
			if (supported_videos[i].edid_support_match == true) {
				displayport_info("already found timing:%d\n", i);
				return;
			} else
				break; /* matched but not found */
		}
	}

	/* check if index is valid and index is bigger than best video */
	if (i >= supported_videos_pre_cnt || i <= displayport->best_video) {
		displayport_info("invalid timing i:%d, best:%d\n", i, displayport->best_video);
		return;
	}

	/* check if found index does not support dex at dex mode */
	if (displayport->dex_setting &&
			(supported_videos[i].dex_support == DEX_NOT_SUPPORT ||
			 supported_videos[i].dex_support > displayport->dex_adapter_type)) {
		displayport_info("dex mode but not supported resolution(%d)\n", i);
		return;
	}

	displayport_info("find same supported timing: %d*%d@%d (%lld)\n",
			supported_videos[i].dv_timings.bt.width,
			supported_videos[i].dv_timings.bt.height,
			supported_videos[i].fps,
			supported_videos[i].dv_timings.bt.pixelclock);

	if (supported_videos[V640X480P60].dv_timings.bt.pixelclock >= pixelclock ||
			supported_videos[V4096X2160P60].dv_timings.bt.pixelclock <= pixelclock) {
		displayport_info("EDID: invalid pixel clock\n");
		return;
	}

	supported_videos[VDUMMYTIMING].dex_support = supported_videos[i].dex_support;
	supported_videos[VDUMMYTIMING].ratio = supported_videos[i].ratio;

	displayport->best_video = VDUMMYTIMING;
	supported_videos[VDUMMYTIMING].dv_timings.bt.width = mode->xres;
	supported_videos[VDUMMYTIMING].dv_timings.bt.height = mode->yres;
	supported_videos[VDUMMYTIMING].dv_timings.bt.interlaced = false;
	supported_videos[VDUMMYTIMING].dv_timings.bt.pixelclock = pixelclock;
	supported_videos[VDUMMYTIMING].dv_timings.bt.hfrontporch = mode->right_margin ? mode->right_margin : 1;
	supported_videos[VDUMMYTIMING].dv_timings.bt.hsync = mode->hsync_len;
	supported_videos[VDUMMYTIMING].dv_timings.bt.hbackporch = mode->left_margin ? mode->left_margin : 1;
	supported_videos[VDUMMYTIMING].dv_timings.bt.vfrontporch = mode->lower_margin ? mode->lower_margin : 1;
	supported_videos[VDUMMYTIMING].dv_timings.bt.vsync = mode->vsync_len;
	supported_videos[VDUMMYTIMING].dv_timings.bt.vbackporch = mode->upper_margin ? mode->upper_margin : 1;
	supported_videos[VDUMMYTIMING].fps = mode->refresh;
	/*  VSYNC bit and HSYNC bit is reversed at fbmon.c */
	supported_videos[VDUMMYTIMING].v_sync_pol = (mode->sync & FB_SYNC_HOR_HIGH_ACT) ? SYNC_POSITIVE : SYNC_NEGATIVE;
	supported_videos[VDUMMYTIMING].h_sync_pol = (mode->sync & FB_SYNC_VERT_HIGH_ACT) ? SYNC_POSITIVE : SYNC_NEGATIVE;

	supported_videos[VDUMMYTIMING].edid_support_match = true;
	preferred_preset = supported_videos[VDUMMYTIMING].dv_timings;

	displayport_dbg("EDID: modedb : %d*%d@%d (%lld)\n", mode->xres, mode->yres, mode->refresh,
			supported_videos[VDUMMYTIMING].dv_timings.bt.pixelclock);
	displayport_info("EDID: modedb : lm(%d) hs(%d) rm(%d)  um(%d) vs(%d) lm(%d)  %d %d pol(%d, v:%d, h:%d)\n",
			mode->left_margin, mode->hsync_len, mode->right_margin,
			mode->upper_margin, mode->vsync_len, mode->lower_margin,
			mode->vmode, mode->flag, mode->sync,
			supported_videos[VDUMMYTIMING].v_sync_pol,
			supported_videos[VDUMMYTIMING].h_sync_pol);
	displayport_dbg("EDID: %s edid_support_match = %d\n", supported_videos[VDUMMYTIMING].name,
			supported_videos[VDUMMYTIMING].edid_support_match);
}

void edid_check_test_device(struct displayport_device *displayport,
		struct fb_monspecs *specs)
{
	if (!strcmp(specs->monitor, "UNIGRAF TE") || !strcmp(specs->monitor, "UFG DPR-120")
		|| !strcmp(specs->monitor, "UCD-400 DP") || !strcmp(specs->monitor, "AGILENT ATR")
		|| !strcmp(specs->monitor, "UFG DP SINK")) {
		displayport->bist_used = 1;
		displayport_info("bist enable in %s\n", __func__);
	}
	if (displayport->forced_bist == 1 || displayport->forced_bist == 0) {
		displayport->bist_used = displayport->forced_bist;
		displayport_info("forced bist enable %d\n", displayport->bist_used);
	}	
}

#ifdef FEATURE_SUPPORT_DISPLAYID
static void edid_mode_displayid_detailed(struct displayid_detailed_timings_1 *timings)
{
	int i;
	struct displayport_device *displayport = get_displayport_drvdata();
	unsigned pixel_clock = (timings->pixel_clock[0] |
				(timings->pixel_clock[1] << 8) |
				(timings->pixel_clock[2] << 16));
	unsigned hactive = (timings->hactive[0] | timings->hactive[1] << 8) + 1;
	unsigned hblank = (timings->hblank[0] | timings->hblank[1] << 8) + 1;
	unsigned hsync = (timings->hsync[0] | (timings->hsync[1] & 0x7f) << 8) + 1;
	unsigned hsync_width = (timings->hsw[0] | timings->hsw[1] << 8) + 1;
	unsigned vactive = (timings->vactive[0] | timings->vactive[1] << 8) + 1;
	unsigned vblank = (timings->vblank[0] | timings->vblank[1] << 8) + 1;
	unsigned vsync = (timings->vsync[0] | (timings->vsync[1] & 0x7f) << 8) + 1;
	unsigned vsync_width = (timings->vsw[0] | timings->vsw[1] << 8) + 1;
	bool hsync_positive = (timings->hsync[1] >> 7) & 0x1;
	bool vsync_positive = (timings->vsync[1] >> 7) & 0x1;
	unsigned htotal = hactive + hblank;
	unsigned vtotal = vactive + vblank;
	unsigned fps = div_u64(((u64)pixel_clock * 10000), (htotal * vtotal));

	displayport_info("DisplayID %d(%d) %d %d %d %d %d %d %d %d %d %d\n",
			pixel_clock * 10,
			fps,
			hactive,
			hsync,
			hsync_width,
			hblank,
			vactive,
			vsync,
			vsync_width,
			vblank,
			hsync_positive,
			vsync_positive);

	for (i = 0; i < supported_videos_pre_cnt; i++) {
		if (supported_videos[i].displayid_timing == DISPLAYID_EXT &&
				(fps == supported_videos[i].fps ||
				 fps == supported_videos[i].fps - 1) &&
				hactive == supported_videos[i].dv_timings.bt.width &&
				vactive == supported_videos[i].dv_timings.bt.height) {
			displayport_info("found matched displayid timing %d\n", i);
			break;
		}
	}

	if (i < supported_videos_pre_cnt) {

		if (i > displayport->best_video)
			displayport->best_video = i;

		supported_videos[i].dv_timings.bt.width = hactive;
		supported_videos[i].dv_timings.bt.height = vactive;
		supported_videos[i].dv_timings.bt.interlaced = false;
		supported_videos[i].dv_timings.bt.pixelclock = pixel_clock * 10000;
		supported_videos[i].dv_timings.bt.hfrontporch = hblank / 2;
		supported_videos[i].dv_timings.bt.hsync = hsync;
		supported_videos[i].dv_timings.bt.hbackporch = hblank - (hblank / 2);
		supported_videos[i].dv_timings.bt.vfrontporch = vblank / 2;
		supported_videos[i].dv_timings.bt.vsync = vsync;
		supported_videos[i].dv_timings.bt.vbackporch = vblank - (vblank / 2);
		supported_videos[i].fps = fps;
		supported_videos[i].v_sync_pol = vsync_positive ? SYNC_POSITIVE : SYNC_NEGATIVE;
		supported_videos[i].h_sync_pol = hsync_positive ? SYNC_POSITIVE : SYNC_NEGATIVE;
		supported_videos[i].edid_support_match = true;
	}
}

static int edid_add_displayid_detailed_1_modes(struct displayid_block *block)
{
	struct displayid_detailed_timing_block *det = (struct displayid_detailed_timing_block *)block;
	int i;
	int num_timings;
	int num_modes = 0;
	/* blocks must be multiple of 20 bytes length */
	if (block->num_bytes % 20)
		return 0;

	num_timings = block->num_bytes / 20;
	for (i = 0; i < num_timings; i++) {
		struct displayid_detailed_timings_1 *timings = &det->timings[i];

		edid_mode_displayid_detailed(timings);
		/* add to support list */
		num_modes++;
	}
	return num_modes;
}

static int edid_validate_displayid(u8 *displayid, int length, int idx)
{
	int i;
	u8 csum = 0;
	struct displayid_hdr *base;

	base = (struct displayid_hdr *)&displayid[idx];

	displayport_info("DisplayId rev 0x%x, len %d, %d %d\n",
			base->rev, base->bytes, base->prod_id, base->ext_count);

	if (base->bytes + 5 > length - idx)
		return -EINVAL;
	for (i = idx; i <= base->bytes + 5; i++) {
		csum += displayid[i];
	}
	if (csum) {
		displayport_info("DisplayID checksum invalid, remainder is %d\n", csum);
		return -EINVAL;
	}

	return 0;
}

static u8 *edid_find_displayid_extension(u8 *edid, int blk_cnt)
{
	u8 *edid_ext = NULL;
	int i;

	if (blk_cnt < 1)
		return NULL;

	for (i = 0; i < blk_cnt; i++) {
		edid_ext = edid + EDID_BLOCK_SIZE * (i + 1);
		if (edid_ext[0] == DISPLAYID_EXT)
			break;
	}

	if (i == blk_cnt)
		return NULL;

	return edid_ext;
}

static int edid_add_displayid_detailed_modes(u8 *edid)
{
	u8 *displayid;
	int ret;
	int idx = 1;
	int length = EDID_BLOCK_SIZE;
	struct displayid_block *block;
	int num_modes = 0;
	int blk_cnt = 0;

	if (!edid)
		return 0;

	blk_cnt = edid[EDID_EXTENSION_FLAG];
	if (blk_cnt > 2)
		blk_cnt = 2;

	displayid = edid_find_displayid_extension(edid, blk_cnt);
	if (!displayid)
		return 0;

	ret = edid_validate_displayid(displayid, length, idx);
	if (ret)
		return 0;

	idx += sizeof(struct displayid_hdr);
	while (block = (struct displayid_block *)&displayid[idx],
	       idx + sizeof(struct displayid_block) <= length &&
	       idx + sizeof(struct displayid_block) + block->num_bytes <= length &&
	       block->num_bytes > 0) {
		idx += block->num_bytes + sizeof(struct displayid_block);
		switch (block->tag) {
		case DATA_BLOCK_TYPE_1_DETAILED_TIMING:
			num_modes += edid_add_displayid_detailed_1_modes(block);
			break;
		}
	}

	return num_modes;	
}
#endif

int edid_update(struct displayport_device *hdev)
{
	struct fb_monspecs specs;
	struct fb_vendor vsdb;
	struct fb_audio sad;
	u8 *edid = NULL;
	int block_cnt = 0;
	int i;
	int basic_audio = 0;
	int edid_test = 0;
	int modedb_len = 0;

	audio_channels = 0;
	audio_sample_rates = 0;
	audio_bit_rates = 0;
	audio_speaker_alloc = 0;

	edid_misc = 0;
	memset(&vsdb, 0, sizeof(vsdb));
	memset(&specs, 0, sizeof(specs));
	memset(&sad, 0, sizeof(sad));

	memset(hdev->edid_manufacturer, 0, sizeof(specs.manufacturer));
	hdev->edid_product = 0;
	hdev->edid_serial = 0;

	preferred_preset = supported_videos[EDID_DEFAULT_TIMINGS_IDX].dv_timings;
	supported_videos[0].edid_support_match = true; /*default support VGA*/
	supported_videos[VDUMMYTIMING].dv_timings.bt.width = 0;
	supported_videos[VDUMMYTIMING].dv_timings.bt.height = 0;
	for (i = 1; i < supported_videos_pre_cnt; i++)
		supported_videos[i].edid_support_match = false;

	if (hdev->do_unit_test)
		block_cnt = edid_read_unit(&edid);
	else if (hdev->edid_test_buf[0] == 1 || hdev->edid_test_buf[0] == 2) {
		edid_test = 1;
		edid = &hdev->edid_test_buf[1];
		block_cnt = hdev->edid_test_buf[0];
		displayport_info("using test edid %d\n", block_cnt);
	} else
		block_cnt = edid_read(hdev, &edid);
	if (block_cnt < 0) {
		hdev->bpc = BPC_6;
		goto out;
	}

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	secdp_bigdata_save_item(BD_EDID, edid);
#endif

	fb_edid_to_monspecs(edid, &specs);
	modedb_len = specs.modedb_len;

	strlcpy(hdev->mon_name, specs.monitor, MON_NAME_LEN);
	displayport_info("mon name: %s, gamma: %u.%u\n", specs.monitor,
			specs.gamma / 100, specs.gamma % 100);

	edid_check_test_device(hdev, &specs);

#ifdef CONFIG_SEC_DISPLAYPORT_BIGDATA
	secdp_bigdata_save_item(BD_SINK_NAME, specs.monitor);
#endif
	for (i = 1; i < block_cnt; i++)
		fb_edid_add_monspecs(edid + i * EDID_BLOCK_SIZE, &specs);

	/* find 2D preset */
	for (i = 0; i < specs.modedb_len; i++)
		edid_find_preset(&specs.modedb[i]);

	/* color depth */
	if (edid[EDID_COLOR_DEPTH] & 0x80) {
		if (((edid[EDID_COLOR_DEPTH] & 0x70) >> 4) == 1)
			hdev->bpc = BPC_6;
	}

	/* vendro block */
	memcpy(hdev->edid_manufacturer, specs.manufacturer, sizeof(specs.manufacturer));
	hdev->edid_product = specs.model;
	hdev->edid_serial = specs.serial;

	/* number of 128bytes blocks to follow */
	if (block_cnt <= 1)
		goto out;

	if (edid[EDID_NATIVE_FORMAT] & EDID_BASIC_AUDIO) {
		basic_audio = 1;
		edid_misc = FB_MISC_HDMI;
	}

	edid_parse_hdmi14_vsdb(edid + EDID_BLOCK_SIZE, &vsdb, block_cnt);
	edid_find_hdmi14_vsdb_update(&vsdb);

	edid_parse_hdmi20_vsdb(edid + EDID_BLOCK_SIZE, &vsdb, block_cnt);

	edid_parse_hdr_metadata(edid + EDID_BLOCK_SIZE, block_cnt);

	for (i = 1; i < block_cnt; i++)
		edid_parse_audio_video_db(edid + (EDID_BLOCK_SIZE * i), &sad);

	if (!edid_misc)
		edid_misc = specs.misc;

	if (edid_misc & FB_MISC_HDMI) {
		audio_speaker_alloc = sad.speaker;
		if (sad.channel_count) {
			audio_channels = sad.channel_count;
			audio_sample_rates = sad.sample_rates;
			audio_bit_rates = sad.bit_rates;
		} else if (basic_audio) {
			audio_channels = 2;
			audio_sample_rates = FB_AUDIO_48KHZ; /*default audio info*/
			audio_bit_rates = FB_AUDIO_16BIT;
		}
	}

	if (test_audio_info.channel_count != 0) {
		audio_channels = test_audio_info.channel_count;
		audio_sample_rates = test_audio_info.sample_rates;
		audio_bit_rates = test_audio_info.bit_rates;
		displayport_info("test audio ch:0x%X, sample:0x%X, bit:0x%X\n",
			audio_channels, audio_sample_rates, audio_bit_rates);
	}

	displayport_info("misc:0x%X, Audio ch:0x%X, sample:0x%X, bit:0x%X\n",
			edid_misc, audio_channels, audio_sample_rates, audio_bit_rates);

out:
#ifdef FEATURE_SUPPORT_DISPLAYID
	edid_add_displayid_detailed_modes(edid);
#endif

	edid_check_detail_timing_desc1(&specs, modedb_len, edid);

	/* No supported preset found, use default */
	if (forced_resolution >= 0) {
		displayport_info("edid_use_default_preset\n");
		edid_use_default_preset();
	}

	if (block_cnt == -EPROTO)
		edid_misc = FB_MISC_HDMI;

	if (!hdev->do_unit_test && !edid_test)
		kfree(edid);

	return block_cnt;
}

struct v4l2_dv_timings edid_preferred_preset(void)
{
	return preferred_preset;
}

bool edid_supports_hdmi(struct displayport_device *hdev)
{
	return edid_misc & FB_MISC_HDMI;
}

bool edid_support_pro_audio(void)
{
	if (audio_channels >= FB_AUDIO_8CH && audio_sample_rates >= FB_AUDIO_192KHZ)
		return true;

	return false;
}

u32 edid_audio_informs(void)
{
	struct displayport_device *displayport = get_displayport_drvdata();
	u32 value = 0, ch_info = 0;
	u32 link_rate = displayport_reg_phy_get_link_bw();

	if (audio_channels > 0)
		ch_info = audio_channels;

	/* support 192KHz sample freq only if current timing supports pro audio */
	if (supported_videos[displayport->cur_video].pro_audio_support &&
		link_rate >= LINK_RATE_5_4Gbps &&
		ch_info >= FB_AUDIO_8CH && audio_sample_rates >= FB_AUDIO_192KHZ) {
		displayport_info("support pro audio\n");
	} else {
		displayport_info("reduce sample freq to 48KHz(lr:0x%X, ch:0x%X, sf:0x%X)\n",
			link_rate, ch_info, audio_sample_rates);
		audio_sample_rates &= 0x7; /* reduce to 48KHz */
	}

	value = ((audio_sample_rates << 19) | (audio_bit_rates << 16) |
			(audio_speaker_alloc << 8) | ch_info);
	value |= (1 << 26); /* 1: DP, 0: HDMI */

	displayport_info("audio info = 0x%X\n", value);

	return value;
}

struct fb_audio *edid_get_test_audio_info(void)
{
	return &test_audio_info;
}

u8 edid_read_checksum(void)
{
	int ret, i;
	u8 buf[EDID_BLOCK_SIZE];
	u8 offset = EDID_OFFSET(0);
	int sum = 0;

	ret = displayport_reg_edid_read(offset, EDID_BLOCK_SIZE, buf);
	if (ret)
		return ret;

	for (i = 0; i < EDID_BLOCK_SIZE; i++)
		sum += buf[i];

	displayport_info("edid_read_checksum %02x, %02x", sum%265, buf[EDID_BLOCK_SIZE-1]);

	return buf[EDID_BLOCK_SIZE-1];
}
