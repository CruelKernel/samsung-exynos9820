# SPDX-License-Identifier: GPL-2.0
dtb-y += exynos/exynos9820.dtb
dtbo-y += samsung/exynos9820-beyond2lte_kor_17.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_kor_18.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_kor_19.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_kor_26.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_kor_25.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_kor_20.dtbo
dtbo-y += samsung/exynos9820-beyond2lte_kor_24.dtbo

targets += dtbs
DTB_LIST  := $(dtb-y) $(dtbo-y)
always    := $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := *.dtb*
