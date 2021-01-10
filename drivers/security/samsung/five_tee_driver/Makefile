#
# Integrity TEE Driver
#
FIVE_TEE_DRIVER_PATH := $(srctree)/drivers/security/samsung/five_tee_driver
TEEC_CLIENT_PATH := multibuild/source/gp-api/client
PROTOCOL_PATH := multibuild/source/gp-api/protocol

obj-$(CONFIG_FIVE_TEE_DRIVER) += five_tee_driver.o

ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -I$(srctree)/security/samsung/five \
                                     -I$(FIVE_TEE_DRIVER_PATH)/$(PROTOCOL_PATH) \
                                     -I$(FIVE_TEE_DRIVER_PATH)/$(TEEC_CLIENT_PATH)

# Emulator
obj-$(CONFIG_FIVE_USE_EMULATOR) += $(TEEC_CLIENT_PATH)/tee_client_api.o \
                                   $(PROTOCOL_PATH)/teec_param_utils.o \
                                   $(TEEC_CLIENT_PATH)/teec_common_emulator.o \
                                   $(TEEC_CLIENT_PATH)/teec_operation.o \
                                   $(PROTOCOL_PATH)/socket_utils.o
ccflags-$(CONFIG_FIVE_USE_EMULATOR) += -I$(FIVE_TEE_DRIVER_PATH)/multibuild/include/gp-api

# Trustonic
obj-$(CONFIG_FIVE_USE_TRUSTONIC) += $(TEEC_CLIENT_PATH)/tee_client_api.o \
                                    $(PROTOCOL_PATH)/teec_param_utils.o \
                                    $(TEEC_CLIENT_PATH)/teec_common_tbase.o \
                                    $(TEEC_CLIENT_PATH)/teec_operation.o

ccflags-$(CONFIG_FIVE_USE_TRUSTONIC) += -I$(FIVE_TEE_DRIVER_PATH)/multibuild/include/gp-api
ifeq ($(CONFIG_FIVE_USE_TRUSTONIC), y)
  ifeq ($(CONFIG_MACH_MT6768), y)
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -I$(srctree)/drivers/misc/mediatek/gud/410/MobiCoreDriver
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -I$(srctree)/drivers/misc/mediatek/gud/410/MobiCoreDriver/public
  else
    ifeq ($(CONFIG_SOC_EXYNOS7570), y)
      PLATFORM := exynos7570
    else ifeq ($(CONFIG_SOC_EXYNOS7870), y)
      PLATFORM := exynos7870
    else ifeq ($(CONFIG_SOC_EXYNOS7885), y)
      PLATFORM := exynos7885
    else ifeq ($(CONFIG_SOC_EXYNOS9810), y)
      PLATFORM := exynos9810
    else ifeq ($(CONFIG_SOC_EXYNOS9820), y)
      PLATFORM := exynos9820
    else ifeq ($(CONFIG_SOC_EXYNOS9610), y)
      PLATFORM := exynos9610
    else ifeq ($(CONFIG_SOC_EXYNOS1000), y)
      PLATFORM := exynos1000
    else
      $(error "Unknown SoC")
    endif
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -I$(srctree)/drivers/gud/gud-$(PLATFORM)/MobiCoreDriver
    ccflags-$(CONFIG_FIVE_TEE_DRIVER) += -I$(srctree)/drivers/gud/gud-$(PLATFORM)/MobiCoreDriver/public
  endif # CONFIG_MACH_MT6768
endif # CONFIG_FIVE_USE_TRUSTONIC

# QSEE
obj-$(CONFIG_FIVE_USE_QSEE) += $(TEEC_CLIENT_PATH)/tee_client_api.o \
                               $(PROTOCOL_PATH)/teec_param_utils.o \
                               $(TEEC_CLIENT_PATH)/teec_common_qsee.o \
                               $(TEEC_CLIENT_PATH)/teec_operation.o

ccflags-$(CONFIG_FIVE_USE_QSEE) += -I$(FIVE_TEE_DRIVER_PATH)/multibuild/include/gp-api \
                                   -I$(srctree)/drivers/misc
aflags-$(CONFIG_FIVE_USE_QSEE) += -I$(srctree)

# Teegris
ifeq ($(CONFIG_SOC_EXYNOS7570), y)
  use_teegris_v2 := y
endif

ifeq ($(CONFIG_SOC_EXYNOS7870), y)
  use_teegris_v2 := y
endif

ifeq ($(CONFIG_SOC_EXYNOS7885), y)
  ifeq ($(CONFIG_TEEGRIS_VERSION), 2)
    use_teegris_v2 := y
  endif
  ifeq ($(CONFIG_TEEGRIS_VERSION), 3)
    ccflags-$(CONFIG_FIVE_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev/3.0/include/tzdev
  endif
endif

ifeq ($(use_teegris_v2), y)
  obj-$(CONFIG_FIVE_USE_TZDEV) += $(TEEC_CLIENT_PATH)/teec_common_teegris_v2.o \
                                  $(TEEC_CLIENT_PATH)/tee_client_api.o \
                                  $(PROTOCOL_PATH)/teec_param_utils.o \
                                  $(TEEC_CLIENT_PATH)/teec_operation.o
  ccflags-$(CONFIG_FIVE_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include \
                                      -I$(FIVE_TEE_DRIVER_PATH)/multibuild/include/gp-api
else
  obj-$(CONFIG_FIVE_USE_TZDEV) += $(TEEC_CLIENT_PATH)/teec_operation_teegris_v3.o
  ccflags-$(CONFIG_FIVE_USE_TZDEV) += -I$(srctree)/drivers/misc/tzdev/include/tzdev
endif

subdir-ccflags-y := -Wformat
