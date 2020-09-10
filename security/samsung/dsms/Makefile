#
# Makefile for the DSMS
#

obj-y := dsms_kernel_api.o
obj-y += dsms_access_control.o
obj-y += dsms_rate_limit.o
obj-y += dsms_policy.o
obj-y += dsms_init.o
obj-y += dsms_debug.o

ccflags-y := -Wformat

EXTRA_CFLAGS += -DDSMS_WHITELIST_IGNORE_NAME_SUFFIXES_ENABLE

# Debugging options:

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
#    CFLAGS_dsms_debug.o := -DDEBUG
endif

ifneq ($(CONFIG_KUNIT), y)
    EXTRA_CFLAGS += -D__visible_for_testing=static
endif
