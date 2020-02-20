#
# Makefile for the sensor drivers.
#

# Each configuration option enables a list of files.
obj-$(CONFIG_SENSORS_SSP)		+= ssp_dev.o ssp_i2c.o ssp_data.o ssp_sysfs.o\
						ssp_firmware.o ssp_debug.o ssp_dump.o
obj-$(CONFIG_SENSORS_SYSFS)		+= sensors_core.o

obj-$(CONFIG_IIO)			+= ssp_iio.o ssp_iio_ring.o

obj-$(CONFIG_SENSORS_SSP_ICM20610)	+= factory/accel_icm20610.o factory/gyro_icm20610.o
obj-$(CONFIG_SENSORS_SSP_MPU6500)	+= factory/accel_mpu6500.o factory/gyro_mpu6500.o
obj-$(CONFIG_SENSORS_SSP_BMI058)	+= factory/accel_bmi058.o factory/gyro_bmi058.o
obj-$(CONFIG_SENSORS_SSP_K6DS3TR)	+= factory/accel_k6ds3tr.o factory/gyro_k6ds3tr.o
obj-$(CONFIG_SENSORS_SSP_LSM6DSL)	+= factory/accel_lsm6dsl.o factory/gyro_lsm6dsl.o
obj-$(CONFIG_SENSORS_SSP_LSM6DSO)	+= factory/accel_lsm6dsl.o factory/gyro_lsm6dsl.o

obj-$(CONFIG_SENSORS_SSP_YAS532)	+= factory/magnetic_yas532.o
obj-$(CONFIG_SENSORS_SSP_YAS537)	+= factory/magnetic_yas537.o
obj-$(CONFIG_SENSORS_SSP_YAS539)	+= factory/magnetic_yas539.o
obj-$(CONFIG_SENSORS_SSP_MAGNETIC_COMMON)	+= factory/magnetic_common.o
obj-$(CONFIG_SENSORS_SSP_AK09911)	+= factory/magnetic_ak09911.o

obj-$(CONFIG_SENSORS_SSP_TMG399x)	+= factory/light_tmg399x.o factory/gesture_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_TMD4903)	+= factory/light_tmg399x.o factory/gesture_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_TMD4905)	+= factory/light_tmg399x.o factory/gesture_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_TMD4904)	+= factory/light_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_TMD4906)	+= factory/light_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_TMD4910) += factory/light_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_TMD4907) += factory/light_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_CM3323)	+= factory/light_cm3323.o

obj-$(CONFIG_SENSORS_SSP_PROX_FACTORYCAL)	+= factory/prox_tmg399x.o
obj-$(CONFIG_SENSORS_SSP_PROX_AUTOCAL_AMS)	+= factory/prox_autocal_ams.o
obj-$(CONFIG_SENSORS_SSP_LIGHT_COLORID) += factory/light_colorid.o
obj-$(CONFIG_SENSORS_SSP_BMP182)	+= factory/pressure_bmp182.o
obj-$(CONFIG_SENSORS_SSP_LPS25H)	+= factory/pressure_lps25h.o
obj-$(CONFIG_SENSORS_SSP_LPS22H)	+= factory/pressure_lps22h.o
obj-$(CONFIG_SENSORS_SSP_SHTC1)		+= factory/temphumidity_shtc1.o

obj-$(CONFIG_SENSORS_SSP_STM32F401)	+= factory/mcu_stm32f401.o

obj-$(CONFIG_SENSORS_SSP_MOBEAM)	 += factory/barcode_emul_tmg3992.o
obj-$(CONFIG_SENSORS_SSP_IRLED)		+= factory/irled_tmd4903.o
obj-$(CONFIG_SENSORS_SSP_SX9306)	 += factory/grip_sx9306.o

obj-$(CONFIG_SENSORS_SSP_SENSORHUB)	+= ssp_sensorhub.o ssp_misc.o
obj-$(CONFIG_SENSORS_SX9310)		+= sx9310.o
obj-$(CONFIG_SENSORS_SX9320)		+= sx9320.o
obj-$(CONFIG_SENSORS_SX9330)		+= sx9330.o
obj-$(CONFIG_SENSORS_SX9360)		+= sx9360.o

obj-$(CONFIG_SENSORS_SSP_BBD)           += ssp_bbd.o factory/mcu_bcm4774.o
obj-$(CONFIG_SENSORS_SSP_BBD)           += bbdpl/
obj-$(CONFIG_SSP_MOTOR_CALLBACK)        += ssp_motorcallback.o
obj-$(CONFIG_SENSORS_SSP_THERMISTOR)        += factory/thermistor.o

