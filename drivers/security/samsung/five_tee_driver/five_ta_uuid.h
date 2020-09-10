/*
 * TEE UUID
 *
 * Copyright (C) 2017 Samsung Electronics, Inc.
 * Egor Uleyskiy, <e.uleyskiy@samsung.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __FIVE_TEE_UUID_H__
#define __FIVE_TEE_UUID_H__

#if defined(CONFIG_FIVE_USE_TZDEV)
static const TEEC_UUID five_ta_uuid = {
	.timeLow = 0x00000000,
	.timeMid = 0x0000,
	.timeHiAndVersion = 0x0000,
	.clockSeqAndNode = {0x00, 0x00, 0x00, 0x00, 0x46, 0x49, 0x56, 0x45},
};
#elif defined(CONFIG_FIVE_USE_QSEE)
#ifdef CONFIG_ARCH_MSM8917
static const TEEC_UUID five_ta_uuid = {
	.timeLow = 0x6d736d38,
	.timeMid = 0x3931,
	.timeHiAndVersion = 0x372f,
	.clockSeqAndNode = {0x66, 0x69, 0x76, 0x65, 0x00, 0x00, 0x00, 0x00},
};
#else
static const TEEC_UUID five_ta_uuid = {
	.timeLow = 0x66697665,
	.timeMid = 0x0000,
	.timeHiAndVersion = 0x0000,
	.clockSeqAndNode = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};
#endif // CONFIG_ARCH_MSM8917
#elif defined(CONFIG_FIVE_USE_TRUSTONIC)
static const TEEC_UUID five_ta_uuid = {
	.timeLow = 0xffffffff,
	.timeMid = 0x0000,
	.timeHiAndVersion = 0x0000,
	.clockSeqAndNode = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x72},
};
#elif defined(CONFIG_FIVE_USE_EMULATOR)
static const TEEC_UUID five_ta_uuid = {
	.timeLow = 0x00000000,
	.timeMid = 0x0000,
	.timeHiAndVersion = 0x0000,
	.clockSeqAndNode = {0x00, 0x00, 0x00, 0x00, 0x46, 0x49, 0x56, 0xff},
};
#endif

#endif /* __FIVE_TEE_UUID_H__ */
