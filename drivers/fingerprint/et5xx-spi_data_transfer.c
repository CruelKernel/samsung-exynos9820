/*
 * Copyright (C) 2016 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include "et5xx.h"


int etspi_io_burst_write_register(struct etspi_data *etspi,
		struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	struct spi_message m;
	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.len = ioc->len + 1,
	};

	if (ioc->len <= 0 || ioc->len + 2 > etspi->bufsiz) {
		status = -ENOMEM;
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}

	memset(etspi->buf, 0, ioc->len + 1);
	*etspi->buf = OP_REG_W_C;
	if (copy_from_user(etspi->buf + 1,
			(const u8 __user *) (uintptr_t) ioc->tx_buf,
			ioc->len)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		goto end;
	}
	pr_debug("%s tx_buf = %p op = %x reg = %x, len = %d\n", __func__,
			ioc->tx_buf, *etspi->buf, *(etspi->buf + 1), xfer.len);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status < 0) {
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}
end:
	return status;
#endif
}

int etspi_io_burst_write_register_backward(struct etspi_data *etspi,
		struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	struct spi_message m;
	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.len = ioc->len + 1,
	};

	if (ioc->len <= 0 || ioc->len + 2 > etspi->bufsiz) {
		status = -ENOMEM;
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}

	memset(etspi->buf, 0, ioc->len + 1);
	*etspi->buf = OP_REG_W_C_BW;
	if (copy_from_user(etspi->buf + 1,
		(const u8 __user *) (uintptr_t)ioc->tx_buf, ioc->len)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		goto end;
	}
	pr_debug("%s tx_buf = %p op = %x reg = %x, len = %d\n", __func__,
		ioc->tx_buf, *etspi->buf, *(etspi->buf + 1), xfer.len);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status < 0) {
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}
end:
	return status;
#endif
}

int etspi_io_burst_read_register(struct etspi_data *etspi,
		struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	struct spi_message m;
	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.rx_buf = etspi->buf,
		.len = ioc->len + 2,
	};

	if (ioc->len <= 0 || ioc->len + 2 > etspi->bufsiz) {
		status = -ENOMEM;
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}

	memset(etspi->buf, 0, xfer.len);
	*etspi->buf = OP_REG_R_C;
	if (copy_from_user(etspi->buf + 1,
			(const u8 __user *) (uintptr_t) ioc->tx_buf, 1)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		goto end;
	}
	pr_debug("%s tx_buf = %p op = %x reg = %x, len = %d\n", __func__,
			ioc->tx_buf, *etspi->buf, *(etspi->buf + 1), xfer.len);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status < 0) {
		status = -ENOMEM;
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}

	if (copy_to_user((u8 __user *) (uintptr_t)ioc->rx_buf, etspi->buf + 2,
				ioc->len)) {
		status = -EFAULT;
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		goto end;
	}
end:
	return status;
#endif
}

int etspi_io_burst_read_register_backward(struct etspi_data *etspi,
		struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	struct spi_message m;
	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.rx_buf = etspi->buf,
		.len = ioc->len + 2,
	};

	if (ioc->len <= 0 || ioc->len + 2 > etspi->bufsiz) {
		status = -ENOMEM;
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}

	memset(etspi->buf, 0, xfer.len);
	*etspi->buf = OP_REG_R_C_BW;
	if (copy_from_user(etspi->buf + 1,
			(const u8 __user *) (uintptr_t)ioc->tx_buf, 1)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		goto end;
	}
	pr_debug("%s tx_buf = %p op = %x reg = %x, len = %d\n", __func__,
			ioc->tx_buf, *etspi->buf, *(etspi->buf + 1), xfer.len);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status < 0) {
		status = -ENOMEM;
		pr_err("%s error status = %d\n", __func__, status);
		goto end;
	}

	if (copy_to_user((u8 __user *) (uintptr_t)ioc->rx_buf, etspi->buf + 2,
			ioc->len)) {
		status = -EFAULT;
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		goto end;
	}
end:
	return status;
#endif
}

int etspi_io_read_registerex(struct etspi_data *etspi, u8 *addr, u8 *buf,
		u32 len)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	struct spi_message m;
	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.rx_buf = etspi->buf,
		.len = len + 2,
	};

	if (len <= 0 || len + 2 > etspi->bufsiz) {
		status = -ENOMEM;
		pr_err("%s error status = %d", __func__, status);
		goto end;
	}

	memset(etspi->buf, 0, xfer.len);
	*etspi->buf = OP_REG_R;

	if (copy_from_user(etspi->buf + 1, (const u8 __user *) (uintptr_t) addr
			, 1)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		goto end;
	}

	pr_debug("%s addr = %p op = %x reg = %x len = %d tx = %p, rx = %p",
			__func__, addr, etspi->buf[0], etspi->buf[1], len,
			xfer.tx_buf, xfer.rx_buf);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s read data error status = %d\n", __func__, status);
		goto end;
	}


	if (copy_to_user((u8 __user *) (uintptr_t) buf, etspi->buf + 2, len)) {
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		status = -EFAULT;
		goto end;
	}
end:
	return status;
#endif
}

/* Read io register */
int etspi_io_read_register(struct etspi_data *etspi, u8 *addr, u8 *buf)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	struct spi_message m;
	int read_len = 1;

	u8 val, addrval;

	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.rx_buf = etspi->buf,
		.len = 3,
	};

	memset(etspi->buf, 0, xfer.len);
	*etspi->buf = OP_REG_R;

	if (copy_from_user(&addrval, (const u8 __user *) (uintptr_t) addr
		, read_len)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		return status;
	}

	*(etspi->buf + 1) = addrval;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s read data error status = %d\n", __func__, status);
		return status;
	}

	val = *(etspi->buf + 2);

	pr_debug("%s len = %d addr = %x val = %x\n", __func__,
			read_len, addrval, val);

	if (copy_to_user((u8 __user *) (uintptr_t) buf, &val, read_len)) {
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		status = -EFAULT;
		return status;
	}

	return status;
