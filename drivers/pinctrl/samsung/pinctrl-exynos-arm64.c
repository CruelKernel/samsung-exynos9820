/*
 * Exynos ARMv8 specific support for Samsung pinctrl/gpiolib driver
 * with eint support.
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 * Copyright (c) 2012 Linaro Ltd
 *		http://www.linaro.org
 * Copyright (c) 2017 Krzysztof Kozlowski <krzk@kernel.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This file contains the Samsung Exynos specific information required by the
 * the Samsung pinctrl/gpiolib driver. It also includes the implementation of
 * external gpio and wakeup interrupt support.
 */

#include <linux/slab.h>
#include <linux/soc/samsung/exynos-regs-pmu.h>

#include "pinctrl-samsung.h"
#include "pinctrl-exynos.h"

static const struct samsung_pin_bank_type bank_type_off = {
	.fld_width = { 4, 1, 2, 2, 2, 2, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

static const struct samsung_pin_bank_type bank_type_alive = {
	.fld_width = { 4, 1, 2, 2, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};

/* Exynos5433 has the 4bit widths for PINCFG_TYPE_DRV bitfields. */
static const struct samsung_pin_bank_type exynos5433_bank_type_off = {
	.fld_width = { 4, 1, 2, 4, 2, 2, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

static const struct samsung_pin_bank_type exynos5433_bank_type_alive = {
	.fld_width = { 4, 1, 2, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};

/* bank type for non-alive type
 * (CON bit field: 4, DAT bit field: 1, PUD bit field: 4, DRV bit field: 4)
 * (CONPDN bit field: 2, PUDPDN bit field: 4)
 */
static struct samsung_pin_bank_type exynos9810_bank_type_off  = {
	.fld_width = { 4, 1, 4, 4, 2, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, },
};

/* bank type for alive type
 * (CON bit field: 4, DAT bit field: 1, PUD bit field: 4, DRV bit field: 4)
 */
static struct samsung_pin_bank_type exynos9810_bank_type_alive = {
	.fld_width = { 4, 1, 4, 4, },
	.reg_offset = { 0x00, 0x04, 0x08, 0x0c, },
};

/* Pad retention control code for accessing PMU regmap */
static atomic_t exynos_shared_retention_refcnt;

/* pin banks of exynos5433 pin-controller - ALIVE */
static const struct samsung_pin_bank_data exynos5433_pin_banks0[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTW(8, 0x000, "gpa0", 0x00),
	EXYNOS5433_PIN_BANK_EINTW(8, 0x020, "gpa1", 0x04),
	EXYNOS5433_PIN_BANK_EINTW(8, 0x040, "gpa2", 0x08),
	EXYNOS5433_PIN_BANK_EINTW(8, 0x060, "gpa3", 0x0c),
	EXYNOS5433_PIN_BANK_EINTW_EXT(8, 0x020, "gpf1", 0x1004, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(4, 0x040, "gpf2", 0x1008, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(4, 0x060, "gpf3", 0x100c, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(8, 0x080, "gpf4", 0x1010, 1),
	EXYNOS5433_PIN_BANK_EINTW_EXT(8, 0x0a0, "gpf5", 0x1014, 1),
};

/* pin banks of exynos5433 pin-controller - AUD */
static const struct samsung_pin_bank_data exynos5433_pin_banks1[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(7, 0x000, "gpz0", 0x00),
	EXYNOS5433_PIN_BANK_EINTG(4, 0x020, "gpz1", 0x04),
};

/* pin banks of exynos5433 pin-controller - CPIF */
static const struct samsung_pin_bank_data exynos5433_pin_banks2[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(2, 0x000, "gpv6", 0x00),
};

/* pin banks of exynos5433 pin-controller - eSE */
static const struct samsung_pin_bank_data exynos5433_pin_banks3[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(3, 0x000, "gpj2", 0x00),
};

/* pin banks of exynos5433 pin-controller - FINGER */
static const struct samsung_pin_bank_data exynos5433_pin_banks4[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(4, 0x000, "gpd5", 0x00),
};

/* pin banks of exynos5433 pin-controller - FSYS */
static const struct samsung_pin_bank_data exynos5433_pin_banks5[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(6, 0x000, "gph1", 0x00),
	EXYNOS5433_PIN_BANK_EINTG(7, 0x020, "gpr4", 0x04),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x040, "gpr0", 0x08),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x060, "gpr1", 0x0c),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x080, "gpr2", 0x10),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x0a0, "gpr3", 0x14),
};

/* pin banks of exynos5433 pin-controller - IMEM */
static const struct samsung_pin_bank_data exynos5433_pin_banks6[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(8, 0x000, "gpf0", 0x00),
};

/* pin banks of exynos5433 pin-controller - NFC */
static const struct samsung_pin_bank_data exynos5433_pin_banks7[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(3, 0x000, "gpj0", 0x00),
};

/* pin banks of exynos5433 pin-controller - PERIC */
static const struct samsung_pin_bank_data exynos5433_pin_banks8[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(6, 0x000, "gpv7", 0x00),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x020, "gpb0", 0x04),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x040, "gpc0", 0x08),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x060, "gpc1", 0x0c),
	EXYNOS5433_PIN_BANK_EINTG(6, 0x080, "gpc2", 0x10),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x0a0, "gpc3", 0x14),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x0c0, "gpg0", 0x18),
	EXYNOS5433_PIN_BANK_EINTG(4, 0x0e0, "gpd0", 0x1c),
	EXYNOS5433_PIN_BANK_EINTG(6, 0x100, "gpd1", 0x20),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x120, "gpd2", 0x24),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x140, "gpd4", 0x28),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x160, "gpd8", 0x2c),
	EXYNOS5433_PIN_BANK_EINTG(7, 0x180, "gpd6", 0x30),
	EXYNOS5433_PIN_BANK_EINTG(3, 0x1a0, "gpd7", 0x34),
	EXYNOS5433_PIN_BANK_EINTG(5, 0x1c0, "gpg1", 0x38),
	EXYNOS5433_PIN_BANK_EINTG(2, 0x1e0, "gpg2", 0x3c),
	EXYNOS5433_PIN_BANK_EINTG(8, 0x200, "gpg3", 0x40),
};

