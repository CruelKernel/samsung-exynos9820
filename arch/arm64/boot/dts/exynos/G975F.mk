# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos9820.dtb
dtbo-y += exynos9820-beyond2lte_eur_open_04.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_16.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_17.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_18.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_19.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_20.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_23.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_24.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_25.dtbo
dtbo-y += exynos9820-beyond2lte_eur_open_26.dtbo

always            := $(dtb-y) $(dtbo-y)
clean-files       := *.dtb*
