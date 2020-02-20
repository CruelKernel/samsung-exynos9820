#
# Integrity TEE Driver
#

ccflags-$(CONFIG_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include

obj-$(CONFIG_ICD) += icd.o oemflag.o
ccflags-$(CONFIG_ICD) += -Isecurity/samsung/five

EXTRA_CFLAGS += -I$(src)
obj-$(CONFIG_ICD_USE_QSEECOM) += qsee_oemflag.o
obj-$(CONFIG_ICD_USE_TZDEV) += tzdev_oemflag.o
ifneq ($(CONFIG_SOC_EXYNOS7885),)
ccflags-$(CONFIG_ICD_USE_TZDEV) += -Idrivers/misc/tzdev/3.0
else
ccflags-$(CONFIG_ICD_USE_TZDEV) += -Idrivers/misc/tzdev
endif
obj-$(CONFIG_ICD_USE_TRUSTONIC) += kinibi_oemflag.o