/* pin banks of exynos5433 pin-controller - TOUCH */
static const struct samsung_pin_bank_data exynos5433_pin_banks9[] __initconst = {
	EXYNOS5433_PIN_BANK_EINTG(3, 0x000, "gpj1", 0x00),
};

/* PMU pin retention groups registers for Exynos5433 (without audio & fsys) */
static const u32 exynos5433_retention_regs[] = {
	EXYNOS5433_PAD_RETENTION_TOP_OPTION,
	EXYNOS5433_PAD_RETENTION_UART_OPTION,
	EXYNOS5433_PAD_RETENTION_EBIA_OPTION,
	EXYNOS5433_PAD_RETENTION_EBIB_OPTION,
	EXYNOS5433_PAD_RETENTION_SPI_OPTION,
	EXYNOS5433_PAD_RETENTION_MIF_OPTION,
	EXYNOS5433_PAD_RETENTION_USBXTI_OPTION,
	EXYNOS5433_PAD_RETENTION_BOOTLDO_OPTION,
	EXYNOS5433_PAD_RETENTION_UFS_OPTION,
	EXYNOS5433_PAD_RETENTION_FSYSGENIO_OPTION,
};

static const struct samsung_retention_data exynos5433_retention_data __initconst = {
	.regs	 = exynos5433_retention_regs,
	.nr_regs = ARRAY_SIZE(exynos5433_retention_regs),
	.value	 = EXYNOS_WAKEUP_FROM_LOWPWR,
	.refcnt	 = &exynos_shared_retention_refcnt,
	.init	 = exynos_retention_init,
};

/* PMU retention control for audio pins can be tied to audio pin bank */
static const u32 exynos5433_audio_retention_regs[] = {
	EXYNOS5433_PAD_RETENTION_AUD_OPTION,
};

