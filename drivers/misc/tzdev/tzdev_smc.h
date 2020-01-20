/*
 * Copyright (C) 2015 Samsung Electronics, Inc.
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

#ifndef __TZDEV_SMC_H__
#define __TZDEV_SMC_H__

/* Fast SMC call mask. Bit 31 of SMC Function Identifier (SFI).
 * SFI[31] = 1: SMC handler must not be pre-empted
 * SFI[31] = 0: SMC handler can be preempted
 */
#define SMC_TYPE_STD			0
#define SMC_TYPE_FAST			1

/* SMC call MBZ mask. The bits must be zero just in case of
 * fastcall, bit 31 of SFI is 1.
 */
#define SMC_FAST_CALL_MBZ		0x00FF0000

/* AARCH64 SMC call mask. Bit 30 of SMC Function Identifier (SFI).
 * SFI[30] = 1: SMC calling convention SMC64 is used
 * SFI[30] = 0: SMC calling convention SMC32 is used
 */
#define SMC_AARCH_32			0
#define SMC_AARCH_64			1

/* SMC call types.
 *  -------------------------------------------------------------
 * | 0     | 0x00000000              | ARM Architecture Calls   |
 * --------------------------------------------------------------
 * | 1     | 0x01000000              | CPU Service Calls        |
 * --------------------------------------------------------------
 * | 2     | 0x02000000              | SIP Service Calls        |
 * --------------------------------------------------------------
 * | 3     | 0x03000000              | OEM Service Calls        |
 * --------------------------------------------------------------
 * | 4     | 0x04000000              | Standard Service Calls   |
 * --------------------------------------------------------------
 * | 5-47  | 0x05000000 – 0x2F000000 | Reserved for future use  |
 * --------------------------------------------------------------
 * | 48-49 | 0x30000000 – 0x31000000 | Trusted Application Calls|
 * --------------------------------------------------------------
 * | 50-63 | 0x32000000 – 0x3F000000 | Trusted OS Calls         |
 * --------------------------------------------------------------
 */
#define SMC_CPU_SERVICE_MASK		0x01000000
#define SMC_SIP_SERVICE_MASK		0x02000000
#define SMC_OEM_SERVICE_MASK		0x03000000
#define SMC_STANDARD_MASK_CALL		0x04000000

#define SMC_RESERVED_RANGE_START	0x05000000
#define SMC_RESERVED_RANGE_END		0x2F000000

#define SMC_TAPP0_SERVICE_MASK		0x30000000
#define SMC_TAPP1_SERVICE_MASK		0x31000000

#define SMC_TOS0_SERVICE_MASK		0x32000000
#define SMC_TOS1_SERVICE_MASK		0x33000000
#define SMC_TOS2_SERVICE_MASK		0x34000000
#define SMC_TOS3_SERVICE_MASK		0x35000000
#define SMC_TOS4_SERVICE_MASK		0x36000000
#define SMC_TOS5_SERVICE_MASK		0x37000000
#define SMC_TOS6_SERVICE_MASK		0x38000000
#define SMC_TOS7_SERVICE_MASK		0x39000000
#define SMC_TOS8_SERVICE_MASK		0x3A000000
#define SMC_TOS9_SERVICE_MASK		0x3B000000
#define SMC_TOS10_SERVICE_MASK		0x3C000000
#define SMC_TOS11_SERVICE_MASK		0x3D000000
#define SMC_TOS12_SERVICE_MASK		0x3E000000
#define SMC_TOS13_SERVICE_MASK		0x3F000000

/* SMC call function ID mask. */
#define SMC_FUNC_ID			0x0000FFFF

/* SMC helper to create SMC argument
 * compatible to ARM SMC calling convention.
 * usage: CREATE_SMC_CMD(SMC_STD_CALL, SMC_AARCH_64, SMC_TOS0_SECVICE_MASK, fid)
 */
#define CREATE_SMC_CMD(type, arch, range, fid) \
	(((unsigned int)(type) << 31)|((arch) << 30) | (range)|(fid))

#endif /*__TZDEV_SMC_H__*/
