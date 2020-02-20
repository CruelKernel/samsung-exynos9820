#
# Makefile for the DSMS
#

obj-y := dsms_kernel_api.o
obj-y += dsms_access_control.o
obj-y += dsms_policy.o
obj-y += dsms_init.o
obj-y += dsms_debug.o

EXTRA_CFLAGS += -DDSMS_WHITELIST_IGNORE_NAME_SUFFIXES_ENABLE

# Debugging options:

DEBUG_ENABLE=true
DEBUG_TRACE_CALLS_ENABLE=true
DEBUG_WHITELIST_ENABLE=false

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))

ifeq ($(DEBUG_ENABLE), true)
    EXTRA_CFLAGS += -DDSMS_DEBUG_ENABLE

    ifeq ($(DEBUG_TRACE_CALLS_ENABLE), true)
        EXTRA_CFLAGS += -DDSMS_DEBUG_TRACE_DSMS_CALLS
    endif

    ifeq ($(DEBUG_WHITELIST_ENABLE), true)
        EXTRA_CFLAGS += -DDSMS_DEBUG_WHITELIST
    endif

endif

endif
