# SPDX-License-Identifier: GPL-2.0
#
# kbuild file for firmware/
#

CONFIG_EXTRA_FIRMWARE_DIR="firmware"

# Create $(fwabs) from $(CONFIG_EXTRA_FIRMWARE_DIR) -- if it doesn't have a
# leading /, it's relative to $(srctree).
fwdir := $(subst $(quote),,$(CONFIG_EXTRA_FIRMWARE_DIR))
fwabs := $(addprefix $(srctree)/,$(filter-out /%,$(fwdir)))$(filter /%,$(fwdir))

fw-external-y := $(subst $(quote),,$(CONFIG_EXTRA_FIRMWARE))

fw-external-$(CONFIG_EXYNOS9810_ACPM) += exynos9810_acpm_tmu.fw
fw-shipped-$(CONFIG_TOUCHSCREEN_SEC_TS_Y771) += tsp_sec/y761_beyond0_phole.bin tsp_sec/y771_beyond1_phole.bin \
						tsp_sec/y771_beyond2_phole.bin tsp_sec/y771_beyondx_phole.bin

fw-shipped-$(CONFIG_TOUCHSCREEN_SEC_TS_Y771_D) += tsp_sec/y771_d1.bin tsp_sec/y771_d2.bin \
						tsp_sec/y771_d1_bringup.bin tsp_sec/y771_davinci_bringup.bin tsp_sec/y771_davinci_bringup2nd.bin

fw-shipped-$(CONFIG_EPEN_WACOM_W9020) += epen/w9020_d2.fw epen/w9020_d1.fw epen/w9020_d1_bringup.fw
ifdef CONFIG_SEC_FACTORY
fw-shipped-$(CONFIG_EPEN_WACOM_W9020) += epen/w9020_d2_unit.fw epen/w9020_d1_unit.fw
endif

fw-shipped-$(CONFIG_KEYBOARD_STM_FSR) += key_stm/fsr1ad04_davinci.fw key_stm/fsr1ad04_d2_dv2.fw \
					key_stm/fsr1ad04_d1.fw key_stm/fsr1ad04_d2.fw \
					key_stm/fsr1ad04_davinci_dv1.fw key_stm/fsr1ad04_d1_dv1.fw \
					key_stm/fsr1ad04_d1_dv2.fw

fw-shipped-$(CONFIG_WIRELESS_FIRMWARE_UPDATE) += mfc/mfc_fw_flash.bin mfc/mfc_fw_flash_lsi.bin
ifdef CONFIG_WIRELESS_FIRMWARE_UPDATE
fw-shipped-$(CONFIG_WIRELESS_CHARGER_MFC_S2MIW04) += mfc/mfc_fw_flash_s2miw04.bin
endif

# There are three cases to care about:
# 1. Building kernel with CONFIG_FIRMWARE_IN_KERNEL=y -- $(fw-shipped-y) should
#    include the firmware files to include, according to .config
# 2. 'make modules_install', which will install firmware for modules, and 
#    _also_ for the in-kernel drivers when CONFIG_FIRMWARE_IN_KERNEL=n
# 3. 'make firmware_install', which installs all firmware, unconditionally.

# For the former two cases we want $(fw-shipped-y) and $(fw-shipped-m) to be
# accurate. In the latter case it doesn't matter -- it'll use $(fw-shipped-all).
# But be aware that the config file might not be included at all.

ifdef CONFIG_ACENIC_OMIT_TIGON_I
acenic-objs := acenic/tg2.bin
fw-shipped- += acenic/tg1.bin
else
acenic-objs := acenic/tg1.bin acenic/tg2.bin
endif
fw-shipped-$(CONFIG_ACENIC) += $(acenic-objs)
fw-shipped-$(CONFIG_ADAPTEC_STARFIRE) += adaptec/starfire_rx.bin \
					 adaptec/starfire_tx.bin
fw-shipped-$(CONFIG_ATARI_DSP56K) += dsp56k/bootstrap.bin
fw-shipped-$(CONFIG_ATM_AMBASSADOR) += atmsar11.fw
fw-shipped-$(CONFIG_BNX2X) += bnx2x/bnx2x-e1-6.2.9.0.fw \
			      bnx2x/bnx2x-e1h-6.2.9.0.fw \
			      bnx2x/bnx2x-e2-6.2.9.0.fw
