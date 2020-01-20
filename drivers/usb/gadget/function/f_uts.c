/*
 * f_uts.c -- USB CDC serial (ACM) function driver
 *
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 by David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 * Copyright (C) 2009 by Samsung Electronics
 * Author: Michal Nazarewicz (mina86@mina86.com)
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>

#include "u_serial.h"


/*
 * This CDC ACM function support just wraps control functions and
 * notifications around the generic serial-over-usb code.
 *
 * Because CDC ACM is standardized by the USB-IF, many host operating
 * systems have drivers for it.  Accordingly, ACM is the preferred
 * interop solution for serial-port type connections.  The control
 * models are often not necessary, and in any case don't do much in
 * this bare-bones implementation.
 *
 * Note that even MS-Windows has some support for ACM.  However, that
 * support is somewhat broken because when you use ACM in a composite
 * device, having multiple interfaces confuses the poor OS.  It doesn't
 * seem to understand CDC Union descriptors.  The new "association"
 * descriptors (roughly equivalent to CDC Unions) may sometimes help.
 */

/* UTS_PORT NUM : /dev/ttyGS* port number */
#define UTS_PORT_NUM                   2

struct f_uts {
	struct gserial			port;
	u8				ctrl_id, data_id;
	u8				port_num;

	u8				pending;

	/* lock is mostly for pending and notify_req ... they get accessed
	 * by callbacks both from tty (open/close/break) under its spinlock,
	 * and notify_req.complete() which can't use that lock.
	 */
	spinlock_t			lock;

	struct usb_ep			*notify;
	struct usb_request		*notify_req;

	struct usb_cdc_line_coding	port_line_coding;	/* 8-N-1 etc */

	/* SetControlLineState request -- CDC 1.1 section 6.2.14 (INPUT) */
	u16				port_handshake_bits;
#define ACM_CTRL_RTS	(1 << 1)	/* unused with full duplex */
#define ACM_CTRL_DTR	(1 << 0)	/* host is ready for data r/w */

	/* SerialState notification -- CDC 1.1 section 6.3.5 (OUTPUT) */
	u16				serial_state;
#define ACM_CTRL_OVERRUN	(1 << 6)
#define ACM_CTRL_PARITY		(1 << 5)
#define ACM_CTRL_FRAMING	(1 << 4)
#define ACM_CTRL_RI		(1 << 3)
#define ACM_CTRL_BRK		(1 << 2)
#define ACM_CTRL_DSR		(1 << 1)
#define ACM_CTRL_DCD		(1 << 0)
};

static inline struct f_uts *func_to_uts(struct usb_function *f)
{
	return container_of(f, struct f_uts, port.func);
}

static inline struct f_uts *port_to_uts(struct gserial *p)
{
	return container_of(p, struct f_uts, port);
}

/*-------------------------------------------------------------------------*/

/* notification endpoint uses smallish and infrequent fixed-size messages */

#define GS_NOTIFY_INTERVAL_MS		32
#define GS_NOTIFY_MAXPACKET		10	/* notification + 2 bytes */

/* interface and class descriptors: */

static struct usb_interface_assoc_descriptor
uts_iad_descriptor = {
	.bLength =		sizeof(uts_iad_descriptor),
	.bDescriptorType =	USB_DT_INTERFACE_ASSOCIATION,

	/* .bFirstInterface =	DYNAMIC, */
	.bInterfaceCount = 2,	// control + data
	.bFunctionClass =	USB_CLASS_COMM,
	.bFunctionSubClass =	USB_CDC_SUBCLASS_ACM,
	.bFunctionProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	/* .iFunction =		DYNAMIC */
};


static struct usb_interface_descriptor uts_control_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	1,
	.bInterfaceClass =	USB_CLASS_COMM,
	.bInterfaceSubClass =	USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol =	USB_CDC_ACM_PROTO_AT_V25TER,
	/* .iInterface = DYNAMIC */
};

static struct usb_interface_descriptor uts_data_interface_desc = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_CDC_DATA,
	.bInterfaceSubClass =	0,
	.bInterfaceProtocol =	0,
	/* .iInterface = DYNAMIC */
};

