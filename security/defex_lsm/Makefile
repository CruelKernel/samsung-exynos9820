#
# Makefile for the Defex
#

# Features to Enable
PED_ENABLE=true
SAFEPLACE_ENABLE=true
INTEGRITY_ENABLE=true

# caches to enable
CACHES_ENABLE=true

obj-y := defex_lsm.o
obj-y += defex_catch_list.o
obj-y += defex_ht.o
obj-y += defex_config.o
obj-y += defex_sysfs.o
obj-y += defex_procs.o
obj-y += defex_catch_list_compat.o
obj-y += defex_rules.o

ifeq ($(CONFIG_DEFEX_KERNEL_ONLY), y)
EXTRA_CFLAGS += -DDEFEX_KERNEL_ONLY
endif

ifeq ($(PED_ENABLE), true)
    obj-y += defex_priv.o
    EXTRA_CFLAGS += -DDEFEX_PED_ENABLE
endif

ifeq ($(SAFEPLACE_ENABLE), true)
    obj-y += defex_safeplace.o
    EXTRA_CFLAGS += -DDEFEX_SAFEPLACE_ENABLE
endif

ifeq ($(INTEGRITY_ENABLE), true)
    EXTRA_CFLAGS += -DDEFEX_INTEGRITY_ENABLE
endif

ifeq ($(CACHES_ENABLE), true)
    obj-y += defex_caches.o
    EXTRA_CFLAGS += -DDEFEX_CACHES_ENABLE
endif

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
    obj-y += defex_debug.o
    EXTRA_CFLAGS += -DDEFEX_PERMISSIVE_SP
    EXTRA_CFLAGS += -DDEFEX_DEBUG_ENABLE
    EXTRA_CFLAGS += -DDEFEX_SYSFS_ENABLE
endif

ifeq ($(CONFIG_SEC_VTS_TEST), y)
EXTRA_CFLAGS += -DDEFEX_PERMISSIVE_SP
EXTRA_CFLAGS += -DDEFEX_PERMISSIVE_PED
endif

ifeq ($(CONFIG_SEC_FACTORY), y)
EXTRA_CFLAGS += -DDEFEX_PERMISSIVE_SP
EXTRA_CFLAGS += -DDEFEX_PERMISSIVE_PED
endif

ifneq ($(wildcard  $(srctree)/$(src)/pack_rules.c),)
    EXTRA_CFLAGS += -DDEFEX_USE_PACKED_RULES
    hostprogs-y := pack_rules
    HOST_EXTRACFLAGS += $(EXTRA_CFLAGS)
    clean-files := $(srctree)/$(src)/defex_packed_rules.inc
    DEPEND_LIST := $(obj)/pack_rules

    $(obj)/defex_sysfs.o: $(obj)/pack_rules $(srctree)/$(src)/defex_packed_rules.inc

    quiet_cmd_pack = PACK    $<
          cmd_pack = $(obj)/pack_rules -p $< $@ $(srctree)/$(src)/defex_packed_rules.bin

    SOURCE_RULES := $(srctree)/$(src)/defex_rules.c
    ifneq ($(wildcard $(srctree)/$(src)/file_list),)
        $(warning '[DEFEX] file_list found')
        SOURCE_RULES := $(srctree)/$(src)/defex_rules_reduced.c
        DEPEND_LIST += $(SOURCE_RULES)
        DEPEND_LIST += $(srctree)/$(src)/file_list
        clean-files += $(DEPEND_LIST)

        quiet_cmd_reduce = REDUCE  $<
              cmd_reduce = $(obj)/pack_rules -r $< $@ $(srctree)/$(src)/file_list

        $(srctree)/$(src)/defex_rules_reduced.c: $(srctree)/$(src)/defex_rules.c $(obj)/pack_rules
		$(call cmd,reduce)
    endif

    $(srctree)/$(src)/defex_packed_rules.inc: $(SOURCE_RULES) $(DEPEND_LIST)
	$(call cmd,pack)

endif

