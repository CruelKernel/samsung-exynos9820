subdir-ccflags-y += -I$(srctree)/drivers/misc/tzdev/include
subdir-ccflags-y += -I$(srctree)/drivers/misc/tzdev
ccflags-$(CONFIG_TZDEV_DEPLOY_TZAR) += -D"_STR(s)=\#s" -D"KBUILD_SRC=_STR($(KBUILD_SRC))"

obj-$(CONFIG_TZDEV)			+= lib/
obj-$(CONFIG_TZDEV)			+= teec/

obj-$(CONFIG_TZDEV)			+= sysdep.o
obj-$(CONFIG_TZDEV)			+= tz_cdev.o
obj-$(CONFIG_TZDEV)			+= tz_cred.o
obj-$(CONFIG_TZDEV)			+= tz_iwio.o
obj-$(CONFIG_TZDEV)			+= tz_iwservice.o
obj-$(CONFIG_TZDEV)			+= tz_iwsock.o
obj-$(CONFIG_TZDEV)			+= tz_mem.o
obj-$(CONFIG_TZDEV)			+= tz_platform.o
obj-$(CONFIG_TZDEV)			+= tz_ree_time.o
obj-$(CONFIG_TZDEV)			+= tz_uiwsock.o
obj-$(CONFIG_TZDEV)			+= tzdev.o

obj-$(CONFIG_TZDEV_KTHREADS_NOTIFIER)	+= tz_kthread_pool_notifier.o
obj-$(CONFIG_TZDEV_KTHREADS_SMPBOOT)	+= tz_kthread_pool_smpboot.o

ifeq ($(CONFIG_SOC_EXYNOS9820), y)
obj-$(CONFIG_ION_FD2PHYS)		+= ion_fd2phys_v1.o
ccflags-y += -DTZDEV_BOOST_CLUSTER_2
else
obj-$(CONFIG_ION_FD2PHYS)		+= ion_fd2phys.o
ccflags-y += -DTZDEV_BOOST_CLUSTER_1
endif
obj-$(CONFIG_MSM_SCM)			+= tz_msm_platform.o
obj-$(CONFIG_TZDEV_BOOST)		+= tz_boost.o
obj-$(CONFIG_TZDEV_CMA)			+= tz_cma.o
obj-$(CONFIG_TZDEV_DEPLOY_TZAR)		+= tz_deploy_tzar.o
obj-$(CONFIG_TZDEV_HOTPLUG)		+= tz_hotplug.o
obj-$(CONFIG_TZIRS)			+= tzirs.o
obj-$(CONFIG_TZLOG)			+= tz_iwlog.o
obj-$(CONFIG_TZLOG_POLLING)		+= tz_iwlog_polling.o
obj-$(CONFIG_TZPROFILER)		+= tzprofiler.o
obj-$(CONFIG_TZTUI)			+= tztui_fts.o
obj-$(CONFIG_TZ_BOOT_LOG)		+= tz_iw_boot_log.o
obj-$(CONFIG_TZ_NWFS)			+= tz_fsdev.o
obj-$(CONFIG_TZ_PANIC_DUMP)		+= tz_panic_dump.o
obj-$(CONFIG_TZ_TELEMETRY)		+= tz_telemetry.o
obj-$(CONFIG_TZ_TRANSPORT)		+= tz_transport.o

obj-$(CONFIG_TZDEV_DEBUG)		+= tests/
