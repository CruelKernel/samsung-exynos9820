/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-tx-auth.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "exynos-hdcp2-iia-auth.h"
#include "../exynos-hdcp2-log.h"

#define HDCP_TX_VERSION_2_2	HDCP_TX_VERSION_2_1

static int next_step;
static int key_found;
static char *hdcp_msgid_str[] = {
	NULL,
	"Null message",
	"AKE_Init",
	"AKE_Send_Cert",
	"AKE_No_Stored_km",
	"AKE_Stored_km",
	"AKE_Send_rrx",
	"AKE_Send_H_prime",
	"AKE_Send_Pairing_Info",
	"LC_Init",
	"LC_Send_L_prime",
	"SKE_Send_Eks",
	"RepeaterAuth_Send_ReceiverID_List",
	"RTT_Ready",
	"RTT_Challenge",
	"RepeaterAuth_Send_Ack",
	"RepeaterAuth_Stream_Manage",
	"RepeaterAuth_Stream_Ready",
	"Receiver_AuthStatus",
	"AKE_Transmitter_Info",
	"AKE_Receiver_Info",
	NULL
};

#define is_precomput(lk) \
	((lk->rx_ctx.version != HDCP_VERSION_2_0) && \
	lk->tx_ctx.lc_precomp && \
	lk->rx_ctx.lc_precomp)


static int send_protocol_msg(struct hdcp_link_data *lk, uint8_t msg_id, struct hdcp_msg_info *msg_info)
{
        int ret = TX_AUTH_SUCCESS;

	hdcp_info("Tx->Rx: %s\n", hdcp_msgid_str[msg_id]);
	ret = cap_protocol_msg(msg_id, msg_info->msg, (size_t *)&msg_info->msg_len, lk->lk_type, &lk->tx_ctx, &lk->rx_ctx);

        if (ret) {
                hdcp_err("cap_protocol_msg() failed. ret(0x%08x)\n", ret);
                return -TX_AUTH_ERROR_MAKE_PROTO_MSG;
        }

        return TX_AUTH_SUCCESS;
}

static int recv_protocol_msg(struct hdcp_link_data *lk, uint8_t msg_id, struct hdcp_msg_info *msg_info)
{
	int ret = TX_AUTH_SUCCESS;
	if (ret >= 0 && msg_info->msg_len > 0) {
		/* parsing received message */
		ret = decap_protocol_msg(msg_id, msg_info->msg, msg_info->msg_len, lk->lk_type, &lk->tx_ctx, &lk->rx_ctx);

		if (ret) {
			hdcp_err("dcap_protocol_msg() failed. msg_id(%d), ret(0x%08x)\n", msg_id, ret);
			ret = -TX_AUTH_ERROR_WRONG_MSG;
		}
	}

	hdcp_info("Rx->Tx: %s\n", hdcp_msgid_str[msg_id]);
	return ret;
}

static int hdcp_ake_find_masterkey(struct hdcp_tx_ctx *tx_ctx,
		struct hdcp_rx_ctx *rx_ctx)
{
	/* todo */
	return -1;
}

static int get_hdcp_session_key(struct hdcp_link_data *lk)
{
	/* todo */
	return 0;
}

static int put_hdcp_session_key(struct hdcp_link_data *lk)
{
	/* todo */
	return 0;
}

