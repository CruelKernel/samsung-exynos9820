#
# Makefile for the DSMS
#

obj-y := dsms_kernel_api.o
obj-y += dsms_access_control.o
obj-y += dsms_policy.o
obj-y += dsms_init.o

EXTRA_CFLAGS += -DDSMS_WHITELIST_IGNORE_NAME_SUFFIXES_ENABLE

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))

	EXTRA_CFLAGS += -DDSMS_DEBUG_ENABLE
	obj-y += dsms_debug.o

	EXTRA_CFLAGS += -DDSMS_DEBUG_TRACE_DSMS_CALLS
	# EXTRA_CFLAGS += -DDSMS_DEBUG_WHITELIST

        EXTRA_CFLAGS += -DDSMS_KERNEL_ENG

endif
