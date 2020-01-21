#
# Makefile for the Linux kernel device drivers.
#
# Sep 2018, Binse Park <h22yap@gmail.com>
# Rewritten to use lists instead of if-statements.
#

FLAG=0

ifeq ($(CONFIG_KPERFMON), y)
FLAG=1
endif

ifeq ($(FLAG), 1)
ifneq ($(shell [ -e $(srctree)/include/linux/olog.pb.h ] && echo exist), exist)
FLAG=0
$(warning "kperfmon: olog.pb.h file is missing.")
endif

ifneq ($(shell [ -e $(srctree)/include/linux/perflog.h ] && echo exist), exist)
FLAG=0
$(warning "kperfmon: perflog.h file is missing.")
endif
endif

ifeq ($(FLAG), 1)
	obj-y				+= kperfmon.o
else
	obj-y				+= ologk.o
endif
