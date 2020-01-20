/*
 * ALSA SoC - Samsung Mailbox driver
 *
 * Copyright (c) 2016 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VTS_MAILBOX_H
#define __VTS_MAILBOX_H

/**
 * Mailbox interrupt generator
 * @param[in]	pdev	pointer to struct platform_device of target mailbox device
 * @param[in]	hw_irq	hardware irq number which is same with bit index of the irq
 * @return		error code if any
 */
extern int mailbox_generate_interrupt(const struct platform_device *pdev, int hw_irq);

/**
 * Mailbox shared register writer
 * @param[in]	pdev 	pointer to struct platform_device of target mailbox device
 * @param[in]	values	values to write
 * @param[in]	start	start position to write. if 0, values are written from ISSR0
 * @param[in]	count	count of value in values.
 */
extern void mailbox_write_shared_register(const struct platform_device *pdev,
		const u32 *values, int start, int count);

/**
 * Mailbox shared register reader
 * @param[in]	pdev	pointer to struct platform_device of target mailbox device
 * @param[out]	values	memory to write
 * @param[in]	start	start position to read. if 1, values are read from ISSR1
 * @param[in]	count	count of value in values.
 */
extern void mailbox_read_shared_register(const struct platform_device *pdev,
		u32 *values, int start, int count);

#endif /* __VTS_MAILBOX_H */