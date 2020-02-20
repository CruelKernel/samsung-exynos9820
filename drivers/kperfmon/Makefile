#
# Makefile for the Linux kernel device drivers.
#
# 15 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

ifeq (existexist, $(shell [ -e $(srctree)/include/linux/olog.pb.h ] && echo exist)$(shell [ -e $(srctree)/../../system/core/liblog/include/log/perflog.h ] && echo exist))
	obj-y				+= kperfmon.o
else
	obj-y				+= ologk.o
$(info kperfmon_DUMMY="$(shell cp -f $(srctree)/drivers/kperfmon/ologk.h $(srctree)/include/linux/)")
endif