int exchange_master_key(struct hdcp_link_data *lk, struct hdcp_msg_info *msg_info)
{
	int rval = TX_AUTH_SUCCESS;

	if (next_step < AKE_INIT)
		UPDATE_STEP_STATE(next_step, AKE_INIT);

	switch (next_step) {
	case AKE_INIT:
	{
		rval = send_protocol_msg(lk, AKE_INIT, msg_info);
		if (rval < 0) {
			hdcp_err("send AKE_Init failed. rval(%d)\n", rval);
		} else {
			UPDATE_STEP_STATE(next_step, AKE_TRANSMITTER_INFO);
#ifdef HDCP_TX_VERSION_2_2
			msg_info->next_step = SEND_MSG;
#else
			msg_info->next_step = RECIEVE_MSG;
#endif
		}
		break;
	}
	case AKE_TRANSMITTER_INFO:
	{
#ifdef HDCP_TX_VERSION_2_2
		rval = send_protocol_msg(lk, AKE_TRANSMITTER_INFO, msg_info);
		if (rval < 0) {
			hdcp_err("send AKE_Transmitter_Info failed. rval(%d)\n", rval);
		} else {
			next_step++;
			UPDATE_STEP_STATE(next_step, AKE_SEND_CERT);
			msg_info->next_step = RECIEVE_MSG;
		}
		break;
#endif
	}
	case AKE_SEND_CERT:
	{
		rval = recv_protocol_msg(lk, AKE_SEND_CERT, msg_info);
		if (rval < 0) {
			hdcp_err("recv AKE_Send_Cert failed. rval(%d)\n", rval);
		} else {
			UPDATE_STEP_STATE(next_step, AKE_RECEIVER_INFO);
			msg_info->next_step = RECIEVE_MSG;
		}
		break;
	}
	case AKE_RECEIVER_INFO:
	{
		rval = recv_protocol_msg(lk, AKE_RECEIVER_INFO, msg_info);
		if (rval < 0) {
			hdcp_err("recv AKE_Receiver_Info failed. rval(%d)\n", rval);
		} else {
			UPDATE_STEP_STATE(next_step, AKE_NO_STORED_KM);
			msg_info->next_step = SEND_MSG;
		}
		break;
	}
	case AKE_NO_STORED_KM:
	{
		key_found = hdcp_ake_find_masterkey(&lk->tx_ctx, &lk->rx_ctx);
		if ((key_found < 0)) {
			rval = send_protocol_msg(lk, AKE_NO_STORED_KM, msg_info);
			if (rval < 0) {
				hdcp_err("send AKE_No_Stored_km failed. rval(%d)\n", rval);
			} else {
				UPDATE_STEP_STATE(next_step, AKE_SEND_RRX);
				msg_info->next_step = RECIEVE_MSG;
			}
			lk->stored_km = HDCP_WITHOUT_STORED_KM;
		} else {
			rval = send_protocol_msg(lk, AKE_STORED_KM, msg_info);
			if (rval < 0) {
				hdcp_err("send AKE_Stored_km failed. rval(%d)\n", rval);
			} else {
				UPDATE_STEP_STATE(next_step, AKE_SEND_RRX);
				msg_info->next_step = RECIEVE_MSG;
			}
			lk->stored_km = HDCP_WITH_STORED_KM;
		}
		break;
	}
	case AKE_SEND_RRX:
	{
		rval = recv_protocol_msg(lk, AKE_SEND_RRX, msg_info);
		if (rval < 0) {
			hdcp_err("recv AKE_Send_rrx failed. rval(%d)\n", rval);
		} else {
			UPDATE_STEP_STATE(next_step, AKE_SEND_H_PRIME);
			msg_info->next_step = RECIEVE_MSG;
		}
		break;
	}
	case AKE_SEND_H_PRIME:
	{
		rval = recv_protocol_msg(lk, AKE_SEND_H_PRIME, msg_info);
		if (rval < 0) {
			hdcp_err("recv AKE_Send_H_Prime failed. rval(%d)\n", rval);
		} else {
			UPDATE_STEP_STATE(next_step, AKE_SEND_PAIRING_INFO);
			if(key_found == -1){
				hdcp_debug("Key found\n");
				msg_info->next_step = RECIEVE_MSG;
			}
			else {
				hdcp_debug("Key not found\n");
				msg_info->next_step = DONE;
			}
		}
		break;
	}
	case AKE_SEND_PAIRING_INFO:
	{
		if (key_found == -1) {
			rval = recv_protocol_msg(lk, AKE_SEND_PAIRING_INFO, msg_info);
			if (rval < 0) {
				hdcp_err("recv AKE_Send_Pairing_Info failed. rval(%d)\n", rval);
				break;
			}
		}
		msg_info->next_step = DONE;
		UPDATE_STEP_STATE(next_step, LC_INIT);
		break;
	}
	default:
		return -ENOTTY;
	}

	if (rval != TX_AUTH_SUCCESS)
		next_step = AKE_INIT;

	return rval;
}

