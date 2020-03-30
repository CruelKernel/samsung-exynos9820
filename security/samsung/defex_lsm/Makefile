#
# Makefile for the Defex
#

# Features to Enable
PED_ENABLE=true
PED_BASED_ON_TGID_ENABLE=true
SAFEPLACE_ENABLE=true
IMMUTABLE_ENABLE=true
LP_ENABLE=true

ifneq ($(wildcard $(srctree)/include/crypto/internal/rsa.h),)
    $(warning [DEFEX] INTEGRITY_ENABLE)
    INTEGRITY_ENABLE=true
endif

# caches to enable
CACHES_ENABLE=true

# OEM Unlock dependency
OEM_UNLOCK_DEPENDENCY=true

# use the ramdisk or system_root to store rules file
RAMDISK_ENABLE=true

# do signing for rules
SIGN_ENABLE=true

obj-y := defex_lsm.o
obj-y += defex_common.o
obj-y += defex_catch_list.o
obj-y += defex_ht.o
obj-y += defex_config.o
obj-y += defex_sysfs.o
obj-y += defex_procs.o
obj-y += defex_rules.o
obj-$(CONFIG_COMPAT) += defex_catch_list_compat.o

# Immutable Feature is applied with permissive mode first.
DEFEX_DEFINES := -DDEFEX_PERMISSIVE_IM

ifeq ($(CONFIG_DEFEX_KERNEL_ONLY), y)
    DEFEX_DEFINES += -DDEFEX_KERNEL_ONLY
    ifeq ($(CONFIG_SAMSUNG_PRODUCT_SHIP), y)
        $(warning [DEFEX] Kernel_only & Ship)
    else
        $(warning [DEFEX] Kernel_only & Noship)
        DEFEX_DEFINES += -DDEFEX_PERMISSIVE_SP
        DEFEX_DEFINES += -DDEFEX_PERMISSIVE_IM
        DEFEX_DEFINES += -DDEFEX_PERMISSIVE_LP
        DEFEX_DEFINES += -DDEFEX_DEBUG_ENABLE
    endif
endif

ifeq ($(PED_ENABLE), true)
    obj-y += defex_priv.o
    DEFEX_DEFINES += -DDEFEX_PED_ENABLE
ifeq ($(PED_BASED_ON_TGID_ENABLE), true)
    DEFEX_DEFINES += -DDEFEX_PED_BASED_ON_TGID_ENABLE
endif
endif

ifeq ($(SAFEPLACE_ENABLE), true)
    obj-y += defex_safeplace.o
    DEFEX_DEFINES += -DDEFEX_SAFEPLACE_ENABLE
endif

ifeq ($(INTEGRITY_ENABLE), true)
    DEFEX_DEFINES += -DDEFEX_INTEGRITY_ENABLE
endif

ifeq ($(IMMUTABLE_ENABLE), true)
    obj-y += defex_immutable.o
    DEFEX_DEFINES += -DDEFEX_IMMUTABLE_ENABLE
endif

ifeq ($(LP_ENABLE), true)
    DEFEX_DEFINES += -DDEFEX_LP_ENABLE
endif

ifeq ($(CACHES_ENABLE), true)
    obj-y += defex_caches.o
    DEFEX_DEFINES += -DDEFEX_CACHES_ENABLE
endif

ifeq ($(OEM_UNLOCK_DEPENDENCY), true)
    DEFEX_DEFINES += -DDEFEX_DEPENDING_ON_OEMUNLOCK
endif

ifeq ($(RAMDISK_ENABLE), true)
    DEFEX_DEFINES += -DDEFEX_RAMDISK_ENABLE
ifeq ($(SIGN_ENABLE), true)
    obj-y += cert/defex_cert.o
    obj-y += defex_sign.o
    DEFEX_DEFINES += -DDEFEX_SIGN_ENABLE
endif
endif

ifneq (,$(filter userdebug eng, $(TARGET_BUILD_VARIANT)))
    obj-y += defex_debug.o
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_SP
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_IM
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_LP
    DEFEX_DEFINES += -DDEFEX_DEBUG_ENABLE
    DEFEX_DEFINES += -DDEFEX_SYSFS_ENABLE
else
    ifeq ($(CONFIG_SECURITY_DSMS), y)
        DEFEX_DEFINES += -DDEFEX_DSMS_ENABLE
    endif
endif

ifeq ($(SEC_BUILD_OPTION_VTS), true)
    $(warning [DEFEX] VTS true $(SEC_BUILD_OPTION_VTS))
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_SP
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_PED
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_IM
    DEFEX_DEFINES += -DDEFEX_PERMISSIVE_LP
else
    $(warning [DEFEX] VTS false $(SEC_BUILD_OPTION_VTS))
endif

EXTRA_CFLAGS += -I$(srctree)/$(src)/cert
EXTRA_AFLAGS += -Isecurity/samsung/defex_lsm/cert

ifneq ($(wildcard  $(srctree)/$(src)/pack_rules.c),)
    DEFEX_DEFINES += -DDEFEX_USE_PACKED_RULES
    EXTRA_CFLAGS += $(DEFEX_DEFINES)
    EXTRA_AFLAGS += $(DEFEX_DEFINES)
    hostprogs-y := pack_rules
    HOST_EXTRACFLAGS += $(DEFEX_DEFINES)
    clean-files := $(srctree)/$(src)/defex_packed_rules.inc
    DEPEND_LIST := $(obj)/pack_rules

    quiet_cmd_copy = COPY    $(obj)/pack_rules
          cmd_copy = cp -n $(obj)/pack_rules $(srctree)/$(src)/pack_rules 2>/dev/null || true

    quiet_cmd_pack = PACK    $<
          cmd_pack = $(obj)/pack_rules -p $< $@ $(srctree)/$(src)/defex_packed_rules.bin

    quiet_cmd_mkey = MAKEKEY $<
          cmd_mkey = cp -n $< $@ 2>/dev/null || true

    $(obj)/defex_sysfs.o: $(obj)/pack_rules $(srctree)/$(src)/defex_packed_rules.inc

    $(obj)/cert/defex_cert.o: $(obj)/cert/pubkey_eng.der $(obj)/cert/pubkey_user.der

    $(obj)/cert/pubkey_eng.der: $(srctree)/$(src)/cert/x509_five_eng.der
	$(call cmd,mkey)

    $(obj)/cert/pubkey_user.der: $(srctree)/$(src)/cert/x509_five_user.der
	$(call cmd,mkey)

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
	$(call cmd,copy)

else
    EXTRA_CFLAGS += $(DEFEX_DEFINES)
    EXTRA_AFLAGS += $(DEFEX_DEFINES)
endif