static const struct samsung_retention_data exynos5433_audio_retention_data __initconst = {
	.regs	 = exynos5433_audio_retention_regs,
	.nr_regs = ARRAY_SIZE(exynos5433_audio_retention_regs),
	.value	 = EXYNOS_WAKEUP_FROM_LOWPWR,
	.init	 = exynos_retention_init,
};

/* PMU retention control for mmc pins can be tied to fsys pin bank */
static const u32 exynos5433_fsys_retention_regs[] = {
	EXYNOS5433_PAD_RETENTION_MMC0_OPTION,
	EXYNOS5433_PAD_RETENTION_MMC1_OPTION,
	EXYNOS5433_PAD_RETENTION_MMC2_OPTION,
};

static const struct samsung_retention_data exynos5433_fsys_retention_data __initconst = {
	.regs	 = exynos5433_fsys_retention_regs,
	.nr_regs = ARRAY_SIZE(exynos5433_fsys_retention_regs),
	.value	 = EXYNOS_WAKEUP_FROM_LOWPWR,
	.init	 = exynos_retention_init,
};

/*
 * Samsung pinctrl driver data for Exynos5433 SoC. Exynos5433 SoC includes
 * ten gpio/pin-mux/pinconfig controllers.
 */
static const struct samsung_pin_ctrl exynos5433_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 data */
		.pin_banks	= exynos5433_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks0),
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.nr_ext_resources = 1,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 1 data */
		.pin_banks	= exynos5433_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks1),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_audio_retention_data,
	}, {
		/* pin-controller instance 2 data */
		.pin_banks	= exynos5433_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 3 data */
		.pin_banks	= exynos5433_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 4 data */
		.pin_banks	= exynos5433_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 5 data */
		.pin_banks	= exynos5433_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_fsys_retention_data,
	}, {
		/* pin-controller instance 6 data */
		.pin_banks	= exynos5433_pin_banks6,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 7 data */
		.pin_banks	= exynos5433_pin_banks7,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks7),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 8 data */
		.pin_banks	= exynos5433_pin_banks8,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks8),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	}, {
		/* pin-controller instance 9 data */
		.pin_banks	= exynos5433_pin_banks9,
		.nr_banks	= ARRAY_SIZE(exynos5433_pin_banks9),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
		.retention_data	= &exynos5433_retention_data,
	},
};

const struct samsung_pinctrl_of_match_data exynos5433_of_data __initconst = {
	.ctrl		= exynos5433_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos5433_pin_ctrl),
};

/* pin banks of exynos7 pin-controller - ALIVE */
static const struct samsung_pin_bank_data exynos7_pin_banks0[] __initconst = {
	EXYNOS_PIN_BANK_EINTW(8, 0x000, "gpa0", 0x00),
	EXYNOS_PIN_BANK_EINTW(8, 0x020, "gpa1", 0x04),
	EXYNOS_PIN_BANK_EINTW(8, 0x040, "gpa2", 0x08),
	EXYNOS_PIN_BANK_EINTW(8, 0x060, "gpa3", 0x0c),
};

/* pin banks of exynos7 pin-controller - BUS0 */
static const struct samsung_pin_bank_data exynos7_pin_banks1[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(5, 0x000, "gpb0", 0x00),
	EXYNOS_PIN_BANK_EINTG(8, 0x020, "gpc0", 0x04),
	EXYNOS_PIN_BANK_EINTG(2, 0x040, "gpc1", 0x08),
	EXYNOS_PIN_BANK_EINTG(6, 0x060, "gpc2", 0x0c),
	EXYNOS_PIN_BANK_EINTG(8, 0x080, "gpc3", 0x10),
	EXYNOS_PIN_BANK_EINTG(4, 0x0a0, "gpd0", 0x14),
	EXYNOS_PIN_BANK_EINTG(6, 0x0c0, "gpd1", 0x18),
	EXYNOS_PIN_BANK_EINTG(8, 0x0e0, "gpd2", 0x1c),
	EXYNOS_PIN_BANK_EINTG(5, 0x100, "gpd4", 0x20),
	EXYNOS_PIN_BANK_EINTG(4, 0x120, "gpd5", 0x24),
	EXYNOS_PIN_BANK_EINTG(6, 0x140, "gpd6", 0x28),
	EXYNOS_PIN_BANK_EINTG(3, 0x160, "gpd7", 0x2c),
	EXYNOS_PIN_BANK_EINTG(2, 0x180, "gpd8", 0x30),
	EXYNOS_PIN_BANK_EINTG(2, 0x1a0, "gpg0", 0x34),
	EXYNOS_PIN_BANK_EINTG(4, 0x1c0, "gpg3", 0x38),
};

