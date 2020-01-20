/*
 *Gadget Driver's IOCTL for Android Connectivity Gadget
 *
 *Copyright (C) 2013 DEVGURU CO.,LTD
 *
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation,and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *ChangeLog:
 *	20140311 - add ioctl to communicate to userland application
*/
#ifndef __CONN_GADGET_IOCTL_DEFINE__
#define __CONN_GADGET_IOCTL_DEFINE__

enum {
	CONN_GADGET_IOCTL_BIND_STATUS_UNDEFINED = 0,
	CONN_GADGET_IOCTL_BIND_STATUS_BIND = 1,
	CONN_GADGET_IOCTL_BIND_STATUS_UNBIND = 2
};

#if defined(__ANDROID__) || defined(__TIZEN__)
enum {
	CONN_GADGET_IOCTL_NR_0 = 0,
	CONN_GADGET_IOCTL_NR_1,
	CONN_GADGET_IOCTL_NR_2,
	CONN_GADGET_IOCTL_NR_MAX
};

#define IOCTL_SUPPORT_LIST_ARRAY_MAX            255

/* ioctl */
#define CONN_GADGET_IOCTL_MAGIC_SIG             's'
#define CONN_GADGET_IOCTL_SUPPORT_LIST          _IOR(CONN_GADGET_IOCTL_MAGIC_SIG, CONN_GADGET_IOCTL_NR_0, int*)
#define CONN_GADGET_IOCTL_BIND_WAIT_NOTIFY      _IOR(CONN_GADGET_IOCTL_MAGIC_SIG, CONN_GADGET_IOCTL_NR_1, int)
#define CONN_GADGET_IOCTL_BIND_GET_STATUS       _IOR(CONN_GADGET_IOCTL_MAGIC_SIG, CONN_GADGET_IOCTL_NR_2, int)
#define CONN_GADGET_IOCTL_MAX_NR                        CONN_GADGET_IOCTL_NR_MAX
#endif
#endif