#endif
}

/* Write data to register */
int etspi_io_write_register(struct etspi_data *etspi, u8 *buf)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status = 0;
	int write_len = 2;
	struct spi_message m;

	u8 val[3];

	struct spi_transfer xfer = {
		.tx_buf = etspi->buf,
		.len = 3,
	};

	memset(etspi->buf, 0, xfer.len);
	*etspi->buf = OP_REG_W;

	if (copy_from_user(val, (const u8 __user *) (uintptr_t) buf,
			write_len)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		return status;
	}

	pr_debug("%s write_len = %d addr = %x data = %x\n", __func__,
			write_len, val[0], val[1]);

	*(etspi->buf + 1) = val[0];
	*(etspi->buf + 2) = val[1];

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s read data error status = %d\n", __func__, status);
		return status;
	}

	return status;
#endif
}

int etspi_write_register(struct etspi_data *etspi, u8 addr, u8 buf)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	int i, buf_size;
	struct spi_message m;

	u8 tx[] = {OP_REG_W, addr, buf};
	u8 *tx_buffer = NULL;

	struct spi_transfer xfer = {
		.rx_buf	= NULL,
		.len = 3,
	};

	buf_size = 3;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_size; i++)
		tx_buffer[i] = tx[i];
	xfer.tx_buf = tx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0) {
		DEBUG_PRINT("%s address = %x\n",__func__, addr);
	} else {
		pr_err("%s read data error status = %d\n", __func__, status);
	}

	kfree(tx_buffer);
	return status;
#endif
}
int etspi_read_register(struct etspi_data *etspi, u8 addr, u8 *buf)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	int i, buf_size;
	struct spi_message m;

	u8 read_value[] = {OP_REG_R, addr, 0x00};
	u8 result[] = {0xFF, 0xFF, 0xFF};

	u8 *tx_buffer = NULL;
	u8 *rx_buffer = NULL;

	struct spi_transfer xfer = {
		.len = 3,
	};

	buf_size = 3;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	rx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (rx_buffer == NULL) {
		kfree(tx_buffer);
		return -ENOMEM;
	}
	for (i = 0; i < buf_size; i++) {
		tx_buffer[i] = read_value[i];
		rx_buffer[i] = result[i];
	}

	xfer.tx_buf = tx_buffer;
	xfer.rx_buf = rx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0) {
		*buf = rx_buffer[2];
		DEBUG_PRINT("%s address = %x result = %x %x\n"
					__func__, addr, rx_buffer[1], rx_buffer[2]);
	} else {
		pr_err("%s read data error status = %d\n", __func__, status);
	}

	kfree(tx_buffer);
	kfree(rx_buffer);

	return status;
#endif
}