/* pin banks of exynos7 pin-controller - NFC */
static const struct samsung_pin_bank_data exynos7_pin_banks2[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(3, 0x000, "gpj0", 0x00),
};

/* pin banks of exynos7 pin-controller - TOUCH */
static const struct samsung_pin_bank_data exynos7_pin_banks3[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(3, 0x000, "gpj1", 0x00),
};

/* pin banks of exynos7 pin-controller - FF */
static const struct samsung_pin_bank_data exynos7_pin_banks4[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(4, 0x000, "gpg4", 0x00),
};

/* pin banks of exynos7 pin-controller - ESE */
static const struct samsung_pin_bank_data exynos7_pin_banks5[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(5, 0x000, "gpv7", 0x00),
};

/* pin banks of exynos7 pin-controller - FSYS0 */
static const struct samsung_pin_bank_data exynos7_pin_banks6[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(7, 0x000, "gpr4", 0x00),
};

/* pin banks of exynos7 pin-controller - FSYS1 */
static const struct samsung_pin_bank_data exynos7_pin_banks7[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(4, 0x000, "gpr0", 0x00),
	EXYNOS_PIN_BANK_EINTG(8, 0x020, "gpr1", 0x04),
	EXYNOS_PIN_BANK_EINTG(5, 0x040, "gpr2", 0x08),
	EXYNOS_PIN_BANK_EINTG(8, 0x060, "gpr3", 0x0c),
};

/* pin banks of exynos7 pin-controller - BUS1 */
static const struct samsung_pin_bank_data exynos7_pin_banks8[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(8, 0x020, "gpf0", 0x00),
	EXYNOS_PIN_BANK_EINTG(8, 0x040, "gpf1", 0x04),
	EXYNOS_PIN_BANK_EINTG(4, 0x060, "gpf2", 0x08),
	EXYNOS_PIN_BANK_EINTG(5, 0x080, "gpf3", 0x0c),
	EXYNOS_PIN_BANK_EINTG(8, 0x0a0, "gpf4", 0x10),
	EXYNOS_PIN_BANK_EINTG(8, 0x0c0, "gpf5", 0x14),
	EXYNOS_PIN_BANK_EINTG(5, 0x0e0, "gpg1", 0x18),
	EXYNOS_PIN_BANK_EINTG(5, 0x100, "gpg2", 0x1c),
	EXYNOS_PIN_BANK_EINTG(6, 0x120, "gph1", 0x20),
	EXYNOS_PIN_BANK_EINTG(3, 0x140, "gpv6", 0x24),
};

static const struct samsung_pin_bank_data exynos7_pin_banks9[] __initconst = {
	EXYNOS_PIN_BANK_EINTG(7, 0x000, "gpz0", 0x00),
	EXYNOS_PIN_BANK_EINTG(4, 0x020, "gpz1", 0x04),
};

static const struct samsung_pin_ctrl exynos7_pin_ctrl[] __initconst = {
	{
		/* pin-controller instance 0 Alive data */
		.pin_banks	= exynos7_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks0),
		.eint_wkup_init = exynos_eint_wkup_init,
	}, {
		/* pin-controller instance 1 BUS0 data */
		.pin_banks	= exynos7_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks1),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 2 NFC data */
		.pin_banks	= exynos7_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 3 TOUCH data */
		.pin_banks	= exynos7_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 4 FF data */
		.pin_banks	= exynos7_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 5 ESE data */
		.pin_banks	= exynos7_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 6 FSYS0 data */
		.pin_banks	= exynos7_pin_banks6,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 7 FSYS1 data */
		.pin_banks	= exynos7_pin_banks7,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks7),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 8 BUS1 data */
		.pin_banks	= exynos7_pin_banks8,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks8),
		.eint_gpio_init = exynos_eint_gpio_init,
	}, {
		/* pin-controller instance 9 AUD data */
		.pin_banks	= exynos7_pin_banks9,
		.nr_banks	= ARRAY_SIZE(exynos7_pin_banks9),
		.eint_gpio_init = exynos_eint_gpio_init,
	},
};

