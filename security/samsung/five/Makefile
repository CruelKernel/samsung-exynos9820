
obj-$(CONFIG_FIVE) += task_integrity.o
obj-$(CONFIG_FIVE) += five.o
obj-$(CONFIG_FIVE_PA_FEATURE) += five_pa.o
obj-$(CONFIG_FIVE_TEE_DRIVER) += five_tee_interface.o

EXTRA_CFLAGS += -I$(src)
asflags-y += -Isecurity/integrity/five
asflags-y += -Isecurity/samsung/five
ccflags-y += -I$(srctree)
ccflags-y += -Wformat

five-y := five_lv.o five_cert.o five_keyring.o five_init.o five_main.o \
	  five_crypto.o five_audit.o five_appraise.o five_state.o \
	  five_cert_builtin.o five_hooks.o five_cache.o five_dmverity.o
five-$(CONFIG_SECURITY_DSMS) += five_dsms.o
