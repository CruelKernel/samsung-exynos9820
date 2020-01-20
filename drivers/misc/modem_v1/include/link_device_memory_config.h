/**
@file		link_device_mem_config.h
@brief		configurations of memory-type interface
@date		2014/02/05
@author		Hankook Jang (hankook.jang@samsung.com)
*/

/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_LINK_DEVICE_MEM_CONFIG_H__
#define __MODEM_LINK_DEVICE_MEM_CONFIG_H__

#define GROUP_MEM_TYPE
/**
@defgroup group_mem_type		Memory Type
*/

#define GROUP_MEM_TYPE_SHMEM
/**
@defgroup group_mem_type_shmem		Shared Memory
@ingroup group_mem_type
*/

#define GROUP_MEM_LINK_DEVICE
/**
@defgroup group_mem_link_device		Memory Link Device
*/

#define GROUP_MEM_LINK_SBD
/**
@defgroup group_mem_link_sbd		Shared Buffer Descriptor
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_SNAPSHOT
/**
@defgroup group_mem_link_snapshot	Memory Snapshot
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_INTERRUPT
/**
@defgroup group_mem_link_interrupt	Interrupt
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_SETUP
/**
@defgroup group_mem_link_setup		Link Setup
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_METHOD
/**
@defgroup group_mem_link_method		Link Method
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_COMMAND
/**
@defgroup group_mem_link_command	Link Command
@ingroup group_mem_link_device
*/

#define GROUP_MEM_IPC_DEVICE
/**
@defgroup group_mem_ipc_device		Logical IPC Device
@ingroup group_mem_link_device
*/

#define GROUP_MEM_IPC_TX
/**
@defgroup group_mem_ipc_tx		TX
@ingroup group_mem_ipc_device
*/

#define GROUP_MEM_IPC_RX
/**
@defgroup group_mem_ipc_rx		RX
@ingroup group_mem_ipc_device
*/

#define GROUP_MEM_FLOW_CONTROL
/**
@defgroup group_mem_flow_control	Flow Control
@ingroup group_mem_ipc_device
*/

#define GROUP_MEM_CP_CRASH
/**
@defgroup group_mem_cp_crash		CP Crash Dump
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_DEBUG
/**
@defgroup group_mem_link_debug		Debugging
@ingroup group_mem_link_device
*/

#define GROUP_MEM_LINK_IOSM_MESSAGE
/**
@defgroup group_mem_link_iosm_message	Link IOSM Message
@ingroup group_mem_link_device
*/

#endif