static struct usb_cdc_header_desc uts_header_desc = {
	.bLength =		sizeof(uts_header_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_HEADER_TYPE,
	.bcdCDC =		cpu_to_le16(0x0110),
};

static struct usb_cdc_call_mgmt_descriptor
uts_call_mgmt_descriptor = {
	.bLength =		sizeof(uts_call_mgmt_descriptor),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_CALL_MANAGEMENT_TYPE,
	.bmCapabilities =	0,
	/* .bDataInterface = DYNAMIC */
};

static struct usb_cdc_acm_descriptor uts_descriptor = {
	.bLength =		sizeof(uts_descriptor),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_ACM_TYPE,
	.bmCapabilities =	USB_CDC_CAP_LINE,
};

static struct usb_cdc_union_desc uts_union_desc = {
	.bLength =		sizeof(uts_union_desc),
	.bDescriptorType =	USB_DT_CS_INTERFACE,
	.bDescriptorSubType =	USB_CDC_UNION_TYPE,
	/* .bMasterInterface0 =	DYNAMIC */
	/* .bSlaveInterface0 =	DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor uts_fs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		GS_NOTIFY_INTERVAL_MS,
};

static struct usb_endpoint_descriptor uts_fs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor uts_fs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *uts_fs_function[] = {
	(struct usb_descriptor_header *) &uts_iad_descriptor,
	(struct usb_descriptor_header *) &uts_control_interface_desc,
	(struct usb_descriptor_header *) &uts_header_desc,
	(struct usb_descriptor_header *) &uts_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &uts_descriptor,
	(struct usb_descriptor_header *) &uts_union_desc,
	(struct usb_descriptor_header *) &uts_fs_notify_desc,
	(struct usb_descriptor_header *) &uts_data_interface_desc,
	(struct usb_descriptor_header *) &uts_fs_in_desc,
	(struct usb_descriptor_header *) &uts_fs_out_desc,
	NULL,
};

/* high speed support: */
static struct usb_endpoint_descriptor uts_hs_notify_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	cpu_to_le16(GS_NOTIFY_MAXPACKET),
	.bInterval =		USB_MS_TO_HS_INTERVAL(GS_NOTIFY_INTERVAL_MS),
};

static struct usb_endpoint_descriptor uts_hs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_endpoint_descriptor uts_hs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(512),
};

static struct usb_descriptor_header *uts_hs_function[] = {
	(struct usb_descriptor_header *) &uts_iad_descriptor,
	(struct usb_descriptor_header *) &uts_control_interface_desc,
	(struct usb_descriptor_header *) &uts_header_desc,
	(struct usb_descriptor_header *) &uts_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &uts_descriptor,
	(struct usb_descriptor_header *) &uts_union_desc,
	(struct usb_descriptor_header *) &uts_hs_notify_desc,
	(struct usb_descriptor_header *) &uts_data_interface_desc,
	(struct usb_descriptor_header *) &uts_hs_in_desc,
	(struct usb_descriptor_header *) &uts_hs_out_desc,
	NULL,
};

static struct usb_endpoint_descriptor uts_ss_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor uts_ss_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor uts_ss_bulk_comp_desc = {
	.bLength =              sizeof(uts_ss_bulk_comp_desc),
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,
};

static struct usb_descriptor_header *uts_ss_function[] = {
	(struct usb_descriptor_header *) &uts_iad_descriptor,
	(struct usb_descriptor_header *) &uts_control_interface_desc,
	(struct usb_descriptor_header *) &uts_header_desc,
	(struct usb_descriptor_header *) &uts_call_mgmt_descriptor,
	(struct usb_descriptor_header *) &uts_descriptor,
	(struct usb_descriptor_header *) &uts_union_desc,
	(struct usb_descriptor_header *) &uts_hs_notify_desc,
	(struct usb_descriptor_header *) &uts_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &uts_data_interface_desc,
	(struct usb_descriptor_header *) &uts_ss_in_desc,
	(struct usb_descriptor_header *) &uts_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &uts_ss_out_desc,
	(struct usb_descriptor_header *) &uts_ss_bulk_comp_desc,
	NULL,
};

/* string descriptors: */

