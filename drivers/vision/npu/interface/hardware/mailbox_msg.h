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

/*
 PURGE : Intend of this command is clean-up a session in device driver.
 Firmware shoud immediatly clean-up all the outstanting requests(Both processing and
 network management) for specified uid. Firmware should response with DONE
 message after the clean-up is completed. If the clean-up was unsuccessful,
 the firmware will resonse NDONE message and driver will forcefully restart
 NPU hardware. Before the firmware response DONE message for PURGE request,
 DONE/NDONE response for outstanding processing requsests are generated.
 Driver should properly handle those replies.

 POWER_DOWN : Intentd of this command is make the NPU ready for power down.
 Driver send this command to notify the NPU is about to power down. It implies
 PURGE request for all valid uid - Firmware should clean-up all the out-standing
 requests at once. After the clean-up is completed, Firmware should make sure that
 there is no outstanding bus request to outside of NPU. After the check is done,
 Firmware should issue WFI call to allow the NPU go power down safely and reply
 DONE message via mailbox.
*/

#ifndef MAILBOX_MSG_H_
#define MAILBOX_MSG_H_

/*
 *  *  +-----------------------------+ ------------
 *  *  |      Mailbox #4             |          |
 *  *  +-----------------------------+          |
 *  *  |      Mailbox #3             |          |
 *  *  |                             |          |
 *  *  +-----------------------------+          |
 *  *  |      Mailbox #2             |         16K
 *  *  |                             |      (= NPU_MAILBOX_SIZE)
 *  *  +-----------------------------+          |
 *  *  |      Mailbox #1             |          |
 *  *  |                             |          |
 *  *  +-----------------------------+  ----    |
 *  *  |       Padding               |   |      |
 *  *  +-----------------------------+   4K     |
 *  *  |    struct mailbox_hdr       |   |      |
 *  *  +-----------------------------+  ----------- <- npu_mailbox->mbox_base by Eung ju kim
 *  */

#define COMMAND_VERSION			6
#define MESSAGE_MAX_CNT			32
#define MESSAGE_MAGIC			0xC0DECAFE
#define MESSAGE_MARK			0xCAFEC0DE

/* payload size of load includes only ncp header */
struct cmd_load {
	u32				oid; /* object id */
	u32				tid; /* task id */
};

struct cmd_unload {
	u32				oid;
};

struct cmd_process {
	u32				oid;
	u32				fid; /* frame id */
};

struct cmd_profile_ctl {
	u32				ctl;
};

struct cmd_purge {
	u32				oid;
};

struct cmd_powerdown {
	u32				dummy;
};

struct cmd_done {
	u32				fid;
};

struct cmd_ndone {
	u32				fid;
	u32				error; /* error code */
};

struct cmd_group_done {
	u32				group_idx; /* group index points one element of group vector array */
};

struct cmd_fw_test {
	u32				param;	/* Number of testcase to be executed */
};

enum message_cmd {
	COMMAND_LOAD,
	COMMAND_UNLOAD,
	COMMAND_PROCESS,
	COMMAND_PROFILE_CTL,
	COMMAND_PURGE,
	COMMAND_POWERDOWN,
	COMMAND_FW_TEST,
	COMMAND_H2F_MAX_ID,
	COMMAND_DONE = 100,
	COMMAND_NDONE,
	COMMNAD_GROUP_DONE,
	COMMNAD_ROLLOVER,
	COMMAND_MAX_ID,
	COMMAND_F2H_MAX_ID
};

enum profile_ctl_code {
	PROFILE_CTL_CODE_START,		/* length: Size of profiling buffer, */
					/* payload: dvaddr of profiling buffer */
	PROFILE_CTL_CODE_STOP,		/* length: 0, payload: 0 */
};

struct message {
	u32				magic; /* magic number */
	u32				mid; /* message id */
	u32				command;
	u32				length; /* size in bytes */
	u32				self; /* self pointer */
	u32				data; /* the pointer of command */
};

struct command {
	union {
		struct cmd_load		load;
		struct cmd_unload	unload;
		struct cmd_process	process;
		struct cmd_profile_ctl	profile_ctl;
		struct cmd_fw_test	fw_test;
		struct cmd_purge	purge;
		struct cmd_powerdown	powerdown;
		struct cmd_done		done;
		struct cmd_ndone	ndone;
		struct cmd_group_done	gdone;
	} c; /* specific command properties */

	u32				length; /* the size of payload */
	u32				payload;
};

#endif /* MAILBOX_MSG_H_ */

