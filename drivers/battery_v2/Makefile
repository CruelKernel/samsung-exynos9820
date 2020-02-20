ifeq ($(CONFIG_BATTERY_SAMSUNG_MHS), y)
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery_mhs.o sec_battery_sysfs.o sec_battery_dt.o
else
obj-$(CONFIG_BATTERY_SAMSUNG)	+= sec_battery.o sec_battery_sysfs.o sec_battery_dt.o
endif
obj-$(CONFIG_OF)		+= sec_adc.o
obj-$(CONFIG_BATTERY_CISD)	+= sec_cisd.o
obj-$(CONFIG_MULTI_CHARGING)	+= sec_multi_charger.o
obj-$(CONFIG_STEP_CHARGING)	+= sec_step_charging.o
obj-$(CONFIG_WIRELESS_AUTH)	+= sec_battery_misc.o
obj-$(CONFIG_DIRECT_CHARGING)	+= sec_direct_charger.o
obj-$(CONFIG_UPDATE_BATTERY_DATA)	+= sec_battery_data.o
obj-$(CONFIG_BATTERY_NOTIFIER)	+= battery_notifier.o

obj-$(CONFIG_FUELGAUGE_MAX17042)	+= max17042_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX17048)	+= max17048_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX17050)	+= max17050_fuelgauge.o sec_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77823)	+= max77823_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77843)	+= max77843_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77833)	+= max77833_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77854)	+= max77854_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77865)	+= max77865_fuelgauge.o
obj-$(CONFIG_FUELGAUGE_MAX77705)	+= max77705_fuelgauge.o

obj-$(CONFIG_CHARGER_MAX8903)	+= max8903_charger.o
obj-$(CONFIG_CHARGER_SMB328)	+= smb328_charger.o
obj-$(CONFIG_CHARGER_SMB347)	+= smb347_charger.o sec_charger.o
obj-$(CONFIG_CHARGER_BQ24157)	+= bq24157_charger.o sec_charger.o
obj-$(CONFIG_CHARGER_BQ24190)	+= bq24190_charger.o
obj-$(CONFIG_CHARGER_BQ24191)	+= bq24190_charger.o
obj-$(CONFIG_CHARGER_BQ24260)	+= bq24260_charger.o sec_charger.o
obj-$(CONFIG_CHARGER_MAX77804)  += max77804_charger.o
obj-$(CONFIG_CHARGER_MAX77693)	+= max77693_charger.o
obj-$(CONFIG_CHARGER_MAX77823)	+= max77823_charger.o
obj-$(CONFIG_CHARGER_MAX77843)	+= max77843_charger.o
obj-$(CONFIG_CHARGER_MAX77888)	+= max77888_charger.o
obj-$(CONFIG_CHARGER_MAX77833)	+= max77833_charger.o
obj-$(CONFIG_CHARGER_MAX77854)	+= max77854_charger.o
obj-$(CONFIG_CHARGER_MAX77865)	+= max77865_charger.o
obj-$(CONFIG_CHARGER_MAX77705)	+= max77705_charger.o
obj-$(CONFIG_CHARGER_S2MU003)   += s2mu003_charger.o
obj-$(CONFIG_CHARGER_DA9155)    += da9155_charger.o

obj-$(CONFIG_WIRELESS_CHARGER_HAL_MFC)	+= mfc_hal_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_BQ51221)	+= bq51221_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_P9220)	+= p9220_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_MFC)	+= mfc_charger.o
obj-$(CONFIG_WIRELESS_CHARGER_MFC_S2MIW04)	+= mfc_s2miw04_charger.o

obj-$(CONFIG_CHARGER_PCA9468)	+= pca9468_charger.o
