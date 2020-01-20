/*
 * drivers/soc/samsung/exynos_hdcp/dp_link/exynos-hdcp2-dplink-if.c
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/delay.h>
#include "../exynos-hdcp2-log.h"
#include "exynos-hdcp2-dplink-reg.h"
#include "exynos-hdcp2-dplink-if.h"

#if defined(CONFIG_HDCP2_EMULATION_MODE)
#define NETLINK_HDCP 31
#define SOCK_BUF_SIZE (1024 * 512)
#define NETLINK_PORT 1000

struct sock *nl_sk = NULL;
struct sk_buff sk_buf;
uint8_t dplink_wait;
uint8_t *nl_data;
struct nlmsghdr *nlh;

void cb_hdcp_nl_recv_msg(struct sk_buff *skb);

int hdcp_dplink_init(void)
{
	struct netlink_kernel_cfg cfg = {
		.input	= cb_hdcp_nl_recv_msg,
	};

	nl_sk = netlink_kernel_create(&init_net, NETLINK_HDCP, &cfg);
	if (!nl_sk) {
		hdcp_err("Error creating socket.\n");
		return -EINVAL;
	}

	nl_data = (uint8_t *)kzalloc(SOCK_BUF_SIZE, GFP_KERNEL);
	if (!nl_data) {
		hdcp_err("Netlink Socket buffer alloc fail\n");
		return -EINVAL;
	}

	dplink_wait = 1;

	return 0;
}

/* callback for netlink driver */
void cb_hdcp_nl_recv_msg(struct sk_buff *skb)
{
	nlh = (struct nlmsghdr *)skb->data;

	memcpy(nl_data, (uint8_t *)nlmsg_data(nlh), nlmsg_len(nlh));
	dplink_wait = 0;
}

static void nl_recv_msg(uint8_t *data, uint32_t size)
{
	/* todo: change to not a busy wait */
	while (dplink_wait) {
		hdcp_err("wait dplink_wait\n");
		msleep(1000);
	}

	memcpy(data, nl_data, size);

	dplink_wait = 1;
}

int hdcp_dplink_recv(uint32_t msg_name, uint8_t *data, uint32_t size)
{
	nl_recv_msg(data, size);
	return 0;
}

int hdcp_dplink_send(uint32_t msg_name, uint8_t *data, uint32_t size)
{
	struct sk_buff *skb_out;
	int ret;

	skb_out = nlmsg_new(size, 0);
	if (!skb_out) {
		hdcp_err("Failed to allocate new skb\n");
		return -1;
	}
	nlh = nlmsg_put(skb_out, 0, 0, 2, size, NLM_F_REQUEST);
	if (!nlh) {
		hdcp_err("fail to nlmsg_put()\n");
		return -1;
	}

	NETLINK_CB(skb_out).dst_group = 0; /* not in mcast group */
	memcpy(nlmsg_data(nlh), data, size);

	ret = nlmsg_unicast(nl_sk, skb_out, NETLINK_PORT);
	if (ret < 0) {
		hdcp_err("Error while sending bak to user\n");
		return -1;
	}
	
	return 0;
}

int hdcp_dplink_is_enabled_hdcp22(void)
{
	/* todo: check hdcp22 enable */
	return 1;
}

void hdcp_dplink_config(int en)
{
	/* dummy function */
}
#else


/* Address define for HDCP within DPCD address space */
uint32_t dpcd_addr[NUM_HDCP22_MSG_NAME] = {
	DPCD_ADDR_HDCP22_Rtx,
	DPCD_ADDR_HDCP22_TxCaps,
	DPCD_ADDR_HDCP22_cert_rx,
	DPCD_ADDR_HDCP22_Rrx,
	DPCD_ADDR_HDCP22_RxCaps,
	DPCD_ADDR_HDCP22_Ekpub_km,
	DPCD_ADDR_HDCP22_Ekh_km_w,
	DPCD_ADDR_HDCP22_m,
	DPCD_ADDR_HDCP22_Hprime,
	DPCD_ADDR_HDCP22_Ekh_km_r,
	DPCD_ADDR_HDCP22_rn,
	DPCD_ADDR_HDCP22_Lprime,
	DPCD_ADDR_HDCP22_Edkey0_ks,
	DPCD_ADDR_HDCP22_Edkey1_ks,
	DPCD_ADDR_HDCP22_riv,
	DPCD_ADDR_HDCP22_RxInfo,
	DPCD_ADDR_HDCP22_seq_num_V,
	DPCD_ADDR_HDCP22_Vprime,
	DPCD_ADDR_HDCP22_Rec_ID_list,
	DPCD_ADDR_HDCP22_V,
	DPCD_ADDR_HDCP22_seq_num_M,
	DPCD_ADDR_HDCP22_k,
	DPCD_ADDR_HDCP22_stream_IDtype,
	DPCD_ADDR_HDCP22_Mprime,
	DPCD_ADDR_HDCP22_RxStatus,
	DPCD_ADDR_HDCP22_Type,
};