int etspi_io_nvm_read(struct etspi_data *etspi, struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	int i, buf_size;
	struct spi_message m;

	u8 addr; /* nvm logical address */
	u8 buf[] = {OP_NVM_RE, 0x00};
	u8 *tx_buffer = NULL;

	struct spi_transfer xfer = {
		.rx_buf	= NULL,
		.len = 2,
	};

	buf_size = 2;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_size; i++)
		tx_buffer[i] = buf[i];
	xfer.tx_buf = tx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0)
		DEBUG_PRINT("%s nvm enabled\n", __func__);
	else
		pr_err("%s nvm enable error status = %d\n", __func__, status);

	kfree(tx_buffer);

	usleep_range(10, 50);

	if (copy_from_user(&addr, (const u8 __user *) (uintptr_t) ioc->tx_buf
		, 1)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		return status;
	}

	etspi->buf[0] = OP_NVM_ON_R;

	pr_debug("%s logical addr(%x) len(%d)\n", __func__, addr, ioc->len);
	if ((addr + ioc->len) > MAX_NVM_LEN)
		return -EINVAL;

	/* transfer to nvm physical address*/
	etspi->buf[1] = ((addr % 2) ? (addr - 1) : addr) / 2;
	/* thansfer to nvm physical length */
	xfer.len = ((ioc->len % 2) ? ioc->len + 1 :
			(addr % 2 ? ioc->len + 2 : ioc->len)) + 3;
	if (xfer.len >= LARGE_SPI_TRANSFER_BUFFER) {
		if ((xfer.len) % DIVISION_OF_IMAGE != 0)
			xfer.len = xfer.len + (DIVISION_OF_IMAGE -
					(xfer.len % DIVISION_OF_IMAGE));
	}
	xfer.tx_buf = xfer.rx_buf = etspi->buf;

	pr_debug("%s nvm read addr(%d) len(%d) xfer.rx_buf(%p), etspi->buf(%p)\n",
			__func__, etspi->buf[1],
			xfer.len, xfer.rx_buf, etspi->buf);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s error status = %d\n", __func__, status);
		return status;
	}

	if (copy_to_user((u8 __user *) (uintptr_t) ioc->rx_buf, xfer.rx_buf + 3
			, ioc->len)) {
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		status = -EFAULT;
		return status;
	}

	return status;
#endif
}

int etspi_io_nvm_write(struct etspi_data *etspi, struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status, i, j, len/* physical nvm length */;
	int buf_size;
	struct spi_message m;
	u8 *bufw = NULL;
	u8 buf[MAX_NVM_LEN + 1] = {OP_NVM_WE, 0x00};
	u8 addr/* nvm physical addr */;
	u8 *tx_buffer = NULL;

	struct spi_transfer xfer = {
		.rx_buf	= NULL,
		.len = 2,
	};

	if (ioc->len > (MAX_NVM_LEN + 1))
		return -EINVAL;

	buf_size = MAX_NVM_LEN+1;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_size; i++)
		tx_buffer[i] = buf[i];

	xfer.tx_buf = tx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0)
		DEBUG_PRINT("%s nvm enabled\n", __func__);
	else
		pr_err("%s nvm enable error status = %d\n", __func__, status);

	kfree(tx_buffer);

	usleep_range(10, 50);

	pr_debug("%s buf(%p) tx_buf(%p) len(%d)\n", __func__, buf,
			ioc->tx_buf, ioc->len);
	if (copy_from_user(buf, (const u8 __user *) (uintptr_t) ioc->tx_buf,
			ioc->len)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		return status;
	}

	if ((buf[0] + (ioc->len - 1)) > MAX_NVM_LEN)
		return -EINVAL;
	if ((buf[0] % 2) || ((ioc->len - 1) % 2)) {
		/* TODO: add non alignment handling */
		pr_err("%s can't handle address alignment issue. %d %d\n",
				__func__, buf[0], ioc->len);
		return -EINVAL;
	}

	bufw = kzalloc(NVM_WRITE_LENGTH, GFP_KERNEL | GFP_DMA);
	/*TODO: need to dynamic assign nvm length*/
	if (bufw == NULL) {
		status = -ENOMEM;
		pr_err("%s bufw kmalloc error\n", __func__);
		return status;
	}
	xfer.tx_buf = xfer.rx_buf = bufw;
	xfer.len = NVM_WRITE_LENGTH;

	len = (ioc->len - 1) / 2;
	pr_debug("%s nvm write addr(%d) len(%d) xfer.tx_buf(%p), etspi->buf(%p)\n",
			__func__, buf[0], len, xfer.tx_buf, etspi->buf);
	for (i = 0, addr = buf[0] / 2/* thansfer to nvm physical length */;
			i < len; i++) {
		bufw[0] = OP_NVM_ON_W;
		bufw[1] = addr++;
		bufw[2] = buf[i * 2 + 1];
		bufw[3] = buf[i * 2 + 2];
		memset(bufw + 4, 1, NVM_WRITE_LENGTH - 4);

		pr_debug("%s write transaction (%d): %x %x %x %x\n",
				__func__, i,
				bufw[0], bufw[1], bufw[2], bufw[3]);
		spi_message_init(&m);
		spi_message_add_tail(&xfer, &m);
		status = spi_sync(etspi->spi, &m);
		if (status < 0) {
			pr_err("%s error status = %d\n", __func__, status);
			goto end;
		}
		for (j = 0; j < NVM_WRITE_LENGTH - 4; j++) {
			if (bufw[4 + j] == 0) {
				pr_debug("%s nvm write ready(%d)\n",
						__func__, j);
				break;
			}
			if (j == NVM_WRITE_LENGTH - 5) {
				pr_err("%s nvm write fail(timeout)\n",
						__func__);
				status = -EIO;
				goto end;
			}
		}
	}