fw-shipped-$(CONFIG_BNX2) += bnx2/bnx2-mips-09-6.2.1a.fw \
			     bnx2/bnx2-rv2p-09-6.0.17.fw \
			     bnx2/bnx2-rv2p-09ax-6.0.17.fw \
			     bnx2/bnx2-mips-06-6.2.1.fw \
			     bnx2/bnx2-rv2p-06-6.0.15.fw
fw-shipped-$(CONFIG_CASSINI) += sun/cassini.bin
fw-shipped-$(CONFIG_CHELSIO_T3) += cxgb3/t3b_psram-1.1.0.bin \
				   cxgb3/t3c_psram-1.1.0.bin \
				   cxgb3/ael2005_opt_edc.bin \
				   cxgb3/ael2005_twx_edc.bin \
				   cxgb3/ael2020_twx_edc.bin
fw-shipped-$(CONFIG_DRM_MGA) += matrox/g200_warp.fw matrox/g400_warp.fw
fw-shipped-$(CONFIG_DRM_R128) += r128/r128_cce.bin
fw-shipped-$(CONFIG_DRM_RADEON) += radeon/R100_cp.bin radeon/R200_cp.bin \
				   radeon/R300_cp.bin radeon/R420_cp.bin \
				   radeon/RS690_cp.bin radeon/RS600_cp.bin \
				   radeon/R520_cp.bin \
				   radeon/R600_pfp.bin radeon/R600_me.bin \
				   radeon/RV610_pfp.bin radeon/RV610_me.bin \
				   radeon/RV630_pfp.bin radeon/RV630_me.bin \
				   radeon/RV620_pfp.bin radeon/RV620_me.bin \
				   radeon/RV635_pfp.bin radeon/RV635_me.bin \
				   radeon/RV670_pfp.bin radeon/RV670_me.bin \
				   radeon/RS780_pfp.bin radeon/RS780_me.bin \
				   radeon/RV770_pfp.bin radeon/RV770_me.bin \
				   radeon/RV730_pfp.bin radeon/RV730_me.bin \
				   radeon/RV710_pfp.bin radeon/RV710_me.bin
fw-shipped-$(CONFIG_DVB_AV7110) += av7110/bootcode.bin
fw-shipped-$(CONFIG_DVB_TTUSB_BUDGET) += ttusb-budget/dspbootcode.bin
fw-shipped-$(CONFIG_E100) += e100/d101m_ucode.bin e100/d101s_ucode.bin \
			     e100/d102e_ucode.bin
fw-shipped-$(CONFIG_MYRI_SBUS) += myricom/lanai.bin
fw-shipped-$(CONFIG_PCMCIA_PCNET) += cis/LA-PCM.cis cis/PCMLM28.cis \
				     cis/DP83903.cis cis/NE2K.cis \
				     cis/tamarack.cis cis/PE-200.cis \
				     cis/PE520.cis
fw-shipped-$(CONFIG_PCMCIA_3C589) += cis/3CXEM556.cis
fw-shipped-$(CONFIG_PCMCIA_3C574) += cis/3CCFEM556.cis
fw-shipped-$(CONFIG_SERIAL_8250_CS) += cis/MT5634ZLX.cis cis/RS-COM-2P.cis \
				       cis/COMpad2.cis cis/COMpad4.cis \
				       cis/SW_555_SER.cis cis/SW_7xx_SER.cis \
				       cis/SW_8xx_SER.cis
fw-shipped-$(CONFIG_PCMCIA_SMC91C92) += ositech/Xilinx7OD.bin
fw-shipped-$(CONFIG_SCSI_ADVANSYS) += advansys/mcode.bin advansys/38C1600.bin \
				      advansys/3550.bin advansys/38C0800.bin
fw-shipped-$(CONFIG_SCSI_QLOGIC_1280) += qlogic/1040.bin qlogic/1280.bin \
					 qlogic/12160.bin
fw-shipped-$(CONFIG_SCSI_QLOGICPTI) += qlogic/isp1000.bin
fw-shipped-$(CONFIG_INFINIBAND_QIB) += qlogic/sd7220.fw
fw-shipped-$(CONFIG_SND_KORG1212) += korg/k1212.dsp
fw-shipped-$(CONFIG_SND_MAESTRO3) += ess/maestro3_assp_kernel.fw \
				     ess/maestro3_assp_minisrc.fw