#define ACM_CTRL_IDX	0
#define ACM_DATA_IDX	1
#define ACM_IAD_IDX	2

/* static strings, in UTF-8 */
static struct usb_string uts_string_defs[] = {
	[ACM_CTRL_IDX].s = "CDC Abstract Control Model (ACM2)",
	[ACM_DATA_IDX].s = "CDC ACM2 Data",
	[ACM_IAD_IDX].s = "CDC Serial",
	{  } /* end of list */
};

static struct usb_gadget_strings uts_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		uts_string_defs,
};

static struct usb_gadget_strings *uts_strings[] = {
	&uts_string_table,
	NULL,
};

/*-------------------------------------------------------------------------*/

/* ACM control ... data handling is delegated to tty library code.
 * The main task of this function is to activate and deactivate
 * that code based on device state; track parameters like line
 * speed, handshake state, and so on; and issue notifications.
 */

static void uts_complete_set_line_coding(struct usb_ep *ep,
		struct usb_request *req)
{
	struct f_uts	*uts = ep->driver_data;
	struct usb_composite_dev *cdev = uts->port.func.config->cdev;

	if (req->status != 0) {
		dev_dbg(&cdev->gadget->dev, "uts ttyGS%d completion, err %d\n",
			uts->port_num, req->status);
		return;
	}

	/* normal completion */
	if (req->actual != sizeof(uts->port_line_coding)) {
		dev_dbg(&cdev->gadget->dev, "uts ttyGS%d short resp, len %d\n",
			uts->port_num, req->actual);
		usb_ep_set_halt(ep);
	} else {
		struct usb_cdc_line_coding	*value = req->buf;

		/* REVISIT:  we currently just remember this data.
		 * If we change that, (a) validate it first, then
		 * (b) update whatever hardware needs updating,
		 * (c) worry about locking.  This is information on
		 * the order of 9600-8-N-1 ... most of which means
		 * nothing unless we control a real RS232 line.
		 */
		uts->port_line_coding = *value;
	}
}