end:
	kfree(bufw);
	return status;
#endif
}

int etspi_nvm_read(struct etspi_data *etspi, struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	int i, buf_size;
	struct spi_message m;

	u8 addr; /* nvm logical address */
	u8 buf[] = {OP_NVM_RE, 0x00};
	u8 *tx_buffer = NULL;

	struct spi_transfer xfer = {
		.rx_buf	= NULL,
		.len = 2,
	};

	buf_size = 2;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_size; i++)
		tx_buffer[i] = buf[i];

	xfer.tx_buf = tx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0)
		DEBUG_PRINT("%s nvm enabled\n", __func__);
	else
		pr_err("%s nvm enable error status = %d\n", __func__, status);

	kfree(tx_buffer);

	usleep_range(10, 50);

	addr = ioc->tx_buf[0];

	etspi->buf[0] = OP_NVM_ON_R;

	pr_debug("%s logical addr(%x) len(%d)\n", __func__, addr, ioc->len);
	if ((addr + ioc->len) > MAX_NVM_LEN)
		return -EINVAL;

	/* transfer to nvm physical address*/
	etspi->buf[1] = ((addr % 2) ? (addr - 1) : addr) / 2;
	/* thansfer to nvm physical length */
	xfer.len = ((ioc->len % 2) ? ioc->len + 1 :
			(addr % 2 ? ioc->len + 2 : ioc->len)) + 3;
	if (xfer.len >= LARGE_SPI_TRANSFER_BUFFER) {
		if ((xfer.len) % DIVISION_OF_IMAGE != 0)
			xfer.len = xfer.len + (DIVISION_OF_IMAGE -
					(xfer.len % DIVISION_OF_IMAGE));
	}
	xfer.tx_buf = xfer.rx_buf = etspi->buf;

	pr_debug("%s nvm read addr(%d) len(%d) xfer.rx_buf(%p), etspi->buf(%p)\n",
			__func__, etspi->buf[1],
			xfer.len, xfer.rx_buf, etspi->buf);
	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s error status = %d\n", __func__, status);
		return status;
	}

	if (memcpy((u8 __user *) (uintptr_t) ioc->rx_buf, xfer.rx_buf + 3,
			ioc->len)) {
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		status = -EFAULT;
		return status;
	}

	return status;
