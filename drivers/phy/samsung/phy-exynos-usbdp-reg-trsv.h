/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Author: Sung-Hyun Na <sunghyun.na@samsung.com>
 *
 * Chip Abstraction Layer for USB PHY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef DRIVER_USB_USBPHY_CAL_PHY_EXYNOS_USBDP_REG_TRSV_H_
#define DRIVER_USB_USBPHY_CAL_PHY_EXYNOS_USBDP_REG_TRSV_H_

#define EXYNOS_USBDP_COM_TRSV_R27	(0x015C)
#define USBDP_TRSV27_MAN_EN_CTRL		USBDP_COMBO_REG_MSK(7, 1)
#define USBDP_TRSV27_MAN_TX_SER_RSTN		USBDP_COMBO_REG_MSK(6, 1)
#define USBDP_TRSV27_MAN_DESKEW_RSTN		USBDP_COMBO_REG_MSK(5, 1)
#define USBDP_TRSV27_MAN_CDR_AFC_RSTN		USBDP_COMBO_REG_MSK(4, 1)
#define USBDP_TRSV27_MAN_CDR_AFC_INIT_RSTN	USBDP_COMBO_REG_MSK(3, 1)
#define USBDP_TRSV27_MAN_VALID_RSTN		USBDP_COMBO_REG_MSK(2, 1)
#define USBDP_TRSV27_MAN_CDR_DES_RSTN		USBDP_COMBO_REG_MSK(1, 1)
#define USBDP_TRSV27_MAN_RSTN_EN		USBDP_COMBO_REG_MSK(0, 1)

#define EXYNOS_USBDP_COM_TRSV_R59	(0x0224)
#define USBDP_TRSV59_TX_JEQ_EN				USBDP_COMBO_REG_MSK(7, 1)
#define USBDP_TRSV59_TX_DRV_PLL_REF_MON_EN		USBDP_COMBO_REG_MSK(6, 1)
#define USBDP_TRSV59_TX_DRV_IDRV_IUP_CTRL_MASK		USBDP_COMBO_REG_MSK(3, 3)
#define USBDP_TRSV59_TX_DRV_IDRV_IUP_CTRL_SET(_x)	USBDP_COMBO_REG_SET(_x, 3, 3)
#define USBDP_TRSV59_TX_DRV_IDRV_IUP_CTRL_GET(_R)	USBDP_COMBO_REG_GET(_R, 3, 3)
#define USBDP_TRSV59_TX_DRV_IDRV_EN			USBDP_COMBO_REG_MSK(2, 1)
#define USBDP_TRSV59_TX_DRV_ACCDRV_EN			USBDP_COMBO_REG_MSK(1, 1)

#define EXYNOS_USBDP_COM_TRSV_R5A	(0x0228)
#define USBDP_TRSV5A_TX_DRV_ACCDRV_CTRL_MASK		USBDP_COMBO_REG_MSK(5, 3)
#define USBDP_TRSV5A_TX_DRV_ACCDRV_CTRL_SET(_x)		USBDP_COMBO_REG_SET(_x, 5, 3)
#define USBDP_TRSV5A_TX_DRV_ACCDRV_CTRL_GET(_R)		USBDP_COMBO_REG_GET(_R, 5, 3)

#endif /* DRIVER_USB_USBPHY_CAL_PHY_EXYNOS_USBDP_REG_TRSV_H_ */
