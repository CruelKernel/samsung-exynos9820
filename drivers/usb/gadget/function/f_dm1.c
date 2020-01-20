/*
 * f_dm1.c - generic USB serial function driver
 * modified from f_serial.c and f_diag.c
 * ttyGS*
 *
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 by David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>

#include <linux/configfs.h>
#include <linux/usb/composite.h>

#include "../configfs.h"
#include "u_serial.h"

#define MAX_INST_NAME_LEN		40
/*  DM1_PORT NUM : /dev/ttyGS* port number */
#define DM1_PORT_NUM			3
/*
 * This function packages a simple "generic serial" port with no real
 * control mechanisms, just raw data transfer over two bulk endpoints.
 *
 * Because it's not standardized, this isn't as interoperable as the
 * CDC ACM driver.  However, for many purposes it's just as functional
 * if you can arrange appropriate host side drivers.
 */

struct dm1_descs {
	struct usb_endpoint_descriptor	*in;
	struct usb_endpoint_descriptor	*out;
};

struct f_dm1 {
	struct gserial			port;
	u8				data_id;
	u8				port_num;

	struct dm1_descs			fs;
	struct dm1_descs			hs;
};

static inline struct f_dm1 *func_to_dm1(struct usb_function *f)
{
	return container_of(f, struct f_dm1, port.func);
}

/*-------------------------------------------------------------------------*/

/* interface descriptor: */
static struct usb_interface_descriptor dm1_interface_desc  = {
	.bLength =		USB_DT_INTERFACE_SIZE,
	.bDescriptorType =	USB_DT_INTERFACE,
	/* .bInterfaceNumber = DYNAMIC */
	.bNumEndpoints =	2,
	.bInterfaceClass =	USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass =	0x10,
	.bInterfaceProtocol =	0x01,
	/* .iInterface = DYNAMIC */
};

/* full speed support: */

static struct usb_endpoint_descriptor dm1_fs_in_desc  = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_endpoint_descriptor dm1_fs_out_desc  = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
};

static struct usb_descriptor_header *dm1_fs_function[]  = {
	(struct usb_descriptor_header *) &dm1_interface_desc,
	(struct usb_descriptor_header *) &dm1_fs_in_desc,
	(struct usb_descriptor_header *) &dm1_fs_out_desc,
	NULL,
};

/* high speed support: */

static struct usb_endpoint_descriptor dm1_hs_in_desc  = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};

static struct usb_endpoint_descriptor dm1_hs_out_desc  = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};
static struct usb_descriptor_header *dm1_hs_function[]  = {
	(struct usb_descriptor_header *) &dm1_interface_desc,
	(struct usb_descriptor_header *) &dm1_hs_in_desc,
	(struct usb_descriptor_header *) &dm1_hs_out_desc,
	NULL,
};

static struct usb_endpoint_descriptor dm1_ss_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_endpoint_descriptor dm1_ss_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor dm1_ss_bulk_comp_desc = {
	.bLength =              sizeof dm1_ss_bulk_comp_desc,
	.bDescriptorType =      USB_DT_SS_ENDPOINT_COMP,
};

static struct usb_descriptor_header *dm1_ss_function[] = {
	(struct usb_descriptor_header *) &dm1_interface_desc,
	(struct usb_descriptor_header *) &dm1_ss_in_desc,
	(struct usb_descriptor_header *) &dm1_ss_bulk_comp_desc,
	(struct usb_descriptor_header *) &dm1_ss_out_desc,
	(struct usb_descriptor_header *) &dm1_ss_bulk_comp_desc,
	NULL,
};


/* string descriptors: */
#define F_DM1_IDX	0

static struct usb_string dm1_string_defs[] = {
	[F_DM1_IDX].s = "Samsung Android DM1",
	{  /* ZEROES END LIST */ },
};

static struct usb_gadget_strings dm1_string_table = {
	.language =		0x0409,	/* en-us */
	.strings =		dm1_string_defs,
};

static struct usb_gadget_strings *dm1_strings[] = {
	&dm1_string_table,
	NULL,
};

struct dm1_instance {
	struct usb_function_instance func_inst;
	const char *name;
	struct f_dm1 *dm1;
};
/*-------------------------------------------------------------------------*/

