/*
 *	Copyright(c) 2013 FCI Inc. All Rights Reserved
 *
 *	File name : fc8080_demux.c
 *
 *	Description : fc8080 TSIF demux source file
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *	History :
 *	----------------------------------------------------------------------
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include "fci_types.h"
#include "fc8080_regs.h"

/* sync byte */
#define SYNC_MASK_FIC           0x80
#define SYNC_MASK_NVIDEO        0xc0
#define SYNC_MASK_VIDEO         0x47
#define SYNC_MASK_VIDEO1        0xb8

/* packet indicator */
#define PKT_IND_NONE            0x00
#define PKT_IND_END             0x20
#define PKT_IND_CONTINUE        0x40
#define PKT_IND_START           0x80
#define PKT_IND_MASK            0xe0

/* data size */
#define FIC_DATA_SIZE           (FIC_BUF_LENGTH / 2)
#define VIDEO_DATA_SIZE         (CH0_BUF_LENGTH / 2) /* the maximum size */
#define NON_VIDEO_DATA_SIZE     (CH0_BUF_LENGTH / 2) /* the maximum size */

#define TS_FRAME_SIZE           188

/* TS service information */
struct TS_FRAME_INFO {
	u8  ind;
	u16 length;     /* current receiving length */
	u8  subch_id;   /* subch id */
	u8  *buffer;
};

/* TS frame header information */
struct TS_FRAME_HDR {
	u8  sync;
	u8  ind;
	u16 length;
	u8  *data;
};

static u32 fic_user_data;
static u32 msc_user_data;
static int (*fic_callback)(u32 userdata, u8 *data, int length);
static int (*msc_callback)(u32 userdata, u8 subch_id, u8 *data, int length);
static u8 video_data[2][VIDEO_DATA_SIZE];
static u8 fic_data[FIC_DATA_SIZE];
static u8 non_video_data[4][NON_VIDEO_DATA_SIZE];

struct TS_FRAME_INFO video_frame_info[2] = {
	/* Video 0 */
	{
		0, 0, 0xff, (u8 *) &video_data[0]
	},
	/* Video 1 */
	{
		0, 0, 0xff, (u8 *) &video_data[1]
	}
};

struct TS_FRAME_INFO fic_frame_info = {
	0, 0, 0xff, (u8 *) &fic_data[0]
};

struct TS_FRAME_INFO non_video_frame_info[4] = {
	{0, 0, 0xff, (u8 *) non_video_data[0]},
	{0, 0, 0xff, (u8 *) non_video_data[1]},
	{0, 0, 0xff, (u8 *) non_video_data[2]},
	{0, 0, 0xff, (u8 *) non_video_data[3]}
};

/* mapping buf id to subch id */
static u8 video_subch[2] = {
	0xff, 0xff
};

/* mapping subch id to buf id */
static u8 non_video_subch[64] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};

static int gether_fic(u8 *data)
{
	struct TS_FRAME_HDR header;
	u16 len;

	/*header.sync = data[0];*/
	header.ind = data[1];
	header.length = (data[2] << 8) | data[3];
	header.data = &data[4];

	/* current real length */
	len = (header.length > 184) ? 184 : header.length;

	switch (header.ind) {
	case PKT_IND_START:
		fic_frame_info.ind = header.ind;
		fic_frame_info.length = 0;
		memcpy((void *) &fic_frame_info.buffer[0], header.data, len);
		fic_frame_info.length = len;
		return BBM_OK;
	case PKT_IND_CONTINUE:
		if (fic_frame_info.ind != PKT_IND_START) {
			fic_frame_info.ind = PKT_IND_NONE;
			return BBM_E_MUX_INDICATOR;
		}

		fic_frame_info.ind = header.ind;
		memcpy((void *) &fic_frame_info.buffer[fic_frame_info.length],
							header.data, len);
		fic_frame_info.length += len;
		return BBM_OK;
	case PKT_IND_END:
		if (fic_frame_info.ind != PKT_IND_CONTINUE) {
			fic_frame_info.ind = PKT_IND_NONE;
			return BBM_E_MUX_INDICATOR;
		}

		fic_frame_info.ind = header.ind;
		memcpy((void *) &fic_frame_info.buffer[fic_frame_info.length],
							header.data, len);
		fic_frame_info.length += len;
		break;
	default:
		return BBM_E_MUX_INDICATOR;
	}

	if ((fic_frame_info.length >= FIC_DATA_SIZE) && (fic_frame_info.ind ==
								PKT_IND_END)) {
		if (fic_callback)
			(*fic_callback)(fic_user_data, fic_frame_info.buffer,
						fic_frame_info.length);

		fic_frame_info.length = 0;
		fic_frame_info.ind = PKT_IND_NONE;
	}

	return BBM_OK;
}

