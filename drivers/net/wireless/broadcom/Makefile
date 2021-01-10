obj-$(CONFIG_B43)		+= b43/
obj-$(CONFIG_B43LEGACY)		+= b43legacy/

obj-$(CONFIG_BRCMFMAC)	+= brcm80211/
obj-$(CONFIG_BRCMSMAC)	+= brcm80211/

ifeq ($(CONFIG_BCM4375),y)
ifneq ($(PLATFORM_VERSION),)
ifeq ($(shell expr $(PLATFORM_VERSION) \>= 11),1)
obj-$(CONFIG_BCM4375)   += bcmdhd_101_16/
obj-$(CONFIG_SOC_EXYNOS9820) += bcmdhd_101_16/dhd_custom_memprealloc.o
obj-$(CONFIG_SOC_EXYNOS9820) += bcmdhd_101_16/dhd_custom_exynos.o
else
obj-$(CONFIG_BCM4375)   += bcmdhd_100_15/
obj-$(CONFIG_SOC_EXYNOS9820) += bcmdhd_100_15/dhd_custom_memprealloc.o
obj-$(CONFIG_SOC_EXYNOS9820) += bcmdhd_100_15/dhd_custom_exynos.o
endif
endif
endif