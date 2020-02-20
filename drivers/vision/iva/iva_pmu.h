/*
 * Copyright (C) 2016 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute	it and/or modify it
 * under  the terms of	the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __IVA_PMU_H__
#define __IVA_PMU_H__

#include "iva_ctrl.h"

#if defined(CONFIG_SOC_EXYNOS9810) || (CONFIG_SOC_EXYNOS9820)
#define ENABLE_SRAM_ACCESS_WORKAROUND
#define ENABLE_CORE_CLK_DURING_DUMP_WORKAROUND
#endif

enum iva_pmu_clk_type {
	pmu_clk_type_none	= -1,
	/*
	 * clock core:
	 *	in iva 1.0, core clock includes sfr clock .
	 *	in iva 2.0, core does not include sfr clock.
	 *	in iva 3.0, seperate core and sfr clock.
	 */
	pmu_clk_type_core	= 0,
#if defined(CONFIG_SOC_EXYNOS9810) || (CONFIG_SOC_EXYNOS9820)
	pmu_clk_type_sfr,
#endif
	pmu_clk_type_max
};

enum iva_pmu_ctrl {
#if defined(CONFIG_SOC_EXYNOS8895)
	pmu_q_ch_vmcu_active	= (0x1 << 0),
	pmu_q_ch_sw_qch		= (0x1 << 1),
	pmu_q_ch_sw_qaccept_n	= (0x1 << 2),
#elif defined(CONFIG_SOC_EXYNOS9810) || (CONFIG_SOC_EXYNOS9820)
	pmu_ctrl_qactive	= (0x1 << 0),
	pmu_vdma_abort		= (0x1 << 1),
	pmu_dspif_abort		= (0x1 << 2),
#endif
};

enum iva_pmu_ip_id {
	pmu_id_none	= (0),
#if defined(CONFIG_SOC_EXYNOS8895)
	pmu_csc_id	= (0x1 << 0),
	pmu_scl0_id	= (0x1 << 1),
	pmu_scl1_id	= (0x1 << 2),
	pmu_orb_id	= (0x1 << 3),
	pmu_mch_id	= (0x1 << 4),
	pmu_lmd_id	= (0x1 << 5),
	pmu_lkt_id	= (0x1 << 6),
	pmu_wig0_id	= (0x1 << 7),
	pmu_wig1_id	= (0x1 << 8),
	pmu_enf_id	= (0x1 << 9),
	pmu_vdma0_id	= (0x1 << 10),
	pmu_vdma1_id	= (0x1 << 11),
	pmu_vcf_id	= (0x1 << 12),
	pmu_vcm_id	= (0x1 << 13),
	pmu_edil_id	= (0x1 << 14),

	//----------------------------
	pmu_all_ids	= (0x1 << 15) - 1,
#elif defined(CONFIG_SOC_EXYNOS9810)
	pmu_csc_id	= (0x1 << 0),
	pmu_scl0_id	= (0x1 << 1),
	pmu_scl1_id	= (0x1 << 2),
	pmu_orb_id	= (0x1 << 3),
	pmu_mch_id	= (0x1 << 4),
	pmu_lmd_id	= (0x1 << 5),
	pmu_lkt_id	= (0x1 << 6),
	pmu_wig0_id	= (0x1 << 7),
	pmu_wig1_id	= (0x1 << 8),
	pmu_enf_id	= (0x1 << 9),
	pmu_vdma_id	= (0x1 << 10),
	pmu_vcm_id	= (0x1 << 11),
	pmu_edil_id	= (0x1 << 12),
	pmu_mfb1_id	= (0x1 << 13),
	pmu_mfb2_id	= (0x1 << 14),
	pmu_bax1_id	= (0x1 << 15),
	pmu_bax2_id	= (0x1 << 16),
	pmu_dspif_id	= (0x1 << 17),
	pmu_wig2_id	= (0x1 << 18),
	pmu_wig3_id	= (0x1 << 19),
	pmu_vcf_id	= (0x1 << 24),

	//----------------------------
	pmu_all_ids	= (pmu_vcf_id | ((0x1 << 20) - 1)),
	pmu_all_wo_vdma	= (pmu_all_ids & ~pmu_vdma_id),
	pmu_all_wo_vcf	= (pmu_all_ids & ~pmu_vcf_id),
#elif defined(CONFIG_SOC_EXYNOS9820)
    pmu_csc_id      = (0x1 << 0),
    pmu_scl0_id     = (0x1 << 1),
    pmu_scl1_id     = (0x1 << 2),
    pmu_orb_id      = (0x1 << 3),
    pmu_mch_id      = (0x1 << 4),
    pmu_lmd_id      = (0x1 << 5),
    pmu_lkt_id      = (0x1 << 6),
    pmu_wig0_id     = (0x1 << 7),
    pmu_wig1_id     = (0x1 << 8),
    pmu_enf_id      = (0x1 << 9),
    pmu_vdma_id     = (0x1 << 10),
    pmu_vcm_id      = (0x1 << 11),
    pmu_edil_id     = (0x1 << 12),
    pmu_mfb1_id     = (0x1 << 13),
    pmu_mfb2_id     = (0x1 << 14),
    pmu_bax1_id     = (0x1 << 15),
    pmu_bax2_id     = (0x1 << 16),
    pmu_dspif_id    = (0x1 << 17),
    pmu_wig2_id     = (0x1 << 18),
    pmu_imu_id      = (0x1 << 20),
    pmu_scl2_id     = (0x1 << 21),
    pmu_vcf_id      = (0x1 << 24),

