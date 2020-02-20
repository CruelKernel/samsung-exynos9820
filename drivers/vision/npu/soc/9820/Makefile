obj-y		                        += npu-system-soc.o

ccflags-y += -Idrivers/vision/include -Idrivers/vision/npu/include -Idrivers/vision/npu
ifdef CONFIG_NPU_LOOPBACK
ccflags-y += -Idrivers/vision/npu/interface/loopback
else
ccflags-y += -Idrivers/vision/npu/interface/hardware
endif

# SoC specific inclusion
ccflags-y += -Idrivers/vision/npu/soc/$(CONFIG_EXYNOS_SOC_NAME)/include
