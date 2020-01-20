#include "../cmucal.h"
#include "cmucal-node.h"
#include "cmucal-vclk.h"

#include "cmucal-vclklut.h"

/*=================CMUCAL version: S5E9810================================*/

/*=================CLK in each VCLK================================*/


/* DVFS List */
enum clk_id cmucal_vclk_vddi[] = {
	DIV_CLK_AUD_AUDIF,
	DIV_PLL_SHARED0_DIV2,
	CLKCMU_G3D_SWITCH,
	CLKCMU_FSYS0_BUS,
	CLKCMU_BUS1_BUS,
	DIV_PLL_SHARED2_DIV2,
	DIV_PLL_SHARED3_DIV2,
	DIV_PLL_SHARED4_DIV2,
	DIV_PLL_SHARED0_DIV4,
	CLKCMU_MFC_BUS,
	CLKCMU_G2D_G2D,
	CLKCMU_FSYS1_MMC_CARD,
	CLKCMU_FSYS1_BUS,
	CLKCMU_CMGP_BUS,
	CLKCMU_DSPM_BUS,
	CLKCMU_BUSC_BUS,
	CLKCMU_CORE_BUS,
	CLKCMU_ISPPRE_BUS,
	CLKCMU_ISPLP_BUS,
	CLKCMU_ISPHQ_BUS,
	CLKCMU_G2D_MSCL,
	CLKCMU_CPUCL0_DBG_BUS,
	CLKCMU_IVA_BUS,
	CLKCMU_DCRD_BUS,
	CLKCMU_CHUB_BUS,
	CLKCMU_DCF_BUS,
	CLKCMU_VTS_BUS,
	CLKCMU_ISPLP_VRA,
	CLKCMU_MFC_WFD,
	CLKCMU_MIF_BUSP,
	CLKCMU_DCPOST_BUS,
	CLKCMU_DSPS_AUD,
	CLKCMU_DPU_BUS,
	DIV_CLKCMU_DPU_BUS,
	MUX_CLKCMU_MIF_SWITCH,
	DIV_CLK_CMU_CMUREF,
	DIV_CLKCMU_DPU,
	MUX_CLKCMU_BUS1_BUS,
	MUX_CLKCMU_MFC_BUS,
	MUX_CLKCMU_CMGP_BUS,
	MUX_CLKCMU_BUSC_BUS,
	MUX_CLKCMU_G2D_G2D,
	MUX_CLKCMU_DSPM_BUS,
	MUX_CLKCMU_CORE_BUS,
	MUX_CLKCMU_ISPPRE_BUS,
	MUX_CLKCMU_ISPLP_BUS,
	MUX_CLKCMU_ISPHQ_BUS,
	MUX_CLKCMU_G2D_MSCL,
	MUX_CLKCMU_HPM,
	MUX_CLKCMU_CPUCL0_DBG_BUS,
	MUX_CLKCMU_FSYS0_BUS,
	MUX_CLKCMU_IVA_BUS,
	MUX_CLKCMU_DCRD_BUS,
	MUX_CLKCMU_DCF_BUS,
	MUX_CLKCMU_APM_BUS,
	MUX_CLKCMU_FSYS1_BUS,
	MUX_CLK_CMU_CMUREF,
	MUX_CLKCMU_ISPLP_VRA,
	MUX_CLKCMU_MFC_WFD,
	MUX_CLKCMU_PERIC0_IP,
	MUX_CLKCMU_PERIC1_IP,
	MUX_CLKCMU_DCPOST_BUS,
	MUX_CLKCMU_ISPLP_GDC,
	MUX_CLKCMU_DSPS_AUD,
	DIV_PLL_SHARED1_DIV2,
	DIV_PLL_SHARED1_DIV4,
	DIV_PLL_SHARED1_DIV3,
	DIV_PLL_SHARED0_DIV3,
};
enum clk_id cmucal_vclk_vdd_mif[] = {
	PLL_MIF,
};
enum clk_id cmucal_vclk_vdd_g3d[] = {
	PLL_G3D,
};
enum clk_id cmucal_vclk_vdd_cam[] = {
	DIV_CLK_IVA_DEBUG,
};

