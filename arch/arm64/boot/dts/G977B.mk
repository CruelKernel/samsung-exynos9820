# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/exynos9820.dtb
dtbo-y += samsung/exynos9820-beyondx_eur_open_05.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_06.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_08.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_03.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_04.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_00.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_01.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_02.dtbo
dtbo-y += samsung/exynos9820-beyondx_eur_open_07.dtbo

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
