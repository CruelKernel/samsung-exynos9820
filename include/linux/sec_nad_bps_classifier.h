/*
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#ifndef SEC_NAD_BPS_CLASSIFIER_H
#define SEC_NAD_BPS_CLASSIFIER_H

#if defined(CONFIG_SEC_FACTORY)
/*
  * very important !!!
  * if the nad partition size has changed it must be change as well.
  */
#define NAD_BPS_REFER_PARTITION_SIZE	(1 * 512 * 1024)

#define NAD_BPS_MAX_QUICKBUILD_LEN	10
#define NAD_BPS_INIT_MAGIC1				0xABCDABCD
#define NAD_BPS_CLASSIFIER_MAGIC2		0xDCBADCBA

/* a structure for storing upload information for fac binary */
struct upload_info {
	int sp;
	int wp;
	int dp;
	int kp;
	int mp;
	int tp;
	int cp;
};

struct bps_info {
	/* magic code for bps */
	unsigned int magic[2];

	/* factory upload info */
	struct upload_info up_cnt;

	/* BPS filter count */
	int pc_lr_cnt;
	int pc_lr_last_idx;
	int bus_cnt;
	int smu_cnt;
	int klg_cnt;

	/* binary download info */
	int dn_cnt;
	char build_id[NAD_BPS_MAX_QUICKBUILD_LEN];
};

static struct bps_info sec_nad_bps_env;
static bool sec_nad_bps_env_initialized;

#endif
#endif
