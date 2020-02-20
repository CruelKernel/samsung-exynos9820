#
# Makefile for the DSMS
#

obj-y := dsms_kernel_api.o
obj-y += dsms_access_control.o
obj-y += dsms_rate_limit.o
obj-y += dsms_policy.o
obj-y += dsms_init.o
obj-y += dsms_debug.o

EXTRA_CFLAGS += -DDSMS_WHITELIST_IGNORE_NAME_SUFFIXES_ENABLE

# Debugging options:

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
#    CFLAGS_dsms_debug.o := -DDEBUG
endif
