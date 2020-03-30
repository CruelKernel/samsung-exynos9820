# SPDX-License-Identifier: GPL-2.0
#
# Makefile for building the SELinux module as part of the kernel tree.
#

# [ SEC_SELINUX_PORTING_COMMON
ccflags-y := -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include

ifeq ($(TARGET_BUILD_VARIANT), eng)
ifeq ($(SEC_BUILD_OPTION_PRODUCT_SHIP), false)
ifeq ($(SEC_BUILD_OPTION_SELINUX_ENFORCE),true)
ccflags-y += -DSEC_SELINUX_DEBUG
endif
endif
endif

ifeq ($(SEC_BUILD_OPTION_VTS),false)
ifeq ($(TARGET_BUILD_VARIANT), user)
ifeq ($(SEC_BUILD_OPTION_PRODUCT_SHIP), true)
ifeq ($(SEC_BUILD_OPTION_SELINUX_ENFORCE),true)
ccflags-y += -DCONFIG_ALWAYS_ENFORCE=true
endif
endif
endif
endif
# ] SEC_SELINUX_PORTING_COMMON

obj-$(CONFIG_SECURITY_SELINUX) := selinux.o

selinux-y := avc.o hooks.o selinuxfs.o netlink.o nlmsgtab.o netif.o \
	     netnode.o netport.o ibpkey.o exports.o \
	     ss/ebitmap.o ss/hashtab.o ss/symtab.o ss/sidtab.o ss/avtab.o \
	     ss/policydb.o ss/services.o ss/conditional.o ss/mls.o ss/status.o

selinux-$(CONFIG_SECURITY_NETWORK_XFRM) += xfrm.o

selinux-$(CONFIG_NETLABEL) += netlabel.o

$(addprefix $(obj)/,$(selinux-y)): $(obj)/flask.h

quiet_cmd_flask = GEN     $(obj)/flask.h $(obj)/av_permissions.h
      cmd_flask = scripts/selinux/genheaders/genheaders $(obj)/flask.h $(obj)/av_permissions.h

targets += flask.h av_permissions.h
$(obj)/flask.h: $(src)/include/classmap.h FORCE
	$(call if_changed,flask)
