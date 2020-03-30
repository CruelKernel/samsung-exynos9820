
ccflags-y := $(KBUILD_FP_SENSOR_CFLAGS)

obj-$(CONFIG_EXYNOS_PMU)	+= exynos-pmu.o
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-el3_mon.o
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-smc.o

obj-$(CONFIG_EXYNOS_PMU_ARM_DRIVERS)	+= exynos3250-pmu.o exynos4-pmu.o \
					exynos5250-pmu.o exynos5420-pmu.o
obj-$(CONFIG_EXYNOS_PM_DOMAINS) += pm_domains.o
obj-$(CONFIG_EXYNOS_CHIPID)	+= exynos-chipid.o

# CAL
obj-$(CONFIG_CAL_IF)            += cal-if/
obj-$(CONFIG_EXYNOS_CHIPID)     += exynos-chipid.o
obj-$(CONFIG_EXYNOS_PD)        += exynos-pd.o exynos-pd-dbg.o
obj-$(CONFIG_EXYNOS_RGT)        += exynos-rgt.o
obj-$(CONFIG_EXYNOS_FLEXPMU_DBG)        += exynos-flexpmu-dbg.o

# ACPM
obj-$(CONFIG_EXYNOS_ACPM)       += acpm/

# ECT
obj-$(CONFIG_ECT)               += ect_parser.o
obj-$(CONFIG_EXYNOS_BCM_DBG)    += exynos-bcm_dbg.o exynos-bcm_dbg-dt.o
obj-$(CONFIG_EXYNOS_BCM_DBG_DUMP)       += exynos-bcm_dbg-dump.o

# SCI
obj-$(CONFIG_EXYNOS_SCI)	+= exynos-sci.o

# USI_V2
obj-$(CONFIG_USI_V2) += usi_v2.o

# reboot
obj-$(CONFIG_EXYNOS_REBOOT)	+= exynos-reboot.o

# DVFS
obj-$(CONFIG_EXYNOS_DVFS_MANAGER)	+= exynos-dm.o
obj-$(CONFIG_EXYNOS_MCINFO)	+= exynos-mcinfo.o

obj-$(CONFIG_EXYNOS_PM_DVS)        += exynos-pm-dvs.o

#PM
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-powermode.o
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-pm.o

#CPUHOTPLUG
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-cpuhp.o
obj-$(CONFIG_EXYNOS_PSTATE_MODE_CHANGER)	+= exynos-emc.o

# CPU Topology
obj-$(CONFIG_ARCH_EXYNOS)	+= exynos-topology.o

# OCP
obj-$(CONFIG_EXYNOS_OCP)	+= exynos-ocp.o

# CP
ifneq ($(CONFIG_CP_PMUCAL), y)
obj-$(CONFIG_SEC_SIPC_MODEM_IF) += pmu-cp.o
endif

#CPU Power Management
obj-$(CONFIG_ARCH_EXYNOS)      += exynos-cpupm.o

# Exynos Secure Log
obj-$(CONFIG_EXYNOS_SECURE_LOG)	+= exynos-seclog.o

# secmem
obj-$(CONFIG_ARCH_EXYNOS)       += secmem.o

# HDCP
obj-$(CONFIG_EXYNOS_HDCP2)  += exynos-hdcp/

# DEBUG
obj-$(CONFIG_EXYNOS_DEBUG)	+= debug/

# SDM
obj-$(CONFIG_EXYNOS_SDM) += exynos-sdm.o

# HAFM & HAFM-TB
obj-$(CONFIG_EXYNOS_PSTATE_HAFM)	+= exynos-hiu.o
obj-$(CONFIG_EXYNOS_PSTATE_HAFM_TB)	+= exynos-hiu-tb.o

#CP linear reseved memory
obj-$(CONFIG_EXYNOS_CP_RESERVE)	+= exynos-cpmem.o

# Exynos PPMPU fail detector
obj-$(CONFIG_EXYNOS_PPMPU_FAIL_DETECTOR)	+= exynos-ppmpu.o

# Exynos CPU Performance
obj-$(CONFIG_EXYNOS_CPU_PERF)	+= exynos_cpu_perf/
