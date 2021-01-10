#
# Integrity TEE Driver
#

ccflags-$(CONFIG_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include

obj-$(CONFIG_ICD) += icd.o oemflag.o
ccflags-$(CONFIG_ICD) += -I$(srctree)/security/samsung/five

EXTRA_CFLAGS += -I$(src)
obj-$(CONFIG_ICD_USE_QSEECOM) += qsee_oemflag.o
obj-$(CONFIG_ICD_USE_TZDEV) += tzdev_oemflag.o
ifneq ($(CONFIG_SOC_EXYNOS7885),)
  ifeq ($(CONFIG_TEEGRIS_VERSION), 3)
    ccflags-$(CONFIG_ICD_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev/3.0
  else
    ccflags-$(CONFIG_ICD_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev
  endif
else
ccflags-$(CONFIG_ICD_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev
endif

ifneq (,$(filter $(CONFIG_MTK_PLATFORM), "mt6768"))
	obj-y += kinibi_atf_oemflag.o
else
	obj-$(CONFIG_ICD_USE_TRUSTONIC) += kinibi_oemflag.o
endif

ccflags-y += -Wformat

ifneq ($(CONFIG_KUNIT), y)
	EXTRA_CFLAGS += -D__visible_for_testing=static
endif

GCOV_PROFILE := $(CONFIG_KUNIT)
ifeq ($(CONFIG_KUNIT), y)
obj-y += test/
endif