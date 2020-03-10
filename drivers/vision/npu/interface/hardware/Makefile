obj-y				+= npu-interface.o
#obj-y				+= npu-mailbox.o
obj-y				+= mailbox_ipc.o

ccflags-y += -Idrivers/vision/include -Idrivers/vision/npu/include -Idrivers/vision/npu -Idrivers/vision/npu/interface/hardware

# SoC specific inclusion
ccflags-y += -Idrivers/vision/npu/soc/$(CONFIG_EXYNOS_SOC_NAME)/include
