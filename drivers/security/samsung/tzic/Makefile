#
# TZIC Driver
#

ccflags-$(CONFIG_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
obj-$(CONFIG_TZIC) += tzic64.o

EXTRA_CFLAGS += -I$(src)
obj-$(CONFIG_TZIC_USE_QSEECOM) += tzic_qsee.o
obj-$(CONFIG_TZIC_USE_TZDEV) += tzic_tzdev.o
ifneq ($(CONFIG_TZIC_EXYNOS7885),)
ccflags-$(CONFIG_TZIC_USE_TZDEV) += -Idrivers/misc/tzdev/3.0
else
ccflags-$(CONFIG_TZIC_USE_TZDEV) += -Idrivers/misc/tzdev
endif
ifeq ($(MTK_PLATFORM),mt6768)
    obj-y += tzic_mtk_atf.o
else
    obj-$(CONFIG_TZIC_USE_TRUSTONIC) += tzic_kinibi.o
endif