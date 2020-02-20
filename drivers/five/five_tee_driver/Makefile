#
# Integrity TEE Driver
#
TEEC_CLIENT_PATH := multibuild/source/gp-api/client
PROTOCOL_PATH := multibuild/source/gp-api/protocol

obj-$(CONFIG_FIVE_TEE_DRIVER) += itee_driver.o

obj-$(CONFIG_FIVE_USE_TRUSTONIC) += $(TEEC_CLIENT_PATH)/teec_common_tbase.o

itee_driver-objs += five_tee_driver.o

ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Isecurity/samsung/five \
                                -Idrivers/five/five_tee_driver/$(PROTOCOL_PATH) \
                                -Idrivers/five/five_tee_driver/$(TEEC_CLIENT_PATH)

ccflags-$(CONFIG_FIVE_USE_TRUSTONIC) += -DCONFIG_TRUSTONIC
ccflags-$(CONFIG_FIVE_USE_TZDEV) += -DCONFIG_TEEGRIS

ifeq ($(CONFIG_FIVE_USE_TRUSTONIC), y)
  ifeq ($(CONFIG_SOC_EXYNOS9820), y)
    PLATFORM := exynos9820
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Idrivers/gud/gud-$(PLATFORM)/MobiCoreDriver
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -Idrivers/gud/gud-$(PLATFORM)/MobiCoreDriver/public
  else
    $(error "Unknown SoC")
  endif
endif

ccflags-$(CONFIG_FIVE_USE_TRUSTONIC) += -Idrivers/five/five_tee_driver/multibuild/include/gp-api
ccflags-$(CONFIG_FIVE_USE_TZDEV) += -Idrivers/misc/tzdev/include/tzdev
