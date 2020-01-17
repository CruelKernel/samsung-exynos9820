#
# Copyright (c) 2013 Samsung Electronics Co., Ltd.
#		http://www.samsung.com
#
# Licensed under GPLv2
#

obj-$(CONFIG_EXYNOS_DPP) += dpp.o
dpp-y := dpp_drv.o
obj-$(CONFIG_EXYNOS_MIPI_DSIM) += dsim.o
dsim-y += dsim_drv.o
obj-$(CONFIG_SOC_EXYNOS9810) += ./cal_9810/dsim_reg.o ./cal_9810/dpp_reg.o ./cal_9810/decon_reg.o
obj-$(CONFIG_SOC_EXYNOS9820) += ./cal_9820/dsim_reg.o ./cal_9820/dpp_reg.o ./cal_9820/decon_reg.o
obj-$(CONFIG_SOC_EXYNOS9110) += ./cal_9110/dsim_reg.o ./cal_9110/dpp_reg.o ./cal_9110/decon_reg.o
ifdef CONFIG_EXYNOS_DISPLAYPORT
obj-$(CONFIG_EXYNOS_DISPLAYPORT) += displayport.o
displayport-y += displayport_drv.o displayport_edid.o displayport_hdcp13.o decon_displayport.o displayport_hdcp22_if.o secdp_unit_test.o secdp_aux_control.o

obj-$(CONFIG_SOC_EXYNOS9810) += ./cal_9810/displayport_reg.o
obj-$(CONFIG_SOC_EXYNOS9820) += ./cal_9820/displayport_reg.o
endif
obj-$(CONFIG_EXYNOS_DPU20) += decon.o
obj-$(CONFIG_EXYNOS_BTS) += bts.o
decon-y += decon_core.o decon_dsi.o decon_wb.o helper.o win_update.o cursor.o fence.o event_log.o
obj-y += panels/
obj-y += dp_logger/
obj-y += mcd_hdr/
ccflags-$(CONFIG_SAMSUNG_TUI)	+= -Idrivers/misc/tui