static int gether_non_video(u8 *data)
{
	struct TS_FRAME_HDR header;
	u16 len;
	u8 subch;
	u8 buf_id;
	u16 data_sz;

	header.sync = data[0];
	header.ind = data[1];
	header.length = (data[2] << 8) | data[3];
	header.data = &data[4];

	len = (header.length >= 184) ? 184 : header.length;
	subch = header.sync & 0x3f;
	buf_id = non_video_subch[subch];

	if (buf_id == 0xff)
		return BBM_E_MUX_SUBCHANNEL;

	if (buf_id > 3)
		return BBM_E_MUX_SUBCHANNEL;

	switch (header.ind) {
	case PKT_IND_START:
		non_video_frame_info[buf_id].length = 0;

		memcpy((void *) &non_video_frame_info[buf_id].buffer[0],
						header.data, len);
		non_video_frame_info[buf_id].length += len;

		if (non_video_frame_info[buf_id].length < header.length)
			non_video_frame_info[buf_id].ind = header.ind;
		else
			non_video_frame_info[buf_id].ind = PKT_IND_END;
		break;
	case PKT_IND_CONTINUE:
		data_sz = non_video_frame_info[buf_id].length;

		if (data_sz + len > NON_VIDEO_DATA_SIZE)
			return BBM_E_MUX_DATA_SIZE;

		memcpy((void *) &non_video_frame_info[buf_id].buffer[data_sz],
							header.data, len);
		non_video_frame_info[buf_id].length += len;
		non_video_frame_info[buf_id].ind = header.ind;
		return BBM_OK;
	case PKT_IND_END:
		data_sz = non_video_frame_info[buf_id].length;

		if (data_sz + len > NON_VIDEO_DATA_SIZE)
			return BBM_E_MUX_DATA_SIZE;

		memcpy((void *) &non_video_frame_info[buf_id].buffer[data_sz],
							header.data, len);
		non_video_frame_info[buf_id].length += len;
		non_video_frame_info[buf_id].ind = header.ind;
		break;
	default:
		return BBM_E_MUX_INDICATOR;
	}

	if ((header.ind == PKT_IND_END) || (header.ind == PKT_IND_START &&
						header.length <= 184)) {
		if (msc_callback)
			(*msc_callback)(msc_user_data, subch,
					&non_video_frame_info[buf_id].buffer[0],
					non_video_frame_info[buf_id].length);

		non_video_frame_info[buf_id].length = 0;
		non_video_frame_info[buf_id].ind = PKT_IND_NONE;
	}

	return BBM_OK;
}

static int gether_video(u8 *data)
{
	u8 subch;
	u8 buf_id;
	u16 data_sz;

	switch (data[0]) {
	case SYNC_MASK_VIDEO:
		buf_id = 0;
		subch = video_subch[0];
		break;
	case SYNC_MASK_VIDEO1:
		buf_id = 1;
		data[0] = 0x47;
		subch = video_subch[1];
		break;
	default:
		return BBM_E_MUX_DATA_MASK;
	}

	if (subch == 0xff)
		return BBM_E_MUX_SUBCHANNEL;

	if (subch > 64)
		return BBM_E_MUX_SUBCHANNEL;

	data_sz = video_frame_info[buf_id].length;

	if (data_sz + TS_FRAME_SIZE > VIDEO_DATA_SIZE)
		return BBM_E_MUX_DATA_SIZE;

	memcpy((void *) (video_frame_info[buf_id].buffer + data_sz), data, 188);
	video_frame_info[buf_id].length += TS_FRAME_SIZE;

	if (video_frame_info[buf_id].length >= VIDEO_DATA_SIZE) {
		if (msc_callback)
			(*msc_callback)(msc_user_data, subch,
					video_frame_info[buf_id].buffer,
					video_frame_info[buf_id].length);
		video_frame_info[buf_id].length = 0;
	}

	return BBM_OK;
}

int fc8080_demux(u8 *data, u32 length)
{
	u32 i;

	for (i = 0; i < length; i += TS_FRAME_SIZE) {
		if (data[i] == SYNC_MASK_FIC)
			gether_fic(&data[i]);
		else if ((data[i] == SYNC_MASK_VIDEO) || (data[i] ==
						SYNC_MASK_VIDEO1))
			gether_video(&data[i]);
		else if (((data[i] & 0xc0) == 0xc0) || ((data[i] & 0xc0) ==
						0x00))
			gether_non_video(&data[i]);
	}

	return BBM_OK;
}

int fc8080_demux_fic_callback_register(u32 userdata,
	int (*callback)(u32 userdata, u8 *data, int length))
{
	fic_user_data = userdata;
	fic_callback = callback;

	return BBM_OK;
}

int fc8080_demux_msc_callback_register(u32 userdata,
	int (*callback)(u32 userdata, u8 subch_id, u8 *data, int length))
{
	msc_user_data = userdata;
	msc_callback = callback;

	return BBM_OK;
}


int fc8080_demux_select_video(u8 subch_id, u8 buf_id)
{
	video_subch[buf_id] = subch_id & 0x3f;

	return BBM_OK;
}

int fc8080_demux_select_channel(u8 subch_id, u8 buf_id)
{
	non_video_subch[subch_id & 0x3f] = buf_id;

	return BBM_OK;
}

int fc8080_demux_deselect_video(u8 subch_id, u8 buf_id)
{
	video_subch[buf_id] = 0xff;

	return BBM_OK;
}

int fc8080_demux_deselect_channel(u8 subch_id, u8 buf_id)
{
	non_video_subch[subch_id & 0x3f] = 0xff;

	return BBM_OK;
}
