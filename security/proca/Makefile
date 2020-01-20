$(obj)/proca_certificate-asn1.o: $(obj)/proca_certificate-asn1.c \
			       $(obj)/proca_certificate-asn1.h

obj-$(CONFIG_PROCA) += proca_identity.o proca_lsm.o proca_fcntl.o \
			       proca_task_descr.o proca_certificate-asn1.o \
			       proca_certificate.o proca_table.o

ccflags-y += -Isecurity/integrity/five/