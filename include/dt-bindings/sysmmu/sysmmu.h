/*
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Device Tree binding constants for Exynos System MMU.
 */

#ifndef _DT_BINDINGS_EXYNOS_SYSTEM_MMU_H
#define _DT_BINDINGS_EXYNOS_SYSTEM_MMU_H

/*
 * RESERVED field is used to define properties between DT and driver.
 *
 * CFG[13:12]
 * - 0x0 = Private way, ID matching
 * - 0x1 = Private way, Address matching
 * - 0x2 = Public way
 */

#define WAY_TYPE_MASK			(0x3 << 12)
#define _PRIVATE_WAY_ID			(0x0 << 12)
#define _PRIVATE_WAY_ADDR		(0x1 << 12)
#define _PUBLIC_WAY			(0x2 << 12)

#define _TARGET_NONE			(0x0 << 8)
#define _TARGET_READ			(0x1 << 8)
#define _TARGET_WRITE			(0x2 << 8)
#define _TARGET_READWRITE		(0x3 << 8)

#define _DIR_DESCENDING			(0x0 << 2)
#define _DIR_ASCENDING			(0x1 << 2)
#define _DIR_PREDICTION			(0x2 << 2)

#define _PREFETCH_ENABLE		(0x1 << 1)
#define _PREFETCH_DISABLE		(0x0 << 1)

#define _PORT_MASK(pmask)		((pmask) << 16)

/*
 * Use below definitions for TLB properties setting.
 *
 * Each definition is combination of "configuration" and "ID".
 *
 * "configuration" is consist of public/private type and BL(Burst Length).
 * It should be expressed like (WAY_TYPE_DEFINE | BL_DEFINE).
 *
 * "ID" is meaningful for private ID matching only.
 * For public way and private addr matching, use SYSMMU_NOID.
 *
 * Here is an example.
 * In device tree, it is described like below.
 *	sysmmu,tlb_property =
 *		<(SYSMMU_PRIV_ID_PREFETCH_ASCENDING_READ | SYSMMU_BL32) SYSMMU_ID(0x1)>,
 *		<(SYSMMU_PRIV_ADDR_NO_PREFETCH_READ | SYSMMU_BL16) SYSMMU_NOID>,
 *		<(SYSMMU_PUBLIC_NO_PREFETCH | SYSMMU_BL16) SYSMMU_NOID>;
 */

/* Definitions for "ID" */
#define SYSMMU_ID(id)			(0xFFFF << 16 | (id))
#define SYSMMU_ID_MASK(id,mask)		((mask) << 16 | (id))
#define SYSMMU_NOID			0

/* BL_DEFINE: Definitions for burst length "configuration". */
#define SYSMMU_BL1			(0x0 << 5)
#define SYSMMU_BL2			(0x1 << 5)
#define SYSMMU_BL4			(0x2 << 5)
#define SYSMMU_BL8			(0x3 << 5)
#define SYSMMU_BL16			(0x4 << 5)
#define SYSMMU_BL32			(0x5 << 5)

/* WAY_TYPE_DEFINE: Definitions for public way "configuration". */
#define SYSMMU_PUBLIC_NO_PREFETCH	(_PUBLIC_WAY | _PREFETCH_DISABLE)
#define SYSMMU_PUBLIC_PREFETCH_ASCENDING		\
	(_PUBLIC_WAY | _PREFETCH_ENABLE | _DIR_ASCENDING)

/* WAY_TYPE_DEFINE: Definitions for private ID matching "configuration". */
#define SYSMMU_PRIV_ID_NO_PREFETCH_READ			\
	(_PRIVATE_WAY_ID | _TARGET_READ | _PREFETCH_DISABLE)
#define SYSMMU_PRIV_ID_NO_PREFETCH_WRITE		\
	(_PRIVATE_WAY_ID | _TARGET_WRITE | _PREFETCH_DISABLE)
#define SYSMMU_PRIV_ID_PREFETCH_ASCENDING_READ		\
	(_PRIVATE_WAY_ID | _TARGET_READ | _DIR_ASCENDING | _PREFETCH_ENABLE)
#define SYSMMU_PRIV_ID_PREFETCH_ASCENDING_WRITE		\
	(_PRIVATE_WAY_ID | _TARGET_WRITE | _DIR_ASCENDING | _PREFETCH_ENABLE)
#define SYSMMU_PRIV_ID_PREFETCH_PREDICTION_READ		\
	(_PRIVATE_WAY_ID | _TARGET_READ | _DIR_PREDICTION | _PREFETCH_ENABLE)
#define SYSMMU_PRIV_ID_PREFETCH_PREDICTION_WRITE	\
	(_PRIVATE_WAY_ID | _TARGET_WRITE | _DIR_PREDICTION | _PREFETCH_ENABLE)

/* WAY_TYPE_DEFINE: Definitions for private address matching "configuration". */
#define SYSMMU_PRIV_ADDR_NO_PREFETCH_READ		\
	(_PRIVATE_WAY_ADDR | _TARGET_READ | _PREFETCH_DISABLE)
#define SYSMMU_PRIV_ADDR_NO_PREFETCH_WRITE		\
	(_PRIVATE_WAY_ADDR | _TARGET_WRITE | _PREFETCH_DISABLE)
#define SYSMMU_PRIV_ADDR_NO_PREFETCH_READWRITE		\
	(_PRIVATE_WAY_ADDR | _TARGET_READWRITE | _PREFETCH_DISABLE)
#define SYSMMU_PRIV_ADDR_PREFETCH_ASCENDING_READ	\
	(_PRIVATE_WAY_ADDR | _TARGET_READ | _DIR_ASCENDING | _PREFETCH_ENABLE)

/* PORT_TYPE_DEFINE: Definition for TLB port dedication "configuration". */
#define SYSMMU_PORT_NO_PREFETCH_READ(pmask)		\
	( _PORT_MASK(pmask) | _TARGET_READ | _PREFETCH_DISABLE)
#define SYSMMU_PORT_NO_PREFETCH_WRITE(pmask)		\
	( _PORT_MASK(pmask) | _TARGET_WRITE | _PREFETCH_DISABLE)
#define SYSMMU_PORT_NO_PREFETCH_READWRITE(pmask)	\
	( _PORT_MASK(pmask) | _TARGET_READWRITE | _PREFETCH_DISABLE)
#define SYSMMU_PORT_PREFETCH_PREDICTION_READ(pmask)	\
	( _PORT_MASK(pmask) | _TARGET_READ | _DIR_PREDICTION | _PREFETCH_ENABLE)
#define SYSMMU_PORT_PREFETCH_PREDICTION_WRITE(pmask)	\
	( _PORT_MASK(pmask) | _TARGET_WRITE | _DIR_PREDICTION | _PREFETCH_ENABLE)
#define SYSMMU_PORT_PREFETCH_PREDICTION_READWRITE(pmask)	\
	( _PORT_MASK(pmask) | _TARGET_READWRITE | _DIR_PREDICTION | _PREFETCH_ENABLE)

#endif /* _DT_BINDINGS_EXYNOS_SYSTEM_MMU_H */
