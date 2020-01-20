#
# Makefile for the CMUCAL drivers.
#

# Object files in subdirectories

obj-y					+= cal-if.o
obj-$(CONFIG_PMUCAL)			+= pmucal_system.o pmucal_local.o pmucal_cpu.o pmucal_rae.o
obj-$(CONFIG_PMUCAL_DBG)		+= pmucal_dbg.o
obj-$(CONFIG_FLEXPMU)			+= pmucal_powermode.o
obj-$(CONFIG_CP_PMUCAL)		+= pmucal_cp.o
obj-$(CONFIG_CMUCAL)			+= cmucal.o ra.o vclk.o pll_spec.o
obj-$(CONFIG_CMUCAL_DEBUG)		+= cmucal-debug.o
obj-$(CONFIG_ACPM_DVFS)			+= acpm_dvfs.o fvmap.o
obj-$(CONFIG_CMU_EWF)			+= cmu_ewf.o

ifndef CONFIG_SOC_EMULATOR8895
obj-$(CONFIG_SOC_EXYNOS8895)		+= exynos8895/cal_data.o
else
obj-$(CONFIG_SOC_EMULATOR8895)		+= emulator8895/cal_data.o
endif

obj-$(CONFIG_SOC_EXYNOS9810)            += exynos9810/cal_data.o

ifndef CONFIG_SOC_EXYNOS9820_EVT0
obj-$(CONFIG_SOC_EXYNOS9820)            += exynos9820/cal_data.o
else
obj-$(CONFIG_SOC_EXYNOS9820_EVT0)            += exynos9820_evt0/cal_data.o
endif
