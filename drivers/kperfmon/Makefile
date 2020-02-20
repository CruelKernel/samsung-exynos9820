#
# Makefile for the Linux kernel device drivers.
#
# Sep 2018, Binse Park <h22yap@gmail.com>
# Rewritten to use lists instead of if-statements.
#

FLAG=1

#$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/include/linux/olog.pb.h  $(srctree)/drivers/kperfmon/)")

ifneq ($(CONFIG_KPERFMON), y)
FLAG=0
$(info kperfmon_DUMMY="CONFIG_KPERFMON is off.")
endif

ifneq ($(shell [ -e $(srctree)/include/linux/olog.pb.h ] && echo exist), exist)
$(info kperfmon_DUMMY="olog.pb.h file is missing... retrying")

$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/../../frameworks/base/proto/src/olog.proto  $(srctree)/drivers/kperfmon/)")
$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/../../vendor/samsung/system/libperflog/aprotoc  $(srctree)/drivers/kperfmon/)")
$(info kperfmon_DUMMY="$(shell $(srctree)/drivers/kperfmon/aprotoc --perflog_out=./  $(srctree)/drivers/kperfmon/olog.proto)")
$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/drivers/kperfmon/olog.pb.h  $(srctree)/include/linux/)")
#$(info kperfmon_DUMMY="$(shell ls $(srctree)/drivers/kperfmon/*)")
#$(info kperfmon_DUMMY="$(shell ls $(srctree)/include/linux/olog*)")

ifneq ($(shell [ -e $(srctree)/include/linux/olog.pb.h ] && echo exist), exist)
$(info kperfmon_DUMMY="olog.pb.h file is missing... again")
FLAG=0
endif
endif

ifneq ($(shell [ -e $(srctree)/../../system/core/liblog/include/log/perflog.h ] && echo exist), exist)
FLAG=0
$(info kperfmon_DUMMY="perflog.h file is missing.")
endif

$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/../../system/core/liblog/include/log/perflog.h  $(srctree)/include/linux/)")
ifeq ($(FLAG), 1)
	obj-y				+= kperfmon.o
else
	obj-y				+= ologk.o
$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/drivers/kperfmon/ologk.h $(srctree)/include/linux/)")
endif
