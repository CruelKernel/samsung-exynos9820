/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/firmware.h>

#include "score_log.h"
#include "score_mem_table.h"
#include "score_system.h"
#include "score_firmware.h"

#define SCORE_TS_PM_NAME	"score_ts_pmw.bin"
#define SCORE_TS_DM_NAME	"score_ts_dmb.bin"
#define SCORE_BR_PM_NAME	"score_br_pmw.bin"
#define SCORE_BR_DM_NAME	"score_br_dmb.bin"

static struct score_firmware_format score_firmwares[] = {
	{
		/* for develop and test */
		.name			= "develop-1",
		.id			= SCORE_FIRMWARE_DEVELOP1,
		.path			= "score/develop1",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		/* for develop and test */
		.name			= "develop-2",
		.id			= SCORE_FIRMWARE_DEVELOP2,
		.path			= "score/develop2",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		/* for develop and test */
		.name			= "develop-3",
		.id			= SCORE_FIRMWARE_DEVELOP3,
		.path			= "score/develop3",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-1",
		.id			= SCORE_FIRMWARE_CAMERA1,
		.path			= "score/camera1",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-2",
		.id			= SCORE_FIRMWARE_CAMERA2,
		.path			= "score/camera2",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-3",
		.id			= SCORE_FIRMWARE_CAMERA3,
		.path			= "score/camera3",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-4",
		.id			= SCORE_FIRMWARE_CAMERA4,
		.path			= "score/camera4",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-5",
		.id			= SCORE_FIRMWARE_CAMERA5,
		.path			= "score/camera5",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-6",
		.id			= SCORE_FIRMWARE_CAMERA6,
		.path			= "score/camera6",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-7",
		.id			= SCORE_FIRMWARE_CAMERA7,
		.path			= "score/camera7",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-8",
		.id			= SCORE_FIRMWARE_CAMERA8,
		.path			= "score/camera8",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}, {
		.name			= "camera-9",
		.id			= SCORE_FIRMWARE_CAMERA9,
		.path			= "score/camera9",
		.fw_load		= true,
		.ts = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.baron = {
			.text_mem_size	= SZ_512K,
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
	}
};

static struct score_firmware_format *__score_firmware_get_format(
		unsigned int id)
{
	if (ARRAY_SIZE(score_firmwares) < SCORE_FIRMWARE_END) {
		score_err("firmware table is unstable (%zu/%u)\n",
				ARRAY_SIZE(score_firmwares),
				SCORE_FIRMWARE_END);
		return ERR_PTR(-EINVAL);
	}

	if (id >= SCORE_FIRMWARE_END) {
		score_err("firmware id is invalid (%u)\n", id);
		return ERR_PTR(-EINVAL);
	}
	score_info("firmware(%s) is requested\n", score_firmwares[id].name);
	return &score_firmwares[id];
}

static int __score_firmware_get_dvaddr(struct score_firmware *fw,
		dma_addr_t *text, dma_addr_t *data)
{
	int ret = 0;

	score_enter();
	if (fw->text.dvaddr == 0 || fw->data.dvaddr == 0) {
		ret = -ENOMEM;
		score_err("dvaddr is not created(core[%d] text:%#8x,data:%8x)\n",
				fw->idx, (unsigned int)fw->text.dvaddr,
				(unsigned int)fw->data.dvaddr);
	} else {
		*text = fw->text.dvaddr;
		*data = fw->data.dvaddr;
	}

	score_leave();
	return ret;
}

/**
 * score_firmware_manager_get_dvaddr -
 *	Get dvaddr of text and data to deliver to SCore device
 * @fwmgr:	[in]	object about score_frame_manager structure
 * @core_id:	[in]	core id of SCore device
 * @text:	[out]	dvaddr for text area
 * @data:	[out]	dvaddr for data area
 *
 * Return 0 if succeeded, otherwise errno
 */
int score_firmware_manager_get_dvaddr(struct score_firmware_manager *fwmgr,
		int core_id, dma_addr_t *text, dma_addr_t *data)
{
	int ret = 0;
	struct score_firmware *fw;

	score_enter();
	if (core_id == SCORE_TS) {
		fw = &fwmgr->ts;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else if (core_id == SCORE_BARON1) {
		fw = &fwmgr->baron;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else if (core_id == SCORE_BARON2) {
		fw = &fwmgr->baron;
		if (fw->br2_data.dvaddr == 0) {
			ret = -ENOMEM;
			score_err("dvaddr is not created(br2_data:%8x)\n",
					(unsigned int)fw->data.dvaddr);
			goto p_end;
		}
		*data = fw->br2_data.dvaddr;
	} else {
		ret = -EINVAL;
		goto p_end;
	}

	score_leave();
p_end:
	return ret;
}

int score_firmware_load(struct score_firmware *fw)
{
	int ret = 0;
	const struct firmware *text;
	const struct firmware *data;

	score_enter();
	if (fw->fw_loaded)
		return 0;

	if (fw->text.kvaddr == NULL || fw->data.kvaddr == NULL) {
		ret = -ENOMEM;
		score_err("kvaddr is NULL(core[%d] %p/%p)\n",
				fw->idx, fw->text.kvaddr, fw->data.kvaddr);
		goto p_err;
	}

	ret = request_firmware(&text, fw->text.path, fw->dev);
	if (ret) {
		score_err("request_firmware(%s) is fail(%d)\n",
				fw->text.path, ret);
		goto p_err;
	}

	if (text->size > fw->text.mem_size) {
		score_err("firmware(%s) size is over(%zu > %zu)\n",
				fw->text.path, text->size, fw->text.mem_size);
		ret = -EIO;
		release_firmware(text);
		goto p_err;
	} else {
		fw->text.fw_size = text->size;
	}

	memcpy(fw->text.kvaddr, text->data, text->size);
	release_firmware(text);

	ret = request_firmware(&data, fw->data.path, fw->dev);
	if (ret) {
		score_err("request_firmware(%s) is fail(%d)\n",
				fw->data.path, ret);
		goto p_err;
	}

	if (data->size > (fw->data.mem_size - fw->data.stack)) {
		score_err("firmware(%s) size is over(%zu > (%zu - %zu))\n",
				fw->data.path, data->size,
				fw->data.mem_size, fw->data.stack);
		ret = -EIO;
		release_firmware(data);
		goto p_err;
	} else {
		fw->data.fw_size = data->size;
	}

	memcpy(fw->data.kvaddr + fw->data.stack, data->data, data->size);
	if (fw->idx == SCORE_BARON) {
		fw->br2_data.fw_size = data->size;
		memcpy(fw->br2_data.kvaddr + fw->br2_data.stack,
				data->data, data->size);
	}
	release_firmware(data);

	fw->fw_loaded = true;

	score_leave();
	return ret;
p_err:
	fw->fw_loaded = false;
	return ret;
}

static void __score_firmware_init(struct score_firmware_manager *fwmgr,
		struct score_firmware_format *fwf)
{
	struct score_firmware *fw;
	struct score_firmware_info *info;

	score_enter();
	fw = &fwmgr->ts;
	info = &fwf->ts;

	fw->fw_loaded = false;
	snprintf(fw->text.path, SCORE_FW_PATH_LEN,
			"%s/%s", fwf->path, SCORE_TS_PM_NAME);
	fw->text.mem_size = info->text_mem_size;

	snprintf(fw->data.path, SCORE_FW_PATH_LEN,
			"%s/%s", fwf->path, SCORE_TS_DM_NAME);
	fw->data.mem_size = info->data_mem_size;
	fw->data.stack = info->data_stack;

	fw = &fwmgr->baron;
	info = &fwf->baron;

	fw->fw_loaded = false;
	snprintf(fw->text.path, SCORE_FW_PATH_LEN,
			"%s/%s", fwf->path, SCORE_BR_PM_NAME);
	fw->text.mem_size = info->text_mem_size;

	snprintf(fw->data.path, SCORE_FW_PATH_LEN,
			"%s/%s", fwf->path, SCORE_BR_DM_NAME);
	fw->data.mem_size = info->data_mem_size;
	fw->data.stack = info->data_stack;

	fw->br2_data.mem_size = info->data_mem_size;
	fw->br2_data.stack = info->data_stack;

	score_leave();
}

static int __score_firmware_set_memory(struct score_firmware_manager *fwmgr,
		void *kvaddr, dma_addr_t dvaddr)
{
	struct score_firmware *ts, *baron;
	size_t tsize;

	score_enter();
	ts = &fwmgr->ts;
	baron = &fwmgr->baron;

	tsize = ts->text.mem_size + ts->data.mem_size +
		baron->text.mem_size + baron->data.mem_size +
		baron->br2_data.mem_size;
	if (tsize > SCORE_FW_SIZE) {
		score_err("Total firmware size is over (%zu > %d)\n",
				tsize, SCORE_FW_SIZE);
		return -EINVAL;
	}

	ts->text.kvaddr = kvaddr;
	ts->text.dvaddr = dvaddr;
	ts->data.kvaddr = ts->text.kvaddr + ts->text.mem_size;
	ts->data.dvaddr = ts->text.dvaddr + ts->text.mem_size;

	baron->text.kvaddr = ts->data.kvaddr + ts->data.mem_size;
	baron->text.dvaddr = ts->data.dvaddr + ts->data.mem_size;
	baron->data.kvaddr = baron->text.kvaddr + baron->text.mem_size;
	baron->data.dvaddr = baron->text.dvaddr + baron->text.mem_size;

	baron->br2_data.kvaddr = baron->data.kvaddr + baron->data.mem_size;
	baron->br2_data.dvaddr = baron->data.dvaddr + baron->data.mem_size;

	score_leave();
	return 0;
}

int score_firmware_manager_open(struct score_firmware_manager *fwmgr,
		unsigned int firmware_id, void *kvaddr, dma_addr_t dvaddr)
{
	int ret = 0;
	struct score_firmware_format *fwf;

	score_enter();
	if (!kvaddr || !dvaddr) {
		ret = -EINVAL;
		score_err("memory for firmware is invalid\n");
		goto p_err;
	}

	fwf = __score_firmware_get_format(firmware_id);
	if (IS_ERR(fwf)) {
		ret = PTR_ERR(fwf);
		goto p_err;
	}

	__score_firmware_init(fwmgr, fwf);

	ret = __score_firmware_set_memory(fwmgr, kvaddr, dvaddr);
	if (ret)
		goto p_err;

	ret = score_firmware_load(&fwmgr->ts);
	if (ret)
		goto p_err;

	ret = score_firmware_load(&fwmgr->baron);
	if (ret)
		goto p_err;

	score_leave();
p_err:
	return ret;
}

static void __score_firmware_deinit(struct score_firmware *fw)
{
	score_enter();
	fw->fw_loaded = false;

	fw->text.fw_size = 0;
	fw->text.kvaddr = NULL;
	fw->text.dvaddr = 0;

	fw->data.fw_size = 0;
	fw->data.kvaddr = NULL;
	fw->data.dvaddr = 0;
	score_leave();
}

void score_firmware_manager_close(struct score_firmware_manager *fwmgr)
{
	score_enter();
	__score_firmware_deinit(&fwmgr->baron);
	__score_firmware_deinit(&fwmgr->ts);
	score_leave();
}

int score_firmware_manager_probe(struct score_system *system)
{
	struct score_firmware_manager *fwmgr;
	struct device *dev;

	score_enter();
	fwmgr = &system->fwmgr;
	dev = system->dev;

	fwmgr->ts.dev = dev;
	fwmgr->ts.idx = SCORE_TS;

	fwmgr->baron.dev = dev;
	fwmgr->baron.idx = SCORE_BARON;

	score_leave();
	return 0;
}

void score_firmware_manager_remove(struct score_firmware_manager *fwmgr)
{
	score_check();
}
