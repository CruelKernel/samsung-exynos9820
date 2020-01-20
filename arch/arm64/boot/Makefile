#
# arch/arm64/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2012, ARM Ltd.
# Author: Will Deacon <will.deacon@arm.com>
#
# Based on the ia64 boot/Makefile.
#

include $(srctree)/arch/arm64/boot/dts/Makefile

OBJCOPYFLAGS_Image :=-O binary -R .note -R .note.gnu.build-id -R .comment -S

targets := Image Image.gz

OS_INFO := $(findstring x86_64, $(shell uname -a))

DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y) $(dtbo-y)
endif
DTB_OBJS := $(addprefix $(obj)/dts/,$(DTB_LIST))

$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)

$(obj)/Image.bz2: $(obj)/Image FORCE
	$(call if_changed,bzip2)

$(obj)/Image-dtb: $(obj)/Image $(DTB_OBJS) FORCE
	$(call if_changed,cat)

$(obj)/Image.gz: $(obj)/Image FORCE
	$(call if_changed,gzip)
ifeq ($(CONFIG_EXYNOS_SIGNATURE_BIN), y)
	@ls -al $(obj)/Image
	@echo "Add zero padding"
	@./tools/adjust_size $(obj)/Image $(obj)/Image_pad.bin $(CONFIG_SB_KERNEL_SIZE)
	@rm $(obj)/Image
ifeq ($(OS_INFO), x86_64)
	@echo "64 bit signer"
	@./tools/cslv/cslv_64 -infile $(obj)/Image_pad.bin -outfile $(obj)/Image -sign_type $(CONFIG_SB_SIGN_TYPE) -key_type $(CONFIG_SB_KEY_TYPE) -rb_count $(CONFIG_SB_RB_COUNT) -dynamic_length no
else
	@echo "32 bit signer"
	@./tools/cslv/cslv_32 -infile $(obj)/Image_pad.bin -outfile $(obj)/Image -sign_type $(CONFIG_SB_SIGN_TYPE) -key_type $(CONFIG_SB_KEY_TYPE) -rb_count $(CONFIG_SB_RB_COUNT) -dynamic_length no
endif
	@rm $(obj)/Image_pad.bin
endif

$(obj)/Image.lz4: $(obj)/Image FORCE
	$(call if_changed,lz4)

$(obj)/Image.lzma: $(obj)/Image FORCE
	$(call if_changed,lzma)

$(obj)/Image.lzo: $(obj)/Image FORCE
	$(call if_changed,lzo)

$(obj)/Image.gz-dtb: $(obj)/Image.gz $(DTB_OBJS) FORCE
	$(call if_changed,cat)

install:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image.gz System.map "$(INSTALL_PATH)"
