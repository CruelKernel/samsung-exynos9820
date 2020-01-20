#
# Makefile for TDMB Device Drivers.
#

ccflags-y += -Idrivers/media/tdmb
#ccflags-y += -v

obj-$(CONFIG_TDMB) += tdmb.o tdmb_data.o

# FC8080
ccflags-$(CONFIG_TDMB_FC8080) += -Idrivers/media/tdmb/fc8080
obj-$(CONFIG_TDMB_FC8080) += tdmb_port_fc8080.o
obj-$(CONFIG_TDMB_FC8080) += fc8080/
# MTV319
ccflags-$(CONFIG_TDMB_MTV319) += -Idrivers/media/tdmb/mtv319
obj-$(CONFIG_TDMB_MTV319) += tdmb_port_mtv319.o
obj-$(CONFIG_TDMB_MTV319) += mtv319/
# i/f
obj-$(CONFIG_TDMB_SPI) += tdmb_spi.o
obj-$(CONFIG_TDMB_EBI) += tdmb_ebi.o
obj-$(CONFIG_TDMB_I2C) += tdmb_i2c.o
obj-$(CONFIG_TDMB_TSIF_SLSI) += tdmb_tsi_slsi.o
obj-$(CONFIG_TDMB_TSIF_QC) += tdmb_tsi_qc.o

obj-$(CONFIG_TDMB_DEBUG) += tdmb_debug.o
obj-$(CONFIG_TDMB_NOTIFIER)	+= tdmb_notifier.o