/* pin banks of exynos9810 pin-controller 0 (ALIVE) */
static struct samsung_pin_bank_data exynos9810_pin_banks0[] = {
	EXYNOS9_PIN_BANK_EINTN(exynos9810_bank_type_alive, 6, 0x000, "etc1"),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x020, "gpa0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x040, "gpa1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x060, "gpa2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x080, "gpa3", 0x0c, 0x18),
	EXYNOS9_PIN_BANK_EINTN(exynos9810_bank_type_alive, 6, 0x0A0, "gpq0"),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 2, 0x0C0, "gpa4", 0x10, 0x20),
};

/* pin banks of exynos9810 pin-controller 1 (AUD) */
static struct samsung_pin_bank_data exynos9810_pin_banks1[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 5, 0x000, "gpb0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x020, "gpb1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x040, "gpb2", 0x08, 0x10),
};

/* pin banks of exynos9810 pin-controller 2 (CHUB) */
static struct samsung_pin_bank_data exynos9810_pin_banks2[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x000, "gph0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 5, 0x020, "gph1", 0x04, 0x08),
};

/* pin banks of exynos9810 pin-controller 3 (CMGP) */
static struct samsung_pin_bank_data exynos9810_pin_banks3[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x000, "gpm0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x020, "gpm1", 0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x040, "gpm2", 0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x060, "gpm3", 0x0C, 0x0C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x080, "gpm4", 0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x0A0, "gpm5", 0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x0C0, "gpm6", 0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x0E0, "gpm7", 0x1C, 0x1C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x100, "gpm10", 0x20, 0x20),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x120, "gpm11", 0x24, 0x24),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x140, "gpm12", 0x28, 0x28),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x160, "gpm13", 0x2C, 0x2C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x180, "gpm14", 0x30, 0x30),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x1A0, "gpm15", 0x34, 0x34),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x1C0, "gpm16", 0x38, 0x38),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x1E0, "gpm17", 0x3C, 0x3C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x200, "gpm40", 0x40, 0x40),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x220, "gpm41", 0x44, 0x44),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x240, "gpm42", 0x48, 0x48),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x260, "gpm43", 0x4C, 0x4C),
};

/* pin banks of exynos9810 pin-controller 4 (FSYS0) */
static struct samsung_pin_bank_data exynos9810_pin_banks4[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 2, 0x000, "gpf0", 0x00, 0x00),
};

/* pin banks of exynos9810 pin-controller 5 (FSYS1) */
static struct samsung_pin_bank_data exynos9810_pin_banks5[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 7, 0x000, "gpf1", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 6, 0x020, "gpf2", 0x04, 0x08),
};

/* pin banks of exynos9810 pin-controller 6 (PERIC0) */
static struct samsung_pin_bank_data exynos9810_pin_banks6[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x000, "gpp0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x020, "gpp1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x040, "gpp2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x060, "gpp3", 0x0C, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x080, "gpg0", 0x10, 0x1C),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x0A0, "gpg1", 0x14, 0x24),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x0C0, "gpg2", 0x18, 0x2C),
};

/* pin banks of exynos9810 pin-controller 7 (PERIC1) */
static struct samsung_pin_bank_data exynos9810_pin_banks7[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x000, "gpp4", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x020, "gpp5", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x040, "gpp6", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x060, "gpc0", 0x0C, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x080, "gpc1", 0x10, 0x1C),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x0A0, "gpd0", 0x14, 0x24),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 7, 0x0C0, "gpg3", 0x18, 0x28),
};

/* pin banks of exynos9810 pin-controller 8 (VTS) */
static struct samsung_pin_bank_data exynos9810_pin_banks8[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 3, 0x000, "gpt0", 0x00, 0x00),
};

