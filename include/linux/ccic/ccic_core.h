/*
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * Author:Wookwang Lee. <wookwang.lee@samsung.com>,
 * Author:Guneet Singh Khurana  <gs.khurana@samsung.com>,
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __LINUX_CCIC_CORE_H__
#define __LINUX_CCIC_CORE_H__

/* CCIC Dock Observer Callback parameter */
enum {
	CCIC_DOCK_DETACHED	= 0,
	CCIC_DOCK_HMT		= 105,	/* Samsung Gear VR */
	CCIC_DOCK_ABNORMAL	= 106,
	CCIC_DOCK_MPA		= 109,	/* Samsung Multi Port Adaptor */
	CCIC_DOCK_DEX		= 110,	/* Samsung Dex */
	CCIC_DOCK_HDMI		= 111,	/* Samsung HDMI Dongle */
	CCIC_DOCK_T_VR		= 112,
	CCIC_DOCK_UVDM		= 113,
	CCIC_DOCK_DEXPAD	= 114,
	CCIC_DOCK_TYPEC_ANALOG_EARPHONE	= 115,	/* RA/RA TypeC Analog Earphone*/
	CCIC_DOCK_NEW		= 200,  /* For New uevent */
};

typedef enum {
	TYPE_C_DETACH = 0,
	TYPE_C_ATTACH_DFP = 1, /* Host */
	TYPE_C_ATTACH_UFP = 2, /* Device */
	TYPE_C_ATTACH_DRP = 3, /* Dual role */
	TYPE_C_ATTACH_SRC = 4, /* SRC */
	TYPE_C_ATTACH_SNK = 5, /* SNK */
} CCIC_OTP_MODE;

#if defined(CONFIG_TYPEC)
typedef enum {
	TRY_ROLE_SWAP_NONE = 0,
	TRY_ROLE_SWAP_PR = 1, /* pr_swap */
	TRY_ROLE_SWAP_DR = 2, /* dr_swap */
	TRY_ROLE_SWAP_TYPE = 3, /* type */
} CCIC_ROLE_SWAP_MODE;

#define TRY_ROLE_SWAP_WAIT_MS 5000
#endif
#define DUAL_ROLE_SET_MODE_WAIT_MS 1500
#define GEAR_VR_DETACH_WAIT_MS		1000

#if defined(CONFIG_CCIC_NOTIFIER)
struct ccic_state_work {
	struct work_struct ccic_work;
	int dest;
	int id;
	int attach;
	int event;
	int sub;
};
typedef enum {
	CLIENT_OFF = 0,
	CLIENT_ON = 1,
} CCIC_DEVICE_REASON;

typedef enum {
	HOST_OFF = 0,
	HOST_ON = 1,
} CCIC_HOST_REASON;
#endif

enum uvdm_data_type {
	TYPE_SHORT = 0,
	TYPE_LONG,
};

enum uvdm_direction_type {
	DIR_OUT = 0,
	DIR_IN,
};

struct uvdm_data {
	unsigned short pid; /* Product ID */
	char type; /* uvdm_data_type */
	char dir; /* uvdm_direction_type */
	unsigned int size; /* data size */
	void __user *pData; /* data pointer */
};

struct ccic_misc_dev {
	struct uvdm_data u_data;
	atomic_t open_excl;
	atomic_t ioctl_excl;
	int (*uvdm_write)(void *data, int size);
	int (*uvdm_read)(void *data);
	int (*uvdm_ready)(void);
	void (*uvdm_close)(void);
	bool (*pps_control)(int en);
};

typedef struct _ccic_data_t {
	const char *name;
	void *ccic_syfs_prop;
	void *drv_data;
	void (*set_enable_alternate_mode)(int);
	struct ccic_misc_dev *misc_dev;
} ccic_data_t, *pccic_data_t;

int ccic_core_init(void);
int ccic_core_register_chip(pccic_data_t pccic_data);
void ccic_core_unregister_chip(void);
int ccic_register_switch_device(int mode);
void ccic_send_dock_intent(int type);
void ccic_send_dock_uevent(u32 vid, u32 pid, int state);
void *ccic_core_get_drvdata(void);
int ccic_misc_init(pccic_data_t pccic_data);
void ccic_misc_exit(void);
extern unsigned int pn_flag;
#endif

