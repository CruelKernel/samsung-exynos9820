#
# Makefile for Exynos-Debug
#

obj-$(CONFIG_EXYNOS_CORESIGHT) 		+= exynos-coresight.o
ifeq ($(CONFIG_SOC_EXYNOS9810),y)
obj-$(CONFIG_EXYNOS_ITMON) 		+= exynos9810-itmon.o
endif
ifeq ($(CONFIG_SOC_EXYNOS9820),y)
obj-$(CONFIG_EXYNOS_ITMON) 		+= exynos9820-itmon.o
endif
obj-$(CONFIG_ARCH_EXYNOS)		+= exynos-handler.o
ifeq ($(CONFIG_DEBUG_SNAPSHOT),y)
obj-$(CONFIG_ARCH_EXYNOS)		+= exynos-helper.o
endif
obj-$(CONFIG_EXYNOS_ADV_TRACER) 	+= exynos-adv-tracer.o exynos-adv-tracer-ipc.o
obj-$(CONFIG_EXYNOS_ADV_TRACER_S2D)	+= exynos-adv-tracer-s2d.o
obj-$(CONFIG_EXYNOS_ADV_TRACER_MISC)	+= exynos-adv-tracer-misc.o
obj-$(CONFIG_EXYNOS_CORESIGHT_ETM)	+= exynos-coresight-etm.o
obj-$(CONFIG_EXYNOS_DEBUG_TEST)		+= exynos-debug-test.o
obj-$(CONFIG_EXYNOS_SAVELOGS)		+= exynos-savelogs.o
obj-$(CONFIG_EXYNOS_EHLD) 		+= exynos-ehld.o
