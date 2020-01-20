/*
 * Copyright (C) 2014 Samsung Electronics.
 *
 * Author: Chulhee Park <chul2.park@samsung.com>
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

#ifndef LINK_CONTROL_MSG_IOSM_H
#define LINK_CONTROL_MSG_IOSM_H

/* direction: CP -> AP */
#define IOSM_C2A_MDM_READY	0x80
#define IOSM_C2A_CONF_CH_RSP	0xA3	/* The answer of flow control msg */
#define IOSM_C2A_STOP_TX_CH	0xB0
#define IOSM_C2A_START_TX_CH	0xB1
#define IOSM_C2A_ACK		0xE0
#define IOSM_C2A_NACK		 0xE1

/* direction: AP -> CP */
#define IOSM_A2C_AP_READY	0x00
#define IOSM_A2C_CONF_CH_REQ	0x22	/* flow control on/off */
#define IOSM_A2C_OPEN_CH	0x24
#define IOSM_A2C_CLOSE_CH	0x25
#define IOSM_A2C_STOP_TX_CH	0x30
#define IOSM_A2C_START_TX_CH	0x30
#define IOSM_A2C_ACK		0x60
#define IOSM_A2C_NACK		0x61

#define IOSM_TRANS_ID_MAX	255
#define IOSM_MSG_AREA_SIZE	(CTRL_RGN_SIZE / 2)
#define IOSM_MSG_TX_OFFSET	CMD_RGN_OFFSET
#define IOSM_MSG_RX_OFFSET	(CMD_RGN_OFFSET + IOSM_MSG_AREA_SIZE)
#define IOSM_MSG_DESC_OFFSET	(CMD_RGN_OFFSET + CMD_RGN_SIZE)

void tx_iosm_message(struct mem_link_device *mld, u8 id, u32 *args);
void iosm_event_work(struct work_struct *work);
void iosm_event_bh(struct mem_link_device *mld, u16 cmd);
#endif
