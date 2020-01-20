/*
 * Copyright (C) 2010 Google, Inc.
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

#ifndef __MODEM_LINK_DEVICE_USB_H__
#define __MODEM_LINK_DEVICE_USB_H__

#include <linux/rtc.h>

enum {
	IF_USB_BOOT_EP = 0,
	IF_USB_FMT_EP = 0,
	IF_USB_RAW_EP,
	IF_USB_RFS_EP,
	IF_USB_CMD_EP,
	_IF_USB_ACMNUM_MAX,
};

#define PS_DATA_CH_01	0xa
#define MULTI_URB 4

enum {
	BOOT_DOWN = 0,
	IPC_CHANNEL
};

enum ch_state {
	STATE_SUSPENDED,
	STATE_RESUMED,
};

struct link_pm_info {
	struct usb_link_device *usb_ld;
};

struct if_usb_devdata;
struct usb_id_info {
	int intf_id;
	int urb_cnt;
	int flags;
#define FLAG_BOOT_DOWN          0x0001  /* USB/HSIC BOOTROM interface */
#define FLAG_IPC_CHANNEL        0x0002  /* normal packet rx/tx interface */
#define FLAG_SEND_NZLP          0x0004  /* hw doesn't require ZLP for tx */
	unsigned int rx_buf_size;
	struct usb_link_device *usb_ld;

	char *description;
	int (*bind)(struct if_usb_devdata *, struct usb_interface *,
			struct usb_link_device *);
	void (*unbind)(struct if_usb_devdata *, struct usb_interface *);
	int (*rx_fixup)(struct if_usb_devdata *, struct sk_buff *skb);
	struct sk_buff *(*tx_fixup)(struct if_usb_devdata *dev,
			struct sk_buff *skb, gfp_t flags);
	void (*intr_complete)(struct urb *urb);
};

struct mif_skb_pool;

struct if_usb_devdata {
	struct usb_interface *data_intf;
	struct usb_link_device *usb_ld;
	struct usb_device *usbdev;
	unsigned int tx_pipe;
	unsigned int rx_pipe;
	struct usb_host_endpoint *status;
	u8 disconnected;

	int format;
	int idx;

	/* Multi-URB style*/
	struct usb_anchor urbs;
	struct usb_anchor reading;

	struct urb *intr_urb;
	unsigned int rx_buf_size;
	enum ch_state state;

	struct usb_id_info *info;
	/* SubClass expend data - optional */
	void *sedata;
	struct io_device *iod;
	unsigned long flags;

	struct sk_buff_head free_rx_q;
	struct sk_buff_head sk_tx_q;
	unsigned tx_pend;
	struct timespec txpend_ts;
	struct rtc_time txpend_tm;
	struct usb_anchor tx_deferd_urbs;
	struct net_device *ndev;
	int net_suspend;
	bool net_connected;

	bool defered_rx;
	struct delayed_work rx_defered_work;

	struct mif_skb_pool *ntb_pool;
};

struct usb_link_device {
	/*COMMON LINK DEVICE*/
	struct link_device ld;

	/*USB SPECIFIC LINK DEVICE*/
	struct usb_device	*usbdev;
	int max_link_ch;
	int max_acm_ch;
	int acm_cnt;
	struct if_usb_devdata *devdata;
	unsigned int		suspended;
	int if_usb_connected;

	struct delayed_work link_event;
	unsigned long events;

	struct notifier_block phy_nfb;
	struct notifier_block pm_nfb;

	/* for debug */
	unsigned debug_pending;
	unsigned tx_cnt;
	unsigned rx_cnt;
	unsigned tx_err;
	unsigned rx_err;
};

enum bit_link_events {
	LINK_EVENT_RECOVERY,
};

/* converts from struct link_device* to struct xxx_link_device* */
#define to_usb_link_device(linkdev) \
			container_of(linkdev, struct usb_link_device, ld)

int usb_tx_skb(struct if_usb_devdata *pipe_data, struct sk_buff *skb);
#endif