static int uts_setup(struct usb_function *f, const struct usb_ctrlrequest *ctrl)
{
	struct f_uts		*uts = func_to_uts(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request	*req = cdev->req;
	int			value = -EOPNOTSUPP;
	u16			w_index = le16_to_cpu(ctrl->wIndex);
	u16			w_value = le16_to_cpu(ctrl->wValue);
	u16			w_length = le16_to_cpu(ctrl->wLength);

	/* composite driver infrastructure handles everything except
	 * CDC class messages; interface activation uses set_alt().
	 *
	 * Note CDC spec table 4 lists the ACM request profile.  It requires
	 * encapsulated command support ... we don't handle any, and respond
	 * to them by stalling.  Options include get/set/clear comm features
	 * (not that useful) and SEND_BREAK.
	 */
	switch ((ctrl->bRequestType << 8) | ctrl->bRequest) {

	/* SET_LINE_CODING ... just read and save what the host sends */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_LINE_CODING:
		if (w_length != sizeof(struct usb_cdc_line_coding)
				|| w_index != uts->ctrl_id)
			goto invalid;

		value = w_length;
		cdev->gadget->ep0->driver_data = uts;
		req->complete = uts_complete_set_line_coding;
		break;

	/* GET_LINE_CODING ... return what host sent, or initial value */
	case ((USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_GET_LINE_CODING:
		if (w_index != uts->ctrl_id)
			goto invalid;

		value = min_t(unsigned int, w_length,
				sizeof(struct usb_cdc_line_coding));
		memcpy(req->buf, &uts->port_line_coding, value);
		break;

	/* SET_CONTROL_LINE_STATE ... save what the host sent */
	case ((USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE) << 8)
			| USB_CDC_REQ_SET_CONTROL_LINE_STATE:
		if (w_index != uts->ctrl_id)
			goto invalid;

		value = 0;

		/* FIXME we should not allow data to flow until the
		 * host sets the ACM_CTRL_DTR bit; and when it clears
		 * that bit, we should return to that no-flow state.
		 */
		uts->port_handshake_bits = w_value;
		break;

	default:
invalid:
		dev_vdbg(&cdev->gadget->dev,
			 "invalid control req%02x.%02x v%04x i%04x l%d\n",
			 ctrl->bRequestType, ctrl->bRequest,
			 w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		dev_dbg(&cdev->gadget->dev,
			"uts ttyGS%d req%02x.%02x v%04x i%04x l%d\n",
			uts->port_num, ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(cdev, "uts response on ttyGS%d, err %d\n",
					uts->port_num, value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
}

static int uts_set_alt(struct usb_function *f, unsigned int intf, unsigned int alt)
{
	struct f_uts		*uts = func_to_uts(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	/* we know alt == 0, so this is an activation or a reset */

	if (intf == uts->ctrl_id) {
		dev_vdbg(&cdev->gadget->dev,
				"reset uts control interface %d\n", intf);
		usb_ep_disable(uts->notify);

		if (!uts->notify->desc)
			if (config_ep_by_speed(cdev->gadget, f, uts->notify))
				return -EINVAL;

		usb_ep_enable(uts->notify);

	} else if (intf == uts->data_id) {
		if (uts->notify->enabled) {
			dev_dbg(&cdev->gadget->dev,
				"reset uts ttyGS%d\n", uts->port_num);
			gserial_disconnect(&uts->port);
		}
		if (!uts->port.in->desc || !uts->port.out->desc) {
			dev_dbg(&cdev->gadget->dev,
				"activate uts ttyGS%d\n", uts->port_num);
			if (config_ep_by_speed(cdev->gadget, f,
					       uts->port.in) ||
			    config_ep_by_speed(cdev->gadget, f,
					       uts->port.out)) {
				uts->port.in->desc = NULL;
				uts->port.out->desc = NULL;
				return -EINVAL;
			}
		}
		gserial_connect(&uts->port, uts->port_num);

	} else
		return -EINVAL;

	return 0;
}

static void uts_disable(struct usb_function *f)
{
	struct f_uts	*uts = func_to_uts(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	dev_dbg(&cdev->gadget->dev, "uts ttyGS%d deactivated\n", uts->port_num);
	gserial_disconnect(&uts->port);
	usb_ep_disable(uts->notify);
}

/*-------------------------------------------------------------------------*/

/**
 * uts_cdc_notify - issue CDC notification to host
 * @uts: wraps host to be notified
 * @type: notification type
 * @value: Refer to cdc specs, wValue field.
 * @data: data to be sent
 * @length: size of data
 * Context: irqs blocked, uts->lock held, uts_notify_req non-null
 *
 * Returns zero on success or a negative errno.
 *
 * See section 6.3.5 of the CDC 1.1 specification for information
 * about the only notification we issue:  SerialState change.
 */
static int uts_cdc_notify(struct f_uts *uts, u8 type, u16 value,
		void *data, unsigned int length)
{
	struct usb_ep			*ep = uts->notify;
	struct usb_request		*req;
	struct usb_cdc_notification	*notify;
	const unsigned int		len = sizeof(*notify) + length;
	void				*buf;
	int				status;

	req = uts->notify_req;
	uts->notify_req = NULL;
	uts->pending = false;

	req->length = len;
	notify = req->buf;
	buf = notify + 1;

	notify->bmRequestType = USB_DIR_IN | USB_TYPE_CLASS
			| USB_RECIP_INTERFACE;
	notify->bNotificationType = type;
	notify->wValue = cpu_to_le16(value);
	notify->wIndex = cpu_to_le16(uts->ctrl_id);
	notify->wLength = cpu_to_le16(length);
	memcpy(buf, data, length);

	/* ep_queue() can complete immediately if it fills the fifo... */
	spin_unlock(&uts->lock);
	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	spin_lock(&uts->lock);

	if (status < 0) {
		ERROR(uts->port.func.config->cdev,
				"uts ttyGS%d can't notify serial state, %d\n",
				uts->port_num, status);
		uts->notify_req = req;
	}

	return status;
}

static int uts_notify_serial_state(struct f_uts *uts)
{
	struct usb_composite_dev *cdev = uts->port.func.config->cdev;
	int			status;
	__le16			serial_state;

	spin_lock(&uts->lock);
	if (uts->notify_req) {
		dev_dbg(&cdev->gadget->dev, "uts ttyGS%d serial state %04x\n",
			uts->port_num, uts->serial_state);
		serial_state = cpu_to_le16(uts->serial_state);
		status = uts_cdc_notify(uts, USB_CDC_NOTIFY_SERIAL_STATE,
				0, &serial_state, sizeof(uts->serial_state));
	} else {
		uts->pending = true;
		status = 0;
	}
	spin_unlock(&uts->lock);
	return status;
}

static void uts_cdc_notify_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct f_uts		*uts = req->context;
	u8			doit = false;

	/* on this call path we do NOT hold the port spinlock,
	 * which is why ACM needs its own spinlock
	 */
	spin_lock(&uts->lock);
	if (req->status != -ESHUTDOWN)
		doit = uts->pending;
	uts->notify_req = req;
	spin_unlock(&uts->lock);

	if (doit)
		uts_notify_serial_state(uts);
}

/* connect == the TTY link is open */

static void uts_connect(struct gserial *port)
{
	struct f_uts		*uts = port_to_uts(port);

	uts->serial_state |= ACM_CTRL_DSR | ACM_CTRL_DCD;
	uts_notify_serial_state(uts);
}

static void uts_disconnect(struct gserial *port)
{
	struct f_uts		*uts = port_to_uts(port);

	uts->serial_state &= ~(ACM_CTRL_DSR | ACM_CTRL_DCD);
	uts_notify_serial_state(uts);
}

static int uts_send_break(struct gserial *port, int duration)
{
	struct f_uts		*uts = port_to_uts(port);
	u16			state;

	state = uts->serial_state;
	state &= ~ACM_CTRL_BRK;
	if (duration)
		state |= ACM_CTRL_BRK;

	uts->serial_state = state;
	return uts_notify_serial_state(uts);
}

/*-------------------------------------------------------------------------*/

/* ACM function driver setup/binding */
static int
uts_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_uts		*uts = func_to_uts(f);
	struct usb_string	*us;
	int			status;
	struct usb_ep		*ep;

	/* REVISIT might want instance-specific strings to help
	 * distinguish instances ...
	 */

	/* maybe allocate device-global string IDs, and patch descriptors */
	us = usb_gstrings_attach(cdev, uts_strings,
			ARRAY_SIZE(uts_string_defs));
	if (IS_ERR(us))
		return PTR_ERR(us);
	uts_control_interface_desc.iInterface = us[ACM_CTRL_IDX].id;
	uts_data_interface_desc.iInterface = us[ACM_DATA_IDX].id;
	uts_iad_descriptor.iFunction = us[ACM_IAD_IDX].id;

	/* allocate instance-specific interface IDs, and patch descriptors */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	uts->ctrl_id = status;
	uts_iad_descriptor.bFirstInterface = status;

	uts_control_interface_desc.bInterfaceNumber = status;
	uts_union_desc .bMasterInterface0 = status;

	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	uts->data_id = status;

	uts_data_interface_desc.bInterfaceNumber = status;
	uts_union_desc.bSlaveInterface0 = status;
	uts_call_mgmt_descriptor.bDataInterface = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &uts_fs_in_desc);
	if (!ep)
		goto fail;
	uts->port.in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &uts_fs_out_desc);
	if (!ep)
		goto fail;
	uts->port.out = ep;

	ep = usb_ep_autoconfig(cdev->gadget, &uts_fs_notify_desc);
	if (!ep)
		goto fail;
	uts->notify = ep;

	/* allocate notification */
	uts->notify_req = gs_alloc_req(ep,
			sizeof(struct usb_cdc_notification) + 2,
			GFP_KERNEL);
	if (!uts->notify_req)
		goto fail;

	uts->notify_req->complete = uts_cdc_notify_complete;
	uts->notify_req->context = uts;

	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	uts_hs_in_desc.bEndpointAddress = uts_fs_in_desc.bEndpointAddress;
	uts_hs_out_desc.bEndpointAddress = uts_fs_out_desc.bEndpointAddress;
	uts_hs_notify_desc.bEndpointAddress =
		uts_fs_notify_desc.bEndpointAddress;

	uts_ss_in_desc.bEndpointAddress = uts_fs_in_desc.bEndpointAddress;
	uts_ss_out_desc.bEndpointAddress = uts_fs_out_desc.bEndpointAddress;

	status = usb_assign_descriptors(f, uts_fs_function, uts_hs_function,
			uts_ss_function, NULL);
	if (status)
		goto fail;

	dev_dbg(&cdev->gadget->dev,
		"uts ttyGS%d: %s speed IN/%s OUT/%s NOTIFY/%s\n",
		uts->port_num,
		gadget_is_superspeed(c->cdev->gadget) ? "super" :
		gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
		uts->port.in->name, uts->port.out->name,
		uts->notify->name);
	return 0;

fail:
	if (uts->notify_req)
		gs_free_req(uts->notify, uts->notify_req);

	ERROR(cdev, "%s/%p: can't bind, err %d\n", f->name, f, status);

	return status;
}

static void uts_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_uts		*uts = func_to_uts(f);

	uts_string_defs[0].id = 0;
	usb_free_all_descriptors(f);
	if (uts->notify_req)
		gs_free_req(uts->notify, uts->notify_req);
}

static void uts_free_func(struct usb_function *f)
{
	struct f_uts		*uts = func_to_uts(f);

	kfree(uts);
}

static struct usb_function *uts_alloc_func(struct usb_function_instance *fi)
{
	struct f_serial_opts *opts;
	struct f_uts *uts;

	uts = kzalloc(sizeof(*uts), GFP_KERNEL);
	if (!uts)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&uts->lock);

	uts->port.connect = uts_connect;
	uts->port.disconnect = uts_disconnect;
	uts->port.send_break = uts_send_break;

	uts->port.func.name = "uts";
	uts->port.func.strings = uts_strings;
	/* descriptors are per-instance copies */
	uts->port.func.bind = uts_bind;
	uts->port.func.set_alt = uts_set_alt;
	uts->port.func.setup = uts_setup;
	uts->port.func.disable = uts_disable;

	opts = container_of(fi, struct f_serial_opts, func_inst);
	uts->port_num = opts->port_num;
	uts->port.func.unbind = uts_unbind;
	uts->port.func.free_func = uts_free_func;

	return &uts->port.func;
}

static inline struct f_serial_opts *to_f_serial_opts(struct config_item *item)
{
	return container_of(to_config_group(item), struct f_serial_opts,
			func_inst.group);
}

static void uts_attr_release(struct config_item *item)
{
	struct f_serial_opts *opts = to_f_serial_opts(item);

	usb_put_function_instance(&opts->func_inst);
}

static struct configfs_item_operations uts_item_ops = {
	.release                = uts_attr_release,
};

static ssize_t f_uts_port_num_show(struct config_item *item, char *page)
{
	return sprintf(page, "%u\n", to_f_serial_opts(item)->port_num);
}

CONFIGFS_ATTR_RO(f_uts_, port_num);

static struct configfs_attribute *uts_attrs[] = {
	&f_uts_attr_port_num,
	NULL,
};

static struct config_item_type uts_func_type = {
	.ct_item_ops    = &uts_item_ops,
	.ct_attrs	= uts_attrs,
	.ct_owner       = THIS_MODULE,
};

static void uts_free_instance(struct usb_function_instance *fi)
{
	struct f_serial_opts *opts;

	opts = container_of(fi, struct f_serial_opts, func_inst);
	gserial_free_line(opts->port_num);
	kfree(opts);
}

static struct usb_function_instance *uts_alloc_instance(void)
{
	struct f_serial_opts *opts;
	int ret;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);
	opts->func_inst.free_func_inst = uts_free_instance;
	opts->port_num = UTS_PORT_NUM;
	ret = gserial_alloc_line(&opts->port_num);
	if (ret) {
		kfree(opts);
		return ERR_PTR(ret);
	}
	config_group_init_type_name(&opts->func_inst.group, "",
			&uts_func_type);
	return &opts->func_inst;
}
DECLARE_USB_FUNCTION_INIT(uts, uts_alloc_instance, uts_alloc_func);
MODULE_LICENSE("GPL");
