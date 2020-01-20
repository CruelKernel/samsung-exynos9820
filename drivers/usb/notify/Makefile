
# usb notify driver
obj-$(CONFIG_USB_HOST_NOTIFY)			+= host_notify_class.o
obj-$(CONFIG_USB_EXTERNAL_NOTIFY)		+= external_notify.o
obj-$(CONFIG_USB_NOTIFY_PROC_LOG)		+= usblog_proc_notify.o
obj-$(CONFIG_USB_NOTIFY_LAYER)			+= usb_notify_layer.o
usb_notify_layer-y				:= usb_notify.o  usb_notify_sysfs.o dock_notify.o
obj-$(CONFIG_USB_NOTIFIER)			+= usb_notifier.o
obj-$(CONFIG_USB_PORT_POWER_OPTIMIZATION)	+= usb_power_notify.o
