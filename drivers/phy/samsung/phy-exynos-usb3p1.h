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

#ifndef DRIVER_USB_USBPHY_CAL_PHY_EXYNOS_USB3P1_H_
#define DRIVER_USB_USBPHY_CAL_PHY_EXYNOS_USB3P1_H_

#define HS_REWA_INTR_SRC_RET_EN		(1 << 0)
#define HS_REWA_INTR_SRC_RET_DIS	(1 << 1)
#define HS_REWA_INTR_SRC_BYPASS_DIS	(1 << 2)
#define HS_REWA_INTR_SRC_DISCON		(1 << 16)
#define HS_REWA_INTR_SRC_ERR_DEV_K	(1 << 17)
#define HS_REWA_INTR_SRC_ERR_SUS	(1 << 18)

#define HS_REWA_EN_STS_ENALBED		0
#define HS_REWA_EN_STS_DISABLED		1
#define HS_REWA_EN_STS_DISCONNECT	2
#define HS_REWA_EN_STS_NOT_SUSPEND	-1


/* initialted */
extern void phy_exynos_usb_v3p1_enable(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_disable(struct exynos_usbphy_info *info);
extern void phy_exynos_usb3p1_pcs_reset(struct exynos_usbphy_info *info);
extern void phy_exynos_usb3p1_sw_rst(struct exynos_usbphy_info *info);
extern u64 phy_exynos_usb3p1_get_logic_trace(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_enable_dp_pullup(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_disable_dp_pullup(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_link_rst(struct exynos_usbphy_info *info);
/* USB/DP PHY control */
extern void phy_exynos_usb_v3p1_pma_ready(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_g2_pma_ready(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_g2_disable(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_pma_sw_rst_release(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_g2_link_pclk_sel(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_g2_pma_sw_rst_release(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_pipe_ovrd(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_pipe_ready(struct exynos_usbphy_info *info);
/* Tune */
extern void phy_exynos_usb_v3p1_config_host_mode(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_tune_host(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_tune_dev(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_tune(struct exynos_usbphy_info *info);
extern void phy_exynos_usb_v3p1_tune_each(struct exynos_usbphy_info *info, char *para_name, int val);
extern void phy_exynos_usb_v3p1_rd_tune_each_from_reg(struct exynos_usbphy_info *info, u32, char *para_name, int *val);
extern void phy_exynos_usb_v3p1_wr_tune_reg(struct exynos_usbphy_info *info, u32 val);
extern void phy_exynos_usb_v3p1_rd_tune_reg(struct exynos_usbphy_info *info, u32 *val);
/* High Speed Remote Wake-up Advisor(HS ReWA) */
extern int phy_exynos_usb3p1_rewa_enable(struct exynos_usbphy_info *info);
extern int phy_exynos_usb3p1_rewa_disable(struct exynos_usbphy_info *info);
extern int phy_exynos_usb3p1_rewa_req_sys_valid(struct exynos_usbphy_info *info);
extern int phy_exynos_usb3p1_rewa_cancel(struct exynos_usbphy_info *info);
/* 3.0 PHY CR Port Access */
extern void phy_exynos_usb_v3p1_cal_cr_write(struct exynos_usbphy_info *info, u16 addr, u16 data);
extern u16 phy_exynos_usb_v3p1_cal_cr_read(struct exynos_usbphy_info *info, u16 addr);
/* 2.0 PHY Test I/F Access */
extern u8 phy_exynos_usb_v3p1_tif_ov_rd(struct exynos_usbphy_info *info, u8 addr);
extern u8 phy_exynos_usb_v3p1_tif_ov_wr(struct exynos_usbphy_info *info, u8 addr, u8 data);
extern u8 phy_exynos_usb_v3p1_tif_sts_rd(struct exynos_usbphy_info *info, u8 addr);
/* FS_VPLUS/VMINUS interrupt */
extern void phy_exynos_usb3p1_set_fs_vplus_vminus(struct exynos_usbphy_info *usbphy_info, u32 fsls_speed_sel, u32 fsv_out_en);
/* BC 1.2 */
extern u8 phy_exynos_usb3p1_bc_data_contact_detect(struct exynos_usbphy_info *usbphy_info);
extern enum exynos_usb_bc phy_exynos_usb3p1_bc_battery_charger_detection(struct exynos_usbphy_info *usbphy_info);

#endif /* DRIVER_USB_USBPHY_CAL_PHY_EXYNOS_USB3P1_H_ */
