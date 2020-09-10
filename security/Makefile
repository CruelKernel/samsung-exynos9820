# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the kernel security code
#

obj-$(CONFIG_KEYS)			+= keys/
subdir-$(CONFIG_SECURITY_SELINUX)	+= selinux
subdir-$(CONFIG_SECURITY_SMACK)		+= smack
subdir-$(CONFIG_SECURITY_TOMOYO)        += tomoyo
subdir-$(CONFIG_SECURITY_APPARMOR)	+= apparmor
subdir-$(CONFIG_SECURITY_YAMA)		+= yama
subdir-$(CONFIG_SECURITY_LOADPIN)	+= loadpin
subdir-$(CONFIG_SECURITY_DEFEX)		+= samsung/defex_lsm
subdir-$(CONFIG_SECURITY_DSMS)		+= samsung/dsms

# always enable default capabilities
obj-y					+= commoncap.o
obj-$(CONFIG_MMU)			+= min_addr.o

# Object file lists
obj-$(CONFIG_SECURITY)			+= security.o
obj-$(CONFIG_SECURITYFS)		+= inode.o
obj-$(CONFIG_SECURITY_SELINUX)		+= selinux/
obj-$(CONFIG_SECURITY_SMACK)		+= smack/
obj-$(CONFIG_AUDIT)			+= lsm_audit.o
obj-$(CONFIG_SECURITY_TOMOYO)		+= tomoyo/
obj-$(CONFIG_SECURITY_APPARMOR)		+= apparmor/
obj-$(CONFIG_SECURITY_YAMA)		+= yama/
obj-$(CONFIG_SECURITY_LOADPIN)		+= loadpin/
obj-$(CONFIG_CGROUP_DEVICE)		+= device_cgroup.o
obj-$(CONFIG_SECURITY_DEFEX)		+= samsung/defex_lsm/built-in.o
obj-$(CONFIG_SECURITY_DSMS)		+= samsung/dsms/

# Object integrity file lists
subdir-$(CONFIG_INTEGRITY)		+= integrity
obj-$(CONFIG_INTEGRITY)			+= integrity/

# MST driver
obj-$(CONFIG_MST_LDO)	  += mstdrv/

# Knox SDP
obj-$(CONFIG_SDP) += sdp/
obj-$(CONFIG_SDP) += sdp/built-in.o

# FIVE
subdir-$(CONFIG_FIVE)			+= samsung/five
obj-$(CONFIG_FIVE)			+= samsung/five/

# PROCA module
subdir-$(CONFIG_PROCA)		+= proca
obj-$(CONFIG_PROCA)		+= proca/

# GAF module for PROCA, Secure Storage 1
subdir-$(CONFIG_GAF) += proca/gaf
obj-$(CONFIG_GAF)    += proca/gaf/