    //----------------------------
    pmu_all_ids = (pmu_vcf_id | pmu_scl2_id | pmu_imu_id | ((0x1 << 19) - 1)),
    pmu_all_wo_vdma = (pmu_all_ids & ~pmu_vdma_id),
    pmu_all_wo_vcf  = (pmu_all_ids & ~pmu_vcf_id),
#endif
};

enum iva_pmu_mcu_ctrl {
	pmu_mcu_reset_n	= (0x1 << 0),
#if defined(CONFIG_SOC_EXYNOS9810)
	pmu_boothold	= (0x1 << 1),
#endif
#if defined(CONFIG_SOC_EXYNOS8895)
	pmu_en_sleep	= (0x1 << 2),
#endif
};

extern uint32_t iva_pmu_ctrl(struct iva_dev_data *iva,
			enum iva_pmu_ctrl ctrl, bool set);

extern uint32_t iva_pmu_reset_hwa(struct iva_dev_data *iva,
			enum iva_pmu_ip_id ip, bool release);

extern uint32_t iva_pmu_is_clock_enabled(struct iva_dev_data *iva,
		enum iva_pmu_clk_type type, enum iva_pmu_ip_id ip);

extern uint32_t iva_pmu_enable_clock(struct iva_dev_data *iva,
		enum iva_pmu_clk_type type, enum iva_pmu_ip_id ip, bool enable);

/* core clock */
static inline uint32_t iva_pmu_is_clk_enabled(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip)
{
	return iva_pmu_is_clock_enabled(iva, pmu_clk_type_core, ip);
}

static inline uint32_t iva_pmu_enable_clk(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
	return iva_pmu_enable_clock(iva, pmu_clk_type_core, ip, enable);
}

#if defined(CONFIG_SOC_EXYNOS9810)
static inline uint32_t iva_pmu_is_sfr_clk_enabled(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip)
{
	return iva_pmu_is_clock_enabled(iva, pmu_clk_type_sfr, ip);
}

static inline uint32_t iva_pmu_enable_sfr_clk(struct iva_dev_data *iva,
		enum iva_pmu_ip_id ip, bool enable)
{
	return iva_pmu_enable_clock(iva, pmu_clk_type_sfr, ip, enable);
}
#elif defined(CONFIG_SOC_EXYNOS9820)
static inline uint32_t iva_pmu_is_core_clk_enabled(struct iva_dev_data *iva,
        enum iva_pmu_ip_id ip)
{
	return iva_pmu_is_clock_enabled(iva, pmu_clk_type_core, ip);
}
static inline uint32_t iva_pmu_enable_core_clk(struct iva_dev_data *iva,
        enum iva_pmu_ip_id ip, bool enable)
{
	return iva_pmu_enable_clock(iva, pmu_clk_type_core, ip, enable);
}
#endif

/* mcu control */
extern uint32_t iva_pmu_ctrl_mcu(struct iva_dev_data *iva,
			enum iva_pmu_mcu_ctrl mcu_ctrl, bool set);
extern void	iva_pmu_prepare_mcu_sram(struct iva_dev_data *iva);
extern void	iva_pmu_reset_mcu(struct iva_dev_data *iva);
extern void	iva_pmu_prepare_sfr_dump(struct iva_dev_data *iva);

static inline uint32_t iva_pmu_reset_all(struct iva_dev_data *iva)
{
	return iva_pmu_reset_hwa(iva, pmu_all_ids, false);
}

static inline uint32_t iva_pmu_release_all(struct iva_dev_data *iva)
{
	return iva_pmu_reset_hwa(iva, pmu_all_ids, true);
}

static inline uint32_t iva_pmu_enable_all_clks(struct iva_dev_data *iva)
{
	return iva_pmu_enable_clk(iva, pmu_all_ids, true);
}

static inline uint32_t iva_pmu_disable_all_clks(struct iva_dev_data *iva)
{
	return iva_pmu_enable_clk(iva, pmu_all_ids, false);
}

#if defined(CONFIG_SOC_EXYNOS9810)
static inline uint32_t iva_pmu_enable_sfr_clks(struct iva_dev_data *iva)
{
	return iva_pmu_enable_sfr_clk(iva, pmu_all_wo_vcf, true);
}

static inline uint32_t iva_pmu_disable_sfr_clks(struct iva_dev_data *iva)
{
	return iva_pmu_enable_sfr_clk(iva, pmu_all_wo_vcf, false);
}
#elif defined(CONFIG_SOC_EXYNOS9820)
static inline uint32_t iva_pmu_enable_core_clks(struct iva_dev_data *iva)
{
	return iva_pmu_enable_core_clk(iva, pmu_all_wo_vcf, true);
}

static inline uint32_t iva_pmu_disable_core_clks(struct iva_dev_data *iva)
{
	return iva_pmu_enable_core_clk(iva, pmu_all_wo_vcf, false);
}
#endif

extern void	iva_pmu_show_qactive_status(struct iva_dev_data *iva);
extern void	iva_pmu_show_status(struct iva_dev_data *iva);

extern int	iva_pmu_init(struct iva_dev_data *iva, bool en_hwa);
extern void	iva_pmu_deinit(struct iva_dev_data *iva);

extern int	iva_pmu_probe(struct iva_dev_data *iva);
extern void	iva_pmu_remove(struct iva_dev_data *iva);

#endif /* __IVA_PMU_H__ */
