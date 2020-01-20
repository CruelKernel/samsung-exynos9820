#
# Copyright (c) 2018 Samsung Electronics Co., Ltd.
#		http://www.samsung.com
#
# Licensed under GPLv2
#

str-objs := camerapp-str-core.o
str-objs += camerapp-str-video.o
str-objs += camerapp-str-hw.o
str-objs += camerapp-str-cal.o

obj-$(CONFIG_VIDEO_EXYNOS_CAMERA_POSTPROCESS_STR)	+= str.o
obj-y							+= sfr/

ccflags-y += -Idrivers/media/platform/exynos/camera-pp
ccflags-y += -Idrivers/media/platform/exynos/camera-pp/str
ccflags-y += -Idrivers/media/platform/exynos/camera-pp/str/sfr
ccflags-y += -Idrivers/media/platform/exynos/camera-pp/str/param
