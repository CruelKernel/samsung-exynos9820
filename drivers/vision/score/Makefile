#
# Makefile for the SCore common driver
#

ccflags-$(CONFIG_EXYNOS_SCORE_FPGA)	+= -DCONFIG_EXYNOS_SCORE_FPGA
ccflags-$(CONFIG_EXYNOS_SCORE_V2)	+= -DCONFIG_EXYNOS_SCORE_V2
ccflags-$(CONFIG_EXYNOS_SCORE_V3)	+= -DCONFIG_EXYNOS_SCORE_V3

ccflags-$(CONFIG_EXYNOS_SCORE_FPGA)	+= -DSCORE_EVT1

ifeq ($(CONFIG_SOC_EXYNOS9810_EVT1),y)
ccflags-$(CONFIG_SOC_EXYNOS9810)	+= -DSCORE_EVT1
else
ccflags-$(CONFIG_SOC_EXYNOS9810)	+= -DSCORE_EVT0
endif

ifeq ($(CONFIG_SOC_EXYNOS9820_EVT0),y)
ccflags-$(CONFIG_SOC_EXYNOS9820)	+= -DSCORE_EVT0
else
ccflags-$(CONFIG_SOC_EXYNOS9820)	+= -DSCORE_EVT1
endif

ifeq ($(CONFIG_EXYNOS_SCORE_FPGA),y)
ccflags-y				+= -I${src}/
ccflags-$(CONFIG_EXYNOS_SCORE_V2)	+= -I${src}/hardware/v2
ccflags-$(CONFIG_EXYNOS_SCORE_V2)	+= -I${src}/hardware/v2/v2_0
ccflags-$(CONFIG_EXYNOS_SCORE_V3)	+= -I${src}/hardware/v3
ccflags-$(CONFIG_EXYNOS_SCORE_V3)	+= -I${src}/hardware/v3/v3_0
else
ccflags-$(CONFIG_EXYNOS_SCORE)		+= -Idrivers/vision/score
ccflags-$(CONFIG_EXYNOS_SCORE_V2)	+= -Idrivers/vision/score/hardware/v2
ccflags-$(CONFIG_EXYNOS_SCORE_V2)	+= -Idrivers/vision/score/hardware/v2/v2_1
ccflags-$(CONFIG_EXYNOS_SCORE_V3)	+= -Idrivers/vision/score/hardware/v3
ccflags-$(CONFIG_EXYNOS_SCORE_V3)	+= -Idrivers/vision/score/hardware/v3/v3_1
endif

score-objs				:= score_device.o
score-objs				+= score_core.o
score-objs				+= score_ioctl.o
ifeq ($(CONFIG_COMPAT),y)
score-objs				+= score_compat_ioctl.o
endif
score-objs				+= score_frame.o
score-objs				+= score_context.o
score-objs				+= score_util.o
score-objs				+= score_mmu.o

score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_system.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_interface.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_firmware.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_scq.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_sm.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_sr.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_print.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_dpmu.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_profiler.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/score_sysfs.o

ifeq ($(CONFIG_EXYNOS_SCORE_FPGA),y)
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_0/score_memory.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_0/score_clk.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_0/score_pm.o
else
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_1/score_memory.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_1/score_clk.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_1/score_pm.o
score-objs-$(CONFIG_EXYNOS_SCORE_V2)	+= hardware/v2/v2_1/score_ion.o
endif

score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_system.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_interface.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_firmware.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_scq.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_sm.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_sr.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_print.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_dpmu.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_profiler.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/score_sysfs.o

ifeq ($(CONFIG_EXYNOS_SCORE_FPGA),y)
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/v3_0/score_memory.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/v3_0/score_clk.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/v3_0/score_pm.o
else
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/v3_1/score_memory.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/v3_1/score_clk.o
score-objs-$(CONFIG_EXYNOS_SCORE_V3)	+= hardware/v3/v3_1/score_pm.o
endif

score-objs				+= $(score-objs-y)

ifeq ($(CONFIG_EXYNOS_SCORE_FPGA),y)
  obj-m += score.o
else
  obj-$(CONFIG_EXYNOS_SCORE) += score.o
endif