static int dm1_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct f_dm1		*dm1 = func_to_dm1(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int status;

	/* we know alt == 0, so this is an activation or a reset */

	if (dm1->port.in->driver_data) {
		DBG(cdev, "reset generic ttyGS%d\n", dm1->port_num);
		gserial_disconnect(&dm1->port);
	} else {
		DBG(cdev, "activate generic ttyGS%d\n", dm1->port_num);
	}
	if (!dm1->port.in->desc || !dm1->port.out->desc) {
			DBG(cdev, "activate dm1 ttyGS%d\n", dm1->port_num);
			if (config_ep_by_speed(cdev->gadget, f,
					       dm1->port.in) ||
			    config_ep_by_speed(cdev->gadget, f,
					       dm1->port.out)) {
				dm1->port.in->desc = NULL;
				dm1->port.out->desc = NULL;
				return -EINVAL;
			}
	}


	status = gserial_connect(&dm1->port, dm1->port_num);
	printk(KERN_DEBUG "usb: %s run generic_connect(%d)", __func__,
			dm1->port_num);

	if (status < 0) {
		printk(KERN_DEBUG "fail to activate generic ttyGS%d\n",
				dm1->port_num);

		return status;
	}

	return 0;
}

static void dm1_disable(struct usb_function *f)
{
	struct f_dm1	*dm1 = func_to_dm1(f);

	printk(KERN_DEBUG "usb: %s generic ttyGS%d deactivated\n", __func__,
			dm1->port_num);
	gserial_disconnect(&dm1->port);
}

/*-------------------------------------------------------------------------*/

/* serial function driver setup/binding */

static int
dm1_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct f_dm1		*dm1 = func_to_dm1(f);
	int			status;
	struct usb_ep		*ep;

	/* maybe allocate device-global string ID */
	if (dm1_string_defs[F_DM1_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		dm1_string_defs[F_DM1_IDX].id = status;
	}
	/* allocate instance-specific interface IDs */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;
	dm1->data_id = status;
	dm1_interface_desc.bInterfaceNumber = status;

	status = -ENODEV;

	/* allocate instance-specific endpoints */
	ep = usb_ep_autoconfig(cdev->gadget, &dm1_fs_in_desc);
	if (!ep)
		goto fail;
	dm1->port.in = ep;
	ep->driver_data = cdev;	/* claim */

	ep = usb_ep_autoconfig(cdev->gadget, &dm1_fs_out_desc);
	if (!ep)
		goto fail;
	dm1->port.out = ep;
	ep->driver_data = cdev;	/* claim */
	printk(KERN_INFO "[%s]   in =0x%p , out =0x%p\n", __func__,
				dm1->port.in, dm1->port.out);

	/* copy descriptors, and track endpoint copies */
	f->fs_descriptors = usb_copy_descriptors(dm1_fs_function);




	/* support all relevant hardware speeds... we expect that when
	 * hardware is dual speed, all bulk-capable endpoints work at
	 * both speeds
	 */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		dm1_hs_in_desc.bEndpointAddress =
				dm1_fs_in_desc.bEndpointAddress;
		dm1_hs_out_desc.bEndpointAddress =
				dm1_fs_out_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(dm1_hs_function);
	}
	if (gadget_is_superspeed(c->cdev->gadget)) {
		dm1_ss_in_desc.bEndpointAddress =
			dm1_fs_in_desc.bEndpointAddress;
		dm1_ss_out_desc.bEndpointAddress =
			dm1_fs_out_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->ss_descriptors = usb_copy_descriptors(dm1_ss_function);
		if (!f->ss_descriptors)
			goto fail;
	}

	printk("usb: [%s] generic ttyGS%d: %s speed IN/%s OUT/%s\n",
			__func__,
			dm1->port_num,
			gadget_is_superspeed(c->cdev->gadget) ? "super" :
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			dm1->port.in->name, dm1->port.out->name);
	return 0;

fail:
	/* we might as well release our claims on endpoints */
	if (dm1->port.out)
		dm1->port.out->driver_data = NULL;
	if (dm1->port.in)
		dm1->port.in->driver_data = NULL;

	printk(KERN_ERR "%s: can't bind, err %d\n", f->name, status);

	return status;
}

static void
dm1_unbind(struct usb_configuration *c, struct usb_function *f)
{
	if (gadget_is_dualspeed(c->cdev->gadget))
		usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->fs_descriptors);
	printk(KERN_DEBUG "usb: %s\n", __func__);
}

/*
 * dm1_bind_config - add a generic serial function to a configuration
 * @c: the configuration to support the serial instance
 * @port_num: /dev/ttyGS* port this interface will use
 * Context: single threaded during gadget setup
 *
 * Returns zero on success, else negative errno.
 *
 * Caller must have called @gserial_setup() with enough ports to
 * handle all the ones it binds.  Caller is also responsible
 * for calling @gserial_cleanup() before module unload.
 */
