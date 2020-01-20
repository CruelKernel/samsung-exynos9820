/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-tx-session.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __HDCP_TX_SESSION_H__
#define __HDCP_TX_SESSION_H__

#include "exynos-hdcp2-protocol-msg.h"

#define WRAP_SKEY_EMPTY		0
#define WRAP_SKEY_STORED	1

#define HDCP_WITHOUT_STORED_KM	0
#define HDCP_WITH_STORED_KM	1

#define MAX_IP_LEN 15 /* IP address format is "xxx.xxx.xxx.xxx" */
#define RECEIVER_ID_BYTE_LEN            (40 / 8)
#define HDCP_PRIVATE_DATA_LEN           16
#define HDCP_WRAPPED_HMAC_LEN           124
#define REPEATER        1
#define NO_RECEIVER     0


/**
 * HDCP Link state & data structure
 */
typedef enum hdcp_tx_hdcp_link_state {
        LINK_ST_INIT = 0,
        LINK_ST_H0_NO_RX_ATTATCHED,
        LINK_ST_H1_TX_LOW_VALUE_CONTENT,
        LINK_ST_A0_DETERMINE_RX_HDCP_CAP,
        LINK_ST_A1_EXCHANGE_MASTER_KEY,
        LINK_ST_A2_LOCALITY_CHECK,
        LINK_ST_A3_EXCHANGE_SESSION_KEY,
        LINK_ST_A4_TEST_REPEATER,
        LINK_ST_A5_AUTHENTICATED,
        LINK_ST_A6_WAIT_RECEIVER_ID_LIST,
        LINK_ST_A7_VERIFY_RECEIVER_ID_LIST,
        LINK_ST_A8_SEND_RECEIVER_ID_LIST_ACK,
        LINK_ST_A9_CONTENT_STREAM_MGT,
        LINK_ST_END
} hdcp_tx_hdcp_link_state;

struct hdcp_session_node {
        struct hdcp_session_data *ss_data;
        struct hdcp_session_node *next;
        struct hdcp_session_node *prev;
};

struct hdcp_session_list {
        struct hdcp_session_node hdcp_session_head;
        struct mutex ss_mutex;
};

struct hdcp_timer {
        struct timeval start;
        struct timeval end;
        uint32_t timeout; /* millisecond */
        uint32_t elapsed_time; /* millisecond */
};

struct hdcp_link_data {
        uint32_t id;
        uint32_t state;
        uint8_t stored_km;
        uint32_t errno; /* error code */
	uint32_t lk_type; /* link type */
        struct hdcp_tx_ctx tx_ctx; /* Transmitter context data */
        struct hdcp_rx_ctx rx_ctx; /* Receiver context data */
        struct hdcp_timer timer; /* to check timeout */
        struct hdcp_session_node *ss_ptr; /* session pointer link belong */
};

struct hdcp_link_node {
        struct hdcp_link_data *lk_data;
        struct hdcp_link_node *next;
        struct hdcp_link_node *prev;
};

struct hdcp_link_list {
        struct hdcp_link_node hdcp_link_head;
        struct mutex lk_mutex;
};

/**
 * HDCP Session status & data structure
 */
typedef enum hdcp_tx_ss_state {
	SESS_ST_INIT = 0,
	SESS_ST_LINK_SETUP,
	SESS_ST_END
} hdcp_tx_ss_state;

struct hdcp_session_data {
	uint32_t id;
	hdcp_tx_ss_state state;
	uint8_t wrap_skey_store;
	uint8_t riv[HDCP_AKE_RTX_BYTE_LEN];
	uint8_t wrap_skey[HDCP_AKE_WKEY_BYTE_LEN];
	struct hdcp_link_list ln;
};

struct hdcp_session_data *hdcp_session_data_create(void);
void hdcp_session_data_destroy(struct hdcp_session_data **ss_data);
struct hdcp_link_data *hdcp_link_data_create(void);
void hdcp_link_data_destroy(struct hdcp_link_data **lk_data);

/* Session list APIs */
void hdcp_session_list_init(struct hdcp_session_list *ss_list);
void hdcp_session_list_add(struct hdcp_session_node *new_ent, struct hdcp_session_list *ss_list);
void hdcp_session_list_del(struct hdcp_session_node *del_ent, struct hdcp_session_list *ss_list);
void hdcp_session_list_print_all(struct hdcp_session_list *ss_list);
void hdcp_session_list_destroy(struct hdcp_session_list *ss_list);
struct hdcp_session_node *hdcp_session_list_find(uint32_t id, struct hdcp_session_list *ss_list);

/* Link list APIs */
void hdcp_link_list_init(struct hdcp_link_list *lk_list);
void hdcp_link_list_add(struct hdcp_link_node *new_ent, struct hdcp_link_list *lk_list);
void hdcp_link_list_del(struct hdcp_link_node *del_ent, struct hdcp_link_list *lk_list);
void hdcp_link_list_print_all(struct hdcp_link_list *lk_list);
void hdcp_link_list_destroy(struct hdcp_link_list *lk_list);
struct hdcp_link_node *hdcp_link_list_find(uint32_t id, struct hdcp_link_list *lk_list);
#endif

