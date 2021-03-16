# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/exynos9820.dtb
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_18.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_19.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_20.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_26.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_21.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_23.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_24.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_17.dtbo
dtbo-y += samsung/exynos9820-beyond1lte_eur_open_22.dtbo

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
