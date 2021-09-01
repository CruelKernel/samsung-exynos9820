#
# TZIC Driver
#

ccflags-$(CONFIG_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))

ifeq ($(CONFIG_ARCH_LAHAINA),y)
  ifeq ($(CONFIG_QGKI), y)
    obj-m += tzic_driver.o
  endif
else
  obj-$(CONFIG_TZIC) += tzic_driver.o
endif

tzic_driver-y += tzic64.o

EXTRA_CFLAGS += -I$(src)

tzic_driver-$(CONFIG_TZIC_USE_QSEECOM) += tzic_qsee.o
tzic_driver-$(CONFIG_TZIC_USE_TZDEV) += tzic_tzdev.o
ifneq ($(CONFIG_TZIC_EXYNOS7885),)
  ccflags-$(CONFIG_TZIC_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev/3.0
else
  ccflags-$(CONFIG_TZIC_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev
endif

ifeq ($(MTK_PLATFORM),mt6768)
  tzic_driver-$(CONFIG_TZIC_USE_TRUSTONIC) += tzic_mtk_atf.o
else
  tzic_driver-$(CONFIG_TZIC_USE_TRUSTONIC) += tzic_kinibi.o
endif

tzic_driver-$(CONFIG_TZIC_DEFAULT) += tzic_default.o

ccflags-y += -Wformat
