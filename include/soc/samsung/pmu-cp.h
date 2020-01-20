/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * EXYNOS - PMU(Power Management Unit) support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __EXYNOS_PMU_CP_H
#define __EXYNOS_PMU_CP_H __FILE__

/* BLK_ALIVE: CP related SFRs */
#define EXYNOS_PMU_CP_CTRL_NS				0x0030
#define EXYNOS_PMU_CP_CTRL_S				0x0034
#define EXYNOS_PMU_CP_STAT					0x0038
#define EXYNOS_PMU_CP_DEBUG					0x003C

#define EXYNOS_PMU_UART_IO_SHARE_CTRL		0x6200
#define EXYNOS_PMU_CP_ADDR_CFG0				0x7140
#define EXYNOS_PMU_CP_ADDR_CFG1				0x7144
#define EXYNOS_PMU_CP_ADDR_CFG2				0x7148
#define EXYNOS_PMU_CP_ADDR_CFG3				0x714C
#define EXYNOS_PMU_CP_ADDR_CFG4				0x7150
#define EXYNOS_PMU_CP_ADDR_CFG5				0x7154
#define EXYNOS_PMU_CP_ADDR_CFG6				0x7158
#define EXYNOS_PMU_CP_ADDR_CFG7				0x715C
#define EXYNOS_PMU_CP_ADDR_CFG8				0x7160
#define EXYNOS_PMU_CP_ADDR_CFG9				0x7164
#define EXYNOS_PMU_CP_ADDR_CFG10			0x7168
#define EXYNOS_PMU_CP_ADDR_CFG11			0x716C
#define EXYNOS_PMU_CP_ADDR_CFG12			0x7170
#define EXYNOS_PMU_CP_ADDR_CFG13			0x7174
#define EXYNOS_PMU_CP_ADDR_CFG14			0x7178
#define EXYNOS_PMU_CP_ADDR_CFG_ADDRMAP		0x717C

#define EXYNOS_PMU_CP2AP_MEM_CONFIG0		0x7200
#define EXYNOS_PMU_CP2AP_MEM_CONFIG1		0x7204
#define EXYNOS_PMU_CP2AP_MEM_CONFIG2		0x7208
#define EXYNOS_PMU_CP2AP_ADDR_RNG			0x720C

#define EXYNOS_PMU_CP2AP_MIF_ACCESS_WIN0	0x7210
#define EXYNOS_PMU_CP2AP_MIF_ACCESS_WIN1	0x7214
#define EXYNOS_PMU_CP2AP_MIF_ACCESS_WIN2	0x7218
#define EXYNOS_PMU_CP2AP_MIF_ACCESS_WIN3	0x721C
#define EXYNOS_PMU_CP2AP_MIF_ACCESS_WIN4	0x7220
#define EXYNOS_PMU_CP2AP_MIF_ACCESS_WIN5	0x7224
#define EXYNOS_PMU_CP2AP_PERI_ACCESS_WIN0	0x7228
#define EXYNOS_PMU_CP2AP_PERI_ACCESS_WIN1	0x722C
#define EXYNOS_PMU_CP2AP_PERI_ACCESS_WIN2	0x7230
#define EXYNOS_PMU_CP2AP_PERI_ACCESS_WIN3	0x7234
#define EXYNOS_PMU_CP2AP_PERI_ACCESS_WIN4	0x7238
#define EXYNOS_PMU_CP2AP_PERI_ACCESS_WIN5	0x723C
#define EXYNOS_PMU_CP_BOOT_TEST_RST_CONFIG	0x7240
#define EXYNOS_PMU_CP_ADDR_MAP_ACCESS_WIN_START	0x7244
#define EXYNOS_PMU_CP_ADDR_MAP_ACCESS_WIN_END	0x7248