#endif
}
int etspi_io_nvm_writeex(struct etspi_data *etspi,
		struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status, i, j, len/* physical nvm length */, wlen;
	int buf_size;
	struct spi_message m;
	u8 *bufw = NULL;
	u8 bufr[MAX_NVM_LEN + 3];
	u8 buf[MAX_NVM_LEN + 3] = {OP_NVM_WE, 0x00};
	u8 addr/* nvm physical addr */, *tmp = NULL;
	u8 *tx_buffer = NULL;
	struct egis_ioc_transfer r;

	struct spi_transfer xfer = {
		.rx_buf	= NULL,
		.len = 2,
	};

	pr_debug("%s buf(%p) tx_buf(%p) len(%d)\n", __func__,
			buf, ioc->tx_buf, ioc->len);
	if (copy_from_user(buf, (const u8 __user *) (uintptr_t) ioc->tx_buf
		, ioc->len)) {
		pr_err("%s buffer copy_from_user fail\n", __func__);
		status = -EFAULT;
		return status;
	}

	if ((buf[0] + (ioc->len - 3)) > MAX_NVM_LEN)
		return -EINVAL;
	if ((buf[0] % 2) || ((ioc->len - 3) % 2)) {
		/* address non-alignment handling */
		pr_debug("%s handle address alignment issue. %d %d\n",
				__func__, buf[0], ioc->len);

		r.tx_buf = r.rx_buf = bufr;
		r.len = ioc->len;
		if (buf[0] % 2) {
			r.tx_buf[0] = buf[0] - 1;
			r.len = ioc->len % 2 ? r.len + 1 : r.len + 2;
		} else {
			if (ioc->len % 2)
				r.len++;
		}
		pr_debug("%s fixed address alignment issue. %d %d\n",
				__func__, r.tx_buf[0], r.len);
		etspi_nvm_read(etspi, &r);

		tmp = bufr;
		if (buf[0] % 2)
			tmp++;
		memcpy(tmp, buf, ioc->len);
	}

	buf[0] = OP_NVM_WE;

	buf_size = MAX_NVM_LEN+1;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_size; i++)
		tx_buffer[i] = buf[i];
	xfer.tx_buf = tx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0)
		DEBUG_PRINT("%s nvm enabled\n", __func__);
	else
		pr_err("%s nvm enable error status = %d\n", __func__, status);

	kfree(tx_buffer);

	usleep_range(10, 50);

	wlen = *(u16 *)(buf + 1);
	pr_debug("%s wlen(%d)\n", __func__, wlen);
	if (wlen > 8192)
		wlen = 8196;
	bufw = kzalloc(wlen, GFP_KERNEL | GFP_DMA);
	if (bufw == NULL) {
		status = -ENOMEM;
		pr_err("%s bufw kmalloc error\n", __func__);
		return status;
	}
	xfer.tx_buf = xfer.rx_buf = bufw;
	xfer.len = wlen;

	if ((buf[0] % 2) || ((ioc->len - 3) % 2)) {
		memcpy(buf, bufr, r.len);
		ioc->len = r.len;
	}
	len = (ioc->len - 3) / 2;
	pr_debug("%s nvm write addr(%d) len(%d) xfer.tx_buf(%p), etspi->buf(%p), wlen(%d)\n",
			 __func__, buf[0], len, xfer.tx_buf, etspi->buf, wlen);
	for (i = 0, addr = buf[0] / 2/* thansfer to nvm physical length */;
			i < len; i++) {
		bufw[0] = OP_NVM_ON_W;
		bufw[1] = addr++;
		bufw[2] = buf[i * 2 + 3];
		bufw[3] = buf[i * 2 + 4];
		memset(bufw + 4, 1, wlen - 4);

		pr_debug("%s write transaction (%d): %x %x %x %x\n",
				__func__, i,
				bufw[0], bufw[1], bufw[2], bufw[3]);
		spi_message_init(&m);
		spi_message_add_tail(&xfer, &m);
		status = spi_sync(etspi->spi, &m);
		if (status < 0) {
			pr_err("%s error status = %d\n", __func__, status);
			goto end;
		}
		for (j = 0; j < wlen - 4; j++) {
			if (bufw[4 + j] == 0) {
				pr_debug("%s nvm write ready(%d)\n",
						__func__, j);
				break;
			}
			if (j == wlen - 5) {
				pr_err("%s nvm write fail(timeout)\n",
						__func__);
				status = -EIO;
				goto end;
			}
		}
	}
end:
	kfree(bufw);
	return status;
#endif
}

int etspi_io_nvm_off(struct etspi_data *etspi, struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	int i, buf_size;
	struct spi_message m;

	u8 buf[] = {OP_NVM_OFF, 0x00};
	u8 *tx_buffer = NULL;

	struct spi_transfer xfer = {
		.rx_buf	= NULL,
		.len = 2,
	};

	buf_size = 2;
	tx_buffer = kzalloc(buf_size*sizeof(u8), GFP_KERNEL | GFP_DMA);
	if (tx_buffer == NULL)
		return -ENOMEM;
	for (i = 0; i < buf_size; i++)
		tx_buffer[i] = buf[i];
	xfer.tx_buf = tx_buffer;

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status == 0)
		DEBUG_PRINT("%s nvm disabled\n", __func__);
	else
		pr_err("%s nvm disable error status = %d\n", __func__, status);

	kfree(tx_buffer);
	return status;
