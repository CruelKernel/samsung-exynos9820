# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the input core drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_INPUT)		+= input-core.o
input-core-y := input.o input-compat.o input-mt.o ff-core.o

obj-$(CONFIG_INPUT_TOUCHSCREEN)	+= sec_cmd.o
obj-$(CONFIG_INPUT_TOUCHSCREEN_TCLMV2)	+= sec_tclm_v2.o

obj-$(CONFIG_INPUT_FF_MEMLESS)	+= ff-memless.o
obj-$(CONFIG_INPUT_POLLDEV)	+= input-polldev.o
obj-$(CONFIG_INPUT_SPARSEKMAP)	+= sparse-keymap.o
obj-$(CONFIG_INPUT_MATRIXKMAP)	+= matrix-keymap.o

obj-$(CONFIG_INPUT_LEDS)	+= input-leds.o
obj-$(CONFIG_INPUT_MOUSEDEV)	+= mousedev.o
obj-$(CONFIG_INPUT_JOYDEV)	+= joydev.o
obj-$(CONFIG_INPUT_EVDEV)	+= evdev.o
obj-$(CONFIG_SEC_INPUT_BOOSTER)	+= input_booster.o
obj-$(CONFIG_ARCH_QCOM)	+= input_booster_qc.o
obj-$(CONFIG_ARCH_EXYNOS)	+= input_booster_lsi.o
obj-$(CONFIG_INPUT_EVBUG)	+= evbug.o

obj-$(CONFIG_INPUT_KEYBOARD)	+= keyboard/
obj-$(CONFIG_INPUT_MOUSE)	+= mouse/
obj-$(CONFIG_INPUT_JOYSTICK)	+= joystick/
obj-$(CONFIG_INPUT_TABLET)	+= tablet/
obj-$(CONFIG_INPUT_TOUCHSCREEN)	+= touchscreen/
obj-$(CONFIG_INPUT_MISC)	+= misc/

obj-$(CONFIG_INPUT_APMPOWER)	+= apm-power.o
obj-$(CONFIG_INPUT_KEYRESET)	+= keyreset.o
obj-$(CONFIG_INPUT_KEYCOMBO)	+= keycombo.o
obj-$(CONFIG_TOUCHSCREEN_DUMP_MODE)	+= sec_tsp_dumpkey.o

obj-$(CONFIG_HALL)		+= hall.o
obj-$(CONFIG_CERTIFY_HALL)	+= certify_hall.o

obj-$(CONFIG_RMI4_CORE)		+= rmi4/

obj-$(CONFIG_INPUT_WACOM)	+= wacom/
