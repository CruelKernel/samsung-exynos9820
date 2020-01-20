ifeq ($(CONFIG_EXYNOS_COMMON_PANEL),y)
	obj-$(CONFIG_EXYNOS_COMMON_PANEL) += common_mipi_lcd.o
else
	obj-$(CONFIG_EXYNOS_DECON_LCD_S6E3HA2K)	+= s6e3ha2k_mipi_lcd.o s6e3ha2k_lcd_ctrl.o
	obj-$(CONFIG_EXYNOS_DECON_LCD_S6E3HF4)	+= s6e3hf4_mipi_lcd.o s6e3hf4_lcd_ctrl.o
	obj-$(CONFIG_EXYNOS_DECON_LCD_S6E3HA6)	+= s6e3ha6_mipi_lcd.o s6e3ha6_lcd_ctrl.o
	obj-$(CONFIG_EXYNOS_DECON_LCD_S6E3HA8)	+= s6e3ha8_mipi_lcd.o s6e3ha8_lcd_ctrl.o
	obj-$(CONFIG_EXYNOS_DECON_LCD_S6E3HA9)	+= s6e3ha9_mipi_lcd.o s6e3ha9_lcd_ctrl.o
	obj-$(CONFIG_EXYNOS_DECON_LCD_S6E3AA2)	+= s6e3aa2_mipi_lcd.o s6e3aa2_lcd_ctrl.o
	obj-$(CONFIG_EXYNOS_DECON_LCD_EMUL_DISP) += emul_disp_mipi_lcd.o emul_disp_lcd_ctrl.o
endif
