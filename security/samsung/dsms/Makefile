#
# Makefile for the DSMS
#
ccflags-y := -Wformat
ccflags-y += -DDSMS_ALLOWLIST_IGNORE_NAME_SUFFIXES_ENABLE
ccflags-y += -I$(srctree)/$(src)

obj-$(CONFIG_SECURITY_DSMS) := dsms_access_control.o
obj-$(CONFIG_SECURITY_DSMS) += dsms_init.o
obj-$(CONFIG_SECURITY_DSMS) += dsms_kernel_api.o
obj-$(CONFIG_SECURITY_DSMS) += dsms_policy.o
obj-$(CONFIG_SECURITY_DSMS) += dsms_rate_limit.o
obj-$(CONFIG_SECURITY_DSMS) += dsms_netlink.o
obj-$(CONFIG_SECURITY_DSMS) += dsms_message_list.o

# kunit tests options:
GCOV_PROFILE := $(CONFIG_KUNIT)
ifeq ($(CONFIG_KUNIT), y)
    ifneq ($(wildcard  $(srctree)/$(src)/test),)
        ccflags-y += -DDSMS_KUNIT_ENABLED
        obj-y += test/security_dsms_access_control_test.o
        obj-y += test/security_dsms_debug_test.o
        obj-y += test/security_dsms_init_test.o
        obj-y += test/security_dsms_kernel_api_test.o
        obj-y += test/security_dsms_policy_test.o
        obj-y += test/security_dsms_rate_limit_test.o
        obj-y += test/security_dsms_test_utils.o
    endif
endif