const struct samsung_pin_ctrl exynos9810_pin_ctrl[] = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks	= exynos9810_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks0),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 AUD data */
		.pin_banks	= exynos9810_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks1),
		//.eint_gpio_init = exynos_eint_gpio_init,
		//.suspend	= exynos_pinctrl_suspend,
		//.resume	= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 2 CHUB data */
		.pin_banks	= exynos9810_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 3 CMGP data */
		.pin_banks	= exynos9810_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 4 FSYS0 data */
		.pin_banks	= exynos9810_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 5 FSYS1 data */
		.pin_banks	= exynos9810_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 6 PERIC0 data */
		.pin_banks	= exynos9810_pin_banks6,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 7 PERIC1 data */
		.pin_banks	= exynos9810_pin_banks7,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks7),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 8 VTS data */
		.pin_banks	= exynos9810_pin_banks8,
		.nr_banks	= ARRAY_SIZE(exynos9810_pin_banks8),
		//.eint_gpio_init = exynos_eint_gpio_init,
		//.suspend	= exynos_pinctrl_suspend,
		//.resume	= exynos_pinctrl_resume,
	},
};

/* pin banks of exynos9820 pin-controller 0 (ALIVE) */
static struct samsung_pin_bank_data exynos9820_pin_banks0[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x000, "gpa0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x020, "gpa1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x040, "gpa2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 8, 0x060, "gpa3", 0x0c, 0x18),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 2, 0x080, "gpa4", 0x10, 0x20),
	EXYNOS9_PIN_BANK_EINTN(exynos9810_bank_type_alive, 7, 0x0A0, "gpq0"),
	EXYNOS9_PIN_BANK_EINTN(exynos9810_bank_type_alive, 6, 0x0C0, "etc0"),
};

/* pin banks of exynos9820 pin-controller 1 (AUD) */
static struct samsung_pin_bank_data exynos9820_pin_banks1[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 5, 0x000, "gpb0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x020, "gpb1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x040, "gpb2", 0x08, 0x10),
};

/* pin banks of exynos9820 pin-controller 3 (CMGP) */
static struct samsung_pin_bank_data exynos9820_pin_banks2[] = {
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x000, "gpm0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x020, "gpm1", 0x04, 0x04),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x040, "gpm2", 0x08, 0x08),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x060, "gpm3", 0x0C, 0x0C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x080, "gpm4", 0x10, 0x10),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x0A0, "gpm5", 0x14, 0x14),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x0C0, "gpm6", 0x18, 0x18),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x0E0, "gpm7", 0x1C, 0x1C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x100, "gpm8", 0x20, 0x20),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x120, "gpm9", 0x24, 0x24),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x140, "gpm10", 0x28, 0x28),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x160, "gpm11", 0x2C, 0x2C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x180, "gpm12", 0x30, 0x30),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x1A0, "gpm13", 0x34, 0x34),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x1C0, "gpm14", 0x38, 0x38),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x1E0, "gpm15", 0x3C, 0x3C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x200, "gpm16", 0x40, 0x40),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x220, "gpm17", 0x44, 0x44),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x240, "gpm18", 0x48, 0x48),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x260, "gpm19", 0x4C, 0x4C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x280, "gpm20", 0x50, 0x50),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x2A0, "gpm21", 0x54, 0x54),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x2C0, "gpm22", 0x58, 0x58),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x2E0, "gpm23", 0x5C, 0x5C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x300, "gpm24", 0x60, 0x60),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x320, "gpm25", 0x64, 0x64),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x340, "gpm26", 0x68, 0x68),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x360, "gpm27", 0x6C, 0x6C),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x380, "gpm28", 0x70, 0x70),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x3A0, "gpm29", 0x74, 0x74),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x3C0, "gpm30", 0x78, 0x78),
	EXYNOS9_PIN_BANK_EINTW(exynos9810_bank_type_alive, 1, 0x3E0, "gpm31", 0x7C, 0x7C),

};

/* pin banks of exynos9820 pin-controller 4 (FSYS0) */
static struct samsung_pin_bank_data exynos9820_pin_banks3[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 6, 0x000, "gpf0", 0x00, 0x00),
};

