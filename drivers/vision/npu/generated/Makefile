obj-$(CONFIG_EXYNOS_NPU_PUBLISH_NPU_BUILD_VER)	+= npu-ver-info.o

drivers/vision/npu/generated/npu-ver-info.c:	FORCE
	(drivers/vision/npu/generated/update_ver_info.sh \
	|| cp drivers/vision/npu/generated/npu-ver-info-default.c  drivers/vision/npu/generated/npu-ver-info~) 2>/dev/null
