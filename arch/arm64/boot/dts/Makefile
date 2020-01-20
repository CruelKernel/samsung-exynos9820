# SPDX-License-Identifier: GPL-2.0
dts-dirs += exynos

subdir-y          := $(dts-dirs)

always            := $(DTB_LIST)

targets += dtbs
dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files       := exynos/*.dtb *.dtb*