/* pin banks of exynos9820 pin-controller 5 (FSYS1) */
static struct samsung_pin_bank_data exynos9820_pin_banks4[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 6, 0x000, "gpf1", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 3, 0x020, "gpf2", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 3, 0x040, "gpf3", 0x08, 0x0C),
};

/* pin banks of exynos9820 pin-controller 6 (PERIC0) */
static struct samsung_pin_bank_data exynos9820_pin_banks5[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x000, "gpp0", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x020, "gpp1", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x040, "gpp2", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 5, 0x060, "gpp3", 0x0C, 0x18),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x080, "gpg0", 0x10, 0x20),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x0A0, "gpg1", 0x14, 0x28),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 7, 0x0C0, "gpg2", 0x18, 0x30),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x0E0, "gpg4", 0x1C, 0x38),
};

/* pin banks of exynos9820 pin-controller 7 (PERIC1) */
static struct samsung_pin_bank_data exynos9820_pin_banks6[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x000, "gpp4", 0x00, 0x00),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x020, "gpp5", 0x04, 0x08),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x040, "gpp6", 0x08, 0x10),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x060, "gpc0", 0x0C, 0x14),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x080, "gpc1", 0x10, 0x1C),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x0A0, "gpd0", 0x14, 0x24),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 7, 0x0C0, "gpg3", 0x18, 0x28),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 8, 0x0E0, "gph0", 0x1C, 0x30),
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 5, 0x100, "gph1", 0x20, 0x38),
};

/* pin banks of exynos9820 pin-controller 8 (VTS) */
static struct samsung_pin_bank_data exynos9820_pin_banks7[] = {
	EXYNOS9_PIN_BANK_EINTG(exynos9810_bank_type_off, 4, 0x000, "gpv0", 0x00, 0x00),
};

static struct samsung_pin_ctrl exynos9820_pin_ctrl[] = {
	{
		/* pin-controller instance 0 ALIVE data */
		.pin_banks	= exynos9820_pin_banks0,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks0),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 1 AUD data */
		.pin_banks	= exynos9820_pin_banks1,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks1),
		//.eint_gpio_init = exynos_eint_gpio_init,
		//.suspend	= exynos_pinctrl_suspend,
		//.resume	= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 3 CMGP data */
		.pin_banks	= exynos9820_pin_banks2,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks2),
		.eint_gpio_init = exynos_eint_gpio_init,
		.eint_wkup_init = exynos_eint_wkup_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 4 FSYS0 data */
		.pin_banks	= exynos9820_pin_banks3,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks3),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 5 FSYS1 data */
		.pin_banks	= exynos9820_pin_banks4,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks4),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 6 PERIC0 data */
		.pin_banks	= exynos9820_pin_banks5,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks5),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 7 PERIC1 data */
		.pin_banks	= exynos9820_pin_banks6,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks6),
		.eint_gpio_init = exynos_eint_gpio_init,
		.suspend	= exynos_pinctrl_suspend,
		.resume		= exynos_pinctrl_resume,
	}, {
		/* pin-controller instance 8 VTS data */
		.pin_banks	= exynos9820_pin_banks7,
		.nr_banks	= ARRAY_SIZE(exynos9820_pin_banks7),
		//.eint_gpio_init = exynos_eint_gpio_init,
		//.suspend	= exynos_pinctrl_suspend,
		//.resume	= exynos_pinctrl_resume,
	},
};

const struct samsung_pinctrl_of_match_data exynos7_of_data __initconst = {
	.ctrl		= exynos7_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos7_pin_ctrl),
};

const struct samsung_pinctrl_of_match_data exynos9810_of_data __initconst = {
       .ctrl           = exynos9810_pin_ctrl,
       .num_ctrl       = ARRAY_SIZE(exynos9810_pin_ctrl),
};

const struct samsung_pinctrl_of_match_data exynos9820_of_data __initconst = {
	.ctrl		= exynos9820_pin_ctrl,
	.num_ctrl	= ARRAY_SIZE(exynos9820_pin_ctrl),
};
