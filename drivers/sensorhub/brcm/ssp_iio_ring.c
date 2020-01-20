/*
 *  Copyright (C) 2011, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>

#include "ssp.h"
#include <linux/iio/iio.h>
#include <linux/iio/buffer.h>
#include <linux/iio/buffer_impl.h>
#include <linux/iio/kfifo_buf.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/sysfs.h>
/*
 *#include "../../staging/iio/iio.h"
 *#include "../../staging/iio/kfifo_buf.h"
 *#include "../../staging/iio/trigger_consumer.h"
 *#include "../../staging/iio/sysfs.h"
 */

void ssp_iio_unconfigure_ring(struct iio_dev *indio_dev)
{
	iio_kfifo_free(indio_dev->buffer);
}

static int ssp_predisable(struct iio_dev *indio_dev)
{
	return 0;
}

static int ssp_preenable(struct iio_dev *indio_dev)
{
	return 0;
}


static const struct iio_buffer_setup_ops ssp_iio_ring_setup_ops = {
	.preenable = &ssp_preenable,
	.predisable = &ssp_predisable,
};

int ssp_iio_configure_ring(struct iio_dev *indio_dev)
{
	struct iio_buffer *ring;

	ring = iio_kfifo_allocate();
	if (!ring)
		return -ENOMEM;

	/* setup ring buffer */
	ring->bytes_per_datum = 8;
	ring->scan_timestamp = true;
	iio_device_attach_buffer(indio_dev, ring);

	indio_dev->setup_ops = &ssp_iio_ring_setup_ops;
	/*
	 *scan count double count timestamp. should subtract 1. but
	 *number of channels still includes timestamp
	 */

	indio_dev->modes |= INDIO_BUFFER_SOFTWARE;

	return 0;
}

