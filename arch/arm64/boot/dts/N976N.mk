# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/exynos9825.dtb
dtbo-y += samsung/exynos9820-d2x_kor_24.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_17.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_19.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_02.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_16.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_21.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_22.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_18.dtbo
dtbo-y += samsung/exynos9820-d2x_kor_23.dtbo

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