fw-shipped-$(CONFIG_SND_SB16_CSP) += sb16/mulaw_main.csp sb16/alaw_main.csp \
				     sb16/ima_adpcm_init.csp \
				     sb16/ima_adpcm_playback.csp \
				     sb16/ima_adpcm_capture.csp
fw-shipped-$(CONFIG_SND_YMFPCI) += yamaha/ds1_ctrl.fw yamaha/ds1_dsp.fw \
				   yamaha/ds1e_ctrl.fw
fw-shipped-$(CONFIG_SND_WAVEFRONT) += yamaha/yss225_registers.bin
fw-shipped-$(CONFIG_TEHUTI) += tehuti/bdx.bin
fw-shipped-$(CONFIG_TIGON3) += tigon/tg3.bin tigon/tg3_tso.bin \
			       tigon/tg3_tso5.bin
fw-shipped-$(CONFIG_TYPHOON) += 3com/typhoon.bin
fw-shipped-$(CONFIG_USB_EMI26) += emi26/loader.fw emi26/firmware.fw \
				  emi26/bitstream.fw
fw-shipped-$(CONFIG_USB_EMI62) += emi62/loader.fw emi62/bitstream.fw \
				  emi62/spdif.fw emi62/midi.fw
fw-shipped-$(CONFIG_USB_KAWETH) += kaweth/new_code.bin kaweth/trigger_code.bin \
				   kaweth/new_code_fix.bin \
				   kaweth/trigger_code_fix.bin

fw-shipped-$(CONFIG_EXYNOS8895_ACPM) += exynos8895_acpm_dvfs.fw
ifdef CONFIG_FIRMWARE_IN_KERNEL
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_MPR) += keyspan/mpr.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA18X) += keyspan/usa18x.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA19) += keyspan/usa19.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA19QI) += keyspan/usa19qi.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA19QW) += keyspan/usa19qw.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA19W) += keyspan/usa19w.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA28) += keyspan/usa28.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA28XA) += keyspan/usa28xa.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA28XB) += keyspan/usa28xb.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA28X) += keyspan/usa28x.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA49W) += keyspan/usa49w.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_USA49WLC) += keyspan/usa49wlc.fw
else
fw-shipped- += keyspan/mpr.fw keyspan/usa18x.fw keyspan/usa19.fw	\
	keyspan/usa19qi.fw keyspan/usa19qw.fw keyspan/usa19w.fw		\
	keyspan/usa28.fw keyspan/usa28xa.fw keyspan/usa28xb.fw		\
	keyspan/usa28x.fw keyspan/usa49w.fw keyspan/usa49wlc.fw
endif
fw-shipped-$(CONFIG_USB_SERIAL_TI) += ti_3410.fw ti_5052.fw \
				      mts_cdma.fw mts_gsm.fw mts_edge.fw
fw-shipped-$(CONFIG_USB_SERIAL_EDGEPORT) += edgeport/boot.fw edgeport/boot2.fw \
					    edgeport/down.fw edgeport/down2.fw
fw-shipped-$(CONFIG_USB_SERIAL_EDGEPORT_TI) += edgeport/down3.bin
fw-shipped-$(CONFIG_USB_SERIAL_WHITEHEAT) += whiteheat_loader.fw whiteheat.fw \
					   # whiteheat_loader_debug.fw
fw-shipped-$(CONFIG_USB_SERIAL_KEYSPAN_PDA) += keyspan_pda/keyspan_pda.fw
fw-shipped-$(CONFIG_USB_SERIAL_XIRCOM) += keyspan_pda/xircom_pgs.fw
fw-shipped-$(CONFIG_USB_VICAM) += vicam/firmware.fw
fw-shipped-$(CONFIG_VIDEO_CPIA2) += cpia2/stv0672_vp4.bin
fw-shipped-$(CONFIG_YAM) += yam/1200.bin yam/9600.bin

fw-shipped-$(CONFIG_FIVE_TEE_DRIVER) += five/ffffffff000000000000000000000072.tlbin

fw-shipped-all := $(fw-shipped-y) $(fw-shipped-m) $(fw-shipped-)

quiet_cmd_ihex  = IHEX    $@
      cmd_ihex  = $(OBJCOPY) -Iihex -Obinary $< $@

