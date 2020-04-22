# SPDX-License-Identifier: GPL-2.0
dtbo-y += samsung/exynos9820-d1x_kor_21.dtbo
dtbo-y += samsung/exynos9820-d1x_kor_23.dtbo
dtbo-y += samsung/exynos9820-d1x_kor_18.dtbo
dtbo-y += samsung/exynos9820-d1x_kor_19.dtbo
dtbo-y += samsung/exynos9820-d1x_kor_22.dtbo
dtb-y += exynos/exynos9825-old.dtb

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