#define EXYNOS_PMU_CENTRAL_SEQ_CP_CONFIGURATION	0x0280
#define EXYNOS_PMU_CENTRAL_SEQ_CP_STATUS	0x0284
#define EXYNOS_PMU_RESET_SEQUENCER_STATUS	0x0504
#define EXYNOS_PMU_RESET_AHEAD_CP_SYS_PWR_REG	0x1320
#define EXYNOS_PMU_CLEANY_BUS_CP_SYS_PWR_REG	0x1324
#define EXYNOS_PMU_LOGIC_RESET_CP_SYS_PWR_REG	0x1328
#define EXYNOS_PMU_TCXO_GATE_SYS_PWR_REG	0x132C
#define EXYNOS_PMU_CP_DISABLE_ISO_SYS_PWR_REG	0X1330
#define EXYNOS_PMU_RESET_ISO_SYS_PWR_REG	0X1334

#define EXYNOS_PMU_CLEANY_BUS_CP_CONFIGURATION	0x3920
#define EXYNOS_PMU_CLEANY_BUS_CP_STATUS		0x3924


/* EXYNOS_PMU_CENTRAL_SEQ_CP_STATUS */
#define CENTRAL_SEQ_CP_STATUS_MASK	(0xFF << 16)
#define CENTRAL_SEQ_CP_STATUS_SHIFT	16

/* EXYNOS_PMU_CLEANY_BUS_CP_STATUS */
#define CLEANY_CP_STATES_MASK	(0x3 << 16)
#define CLEANY_CP_STATES_SHIFT	16


/* EXYNOS_PMU_UART_IO_SHARE_CTRL */
#define SEL_CP_UART_DBG			BIT(8)
#define SEL_UART_DBG_GPIO		BIT(4)
#define FUNC_ISO_EN				BIT(0)

/* EXYNOS_PMU_CP_CTRL_NS */
#define CP_PWRON				BIT(1)
#define CP_RESET_SET			BIT(2)
#define CP_ACTIVE_REQ_EN		BIT(5)
#define CP_ACTIVE_REQ_CLR		BIT(6)
#define CP_RESET_REQ_EN			BIT(7)
#define CP_RESET_REQ_CLR		BIT(8)
#define MASK_CP_PWRDN_DONE		BIT(9)
#define RTC_OUT_EN				BIT(10)
#define SET_SW_MIF_REQ			BIT(12)
#define MASK_MIF_REQ			BIT(13)
#define SWEEPER_BYPASS_DATA_EN	BIT(16)
#define WDT_FLAG				BIT(20)
#define WDT_FLAG_CLR_EN			BIT(21)
#define MASK_COMMON_CP_RESET	BIT(24)
#define TCXO_SEL				BIT(28)
#define CR7_MP_DISABLE			BIT(29)

/* EXYNOS_PMU_CP_CTRL_N */
#define CP_START		        BIT(3)

/* EXYNOS_PMU_CP_STAT */
#define CP_PWRDN_DONE	        BIT(0)
#define CP_ACCESS_MIF	        BIT(4)


#define SMC_ID		0x82000700
#define READ_CTRL	0x3
#define WRITE_CTRL	0x4

enum cp_mode {
	CP_POWER_ON,
	CP_RESET,
	CP_POWER_OFF,
	NUM_CP_MODE,
};

enum reset_mode {
	CP_HW_RESET,
	CP_SW_RESET,
};

enum cp_control {
	CP_CTRL_S,
	CP_CTRL_NS,
};

extern int exynos_cp_reset(void);
extern int exynos_cp_release(void);
extern int exynos_cp_init(void);
extern int exynos_cp_active_clear(void);
extern int exynos_clear_cp_reset(void);
extern int exynos_get_cp_power_status(void);
extern int exynos_set_cp_power_onoff(enum cp_mode mode);
extern void exynos_sys_powerdown_conf_cp(void);
extern int exynos_pmu_cp_init(void);
#endif /* __EXYNOS_PMU_CP_H */
