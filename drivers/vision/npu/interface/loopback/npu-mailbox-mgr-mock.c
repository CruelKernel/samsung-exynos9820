/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>

#include "npu-common.h"
#include "npu-log.h"
#include "npu-if-protodrv-mbox.h"
#include "npu-config.h"
#include "npu-util-autosleepthr.h"
#include "npu-device.h"
#include "npu-mailbox-mgr-mock.h"

#define	MBOX_MOCK_KEEP_FRAME_ID		3624
#define MBOX_MOCK_RESUME_FRAME_ID	3824

struct mailbox_mgr_mock {
	struct npu_if_protodrv_mbox_ctx *if_protodrv_ctx;
	struct auto_sleep_thread		thr;
	struct auto_sleep_thread_param		thr_param;
} mailbox_mgr_mock;


static int mailbox_mgr_mock_do_task(struct auto_sleep_thread_param *data)
{
	struct npu_nw nw_request;
	struct npu_frame frame_request;
	int ret = 0;

	static int is_keeping;
	static struct npu_frame frame_keeping;

	/* Frame request */
	while (LLQ_GET(mailbox_mgr_mock.if_protodrv_ctx->mbox_frame_req, &frame_request) > 0) {
		/* Set result and put it back to result */
		frame_request.result_code = 0;
		npu_dbg("processed frame at mbox_mock: uid=%u, frame_id=%u, req_id=%u, IO=(%p,%p)",
			frame_request.uid, frame_request.frame_id,
			frame_request.npu_req_id, frame_request.input, frame_request.output);

		if (frame_request.frame_id == MBOX_MOCK_KEEP_FRAME_ID) {
			/* Keep the frame, until got the RESUME ID */
			is_keeping = 1;
			frame_keeping = frame_request;
		} else {
			while (LLQ_PUT(mailbox_mgr_mock.if_protodrv_ctx->mbox_frame_resp, frame_request) == 0) {
				/* Wait until the output queue is available */
				msleep(1);
			}
		}

		if (frame_request.frame_id == MBOX_MOCK_RESUME_FRAME_ID) {
			/* Return kept frame */
			if (is_keeping) {
				while (LLQ_PUT(mailbox_mgr_mock.if_protodrv_ctx->mbox_frame_resp, frame_keeping) == 0) {
					/* Wait until the output queue is available */
					msleep(1);
				}
			} else {
				npu_err("No kept frame");
			}
		}
		ret++;
	}

	/* Network request */
	while (LLQ_GET(mailbox_mgr_mock.if_protodrv_ctx->mbox_nw_req, &nw_request) > 0) {
		/* Set result and put it back to result */
		nw_request.result_code = 0;
		npu_dbg("processed NW at mbox_mock:	uid=%u, frame_id=%u, req_id=%u, ncp_hdr@=%p",
			nw_request.uid, nw_request.frame_id,
			nw_request.npu_req_id, nw_request.ncp_info.kvaddr);
		while (LLQ_PUT(mailbox_mgr_mock.if_protodrv_ctx->mbox_nw_resp, nw_request) == 0) {
			/* Wait until the output queue is available */
			msleep(1);
		}
		ret++;
	}

	return ret;
}

int mailbox_mgr_mock_check_work(struct auto_sleep_thread_param *data)
{
	return !LLQ_IS_EMPTY(mailbox_mgr_mock.if_protodrv_ctx->mbox_nw_req)
	       || !LLQ_IS_EMPTY(mailbox_mgr_mock.if_protodrv_ctx->mbox_frame_req);
}

static void mailbox_mgr_mock_signal(void)
{
	auto_sleep_thread_signal(&mailbox_mgr_mock.thr);
}


int mailbox_mgr_mock_probe(struct npu_device *npu_device)
{
	probe_info("start in mailbox_mgr_mock_probe\n");
	probe_info("complete in mailbox_mgr_mock_probe\n");
	return 0;
}

int mailbox_mgr_mock_release(void)
{
	probe_info("start in mailbox_mgr_mock_release\n");
	probe_info("complete in mailbox_mgr_mock_release\n");
	return 0;
}

int mailbox_mgr_mock_open(struct npu_device *npu_device)
{
	int ret = 0;

	npu_info("start in mailbox_mgr_mock_open\n");

	/* Initialize interface with mailbox manager */
	mailbox_mgr_mock.if_protodrv_ctx = npu_if_protodrv_mbox_ctx_open();
	if (!mailbox_mgr_mock.if_protodrv_ctx) {
		npu_err("init failed in npu_if_protodrv_mbox_ctx ");
		ret = -EFAULT;
		goto err_exit;
	}

	LLQ_REGISTER_TASK(mailbox_mgr_mock.if_protodrv_ctx->mbox_nw_req,
			  mailbox_mgr_mock_signal);
	LLQ_REGISTER_TASK(mailbox_mgr_mock.if_protodrv_ctx->mbox_frame_req,
			  mailbox_mgr_mock_signal);

	/* Initialize and run the mbox mock */
	ret = auto_sleep_thread_create(&mailbox_mgr_mock.thr, "mailbox_mgr_mock"
				       , mailbox_mgr_mock_do_task, mailbox_mgr_mock_check_work);
	if (ret) {
		return -1;
	}

	ret = auto_sleep_thread_start(&mailbox_mgr_mock.thr, mailbox_mgr_mock.thr_param);
	if (ret) {
		return -2;
	}

	npu_info("complete in mailbox_mgr_mock_open\n");
	return 0;

err_exit:
	return ret;
}

int mailbox_mgr_mock_close(struct npu_device *npu_device)
{
	npu_info("start in mailbox_mgr_mock_close\n");

	npu_info("stop AST in mailbox_mgr_mock_close\n");
	auto_sleep_thread_terminate(&mailbox_mgr_mock.thr);

	npu_info("closing npu_if_protodrv_mbox in mailbox_mgr_mock_close.\n");
	npu_if_protodrv_mbox_ctx_close();
	mailbox_mgr_mock.if_protodrv_ctx = NULL;

	npu_info("complete in mailbox_mgr_mock_close\n");
	return 0;
}

int mailbox_mgr_mock_start(struct npu_device *npu_device)
{
	npu_info("start in mailbox_mgr_mock_start\n");
	npu_info("complete in mailbox_mgr_mock_start\n");
	return 0;
}

int mailbox_mgr_mock_stop(struct npu_device *npu_device)
{
	npu_info("start in mailbox_mgr_mock_stop\n");
	npu_info("complete in mailbox_mgr_mock_stop\n");
	return 0;
}
