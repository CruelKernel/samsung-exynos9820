# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/exynos9820.dtb
dtbo-y += samsung/exynos9820-beyondx_kor_08.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_06.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_00.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_03.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_04.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_07.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_01.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_02.dtbo
dtbo-y += samsung/exynos9820-beyondx_kor_05.dtbo

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
