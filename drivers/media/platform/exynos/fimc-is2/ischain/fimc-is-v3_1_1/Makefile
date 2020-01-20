obj-$(CONFIG_FIMC_IS_V3_1_1) += setup-fimc-is.o
obj-$(CONFIG_FIMC_IS_V3_1_1) += setup-fimc-is-sensor.o
obj-$(CONFIG_FIMC_IS_V3_1_1) += setup-fimc-is-preprocessor.o
obj-$(CONFIG_FIMC_IS_V3_1_1) += fimc-is-hw-pwr.o
obj-$(CONFIG_FIMC_IS_V3_1_1) += fimc-is-hw-chain.o
ifeq ($(CONFIG_PM_DEVFREQ),y)
obj-$(CONFIG_FIMC_IS_V3_1_1) += fimc-is-hw-dvfs.o
endif

EXTRA_CFLAGS += -Idrivers/media/platform/exynos/fimc-is2
EXTRA_CFLAGS += -Idrivers/media/platform/exynos/fimc-is2/include