int locality_check(struct hdcp_link_data *lk, struct hdcp_msg_info *msg_info)
{
	int rval = TX_AUTH_SUCCESS;

	switch (next_step) {
	case LC_INIT:
	{
		rval = send_protocol_msg(lk, LC_INIT, msg_info);
		if (rval < 0) {
			hdcp_err("send LC_Init failed. rval(%d)\n", rval);
			break;
		}
		else {
			UPDATE_STEP_STATE(next_step, RTT_READY);
			msg_info->next_step = RECIEVE_MSG;
			break;
		}
	}
	case RTT_READY:
	{
		/* if no precompute */
		if (!is_precomput(lk)) {
			hdcp_debug("LC with no precompute\n");
			rval = recv_protocol_msg(lk, LC_SEND_L_PRIME, msg_info);
			if (rval == TX_AUTH_SUCCESS) {
				UPDATE_STEP_STATE(next_step, AKE_INIT);
				msg_info->next_step = DONE;
			}
			else {
				hdcp_err("recv LC_Send_L_prime failed. rval(%d)\n", rval);
			}
			break;
		} else {
			/* if precompute */
			rval = recv_protocol_msg(lk, RTT_READY, msg_info);
			if (rval < 0) {
				hdcp_err("recv RTT_Ready failed. rval(%d)\n", rval);
			}
			else {
				UPDATE_STEP_STATE(next_step, RTT_CHALLENGE);
				msg_info->next_step = SEND_MSG;
			}
			break;
		}
	}
	case RTT_CHALLENGE:
	{
		rval = send_protocol_msg(lk, RTT_CHALLENGE, msg_info);
		if (rval < 0) {
			hdcp_err("send RTT_Challenge failed. rval(%d)\n", rval);
		}
		else {
			UPDATE_STEP_STATE(next_step, LC_SEND_L_PRIME);
			msg_info->next_step = RECIEVE_MSG;
		}
		break;
	}
	case LC_SEND_L_PRIME:
	{
		rval = recv_protocol_msg(lk, LC_SEND_L_PRIME, msg_info);
		if (rval == TX_AUTH_SUCCESS){
			msg_info->next_step = DONE;
			if (evaluate_repeater(lk))
				UPDATE_STEP_STATE(next_step, REPEATERAUTH_SEND_RECEIVERID_LIST);
			else
				UPDATE_STEP_STATE(next_step, AKE_INIT);
		}
		else {
			hdcp_err("recv LC_Send_L_prime failed. rval(%d)\n", rval);
		}
		break;
	}
	default:
		return -ENOTTY;
	}

	return rval;
}

int exchange_hdcp_session_key(struct hdcp_link_data *lk, struct hdcp_msg_info *msg_info)
{
	int rval = TX_AUTH_SUCCESS;

	/* find session key from the session data */
	if (get_hdcp_session_key(lk) < 0) {
		hdcp_err("get_hdcp_session_key() failed\n");
		rval = -TX_AUTH_ERRRO_RESTORE_SKEY;
	}

	rval = send_protocol_msg(lk, SKE_SEND_EKS, msg_info);
	if (rval < 0)
		hdcp_err("send SKE_Send_Eks() failed\n");

	if (put_hdcp_session_key(lk) < 0) {
		hdcp_err("put_hdcp_session_key() failed\n");
		rval = -TX_AUTH_ERROR_STORE_SKEY;
	}

	return rval;
}

int evaluate_repeater(struct hdcp_link_data *lk)
{
	if (lk->rx_ctx.repeater == REPEATER)
		return TRUE;
	else
		return FALSE;
}

int wait_for_receiver_id_list(struct hdcp_link_data *lk, struct hdcp_msg_info *msg_info)
{
        int rval = TX_AUTH_SUCCESS;

        rval = recv_protocol_msg(lk, REPEATERAUTH_SEND_RECEIVERID_LIST, msg_info);
        if (rval < 0) {
                hdcp_err("recv RepeaterAuth_Send_ReceiverID_List failed. rval(%d)\n", rval);
                return rval;
        }

	UPDATE_STEP_STATE(next_step, REPEATERAUTH_SEND_ACK);
        return rval;
}