/* SPECIAL List */
enum clk_id cmucal_vclk_spl_clk_usi_cmgp02_blk_apm[] = {
	CLKCMU_APM_DLL_CMGP,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp00_blk_apm[] = {
	CLKCMU_APM_DLL_CMGP,
};
enum clk_id cmucal_vclk_clk_cmgp_adc_blk_apm[] = {
	CLKCMU_APM_DLL_CMGP,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp03_blk_apm[] = {
	CLKCMU_APM_DLL_CMGP,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp01_blk_apm[] = {
	CLKCMU_APM_DLL_CMGP,
};
enum clk_id cmucal_vclk_spl_clk_aud_uaif0_blk_aud[] = {
	MUX_CLK_AUD_UAIF0,
	DIV_CLK_AUD_UAIF0,
};
enum clk_id cmucal_vclk_spl_clk_aud_uaif2_blk_aud[] = {
	MUX_CLK_AUD_UAIF2,
	DIV_CLK_AUD_UAIF2,
};
enum clk_id cmucal_vclk_spl_clk_aud_cpu_pclkdbg_blk_aud[] = {
	DIV_CLK_AUD_CPU_PCLKDBG,
};
enum clk_id cmucal_vclk_spl_clk_aud_uaif1_blk_aud[] = {
	MUX_CLK_AUD_UAIF1,
	DIV_CLK_AUD_UAIF1,
};
enum clk_id cmucal_vclk_spl_clk_aud_uaif3_blk_aud[] = {
	MUX_CLK_AUD_UAIF3,
	DIV_CLK_AUD_UAIF3,
};
enum clk_id cmucal_vclk_div_clk_aud_dmic_blk_aud[] = {
	DIV_CLK_AUD_DMIC,
};
enum clk_id cmucal_vclk_spl_clk_chub_usi01_blk_chub[] = {
	DIV_CLK_CHUB_USI01,
	MUX_CLK_CHUB_USI01,
};
enum clk_id cmucal_vclk_spl_clk_chub_usi00_blk_chub[] = {
	DIV_CLK_CHUB_USI00,
	MUX_CLK_CHUB_USI00,
};
enum clk_id cmucal_vclk_mux_clk_chub_timer_fclk_blk_chub[] = {
	CLK_CHUB_TIMER_FCLK,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp02_blk_cmgp[] = {
	DIV_CLK_USI_CMGP02,
	MUX_CLK_USI_CMGP02,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp00_blk_cmgp[] = {
	DIV_CLK_USI_CMGP00,
	MUX_CLK_USI_CMGP00,
};
enum clk_id cmucal_vclk_clk_cmgp_adc_blk_cmgp[] = {
	CLK_CMGP_ADC,
	DIV_CLK_CMGP_ADC,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp01_blk_cmgp[] = {
	DIV_CLK_USI_CMGP01,
	MUX_CLK_USI_CMGP01,
};
enum clk_id cmucal_vclk_spl_clk_usi_cmgp03_blk_cmgp[] = {
	DIV_CLK_USI_CMGP03,
	MUX_CLK_USI_CMGP03,
};
enum clk_id cmucal_vclk_clk_fsys0_ufs_embd_blk_cmu[] = {
	CLKCMU_FSYS0_UFS_EMBD,
	MUX_CLKCMU_FSYS0_UFS_EMBD,
};
enum clk_id cmucal_vclk_clk_fsys1_mmc_card_blk_cmu[] = {
	MUX_CLKCMU_FSYS1_MMC_CARD,
};
enum clk_id cmucal_vclk_clkcmu_hpm_blk_cmu[] = {
	CLKCMU_HPM,
};
enum clk_id cmucal_vclk_spl_clk_chub_usi01_blk_cmu[] = {
	MUX_CLKCMU_CHUB_BUS,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk0_blk_cmu[] = {
	CLKCMU_CIS_CLK0,
	MUX_CLKCMU_CIS_CLK0,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk2_blk_cmu[] = {
	CLKCMU_CIS_CLK2,
	MUX_CLKCMU_CIS_CLK2,
};
enum clk_id cmucal_vclk_clk_fsys0_dpgtc_blk_cmu[] = {
	CLKCMU_FSYS0_DPGTC,
	MUX_CLKCMU_FSYS0_DPGTC,
};
enum clk_id cmucal_vclk_clk_fsys0_usb30drd_blk_cmu[] = {
	CLKCMU_FSYS0_USB30DRD,
	MUX_CLKCMU_FSYS0_USB30DRD,
};
enum clk_id cmucal_vclk_mux_clkcmu_fsys0_usbdp_debug_user_blk_cmu[] = {
	MUX_CLKCMU_FSYS0_USBDP_DEBUG,
};
enum clk_id cmucal_vclk_clk_fsys1_ufs_card_blk_cmu[] = {
	CLKCMU_FSYS1_UFS_CARD,
	MUX_CLKCMU_FSYS1_UFS_CARD,
};
enum clk_id cmucal_vclk_occ_mif_cmuref_blk_cmu[] = {
	MUX_CLKCMU_MIF_BUSP,
};
enum clk_id cmucal_vclk_clk_peric0_usi01_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi04_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi13_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam1_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_spi_cam0_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_usi07_usi_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_usi10_usi_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_occ_cmu_cmuref_blk_cmu[] = {
	MUX_CMU_CMUREF,
};
enum clk_id cmucal_vclk_spl_clk_chub_usi00_blk_cmu[] = {
	MUX_CLKCMU_CHUB_BUS,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk1_blk_cmu[] = {
	CLKCMU_CIS_CLK1,
	MUX_CLKCMU_CIS_CLK1,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk3_blk_cmu[] = {
	CLKCMU_CIS_CLK3,
	MUX_CLKCMU_CIS_CLK3,
};
enum clk_id cmucal_vclk_clk_peric0_usi00_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi05_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam0_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_uart_bt_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_usi09_usi_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric0_uart_dbg_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi12_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam3_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_usi11_usi_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi02_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam2_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi03_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_clk_peric1_usi08_usi_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric1_usi06_usi_blk_cmu[] = {
	CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_clk_peric0_usi14_usi_blk_cmu[] = {
	CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_mux_clk_cluster0_sclk_blk_cpucl0[] = {
	MUX_CLK_CLUSTER0_SCLK,
};
enum clk_id cmucal_vclk_spl_clk_cpucl0_cmuref_blk_cpucl0[] = {
	DIV_CLK_CPUCL0_CMUREF,
};
enum clk_id cmucal_vclk_clk_cluster0_aclkp_blk_cpucl0[] = {
	MUX_CLK_CLUSTER0_ACLKP,
	DIV_CLK_CLUSTER0_ACLKP,
};
enum clk_id cmucal_vclk_div_clk_cluster0_periphclk_blk_cpucl0[] = {
	DIV_CLK_CLUSTER0_PERIPHCLK,
};
enum clk_id cmucal_vclk_clk_cluster0_aclk_blk_cpucl0[] = {
	MUX_CLK_CLUSTER0_ACLK,
	DIV_CLK_CLUSTER0_ACLK,
};
enum clk_id cmucal_vclk_div_clk_cluster1_aclk_blk_cpucl1[] = {
	DIV_CLK_CLUSTER1_ACLK,
};
enum clk_id cmucal_vclk_div_clk_cpucl1_pclkdbg_blk_cpucl1[] = {
	DIV_CLK_CPUCL1_PCLKDBG,
};
enum clk_id cmucal_vclk_div_clk_cluster1_atclk_blk_cpucl1[] = {
	DIV_CLK_CLUSTER1_ATCLK,
};
enum clk_id cmucal_vclk_spl_clk_cpucl1_cmuref_blk_cpucl1[] = {
	DIV_CLK_CPUCL1_CMUREF,
};
enum clk_id cmucal_vclk_occ_mif_cmuref_blk_mif[] = {
	MUX_MIF_CMUREF,
};
enum clk_id cmucal_vclk_clk_peric0_uart_dbg_blk_peric0[] = {
	DIV_CLK_PERIC0_UART_DBG,
};
enum clk_id cmucal_vclk_clk_peric0_usi01_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI01_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi03_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI03_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi05_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI05_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi13_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI13_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi04_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI04_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi02_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI02_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi12_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI12_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi00_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI00_USI,
};
enum clk_id cmucal_vclk_clk_peric0_usi14_usi_blk_peric0[] = {
	DIV_CLK_PERIC0_USI14_USI,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam0_blk_peric1[] = {
	DIV_CLK_PERIC1_I2C_CAM0,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam2_blk_peric1[] = {
	DIV_CLK_PERIC1_I2C_CAM2,
};
enum clk_id cmucal_vclk_clk_peric1_spi_cam0_blk_peric1[] = {
	DIV_CLK_PERIC1_SPI_CAM0,
};
enum clk_id cmucal_vclk_clk_peric1_usi06_usi_blk_peric1[] = {
	DIV_CLK_PERIC1_USI06_USI,
};
enum clk_id cmucal_vclk_clk_peric1_usi08_usi_blk_peric1[] = {
	DIV_CLK_PERIC1_USI08_USI,
};
enum clk_id cmucal_vclk_clk_peric1_usi10_usi_blk_peric1[] = {
	DIV_CLK_PERIC1_USI10_USI,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam1_blk_peric1[] = {
	DIV_CLK_PERIC1_I2C_CAM1,
};
enum clk_id cmucal_vclk_clk_peric1_usi07_usi_blk_peric1[] = {
	DIV_CLK_PERIC1_USI07_USI,
};
enum clk_id cmucal_vclk_clk_peric1_uart_bt_blk_peric1[] = {
	DIV_CLK_PERIC1_UART_BT,
};
enum clk_id cmucal_vclk_clk_peric1_usi09_usi_blk_peric1[] = {
	DIV_CLK_PERIC1_USI09_USI,
};
enum clk_id cmucal_vclk_clk_peric1_i2c_cam3_blk_peric1[] = {
	DIV_CLK_PERIC1_I2C_CAM3,
};
enum clk_id cmucal_vclk_clk_peric1_usi11_usi_blk_peric1[] = {
	DIV_CLK_PERIC1_USI11_USI,
};
enum clk_id cmucal_vclk_spl_clk_vts_dmic_if_div2_blk_vts[] = {
	DIV_CLK_VTS_DMIC_DIV2,
};
enum clk_id cmucal_vclk_div_clk_vts_dmic_blk_vts[] = {
	DIV_CLK_VTS_DMIC,
};

/* COMMON List */
enum clk_id cmucal_vclk_blk_apm[] = {
	DIV_CLK_APM_BUS,
	MUX_CLK_APM_BUS,
	CLKCMU_APM_DLL_VTS,
};
enum clk_id cmucal_vclk_blk_aud[] = {
	DIV_CLK_AUD_CPU_ATCLK,
	DIV_CLK_AUD_CPU_ACLK,
	DIV_CLK_AUD_BUS,
	DIV_CLK_AUD_BUSP,
	PLL_AUD,
	DIV_CLK_AUD_PLL,
	DIV_CLK_AUD_DSIF,
};
enum clk_id cmucal_vclk_blk_busc[] = {
	DIV_CLK_BUSC_BUSP,
};
enum clk_id cmucal_vclk_blk_chub[] = {
	DIV_CLK_CHUB_I2C,
	DIV_CLK_CHUB_BUS,
	MUX_CLK_CHUB_I2C,
	MUX_CLK_CHUB_BUS,
};
enum clk_id cmucal_vclk_blk_cmgp[] = {
	DIV_CLK_I2C_CMGP,
	MUX_CLK_I2C_CMGP,
	MUX_CLK_CMGP_BUS,
};
enum clk_id cmucal_vclk_blk_cmu[] = {
	CLKCMU_APM_BUS,
	CLKCMU_PERIC0_BUS,
	CLKCMU_PERIS_BUS,
	CLKCMU_PERIC1_BUS,
	PLL_SHARED3,
	CLKCMU_MODEM_SHARED0,
	CLKCMU_MODEM_SHARED1,
	CLKCMU_ISPLP_GDC,
	MUX_CLKCMU_PERIC0_BUS,
	MUX_CLKCMU_PERIC1_BUS,
	MUX_CLKCMU_PERIS_BUS,
	MUX_CLKCMU_FSYS1_PCIE,
	MUX_CLKCMU_VTS_BUS,
	MUX_CLKCMU_DPU_BUS,
	PLL_SHARED4,
	PLL_MMC,
	PLL_SHARED2,
	PLL_SHARED0,
	PLL_SHARED1,
};
enum clk_id cmucal_vclk_blk_core[] = {
	DIV_CLK_CORE_BUSP,
};
enum clk_id cmucal_vclk_blk_cpucl0[] = {
	DIV_CLK_CLUSTER0_ATCLK,
	DIV_CLK_CLUSTER0_PCLKDBG,
	DIV_CLK_CPUCL0_DBG_PCLKDBG,
	DIV_CLK_CPUCL0_PCLK,
	PLL_CPUCL0,
};
enum clk_id cmucal_vclk_blk_cpucl1[] = {
	DIV_CLK_CPUCL1_PCLK,
	PLL_CPUCL1,
};
enum clk_id cmucal_vclk_blk_dcf[] = {
	DIV_CLK_DCF_BUSP,
};
enum clk_id cmucal_vclk_blk_dcpost[] = {
	DIV_CLK_DCPOST_BUSP,
};
enum clk_id cmucal_vclk_blk_dcrd[] = {
	DIV_CLK_DCRD_BUSP,
	DIV_CLK_DCRD_BUSD_HALF,
};
enum clk_id cmucal_vclk_blk_dpu[] = {
	DIV_CLK_DPU_BUSP,
};
enum clk_id cmucal_vclk_blk_dspm[] = {
	DIV_CLK_DSPM_BUSP,
};
enum clk_id cmucal_vclk_blk_dsps[] = {
	DIV_CLK_DSPS_BUSP,
	MUX_CLK_DSPS_BUS,
};
enum clk_id cmucal_vclk_blk_g2d[] = {
	DIV_CLK_G2D_BUSP,
};
enum clk_id cmucal_vclk_blk_g3d[] = {
	DIV_CLK_G3D_BUSP,
	MUX_CLK_G3D_BUSD,
};
enum clk_id cmucal_vclk_blk_isphq[] = {
	DIV_CLK_ISPHQ_BUSP,
};
enum clk_id cmucal_vclk_blk_isplp[] = {
	DIV_CLK_ISPLP_BUSP,
};
enum clk_id cmucal_vclk_blk_isppre[] = {
	DIV_CLK_ISPPRE_BUSP,
};
enum clk_id cmucal_vclk_blk_iva[] = {
	DIV_CLK_IVA_BUSP,
};
enum clk_id cmucal_vclk_blk_mfc[] = {
	DIV_CLK_MFC_BUSP,
};
enum clk_id cmucal_vclk_blk_mif[] = {
	DIV_CLK_MIF_PRE,
	CLKMUX_MIF_DDRPHY2X,
};
enum clk_id cmucal_vclk_blk_peric0[] = {
	DIV_CLK_PERIC0_USI_I2C,
};
enum clk_id cmucal_vclk_blk_peric1[] = {
	DIV_CLK_PERIC1_USI_I2C,
};
enum clk_id cmucal_vclk_blk_s2d[] = {
	MUX_CLK_S2D_CORE,
	CLKCMU_MIF_DDRPHY2X_S2D,
	PLL_MIF_S2D,
};
enum clk_id cmucal_vclk_blk_vts[] = {
	DIV_CLK_VTS_BUS,
	MUX_CLK_VTS_BUS,
	DIV_CLK_VTS_DMIC_IF,
};

/* GATING List */
enum clk_id cmucal_vclk_apbif_gpio_alive[] = {
	GOUT_BLK_APM_UID_APBIF_GPIO_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbif_pmu_alive[] = {
	GOUT_BLK_APM_UID_APBIF_PMU_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbif_rtc[] = {
	GOUT_BLK_APM_UID_APBIF_RTC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbif_top_rtc[] = {
	GOUT_BLK_APM_UID_APBIF_TOP_RTC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apm_cmu_apm[] = {
	CLK_BLK_APM_UID_APM_CMU_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_grebeintegration[] = {
	GOUT_BLK_APM_UID_GREBEINTEGRATION_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_intmem[] = {
	GOUT_BLK_APM_UID_INTMEM_IPCLKPORT_ACLK,
	GOUT_BLK_APM_UID_INTMEM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_apm[] = {
	GOUT_BLK_APM_UID_LHM_AXI_P_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_apm_chub[] = {
	GOUT_BLK_APM_UID_LHM_AXI_P_APM_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_apm_cp[] = {
	GOUT_BLK_APM_UID_LHM_AXI_P_APM_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_apm_gnss[] = {
	GOUT_BLK_APM_UID_LHM_AXI_P_APM_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_apm[] = {
	GOUT_BLK_APM_UID_LHS_AXI_D_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g_scan2dram[] = {
	GOUT_BLK_APM_UID_LHS_AXI_G_SCAN2DRAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_lp_chub[] = {
	GOUT_BLK_APM_UID_LHS_AXI_LP_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_apm2cmgp[] = {
	GOUT_BLK_APM_UID_LHS_AXI_P_APM2CMGP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mailbox_ap2chub[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP2CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_ap2cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_ap2cp_s[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP2CP_S_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_ap2gnss[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP2GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_ap2vts[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP2VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_apm2ap[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_apm2chub[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_apm2cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_apm2gnss[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_chub2cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_CHUB2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_gnss2chub[] = {
	GOUT_BLK_APM_UID_MAILBOX_GNSS2CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_gnss2cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_GNSS2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pem[] = {
	GOUT_BLK_APM_UID_PEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pgen_apm[] = {
	GOUT_BLK_APM_UID_PGEN_APM_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_pmu_intr_gen[] = {
	GOUT_BLK_APM_UID_PMU_INTR_GEN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_speedy_apm[] = {
	GOUT_BLK_APM_UID_SPEEDY_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_speedy_sub_apm[] = {
	GOUT_BLK_APM_UID_SPEEDY_SUB_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_apm[] = {
	GOUT_BLK_APM_UID_SYSREG_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wdt_apm[] = {
	GOUT_BLK_APM_UID_WDT_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_dp_apm[] = {
	GOUT_BLK_APM_UID_XIU_DP_APM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_abox[] = {
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_DSIF,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF0,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF1,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF2,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF3,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_CCLK_ATB,
};
enum clk_id cmucal_vclk_abox_dap[] = {
	GOUT_BLK_AUD_UID_ABOX_DAP_IPCLKPORT_dapclk,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_aud[] = {
	GOUT_BLK_AUD_UID_AD_APB_SYSMMU_AUD_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_aud[] = {
	GOUT_BLK_AUD_UID_AXI2APB_AUD_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_dftmux_aud[] = {
	CLK_BLK_AUD_UID_DFTMUX_AUD_IPCLKPORT_AUD_CODEC_MCLK,
};
enum clk_id cmucal_vclk_dmic[] = {
	CLK_BLK_AUD_UID_DMIC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_aud[] = {
	GOUT_BLK_AUD_UID_LHM_AXI_P_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_aud[] = {
	GOUT_BLK_AUD_UID_LHS_ATB_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_peri_axi_asb[] = {
	GOUT_BLK_AUD_UID_PERI_AXI_ASB_IPCLKPORT_ACLKS,
};
enum clk_id cmucal_vclk_xiu_p_aud[] = {
	GOUT_BLK_AUD_UID_XIU_P_AUD_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_bus1[] = {
	GOUT_BLK_BUS1_UID_AXI2APB_BUS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_bus1_trex[] = {
	GOUT_BLK_BUS1_UID_AXI2APB_BUS1_TREX_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_baaw_p_chub[] = {
	GOUT_BLK_BUS1_UID_BAAW_P_CHUB_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_baaw_p_gnss[] = {
	GOUT_BLK_BUS1_UID_BAAW_P_GNSS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_bus1_cmu_bus1[] = {
	CLK_BLK_BUS1_UID_BUS1_CMU_BUS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_apm[] = {
	GOUT_BLK_BUS1_UID_LHM_AXI_D_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_chub[] = {
	GOUT_BLK_BUS1_UID_LHM_AXI_D_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_gnss[] = {
	GOUT_BLK_BUS1_UID_LHM_AXI_D_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_g_cssys[] = {
	GOUT_BLK_BUS1_UID_LHM_AXI_G_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_bus1[] = {
	GOUT_BLK_BUS1_UID_LHS_AXI_D_BUS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_chub[] = {
	GOUT_BLK_BUS1_UID_LHS_AXI_P_CHUB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cssys[] = {
	GOUT_BLK_BUS1_UID_LHS_AXI_P_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_gnss[] = {
	GOUT_BLK_BUS1_UID_LHS_AXI_P_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_sysreg_bus1[] = {
	GOUT_BLK_BUS1_UID_SYSREG_BUS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_p_bus1[] = {
	GOUT_BLK_BUS1_UID_TREX_P_BUS1_IPCLKPORT_PCLK_BUS1,
	GOUT_BLK_BUS1_UID_TREX_P_BUS1_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_xiu_d_bus1[] = {
	GOUT_BLK_BUS1_UID_XIU_D_BUS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_hpm_busc[] = {
	CLK_BLK_BUSC_UID_HPM_BUSC_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_i2c_chub00[] = {
	GOUT_BLK_CHUB_UID_I2C_CHUB00_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_i2c_chub01[] = {
	GOUT_BLK_CHUB_UID_I2C_CHUB01_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_usi_chub00[] = {
	GOUT_BLK_CHUB_UID_USI_CHUB00_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_usi_chub01[] = {
	GOUT_BLK_CHUB_UID_USI_CHUB01_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_adc_cmgp[] = {
	GOUT_BLK_CMGP_UID_ADC_CMGP_IPCLKPORT_PCLK_S0,
	GOUT_BLK_CMGP_UID_ADC_CMGP_IPCLKPORT_PCLK_S1,
};
enum clk_id cmucal_vclk_axi2apb_cmgp0[] = {
	GOUT_BLK_CMGP_UID_AXI2APB_CMGP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_cmgp1[] = {
	GOUT_BLK_CMGP_UID_AXI2APB_CMGP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_cmgp_cmu_cmgp[] = {
	CLK_BLK_CMGP_UID_CMGP_CMU_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gpio_cmgp[] = {
	GOUT_BLK_CMGP_UID_GPIO_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cmgp00[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP00_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP00_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cmgp01[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP01_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP01_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cmgp02[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP02_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP02_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cmgp03[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP03_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP03_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_apm2cmgp[] = {
	GOUT_BLK_CMGP_UID_LHM_AXI_P_APM2CMGP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_sysreg_cmgp[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cmgp2chub[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cmgp2cp[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cmgp2gnss[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cmgp2pmu_ap[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2PMU_AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cmgp2pmu_chub[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2PMU_CHUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi_cmgp00[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP00_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP00_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi_cmgp01[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP01_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP01_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi_cmgp02[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP02_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP02_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi_cmgp03[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP03_IPCLKPORT_IPCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP03_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_cmgp[] = {
	GOUT_BLK_CMGP_UID_XIU_P_CMGP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_adm_apb_g_bdu[] = {
	GOUT_BLK_CORE_UID_ADM_APB_G_BDU_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_apb_async_ppfw_dp[] = {
	GOUT_BLK_CORE_UID_APB_ASYNC_PPFW_DP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_apb_async_ppfw_g3d[] = {
	GOUT_BLK_CORE_UID_APB_ASYNC_PPFW_G3D_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_apb_async_ppfw_io[] = {
	GOUT_BLK_CORE_UID_APB_ASYNC_PPFW_IO_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_core[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_core_tp[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_TP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_baaw_cp[] = {
	GOUT_BLK_CORE_UID_BAAW_CP_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_bdu[] = {
	GOUT_BLK_CORE_UID_BDU_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_bps_p_g3d[] = {
	GOUT_BLK_CORE_UID_BPS_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_busif_hpmcore[] = {
	GOUT_BLK_CORE_UID_BUSIF_HPMCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_cci[] = {
	GOUT_BLK_CORE_UID_CCI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_core_cmu_core[] = {
	CLK_BLK_CORE_UID_CORE_CMU_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_hpm_core[] = {
	CLK_BLK_CORE_UID_HPM_CORE_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhs_axi_p_apm[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cp[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cpucl0[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cpucl1[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CPUCL1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_g3d[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ppcfw_g3d[] = {
	GOUT_BLK_CORE_UID_PPCFW_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmuppc_cci[] = {
	GOUT_BLK_CORE_UID_PPMUPPC_CCI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_cpucl0[] = {
	GOUT_BLK_CORE_UID_PPMU_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_cpucl1[] = {
	GOUT_BLK_CORE_UID_PPMU_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_g3d0[] = {
	GOUT_BLK_CORE_UID_PPMU_G3D0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_g3d1[] = {
	GOUT_BLK_CORE_UID_PPMU_G3D1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_g3d2[] = {
	GOUT_BLK_CORE_UID_PPMU_G3D2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_g3d3[] = {
	GOUT_BLK_CORE_UID_PPMU_G3D3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_core[] = {
	GOUT_BLK_CORE_UID_SYSREG_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_d_core[] = {
	GOUT_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_trex_p0_core[] = {
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_PCLK_CORE,
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_trex_p1_core[] = {
	GOUT_BLK_CORE_UID_TREX_P1_CORE_IPCLKPORT_PCLK_CORE,
	GOUT_BLK_CORE_UID_TREX_P1_CORE_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_adm_apb_g_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_ADM_APB_G_CLUSTER0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ads_ahb_g_sss[] = {
	GOUT_BLK_CPUCL0_UID_ADS_AHB_G_SSS_IPCLKPORT_HCLKS,
};
enum clk_id cmucal_vclk_ads_apb_g_bdu[] = {
	GOUT_BLK_CPUCL0_UID_ADS_APB_G_BDU_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ads_apb_g_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_ADS_APB_G_CLUSTER0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ads_apb_g_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_ADS_APB_G_CLUSTER1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ads_apb_g_dspm[] = {
	GOUT_BLK_CPUCL0_UID_ADS_APB_G_DSPM_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_asb_apb_p_dumppc_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_ASB_APB_P_DUMPPC_CLUSTER0_IPCLKPORT_PCLKM,
	GOUT_BLK_CPUCL0_UID_ASB_APB_P_DUMPPC_CLUSTER0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_asb_apb_p_dumppc_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_ASB_APB_P_DUMPPC_CLUSTER1_IPCLKPORT_PCLKM,
	GOUT_BLK_CPUCL0_UID_ASB_APB_P_DUMPPC_CLUSTER1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_AXI2APB_CPUCL0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_p_cssys[] = {
	GOUT_BLK_CPUCL0_UID_AXI2APB_P_CSSYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi_ds_64to32_g_cssys[] = {
	GOUT_BLK_CPUCL0_UID_AXI_DS_64to32_G_CSSYS_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_busif_hpmcpucl0[] = {
	GOUT_BLK_CPUCL0_UID_BUSIF_HPMCPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_cluster0[] = {
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_ATCLK,
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_PCLK,
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_PERIPHCLK,
	CLK_BLK_CPUCL0_UID_CLUSTER0_IPCLKPORT_SCLK,
};
enum clk_id cmucal_vclk_cpucl0_cmu_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_CPUCL0_CMU_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_cssys[] = {
	GOUT_BLK_CPUCL0_UID_CSSYS_IPCLKPORT_ATCLK,
	GOUT_BLK_CPUCL0_UID_CSSYS_IPCLKPORT_PCLKDBG,
};
enum clk_id cmucal_vclk_dumppc_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_DUMPPC_CLUSTER0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dumppc_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_DUMPPC_CLUSTER1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_hpm_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_HPM_CPUCL0_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_atb_t0_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t0_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T0_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t1_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t1_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T1_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t2_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T2_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t2_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T2_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t3_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T3_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t3_cluster1[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T3_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t_aud[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t_bdu[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T_BDU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_cssys[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_P_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_ace_d_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ACE_D_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_t0_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_t1_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_t2_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T2_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_t3_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T3_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g_cssys[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_G_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g_etr[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_G_ETR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_P_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_secjtag[] = {
	GOUT_BLK_CPUCL0_UID_SECJTAG_IPCLKPORT_i_clk,
};
enum clk_id cmucal_vclk_sysreg_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_SYSREG_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_cpucl1[] = {
	GOUT_BLK_CPUCL1_UID_AXI2APB_CPUCL1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_hpmcpucl1[] = {
	GOUT_BLK_CPUCL1_UID_BUSIF_HPMCPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_cluster1[] = {
	CLK_BLK_CPUCL1_UID_CLUSTER1_IPCLKPORT_PCLKDBG,
};
enum clk_id cmucal_vclk_cpucl1_cmu_cpucl1[] = {
	CLK_BLK_CPUCL1_UID_CPUCL1_CMU_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_hpm_cpucl1_0[] = {
	CLK_BLK_CPUCL1_UID_HPM_CPUCL1_0_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_hpm_cpucl1_1[] = {
	CLK_BLK_CPUCL1_UID_HPM_CPUCL1_1_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_hpm_cpucl1_2[] = {
	CLK_BLK_CPUCL1_UID_HPM_CPUCL1_2_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_cpucl1[] = {
	GOUT_BLK_CPUCL1_UID_LHM_AXI_P_CPUCL1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_sysreg_cpucl1[] = {
	GOUT_BLK_CPUCL1_UID_SYSREG_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dcpost_cmu_dcpost[] = {
	CLK_BLK_DCPOST_UID_DCPOST_CMU_DCPOST_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_is_dcpost[] = {
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_AD_APB_DCPOST_M_C2SYNC_1SLV_PCLKM,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_AD_APB_DCPOST_M_CIP2_PCLKM,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_AD_APB_DCPOST_S_C2SYNC_1SLV_PCLKS,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_AD_APB_DCPOST_S_CIP2_PCLKS,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_AXI2APB_DCPOST_ACLK,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_C2SYNC_1SLV_ACLK,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_CIP2_ACLK,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_QE_DCPOST_ACLK,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_QE_DCPOST_PCLK,
	GOUT_BLK_DCPOST_UID_IS_DCPOST_IPCLKPORT_SYSREG_DCPOST_PCLK,
};
enum clk_id cmucal_vclk_lhm_atb_dcfdcpost[] = {
	GOUT_BLK_DCPOST_UID_LHM_ATB_DCFDCPOST_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_dcrddcpost[] = {
	GOUT_BLK_DCPOST_UID_LHM_ATB_DCRDDCPOST_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dcfdcpost[] = {
	GOUT_BLK_DCPOST_UID_LHM_AXI_P_DCFDCPOST_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_dcpostdcf[] = {
	GOUT_BLK_DCPOST_UID_LHS_ATB_DCPOSTDCF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_dcpostdcrd[] = {
	GOUT_BLK_DCPOST_UID_LHS_ATB_DCPOSTDCRD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_dcpostdcf[] = {
	GOUT_BLK_DCPOST_UID_LHS_AXI_D_DCPOSTDCF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ad_apb_decon0[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_decon1[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_decon2[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpp[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpu_dma[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPU_DMA_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpu_dma_pgen[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPU_DMA_PGEN_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpu_wb_mux[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPU_WB_MUX_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_mipi_dsim0[] = {
	GOUT_BLK_DPU_UID_AD_APB_MIPI_DSIM0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_mipi_dsim1[] = {
	GOUT_BLK_DPU_UID_AD_APB_MIPI_DSIM1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_dpud0[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_dpud0_s[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD0_S_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_dpud1[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_dpud1_s[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD1_S_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_dpud2[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_sysmmu_dpud2_s[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD2_S_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_dpup0[] = {
	GOUT_BLK_DPU_UID_AXI2APB_DPUP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_dpup1[] = {
	GOUT_BLK_DPU_UID_AXI2APB_DPUP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_dpud0[] = {
	GOUT_BLK_DPU_UID_BTM_DPUD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_dpud1[] = {
	GOUT_BLK_DPU_UID_BTM_DPUD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_dpud2[] = {
	GOUT_BLK_DPU_UID_BTM_DPUD2_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dpu_cmu_dpu[] = {
	CLK_BLK_DPU_UID_DPU_CMU_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dpu[] = {
	GOUT_BLK_DPU_UID_LHM_AXI_P_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ppmu_dpud0[] = {
	GOUT_BLK_DPU_UID_PPMU_DPUD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_dpud1[] = {
	GOUT_BLK_DPU_UID_PPMU_DPUD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_dpud2[] = {
	GOUT_BLK_DPU_UID_PPMU_DPUD2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_dpu[] = {
	GOUT_BLK_DPU_UID_SYSREG_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_dpu[] = {
	GOUT_BLK_DPU_UID_XIU_P_DPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_wrapper_for_s5i6211_hsi_dcphy_combo_top[] = {
	GOUT_BLK_DPU_UID_wrapper_for_s5i6211_hsi_dcphy_combo_top_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_adm_apb_dspm[] = {
	GOUT_BLK_DSPM_UID_ADM_APB_DSPM_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ad_apb_dspm0[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM0_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dspm1[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM1_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dspm3[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM3_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM3_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dspm4[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM4_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM4_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_axi_dspm0[] = {
	GOUT_BLK_DSPM_UID_AD_AXI_DSPM0_IPCLKPORT_ACLKM,
	GOUT_BLK_DSPM_UID_AD_AXI_DSPM0_IPCLKPORT_ACLKS,
};
enum clk_id cmucal_vclk_axi2apb_dspm[] = {
	GOUT_BLK_DSPM_UID_AXI2APB_DSPM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_dspm0[] = {
	GOUT_BLK_DSPM_UID_BTM_DSPM0_IPCLKPORT_I_ACLK,
	GOUT_BLK_DSPM_UID_BTM_DSPM0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_dspm1[] = {
	GOUT_BLK_DSPM_UID_BTM_DSPM1_IPCLKPORT_I_ACLK,
	GOUT_BLK_DSPM_UID_BTM_DSPM1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dspm_cmu_dspm[] = {
	CLK_BLK_DSPM_UID_DSPM_CMU_DSPM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_d0_dspsdspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_D0_DSPSDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d1_dspsdspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_D1_DSPSDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_P_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_ivadspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_P_IVADSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d0_dspm[] = {
	GOUT_BLK_DSPM_UID_LHS_ACEL_D0_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d1_dspm[] = {
	GOUT_BLK_DSPM_UID_LHS_ACEL_D1_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d2_dspm[] = {
	GOUT_BLK_DSPM_UID_LHS_ACEL_D2_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_dspmdsps[] = {
	GOUT_BLK_DSPM_UID_LHS_AXI_P_DSPMDSPS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_dspmiva[] = {
	GOUT_BLK_DSPM_UID_LHS_AXI_P_DSPMIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pgen_lite_dspm[] = {
	GOUT_BLK_DSPM_UID_PGEN_lite_DSPM_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ppmu_dspm0[] = {
	GOUT_BLK_DSPM_UID_PPMU_DSPM0_IPCLKPORT_ACLK,
	GOUT_BLK_DSPM_UID_PPMU_DSPM0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppmu_dspm1[] = {
	GOUT_BLK_DSPM_UID_PPMU_DSPM1_IPCLKPORT_ACLK,
	GOUT_BLK_DSPM_UID_PPMU_DSPM1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_score_master[] = {
	GOUT_BLK_DSPM_UID_SCORE_MASTER_IPCLKPORT_i_CLK,
};
enum clk_id cmucal_vclk_sysmmu_dspm0[] = {
	GOUT_BLK_DSPM_UID_SYSMMU_DSPM0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_sysmmu_dspm1[] = {
	GOUT_BLK_DSPM_UID_SYSMMU_DSPM1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_sysreg_dspm[] = {
	GOUT_BLK_DSPM_UID_SYSREG_DSPM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wrap2_conv_dspm[] = {
	GOUT_BLK_DSPM_UID_WRAP2_CONV_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_xiu_p_dspm[] = {
	GOUT_BLK_DSPM_UID_XIU_P_DSPM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_dsps[] = {
	GOUT_BLK_DSPS_UID_AXI2APB_DSPS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_dsps_cmu_dsps[] = {
	CLK_BLK_DSPS_UID_DSPS_CMU_DSPS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dspmdsps[] = {
	GOUT_BLK_DSPS_UID_LHM_AXI_P_DSPMDSPS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_sysreg_dsps[] = {
	GOUT_BLK_DSPS_UID_SYSREG_DSPS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dp_link[] = {
	GOUT_BLK_FSYS0_UID_DP_LINK_IPCLKPORT_I_DP_GTC_CLK,
};
enum clk_id cmucal_vclk_ufs_embd[] = {
	GOUT_BLK_FSYS0_UID_UFS_EMBD_IPCLKPORT_I_CLK_UNIPRO,
};
enum clk_id cmucal_vclk_usb30drd[] = {
	GOUT_BLK_FSYS0_UID_USB30DRD_IPCLKPORT_I_USB30DRD_ref_clk,
	CLK_BLK_FSYS0_UID_USB30DRD_IPCLKPORT_I_USBDPPHY_REF_SOC_PLL,
};
enum clk_id cmucal_vclk_adm_ahb_sss[] = {
	GOUT_BLK_FSYS1_UID_ADM_AHB_SSS_IPCLKPORT_HCLKM,
};
enum clk_id cmucal_vclk_ahbbr_fsys1[] = {
	GOUT_BLK_FSYS1_UID_AHBBR_FSYS1_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_axi2ahb_fsys1[] = {
	GOUT_BLK_FSYS1_UID_AXI2AHB_FSYS1_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_axi2apb_fsys1p0[] = {
	GOUT_BLK_FSYS1_UID_AXI2APB_FSYS1P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_fsys1p1[] = {
	GOUT_BLK_FSYS1_UID_AXI2APB_FSYS1P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_fsys1[] = {
	GOUT_BLK_FSYS1_UID_BTM_FSYS1_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_BTM_FSYS1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_fsys1_cmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_FSYS1_CMU_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gpio_fsys1[] = {
	GOUT_BLK_FSYS1_UID_GPIO_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_fsys1[] = {
	GOUT_BLK_FSYS1_UID_LHM_AXI_P_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d_fsys1[] = {
	GOUT_BLK_FSYS1_UID_LHS_ACEL_D_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mmc_card[] = {
	GOUT_BLK_FSYS1_UID_MMC_CARD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_MMC_CARD_IPCLKPORT_SDCLKIN,
};
enum clk_id cmucal_vclk_pcie_gen2[] = {
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_dbi_aclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_ieee1500_wrapper_for_pcieg2_phy_x1_inst_0_i_scl_apb_pclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_mstr_aclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_pcie_sub_ctrl_inst_0_i_driver_apb_clk,
	CLK_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_phy_refclk_in,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_pipe2_digital_x1_wrap_inst_0_i_apb_pclk_scl,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_slv_aclk,
};
enum clk_id cmucal_vclk_pcie_gen3[] = {
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_LT_PCIE3_pcie_sub_ctrl_inst_0_i_driver_apb_clk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_dbi_aclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_ieee1500_wrapper_for_qchannel_wrapper_for_pcieg3_phy_x1_top_inst_0_i_apb_pclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_mstr_aclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_phy_refclk_in,
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_pipe42_pcie_pcs_x1_wrap_inst_0_i_apb_pclk,
	GOUT_BLK_FSYS1_UID_PCIE_GEN3_IPCLKPORT_slv_aclk,
};
enum clk_id cmucal_vclk_pcie_ia_gen2[] = {
	GOUT_BLK_FSYS1_UID_PCIE_IA_GEN2_IPCLKPORT_i_CLK,
};
enum clk_id cmucal_vclk_pcie_ia_gen3[] = {
	GOUT_BLK_FSYS1_UID_PCIE_IA_GEN3_IPCLKPORT_i_CLK,
};
enum clk_id cmucal_vclk_pgen_lite_fsys1[] = {
	GOUT_BLK_FSYS1_UID_PGEN_LITE_FSYS1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ppmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_PPMU_FSYS1_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS1_UID_PPMU_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_rtic[] = {
	GOUT_BLK_FSYS1_UID_RTIC_IPCLKPORT_i_ACLK,
	GOUT_BLK_FSYS1_UID_RTIC_IPCLKPORT_i_PCLK,
};
enum clk_id cmucal_vclk_sss[] = {
	GOUT_BLK_FSYS1_UID_SSS_IPCLKPORT_i_ACLK,
	GOUT_BLK_FSYS1_UID_SSS_IPCLKPORT_i_PCLK,
};
enum clk_id cmucal_vclk_sysmmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_SYSMMU_FSYS1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_sysreg_fsys1[] = {
	GOUT_BLK_FSYS1_UID_SYSREG_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ufs_card[] = {
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_CLK_UNIPRO,
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_FMP_CLK,
};
enum clk_id cmucal_vclk_xiu_d_fsys1[] = {
	GOUT_BLK_FSYS1_UID_XIU_D_FSYS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_pcie_gen2_dbi[] = {
	GOUT_BLK_FSYS1_UID_XIU_PCIE_GEN2_DBI_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_pcie_gen2_slv[] = {
	GOUT_BLK_FSYS1_UID_XIU_PCIE_GEN2_SLV_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_pcie_gen3_dbi[] = {
	GOUT_BLK_FSYS1_UID_XIU_PCIE_GEN3_DBI_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_pcie_gen3_slv[] = {
	GOUT_BLK_FSYS1_UID_XIU_PCIE_GEN3_SLV_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_fsys1[] = {
	GOUT_BLK_FSYS1_UID_XIU_P_FSYS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_g3d[] = {
	GOUT_BLK_G3D_UID_AXI2APB_G3D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_hpmg3d[] = {
	GOUT_BLK_G3D_UID_BUSIF_HPMG3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_g3d_cmu_g3d[] = {
	CLK_BLK_G3D_UID_G3D_CMU_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gpu[] = {
	CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_gray2bin_g3d[] = {
	GOUT_BLK_G3D_UID_GRAY2BIN_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_hpm_g3d0[] = {
	CLK_BLK_G3D_UID_HPM_G3D0_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_g3dsfr[] = {
	GOUT_BLK_G3D_UID_LHM_AXI_G3DSFR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_g3d[] = {
	GOUT_BLK_G3D_UID_LHM_AXI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g3dsfr[] = {
	GOUT_BLK_G3D_UID_LHS_AXI_G3DSFR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pgen_lite_g3d[] = {
	GOUT_BLK_G3D_UID_PGEN_LITE_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_sysreg_g3d[] = {
	GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_g3d[] = {
	GOUT_BLK_G3D_UID_XIU_P_G3D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_adm_dap_iva[] = {
	GOUT_BLK_IVA_UID_ADM_DAP_IVA_IPCLKPORT_dapclkm,
};
enum clk_id cmucal_vclk_ad_apb_iva0[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_iva1[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_iva2[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_2m_iva[] = {
	GOUT_BLK_IVA_UID_AXI2APB_2M_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_iva[] = {
	GOUT_BLK_IVA_UID_AXI2APB_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_iva[] = {
	GOUT_BLK_IVA_UID_BTM_IVA_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_iva[] = {
	GOUT_BLK_IVA_UID_IVA_IPCLKPORT_dap_clk,
};
enum clk_id cmucal_vclk_iva_cmu_iva[] = {
	CLK_BLK_IVA_UID_IVA_CMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_iva[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_P_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pgen_lite_iva[] = {
	GOUT_BLK_IVA_UID_PGEN_lite_IVA_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ppmu_iva[] = {
	GOUT_BLK_IVA_UID_PPMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_iva[] = {
	GOUT_BLK_IVA_UID_SYSREG_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_rb_iva[] = {
	GOUT_BLK_IVA_UID_TREX_RB_IVA_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_xiu_p_iva[] = {
	GOUT_BLK_IVA_UID_XIU_P_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_hpm_mif[] = {
	CLK_BLK_MIF_UID_HPM_MIF_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_axi2apb_peric0p0[] = {
	GOUT_BLK_PERIC0_UID_AXI2APB_PERIC0P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_peric0p1[] = {
	GOUT_BLK_PERIC0_UID_AXI2APB_PERIC0P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_gpio_peric0[] = {
	GOUT_BLK_PERIC0_UID_GPIO_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_peric0[] = {
	GOUT_BLK_PERIC0_UID_LHM_AXI_P_PERIC0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_peric0_cmu_peric0[] = {
	CLK_BLK_PERIC0_UID_PERIC0_CMU_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pwm[] = {
	GOUT_BLK_PERIC0_UID_PWM_IPCLKPORT_i_PCLK_S0,
};
enum clk_id cmucal_vclk_sysreg_peric0[] = {
	GOUT_BLK_PERIC0_UID_SYSREG_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_uart_dbg[] = {
	GOUT_BLK_PERIC0_UID_UART_DBG_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_UART_DBG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi00_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI00_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI00_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi00_usi[] = {
	GOUT_BLK_PERIC0_UID_USI00_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI00_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi01_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI01_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI01_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi01_usi[] = {
	GOUT_BLK_PERIC0_UID_USI01_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI01_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi02_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI02_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI02_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi02_usi[] = {
	GOUT_BLK_PERIC0_UID_USI02_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI02_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi03_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI03_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI03_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi03_usi[] = {
	GOUT_BLK_PERIC0_UID_USI03_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI03_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi04_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI04_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI04_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi04_usi[] = {
	GOUT_BLK_PERIC0_UID_USI04_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI04_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi05_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI05_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI05_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi05_usi[] = {
	GOUT_BLK_PERIC0_UID_USI05_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI05_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi12_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI12_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI12_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi12_usi[] = {
	GOUT_BLK_PERIC0_UID_USI12_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI12_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi13_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI13_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI13_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi13_usi[] = {
	GOUT_BLK_PERIC0_UID_USI13_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI13_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi14_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI14_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI14_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi14_usi[] = {
	GOUT_BLK_PERIC0_UID_USI14_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI14_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_peric0[] = {
	GOUT_BLK_PERIC0_UID_XIU_P_PERIC0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_peric1p0[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_peric1p1[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_gpio_peric1[] = {
	GOUT_BLK_PERIC1_UID_GPIO_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cam0[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM0_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cam1[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM1_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cam2[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM2_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_i2c_cam3[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM3_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_peric1[] = {
	GOUT_BLK_PERIC1_UID_LHM_AXI_P_PERIC1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_peric1_cmu_peric1[] = {
	CLK_BLK_PERIC1_UID_PERIC1_CMU_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_spi_cam0[] = {
	GOUT_BLK_PERIC1_UID_SPI_CAM0_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_SPI_CAM0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_peric1[] = {
	GOUT_BLK_PERIC1_UID_SYSREG_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_uart_bt[] = {
	GOUT_BLK_PERIC1_UID_UART_BT_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_UART_BT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi06_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI06_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI06_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi06_usi[] = {
	GOUT_BLK_PERIC1_UID_USI06_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI06_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi07_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI07_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI07_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi07_usi[] = {
	GOUT_BLK_PERIC1_UID_USI07_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI07_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi08_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI08_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI08_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi08_usi[] = {
	GOUT_BLK_PERIC1_UID_USI08_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI08_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi09_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI09_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI09_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi09_usi[] = {
	GOUT_BLK_PERIC1_UID_USI09_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI09_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi10_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI10_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI10_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi10_usi[] = {
	GOUT_BLK_PERIC1_UID_USI10_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI10_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi11_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI11_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI11_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi11_usi[] = {
	GOUT_BLK_PERIC1_UID_USI11_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI11_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_peric1[] = {
	GOUT_BLK_PERIC1_UID_XIU_P_PERIC1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ad_axi_p_peris[] = {
	GOUT_BLK_PERIS_UID_AD_AXI_P_PERIS_IPCLKPORT_ACLKM,
	GOUT_BLK_PERIS_UID_AD_AXI_P_PERIS_IPCLKPORT_ACLKS,
};
enum clk_id cmucal_vclk_axi2apb_perisp[] = {
	GOUT_BLK_PERIS_UID_AXI2APB_PERISP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_tmu[] = {
	GOUT_BLK_PERIS_UID_BUSIF_TMU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gic[] = {
	GOUT_BLK_PERIS_UID_GIC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_peris[] = {
	GOUT_BLK_PERIS_UID_LHM_AXI_P_PERIS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mct[] = {
	GOUT_BLK_PERIS_UID_MCT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_otp_con_bira[] = {
	GOUT_BLK_PERIS_UID_OTP_CON_BIRA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_otp_con_top[] = {
	GOUT_BLK_PERIS_UID_OTP_CON_TOP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_peris_cmu_peris[] = {
	CLK_BLK_PERIS_UID_PERIS_CMU_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_peris[] = {
	GOUT_BLK_PERIS_UID_SYSREG_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wdt_cluster0[] = {
	GOUT_BLK_PERIS_UID_WDT_CLUSTER0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wdt_cluster1[] = {
	GOUT_BLK_PERIS_UID_WDT_CLUSTER1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_peris[] = {
	GOUT_BLK_PERIS_UID_XIU_P_PERIS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_s2d_cmu_s2d[] = {
	CLK_BLK_S2D_UID_S2D_CMU_S2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dmic_if[] = {
	GOUT_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_IF_CLK,
	CLK_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_IF_DIV2_CLK,
};
enum clk_id cmucal_vclk_u_dmic_clk_mux[] = {
	CLK_BLK_VTS_UID_u_DMIC_CLK_MUX_IPCLKPORT_D0,
};

/* Switching LUT */
/* -1 is the Value of EMPTY_CAL_ID */
struct switch_lut spl_clk_aud_cpu_pclkdbg_blk_cmu_lut[] = {
	{2132000, 0, 0},
	{1066000, 0, 0},
	{672000, 3, 0},
	{400000, 2, 1},
};
struct switch_lut spl_clk_cpucl0_cmuref_blk_cmu_lut[] = {
	{2132000, 0, 0},
	{1066000, 0, 0},
	{800000, 2, 0},
	{400000, 2, 1},
};
struct switch_lut div_clk_cpucl1_pclkdbg_blk_cmu_lut[] = {
	{2132000, 0, 0},
	{1066000, 0, 0},
	{932750, 1, 0},
	{266500, 0, 3},
};

/* DVFS LUT */
struct vclk_lut vddi_lut[] = {
	{400000, vddi_nm_lut_params},
	{300000, vddi_od_lut_params},
	{200000, vddi_sud_lut_params},
	{100000, vddi_ud_lut_params},
};
struct vclk_lut vdd_mif_lut[] = {
	{4264000, vdd_mif_od_lut_params},
	{3731000, vdd_mif_nm_lut_params},
	{2576599, vdd_mif_ud_lut_params},
	{1418000, vdd_mif_sud_lut_params},
};
struct vclk_lut vdd_g3d_lut[] = {
	{860000, vdd_g3d_nm_lut_params},
	{650000, vdd_g3d_ud_lut_params},
	{320000, vdd_g3d_sud_lut_params},
};
struct vclk_lut vdd_cam_lut[] = {
	{300000, vdd_cam_nm_lut_params},
	{200000, vdd_cam_sud_lut_params},
	{100000, vdd_cam_ud_lut_params},
};

/* SPECIAL LUT */
struct vclk_lut spl_clk_usi_cmgp02_blk_apm_lut[] = {
	{100000, spl_clk_usi_cmgp02_blk_apm_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp00_blk_apm_lut[] = {
	{100000, spl_clk_usi_cmgp00_blk_apm_nm_lut_params},
};
struct vclk_lut clk_cmgp_adc_blk_apm_lut[] = {
	{100000, clk_cmgp_adc_blk_apm_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp03_blk_apm_lut[] = {
	{100000, spl_clk_usi_cmgp03_blk_apm_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp01_blk_apm_lut[] = {
	{100000, spl_clk_usi_cmgp01_blk_apm_nm_lut_params},
};
struct vclk_lut spl_clk_aud_uaif0_blk_aud_lut[] = {
	{26000, spl_clk_aud_uaif0_blk_aud_nm_lut_params},
};
struct vclk_lut spl_clk_aud_uaif2_blk_aud_lut[] = {
	{26000, spl_clk_aud_uaif2_blk_aud_nm_lut_params},
};
struct vclk_lut spl_clk_aud_cpu_pclkdbg_blk_aud_lut[] = {
	{149999, spl_clk_aud_cpu_pclkdbg_blk_aud_nm_lut_params},
	{133250, spl_clk_aud_cpu_pclkdbg_blk_aud_sud_lut_params},
};
struct vclk_lut spl_clk_aud_uaif1_blk_aud_lut[] = {
	{26000, spl_clk_aud_uaif1_blk_aud_nm_lut_params},
};
struct vclk_lut spl_clk_aud_uaif3_blk_aud_lut[] = {
	{26000, spl_clk_aud_uaif3_blk_aud_nm_lut_params},
};
struct vclk_lut div_clk_aud_dmic_blk_aud_lut[] = {
	{74999, div_clk_aud_dmic_blk_aud_sud_lut_params},
	{46153, div_clk_aud_dmic_blk_aud_ud_lut_params},
	{24999, div_clk_aud_dmic_blk_aud_nm_lut_params},
};
struct vclk_lut spl_clk_chub_usi01_blk_chub_lut[] = {
	{177666, spl_clk_chub_usi01_blk_chub_nm_lut_params},
};
struct vclk_lut spl_clk_chub_usi00_blk_chub_lut[] = {
	{177666, spl_clk_chub_usi00_blk_chub_nm_lut_params},
};
struct vclk_lut mux_clk_chub_timer_fclk_blk_chub_lut[] = {
	{26000, mux_clk_chub_timer_fclk_blk_chub_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp02_blk_cmgp_lut[] = {
	{200000, spl_clk_usi_cmgp02_blk_cmgp_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp00_blk_cmgp_lut[] = {
	{200000, spl_clk_usi_cmgp00_blk_cmgp_nm_lut_params},
};
struct vclk_lut clk_cmgp_adc_blk_cmgp_lut[] = {
	{100000, clk_cmgp_adc_blk_cmgp_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp01_blk_cmgp_lut[] = {
	{200000, spl_clk_usi_cmgp01_blk_cmgp_nm_lut_params},
};
struct vclk_lut spl_clk_usi_cmgp03_blk_cmgp_lut[] = {
	{200000, spl_clk_usi_cmgp03_blk_cmgp_nm_lut_params},
};
struct vclk_lut clk_fsys0_ufs_embd_blk_cmu_lut[] = {
	{710666, clk_fsys0_ufs_embd_blk_cmu_od_lut_params},
	{177666, clk_fsys0_ufs_embd_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_fsys1_mmc_card_blk_cmu_lut[] = {
	{800000, clk_fsys1_mmc_card_blk_cmu_nm_lut_params},
};
struct vclk_lut clkcmu_hpm_blk_cmu_lut[] = {
	{1865500, clkcmu_hpm_blk_cmu_od_lut_params},
	{800000, clkcmu_hpm_blk_cmu_nm_lut_params},
	{466375, clkcmu_hpm_blk_cmu_ud_lut_params},
};
struct vclk_lut spl_clk_chub_usi01_blk_cmu_lut[] = {
	{2132000, spl_clk_chub_usi01_blk_cmu_od_lut_params},
	{710666, spl_clk_chub_usi01_blk_cmu_nm_lut_params},
};
struct vclk_lut clkcmu_cis_clk0_blk_cmu_lut[] = {
	{200000, clkcmu_cis_clk0_blk_cmu_od_lut_params},
	{100000, clkcmu_cis_clk0_blk_cmu_nm_lut_params},
};
struct vclk_lut clkcmu_cis_clk2_blk_cmu_lut[] = {
	{200000, clkcmu_cis_clk2_blk_cmu_od_lut_params},
	{100000, clkcmu_cis_clk2_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_fsys0_dpgtc_blk_cmu_lut[] = {
	{533000, clk_fsys0_dpgtc_blk_cmu_od_lut_params},
	{133250, clk_fsys0_dpgtc_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_fsys0_usb30drd_blk_cmu_lut[] = {
	{266500, clk_fsys0_usb30drd_blk_cmu_od_lut_params},
	{66625, clk_fsys0_usb30drd_blk_cmu_nm_lut_params},
};
struct vclk_lut mux_clkcmu_fsys0_usbdp_debug_user_blk_cmu_lut[] = {
	{26000, mux_clkcmu_fsys0_usbdp_debug_user_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_fsys1_ufs_card_blk_cmu_lut[] = {
	{710666, clk_fsys1_ufs_card_blk_cmu_od_lut_params},
	{177666, clk_fsys1_ufs_card_blk_cmu_nm_lut_params},
};
struct vclk_lut occ_mif_cmuref_blk_cmu_lut[] = {
	{2132000, occ_mif_cmuref_blk_cmu_od_lut_params},
	{533000, occ_mif_cmuref_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi01_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi01_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi01_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi04_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi04_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi04_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi13_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi13_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi13_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam1_blk_cmu_lut[] = {
	{2132000, clk_peric1_i2c_cam1_blk_cmu_od_lut_params},
	{400000, clk_peric1_i2c_cam1_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_spi_cam0_blk_cmu_lut[] = {
	{2132000, clk_peric1_spi_cam0_blk_cmu_od_lut_params},
	{400000, clk_peric1_spi_cam0_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_usi07_usi_blk_cmu_lut[] = {
	{2132000, clk_peric1_usi07_usi_blk_cmu_od_lut_params},
	{400000, clk_peric1_usi07_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_usi10_usi_blk_cmu_lut[] = {
	{2132000, clk_peric1_usi10_usi_blk_cmu_od_lut_params},
	{400000, clk_peric1_usi10_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut occ_cmu_cmuref_blk_cmu_lut[] = {
	{800000, occ_cmu_cmuref_blk_cmu_od_lut_params},
	{400000, occ_cmu_cmuref_blk_cmu_nm_lut_params},
	{266500, occ_cmu_cmuref_blk_cmu_sud_lut_params},
};
struct vclk_lut spl_clk_chub_usi00_blk_cmu_lut[] = {
	{2132000, spl_clk_chub_usi00_blk_cmu_od_lut_params},
	{710666, spl_clk_chub_usi00_blk_cmu_nm_lut_params},
};
struct vclk_lut clkcmu_cis_clk1_blk_cmu_lut[] = {
	{200000, clkcmu_cis_clk1_blk_cmu_od_lut_params},
	{100000, clkcmu_cis_clk1_blk_cmu_nm_lut_params},
};
struct vclk_lut clkcmu_cis_clk3_blk_cmu_lut[] = {
	{200000, clkcmu_cis_clk3_blk_cmu_od_lut_params},
	{100000, clkcmu_cis_clk3_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi00_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi00_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi00_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi05_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi05_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi05_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam0_blk_cmu_lut[] = {
	{2132000, clk_peric1_i2c_cam0_blk_cmu_od_lut_params},
	{400000, clk_peric1_i2c_cam0_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_uart_bt_blk_cmu_lut[] = {
	{2132000, clk_peric1_uart_bt_blk_cmu_od_lut_params},
	{400000, clk_peric1_uart_bt_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_usi09_usi_blk_cmu_lut[] = {
	{2132000, clk_peric1_usi09_usi_blk_cmu_od_lut_params},
	{400000, clk_peric1_usi09_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_uart_dbg_blk_cmu_lut[] = {
	{2132000, clk_peric0_uart_dbg_blk_cmu_od_lut_params},
	{400000, clk_peric0_uart_dbg_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi12_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi12_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi12_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam3_blk_cmu_lut[] = {
	{2132000, clk_peric1_i2c_cam3_blk_cmu_od_lut_params},
	{400000, clk_peric1_i2c_cam3_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_usi11_usi_blk_cmu_lut[] = {
	{2132000, clk_peric1_usi11_usi_blk_cmu_od_lut_params},
	{400000, clk_peric1_usi11_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi02_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi02_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi02_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam2_blk_cmu_lut[] = {
	{2132000, clk_peric1_i2c_cam2_blk_cmu_od_lut_params},
	{400000, clk_peric1_i2c_cam2_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi03_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi03_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi03_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_usi08_usi_blk_cmu_lut[] = {
	{2132000, clk_peric1_usi08_usi_blk_cmu_od_lut_params},
	{400000, clk_peric1_usi08_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric1_usi06_usi_blk_cmu_lut[] = {
	{2132000, clk_peric1_usi06_usi_blk_cmu_od_lut_params},
	{400000, clk_peric1_usi06_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut clk_peric0_usi14_usi_blk_cmu_lut[] = {
	{2132000, clk_peric0_usi14_usi_blk_cmu_od_lut_params},
	{400000, clk_peric0_usi14_usi_blk_cmu_nm_lut_params},
};
struct vclk_lut mux_clk_cluster0_sclk_blk_cpucl0_lut[] = {
	{1850333, mux_clk_cluster0_sclk_blk_cpucl0_sod_lut_params},
	{1499333, mux_clk_cluster0_sclk_blk_cpucl0_od_lut_params},
	{1150500, mux_clk_cluster0_sclk_blk_cpucl0_nm_lut_params},
	{650000, mux_clk_cluster0_sclk_blk_cpucl0_ud_lut_params},
	{349916, mux_clk_cluster0_sclk_blk_cpucl0_sud_lut_params},
};
struct vclk_lut spl_clk_cpucl0_cmuref_blk_cpucl0_lut[] = {
	{925166, spl_clk_cpucl0_cmuref_blk_cpucl0_sod_lut_params},
	{749666, spl_clk_cpucl0_cmuref_blk_cpucl0_od_lut_params},
	{575250, spl_clk_cpucl0_cmuref_blk_cpucl0_nm_lut_params},
	{325000, spl_clk_cpucl0_cmuref_blk_cpucl0_ud_lut_params},
	{174958, spl_clk_cpucl0_cmuref_blk_cpucl0_sud_lut_params},
};
struct vclk_lut clk_cluster0_aclkp_blk_cpucl0_lut[] = {
	{1150500, clk_cluster0_aclkp_blk_cpucl0_nm_lut_params},
	{925166, clk_cluster0_aclkp_blk_cpucl0_sod_lut_params},
	{749666, clk_cluster0_aclkp_blk_cpucl0_od_lut_params},
	{650000, clk_cluster0_aclkp_blk_cpucl0_ud_lut_params},
	{349916, clk_cluster0_aclkp_blk_cpucl0_sud_lut_params},
};
struct vclk_lut div_clk_cluster0_periphclk_blk_cpucl0_lut[] = {
	{925166, div_clk_cluster0_periphclk_blk_cpucl0_sod_lut_params},
	{749666, div_clk_cluster0_periphclk_blk_cpucl0_od_lut_params},
	{575250, div_clk_cluster0_periphclk_blk_cpucl0_nm_lut_params},
	{325000, div_clk_cluster0_periphclk_blk_cpucl0_ud_lut_params},
	{174958, div_clk_cluster0_periphclk_blk_cpucl0_sud_lut_params},
};
struct vclk_lut clk_cluster0_aclk_blk_cpucl0_lut[] = {
	{1150500, clk_cluster0_aclk_blk_cpucl0_nm_lut_params},
	{925166, clk_cluster0_aclk_blk_cpucl0_sod_lut_params},
	{749666, clk_cluster0_aclk_blk_cpucl0_od_lut_params},
	{650000, clk_cluster0_aclk_blk_cpucl0_ud_lut_params},
	{349916, clk_cluster0_aclk_blk_cpucl0_sud_lut_params},
};
struct vclk_lut div_clk_cluster1_aclk_blk_cpucl1_lut[] = {
	{1163500, div_clk_cluster1_aclk_blk_cpucl1_sod_lut_params},
	{946833, div_clk_cluster1_aclk_blk_cpucl1_od_lut_params},
	{739375, div_clk_cluster1_aclk_blk_cpucl1_nm_lut_params},
	{464100, div_clk_cluster1_aclk_blk_cpucl1_ud_lut_params},
	{200000, div_clk_cluster1_aclk_blk_cpucl1_sud_lut_params},
};
struct vclk_lut div_clk_cpucl1_pclkdbg_blk_cpucl1_lut[] = {
	{290875, div_clk_cpucl1_pclkdbg_blk_cpucl1_sod_lut_params},
	{236708, div_clk_cpucl1_pclkdbg_blk_cpucl1_od_lut_params},
	{184843, div_clk_cpucl1_pclkdbg_blk_cpucl1_nm_lut_params},
	{116025, div_clk_cpucl1_pclkdbg_blk_cpucl1_ud_lut_params},
	{50000, div_clk_cpucl1_pclkdbg_blk_cpucl1_sud_lut_params},
};
struct vclk_lut div_clk_cluster1_atclk_blk_cpucl1_lut[] = {
	{581750, div_clk_cluster1_atclk_blk_cpucl1_sod_lut_params},
	{473416, div_clk_cluster1_atclk_blk_cpucl1_od_lut_params},
	{369687, div_clk_cluster1_atclk_blk_cpucl1_nm_lut_params},
	{232050, div_clk_cluster1_atclk_blk_cpucl1_ud_lut_params},
	{100000, div_clk_cluster1_atclk_blk_cpucl1_sud_lut_params},
};
struct vclk_lut spl_clk_cpucl1_cmuref_blk_cpucl1_lut[] = {
	{1163500, spl_clk_cpucl1_cmuref_blk_cpucl1_sod_lut_params},
	{946833, spl_clk_cpucl1_cmuref_blk_cpucl1_od_lut_params},
	{739375, spl_clk_cpucl1_cmuref_blk_cpucl1_nm_lut_params},
	{464100, spl_clk_cpucl1_cmuref_blk_cpucl1_ud_lut_params},
	{200000, spl_clk_cpucl1_cmuref_blk_cpucl1_sud_lut_params},
};
struct vclk_lut occ_mif_cmuref_blk_mif_lut[] = {
	{266500, occ_mif_cmuref_blk_mif_nm_lut_params},
	{26000, occ_mif_cmuref_blk_mif_od_lut_params},
};
struct vclk_lut clk_peric0_uart_dbg_blk_peric0_lut[] = {
	{200000, clk_peric0_uart_dbg_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi01_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi01_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi03_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi03_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi05_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi05_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi13_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi13_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi04_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi04_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi02_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi02_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi12_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi12_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi00_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi00_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric0_usi14_usi_blk_peric0_lut[] = {
	{400000, clk_peric0_usi14_usi_blk_peric0_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam0_blk_peric1_lut[] = {
	{200000, clk_peric1_i2c_cam0_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam2_blk_peric1_lut[] = {
	{200000, clk_peric1_i2c_cam2_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_spi_cam0_blk_peric1_lut[] = {
	{400000, clk_peric1_spi_cam0_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_usi06_usi_blk_peric1_lut[] = {
	{400000, clk_peric1_usi06_usi_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_usi08_usi_blk_peric1_lut[] = {
	{400000, clk_peric1_usi08_usi_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_usi10_usi_blk_peric1_lut[] = {
	{400000, clk_peric1_usi10_usi_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam1_blk_peric1_lut[] = {
	{200000, clk_peric1_i2c_cam1_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_usi07_usi_blk_peric1_lut[] = {
	{400000, clk_peric1_usi07_usi_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_uart_bt_blk_peric1_lut[] = {
	{200000, clk_peric1_uart_bt_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_usi09_usi_blk_peric1_lut[] = {
	{400000, clk_peric1_usi09_usi_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_i2c_cam3_blk_peric1_lut[] = {
	{200000, clk_peric1_i2c_cam3_blk_peric1_nm_lut_params},
};
struct vclk_lut clk_peric1_usi11_usi_blk_peric1_lut[] = {
	{400000, clk_peric1_usi11_usi_blk_peric1_nm_lut_params},
};
struct vclk_lut spl_clk_vts_dmic_if_div2_blk_vts_lut[] = {
	{50000, spl_clk_vts_dmic_if_div2_blk_vts_nm_lut_params},
};
struct vclk_lut div_clk_vts_dmic_blk_vts_lut[] = {
	{100000, div_clk_vts_dmic_blk_vts_nm_lut_params},
};

/* COMMON LUT */
struct vclk_lut blk_apm_lut[] = {
	{355333, blk_apm_lut_params},
};
struct vclk_lut blk_aud_lut[] = {
	{1199999, blk_aud_lut_params},
};
struct vclk_lut blk_busc_lut[] = {
	{266500, blk_busc_lut_params},
};
struct vclk_lut blk_chub_lut[] = {
	{177666, blk_chub_lut_params},
};
struct vclk_lut blk_cmgp_lut[] = {
	{200000, blk_cmgp_lut_params},
};
struct vclk_lut blk_cmu_lut[] = {
	{2132000, blk_cmu_lut_params},
};
struct vclk_lut blk_core_lut[] = {
	{355333, blk_core_lut_params},
};
struct vclk_lut blk_cpucl0_lut[] = {
	{1150500, blk_cpucl0_lut_params},
};
struct vclk_lut blk_cpucl1_lut[] = {
	{1478750, blk_cpucl1_lut_params},
};
struct vclk_lut blk_dcf_lut[] = {
	{266500, blk_dcf_lut_params},
};
struct vclk_lut blk_dcpost_lut[] = {
	{266500, blk_dcpost_lut_params},
};
struct vclk_lut blk_dcrd_lut[] = {
	{336000, blk_dcrd_lut_params},
};
struct vclk_lut blk_dpu_lut[] = {
	{160000, blk_dpu_lut_params},
};
struct vclk_lut blk_dspm_lut[] = {
	{266500, blk_dspm_lut_params},
};
struct vclk_lut blk_dsps_lut[] = {
	{266500, blk_dsps_lut_params},
};
struct vclk_lut blk_g2d_lut[] = {
	{266500, blk_g2d_lut_params},
};
struct vclk_lut blk_g3d_lut[] = {
	{860000, blk_g3d_lut_params},
};
struct vclk_lut blk_isphq_lut[] = {
	{233187, blk_isphq_lut_params},
};
struct vclk_lut blk_isplp_lut[] = {
	{233187, blk_isplp_lut_params},
};
struct vclk_lut blk_isppre_lut[] = {
	{233187, blk_isppre_lut_params},
};
struct vclk_lut blk_iva_lut[] = {
	{266500, blk_iva_lut_params},
};
struct vclk_lut blk_mfc_lut[] = {
	{168000, blk_mfc_lut_params},
};
struct vclk_lut blk_mif_lut[] = {
	{3731000, blk_mif_lut_params},
};
struct vclk_lut blk_peric0_lut[] = {
	{200000, blk_peric0_lut_params},
};
struct vclk_lut blk_peric1_lut[] = {
	{200000, blk_peric1_lut_params},
};
struct vclk_lut blk_s2d_lut[] = {
	{1664000, blk_s2d_lut_params},
};
struct vclk_lut blk_vts_lut[] = {
	{100000, blk_vts_lut_params},
};
/*=================VCLK Switch list================================*/

struct vclk_switch vclk_switch_blk_aud[] = {
	{MUX_CLK_AUD_CPU, MUX_CLKCMU_AUD_CPU, CLKCMU_AUD_CPU, GATE_CLKCMU_AUD_CPU, MUX_CLKCMU_AUD_CPU_USER, spl_clk_aud_cpu_pclkdbg_blk_cmu_lut, 4},
};
struct vclk_switch vclk_switch_blk_cpucl0[] = {
	{MUX_CLK_CPUCL0_PLL, MUX_CLKCMU_CPUCL0_SWITCH, CLKCMU_CPUCL0_SWITCH, GATE_CLKCMU_CPUCL0_SWITCH, MUX_CLKCMU_CPUCL0_SWITCH_USER, spl_clk_cpucl0_cmuref_blk_cmu_lut, 4},
};
struct vclk_switch vclk_switch_blk_cpucl1[] = {
	{MUX_CLK_CPUCL1_PLL, MUX_CLKCMU_CPUCL1_SWITCH, CLKCMU_CPUCL1_SWITCH, GATE_CLKCMU_CPUCL1_SWITCH, MUX_CLKCMU_CPUCL1_SWITCH_USER, div_clk_cpucl1_pclkdbg_blk_cmu_lut, 4},
};

/*=================VCLK list================================*/

struct vclk cmucal_vclk_list[] = {

/* DVFS VCLK */
	CMUCAL_VCLK(VCLK_VDDI, vddi_lut, cmucal_vclk_vddi, NULL, NULL),
	CMUCAL_VCLK(VCLK_VDD_MIF, vdd_mif_lut, cmucal_vclk_vdd_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_VDD_G3D, vdd_g3d_lut, cmucal_vclk_vdd_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_VDD_CAM, vdd_cam_lut, cmucal_vclk_vdd_cam, NULL, NULL),

/* SPECIAL VCLK */
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP02_BLK_APM, spl_clk_usi_cmgp02_blk_apm_lut, cmucal_vclk_spl_clk_usi_cmgp02_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP00_BLK_APM, spl_clk_usi_cmgp00_blk_apm_lut, cmucal_vclk_spl_clk_usi_cmgp00_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_CMGP_ADC_BLK_APM, clk_cmgp_adc_blk_apm_lut, cmucal_vclk_clk_cmgp_adc_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP03_BLK_APM, spl_clk_usi_cmgp03_blk_apm_lut, cmucal_vclk_spl_clk_usi_cmgp03_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP01_BLK_APM, spl_clk_usi_cmgp01_blk_apm_lut, cmucal_vclk_spl_clk_usi_cmgp01_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_AUD_UAIF0_BLK_AUD, spl_clk_aud_uaif0_blk_aud_lut, cmucal_vclk_spl_clk_aud_uaif0_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_AUD_UAIF2_BLK_AUD, spl_clk_aud_uaif2_blk_aud_lut, cmucal_vclk_spl_clk_aud_uaif2_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_AUD_CPU_PCLKDBG_BLK_AUD, spl_clk_aud_cpu_pclkdbg_blk_aud_lut, cmucal_vclk_spl_clk_aud_cpu_pclkdbg_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_AUD_UAIF1_BLK_AUD, spl_clk_aud_uaif1_blk_aud_lut, cmucal_vclk_spl_clk_aud_uaif1_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_AUD_UAIF3_BLK_AUD, spl_clk_aud_uaif3_blk_aud_lut, cmucal_vclk_spl_clk_aud_uaif3_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_AUD_DMIC_BLK_AUD, div_clk_aud_dmic_blk_aud_lut, cmucal_vclk_div_clk_aud_dmic_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CHUB_USI01_BLK_CHUB, spl_clk_chub_usi01_blk_chub_lut, cmucal_vclk_spl_clk_chub_usi01_blk_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CHUB_USI00_BLK_CHUB, spl_clk_chub_usi00_blk_chub_lut, cmucal_vclk_spl_clk_chub_usi00_blk_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLK_CHUB_TIMER_FCLK_BLK_CHUB, mux_clk_chub_timer_fclk_blk_chub_lut, cmucal_vclk_mux_clk_chub_timer_fclk_blk_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP02_BLK_CMGP, spl_clk_usi_cmgp02_blk_cmgp_lut, cmucal_vclk_spl_clk_usi_cmgp02_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP00_BLK_CMGP, spl_clk_usi_cmgp00_blk_cmgp_lut, cmucal_vclk_spl_clk_usi_cmgp00_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_CMGP_ADC_BLK_CMGP, clk_cmgp_adc_blk_cmgp_lut, cmucal_vclk_clk_cmgp_adc_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP01_BLK_CMGP, spl_clk_usi_cmgp01_blk_cmgp_lut, cmucal_vclk_spl_clk_usi_cmgp01_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_USI_CMGP03_BLK_CMGP, spl_clk_usi_cmgp03_blk_cmgp_lut, cmucal_vclk_spl_clk_usi_cmgp03_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_FSYS0_UFS_EMBD_BLK_CMU, clk_fsys0_ufs_embd_blk_cmu_lut, cmucal_vclk_clk_fsys0_ufs_embd_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_FSYS1_MMC_CARD_BLK_CMU, clk_fsys1_mmc_card_blk_cmu_lut, cmucal_vclk_clk_fsys1_mmc_card_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_HPM_BLK_CMU, clkcmu_hpm_blk_cmu_lut, cmucal_vclk_clkcmu_hpm_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CHUB_USI01_BLK_CMU, spl_clk_chub_usi01_blk_cmu_lut, cmucal_vclk_spl_clk_chub_usi01_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK0_BLK_CMU, clkcmu_cis_clk0_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk0_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK2_BLK_CMU, clkcmu_cis_clk2_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk2_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_FSYS0_DPGTC_BLK_CMU, clk_fsys0_dpgtc_blk_cmu_lut, cmucal_vclk_clk_fsys0_dpgtc_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_FSYS0_USB30DRD_BLK_CMU, clk_fsys0_usb30drd_blk_cmu_lut, cmucal_vclk_clk_fsys0_usb30drd_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLKCMU_FSYS0_USBDP_DEBUG_USER_BLK_CMU, mux_clkcmu_fsys0_usbdp_debug_user_blk_cmu_lut, cmucal_vclk_mux_clkcmu_fsys0_usbdp_debug_user_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_FSYS1_UFS_CARD_BLK_CMU, clk_fsys1_ufs_card_blk_cmu_lut, cmucal_vclk_clk_fsys1_ufs_card_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_MIF_CMUREF_BLK_CMU, occ_mif_cmuref_blk_cmu_lut, cmucal_vclk_occ_mif_cmuref_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI01_USI_BLK_CMU, clk_peric0_usi01_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi01_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI04_USI_BLK_CMU, clk_peric0_usi04_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi04_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI13_USI_BLK_CMU, clk_peric0_usi13_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi13_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM1_BLK_CMU, clk_peric1_i2c_cam1_blk_cmu_lut, cmucal_vclk_clk_peric1_i2c_cam1_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_SPI_CAM0_BLK_CMU, clk_peric1_spi_cam0_blk_cmu_lut, cmucal_vclk_clk_peric1_spi_cam0_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI07_USI_BLK_CMU, clk_peric1_usi07_usi_blk_cmu_lut, cmucal_vclk_clk_peric1_usi07_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI10_USI_BLK_CMU, clk_peric1_usi10_usi_blk_cmu_lut, cmucal_vclk_clk_peric1_usi10_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_CMU_CMUREF_BLK_CMU, occ_cmu_cmuref_blk_cmu_lut, cmucal_vclk_occ_cmu_cmuref_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CHUB_USI00_BLK_CMU, spl_clk_chub_usi00_blk_cmu_lut, cmucal_vclk_spl_clk_chub_usi00_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK1_BLK_CMU, clkcmu_cis_clk1_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk1_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK3_BLK_CMU, clkcmu_cis_clk3_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk3_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI00_USI_BLK_CMU, clk_peric0_usi00_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi00_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI05_USI_BLK_CMU, clk_peric0_usi05_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi05_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM0_BLK_CMU, clk_peric1_i2c_cam0_blk_cmu_lut, cmucal_vclk_clk_peric1_i2c_cam0_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_UART_BT_BLK_CMU, clk_peric1_uart_bt_blk_cmu_lut, cmucal_vclk_clk_peric1_uart_bt_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI09_USI_BLK_CMU, clk_peric1_usi09_usi_blk_cmu_lut, cmucal_vclk_clk_peric1_usi09_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_UART_DBG_BLK_CMU, clk_peric0_uart_dbg_blk_cmu_lut, cmucal_vclk_clk_peric0_uart_dbg_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI12_USI_BLK_CMU, clk_peric0_usi12_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi12_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM3_BLK_CMU, clk_peric1_i2c_cam3_blk_cmu_lut, cmucal_vclk_clk_peric1_i2c_cam3_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI11_USI_BLK_CMU, clk_peric1_usi11_usi_blk_cmu_lut, cmucal_vclk_clk_peric1_usi11_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI02_USI_BLK_CMU, clk_peric0_usi02_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi02_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM2_BLK_CMU, clk_peric1_i2c_cam2_blk_cmu_lut, cmucal_vclk_clk_peric1_i2c_cam2_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI03_USI_BLK_CMU, clk_peric0_usi03_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi03_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI08_USI_BLK_CMU, clk_peric1_usi08_usi_blk_cmu_lut, cmucal_vclk_clk_peric1_usi08_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI06_USI_BLK_CMU, clk_peric1_usi06_usi_blk_cmu_lut, cmucal_vclk_clk_peric1_usi06_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI14_USI_BLK_CMU, clk_peric0_usi14_usi_blk_cmu_lut, cmucal_vclk_clk_peric0_usi14_usi_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLK_CLUSTER0_SCLK_BLK_CPUCL0, mux_clk_cluster0_sclk_blk_cpucl0_lut, cmucal_vclk_mux_clk_cluster0_sclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CPUCL0_CMUREF_BLK_CPUCL0, spl_clk_cpucl0_cmuref_blk_cpucl0_lut, cmucal_vclk_spl_clk_cpucl0_cmuref_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_CLUSTER0_ACLKP_BLK_CPUCL0, clk_cluster0_aclkp_blk_cpucl0_lut, cmucal_vclk_clk_cluster0_aclkp_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER0_PERIPHCLK_BLK_CPUCL0, div_clk_cluster0_periphclk_blk_cpucl0_lut, cmucal_vclk_div_clk_cluster0_periphclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_CLUSTER0_ACLK_BLK_CPUCL0, clk_cluster0_aclk_blk_cpucl0_lut, cmucal_vclk_clk_cluster0_aclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER1_ACLK_BLK_CPUCL1, div_clk_cluster1_aclk_blk_cpucl1_lut, cmucal_vclk_div_clk_cluster1_aclk_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL1_PCLKDBG_BLK_CPUCL1, div_clk_cpucl1_pclkdbg_blk_cpucl1_lut, cmucal_vclk_div_clk_cpucl1_pclkdbg_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER1_ATCLK_BLK_CPUCL1, div_clk_cluster1_atclk_blk_cpucl1_lut, cmucal_vclk_div_clk_cluster1_atclk_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CPUCL1_CMUREF_BLK_CPUCL1, spl_clk_cpucl1_cmuref_blk_cpucl1_lut, cmucal_vclk_spl_clk_cpucl1_cmuref_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_MIF_CMUREF_BLK_MIF, occ_mif_cmuref_blk_mif_lut, cmucal_vclk_occ_mif_cmuref_blk_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_UART_DBG_BLK_PERIC0, clk_peric0_uart_dbg_blk_peric0_lut, cmucal_vclk_clk_peric0_uart_dbg_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI01_USI_BLK_PERIC0, clk_peric0_usi01_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi01_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI03_USI_BLK_PERIC0, clk_peric0_usi03_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi03_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI05_USI_BLK_PERIC0, clk_peric0_usi05_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi05_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI13_USI_BLK_PERIC0, clk_peric0_usi13_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi13_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI04_USI_BLK_PERIC0, clk_peric0_usi04_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi04_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI02_USI_BLK_PERIC0, clk_peric0_usi02_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi02_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI12_USI_BLK_PERIC0, clk_peric0_usi12_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi12_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI00_USI_BLK_PERIC0, clk_peric0_usi00_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi00_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC0_USI14_USI_BLK_PERIC0, clk_peric0_usi14_usi_blk_peric0_lut, cmucal_vclk_clk_peric0_usi14_usi_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM0_BLK_PERIC1, clk_peric1_i2c_cam0_blk_peric1_lut, cmucal_vclk_clk_peric1_i2c_cam0_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM2_BLK_PERIC1, clk_peric1_i2c_cam2_blk_peric1_lut, cmucal_vclk_clk_peric1_i2c_cam2_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_SPI_CAM0_BLK_PERIC1, clk_peric1_spi_cam0_blk_peric1_lut, cmucal_vclk_clk_peric1_spi_cam0_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI06_USI_BLK_PERIC1, clk_peric1_usi06_usi_blk_peric1_lut, cmucal_vclk_clk_peric1_usi06_usi_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI08_USI_BLK_PERIC1, clk_peric1_usi08_usi_blk_peric1_lut, cmucal_vclk_clk_peric1_usi08_usi_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI10_USI_BLK_PERIC1, clk_peric1_usi10_usi_blk_peric1_lut, cmucal_vclk_clk_peric1_usi10_usi_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM1_BLK_PERIC1, clk_peric1_i2c_cam1_blk_peric1_lut, cmucal_vclk_clk_peric1_i2c_cam1_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI07_USI_BLK_PERIC1, clk_peric1_usi07_usi_blk_peric1_lut, cmucal_vclk_clk_peric1_usi07_usi_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_UART_BT_BLK_PERIC1, clk_peric1_uart_bt_blk_peric1_lut, cmucal_vclk_clk_peric1_uart_bt_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI09_USI_BLK_PERIC1, clk_peric1_usi09_usi_blk_peric1_lut, cmucal_vclk_clk_peric1_usi09_usi_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_I2C_CAM3_BLK_PERIC1, clk_peric1_i2c_cam3_blk_peric1_lut, cmucal_vclk_clk_peric1_i2c_cam3_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLK_PERIC1_USI11_USI_BLK_PERIC1, clk_peric1_usi11_usi_blk_peric1_lut, cmucal_vclk_clk_peric1_usi11_usi_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_VTS_DMIC_IF_DIV2_BLK_VTS, spl_clk_vts_dmic_if_div2_blk_vts_lut, cmucal_vclk_spl_clk_vts_dmic_if_div2_blk_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_VTS_DMIC_BLK_VTS, div_clk_vts_dmic_blk_vts_lut, cmucal_vclk_div_clk_vts_dmic_blk_vts, NULL, NULL),

/* COMMON VCLK */
	CMUCAL_VCLK(VCLK_BLK_APM, blk_apm_lut, cmucal_vclk_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_AUD, blk_aud_lut, cmucal_vclk_blk_aud, NULL, vclk_switch_blk_aud),
	CMUCAL_VCLK(VCLK_BLK_BUSC, blk_busc_lut, cmucal_vclk_blk_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CHUB, blk_chub_lut, cmucal_vclk_blk_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CMGP, blk_cmgp_lut, cmucal_vclk_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CMU, blk_cmu_lut, cmucal_vclk_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CORE, blk_core_lut, cmucal_vclk_blk_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CPUCL0, blk_cpucl0_lut, cmucal_vclk_blk_cpucl0, NULL, vclk_switch_blk_cpucl0),
	CMUCAL_VCLK(VCLK_BLK_CPUCL1, blk_cpucl1_lut, cmucal_vclk_blk_cpucl1, NULL, vclk_switch_blk_cpucl1),
	CMUCAL_VCLK(VCLK_BLK_DCF, blk_dcf_lut, cmucal_vclk_blk_dcf, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DCPOST, blk_dcpost_lut, cmucal_vclk_blk_dcpost, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DCRD, blk_dcrd_lut, cmucal_vclk_blk_dcrd, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DPU, blk_dpu_lut, cmucal_vclk_blk_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DSPM, blk_dspm_lut, cmucal_vclk_blk_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DSPS, blk_dsps_lut, cmucal_vclk_blk_dsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G2D, blk_g2d_lut, cmucal_vclk_blk_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G3D, blk_g3d_lut, cmucal_vclk_blk_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPHQ, blk_isphq_lut, cmucal_vclk_blk_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPLP, blk_isplp_lut, cmucal_vclk_blk_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPPRE, blk_isppre_lut, cmucal_vclk_blk_isppre, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_IVA, blk_iva_lut, cmucal_vclk_blk_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MFC, blk_mfc_lut, cmucal_vclk_blk_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MIF, blk_mif_lut, cmucal_vclk_blk_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_PERIC0, blk_peric0_lut, cmucal_vclk_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_PERIC1, blk_peric1_lut, cmucal_vclk_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_S2D, blk_s2d_lut, cmucal_vclk_blk_s2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_VTS, blk_vts_lut, cmucal_vclk_blk_vts, NULL, NULL),

/* GATING VCLK */
	CMUCAL_VCLK(VCLK_APBIF_GPIO_ALIVE, NULL, cmucal_vclk_apbif_gpio_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBIF_PMU_ALIVE, NULL, cmucal_vclk_apbif_pmu_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBIF_RTC, NULL, cmucal_vclk_apbif_rtc, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBIF_TOP_RTC, NULL, cmucal_vclk_apbif_top_rtc, NULL, NULL),
	CMUCAL_VCLK(VCLK_APM_CMU_APM, NULL, cmucal_vclk_apm_cmu_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_GREBEINTEGRATION, NULL, cmucal_vclk_grebeintegration, NULL, NULL),
	CMUCAL_VCLK(VCLK_INTMEM, NULL, cmucal_vclk_intmem, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_APM, NULL, cmucal_vclk_lhm_axi_p_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_APM_CHUB, NULL, cmucal_vclk_lhm_axi_p_apm_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_APM_CP, NULL, cmucal_vclk_lhm_axi_p_apm_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_APM_GNSS, NULL, cmucal_vclk_lhm_axi_p_apm_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_APM, NULL, cmucal_vclk_lhs_axi_d_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G_SCAN2DRAM, NULL, cmucal_vclk_lhs_axi_g_scan2dram, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_LP_CHUB, NULL, cmucal_vclk_lhs_axi_lp_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_APM2CMGP, NULL, cmucal_vclk_lhs_axi_p_apm2cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_AP2CHUB, NULL, cmucal_vclk_mailbox_ap2chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_AP2CP, NULL, cmucal_vclk_mailbox_ap2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_AP2CP_S, NULL, cmucal_vclk_mailbox_ap2cp_s, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_AP2GNSS, NULL, cmucal_vclk_mailbox_ap2gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_AP2VTS, NULL, cmucal_vclk_mailbox_ap2vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2AP, NULL, cmucal_vclk_mailbox_apm2ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2CHUB, NULL, cmucal_vclk_mailbox_apm2chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2CP, NULL, cmucal_vclk_mailbox_apm2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2GNSS, NULL, cmucal_vclk_mailbox_apm2gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_CHUB2CP, NULL, cmucal_vclk_mailbox_chub2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_GNSS2CHUB, NULL, cmucal_vclk_mailbox_gnss2chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_GNSS2CP, NULL, cmucal_vclk_mailbox_gnss2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_PEM, NULL, cmucal_vclk_pem, NULL, NULL),
	CMUCAL_VCLK(VCLK_PGEN_APM, NULL, cmucal_vclk_pgen_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_INTR_GEN, NULL, cmucal_vclk_pmu_intr_gen, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY_APM, NULL, cmucal_vclk_speedy_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY_SUB_APM, NULL, cmucal_vclk_speedy_sub_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_APM, NULL, cmucal_vclk_sysreg_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_APM, NULL, cmucal_vclk_wdt_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_DP_APM, NULL, cmucal_vclk_xiu_dp_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_ABOX, NULL, cmucal_vclk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_ABOX_DAP, NULL, cmucal_vclk_abox_dap, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_AUD, NULL, cmucal_vclk_ad_apb_sysmmu_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_AUD, NULL, cmucal_vclk_axi2apb_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_DFTMUX_AUD, NULL, cmucal_vclk_dftmux_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMIC, NULL, cmucal_vclk_dmic, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_AUD, NULL, cmucal_vclk_lhm_axi_p_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_AUD, NULL, cmucal_vclk_lhs_atb_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERI_AXI_ASB, NULL, cmucal_vclk_peri_axi_asb, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_AUD, NULL, cmucal_vclk_xiu_p_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUS1, NULL, cmucal_vclk_axi2apb_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUS1_TREX, NULL, cmucal_vclk_axi2apb_bus1_trex, NULL, NULL),
	CMUCAL_VCLK(VCLK_BAAW_P_CHUB, NULL, cmucal_vclk_baaw_p_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_BAAW_P_GNSS, NULL, cmucal_vclk_baaw_p_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUS1_CMU_BUS1, NULL, cmucal_vclk_bus1_cmu_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_APM, NULL, cmucal_vclk_lhm_axi_d_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_CHUB, NULL, cmucal_vclk_lhm_axi_d_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_GNSS, NULL, cmucal_vclk_lhm_axi_d_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_G_CSSYS, NULL, cmucal_vclk_lhm_axi_g_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_BUS1, NULL, cmucal_vclk_lhs_axi_d_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CHUB, NULL, cmucal_vclk_lhs_axi_p_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CSSYS, NULL, cmucal_vclk_lhs_axi_p_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_GNSS, NULL, cmucal_vclk_lhs_axi_p_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_BUS1, NULL, cmucal_vclk_sysreg_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P_BUS1, NULL, cmucal_vclk_trex_p_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_BUS1, NULL, cmucal_vclk_xiu_d_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_BUSC, NULL, cmucal_vclk_hpm_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CHUB00, NULL, cmucal_vclk_i2c_chub00, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CHUB01, NULL, cmucal_vclk_i2c_chub01, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI_CHUB00, NULL, cmucal_vclk_usi_chub00, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI_CHUB01, NULL, cmucal_vclk_usi_chub01, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADC_CMGP, NULL, cmucal_vclk_adc_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CMGP0, NULL, cmucal_vclk_axi2apb_cmgp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CMGP1, NULL, cmucal_vclk_axi2apb_cmgp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CMGP_CMU_CMGP, NULL, cmucal_vclk_cmgp_cmu_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_CMGP, NULL, cmucal_vclk_gpio_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CMGP00, NULL, cmucal_vclk_i2c_cmgp00, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CMGP01, NULL, cmucal_vclk_i2c_cmgp01, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CMGP02, NULL, cmucal_vclk_i2c_cmgp02, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CMGP03, NULL, cmucal_vclk_i2c_cmgp03, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_APM2CMGP, NULL, cmucal_vclk_lhm_axi_p_apm2cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CMGP, NULL, cmucal_vclk_sysreg_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CMGP2CHUB, NULL, cmucal_vclk_sysreg_cmgp2chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CMGP2CP, NULL, cmucal_vclk_sysreg_cmgp2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CMGP2GNSS, NULL, cmucal_vclk_sysreg_cmgp2gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CMGP2PMU_AP, NULL, cmucal_vclk_sysreg_cmgp2pmu_ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CMGP2PMU_CHUB, NULL, cmucal_vclk_sysreg_cmgp2pmu_chub, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI_CMGP00, NULL, cmucal_vclk_usi_cmgp00, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI_CMGP01, NULL, cmucal_vclk_usi_cmgp01, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI_CMGP02, NULL, cmucal_vclk_usi_cmgp02, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI_CMGP03, NULL, cmucal_vclk_usi_cmgp03, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_CMGP, NULL, cmucal_vclk_xiu_p_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADM_APB_G_BDU, NULL, cmucal_vclk_adm_apb_g_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_APB_ASYNC_PPFW_DP, NULL, cmucal_vclk_apb_async_ppfw_dp, NULL, NULL),
	CMUCAL_VCLK(VCLK_APB_ASYNC_PPFW_G3D, NULL, cmucal_vclk_apb_async_ppfw_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_APB_ASYNC_PPFW_IO, NULL, cmucal_vclk_apb_async_ppfw_io, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CORE, NULL, cmucal_vclk_axi2apb_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CORE_TP, NULL, cmucal_vclk_axi2apb_core_tp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BAAW_CP, NULL, cmucal_vclk_baaw_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BDU, NULL, cmucal_vclk_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BPS_P_G3D, NULL, cmucal_vclk_bps_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMCORE, NULL, cmucal_vclk_busif_hpmcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_CCI, NULL, cmucal_vclk_cci, NULL, NULL),
	CMUCAL_VCLK(VCLK_CORE_CMU_CORE, NULL, cmucal_vclk_core_cmu_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CORE, NULL, cmucal_vclk_hpm_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_APM, NULL, cmucal_vclk_lhs_axi_p_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CP, NULL, cmucal_vclk_lhs_axi_p_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CPUCL0, NULL, cmucal_vclk_lhs_axi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CPUCL1, NULL, cmucal_vclk_lhs_axi_p_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_G3D, NULL, cmucal_vclk_lhs_axi_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPCFW_G3D, NULL, cmucal_vclk_ppcfw_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMUPPC_CCI, NULL, cmucal_vclk_ppmuppc_cci, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_CPUCL0, NULL, cmucal_vclk_ppmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_CPUCL1, NULL, cmucal_vclk_ppmu_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_G3D0, NULL, cmucal_vclk_ppmu_g3d0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_G3D1, NULL, cmucal_vclk_ppmu_g3d1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_G3D2, NULL, cmucal_vclk_ppmu_g3d2, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_G3D3, NULL, cmucal_vclk_ppmu_g3d3, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CORE, NULL, cmucal_vclk_sysreg_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_D_CORE, NULL, cmucal_vclk_trex_d_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P0_CORE, NULL, cmucal_vclk_trex_p0_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P1_CORE, NULL, cmucal_vclk_trex_p1_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADM_APB_G_CLUSTER0, NULL, cmucal_vclk_adm_apb_g_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADS_AHB_G_SSS, NULL, cmucal_vclk_ads_ahb_g_sss, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADS_APB_G_BDU, NULL, cmucal_vclk_ads_apb_g_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADS_APB_G_CLUSTER0, NULL, cmucal_vclk_ads_apb_g_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADS_APB_G_CLUSTER1, NULL, cmucal_vclk_ads_apb_g_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADS_APB_G_DSPM, NULL, cmucal_vclk_ads_apb_g_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_ASB_APB_P_DUMPPC_CLUSTER0, NULL, cmucal_vclk_asb_apb_p_dumppc_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_ASB_APB_P_DUMPPC_CLUSTER1, NULL, cmucal_vclk_asb_apb_p_dumppc_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CPUCL0, NULL, cmucal_vclk_axi2apb_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_P_CSSYS, NULL, cmucal_vclk_axi2apb_p_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI_DS_64to32_G_CSSYS, NULL, cmucal_vclk_axi_ds_64to32_g_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMCPUCL0, NULL, cmucal_vclk_busif_hpmcpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLUSTER0, NULL, cmucal_vclk_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CPUCL0_CMU_CPUCL0, NULL, cmucal_vclk_cpucl0_cmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CSSYS, NULL, cmucal_vclk_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_DUMPPC_CLUSTER0, NULL, cmucal_vclk_dumppc_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DUMPPC_CLUSTER1, NULL, cmucal_vclk_dumppc_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CPUCL0, NULL, cmucal_vclk_hpm_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T0_CLUSTER0, NULL, cmucal_vclk_lhm_atb_t0_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T0_CLUSTER1, NULL, cmucal_vclk_lhm_atb_t0_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T1_CLUSTER0, NULL, cmucal_vclk_lhm_atb_t1_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T1_CLUSTER1, NULL, cmucal_vclk_lhm_atb_t1_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T2_CLUSTER0, NULL, cmucal_vclk_lhm_atb_t2_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T2_CLUSTER1, NULL, cmucal_vclk_lhm_atb_t2_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T3_CLUSTER0, NULL, cmucal_vclk_lhm_atb_t3_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T3_CLUSTER1, NULL, cmucal_vclk_lhm_atb_t3_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T_AUD, NULL, cmucal_vclk_lhm_atb_t_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_T_BDU, NULL, cmucal_vclk_lhm_atb_t_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CPUCL0, NULL, cmucal_vclk_lhm_axi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CSSYS, NULL, cmucal_vclk_lhm_axi_p_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACE_D_CLUSTER0, NULL, cmucal_vclk_lhs_ace_d_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_T0_CLUSTER0, NULL, cmucal_vclk_lhs_atb_t0_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_T1_CLUSTER0, NULL, cmucal_vclk_lhs_atb_t1_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_T2_CLUSTER0, NULL, cmucal_vclk_lhs_atb_t2_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_T3_CLUSTER0, NULL, cmucal_vclk_lhs_atb_t3_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G_CSSYS, NULL, cmucal_vclk_lhs_axi_g_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G_ETR, NULL, cmucal_vclk_lhs_axi_g_etr, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CLUSTER0, NULL, cmucal_vclk_lhs_axi_p_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SECJTAG, NULL, cmucal_vclk_secjtag, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CPUCL0, NULL, cmucal_vclk_sysreg_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CPUCL1, NULL, cmucal_vclk_axi2apb_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMCPUCL1, NULL, cmucal_vclk_busif_hpmcpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLUSTER1, NULL, cmucal_vclk_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CPUCL1_CMU_CPUCL1, NULL, cmucal_vclk_cpucl1_cmu_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CPUCL1_0, NULL, cmucal_vclk_hpm_cpucl1_0, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CPUCL1_1, NULL, cmucal_vclk_hpm_cpucl1_1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CPUCL1_2, NULL, cmucal_vclk_hpm_cpucl1_2, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CPUCL1, NULL, cmucal_vclk_lhm_axi_p_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CPUCL1, NULL, cmucal_vclk_sysreg_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DCPOST_CMU_DCPOST, NULL, cmucal_vclk_dcpost_cmu_dcpost, NULL, NULL),
	CMUCAL_VCLK(VCLK_IS_DCPOST, NULL, cmucal_vclk_is_dcpost, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_DCFDCPOST, NULL, cmucal_vclk_lhm_atb_dcfdcpost, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_DCRDDCPOST, NULL, cmucal_vclk_lhm_atb_dcrddcpost, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DCFDCPOST, NULL, cmucal_vclk_lhm_axi_p_dcfdcpost, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_DCPOSTDCF, NULL, cmucal_vclk_lhs_atb_dcpostdcf, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_DCPOSTDCRD, NULL, cmucal_vclk_lhs_atb_dcpostdcrd, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_DCPOSTDCF, NULL, cmucal_vclk_lhs_axi_d_dcpostdcf, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON0, NULL, cmucal_vclk_ad_apb_decon0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON1, NULL, cmucal_vclk_ad_apb_decon1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON2, NULL, cmucal_vclk_ad_apb_decon2, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPP, NULL, cmucal_vclk_ad_apb_dpp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPU_DMA, NULL, cmucal_vclk_ad_apb_dpu_dma, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPU_DMA_PGEN, NULL, cmucal_vclk_ad_apb_dpu_dma_pgen, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPU_WB_MUX, NULL, cmucal_vclk_ad_apb_dpu_wb_mux, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_MIPI_DSIM0, NULL, cmucal_vclk_ad_apb_mipi_dsim0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_MIPI_DSIM1, NULL, cmucal_vclk_ad_apb_mipi_dsim1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_DPUD0, NULL, cmucal_vclk_ad_apb_sysmmu_dpud0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_DPUD0_S, NULL, cmucal_vclk_ad_apb_sysmmu_dpud0_s, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_DPUD1, NULL, cmucal_vclk_ad_apb_sysmmu_dpud1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_DPUD1_S, NULL, cmucal_vclk_ad_apb_sysmmu_dpud1_s, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_DPUD2, NULL, cmucal_vclk_ad_apb_sysmmu_dpud2, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SYSMMU_DPUD2_S, NULL, cmucal_vclk_ad_apb_sysmmu_dpud2_s, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DPUP0, NULL, cmucal_vclk_axi2apb_dpup0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DPUP1, NULL, cmucal_vclk_axi2apb_dpup1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DPUD0, NULL, cmucal_vclk_btm_dpud0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DPUD1, NULL, cmucal_vclk_btm_dpud1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DPUD2, NULL, cmucal_vclk_btm_dpud2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DPU_CMU_DPU, NULL, cmucal_vclk_dpu_cmu_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DPU, NULL, cmucal_vclk_lhm_axi_p_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_DPUD0, NULL, cmucal_vclk_ppmu_dpud0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_DPUD1, NULL, cmucal_vclk_ppmu_dpud1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_DPUD2, NULL, cmucal_vclk_ppmu_dpud2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DPU, NULL, cmucal_vclk_sysreg_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_DPU, NULL, cmucal_vclk_xiu_p_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_wrapper_for_s5i6211_hsi_dcphy_combo_top, NULL, cmucal_vclk_wrapper_for_s5i6211_hsi_dcphy_combo_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADM_APB_DSPM, NULL, cmucal_vclk_adm_apb_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DSPM0, NULL, cmucal_vclk_ad_apb_dspm0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DSPM1, NULL, cmucal_vclk_ad_apb_dspm1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DSPM3, NULL, cmucal_vclk_ad_apb_dspm3, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DSPM4, NULL, cmucal_vclk_ad_apb_dspm4, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_AXI_DSPM0, NULL, cmucal_vclk_ad_axi_dspm0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DSPM, NULL, cmucal_vclk_axi2apb_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DSPM0, NULL, cmucal_vclk_btm_dspm0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DSPM1, NULL, cmucal_vclk_btm_dspm1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DSPM_CMU_DSPM, NULL, cmucal_vclk_dspm_cmu_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D0_DSPSDSPM, NULL, cmucal_vclk_lhm_axi_d0_dspsdspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D1_DSPSDSPM, NULL, cmucal_vclk_lhm_axi_d1_dspsdspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DSPM, NULL, cmucal_vclk_lhm_axi_p_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_IVADSPM, NULL, cmucal_vclk_lhm_axi_p_ivadspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D0_DSPM, NULL, cmucal_vclk_lhs_acel_d0_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D1_DSPM, NULL, cmucal_vclk_lhs_acel_d1_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D2_DSPM, NULL, cmucal_vclk_lhs_acel_d2_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_DSPMDSPS, NULL, cmucal_vclk_lhs_axi_p_dspmdsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_DSPMIVA, NULL, cmucal_vclk_lhs_axi_p_dspmiva, NULL, NULL),
	CMUCAL_VCLK(VCLK_PGEN_lite_DSPM, NULL, cmucal_vclk_pgen_lite_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_DSPM0, NULL, cmucal_vclk_ppmu_dspm0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_DSPM1, NULL, cmucal_vclk_ppmu_dspm1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SCORE_MASTER, NULL, cmucal_vclk_score_master, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSMMU_DSPM0, NULL, cmucal_vclk_sysmmu_dspm0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSMMU_DSPM1, NULL, cmucal_vclk_sysmmu_dspm1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DSPM, NULL, cmucal_vclk_sysreg_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_WRAP2_CONV_DSPM, NULL, cmucal_vclk_wrap2_conv_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_DSPM, NULL, cmucal_vclk_xiu_p_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DSPS, NULL, cmucal_vclk_axi2apb_dsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_DSPS_CMU_DSPS, NULL, cmucal_vclk_dsps_cmu_dsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DSPMDSPS, NULL, cmucal_vclk_lhm_axi_p_dspmdsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DSPS, NULL, cmucal_vclk_sysreg_dsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_DP_LINK, NULL, cmucal_vclk_dp_link, NULL, NULL),
	CMUCAL_VCLK(VCLK_UFS_EMBD, NULL, cmucal_vclk_ufs_embd, NULL, NULL),
	CMUCAL_VCLK(VCLK_USB30DRD, NULL, cmucal_vclk_usb30drd, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADM_AHB_SSS, NULL, cmucal_vclk_adm_ahb_sss, NULL, NULL),
	CMUCAL_VCLK(VCLK_AHBBR_FSYS1, NULL, cmucal_vclk_ahbbr_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2AHB_FSYS1, NULL, cmucal_vclk_axi2ahb_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_FSYS1P0, NULL, cmucal_vclk_axi2apb_fsys1p0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_FSYS1P1, NULL, cmucal_vclk_axi2apb_fsys1p1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_FSYS1, NULL, cmucal_vclk_btm_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_FSYS1_CMU_FSYS1, NULL, cmucal_vclk_fsys1_cmu_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_FSYS1, NULL, cmucal_vclk_gpio_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_FSYS1, NULL, cmucal_vclk_lhm_axi_p_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D_FSYS1, NULL, cmucal_vclk_lhs_acel_d_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_MMC_CARD, NULL, cmucal_vclk_mmc_card, NULL, NULL),
	CMUCAL_VCLK(VCLK_PCIE_GEN2, NULL, cmucal_vclk_pcie_gen2, NULL, NULL),
	CMUCAL_VCLK(VCLK_PCIE_GEN3, NULL, cmucal_vclk_pcie_gen3, NULL, NULL),
	CMUCAL_VCLK(VCLK_PCIE_IA_GEN2, NULL, cmucal_vclk_pcie_ia_gen2, NULL, NULL),
	CMUCAL_VCLK(VCLK_PCIE_IA_GEN3, NULL, cmucal_vclk_pcie_ia_gen3, NULL, NULL),
	CMUCAL_VCLK(VCLK_PGEN_LITE_FSYS1, NULL, cmucal_vclk_pgen_lite_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_FSYS1, NULL, cmucal_vclk_ppmu_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_RTIC, NULL, cmucal_vclk_rtic, NULL, NULL),
	CMUCAL_VCLK(VCLK_SSS, NULL, cmucal_vclk_sss, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSMMU_FSYS1, NULL, cmucal_vclk_sysmmu_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_FSYS1, NULL, cmucal_vclk_sysreg_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_UFS_CARD, NULL, cmucal_vclk_ufs_card, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_FSYS1, NULL, cmucal_vclk_xiu_d_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_PCIE_GEN2_DBI, NULL, cmucal_vclk_xiu_pcie_gen2_dbi, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_PCIE_GEN2_SLV, NULL, cmucal_vclk_xiu_pcie_gen2_slv, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_PCIE_GEN3_DBI, NULL, cmucal_vclk_xiu_pcie_gen3_dbi, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_PCIE_GEN3_SLV, NULL, cmucal_vclk_xiu_pcie_gen3_slv, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_FSYS1, NULL, cmucal_vclk_xiu_p_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_G3D, NULL, cmucal_vclk_axi2apb_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMG3D, NULL, cmucal_vclk_busif_hpmg3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_G3D_CMU_G3D, NULL, cmucal_vclk_g3d_cmu_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPU, NULL, cmucal_vclk_gpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_GRAY2BIN_G3D, NULL, cmucal_vclk_gray2bin_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_G3D0, NULL, cmucal_vclk_hpm_g3d0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_G3DSFR, NULL, cmucal_vclk_lhm_axi_g3dsfr, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_G3D, NULL, cmucal_vclk_lhm_axi_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G3DSFR, NULL, cmucal_vclk_lhs_axi_g3dsfr, NULL, NULL),
	CMUCAL_VCLK(VCLK_PGEN_LITE_G3D, NULL, cmucal_vclk_pgen_lite_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_G3D, NULL, cmucal_vclk_sysreg_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_G3D, NULL, cmucal_vclk_xiu_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADM_DAP_IVA, NULL, cmucal_vclk_adm_dap_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_IVA0, NULL, cmucal_vclk_ad_apb_iva0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_IVA1, NULL, cmucal_vclk_ad_apb_iva1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_IVA2, NULL, cmucal_vclk_ad_apb_iva2, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_2M_IVA, NULL, cmucal_vclk_axi2apb_2m_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_IVA, NULL, cmucal_vclk_axi2apb_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_IVA, NULL, cmucal_vclk_btm_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_IVA, NULL, cmucal_vclk_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_IVA_CMU_IVA, NULL, cmucal_vclk_iva_cmu_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_IVA, NULL, cmucal_vclk_lhm_axi_p_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_PGEN_lite_IVA, NULL, cmucal_vclk_pgen_lite_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPMU_IVA, NULL, cmucal_vclk_ppmu_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_IVA, NULL, cmucal_vclk_sysreg_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_RB_IVA, NULL, cmucal_vclk_trex_rb_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_IVA, NULL, cmucal_vclk_xiu_p_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_MIF, NULL, cmucal_vclk_hpm_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC0P0, NULL, cmucal_vclk_axi2apb_peric0p0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC0P1, NULL, cmucal_vclk_axi2apb_peric0p1, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_PERIC0, NULL, cmucal_vclk_gpio_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_PERIC0, NULL, cmucal_vclk_lhm_axi_p_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERIC0_CMU_PERIC0, NULL, cmucal_vclk_peric0_cmu_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PWM, NULL, cmucal_vclk_pwm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_PERIC0, NULL, cmucal_vclk_sysreg_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_UART_DBG, NULL, cmucal_vclk_uart_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI00_I2C, NULL, cmucal_vclk_usi00_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI00_USI, NULL, cmucal_vclk_usi00_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI01_I2C, NULL, cmucal_vclk_usi01_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI01_USI, NULL, cmucal_vclk_usi01_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI02_I2C, NULL, cmucal_vclk_usi02_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI02_USI, NULL, cmucal_vclk_usi02_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI03_I2C, NULL, cmucal_vclk_usi03_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI03_USI, NULL, cmucal_vclk_usi03_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI04_I2C, NULL, cmucal_vclk_usi04_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI04_USI, NULL, cmucal_vclk_usi04_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI05_I2C, NULL, cmucal_vclk_usi05_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI05_USI, NULL, cmucal_vclk_usi05_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI12_I2C, NULL, cmucal_vclk_usi12_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI12_USI, NULL, cmucal_vclk_usi12_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI13_I2C, NULL, cmucal_vclk_usi13_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI13_USI, NULL, cmucal_vclk_usi13_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI14_I2C, NULL, cmucal_vclk_usi14_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI14_USI, NULL, cmucal_vclk_usi14_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_PERIC0, NULL, cmucal_vclk_xiu_p_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC1P0, NULL, cmucal_vclk_axi2apb_peric1p0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC1P1, NULL, cmucal_vclk_axi2apb_peric1p1, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_PERIC1, NULL, cmucal_vclk_gpio_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CAM0, NULL, cmucal_vclk_i2c_cam0, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CAM1, NULL, cmucal_vclk_i2c_cam1, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CAM2, NULL, cmucal_vclk_i2c_cam2, NULL, NULL),
	CMUCAL_VCLK(VCLK_I2C_CAM3, NULL, cmucal_vclk_i2c_cam3, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_PERIC1, NULL, cmucal_vclk_lhm_axi_p_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERIC1_CMU_PERIC1, NULL, cmucal_vclk_peric1_cmu_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPI_CAM0, NULL, cmucal_vclk_spi_cam0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_PERIC1, NULL, cmucal_vclk_sysreg_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_UART_BT, NULL, cmucal_vclk_uart_bt, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI06_I2C, NULL, cmucal_vclk_usi06_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI06_USI, NULL, cmucal_vclk_usi06_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI07_I2C, NULL, cmucal_vclk_usi07_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI07_USI, NULL, cmucal_vclk_usi07_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI08_I2C, NULL, cmucal_vclk_usi08_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI08_USI, NULL, cmucal_vclk_usi08_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI09_I2C, NULL, cmucal_vclk_usi09_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI09_USI, NULL, cmucal_vclk_usi09_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI10_I2C, NULL, cmucal_vclk_usi10_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI10_USI, NULL, cmucal_vclk_usi10_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI11_I2C, NULL, cmucal_vclk_usi11_i2c, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI11_USI, NULL, cmucal_vclk_usi11_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_PERIC1, NULL, cmucal_vclk_xiu_p_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_AXI_P_PERIS, NULL, cmucal_vclk_ad_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERISP, NULL, cmucal_vclk_axi2apb_perisp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_TMU, NULL, cmucal_vclk_busif_tmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_GIC, NULL, cmucal_vclk_gic, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_PERIS, NULL, cmucal_vclk_lhm_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_MCT, NULL, cmucal_vclk_mct, NULL, NULL),
	CMUCAL_VCLK(VCLK_OTP_CON_BIRA, NULL, cmucal_vclk_otp_con_bira, NULL, NULL),
	CMUCAL_VCLK(VCLK_OTP_CON_TOP, NULL, cmucal_vclk_otp_con_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERIS_CMU_PERIS, NULL, cmucal_vclk_peris_cmu_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_PERIS, NULL, cmucal_vclk_sysreg_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_CLUSTER0, NULL, cmucal_vclk_wdt_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_CLUSTER1, NULL, cmucal_vclk_wdt_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_PERIS, NULL, cmucal_vclk_xiu_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_S2D_CMU_S2D, NULL, cmucal_vclk_s2d_cmu_s2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMIC_IF, NULL, cmucal_vclk_dmic_if, NULL, NULL),
	CMUCAL_VCLK(VCLK_u_DMIC_CLK_MUX, NULL, cmucal_vclk_u_dmic_clk_mux, NULL, NULL),
};
unsigned int cmucal_vclk_size = 482;

