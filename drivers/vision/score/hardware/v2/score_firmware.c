/*
 * Samsung Exynos SoC series SCore driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
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

static struct score_firmware_format score_firmwares[] = {
	{
		/* for develop and test */
		.name			= "develop-1",
		.id			= SCORE_FIRMWARE_DEVELOP1,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/develop1/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/develop1/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/develop1/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/develop1/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* for develop and test */
		.name			= "develop-2",
		.id			= SCORE_FIRMWARE_DEVELOP2,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/develop2/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/develop2/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/develop2/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/develop2/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* for develop and test */
		.name			= "develop-3",
		.id			= SCORE_FIRMWARE_DEVELOP3,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/develop3/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/develop3/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/develop3/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/develop3/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		.name			= "camera-1",
		.id			= SCORE_FIRMWARE_CAMERA1,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera1/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera1/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera1/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera1/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-2",
		.id			= SCORE_FIRMWARE_CAMERA2,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera2/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera2/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera2/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera2/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-3",
		.id			= SCORE_FIRMWARE_CAMERA3,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera3/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera3/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera3/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera3/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-4",
		.id			= SCORE_FIRMWARE_CAMERA4,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera4/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera4/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera4/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera4/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-5",
		.id			= SCORE_FIRMWARE_CAMERA5,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera5/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera5/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera5/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera5/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-6",
		.id			= SCORE_FIRMWARE_CAMERA6,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera6/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera6/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera6/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera6/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-7",
		.id			= SCORE_FIRMWARE_CAMERA7,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera7/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera7/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera7/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera7/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-8",
		.id			= SCORE_FIRMWARE_CAMERA8,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera8/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera8/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera8/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera8/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
	}, {
		/* reserved */
		.name			= "camera-9",
		.id			= SCORE_FIRMWARE_CAMERA9,
		.fw_load		= true,
		.mc = {
			.text_path	= "score/camera9/score_mc_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera9/score_mc_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.kc1 = {
			.text_path	= "score/camera9/score_kc1_pmw.bin",
			.text_mem_size	= SZ_512K,
			.data_path	= "score/camera9/score_kc1_dmb.bin",
			.data_mem_size	= SZ_256K,
			.data_stack	= SZ_32K,
		},
		.duplication		= true,
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
	if (core_id == SCORE_MASTER) {
		fw = &fwmgr->master;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else if (core_id == SCORE_KNIGHT1) {
		fw = &fwmgr->knight1;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else if (core_id == SCORE_KNIGHT2) {
		fw = &fwmgr->knight2;
		ret = __score_firmware_get_dvaddr(fw, text, data);
	} else {
		ret = -EINVAL;
	}

	score_leave();
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
	release_firmware(data);

	fw->fw_loaded = true;

	score_leave();
	return ret;
p_err:
	fw->fw_loaded = false;
	return ret;
}

int score_firmware_copy(struct score_firmware *dst, struct score_firmware *src)
{
	int ret = 0;

	score_enter();
	if (dst->fw_loaded)
		return 0;

	if (dst->text.kvaddr == NULL || dst->data.kvaddr == NULL) {
		ret = -ENOMEM;
		score_err("kvaddr of dst_fw is NULL(core[%d] %p/%p)\n",
				dst->idx, dst->text.kvaddr,
				dst->data.kvaddr);
		goto p_err;
	}

	if (src->text.kvaddr == NULL || src->data.kvaddr == NULL) {
		ret = -ENOMEM;
		score_err("kvaddr of src_fw is NULL(core[%d] %p/%p)\n",
				src->idx, src->text.kvaddr,
				src->data.kvaddr);
		goto p_err;
	}

	if (dst->text.mem_size < src->text.fw_size) {
		ret = -EIO;
		score_err("src_fw(%s) size is over(%zu < %zu)\n",
				src->text.path, dst->text.mem_size,
				src->text.fw_size);
		goto p_err;
	} else {
		dst->text.fw_size = src->text.fw_size;
	}

	memcpy(dst->text.kvaddr, src->text.kvaddr, src->text.fw_size);

	if ((dst->data.mem_size - dst->data.stack) < src->data.fw_size) {
		ret = -EIO;
		score_err("src_fw(%s) size is over((%zu - %zu) < %zu)\n",
				src->data.path, dst->data.mem_size,
				dst->data.stack, src->data.fw_size);
		goto p_err;
	} else {
		dst->data.fw_size = src->data.fw_size;
	}

	memcpy(dst->data.kvaddr + dst->data.stack,
			src->data.kvaddr + src->data.stack, src->data.fw_size);

	dst->fw_loaded = true;

	score_leave();
	return ret;
p_err:
	dst->fw_loaded = false;
	return ret;
}

static void __score_firmware_init(struct score_firmware *fw,
		struct score_firmware_info *fw_info)
{
	score_enter();
	fw->fw_loaded = false;

	snprintf(fw->text.path, SCORE_FW_PATH_LEN, "%s", fw_info->text_path);
	fw->text.mem_size = fw_info->text_mem_size;

	snprintf(fw->data.path, SCORE_FW_PATH_LEN, "%s", fw_info->data_path);
	fw->data.mem_size = fw_info->data_mem_size;
	fw->data.stack = fw_info->data_stack;

	score_leave();
}

static void __score_firmware_set_memory(struct score_firmware_manager *fwmgr,
		void *kvaddr, dma_addr_t dvaddr)
{
	struct score_firmware *mc, *kc1, *kc2;

	score_enter();
	mc = &fwmgr->master;
	kc1 = &fwmgr->knight1;
	kc2 = &fwmgr->knight2;

	mc->text.kvaddr = kvaddr;
	mc->text.dvaddr = dvaddr;
	mc->data.kvaddr = mc->text.kvaddr + mc->text.mem_size;
	mc->data.dvaddr = mc->text.dvaddr + mc->text.mem_size;

	kc1->text.kvaddr = mc->data.kvaddr + mc->data.mem_size;
	kc1->text.dvaddr = mc->data.dvaddr + mc->data.mem_size;
	kc1->data.kvaddr = kc1->text.kvaddr + kc1->text.mem_size;
	kc1->data.dvaddr = kc1->text.dvaddr + kc1->text.mem_size;

	kc2->text.kvaddr = kc1->data.kvaddr + kc1->data.mem_size;
	kc2->text.dvaddr = kc1->data.dvaddr + kc1->data.mem_size;
	kc2->data.kvaddr = kc2->text.kvaddr + kc2->text.mem_size;
	kc2->data.dvaddr = kc2->text.dvaddr + kc2->text.mem_size;

	score_leave();
}

int score_firmware_manager_open(struct score_firmware_manager *fwmgr,
		unsigned int firmware_id, void *kvaddr, dma_addr_t dvaddr)
{
	int ret = 0;
	struct score_firmware_format *fmt;

	score_enter();
	if (!kvaddr || !dvaddr) {
		ret = -EINVAL;
		score_err("memory for firmware is invalid\n");
		goto p_err;
	}

	fmt = __score_firmware_get_format(firmware_id);
	if (IS_ERR(fmt)) {
		ret = PTR_ERR(fmt);
		goto p_err;
	}

	__score_firmware_init(&fwmgr->master, &fmt->mc);
	__score_firmware_init(&fwmgr->knight1, &fmt->kc1);

	if (fmt->duplication)
		__score_firmware_init(&fwmgr->knight2, &fmt->kc1);
	else
		__score_firmware_init(&fwmgr->knight2, &fmt->kc2);

	__score_firmware_set_memory(fwmgr, kvaddr, dvaddr);

	ret = score_firmware_load(&fwmgr->master);
	if (ret)
		goto p_err;

	ret = score_firmware_load(&fwmgr->knight1);
	if (ret)
		goto p_err;

	if (fmt->duplication)
		ret = score_firmware_copy(&fwmgr->knight2, &fwmgr->knight1);
	else
		ret = score_firmware_load(&fwmgr->knight2);
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
	__score_firmware_deinit(&fwmgr->knight2);
	__score_firmware_deinit(&fwmgr->knight1);
	__score_firmware_deinit(&fwmgr->master);
	score_leave();
}

int score_firmware_manager_probe(struct score_system *system)
{
	struct score_firmware_manager *fwmgr;
	struct device *dev;

	score_enter();
	fwmgr = &system->fwmgr;
	dev = system->dev;

	fwmgr->master.dev = dev;
	fwmgr->master.idx = SCORE_MASTER;

	fwmgr->knight1.dev = dev;
	fwmgr->knight1.idx = SCORE_KNIGHT1;

	fwmgr->knight2.dev = dev;
	fwmgr->knight2.idx = SCORE_KNIGHT2;

	score_leave();
	return 0;
}

void score_firmware_manager_remove(struct score_firmware_manager *fwmgr)
{
	score_enter();
	score_leave();
}