int send_receiver_id_list_ack(struct hdcp_link_data *lk, struct hdcp_msg_info *msg_info)
{
        int rval = TX_AUTH_SUCCESS;

	switch (next_step) {
	case REPEATERAUTH_SEND_ACK:
	{
	        rval = send_protocol_msg(lk, REPEATERAUTH_SEND_ACK, msg_info);
		if (rval < 0) {
			hdcp_err("send RepeaterAuth_Send_Ack() failed. rval(%d)\n", rval);
		}
		else {
			UPDATE_STEP_STATE(next_step, RECEIVER_AUTHSTATUS);
			msg_info->next_step = RP_RECIEVE_MSG;
		}
		break;
	}
	case RECEIVER_AUTHSTATUS:
	{
		rval = recv_protocol_msg(lk, RECEIVER_AUTHSTATUS, msg_info);
		if (rval < 0) {
			hdcp_err("send Receiver_AuthStatus() failed. rval(%d)\n", rval);
		}
		else {
			UPDATE_STEP_STATE(next_step, REPEATERAUTH_STREAM_MANAGE);
			msg_info->next_step = DONE;
		}
		break;
	}
	default:
		return -ENOTTY;
	}

        if (lk->tx_ctx.rp_reauth) {
                hdcp_err("receiver request reauthentication\n");
                return -TX_AUTH_ERROR_TIME_EXCEED;
        }
        else
                hdcp_err("Repeater send reAuth value:: %d\n", lk->tx_ctx.rp_reauth);

        return rval;
}

int manage_content_stream(struct hdcp_link_data *lk, struct contents_info *stream_ctrl, struct hdcp_stream_info *stream_info)
{

        int rval = TX_AUTH_SUCCESS;
	struct hdcp_msg_info msg_info;

	if (stream_info->msg_len < sizeof(msg_info.msg) && stream_info->msg_len > 0)
		msg_info.msg_len = stream_info->msg_len;
	else
		return HDCP_ERROR_INVALID_STREAM_LEN;

	memcpy(msg_info.msg, stream_info->msg, stream_info->msg_len);
	switch (next_step) {
	case REPEATERAUTH_STREAM_MANAGE:
	{
		memcpy(&lk->tx_ctx.stream_ctrl, stream_ctrl, sizeof(struct contents_info));
		rval = send_protocol_msg(lk, REPEATERAUTH_STREAM_MANAGE, &msg_info);
		if (rval < 0) {
			hdcp_err("send RepeaterAuth_stream_manage() failed. rval(%d)\n", rval);
			break;
		}
		else {
			UPDATE_STEP_STATE(next_step, REPEATERAUTH_STREAM_READY);
			stream_info->next_step = ST_RECIEVE_MSG;
			if (msg_info.msg_len > STREAM_MAX_LEN) {
				hdcp_err("send RepeaterAuth_stream_manage() failed. rval(%d)\n", rval);
				return TX_AUTH_ERROR_WRONG_MSG;
			}

			memcpy(stream_info->msg, msg_info.msg, msg_info.msg_len);
			stream_info->msg_len = msg_info.msg_len;
			break;
		}

	}
	case REPEATERAUTH_STREAM_READY:
	{
	        rval = recv_protocol_msg(lk, REPEATERAUTH_STREAM_READY, &msg_info);
		if (rval < 0) {
			hdcp_err("send Receiver_AuthStatus() failed. rval(%d)\n", rval);
			break;
		}
		else {
			UPDATE_STEP_STATE(next_step, AKE_INIT);
			stream_info->next_step = RP_FINISHED;
			break;
		}

		if (msg_info.msg_len < sizeof(stream_info->msg) && msg_info.msg_len > 0)
			memcpy(stream_info->msg, msg_info.msg, msg_info.msg_len);
	}
	default:
		return -ENOTTY;
	}

        return TX_AUTH_SUCCESS;
}

int determine_rx_hdcp_cap(struct hdcp_link_data *lk)
{
	/* todo */
	return 0;
}