#if defined(CONFIG_EXYNOS_DISPLAYPORT)
extern int displayport_dpcd_read_for_hdcp22(u32 address, u32 length, u8 *data);
extern int displayport_dpcd_write_for_hdcp22(u32 address, u32 length, u8 *data);
extern void displayport_hdcp22_enable(u32 en);

#else
int displayport_dpcd_read_for_hdcp22(u32 address, u32 length, u8 *data)
{
	return 0;
}
int displayport_dpcd_write_for_hdcp22(u32 address, u32 length, u8 *data)
{
	return 0;
}
void displayport_hdcp22_enable(u32 en)
{
	return;
}
#endif

int hdcp_dplink_init(void)
{
	/* todo */
	return 0;
}

void hdcp_dplink_config(int en)
{
	displayport_hdcp22_enable(en);
}

int hdcp_dplink_is_enabled_hdcp22(void)
{
	/* todo: check hdcp22 enable */
	return 1;
}

/* todo: get stream info from DP */
#define HDCP_DP_STREAM_NUM	0x01
static uint8_t stream_id[1] = {0x00};
int hdcp_dplink_get_stream_info(uint16_t *num, uint8_t *strm_id)
{
	*num = HDCP_DP_STREAM_NUM;
	memcpy(strm_id, stream_id, sizeof(uint8_t) * (*num));

	return 0;
}

int hdcp_dplink_recv(uint32_t msg_name, uint8_t *data, uint32_t size)
{
	int i;
	int ret;
	int remain;

	if (size > DPCD_PACKET_SIZE) {
		for (i = 0; i < (size / DPCD_PACKET_SIZE); i++) {
			ret = displayport_dpcd_read_for_hdcp22(
				dpcd_addr[msg_name] + i * DPCD_PACKET_SIZE,
				DPCD_PACKET_SIZE,
				&data[i * DPCD_PACKET_SIZE]);
			if (ret) {
				hdcp_err("dpcd read fail. ret(%d)\n", ret);
				return ret;
			}
		}

		remain = size % DPCD_PACKET_SIZE;
		if (remain) {
			ret = displayport_dpcd_read_for_hdcp22(
				dpcd_addr[msg_name] + i * DPCD_PACKET_SIZE,
				remain,
				&data[i * DPCD_PACKET_SIZE]);
			if (ret) {
				hdcp_err("dpcd read fail. ret(%d)\n", ret);
				return ret;
			}
		}
		return 0;
	} else
		return displayport_dpcd_read_for_hdcp22(dpcd_addr[msg_name], size, data);
}

int hdcp_dplink_send(uint32_t msg_name, uint8_t *data, uint32_t size)
{
	int i;
	int ret;

	if (size > DPCD_PACKET_SIZE) {
		for (i = 0; i < (size / DPCD_PACKET_SIZE); i++) {
			ret = displayport_dpcd_write_for_hdcp22(
				dpcd_addr[msg_name] + i * DPCD_PACKET_SIZE,
				DPCD_PACKET_SIZE,
				&data[i * DPCD_PACKET_SIZE]);
			if (ret) {
				hdcp_err("dpcd write fail. ret(%d)\n", ret);
				return ret;
			}
		}
		return 0;
	}
	else
		return displayport_dpcd_write_for_hdcp22(dpcd_addr[msg_name], size, data);
}
#endif
