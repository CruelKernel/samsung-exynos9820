/* driver/soc/samsung/exynos-hdcp/exynos-hdcp2.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_H__
#define __EXYNOS_HDCP2_H__

#include <linux/device.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>

#include "exynos-hdcp2-session.h"

#define HDCP_CMD_SESSION_OPEN		0x0001
#define HDCP_CMD_SESSION_CLOSE		0x0002
#define HDCP_CMD_LINK_OPEN		0x0003
#define HDCP_CMD_LINK_CLOSE		0x0004
#define HDCP_CMD_LINK_AUTH		0x0005
#define HDCP_CMD_LINK_ENC		0x0006

/* todo: define max message length based on HDCP spec */
#define HDCP_PROTO_MSG_LEN_MAX 1024

/* HDCP Link types */
#define HDCP_LINK_TYPE_IIA     0
#define HDCP_LINK_TYPE_DP      1

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define TEMP_ERROR 			-ENOTTY

static char *hdcp_session_st_str[] = {
	"ST_INIT",
	"ST_LINK_SETUP",
	"ST_END",
	NULL
};

static char *hdcp_link_st_str[] = {
	"ST_INIT",
	"ST_H0_NO_RX_ATTACHED",
	"ST_H1_TX_LOW_VALUE_CONTENT",
	"ST_A0_DETERMINE_RX_HDCP_CAP",
	"ST_A1_EXCHANGE_MASTER_KEY",
	"ST_A2_LOCALITY_CHECK",
	"ST_A3_EXCHANGE_SESSION_KEY",
	"ST_A4_TEST_REPEATER",
	"ST_A5_AUTHENTICATED",
	"ST_A6_WAIT_RECEIVER_ID_LIST",
	"ST_A7_VERIFY_RECEIVER_ID_LIST",
	"ST_A8_SEND_RECEIVER_ID_LIST_ACK",
	"ST_A9_CONTENT_STREAM_MGT",
	"ST_END",
	NULL
};

#define UPDATE_SESSION_STATE(sess, st)          do { \
        printk("[HDCP2]HDCP Session(%d): %s -> %s\n", sess->id, hdcp_session_st_str[sess->state], hdcp_session_st_str[st]); \
        sess->state = st; \
        } while (0)

#define UPDATE_LINK_STATE(link, st)             do { \
        printk("[HDCP2]HDCP Link(%d): %s -> %s\n", link->id, hdcp_link_st_str[link->state], hdcp_link_st_str[st]); \
        link->state = st; \
        } while (0)

#define UPDATE_STEP_STATE(step, st)          do { \
        printk("[HDCP2]HDCP STEP(%d): %s -> %s\n", st, hdcp_msgid_str[step], hdcp_msgid_str[st]); \
        step = st; \
        } while (0)

enum {
	SEND_MSG 		= 0,
	RECIEVE_MSG 		= 1,
	RP_SEND_MSG 		= 2,
	RP_RECIEVE_MSG 		= 3,
	ST_SEND_MSG 		= 4,
	ST_RECIEVE_MSG 		= 5,
	DONE 			= 6,
	AUTH_FINISHED 		= 7,
	RP_FINISHED 		= 8,
	WAIT_STATE 		= 100
};

enum {
	TX_AUTH_SUCCESS = 0,
	TX_AUTH_ERROR_CONNECT_TEE,
	TX_AUTH_ERROR_MAKE_PROTO_MSG,
	TX_AUTH_ERROR_SEND_PROTO_MSG,
	TX_AUTH_ERROR_RECV_PROTO_MSG,
	TX_AUTH_ERROR_TIME_EXCEED,
	TX_AUTH_ERROR_NET_TIMEOUT,
	TX_AUTH_ERROR_NET_CONN_CLOSED,
	TX_AUTH_ERROR_CRYPTO,
	TX_AUTH_ERROR_WRONG_MSG_ID,
	TX_AUTH_ERROR_WRONG_MSG,
	TX_AUTH_ERROR_WRONG_MSG_SIZE,
	TX_AUTH_ERROR_STORE_SKEY,
	TX_AUTH_ERRRO_RESTORE_SKEY,
	TX_AUTH_ERROR_BUFFER_OVERFLLOW,
	TX_AUTH_ERROR_RETRYCOUNT_EXCEED,
	TX_AUTH_ERROR_ABORT,
};