int dm1_bind_config(struct usb_configuration *c, u8 port_num)
{
	struct f_dm1	*dm1;
	int		status;

	/* REVISIT might want instance-specific strings to help
	 * distinguish instances ...
	 */

	/* maybe allocate device-global string ID */
	if (dm1_string_defs[F_DM1_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			return status;
		dm1_string_defs[F_DM1_IDX].id = status;
	}

	/* allocate and initialize one new instance */
	dm1 = kzalloc(sizeof *dm1, GFP_KERNEL);
	if (!dm1)
		return -ENOMEM;

	dm1->port_num = DM1_PORT_NUM;

	dm1->port.func.name = "dm1";
	dm1->port.func.strings = dm1_strings;
	dm1->port.func.bind = dm1_bind;
	dm1->port.func.unbind = dm1_unbind;
	dm1->port.func.set_alt = dm1_set_alt;
	dm1->port.func.disable = dm1_disable;

	status = usb_add_function(c, &dm1->port.func);
	if (status)
		kfree(dm1);
	return status;
}
static struct dm1_instance *to_dm1_instance(struct config_item *item)
{
	return container_of(to_config_group(item), struct dm1_instance,
		func_inst.group);
}
static void dm1_attr_release(struct config_item *item)
{
	struct dm1_instance *fi_dm1 = to_dm1_instance(item);
	usb_put_function_instance(&fi_dm1->func_inst);
}

static struct configfs_item_operations dm1_item_ops = {
	.release        = dm1_attr_release,
};

static struct config_item_type dm1_func_type = {
	.ct_item_ops    = &dm1_item_ops,
	.ct_owner       = THIS_MODULE,
};


static struct dm1_instance *to_fi_dm1(struct usb_function_instance *fi)
{
	return container_of(fi, struct dm1_instance, func_inst);
}

static int dm1_set_inst_name(struct usb_function_instance *fi, const char *name)
{
	struct dm1_instance *fi_dm1;
	char *ptr;
	int name_len;

	name_len = strlen(name) + 1;
	if (name_len > MAX_INST_NAME_LEN)
		return -ENAMETOOLONG;

	ptr = kstrndup(name, name_len, GFP_KERNEL);
	if (!ptr)
		return -ENOMEM;

	fi_dm1 = to_fi_dm1(fi);
	fi_dm1->name = ptr;

	return 0;
}

static void dm1_free_inst(struct usb_function_instance *fi)
{
	struct dm1_instance *fi_dm1;

	fi_dm1 = to_fi_dm1(fi);
	kfree(fi_dm1->name);
	kfree(fi_dm1);
}

struct usb_function_instance *alloc_inst_dm1(bool dm1_config)
{
	struct dm1_instance *fi_dm1;

	fi_dm1 = kzalloc(sizeof(*fi_dm1), GFP_KERNEL);
	if (!fi_dm1)
		return ERR_PTR(-ENOMEM);
	fi_dm1->func_inst.set_inst_name = dm1_set_inst_name;
	fi_dm1->func_inst.free_func_inst = dm1_free_inst;


	config_group_init_type_name(&fi_dm1->func_inst.group,
					"", &dm1_func_type);

	return  &fi_dm1->func_inst;
}
EXPORT_SYMBOL_GPL(alloc_inst_dm1);

static struct usb_function_instance *dm1_alloc_inst(void)
{
		return alloc_inst_dm1(true);
}

static void dm1_free(struct usb_function *f)
{
	struct f_dm1	*dm1 = func_to_dm1(f);

	kfree(dm1);
}

struct usb_function *function_alloc_dm1(struct usb_function_instance *fi, bool dm1_config)
{

	struct dm1_instance *fi_dm1 = to_fi_dm1(fi);
	struct f_dm1	*dm1;
	int		ret;

	/* REVISIT might want instance-specific strings to help
	 * distinguish instances ...
	 */

	/* allocate and initialize one new instance */
	dm1 = kzalloc(sizeof *dm1, GFP_KERNEL);
	if (!dm1)
		return ERR_PTR(-ENOMEM);


	dm1->port_num = DM1_PORT_NUM;

	dm1->port.func.name = "dm1";
	dm1->port.func.strings = dm1_strings;
	dm1->port.func.bind = dm1_bind;
	dm1->port.func.unbind = dm1_unbind;
	dm1->port.func.set_alt = dm1_set_alt;
	dm1->port.func.disable = dm1_disable;
	dm1->port.func.free_func = dm1_free;

	fi_dm1->dm1 = dm1;

	ret = gserial_alloc_line(&dm1->port_num);
	if (ret) {
		kfree(dm1);
		return ERR_PTR(ret);
	}

	return &dm1->port.func;
}
EXPORT_SYMBOL_GPL(function_alloc_dm1);

static struct usb_function *dm1_alloc(struct usb_function_instance *fi)
{
	return function_alloc_dm1(fi, true);
}

DECLARE_USB_FUNCTION_INIT(dm1, dm1_alloc_inst, dm1_alloc);
MODULE_LICENSE("GPL");
