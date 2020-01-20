/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _UAPI_SDP_MM_H_
#define _UAPI_SDP_MM_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define MAX_SENSITIVE_PROC 100

/*
 * struct sdp_mm_sensitive_proc_req - for setting the process as sensitive
 * @proc_id - process id of the process to be set (as sensitive)
 */
struct sdp_mm_sensitive_proc_req {
	unsigned int proc_id; /* in */
};

/*
 * struct sdp_mm_sensitive_proc_list_resp - for querying sensitive process list
 * @sensitive_proc_list_len - number of sensitive processes in the list
 * @sensitive_proc_list - sensitive process list
 */
struct sdp_mm_sensitive_proc_list_resp {
	unsigned int sensitive_proc_list_len; 
	unsigned int sensitive_proc_list[MAX_SENSITIVE_PROC];
};


#define SDP_MM_IOC_MAGIC    0x77

#define SDP_MM_IOCTL_PROC_SENSITIVE_QUERY_REQ \
	_IOWR(SDP_MM_IOC_MAGIC, 1, struct sdp_mm_sensitive_proc_list_resp)

#define SDP_MM_IOCTL_SET_SENSITIVE_PROC_REQ \
	_IOWR(SDP_MM_IOC_MAGIC, 2, struct sdp_mm_sensitive_proc_req)

#endif /* _UAPI_SDP_MM_H_ */
