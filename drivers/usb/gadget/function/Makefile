# SPDX-License-Identifier: GPL-2.0
#
# USB peripheral controller drivers
#

ccflags-y			:= -I$(srctree)/drivers/usb/gadget/
ccflags-y			+= -I$(srctree)/drivers/usb/gadget/udc/

# USB Functions
usb_f_acm-y			:= f_acm.o
obj-$(CONFIG_USB_F_ACM)		+= usb_f_acm.o
usb_f_uts-y			:= f_uts.o
obj-$(CONFIG_USB_F_ACM)		+= usb_f_uts.o
usb_f_ss_lb-y			:= f_loopback.o f_sourcesink.o
obj-$(CONFIG_USB_F_SS_LB)	+= usb_f_ss_lb.o
obj-$(CONFIG_USB_U_SERIAL)	+= u_serial.o
usb_f_serial-y			:= f_serial.o
obj-$(CONFIG_USB_F_SERIAL)	+= usb_f_serial.o
usb_f_obex-y			:= f_obex.o
obj-$(CONFIG_USB_F_OBEX)	+= usb_f_obex.o
obj-$(CONFIG_USB_U_ETHER)	+= u_ether.o
usb_f_ncm-y			:= f_ncm.o
obj-$(CONFIG_USB_F_NCM)		+= usb_f_ncm.o
usb_f_ecm-y			:= f_ecm.o
obj-$(CONFIG_USB_F_ECM)		+= usb_f_ecm.o
usb_f_phonet-y			:= f_phonet.o
obj-$(CONFIG_USB_F_PHONET)	+= usb_f_phonet.o
usb_f_eem-y			:= f_eem.o
obj-$(CONFIG_USB_F_EEM)		+= usb_f_eem.o
usb_f_ecm_subset-y		:= f_subset.o
obj-$(CONFIG_USB_F_SUBSET)	+= usb_f_ecm_subset.o
usb_f_rndis-y			:= f_rndis.o rndis.o
obj-$(CONFIG_USB_F_RNDIS)	+= usb_f_rndis.o
usb_f_mass_storage-y		:= f_mass_storage.o storage_common.o
obj-$(CONFIG_USB_F_MASS_STORAGE)+= usb_f_mass_storage.o
usb_f_fs-y			:= f_fs.o
obj-$(CONFIG_USB_F_FS)		+= usb_f_fs.o
obj-$(CONFIG_USB_U_AUDIO)	+= u_audio.o
usb_f_uac1-y			:= f_uac1.o
obj-$(CONFIG_USB_F_UAC1)	+= usb_f_uac1.o
usb_f_uac1_legacy-y		:= f_uac1_legacy.o u_uac1_legacy.o
obj-$(CONFIG_USB_F_UAC1_LEGACY)	+= usb_f_uac1_legacy.o
usb_f_uac2-y			:= f_uac2.o
obj-$(CONFIG_USB_F_UAC2)	+= usb_f_uac2.o
usb_f_uvc-y			:= f_uvc.o uvc_queue.o uvc_v4l2.o uvc_video.o uvc_configfs.o
obj-$(CONFIG_USB_F_UVC)		+= usb_f_uvc.o
usb_f_midi-y			:= f_midi.o
obj-$(CONFIG_USB_F_MIDI)	+= usb_f_midi.o
usb_f_hid-y			:= f_hid.o
obj-$(CONFIG_USB_F_HID)		+= usb_f_hid.o
usb_f_printer-y			:= f_printer.o
obj-$(CONFIG_USB_F_PRINTER)	+= usb_f_printer.o
usb_f_dm-y			:= f_dm.o
obj-$(CONFIG_USB_CONFIGFS_F_DM)	+= usb_f_dm.o
usb_f_dm1-y			:= f_dm1.o
obj-$(CONFIG_USB_CONFIGFS_F_DM)	+= usb_f_dm1.o
usb_f_adb-y			:= f_adb.o
obj-$(CONFIG_USB_CONFIGFS_F_ADB)+= usb_f_adb.o
usb_f_tcm-y			:= f_tcm.o
obj-$(CONFIG_USB_F_TCM)		+= usb_f_tcm.o
ifeq ($(CONFIG_SEC_FACTORY),y)
	usb_f_mtp-y             := f_mtp.o
	obj-$(CONFIG_USB_F_MTP) += usb_f_mtp.o
else
ifeq ($(CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE),y)
	usb_f_mtp_samsung-y     := f_mtp_samsung.o
	obj-$(CONFIG_USB_F_MTP) += usb_f_mtp_samsung.o
else
	usb_f_mtp-y             := f_mtp.o
        obj-$(CONFIG_USB_F_MTP) += usb_f_mtp.o
endif
endif
usb_f_ptp-y                     := f_ptp.o
obj-$(CONFIG_USB_F_PTP)         += usb_f_ptp.o
usb_f_audio_source-y            := f_audio_source.o
obj-$(CONFIG_USB_F_AUDIO_SRC)   += usb_f_audio_source.o
usb_f_accessory-y               := f_accessory.o
obj-$(CONFIG_USB_F_ACC)         += usb_f_accessory.o
usb_f_conn_gadget-y             := f_conn_gadget.o
obj-$(CONFIG_USB_F_CONN_GADGET) += usb_f_conn_gadget.o
obj-$(CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE)	+= multi_config.o
usb_f_ss_mon_gadget-y             := f_ss_mon_gadget.o
obj-$(CONFIG_USB_F_SS_MON_GADGET) += usb_f_ss_mon_gadget.o