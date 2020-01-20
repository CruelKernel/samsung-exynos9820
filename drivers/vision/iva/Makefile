ifdef CONFIG_IVA_FPGA
obj-y := ext_bootmem.o
endif

obj-$(CONFIG_EXYNOS_IVA) := iva_ctl.o

ccflags-y += -Idrivers/misc/

iva_ctl-objs=\
	iva_ctrl.o\
	iva_pmu.o\
	iva_mcu.o\
	iva_sh_mem.o\
	iva_ram_dump.o\
	iva_mbox.o\
	iva_ipc_queue.o\
	iva_mem.o\
	iva_sysfs.o\
	iva_rt_table.o

ifdef CONFIG_SOC_EXYNOS8895
iva_ctl-objs += iva_vdma.o
endif
ifdef CONFIG_SOC_EXYNOS9810
iva_ctl-objs += iva_vdma20.o
endif
ifdef CONFIG_SOC_EXYNOS9820
iva_ctl-objs += iva_vdma30.o
endif
ifdef CONFIG_IVA_FPGA
ifdef CONFIG_SOC_EXYNOS9810
iva_ctl-objs +=\
	iva_mem_sync.o\
	iva_vdma20.o
endif
ifdef CONFIG_SOC_EXYNOS9820
iva_ctl-objs +=\
	iva_mem_sync.o\
	iva_vdma30.o
endif
endif

obj-ko := $(patsubst %.o,%.ko,$(obj-m))
PWD := $(shell pwd)
KERNELDIR ?=/lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -rf $(obj-ko) *.o *.mod.c modules.order Module.symvers .*.ko.cmd .*.mod.o.cmd .*.o.cmd .*.o.d Module.markers .tmp_versions