#endif
}
int etspi_io_vdm_read(struct etspi_data *etspi, struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	struct spi_message m;
	u8 *buf = NULL;

	struct spi_transfer xfer = {
		.tx_buf = NULL,
		.rx_buf = NULL,
		.len = ioc->len + 1,
	};

	if (xfer.len >= LARGE_SPI_TRANSFER_BUFFER) {
		if ((xfer.len) % DIVISION_OF_IMAGE != 0)
			xfer.len = xfer.len + (DIVISION_OF_IMAGE -
					(xfer.len % DIVISION_OF_IMAGE));
	}

	buf = kzalloc(xfer.len, GFP_KERNEL | GFP_DMA);

	if (buf == NULL)
		return -ENOMEM;

	xfer.tx_buf = xfer.rx_buf = buf;
	buf[0] = OP_VDM_R;

	pr_debug("%s len = %d, xfer.len = %d, buf = %p, rx_buf = %p\n",
			__func__, ioc->len, xfer.len, buf, ioc->rx_buf);

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s read data error status = %d\n", __func__, status);
		goto end;
	}

	if (copy_to_user((u8 __user *) (uintptr_t) ioc->rx_buf, buf + 1,
			ioc->len)) {
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		status = -EFAULT;
	}
end:
	kfree(buf);
	return status;
#endif
}

int etspi_io_vdm_write(struct etspi_data *etspi, struct egis_ioc_transfer *ioc)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	struct spi_message m;
	u8 *buf = NULL;

	struct spi_transfer xfer = {
		.tx_buf = NULL,
		.rx_buf = NULL,
		.len = ioc->len + 1,
	};

	if (xfer.len >= LARGE_SPI_TRANSFER_BUFFER) {
		if ((xfer.len) % DIVISION_OF_IMAGE != 0)
			xfer.len = xfer.len + (DIVISION_OF_IMAGE -
					(xfer.len % DIVISION_OF_IMAGE));
	}

	buf = kzalloc(xfer.len, GFP_KERNEL | GFP_DMA);
	if (buf == NULL)
		return -ENOMEM;

	if (copy_from_user((u8 __user *) (uintptr_t) buf + 1, ioc->tx_buf,
			ioc->len)) {
		pr_err("buffer copy_from_user fail status\n");
		status = -EFAULT;
		goto end;
	}

	xfer.tx_buf = xfer.rx_buf = buf;
	buf[0] = OP_VDM_W;

	pr_debug("%s len = %d, xfer.len = %d, buf = %p, tx_buf = %p\n",
			 __func__, ioc->len, xfer.len, buf, ioc->tx_buf);

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);

	if (status < 0)
		pr_err("%s read data error status = %d\n", __func__, status);
end:
	kfree(buf);
	return status;
#endif
}

int etspi_io_get_frame(struct etspi_data *etspi, u8 *fr, u32 size)
{
#ifdef ENABLE_SENSORS_FPRINT_SECURE
	return 0;
#else
	int status;
	struct spi_message m;
	u8 *buf = NULL;

	struct spi_transfer xfer = {
		.tx_buf = NULL,
		.rx_buf = NULL,
		.len = size + 1,
	};

	if (xfer.len >= LARGE_SPI_TRANSFER_BUFFER) {
		if ((xfer.len) % DIVISION_OF_IMAGE != 0)
			xfer.len = xfer.len + (DIVISION_OF_IMAGE -
					(xfer.len % DIVISION_OF_IMAGE));
	}

	buf = kzalloc(xfer.len, GFP_KERNEL | GFP_DMA);

	if (buf == NULL)
		return -ENOMEM;

	xfer.tx_buf = xfer.rx_buf = buf;
	buf[0] = OP_IMG_R;

	pr_debug("%s size = %d, xfer.len = %d, buf = %p, fr = %p\n", __func__,
		size, xfer.len, buf, fr);

	spi_message_init(&m);
	spi_message_add_tail(&xfer, &m);
	status = spi_sync(etspi->spi, &m);
	if (status < 0) {
		pr_err("%s read data error status = %d\n", __func__, status);
		goto end;
	}

	if (copy_to_user((u8 __user *) (uintptr_t) fr, buf + 1, size)) {
		pr_err("%s buffer copy_to_user fail status\n", __func__);
		status = -EFAULT;
	}
end:
	kfree(buf);
	return status;
#endif
}
