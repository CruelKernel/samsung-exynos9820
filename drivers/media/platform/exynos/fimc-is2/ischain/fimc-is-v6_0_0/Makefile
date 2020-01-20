obj-$(CONFIG_FIMC_IS_V6_0_0) += setup-fimc-is.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += setup-fimc-is-sensor.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-hw-pwr.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-hw-chain.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-hw-virtual-otf.o
ifeq ($(CONFIG_PM_DEVFREQ),y)
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-hw-dvfs.o
endif

obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-3aa.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-3ac.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-3ap.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-isp.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-ixc.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-ixp.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-mexc.o

obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-dcp.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-dcxs.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-dcxc.o

obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-mcs.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-mcsp.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-vra.o
ifeq ($(CONFIG_USE_SENSOR_GROUP),y)
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-sensor.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-sensor_vc0.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-sensor_vc1.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-sensor_vc2.o
obj-$(CONFIG_FIMC_IS_V6_0_0) += fimc-is-subdev-sensor_vc3.o
endif

EXTRA_CFLAGS += -Idrivers/media/platform/exynos/fimc-is2
EXTRA_CFLAGS += -Idrivers/media/platform/exynos/fimc-is2/include
