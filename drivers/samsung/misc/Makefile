# drivers/samsung/misc/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

obj-$(CONFIG_SEC_EXT)		+= sec_log.o
obj-$(CONFIG_SEC_MEM)		+= sec_memory.o
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_batt.o
# Samsung Mobile Power Management Debugging Support
obj-$(CONFIG_SEC_PM_DEBUG)		+= sec_pm_debug.o
# Samsung sec NAD Feature
obj-$(CONFIG_SEC_NAD)		+= sec_nad.o
# Samsung TN NAD Balancer Feature
obj-$(CONFIG_SEC_NAD_BALANCER)		+= sec_nad_balancer.o
# Samsung HQM device feature
obj-$(CONFIG_SEC_HQM_DEVICE)		+= sec_hqm_device.o
# Samsung TN Heavy Cpu Count
obj-$(CONFIG_SEC_HEAVY_TASK_CPU)	+= sec_heavy_task_cpu.o
# Samsung Throughput monitoring Feature
obj-$(CONFIG_ARGOS)		+= sec_argos.o
# Samsung System Update
obj-$(CONFIG_SEC_SYSUP)		+= sec_sysup.o

obj-$(CONFIG_VBUS_NOTIFIER)	+= vbus_notifier.o

