#
# Copyright (c) 2016 Samsung Electronics Co., Ltd.
#		http://www.samsung.com
#
# Licensed under GPLv2
#

gdc-objs := camerapp-gdc-core.o \
		camerapp-gdc-grid.o

obj-$(CONFIG_VIDEO_EXYNOS_CAMERA_POSTPROCESS_GDC)	+= gdc.o
obj-$(CONFIG_CAMERA_PP_GDC_V1_0_0_OBJ)	+= camerapp-hw-api-gdc-v100.o
obj-$(CONFIG_CAMERA_PP_GDC_V2_0_0_OBJ)	+= camerapp-hw-api-gdc-v200.o
obj-$(CONFIG_CAMERA_PP_GDC_V2_1_0_OBJ)	+= camerapp-hw-api-gdc-v210.o

ccflags-y += -Idrivers/media/platform/exynos/camera-pp/
ccflags-y += -Idrivers/media/platform/exynos/fimc-is2/include
