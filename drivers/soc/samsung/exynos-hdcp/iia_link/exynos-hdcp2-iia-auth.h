/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-tx-auth.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __HDCP_TX_AUTH_H__
#define __HDCP_TX_AUTH_H__

#include "../exynos-hdcp2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ERR_WRONG_MESSAGE_LENGTH        0x1003
#define ERR_WRONG_MESSAGE_ID            0x1004
#define ERR_GENERATE_NON_SECKEY         0x1005
#define ERR_FILE_OPEN                   0x1006

int determine_rx_hdcp_cap(struct hdcp_link_data *);
int exchange_master_key(struct hdcp_link_data *, struct hdcp_msg_info *);
int locality_check(struct hdcp_link_data *, struct hdcp_msg_info *);
int exchange_hdcp_session_key(struct hdcp_link_data *, struct hdcp_msg_info *);
int evaluate_repeater(struct hdcp_link_data *);
int wait_for_receiver_id_list(struct hdcp_link_data *, struct hdcp_msg_info *);
int send_receiver_id_list_ack(struct hdcp_link_data *, struct hdcp_msg_info *);
int manage_content_stream(struct hdcp_link_data *lk, struct contents_info *stream_ctrl, struct hdcp_stream_info *stream_info);
int unwrapped_key(char *key, int wrapped);

/* todo */
int determine_rx_hdcp_cap(struct hdcp_link_data *lk);

#ifdef __cplusplus
}
#endif

#endif
