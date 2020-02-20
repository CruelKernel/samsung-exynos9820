#
# Integrity TEE Driver
#

ccflags-$(CONFIG_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include

obj-$(CONFIG_ICD) += icd.o oemflag.o
ccflags-$(CONFIG_ICD) += -Isecurity/integrity/five

obj-$(CONFIG_ICD_USE_QSEECOM) += qsee_oemflag.o
obj-$(CONFIG_ICD_USE_TZDEV) += tzdev_oemflag.o
obj-$(CONFIG_ICD_USE_TRUSTONIC) += kinibi_oemflag.o
