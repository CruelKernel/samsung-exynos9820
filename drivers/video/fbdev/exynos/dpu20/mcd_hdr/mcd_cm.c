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


struct io_dataspace {
    enum gamut_index src_gamut;
    enum gamma_index src_gamma;
    enum gamut_index tgt_gamut;
    enum gamma_index tgt_gamma;
};

static unsigned int luminance2pqindex(unsigned int luminance)
{
    return (luminance <= 1000) ? INDEX_PQ1000 :
           (luminance <= 2000) ? INDEX_PQ2000 :
           (luminance <= 3000) ? INDEX_PQ3000 :
           (luminance <= 4000) ? INDEX_PQ4000 : INDEX_PQ1000;
}
static unsigned int luminance2targetindex(unsigned int luminance)
{
    return (luminance <= 200) ? INDEX_T0200 :
           (luminance <= 250) ? INDEX_T0250 :
           (luminance <= 300) ? INDEX_T0300 :
           (luminance <= 350) ? INDEX_T0350 :
           (luminance <= 400) ? INDEX_T0400 :
           (luminance <= 450) ? INDEX_T0450 :
           (luminance <= 500) ? INDEX_T0500 : INDEX_T0550;
}
void get_tables(unsigned int bypass, unsigned int isHDR10p, struct mcd_cm_params_info * params, struct io_dataspace * ds, struct cm_tables * tables)
{
    enum gamma_type_index i_type;
    enum gamma_type_index o_type;
    unsigned int idx_pq;
    unsigned int idx_td;
    //////////////////////////////////////////////////////////////////////////////////////////////
    if (bypass) {
        tables->eotf = TABLE_EOTF_BYPASS;
        tables->oetf = TABLE_OETF_BYPASS;
        tables->gm   = TABLE_GM_BYPASS;
        tables->sc   = TABLE_SC_BYPASS;
        tables->tm   = TABLE_TM_BYPASS;
        tables->tms  = TABLE_TMS_BYPASS;
        return;
    }
    //////////////////////////////////////////////////////////////////////////////////////////////
    i_type = (ds->src_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ :
             (ds->src_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
    o_type = (ds->tgt_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
             (ds->tgt_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
    idx_pq = luminance2pqindex(params->src_max_luminance);
    idx_td = luminance2targetindex(params->dst_max_luminance);
    //////////////////////////////////////////////////////////////////////////////////////////////
    // select predefined GM table
    tables->gm = TABLE_GM[ds->src_gamut][ds->tgt_gamut];
    // select predefined OETF table
    tables->oetf = TABLE_OETF[ds->tgt_gamma];
    // select predefined EOTF table
    tables->eotf = TABLE_EOTF[ds->src_gamma];
    // select predefined SC table
    tables->sc = TABLE_SC[idx_pq];
    // select predefined TM/TMS table
    if (i_type == INDEX_TYPE_PQ) {
        if (o_type == INDEX_TYPE_HLG) {
            tables->tm = TABLE_TM_PQ[INDEX_THLG][idx_pq];
            tables->tms = TABLE_TMS_PQ[INDEX_THLG];
        }
        else if (o_type == INDEX_TYPE_PQ) {
            tables->tm = TABLE_TM_PQ[INDEX_T1000][idx_pq];
            tables->tms = TABLE_TMS_PQ[INDEX_T1000];
        }
        else {
            if (isHDR10p == 0) {  // sperate HDR10 path
                tables->eotf = TABLE_EOTF_PQ[idx_pq];
                tables->tm   = TABLE_TM_PQ[idx_td][idx_pq];
                tables->tms  = TABLE_TMS_PQ_EXT[idx_td];
                tables->oetf = TABLE_OETF_EXT[ds->tgt_gamma][idx_pq];
            }
            else {
                tables->tm  = TABLE_TM_PQ[idx_td][idx_pq];
                tables->tms = TABLE_TMS_PQ[idx_td];
            }
        }
    }
    else {
        tables->tm = TABLE_TM[i_type][o_type];
        tables->tms = TABLE_TMS[i_type][o_type];
    }
}


void mcd_cm_sfr(struct mcd_hdr_device *hdr, struct mcd_cm_params_info *params, struct io_dataspace *ds)
{
    unsigned int legacy_mode;
    enum gamma_type_index i_type;
    enum gamma_type_index o_type;
    struct io_dataspace   act_ds;
    unsigned int is_hdr10 = 0;
    
    unsigned int remap_support      = 0;
    unsigned int suppress_test      = 0;
    unsigned int suppress_threshold = 0;
    unsigned int suppress_require   = 0;
    unsigned int hlgootf_require    = 0;
    unsigned int bypass_condition   = 0;

    struct cm_tables tables;
    unsigned int *table_dm = 0;
    unsigned int sfr_con   = 0;
    unsigned int en_all    = 0;
    unsigned int en_dither = 0;
    unsigned int en_alpha  = 0;
    unsigned int en_gamma  = 0;
    unsigned int en_gamut  = 0;
    unsigned int en_remap  = 0;
    unsigned int en_hdr10p = 0;
    //////////////////////////////////////////////////////////////////////////////////////////////
    legacy_mode = ((ds->tgt_gamma == INDEX_GAMMA_UNSPECIFIED) &&
                   (ds->tgt_gamut == INDEX_GAMUT_UNSPECIFIED)) ? 1 : 0;

    i_type      = (ds->src_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ :
                  (ds->src_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
    act_ds.src_gamma = ((legacy_mode == 1) && (i_type == INDEX_TYPE_SDR)) ? INDEX_GAMMA_GAMMA2_2 : ds->src_gamma;
    act_ds.src_gamut = ((legacy_mode == 1) && (i_type == INDEX_TYPE_SDR)) ? INDEX_GAMUT_DCI_P3   : ds->src_gamut;
    act_ds.tgt_gamma = ((legacy_mode == 1)                              ) ? INDEX_GAMMA_GAMMA2_2 : ds->tgt_gamma;
    act_ds.tgt_gamut = ((legacy_mode == 1)                              ) ? INDEX_GAMUT_DCI_P3   : ds->tgt_gamut;

    if ((act_ds.src_gamma == INDEX_GAMMA_UNSPECIFIED) ||
        (act_ds.tgt_gamma == INDEX_GAMMA_UNSPECIFIED) ||
        (act_ds.src_gamut == INDEX_GAMUT_UNSPECIFIED) ||
        (act_ds.tgt_gamut == INDEX_GAMUT_UNSPECIFIED))
    {   // bypass
        act_ds.src_gamma = INDEX_GAMMA_GAMMA2_2;
        act_ds.tgt_gamma = INDEX_GAMMA_GAMMA2_2;
        act_ds.src_gamut = INDEX_GAMUT_DCI_P3;
        act_ds.tgt_gamut = INDEX_GAMUT_DCI_P3;
    }

    i_type = (act_ds.src_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ :
             (act_ds.src_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
    o_type = (act_ds.tgt_gamma == INDEX_GAMMA_ST2084) ? INDEX_TYPE_PQ  :
             (act_ds.tgt_gamma == INDEX_GAMMA_HLG   ) ? INDEX_TYPE_HLG : INDEX_TYPE_SDR;
    //////////////////////////////////////////////////////////////////////////////////////////////
    is_hdr10 = ((i_type == INDEX_TYPE_PQ) && (act_ds.src_gamut == INDEX_GAMUT_BT2020)) ? 1 : 0;
    //////////////////////////////////////////////////////////////////////////////////////////////
    remap_support      = (hdr->id == MCD_GF0) || (hdr->id == MCD_GF1) ? 0 : 1;
    suppress_test      = (params->src_max_luminance > 0) ? params->src_max_luminance : HDR_REF_NIT;
    suppress_threshold = (o_type == INDEX_TYPE_SDR ) ? PNL_REF_NIT           : HDR_REF_NIT;
    suppress_require   = ( (i_type == INDEX_TYPE_PQ) && (suppress_test > suppress_threshold)) ? 1 : 0;
    hlgootf_require    = ((i_type != o_type) && ((i_type == INDEX_TYPE_HLG) || (o_type == INDEX_TYPE_HLG))) ? 1 : 0;
    //////////////////////////////////////////////////////////////////////////////////////////////
    en_remap = ((remap_support == 1) && ((suppress_require == 1) || (hlgootf_require == 1))) ? 1 : 0;
    en_gamut = ( act_ds.src_gamut != act_ds.tgt_gamut) ? 1 : 0;
    en_gamma = ((act_ds.src_gamma != act_ds.tgt_gamma) || (en_remap || en_gamut)) ? 1 : 0;
    //////////////////////////////////////////////////////////////////////////////////////////////
    if (hdr->id == MCD_VGRFS && params->hdr10p_lut != NULL)
    {// vgrfs layer is used and HDR10_lut is exist
        if (((*params->hdr10p_lut) > 0) && (is_hdr10 && (o_type == INDEX_TYPE_SDR)))
        {
            en_gamma  = 1;
            en_remap  = 0;
            en_hdr10p = ((*params->hdr10p_lut) == 1) ? 1 : 0;
        }
    }
    en_alpha  = (params->isAlphaPremultiplied && en_gamma);
    en_dither = params->needDither;
    en_all    = (params->needDither || en_gamma);
    //////////////////////////////////////////////////////////////////////////////////////////////
    // construct sfr
    bypass_condition = (!en_all) || (en_all && (!en_gamma));
    get_tables(bypass_condition, en_hdr10p, params, &act_ds, &tables);
    table_dm = (params->hdr10p_lut != NULL) ? (&params->hdr10p_lut[1]) : NULL;
    if (en_all == 1)
        sfr_con |= (CON_SFR_ALL);
    if ((en_gamma == 1) && (tables.eotf != 0 && tables.oetf != 0))
        sfr_con |= (CON_SFR_GAMMA);
    if ((en_gamut == 1) && (tables.gm != 0))
        sfr_con |= (CON_SFR_GAMUT);

    if ((en_hdr10p == 1) && (table_dm != NULL))
        sfr_con |= (CON_SFR_HDR10P);
    else if ((en_remap == 1) && (tables.tm != 0))
        sfr_con |= (CON_SFR_REMAP);

    if (en_alpha == 1)
        sfr_con |= (CON_SFR_ALPHA);
    if (en_dither == 1)
        sfr_con |= (CON_SFR_DITHER);
    //////////////////////////////////////////////////////////////////////////////////////////////
    // sfr write
    mcd_reg_write(hdr, DPP_MCD_CM_CON_ADDR, sfr_con);
    mcd_reg_write(hdr, DPP_MCD_CM_TMS_ADDR, tables.tms);

    if ((en_gamma == 1) && (tables.eotf != 0))
        mcd_reg_writes(hdr, DPP_MCD_CM_EOTF_ADDR(0), tables.eotf, DPP_MCD_CM_EOTF_SZ);

    if ((en_gamut == 1) && (tables.gm   != 0))
        mcd_reg_writes(hdr, DPP_MCD_CM_GM_ADDR(0), tables.gm, DPP_MCD_CM_GM_SZ);

    if ((en_gamma == 1) && (tables.oetf != 0))
        mcd_reg_writes(hdr, DPP_MCD_CM_OETF_ADDR(0), tables.oetf, DPP_MCD_CM_OETF_SZ);

    if ((en_hdr10p == 1) && (table_dm   != NULL)) {
        mcd_reg_writes(hdr, DPP_MCD_CM_DM_ADDR(0), table_dm, DPP_MCD_CM_DM_SZ);
    }
    else if (en_remap == 1) {
		if (tables.tm != 0)
			mcd_reg_writes(hdr, DPP_MCD_CM_TM_ADDR(0), tables.tm, DPP_MCD_CM_TM_SZ);
        if (tables.sc != 0)
            mcd_reg_writes(hdr, DPP_MCD_CM_SC_ADDR(0), tables.sc, DPP_MCD_CM_SC_SZ);
    }
}

void mcd_cm_sfr_bypass(struct mcd_hdr_device *hdr, struct mcd_cm_params_info * params)
{
    unsigned int sfr_con   = 0;
    unsigned int en_all    = 0;
    unsigned int en_dither = 0;
    //////////////////////////////////////////////////////////////////////////////////////////////
    en_dither = params->needDither ? 1 : 0;
    en_all = params->needDither ? 1 : 0;
    //////////////////////////////////////////////////////////////////////////////////////////////
    if (en_all)
        sfr_con |= (CON_SFR_ALL);
    if (en_dither)
        sfr_con |= (CON_SFR_DITHER);
    //////////////////////////////////////////////////////////////////////////////////////////////
    mcd_reg_write(hdr, DPP_MCD_CM_CON_ADDR, sfr_con);
    mcd_reg_write(hdr, DPP_MCD_CM_TMS_ADDR, TABLE_TMS_BYPASS);
}

void mcd_cm_reg_set_params(struct mcd_hdr_device *hdr, struct mcd_cm_params_info *params)
{
    struct io_dataspace ds;
    unsigned int is_src_hlg;
    unsigned int is_src_pq;
    unsigned int is_diff_gamut;
    unsigned int is_diff_gamma;
    unsigned int is_diff_tone;
    unsigned int set_bypass = 0;

	ds.src_gamma = params->src_gamma;
	ds.src_gamut = params->src_gamut;
	ds.tgt_gamma = params->dst_gamma;
	ds.tgt_gamut = params->dst_gamut;

    is_src_hlg = (ds.src_gamma == INDEX_GAMMA_HLG) ? 1 : 0;
    is_src_pq  = (ds.src_gamma == INDEX_GAMMA_ST2084) ? 1 : 0;

    if (hdr->id == MCD_GF0 || hdr->id == MCD_GF1) {
        if (is_src_pq || is_src_hlg) {
            is_diff_gamut = (ds.src_gamut != ds.tgt_gamut) ? 1 : 0;
            is_diff_gamma = (ds.src_gamma != ds.tgt_gamma) ? 1 : 0;
            is_diff_tone  = (params->src_max_luminance > HDR_REF_NIT) ? 1 : 0;

            if (is_diff_gamut | is_diff_gamma | is_diff_tone) {
                hdr_err("HDR:ERR:%sunsupported gamma, gamut type!!\n", __func__);
                set_bypass = 1;
            }
        }
    } 

    if (set_bypass)
        mcd_cm_sfr_bypass(hdr, params);
    else
        mcd_cm_sfr(hdr, params, &ds);
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

