/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung EXYNOS9SoC series HDR driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "mcd_cm_lut.h"
#include "hdr_drv.h"
#include "hdr_reg.h"
#include "mcd_cm.h"

#define TUNE_ACTIVATE

enum pq_index luminance2pqindex(unsigned int luminance)
{
    return (luminance <   100) ? INDEX_PQ1000 :
           (luminance <=  200) ? INDEX_PQ0200 :
           (luminance <=  250) ? INDEX_PQ0250 :
           (luminance <=  300) ? INDEX_PQ0300 :
           (luminance <=  350) ? INDEX_PQ0350 :
           (luminance <=  400) ? INDEX_PQ0400 :
           (luminance <=  450) ? INDEX_PQ0450 :
           (luminance <=  500) ? INDEX_PQ0500 :
           (luminance <=  550) ? INDEX_PQ0550 :
           (luminance <=  600) ? INDEX_PQ0600 :
           (luminance <=  650) ? INDEX_PQ0650 :
           (luminance <=  700) ? INDEX_PQ0700 :
           (luminance <=  750) ? INDEX_PQ0750 :
           (luminance <=  800) ? INDEX_PQ0800 :
           (luminance <=  850) ? INDEX_PQ0850 :
           (luminance <=  900) ? INDEX_PQ0900 :
           (luminance <=  950) ? INDEX_PQ0950 :
           (luminance <= 1000) ? INDEX_PQ1000 :
           (luminance <= 2000) ? INDEX_PQ2000 :
           (luminance <= 3000) ? INDEX_PQ3000 :
           (luminance <= 4000) ? INDEX_PQ4000 : INDEX_PQ4000;
}
enum target_nit_index luminance2targetindex(unsigned int luminance)
{
    return (luminance <=  200) ? INDEX_T0200 :
           (luminance <=  250) ? INDEX_T0250 :
           (luminance <=  300) ? INDEX_T0300 :
           (luminance <=  350) ? INDEX_T0350 :
           (luminance <=  400) ? INDEX_T0400 :
           (luminance <=  450) ? INDEX_T0450 :
           (luminance <=  500) ? INDEX_T0500 :
           (luminance <=  550) ? INDEX_T0550 :
           (luminance <=  600) ? INDEX_T0600 :
           (luminance <=  650) ? INDEX_T0650 :
           (luminance <=  700) ? INDEX_T0700 :
           (luminance <=  750) ? INDEX_T0750 :
           (luminance <=  800) ? INDEX_T0800 :
           (luminance <=  850) ? INDEX_T0850 :
           (luminance <=  900) ? INDEX_T0900 :
           (luminance <=  950) ? INDEX_T0950 :
           (luminance <= 1000) ? INDEX_T1000 : INDEX_T1000;
}
void get_tables_hdr10plus(struct cm_tables * tables,
    const enum gamma_index src_gamma, const enum gamut_index src_gamut, const unsigned int src_max_luminance,
    const enum gamma_index dst_gamma, const enum gamut_index dst_gamut, const unsigned int dst_max_luminance)
{
    //-------------------------------------------------------------------------------------------------
    tables->eotf = 0;
    tables->gm   = 0;
    tables->oetf = 0;
    tables->sc   = 0;
    tables->tm   = 0;
    tables->tms  = TABLE_TMS_BYPASS;
    //-------------------------------------------------------------------------------------------------
    {
        const enum gamma_type_index i_type = (src_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                             (src_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
        const enum gamma_type_index o_type = (dst_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                             (dst_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;

        if ((i_type == INDEX_TYPE_PQ) && (o_type == INDEX_TYPE_SDR))
        {
            const enum target_nit_index idx_td = (dst_max_luminance != 0) ? luminance2targetindex(dst_max_luminance) : INDEX_T0250;
            const enum pq_index idx_pq = luminance2pqindex(src_max_luminance);

            tables->gm = TABLE_GM[src_gamut][dst_gamut];
            tables->tm = TABLE_TM_PQ[idx_td][idx_pq];
            if (tables->tm != 0)
                tables->sc  = TABLE_SC[idx_pq];
            tables->tms = TABLE_TMS_PQ[idx_td][0];
            tables->eotf = TABLE_EOTF[src_gamma];
            tables->oetf = TABLE_OETF[dst_gamma][0];
        }
    }
}
void get_tables_standard(struct cm_tables * tables,
    const enum gamma_index src_gamma, const enum gamut_index src_gamut, const unsigned int src_max_luminance,
    const enum gamma_index dst_gamma, const enum gamut_index dst_gamut, const unsigned int dst_max_luminance)
{
    //-------------------------------------------------------------------------------------------------
    tables->eotf = 0;
    tables->gm   = 0;
    tables->oetf = 0;
    tables->sc   = 0;
    tables->tm   = 0;
    tables->tms  = TABLE_TMS_BYPASS;
    //-------------------------------------------------------------------------------------------------
    {
        const enum gamma_type_index i_type = (src_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                             (src_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
        const enum gamma_type_index o_type = (dst_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                             (dst_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
        const enum target_nit_index idx_td = (o_type == INDEX_TYPE_HLG) ? INDEX_THLG  :
                                             (dst_max_luminance != 0) ? luminance2targetindex(dst_max_luminance) :
                                             (o_type == INDEX_TYPE_SDR) ? INDEX_T0250 : INDEX_T1000;
        unsigned int half_mode = 0;
        tables->gm = TABLE_GM[src_gamut][dst_gamut];
        if (((i_type != INDEX_TYPE_SDR) || (o_type != INDEX_TYPE_SDR)) && (tables->gm != 0))
            half_mode = 1;

        if (i_type == INDEX_TYPE_PQ)
        {
            const enum pq_index idx_pq = luminance2pqindex(src_max_luminance);

            tables->tm = TABLE_TM_PQ[idx_td][idx_pq];
            if (tables->tm != 0)
                tables->sc  = TABLE_SC[idx_pq];
            tables->tms = TABLE_TMS_PQ[idx_td][half_mode];
        }
        else 
        {
            tables->sc  = 0;
            tables->tm  = 0;
            tables->tms = TABLE_TMS[i_type][o_type][half_mode];
        }
        if ((dst_gamma != src_gamma) || (tables->gm != 0) || (tables->tm != 0) || (tables->tms != TABLE_TMS_BYPASS)) {
            tables->eotf = TABLE_EOTF[src_gamma];
            tables->oetf = (o_type != INDEX_TYPE_PQ ) ? TABLE_OETF[dst_gamma][half_mode] :
                           (i_type == INDEX_TYPE_SDR) ? TABLE_OETF_SDR_TO_PQ[idx_td][half_mode] :
                                                        TABLE_OETF_HDR_TO_PQ[idx_td][half_mode];
        }
    }
}
void get_con_standard(unsigned int * con, const struct cm_tables * tables, const unsigned int isAlphaPremultiplied, const unsigned int applyDither) {
    *con = 0;
    if (tables->gm != 0)
        *con |= (CON_SFR_GAMUT);
    if ((tables->eotf != 0) && (tables->oetf != 0))
        *con |= (CON_SFR_GAMMA);
    if (tables->tm != 0)
        *con |= (CON_SFR_REMAP);
    if (*con > 0)
    {
        if (isAlphaPremultiplied)
            *con |= (CON_SFR_ALPHA);
    }
    if (applyDither)
        *con |= (CON_SFR_DITHER);
    if (*con > 0)
        *con |= (CON_SFR_ALL);
}

void get_tables(struct cm_tables * tables, unsigned int is_hdr10p,
    const enum gamma_index src_gamma, const enum gamut_index src_gamut, const unsigned int src_max_luminance,
    const enum gamma_index dst_gamma, const enum gamut_index dst_gamut, const unsigned int dst_max_luminance)
{
    if (is_hdr10p)
    {
        get_tables_hdr10plus(tables, src_gamma, src_gamut, src_max_luminance,
                                     dst_gamma, dst_gamut, dst_max_luminance);
    }
    else
    {
        get_tables_standard(tables, src_gamma, src_gamut, src_max_luminance,
                                    dst_gamma, dst_gamut, dst_max_luminance);
        {
            const enum gamma_type_index i_type = (src_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                                 (src_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
            const enum gamma_type_index o_type = (dst_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                                 (dst_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
            //-------------------------------------------------------------------------------------------------
            // tuning
            if ((i_type == INDEX_TYPE_PQ) && (o_type == INDEX_TYPE_SDR)) {
#ifdef TUNE_ACTIVATE
                if ((src_max_luminance > 3000) && (dst_gamma == INDEX_GAMMA_GAMMA2_2)) {
                    const enum pq_index         idx_pq = luminance2pqindex(src_max_luminance);
                    const enum target_nit_index idx_td = luminance2targetindex(dst_max_luminance);

                    tables->eotf = TABLE_EOTFvN_PQ4000;
                    tables->sc   = TABLE_SC[idx_pq];
                    tables->tm   = TABLE_TM_PQ[idx_td][idx_pq];
                    tables->tms  = TABLE_TMS_PQ[idx_td][1];
                    tables->oetf = TABLE_OETFvN_GAMMA2_2;
                }
#endif
            }
        }
    }
}

void get_con(unsigned int * con, const struct cm_tables * tables, const unsigned int isAlphaPremultiplied, const unsigned int applyDither, const unsigned int isHDR10p)
{
    get_con_standard(con, tables, isAlphaPremultiplied, applyDither);
    if (((*con) > 0) && (isHDR10p == 1))
    {
        *con &= (~CON_SFR_REMAP);
        *con |= (CON_SFR_HDR10P);
    }
}

void customize_dataspace(enum gamma_index * src_gamma, enum gamut_index * src_gamut,
                         enum gamma_index * dst_gamma, enum gamut_index * dst_gamut,
                         const unsigned int s_gamma, const unsigned int s_gamut,
                         const unsigned int d_gamma, const unsigned int d_gamut)
{
    const unsigned int legacy_mode = ((d_gamma == INDEX_GAMMA_UNSPECIFIED) &&
                                      (d_gamut == INDEX_GAMUT_UNSPECIFIED)) ? 1 : 0;
    const enum gamma_type_index i_type = (s_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ :
                                         (s_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
    //-------------------------------------------------------------------------------------------------
    if (legacy_mode == 1)
    {
        *src_gamut = (i_type == INDEX_TYPE_SDR) ? INDEX_GAMUT_DCI_P3 : ((enum gamut_index)s_gamut);
        *src_gamma = (i_type == INDEX_TYPE_SDR) ? INDEX_GAMMA_SRGB   : ((enum gamma_index)s_gamma);
        *dst_gamut = INDEX_GAMUT_DCI_P3;
        *dst_gamma = (i_type == INDEX_TYPE_SDR) ? INDEX_GAMMA_SRGB   : INDEX_GAMMA_GAMMA2_2;
    }
    else
    {
        *src_gamut = (enum gamut_index)s_gamut;
        *dst_gamut = (enum gamut_index)d_gamut;
        *src_gamma = (enum gamma_index)s_gamma;
        *dst_gamma = (enum gamma_index)d_gamma;
#ifdef ASSUME_SRGB_IS_GAMMA22_FOR_HDR
        if ((i_type != INDEX_TYPE_SDR) && ((*dst_gamma) == INDEX_GAMMA_SRGB)) *dst_gamma = INDEX_GAMMA_GAMMA2_2;
#endif
    }
}

void mcd_cm_sfr_bypass(struct mcd_hdr_device *hdr)
{
    const unsigned int sfr_con = 0;
    mcd_reg_write(hdr, DPP_MCD_CM_CON_ADDR, sfr_con);
    mcd_reg_write(hdr, DPP_MCD_CM_TMS_ADDR, TABLE_TMS_BYPASS);
}

void mcd_cm_sfr(struct mcd_hdr_device *hdr, const struct mcd_cm_params_info *params)
{
    enum gamma_index src_gamma, dst_gamma;
    enum gamut_index src_gamut, dst_gamut;

    customize_dataspace(&src_gamma, &src_gamut, &dst_gamma, &dst_gamut,
        params->src_gamma, params->src_gamut, params->dst_gamma, params->dst_gamut);
    //-------------------------------------------------------------------------------------------------
    if ((src_gamma == INDEX_GAMMA_UNSPECIFIED) || (dst_gamma == INDEX_GAMMA_UNSPECIFIED) ||
        (src_gamut == INDEX_GAMUT_UNSPECIFIED) || (dst_gamut == INDEX_GAMUT_UNSPECIFIED))
    {
        mcd_cm_sfr_bypass(hdr);
    }
    else
    {
        unsigned int is_hdr10p = 0;
        unsigned int sfr_con = 0;
        unsigned int *table_dm = 0;
        struct cm_tables tables;
        {
            const enum gamma_type_index o_type = (dst_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
                                                 (dst_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG :
                                                 INDEX_TYPE_SDR;
            //-------------------------------------------------------------------------------------------------
            unsigned int *hdr10p_lut = (params->hdr10p_lut != NULL) ? (((*params->hdr10p_lut) == 1) ? (&params->hdr10p_lut[1]) : NULL) : NULL;
            const unsigned int valid_hdr10 = ((src_gamma == INDEX_GAMMA_ST2084) && (src_gamut == INDEX_GAMUT_BT2020)) ? 1 : 0;
            is_hdr10p = ((hdr->id == MCD_VGRFS) && (hdr10p_lut != NULL) && (valid_hdr10 == 1) && (o_type == INDEX_TYPE_SDR)) ? 1 : 0;
            table_dm = (is_hdr10p) ? hdr10p_lut : NULL;
            //-------------------------------------------------------------------------------------------------
            {
                const unsigned int dither_allow = ((is_hdr10p == 0) && (valid_hdr10) && (params->src_max_luminance > 3000)) ? 1 : 0;
                const unsigned int applyDither = (params->needDither && (o_type == INDEX_TYPE_SDR) && dither_allow) ? 1 : 0;
                //-------------------------------------------------------------------------------------------------
                get_tables(&tables, is_hdr10p, src_gamma, src_gamut, params->src_max_luminance,
                                               dst_gamma, dst_gamut, params->dst_max_luminance);
                get_con(&sfr_con, &tables, params->isAlphaPremultiplied, applyDither, is_hdr10p);
            }
            //-------------------------------------------------------------------------------------------------
        }
        // sfr write
        mcd_reg_write(hdr, DPP_MCD_CM_CON_ADDR, sfr_con);
        mcd_reg_write(hdr, DPP_MCD_CM_TMS_ADDR, tables.tms);
        if (tables.eotf != 0)
            mcd_reg_writes(hdr, DPP_MCD_CM_EOTF_ADDR(0), tables.eotf, DPP_MCD_CM_EOTF_SZ);
        if (tables.gm   != 0)
            mcd_reg_writes(hdr, DPP_MCD_CM_GM_ADDR(0), tables.gm, DPP_MCD_CM_GM_SZ);
        if (tables.oetf != 0)
            mcd_reg_writes(hdr, DPP_MCD_CM_OETF_ADDR(0), tables.oetf, DPP_MCD_CM_OETF_SZ);
        if (is_hdr10p == 1)
        {
            if (table_dm != NULL)
                mcd_reg_writes(hdr, DPP_MCD_CM_DM_ADDR(0), table_dm, DPP_MCD_CM_DM_SZ);
        }
        else
        {
            if (tables.tm != 0)
                mcd_reg_writes(hdr, DPP_MCD_CM_TM_ADDR(0), tables.tm, DPP_MCD_CM_TM_SZ);
            if (tables.sc != 0)
                mcd_reg_writes(hdr, DPP_MCD_CM_SC_ADDR(0), tables.sc, DPP_MCD_CM_SC_SZ);
        }
    }
}

void mcd_cm_reg_set_params(struct mcd_hdr_device *hdr, struct mcd_cm_params_info *params)
{
    unsigned int set_bypass = 0;

    if (hdr->id == MCD_GF0 || hdr->id == MCD_GF1)
    {
        int src_pq  = (params->src_gamma == INDEX_GAMMA_ST2084) ? 1 : 0;
        int src_hlg = (params->src_gamma == INDEX_GAMMA_HLG   ) ? 1 : 0;
        int src_sdr = ((src_pq == 0)     && (src_hlg == 0)    ) ? 1 : 0;
        int dst_pq  = (params->dst_gamma == INDEX_GAMMA_ST2084) ? 1 : 0;
        int dst_hlg = (params->dst_gamma == INDEX_GAMMA_HLG   ) ? 1 : 0;
        int dst_sdr = ((dst_pq == 0)     && (dst_hlg == 0)    ) ? 1 : 0;

        int allow_case0 = ((src_sdr == 1) && (dst_sdr == 1)) ? 1 : 0;
        int allow_case1 = ((src_sdr == 1) && (dst_pq  == 1)) ? 1 : 0;
        int allow_case2 = ((src_hlg == 1) && (dst_hlg == 1)) ? 1 : 0;
        int allow_case  = ((allow_case0 == 1) || (allow_case1 == 1) || (allow_case2 == 1)) ? 1 : 0;

        if (allow_case == 0)
        {
            hdr_err("HDR:ERR:%sunsupported gamma type!!\n", __func__);
            set_bypass = 1;
        }
    }

    if (set_bypass)
        mcd_cm_sfr_bypass(hdr);
    else
        mcd_cm_sfr(hdr, params);
}


int mcd_cm_reg_reset(struct mcd_hdr_device *hdr)
{
    int ret = 0;

    mcd_reg_write(hdr, DPP_MCD_CM_CON_ADDR, 0);

    return ret;
}


int mcd_cm_reg_dump(struct mcd_hdr_device *hdr)
{
	int ret = 0;

	hdr_info("\n==== MCD IP SFR : %d ===\n", hdr->id);

	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			hdr->regs, 0x40, false);

	hdr_info("\n==== MCD IP SHD SFR : %d ===\n", hdr->id);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_ADDRESS, 32, 4,
			hdr->regs + DPP_MCD_CM_CON_ADDR_SHD, 0x40, false);

	return ret;
}

