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

#define MAILBOX_VERSION		4
#define MAILBOX_SIGNATURE1		0x0C0FFEE0
#define MAILBOX_SIGNATURE2		0xC0DEC0DE
#define MAILBOX_SHARED_MAX		64

#define MAILBOX_H2FCTRL_LPRIORITY	0
#define MAILBOX_H2FCTRL_HPRIORITY	1
#define MAILBOX_H2FCTRL_MAX		2

#define MAILBOX_F2HCTRL_RESPONSE	0
#define MAILBOX_F2HCTRL_REPORT		1
#define MAILBOX_F2HCTRL_MAX		2

#define MAILBOX_GROUP_LPRIORITY	0
#define MAILBOX_GROUP_HPRIORITY	2
#define MAILBOX_GROUP_RESPONSE		3
#define MAILBOX_GROUP_REPORT		4
#define MAILBOX_GROUP_MAX		5
/*
 *  +-----------------------------+ ------------
 *  |      Mailbox #4             |          |
 *  +-----------------------------+          |
 *  |      Mailbox #3             |          |
 *  |                             |          |
 *  +-----------------------------+          |
 *  |      Mailbox #2             |         16K
 *  |                             |      (= NPU_MAILBOX_SIZE)
 *  +-----------------------------+          |
 *  |      Mailbox #1             |          |
 *  |                             |          |
 *  +-----------------------------+  ----    |
 *  |       Padding               |   |      |
 *  +-----------------------------+   4K     |
 *  |    struct mailbox_hdr       |   |      |
 *  +-----------------------------+  ----------- <- npu_mailbox->mbox_base by Eung ju kim
 */

/* Mailbox HW */
/* group0 : low priority
 * group1 : reserved for score
 * group2 : high priority
 * group3 : response
 * group4 : log
 */
struct intr_group {
	u32 g; /* Interrupt Generation Register */
	u32 c; /* Interrupt Clear Register */
	u32 m; /* Interrupt Mask register */
	u32 s; /* Interrupt Status Register */
	u32 ms; /* Interrupt Mask Status Register */
};

struct mailbox_sfr {
	u32			mcuctl;
	u32			pad0;
	struct intr_group	grp[MAILBOX_GROUP_MAX];
	u32			pad1;
	u32			version;
	u32			pad2[3];
	u32			shared[MAILBOX_SHARED_MAX];
};

struct mailbox_ctrl {
	u32			sgmt_ofs; /* minus offset from SRAM END(0x80000) */
	u32			sgmt_len; /* plus length in byte from sgmt_ofs */
	u32			wptr;
	u32			rptr;
};

struct mailbox_hdr {
	u32			max_slot; /* the maximum number of ncp object slot */
	u32			debug_time; /* start time in micro second at boot */
	u32			debug_code; /* firmware boot progress */
	u32			log_level; /* this means the time when read trigger is generated on report mailbox */
	u32			reserved[9];
	struct mailbox_ctrl	h2fctrl[MAILBOX_H2FCTRL_MAX];
	struct mailbox_ctrl	f2hctrl[MAILBOX_F2HCTRL_MAX];
	u32			totsize;
	u32			version;	/* half low 16 bits is mailbox ipc version, half high 16 bits is command version */
	u32			signature2; /* host should update this after host configuration is done */
	u32			signature1; /* firmware should update this after firmware initialization is done */
};

struct MAILBOX {
	volatile struct mailbox_sfr *sfr;
	struct uk_event *event[MAILBOX_H2FCTRL_MAX];
};

//void mailbox_init(u32 addr);
void mailbox_send_interrupt(u32 idx);

#endif /* PERI_MAILBOX_H_ */
