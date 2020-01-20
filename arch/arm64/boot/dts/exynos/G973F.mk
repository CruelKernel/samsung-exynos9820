# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos9820.dtb
dtbo-y += exynos9820-beyond1lte_eur_open_17.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_18.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_19.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_20.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_21.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_22.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_23.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_24.dtbo
dtbo-y += exynos9820-beyond1lte_eur_open_26.dtbo

always            := $(dtb-y) $(dtbo-y)
clean-files       := *.dtb*
