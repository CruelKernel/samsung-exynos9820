# SPDX-License-Identifier: GPL-2.0
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_25.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_16.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_04.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_18.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_17.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_26.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_20.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_23.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_19.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_eur_open_24.dtbo
dtb-y += exynos/exynos9820.dtb

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
