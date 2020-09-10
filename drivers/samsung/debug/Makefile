# drivers/samsung/debug/Makefile
#
# Copyright (c) 2010-2011 Samsung Electronics Co., Ltd.
#		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Samsung TN Debugging Features
obj-$(CONFIG_SEC_DEBUG)              += sec_debug.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_test.o
obj-$(CONFIG_SEC_DEBUG)              += sec_debug_memtab.o
obj-$(CONFIG_SEC_DEBUG_INIT_LOG) 	+= sec_debug_init_log.o
obj-$(CONFIG_SEC_DEBUG_HIST_LOG) 	+= sec_debug_hist.o
obj-$(CONFIG_SEC_DEBUG_RESET_REASON) += sec_debug_reset_reason.o
obj-$(CONFIG_SEC_DEBUG_EXTRA_INFO)	+= sec_debug_extra_info.o
obj-$(CONFIG_SEC_DEBUG_HW_PARAM)	+= sec_debug_hw_param.o
obj-$(CONFIG_SEC_DEBUG_AUTO_COMMENT)	+= sec_debug_auto_comment.o
#obj-$(CONFIG_SEC_DEBUG_DUMP_SUMMARY)	+= sec_debug_dump_summary.o
#obj-$(CONFIG_SEC_DEBUG_DUMP_SUMMARY)	+= sec_debug_summary.o
obj-$(CONFIG_SEC_DEBUG_LAST_KMSG)	+= sec_debug_last_kmsg.o
obj-$(CONFIG_SEC_DEBUG_FIRST_KMSG)	+= sec_debug_first_kmsg.o
obj-$(CONFIG_SEC_DEBUG_DTASK)		+= sec_debug_dtask.o
obj-$(CONFIG_SEC_BOOTSTAT)		+= sec_bootstat.o

# Samsung platform watchdog
obj-$(CONFIG_SEC_DEBUG_PWDT)		+= sec_debug_platform_watchdog.o