typedef enum hdcp_result {
	HDCP_SUCCESS = 0,
	HDCP_ERROR_INIT_FAILED,
	HDCP_ERROR_TERMINATE_FAILED,
	HDCP_ERROR_SESSION_OPEN_FAILED,
	HDCP_ERROR_SESSION_CLOSE_FAILED,
	HDCP_ERROR_LINK_OPEN_FAILED,
	HDCP_ERROR_LINK_CLOSE_FAILED,
	HDCP_ERROR_LINK_STREAM_FAILED,
	HDCP_ERROR_AUTHENTICATE_FAILED,
	HDCP_ERROR_COPY_TO_USER_FAILED,
	HDCP_ERROR_COPY_FROM_USER_FAILED,
	HDCP_ERROR_USER_FAILED,
	HDCP_ERROR_RX_NOT_HDCP_CAPABLE,
	HDCP_ERROR_EXCHANGE_KM,
	HDCP_ERROR_LOCALITY_CHECK,
	HDCP_ERROR_EXCHANGE_KS,
	HDCP_ERROR_WAIT_RECEIVER_ID_LIST,
	HDCP_ERROR_VERIFY_RECEIVER_ID_LIST,
	HDCP_ERROR_WAIT_AKE_INIT,
	HDCP_ERROR_MALLOC_FAILED = 0x1000,
	HDCP_ERROR_INVALID_HANDLE,
	HDCP_ERROR_INVALID_INPUT,
	HDCP_ERROR_INVALID_STATE,
	HDCP_ERROR_NET_UNREACHABLE,
	HDCP_ERROR_NET_CLOSED,
	HDCP_ERROR_ENCRYPTION,
	HDCP_ERROR_DECRYPTION,
	HDCP_ERROR_STREAM_MANAGE,
	HDCP_ERROR_WRAP_FAIL,
	HDCP_ERROR_UNWRAP_FAIL,
	HDCP_ERROR_WRONG_SIZE,
	HDCP_ERROR_DO_NOT_SUPPORT_YET,
	HDCP_ERROR_IOCTL_OPEN,
	HDCP_ERROR_END
} hdcp_result;

enum {
	HDCP_ERROR_INVALID_SESSION_HANDLE = 0x2000,
	HDCP_ERROR_INVALID_SESSION_STATE,
	HDCP_ERROR_INVALID_STREAM_LEN,
};

struct hdcp_info {
	struct device *dev;
	struct hdcp_session_list ss_list;
};

struct hdcp_sess_info {
	uint32_t ss_id;
	uint8_t wkey[HDCP_WRAP_MAX_SIZE];
};

struct hdcp_link_info {
	uint32_t ss_id;
	uint32_t lk_id;
};

struct hdcp_link_auth {
	uint32_t ss_id;
	uint32_t lk_id;
};

struct hdcp_link_enc {
	uint32_t ss_id;
	uint32_t lk_id;
};

struct hdcp_msg_info {
	uint32_t ss_handle;
	uint32_t lk_id;
	uint8_t msg[1024];
	uint32_t msg_len;
	uint32_t next_step;
};

struct hdcp_stream_info {
	uint32_t ss_id;
	uint32_t lk_id;
	uint32_t num;
	uint8_t type;
	uint16_t stream_pid;
	uint32_t stream_ctr;
	uint8_t msg[1024];
	uint32_t msg_len;
	uint32_t next_step;
};

struct hdcp_enc_info {
	uint32_t id;
	uint8_t pes_private[16];
	uint32_t priv_len;
	uint8_t str_ctr[4];
	uint8_t input_ctr[8];
	uint64_t input;
	unsigned long input_phys;
	uint32_t input_len;
	uint64_t output;
	unsigned long output_phys;
	uint32_t output_len;
};

struct hdcp_wrapped_key {
	uint8_t key[16];
	uint8_t enc_key[128];
	uint32_t wrapped;
	uint32_t key_len;
};

#define HDCP_IOC_SESSION_OPEN		_IOWR('S', 1, struct hdcp_sess_info)
#define HDCP_IOC_SESSION_CLOSE		_IOWR('S', 2, struct hdcp_sess_info)
#define HDCP_IOC_LINK_OPEN		_IOWR('S', 3, struct hdcp_link_info)
#define HDCP_IOC_LINK_CLOSE		_IOWR('S', 4, struct hdcp_link_info)
#define HDCP_IOC_LINK_AUTH		_IOWR('S', 5, struct hdcp_msg_info)
#define HDCP_IOC_LINK_ENC		_IOWR('S', 6, struct hdcp_enc_info)
#define HDCP_IOC_RESERVED_0 		_IO('S', 7)
#define HDCP_IOC_STREAM_MANAGE		_IOWR('S', 8, struct hdcp_stream_info)
#define HDCP_IOC_IIA_TX_SELFTEST	_IOWR('S', 90, int)
#define HDCP_IOC_DPLINK_TX_AUTH		_IOWR('S', 100, int)
#define HDCP_IOC_DPLINK_TX_EMUL 	_IOWR('S', 110, int)
#define HDCP_IOC_DPLINK_TX_SELFTEST	_IOWR('S', 120, int)
#define HDCP_IOC_WRAP_KEY		_IOWR('S', 200, struct hdcp_wrapped_key)
#define HDCP_FUNC_TEST_MODE		_IOWR('S', 220, int)

typedef struct hdcp_tx_link_handle {
        uint32_t ss_handle;
        uint32_t lk_id;
} hdcp_tx_link_handle_t;

enum hdcp_result hdcp_session_open(struct hdcp_sess_info *ss_info);
enum hdcp_result hdcp_session_close(struct hdcp_sess_info *ss_info);
enum hdcp_result hdcp_link_open(struct hdcp_link_info *link_info, uint32_t lk_type);
enum hdcp_result hdcp_link_close(struct hdcp_link_info *link_info);
enum hdcp_result hdcp_link_authenticate(struct hdcp_msg_info *msg_info);
enum hdcp_result hdcp_link_stream_manage(struct hdcp_stream_info *stream_info);
enum hdcp_result hdcp_wrap_key(struct hdcp_wrapped_key *key_info);
enum hdcp_result hdcp_unwrap_key(char *wkey);
#endif
