# drivers/staging/samsung/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_batt.o
# Samsung Debug Feature
obj-$(CONFIG_SEC_EXT)		+= sec_log.o

obj-$(CONFIG_SEC_MEM)		+= sec_memory.o
# Samsung Reset Feature
obj-$(CONFIG_SEC_REBOOT)	+= sec_reboot.o
# Samsung Misc Feature
obj-$(CONFIG_SEC_MISC)		+= sec_misc.o
# Samsung sec sysfs Feature
obj-$(CONFIG_SEC_SYSFS)		+= sec_sysfs.o

# Samsung TN Debugging Features
obj-$(CONFIG_SEC_DEBUG)              += sec_debug.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_test.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_hard_reset_hook.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_upload.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_crash_key.o
obj-$(CONFIG_SEC_DEBUG_INIT_LOG)      += sec_debug_init_log.o
obj-$(CONFIG_SEC_DEBUG_RESET_REASON) += sec_debug_reset_reason.o
obj-$(CONFIG_SEC_DEBUG_EXTRA_INFO)	+= sec_debug_extra_info.o
obj-$(CONFIG_SEC_DEBUG_HW_PARAM)	+= sec_debug_hw_param.o
obj-$(CONFIG_SEC_DEBUG_AUTO_COMMENT)	+= sec_debug_auto_comment.o
#obj-$(CONFIG_SEC_DEBUG_DUMP_SUMMARY)	+= sec_debug_dump_summary.o
#obj-$(CONFIG_SEC_DEBUG_DUMP_SUMMARY)	+= sec_debug_summary.o
obj-$(CONFIG_SEC_DEBUG_LAST_KMSG)	+= sec_debug_last_kmsg.o
obj-$(CONFIG_SEC_PARAM)              += sec_param.o
obj-$(CONFIG_SEC_BOOTSTAT)		+= sec_bootstat.o
# SecureStorage
obj-$(CONFIG_SEC_DEBUG_GAF_V4)		+= sec_gaf_v4.o
obj-$(CONFIG_SEC_DEBUG_GAF_V5)		+= sec_gaf_v5.o

# Samsung Mobile Power Management Debugging Support
obj-$(CONFIG_SEC_PM_DEBUG)		+= sec_pm_debug.o

# Samsung sec NAD Feature
obj-$(CONFIG_SEC_NAD)		+= sec_nad.o

# Samsung TN NAD Balancer Feature
obj-$(CONFIG_SEC_NAD_BALANCER)		+= sec_nad_balancer.o

# Samsung TN Heavy Cpu Count
obj-$(CONFIG_SEC_HEAVY_TASK_CPU)		+= sec_heavy_task_cpu.o

# Samsung HQM device feature
obj-$(CONFIG_SEC_HQM_DEVICE)		+= sec_hqm_device.o

# Samsung Throughput monitoring Feature
obj-$(CONFIG_ARGOS)                  += sec_argos.o
