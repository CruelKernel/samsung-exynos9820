# SPDX-License-Identifier: GPL-2.0
dtb-$(CONFIG_ARCH_EXYNOS) += \
	exynos5433-tm2.dtb	\
	exynos5433-tm2e.dtb	\
	exynos7-espresso.dtb

dtb-$(CONFIG_SOC_EXYNOS9810) += \
	exynos9810.dtb

dtbo-$(CONFIG_SOC_EXYNOS9810) += \
	exynos9810-universal9810-2.dtbo \
	exynos9810-universal9810-2-rev02.dtbo

dtb-$(CONFIG_SOC_EMULATOR9820) += exynos9820-emulator9820.dtb

ifeq ($(CONFIG_SOC_EXYNOS9820_EVT0), y)
dtb-$(CONFIG_SOC_EXYNOS9820) += \
	exynos9820_evt0.dtb
dtb-$(CONFIG_SOC_EXYNOS9820) += \
	exynos9820_evt0-smdk9820.dtbo \
	exynos9820_evt0-universal9820_rev0.dtbo \
	exynos9820_evt0-universal9820_rev1.dtbo \
	exynos9820_evt0-universal9820_rev2.dtbo \
	exynos9820_evt0-universal9820_rev3.dtbo \
	exynos9820_evt0-universal9820_rev4.dtbo \
	exynos9820_evt0-universal9820_s5100_rev0.dtbo

else
dtb-$(CONFIG_SOC_EXYNOS9820) += \
	exynos9820.dtb
dtb-$(CONFIG_SOC_EXYNOS9820) += \
	exynos9820-smdk9820.dtbo \
	exynos9820-universal9820_rev5.dtbo \
	exynos9820-universal9820_revC01.dtbo \
	exynos9820-universal9820_revC03.dtbo \
	exynos9820-universal9820_revC04.dtbo \
	exynos9820-universal9820_revC06.dtbo \
	exynos9820-universal9820_5g.dtbo \
	exynos9820-universal9820_s5100_rev1.dtbo
endif

# always		:= $(dtb-y) $(dtbo-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb *.dtbo
