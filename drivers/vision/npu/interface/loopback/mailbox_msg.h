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

#ifndef MAILBOX_MSG_H_
#define MAILBOX_MSG_H_

#define COMMAND_VERSION		1
#define COMMNAD_MAGIC		0xC00DCAFE

enum message_cmd {
	COMMAND_LOAD,
	COMMAND_UNLOAD,
	COMMAND_PROCESS
};

struct message {
	u32			magic_number;
	u32			mid; /* message id */
	u32			command;
	u32			length; /* size in bytes */
	void			*data;
};

struct cmd_load {
	u32			oid; /* object id */
	u32			tid; /* task id */
	u32			length; /* size in bytes, this length includes only header + vector */
	void			*data;
};

struct cmd_unload {
	u32			oid;
	u32			length;
	void			*data;
};

struct cmd_process {
	u32			oid;
	u32			fid; /* frame id */
	u32			length; /* the count of address vector */
	void			*data; /* the pointer of address vector */
};

#endif /* MAILBOX_MSG_H_ */