quiet_cmd_ihex2fw  = IHEX2FW $@
      cmd_ihex2fw  = $(objtree)/$(obj)/ihex2fw $< $@

quiet_cmd_h16tofw  = H16TOFW $@
      cmd_h16tofw  = $(objtree)/$(obj)/ihex2fw -w $< $@

quiet_cmd_fwbin = MK_FW   $@
      cmd_fwbin = FWNAME="$(patsubst firmware/%.gen.S,%,$@)";		     \
		  FWSTR="$(subst /,_,$(subst .,_,$(subst -,_,$(patsubst	     \
				firmware/%.gen.S,%,$@))))";		     \
		  ASM_WORD=$(if $(CONFIG_64BIT),.quad,.long);		     \
		  ASM_ALIGN=$(if $(CONFIG_64BIT),3,2);			     \
		  PROGBITS=$(if $(CONFIG_ARM),%,@)progbits;		     \
		  echo "/* Generated by firmware/Makefile */"		> $@;\
		  echo "    .section .rodata"				>>$@;\
		  echo "    .p2align $${ASM_ALIGN}"			>>$@;\
		  echo "_fw_$${FWSTR}_bin:"				>>$@;\
		  echo "    .incbin \"$(2)\""				>>$@;\
		  echo "_fw_end:"					>>$@;\
		  echo "   .section .rodata.str,\"aMS\",$${PROGBITS},1"	>>$@;\
		  echo "    .p2align $${ASM_ALIGN}"			>>$@;\
		  echo "_fw_$${FWSTR}_name:"				>>$@;\
		  echo "    .string \"$$FWNAME\""			>>$@;\
		  echo "    .section .builtin_fw,\"a\",$${PROGBITS}"	>>$@;\
		  echo "    .p2align $${ASM_ALIGN}"			>>$@;\
		  echo "    $${ASM_WORD} _fw_$${FWSTR}_name"		>>$@;\
		  echo "    $${ASM_WORD} _fw_$${FWSTR}_bin"		>>$@;\
		  echo "    $${ASM_WORD} _fw_end - _fw_$${FWSTR}_bin"	>>$@;

# One of these files will change, or come into existence, whenever
# the configuration changes between 32-bit and 64-bit. The .S files
# need to change when that happens.
wordsize_deps := $(wildcard include/config/64bit.h include/config/32bit.h \
		include/config/ppc32.h include/config/ppc64.h \
		include/config/superh32.h include/config/superh64.h \
		include/config/x86_32.h include/config/x86_64.h \
		firmware/Makefile)

$(patsubst %,$(obj)/%.gen.S, $(fw-shipped-y)): %: $(wordsize_deps)
	$(call cmd,fwbin,$(fwabs)/$(patsubst $(obj)/%.gen.S,%,$@))
$(patsubst %,$(obj)/%.gen.S, $(fw-external-y)): %: $(wordsize_deps)
	$(call cmd,fwbin,$(fwabs)/$(patsubst $(obj)/%.gen.S,%,$@))

# The .o files depend on the binaries directly; the .S files don't.
$(patsubst %,$(obj)/%.gen.o, $(fw-shipped-y)): $(obj)/%.gen.o: $(fwdir)/%
$(patsubst %,$(obj)/%.gen.o, $(fw-external-y)): $(obj)/%.gen.o: $(fwdir)/%

# .ihex is used just as a simple way to hold binary files in a source tree
# where binaries are frowned upon. They are directly converted with objcopy.
$(obj)/%: $(obj)/%.ihex
	$(call cmd,ihex)

obj-y				 += $(patsubst %,%.gen.o, $(fw-external-y))
obj-$(CONFIG_FIRMWARE_IN_KERNEL) += $(patsubst %,%.gen.o, $(fw-shipped-y))

ifeq ($(KBUILD_SRC),)
# Makefile.build only creates subdirectories for O= builds, but external
# firmware might live outside the kernel source tree
_dummy := $(foreach d,$(addprefix $(obj)/,$(dir $(fw-external-y))), $(shell [ -d $(d) ] || mkdir -p $(d)))
endif

targets := $(patsubst $(obj)/%,%, \
                                $(shell find $(obj) -name \*.gen.S 2>/dev/null))
# Without this, built-in.o won't be created when it's empty, and the
# final vmlinux link will fail.
obj- := dummy
