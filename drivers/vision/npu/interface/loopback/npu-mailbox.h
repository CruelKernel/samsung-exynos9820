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

#ifndef PERI_MAILBOX_H_
#define PERI_MAILBOX_H_

//#include <mtasker.h>
//#include <base/type.h>

#define MAILBOX_VERSION

struct int_group {
	u32 g;
	u32 c;
	u32 m;
	u32 s;
	u32 ms;
};

struct mailbox {
	u32			mcuctl;
	u32			pad0;
	struct int_group	g0;
	struct int_group	g1;
	struct int_group	g2;
	struct int_group	g3;
	struct int_group	g4;
	u32			pad1;
	u32			version;
	u32			pad2[3];
	u32			shared[64];
};

struct mailbox_h2f_hdr {
	u32			data_ofs;
	u32			data_len;
	u32			wptr_ofs;
	u32			rptr_ofs;
};

struct mailbox_f2h_hdr {
	u32			data_ofs;
	u32			data_len;
	u32			wptr_ofs;
	u32			rptr_ofs;
};

struct mailbox_hdr {
	u32			signature;
	u32			stack_ofs;
	u32			version;
	u32			totsize;
	u32			reserved[10];
	struct mailbox_h2f_hdr	h2fbox[2];
	struct mailbox_f2h_hdr	f2hbox[2];
};

enum npu_mailbox_h2f_type {
	NPU_H2F_LOW,
	NPU_H2F_NORMAL,
	NPU_H2F_HIGH,
};

enum npu_mailbox_f2h_type {
	NPU_F2H_LOW,
	NPU_F2H_NORMAL,
	NPU_F2H_HIGH,
	NPU_F2H_REPORT,
	NPU_F2H_DEBUG,
	NPU_F2H_ERROR,
};

struct mailbox_ctrl {
	struct mailbox_hdr		*hdr;
	struct mailbox_stack	*stack;
};

void mailbox_s_interrupt(void);

int npu_mailbox_init(struct mailbox_ctrl *mBox);
int npu_mailbox_ready(struct mailbox_ctrl *mBox);
int npu_mailbox_read(struct mailbox_ctrl *mBox);
int npu_mailbox_write(struct mailbox_ctrl *mBox);

#endif


