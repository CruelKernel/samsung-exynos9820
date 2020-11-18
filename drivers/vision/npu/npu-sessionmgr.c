/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "npu-sessionmgr.h"

int npu_sessionmgr_probe(struct npu_sessionmgr *sessionmgr)
{
	int i = 0;
	mutex_init(&sessionmgr->mlock);

	for (i = 0; i < NPU_MAX_SESSION; i++) {
		sessionmgr->session[i] = NULL;
	}
	atomic_set(&sessionmgr->session_cnt, 0);

	return 0;
}

int npu_sessionmgr_open(struct npu_sessionmgr *sessionmgr)
{
	int ret = 0;

	return ret;
}

int npu_sessionmgr_close(struct npu_sessionmgr *sessionmgr)
{
	int ret = 0;


	return ret;
}

int npu_sessionmgr_start(struct npu_sessionmgr *sessionmgr)
{
	int ret = 0;

	return ret;
}

int npu_sessionmgr_stop(struct npu_sessionmgr *sessionmgr)
{
	int ret = 0;

	return ret;
}

int npu_sessionmgr_regID(struct npu_sessionmgr *sessionmgr, struct npu_session *session)
{
	int ret = 0;
	u32 index;

	BUG_ON(!sessionmgr);
	BUG_ON(!session);

	mutex_lock(&sessionmgr->mlock);

	for (index = 0; index < NPU_MAX_SESSION; index++) {
		if (!sessionmgr->session[index]) {
			sessionmgr->session[index] = session;
			session->uid = index;
			atomic_inc(&sessionmgr->session_cnt);
			break;
		}
	}

	if (index >= NPU_MAX_SESSION)
		ret = -EINVAL;

	mutex_unlock(&sessionmgr->mlock);
	session->global_lock = &sessionmgr->mlock;

	return ret;
}

void npu_sessionmgr_unregID(struct npu_sessionmgr *sessionmgr, struct npu_session *session)
{
	BUG_ON(!sessionmgr);
	BUG_ON(!session);

	mutex_lock(&sessionmgr->mlock);
	sessionmgr->session[session->uid] = NULL;
	atomic_dec(&sessionmgr->session_cnt);
	mutex_unlock(&sessionmgr->mlock);
}

