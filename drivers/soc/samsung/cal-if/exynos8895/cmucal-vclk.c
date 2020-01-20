#include "../cmucal.h"
#include "cmucal-node.h"
#include "cmucal-vclk.h"

#include "cmucal-vclklut.h"

/*=================CMUCAL version: S5E8895================================*/

/*=================CLK in each VCLK================================*/


/* DVFS List */
enum clk_id cmucal_vclk_vddi[] = {
	CLKCMU_FSYS0_BUS,
	CLKCMU_DPU_BUS,
	CLKCMU_BUS1_BUS,
	CLKCMU_MFC_BUS,
	CLKCMU_G2D_G2D,
	CLKCMU_FSYS1_BUS,
	CLKCMU_VPU_BUS,
	CLKCMU_DSP_BUS,
	CLKCMU_BUSC_BUS,
	CLKCMU_CAM_BUS,
	CLKCMU_CAM_TPU0,
	CLKCMU_CAM_TPU1,
	CLKCMU_ISPLP_BUS,
	CLKCMU_ISPHQ_BUS,
	CLKCMU_G2D_JPEG,
	CLKCMU_DBG_BUS,
	CLKCMU_IVA_BUS,
	CLKCMU_CAM_VRA,
	CLKCMU_DCAM_BUS,
	CLKCMU_IMEM_BUS,
	CLKCMU_SRDZ_IMGD,
	CLKCMU_SRDZ_BUS,
	CLKCMU_DCAM_IMGD,
	DIV_CLK_CMU_CMUREF,
	MUX_CLKCMU_DPU_BUS,
	MUX_CLKCMU_BUS1_BUS,
	MUX_CLKCMU_MFC_BUS,
	MUX_CLKCMU_VPU_BUS,
	MUX_CLKCMU_BUSC_BUS,
	MUX_CLKCMU_G2D_G2D,
	MUX_CLKCMU_DSP_BUS,
	MUX_CLKCMU_CAM_BUS,
	MUX_CLKCMU_CAM_TPU0,
	MUX_CLKCMU_CAM_TPU1,
	MUX_CLKCMU_ISPLP_BUS,
	MUX_CLKCMU_ISPHQ_BUS,
	MUX_CLKCMU_G2D_JPEG,
	MUX_CLKCMU_DBG_BUS,
	MUX_CLKCMU_FSYS0_BUS,
	MUX_CLKCMU_IVA_BUS,
	MUX_CLKCMU_CAM_VRA,
	MUX_CLKCMU_DCAM_BUS,
	MUX_CLKCMU_IMEM_BUS,
	MUX_CLKCMU_SRDZ_BUS,
	MUX_CLKCMU_SRDZ_IMGD,
	MUX_CLKCMU_DCAM_IMGD,
	MUX_CLK_CMU_CMUREF,
};
enum clk_id cmucal_vclk_vdd_mif[] = {
	PLL_MIF,
};
enum clk_id cmucal_vclk_vdd_g3d[] = {
	PLL_G3D,
};
enum clk_id cmucal_vclk_vdd_mngs[] = {
	PLL_CPUCL0,
	DIV_CLK_CLUSTER0_ACLK,
};
enum clk_id cmucal_vclk_vdd_apollo[] = {
	PLL_CPUCL1,
	DIV_CLK_CLUSTER1_ACLK,
};
enum clk_id cmucal_vclk_dfs_abox[] = {
	DIV_CLK_ABOX_BUS,
	DIV_CLK_ABOX_PLL,
};

/* SPECIAL List */
enum clk_id cmucal_vclk_spl_clk_abox_uaif0_blk_abox[] = {
	MUX_CLK_ABOX_UAIF0,
	DIV_CLK_ABOX_UAIF0,
};
enum clk_id cmucal_vclk_spl_clk_abox_uaif2_blk_abox[] = {
	MUX_CLK_ABOX_UAIF2,
	DIV_CLK_ABOX_UAIF2,
};
enum clk_id cmucal_vclk_spl_clk_abox_uaif4_blk_abox[] = {
	MUX_CLK_ABOX_UAIF4,
	DIV_CLK_ABOX_UAIF4,
};
enum clk_id cmucal_vclk_div_clk_abox_dmic_blk_abox[] = {
	DIV_CLK_ABOX_DMIC,
};
enum clk_id cmucal_vclk_spl_clk_abox_cpu_pclkdbg_blk_abox[] = {
	DIV_CLK_ABOX_CPU_PCLKDBG,
};
enum clk_id cmucal_vclk_spl_clk_abox_uaif1_blk_abox[] = {
	MUX_CLK_ABOX_UAIF1,
	DIV_CLK_ABOX_UAIF1,
};
enum clk_id cmucal_vclk_spl_clk_abox_uaif3_blk_abox[] = {
	MUX_CLK_ABOX_UAIF3,
	DIV_CLK_ABOX_UAIF3,
};
enum clk_id cmucal_vclk_spl_clk_abox_cpu_aclk_blk_abox[] = {
	DIV_CLK_ABOX_CPU_ACLK,
};
enum clk_id cmucal_vclk_spl_clk_fsys1_mmc_card_blk_cmu[] = {
	CLKCMU_FSYS1_MMC_CARD,
	MUX_CLKCMU_FSYS1_MMC_CARD,
};
enum clk_id cmucal_vclk_spl_clk_peric0_usi00_blk_cmu[] = {
	CLKCMU_PERIC0_USI00,
	MUX_CLKCMU_PERIC0_USI00,
};
enum clk_id cmucal_vclk_spl_clk_peric0_usi03_blk_cmu[] = {
	CLKCMU_PERIC0_USI03,
	MUX_CLKCMU_PERIC0_USI03,
};
enum clk_id cmucal_vclk_spl_clk_peric1_uart_bt_blk_cmu[] = {
	CLKCMU_PERIC1_UART_BT,
	MUX_CLKCMU_PERIC1_UART_BT,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi06_blk_cmu[] = {
	CLKCMU_PERIC1_USI06,
	MUX_CLKCMU_PERIC1_USI06,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi09_blk_cmu[] = {
	CLKCMU_PERIC1_USI09,
	MUX_CLKCMU_PERIC1_USI09,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi12_blk_cmu[] = {
	CLKCMU_PERIC1_USI12,
	MUX_CLKCMU_PERIC1_USI12,
};
enum clk_id cmucal_vclk_occ_cmu_cmuref_blk_cmu[] = {
	MUX_CMU_CMUREF,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk0_blk_cmu[] = {
	CLKCMU_CIS_CLK0,
	MUX_CLKCMU_CIS_CLK0,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk2_blk_cmu[] = {
	CLKCMU_CIS_CLK2,
	MUX_CLKCMU_CIS_CLK2,
};
enum clk_id cmucal_vclk_clkcmu_hpm_blk_cmu[] = {
	CLKCMU_HPM,
	MUX_CLKCMU_HPM,
};
enum clk_id cmucal_vclk_spl_clk_fsys0_dpgtc_blk_cmu[] = {
	CLKCMU_FSYS0_DPGTC,
	MUX_CLKCMU_FSYS0_DPGTC,
};
enum clk_id cmucal_vclk_spl_clk_fsys0_mmc_embd_blk_cmu[] = {
	CLKCMU_FSYS0_MMC_EMBD,
	MUX_CLKCMU_FSYS0_MMC_EMBD,
};
enum clk_id cmucal_vclk_spl_clk_fsys0_ufs_embd_blk_cmu[] = {
	CLKCMU_FSYS0_UFS_EMBD,
	MUX_CLKCMU_FSYS0_UFS_EMBD,
};
enum clk_id cmucal_vclk_spl_clk_fsys0_usbdrd30_blk_cmu[] = {
	CLKCMU_FSYS0_USBDRD30,
	MUX_CLKCMU_FSYS0_USBDRD30,
};
enum clk_id cmucal_vclk_spl_clk_fsys1_ufs_card_blk_cmu[] = {
	CLKCMU_FSYS1_UFS_CARD,
	MUX_CLKCMU_FSYS1_UFS_CARD,
};
enum clk_id cmucal_vclk_spl_clk_peric0_uart_dbg_blk_cmu[] = {
	CLKCMU_PERIC0_UART_DBG,
	MUX_CLKCMU_PERIC0_UART_DBG,
};
enum clk_id cmucal_vclk_spl_clk_peric0_usi01_blk_cmu[] = {
	CLKCMU_PERIC0_USI01,
	MUX_CLKCMU_PERIC0_USI01,
};
enum clk_id cmucal_vclk_spl_clk_peric0_usi02_blk_cmu[] = {
	CLKCMU_PERIC0_USI02,
	MUX_CLKCMU_PERIC0_USI02,
};
enum clk_id cmucal_vclk_spl_clk_peric1_spi_cam0_blk_cmu[] = {
	CLKCMU_PERIC1_SPI_CAM0,
	MUX_CLKCMU_PERIC1_SPI_CAM0,
};
enum clk_id cmucal_vclk_spl_clk_peric1_spi_cam1_blk_cmu[] = {
	CLKCMU_PERIC1_SPI_CAM1,
	MUX_CLKCMU_PERIC1_SPI_CAM1,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi04_blk_cmu[] = {
	CLKCMU_PERIC1_USI04,
	MUX_CLKCMU_PERIC1_USI04,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi05_blk_cmu[] = {
	CLKCMU_PERIC1_USI05,
	MUX_CLKCMU_PERIC1_USI05,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi07_blk_cmu[] = {
	CLKCMU_PERIC1_USI07,
	MUX_CLKCMU_PERIC1_USI07,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi08_blk_cmu[] = {
	CLKCMU_PERIC1_USI08,
	MUX_CLKCMU_PERIC1_USI08,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi10_blk_cmu[] = {
	CLKCMU_PERIC1_USI10,
	MUX_CLKCMU_PERIC1_USI10,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi11_blk_cmu[] = {
	CLKCMU_PERIC1_USI11,
	MUX_CLKCMU_PERIC1_USI11,
};
enum clk_id cmucal_vclk_spl_clk_peric1_usi13_blk_cmu[] = {
	CLKCMU_PERIC1_USI13,
	MUX_CLKCMU_PERIC1_USI13,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk1_blk_cmu[] = {
	CLKCMU_CIS_CLK1,
	MUX_CLKCMU_CIS_CLK1,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk3_blk_cmu[] = {
	CLKCMU_CIS_CLK3,
	MUX_CLKCMU_CIS_CLK3,
};
enum clk_id cmucal_vclk_occ_clk_fsys1_pcie_blk_cmu[] = {
	MUX_CLKCMU_FSYS1_PCIE,
};
enum clk_id cmucal_vclk_div_clk_cluster0_atclk_blk_cpucl0[] = {
	DIV_CLK_CLUSTER0_ATCLK,
};
enum clk_id cmucal_vclk_spl_clk_cpucl0_cmuref_blk_cpucl0[] = {
	DIV_CLK_CPUCL0_CMUREF,
};
enum clk_id cmucal_vclk_div_clk_cpucl0_pclkdbg_blk_cpucl0[] = {
	DIV_CLK_CPUCL0_PCLKDBG,
};
enum clk_id cmucal_vclk_div_clk_cluster1_cntclk_blk_cpucl1[] = {
	DIV_CLK_CLUSTER1_CNTCLK,
};
enum clk_id cmucal_vclk_div_clk_cpucl1_cmuref_blk_cpucl1[] = {
	DIV_CLK_CPUCL1_CMUREF,
};
enum clk_id cmucal_vclk_div_clk_cluster1_atclk_blk_cpucl1[] = {
	DIV_CLK_CLUSTER1_ATCLK,
};
enum clk_id cmucal_vclk_div_clk_cluster1_pclkdbg_blk_cpucl1[] = {
	DIV_CLK_CLUSTER1_PCLKDBG,
};
enum clk_id cmucal_vclk_spl_clk_dpu1_decon2_blk_dpu1[] = {
	DIV_CLKCMU_DPU1_DECON2,
	PLL_DPU,
};
enum clk_id cmucal_vclk_occ_mif_cmuref_blk_mif[] = {
	MUX_MIF_CMUREF,
};
enum clk_id cmucal_vclk_occ_mif1_cmuref_blk_mif1[] = {
	MUX_MIF1_CMUREF,
};
enum clk_id cmucal_vclk_occ_mif2_cmuref_blk_mif2[] = {
	MUX_MIF2_CMUREF,
};
enum clk_id cmucal_vclk_occ_mif3_cmuref_blk_mif3[] = {
	MUX_MIF3_CMUREF,
};
enum clk_id cmucal_vclk_spl_clk_vts_dmicif_div2_blk_vts[] = {
	DIV_CLK_VTS_DMIC_DIV2,
};
enum clk_id cmucal_vclk_div_clk_vts_dmic_blk_vts[] = {
	DIV_CLK_VTS_DMIC,
};

/* COMMON List */
enum clk_id cmucal_vclk_blk_abox[] = {
	DIV_CLK_ABOX_CPU_ATCLK,
	DIV_CLK_ABOX_BUSP,
	DIV_CLK_ABOX_AUDIF,
	PLL_AUD,
	DIV_CLK_ABOX_DSIF,
};
enum clk_id cmucal_vclk_blk_bus1[] = {
	DIV_CLK_BUS1_BUSP,
};
enum clk_id cmucal_vclk_blk_busc[] = {
	DIV_CLK_BUSC_BUSP,
};
enum clk_id cmucal_vclk_blk_cam[] = {
	DIV_CLK_CAM_BUSP,
};
enum clk_id cmucal_vclk_blk_cmu[] = {
	CLKCMU_APM_BUS,
	CLKCMU_PERIC0_BUS,
	CLKCMU_PERIS_BUS,
	CLKCMU_PERIC1_BUS,
	PLL_SHARED3,
	CLKCMU_BUSC_BUSPHSI2C,
	CLKCMU_PERIC1_SPEEDY2,
	CLKCMU_MODEM_SHARED0,
	CLKCMU_MODEM_SHARED1,
	MUX_CLKCMU_DROOPDETECTOR,
	MUX_CLKCMU_BUSC_BUSPHSI2C,
	MUX_CLKCMU_PERIC0_BUS,
	MUX_CLKCMU_PERIC1_BUS,
	MUX_CLKCMU_PERIS_BUS,
	MUX_CLKCMU_PERIC1_SPEEDY2,
	MUX_CLKCMU_APM_BUS,
	MUX_CLKCMU_FSYS1_BUS,
	PLL_SHARED4,
	PLL_SHARED1,
	PLL_SHARED2,
	PLL_SHARED0,
};
enum clk_id cmucal_vclk_blk_core[] = {
	DIV_CLK_CORE_BUSP,
};
enum clk_id cmucal_vclk_blk_cpucl0[] = {
	DIV_CLK_CPUCL0_PCLK,
};
enum clk_id cmucal_vclk_blk_cpucl1[] = {
	DIV_CLK_CPUCL1_PCLK,
};
enum clk_id cmucal_vclk_blk_dbg[] = {
	DIV_CLK_DBG_PCLKDBG,
};
enum clk_id cmucal_vclk_blk_dcam[] = {
	DIV_CLK_DCAM_BUSP,
};
enum clk_id cmucal_vclk_blk_dpu0[] = {
	DIV_CLK_DPU0_BUSP,
};
enum clk_id cmucal_vclk_blk_dsp[] = {
	DIV_CLK_DSP_BUSP,
};
enum clk_id cmucal_vclk_blk_g2d[] = {
	DIV_CLK_G2D_BUSP,
};
enum clk_id cmucal_vclk_blk_g3d[] = {
	DIV_CLK_G3D_BUSP,
};
enum clk_id cmucal_vclk_blk_isphq[] = {
	DIV_CLK_ISPHQ_BUSP,
};
enum clk_id cmucal_vclk_blk_isplp[] = {
	DIV_CLK_ISPLP_BUSP,
};
enum clk_id cmucal_vclk_blk_iva[] = {
	DIV_CLK_IVA_BUSP,
};
enum clk_id cmucal_vclk_blk_mfc[] = {
	DIV_CLK_MFC_BUSP,
};
enum clk_id cmucal_vclk_blk_mif[] = {
	DIV_CLK_MIF_BUSP,
	DIV_CLK_MIF_PRE,
};
enum clk_id cmucal_vclk_blk_mif1[] = {
	PLL_MIF1,
	DIV_CLK_MIF1_PRE,
	DIV_CLK_MIF1_BUSP,
};
enum clk_id cmucal_vclk_blk_mif2[] = {
	PLL_MIF2,
	DIV_CLK_MIF2_BUSP,
	DIV_CLK_MIF2_PRE,
};
enum clk_id cmucal_vclk_blk_mif3[] = {
	PLL_MIF3,
	DIV_CLK_MIF3_PRE,
	DIV_CLK_MIF3_BUSP,
};
enum clk_id cmucal_vclk_blk_srdz[] = {
	DIV_CLK_SRDZ_BUSP,
};
enum clk_id cmucal_vclk_blk_vpu[] = {
	DIV_CLK_VPU_BUSP,
};
enum clk_id cmucal_vclk_blk_vts[] = {
	DIV_CLK_VTS_BUS,
	MUX_CLK_VTS_CMUREF,
	DIV_CLK_VTS_DMICIF,
};

/* GATING List */
enum clk_id cmucal_vclk_abox_cmu_abox[] = {
	CLK_BLK_ABOX_UID_ABOX_CMU_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_abox_dap[] = {
	GOUT_BLK_ABOX_UID_ABOX_DAP_IPCLKPORT_dapclk,
};
enum clk_id cmucal_vclk_abox_top[] = {
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_ACLK,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_BCLK_DSIF,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_BCLK_UAIF0,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_BCLK_UAIF1,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_BCLK_UAIF2,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_BCLK_UAIF3,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_BCLK_UAIF4,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_CCLK_ASB,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_CCLK_ATB,
	GOUT_BLK_ABOX_UID_ABOX_TOP_IPCLKPORT_CCLK_CA7,
};
enum clk_id cmucal_vclk_axi_us_32to128[] = {
	GOUT_BLK_ABOX_UID_AXI_US_32to128_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_btm_abox[] = {
	GOUT_BLK_ABOX_UID_BTM_ABOX_IPCLKPORT_I_ACLK,
	GOUT_BLK_ABOX_UID_BTM_ABOX_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dftmux_abox[] = {
	CLK_BLK_ABOX_UID_DFTMUX_ABOX_IPCLKPORT_AUD_CODEC_MCLK,
};
enum clk_id cmucal_vclk_dmic[] = {
	CLK_BLK_ABOX_UID_DMIC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_gpio_abox[] = {
	GOUT_BLK_ABOX_UID_GPIO_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_abox[] = {
	GOUT_BLK_ABOX_UID_LHM_AXI_P_ABOX_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_abox[] = {
	GOUT_BLK_ABOX_UID_LHS_ATB_ABOX_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_abox[] = {
	GOUT_BLK_ABOX_UID_LHS_AXI_D_ABOX_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_peri_axi_asb[] = {
	GOUT_BLK_ABOX_UID_PERI_AXI_ASB_IPCLKPORT_ACLKM,
	GOUT_BLK_ABOX_UID_PERI_AXI_ASB_IPCLKPORT_ACLKS,
	GOUT_BLK_ABOX_UID_PERI_AXI_ASB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_abox[] = {
	GOUT_BLK_ABOX_UID_PMU_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_abox[] = {
	GOUT_BLK_ABOX_UID_BCM_ABOX_IPCLKPORT_ACLK,
	GOUT_BLK_ABOX_UID_BCM_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_smmu_abox[] = {
	GOUT_BLK_ABOX_UID_SMMU_ABOX_IPCLKPORT_ACLK,
	GOUT_BLK_ABOX_UID_SMMU_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_abox[] = {
	GOUT_BLK_ABOX_UID_SYSREG_ABOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_abox[] = {
	GOUT_BLK_ABOX_UID_TREX_ABOX_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_wdt_aboxcpu[] = {
	GOUT_BLK_ABOX_UID_WDT_ABOXCPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wrap2_conv_abox[] = {
	GOUT_BLK_ABOX_UID_WRAP2_CONV_ABOX_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_apm[] = {
	GOUT_BLK_APM_UID_APM_IPCLKPORT_ACLK_CPU,
	GOUT_BLK_APM_UID_APM_IPCLKPORT_ACLK_SYS,
	GOUT_BLK_APM_UID_APM_IPCLKPORT_ACLK_XIU_D_ALIVE,
};
enum clk_id cmucal_vclk_apm_cmu_apm[] = {
	CLK_BLK_APM_UID_APM_CMU_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_alive[] = {
	GOUT_BLK_APM_UID_LHM_AXI_P_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_alive[] = {
	GOUT_BLK_APM_UID_LHS_AXI_D_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mailbox_apm2ap[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_apm2cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_mailbox_apm2gnss[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM2GNSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_scan2axi[] = {
	GOUT_BLK_APM_UID_SCAN2AXI_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_sysreg_apm[] = {
	GOUT_BLK_APM_UID_SYSREG_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wdt_apm[] = {
	GOUT_BLK_APM_UID_WDT_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_bus1[] = {
	GOUT_BLK_BUS1_UID_AXI2APB_BUS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_bus1_trex[] = {
	GOUT_BLK_BUS1_UID_AXI2APB_BUS1_TREX_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_bus1_cmu_bus1[] = {
	GOUT_BLK_BUS1_UID_BUS1_CMU_BUS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_acel_d_fsys1[] = {
	GOUT_BLK_BUS1_UID_LHM_ACEL_D_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_alive[] = {
	GOUT_BLK_BUS1_UID_LHM_AXI_D_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_gnss[] = {
	GOUT_BLK_BUS1_UID_LHM_AXI_D_GNSS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_alive[] = {
	GOUT_BLK_BUS1_UID_LHS_AXI_P_ALIVE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_fsys1[] = {
	GOUT_BLK_BUS1_UID_LHS_AXI_P_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_bus1[] = {
	GOUT_BLK_BUS1_UID_PMU_BUS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_bus1[] = {
	GOUT_BLK_BUS1_UID_SYSREG_BUS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_d_bus1[] = {
	GOUT_BLK_BUS1_UID_TREX_D_BUS1_IPCLKPORT_ACLK,
	GOUT_BLK_BUS1_UID_TREX_D_BUS1_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_trex_p_bus1[] = {
	GOUT_BLK_BUS1_UID_TREX_P_BUS1_IPCLKPORT_PCLK_BUS1,
	GOUT_BLK_BUS1_UID_TREX_P_BUS1_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_adcif_busc[] = {
	GOUT_BLK_BUSC_UID_ADCIF_BUSC_IPCLKPORT_PCLK_S0,
	GOUT_BLK_BUSC_UID_ADCIF_BUSC_IPCLKPORT_PCLK_S1,
};
enum clk_id cmucal_vclk_ad_apb_hsi2cdf[] = {
	GOUT_BLK_BUSC_UID_AD_APB_HSI2CDF_IPCLKPORT_PCLKM,
	GOUT_BLK_BUSC_UID_AD_APB_HSI2CDF_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_pdma0[] = {
	GOUT_BLK_BUSC_UID_AD_APB_PDMA0_IPCLKPORT_PCLKM,
	GOUT_BLK_BUSC_UID_AD_APB_PDMA0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_spdma[] = {
	GOUT_BLK_BUSC_UID_AD_APB_SPDMA_IPCLKPORT_PCLKM,
	GOUT_BLK_BUSC_UID_AD_APB_SPDMA_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_asyncsfr_wr_sci[] = {
	GOUT_BLK_BUSC_UID_ASYNCSFR_WR_SCI_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_asyncsfr_wr_smc[] = {
	GOUT_BLK_BUSC_UID_ASYNCSFR_WR_SMC_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_buscp0[] = {
	GOUT_BLK_BUSC_UID_AXI2APB_BUSCP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_buscp1[] = {
	GOUT_BLK_BUSC_UID_AXI2APB_BUSCP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_busc_tdp[] = {
	GOUT_BLK_BUSC_UID_AXI2APB_BUSC_TDP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busc_cmu_busc[] = {
	CLK_BLK_BUSC_UID_BUSC_CMU_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_busif_cmutopc[] = {
	GOUT_BLK_BUSC_UID_BUSIF_CMUTOPC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gnssmbox[] = {
	GOUT_BLK_BUSC_UID_GNSSMBOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gpio_busc[] = {
	GOUT_BLK_BUSC_UID_GPIO_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_hsi2cdf[] = {
	GOUT_BLK_BUSC_UID_HSI2CDF_IPCLKPORT_iPCLK,
};
enum clk_id cmucal_vclk_lhm_acel_d0_g2d[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D0_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_acel_d1_g2d[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D1_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_acel_d2_g2d[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D2_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_acel_d_dsp[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_DSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_acel_d_fsys0[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_acel_d_iva[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_acel_d_vpu[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_VPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d0_cam[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D0_CAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d0_dpu[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D0_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d0_mfc[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D0_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d1_cam[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D1_CAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d1_dpu[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D1_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d1_mfc[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D1_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d2_dpu[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D2_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_abox[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_ABOX_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_isplp[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_srdz[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_SRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_vts[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_g_cssys[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_G_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_ivasc[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_D_IVASC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p0_dpu[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P0_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p1_dpu[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P1_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_abox[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_ABOX_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cam[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_CAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_dsp[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_DSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_fsys0[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_g2d[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_isphq[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_isplp[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_iva[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_mfc[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_mif0[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_mif1[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_mif2[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_mif3[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_peric0[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_PERIC0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_peric1[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_PERIC1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_peris[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_PERIS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_srdz[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_SRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_vpu[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_VPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_vts[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mbox[] = {
	GOUT_BLK_BUSC_UID_MBOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pdma0[] = {
	GOUT_BLK_BUSC_UID_PDMA0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_pmu_busc[] = {
	GOUT_BLK_BUSC_UID_PMU_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_secmbox[] = {
	GOUT_BLK_BUSC_UID_SECMBOX_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_spdma[] = {
	GOUT_BLK_BUSC_UID_SPDMA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_speedy[] = {
	GOUT_BLK_BUSC_UID_SPEEDY_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_speedy_batcher_wrap[] = {
	GOUT_BLK_BUSC_UID_SPEEDY_BATCHER_WRAP_IPCLKPORT_PCLK_BATCHER_SPEEDY,
	GOUT_BLK_BUSC_UID_SPEEDY_BATCHER_WRAP_IPCLKPORT_i_pclk_BATCHER_AP,
	GOUT_BLK_BUSC_UID_SPEEDY_BATCHER_WRAP_IPCLKPORT_i_pclk_BATCHER_CP,
};
enum clk_id cmucal_vclk_sysreg_busc[] = {
	GOUT_BLK_BUSC_UID_SYSREG_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_d_busc[] = {
	GOUT_BLK_BUSC_UID_TREX_D_BUSC_IPCLKPORT_ACLK,
	GOUT_BLK_BUSC_UID_TREX_D_BUSC_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_trex_p_busc[] = {
	GOUT_BLK_BUSC_UID_TREX_P_BUSC_IPCLKPORT_ACLK_BUSC,
	GOUT_BLK_BUSC_UID_TREX_P_BUSC_IPCLKPORT_PCLK_BUSC,
	GOUT_BLK_BUSC_UID_TREX_P_BUSC_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_btm_camd0[] = {
	GOUT_BLK_CAM_UID_BTM_CAMD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_CAM_UID_BTM_CAMD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_camd1[] = {
	GOUT_BLK_CAM_UID_BTM_CAMD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_CAM_UID_BTM_CAMD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_cam_cmu_cam[] = {
	CLK_BLK_CAM_UID_CAM_CMU_CAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_isp_ewgen_cam[] = {
	GOUT_BLK_CAM_UID_ISP_EWGEN_CAM_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_is_cam[] = {
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_BNS,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_CSIS0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_CSIS1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_CSIS2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_CSIS3,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_CSISX4_DMA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_MC_SCALER,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_APB_ASYNCM_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_AXI2APB_IS_CAM_0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_AXI2APB_IS_CAM_1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_BNS,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS0_DIV2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS1_DIV2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS2_DIV2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS3,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSIS3_DIV2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_CSISX4_DMA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_MC_SCALER,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_BCM_CAM0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_BCM_CAM1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_1to2_TPU0_MCSC,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_1to2_TPU1_MCSC,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_CAM_ISPHQ,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_CAM_ISPLP,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_ISP_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_ISP_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBM_MCSC_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_2to1_ISP_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_2to1_ISP_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_2to1_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_ISPHQ_CAM,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_ISPLP_CAM,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_TPU0_MCSC,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_PXL_ASBS_TPU1_MCSC,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_QE_CSISX4,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_QE_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_QE_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_QE_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_SYSMMU_CAM0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_SYSMMU_CAM1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_ASYNCM_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_ASYNCM_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_ASYNCM_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_ASYNCS_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_ASYNCS_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_ASYNCS_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_D_CAM0_4x1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_ACLK_XIU_P_CAM_1x2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_BNS,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_CSIS0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_CSIS1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_CSIS2,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_CSIS3,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_CSISX4_DMA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_MC_SCALER,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_ASYNCS_APB_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_CFW_CAM,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_BCM_CAM0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_BCM_CAM1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_QE_CSISX4,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_QE_TPU0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_QE_TPU1,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_QE_VRA,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_SYSMMU_CAM0,
	GOUT_BLK_CAM_UID_IS_CAM_IPCLKPORT_PCLK_SYSMMU_CAM1,
};
enum clk_id cmucal_vclk_lhm_atb_srdzcam[] = {
	GOUT_BLK_CAM_UID_LHM_ATB_SRDZCAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_cam[] = {
	GOUT_BLK_CAM_UID_LHM_AXI_P_CAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d0_cam[] = {
	GOUT_BLK_CAM_UID_LHS_AXI_D0_CAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d1_cam[] = {
	GOUT_BLK_CAM_UID_LHS_AXI_D1_CAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_cam[] = {
	GOUT_BLK_CAM_UID_PMU_CAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cam[] = {
	GOUT_BLK_CAM_UID_SYSREG_CAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_adm_apb_g_bdu[] = {
	GOUT_BLK_CORE_UID_ADM_APB_G_BDU_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_apbbr_cci[] = {
	GOUT_BLK_CORE_UID_APBBR_CCI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_core[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_core_tp[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_TP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_bdu[] = {
	GOUT_BLK_CORE_UID_BDU_IPCLKPORT_I_CLK,
	GOUT_BLK_CORE_UID_BDU_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_busif_hpmcore[] = {
	GOUT_BLK_CORE_UID_BUSIF_HPMCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_cci[] = {
	GOUT_BLK_CORE_UID_CCI_IPCLKPORT_PCLK,
	GOUT_BLK_CORE_UID_CCI_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_core_cmu_core[] = {
	CLK_BLK_CORE_UID_CORE_CMU_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_hpm_core[] = {
	CLK_BLK_CORE_UID_HPM_CORE_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_ace_d0_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D0_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_ace_d1_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D1_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_ace_d2_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D2_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_ace_d3_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D3_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_ace_d_cpucl1[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D_CPUCL1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_cp[] = {
	GOUT_BLK_CORE_UID_LHM_AXI_D_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_cp[] = {
	GOUT_BLK_CORE_UID_LHM_AXI_P_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_cpace_d_cpucl0[] = {
	GOUT_BLK_CORE_UID_LHM_CPACE_D_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_dbg_g0_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G0_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_dbg_g1_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G1_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_dbg_g2_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G2_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_dbg_g3_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G3_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_t_bdu[] = {
	GOUT_BLK_CORE_UID_LHS_ATB_T_BDU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cpucl0[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_cpucl1[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CPUCL1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_dbg[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_DBG_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_g3d[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_imem[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_IMEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_pscdc_d_mif0[] = {
	GOUT_BLK_CORE_UID_LHS_PSCDC_D_MIF0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_pscdc_d_mif1[] = {
	GOUT_BLK_CORE_UID_LHS_PSCDC_D_MIF1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_pscdc_d_mif2[] = {
	GOUT_BLK_CORE_UID_LHS_PSCDC_D_MIF2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_pscdc_d_mif3[] = {
	GOUT_BLK_CORE_UID_LHS_PSCDC_D_MIF3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mpace2axi_0[] = {
	GOUT_BLK_CORE_UID_MPACE2AXI_0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_mpace2axi_1[] = {
	GOUT_BLK_CORE_UID_MPACE2AXI_1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_pmu_core[] = {
	GOUT_BLK_CORE_UID_PMU_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ppcfw_g3d[] = {
	GOUT_BLK_CORE_UID_PPCFW_G3D_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPCFW_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_cci[] = {
	GOUT_BLK_CORE_UID_BCMPPC_CCI_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCMPPC_CCI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_cpucl0[] = {
	GOUT_BLK_CORE_UID_BCM_CPUCL0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCM_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_cpucl1[] = {
	GOUT_BLK_CORE_UID_BCM_CPUCL1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCM_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g3d0[] = {
	GOUT_BLK_CORE_UID_BCM_G3D0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCM_G3D0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g3d1[] = {
	GOUT_BLK_CORE_UID_BCM_G3D1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCM_G3D1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g3d2[] = {
	GOUT_BLK_CORE_UID_BCM_G3D2_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCM_G3D2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g3d3[] = {
	GOUT_BLK_CORE_UID_BCM_G3D3_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_BCM_G3D3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_core[] = {
	GOUT_BLK_CORE_UID_SYSREG_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_trex_d_core[] = {
	GOUT_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_trex_p0_core[] = {
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_ACLK_CORE,
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_PCLK_CORE,
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_trex_p1_core[] = {
	GOUT_BLK_CORE_UID_TREX_P1_CORE_IPCLKPORT_PCLK_CORE,
	GOUT_BLK_CORE_UID_TREX_P1_CORE_IPCLKPORT_pclk,
};
enum clk_id cmucal_vclk_axi2apb_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_AXI2APB_CPUCL0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_droopdetector_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_BUSIF_DROOPDETECTOR_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_busif_hpmcpucl0[] = {
	GOUT_BLK_CPUCL0_UID_BUSIF_HPMCPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_cpucl0_cmu_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_CPUCL0_CMU_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_droop_detector_cpucl0_grp0[] = {
	CLK_BLK_CPUCL0_UID_DROOP_DETECTOR_CPUCL0_GRP0_IPCLKPORT_CK_IN,
};
enum clk_id cmucal_vclk_droop_detector_cpucl0_grp1[] = {
	CLK_BLK_CPUCL0_UID_DROOP_DETECTOR_CPUCL0_GRP1_IPCLKPORT_CK_IN,
};
enum clk_id cmucal_vclk_hpm_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_HPM_CPUCL0_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_PMU_CPUCL0_IPCLKPORT_PCLK,
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
enum clk_id cmucal_vclk_cpucl1_cmu_cpucl1[] = {
	CLK_BLK_CPUCL1_UID_CPUCL1_CMU_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_hpm_cpucl1[] = {
	CLK_BLK_CPUCL1_UID_HPM_CPUCL1_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_cpucl1[] = {
	GOUT_BLK_CPUCL1_UID_LHM_AXI_P_CPUCL1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_cpucl1[] = {
	GOUT_BLK_CPUCL1_UID_PMU_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_cpucl1[] = {
	GOUT_BLK_CPUCL1_UID_SYSREG_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_coresight[] = {
	GOUT_BLK_DBG_UID_AXI2APB_CORESIGHT_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_cssys[] = {
	GOUT_BLK_DBG_UID_CSSYS_IPCLKPORT_ATCLK,
	GOUT_BLK_DBG_UID_CSSYS_IPCLKPORT_PCLKDBG,
};
enum clk_id cmucal_vclk_dbg_cmu_dbg[] = {
	CLK_BLK_DBG_UID_DBG_CMU_DBG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dumppc_cpucl0[] = {
	GOUT_BLK_DBG_UID_DUMPPC_CPUCL0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dumppc_cpucl1[] = {
	GOUT_BLK_DBG_UID_DUMPPC_CPUCL1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_lhm_atb_t0_cluster0[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t0_cluster1[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T0_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t1_cluster0[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t1_cluster1[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T1_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t2_cluster0[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T2_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t2_cluster1[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T2_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t3_cluster0[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T3_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t3_cluster1[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T3_CLUSTER1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t_aud[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_atb_t_bdu[] = {
	GOUT_BLK_DBG_UID_LHM_ATB_T_BDU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dbg[] = {
	GOUT_BLK_DBG_UID_LHM_AXI_P_DBG_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g_cssys[] = {
	GOUT_BLK_DBG_UID_LHS_AXI_G_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g_etr[] = {
	GOUT_BLK_DBG_UID_LHS_AXI_G_ETR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_dbg[] = {
	GOUT_BLK_DBG_UID_PMU_DBG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_secjtag[] = {
	GOUT_BLK_DBG_UID_SECJTAG_IPCLKPORT_i_clk,
};
enum clk_id cmucal_vclk_stm_txactor[] = {
	GOUT_BLK_DBG_UID_STM_TXACTOR_IPCLKPORT_I_ATCLK,
	GOUT_BLK_DBG_UID_STM_TXACTOR_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_sysreg_dbg[] = {
	GOUT_BLK_DBG_UID_SYSREG_DBG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ad_apb_dcam[] = {
	GOUT_BLK_DCAM_UID_AD_APB_DCAM_IPCLKPORT_PCLKM,
	GOUT_BLK_DCAM_UID_AD_APB_DCAM_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_dcam[] = {
	GOUT_BLK_DCAM_UID_AXI2APB_DCAM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_dcam_sys[] = {
	GOUT_BLK_DCAM_UID_AXI2APB_DCAM_SYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_dcam[] = {
	GOUT_BLK_DCAM_UID_BTM_DCAM_IPCLKPORT_I_ACLK,
	GOUT_BLK_DCAM_UID_BTM_DCAM_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dcam_cmu_dcam[] = {
	CLK_BLK_DCAM_UID_DCAM_CMU_DCAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dcp[] = {
	GOUT_BLK_DCAM_UID_DCP_IPCLKPORT_Clk,
};
enum clk_id cmucal_vclk_lhm_axi_p_srdzdcam[] = {
	GOUT_BLK_DCAM_UID_LHM_AXI_P_SRDZDCAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_dcamsrdz[] = {
	GOUT_BLK_DCAM_UID_LHS_ATB_DCAMSRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_dcamsrdz[] = {
	GOUT_BLK_DCAM_UID_LHS_AXI_D_DCAMSRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_dcam[] = {
	GOUT_BLK_DCAM_UID_PMU_DCAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_dcam[] = {
	GOUT_BLK_DCAM_UID_BCM_DCAM_IPCLKPORT_ACLK,
	GOUT_BLK_DCAM_UID_BCM_DCAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pxl_asbm_dcam[] = {
	GOUT_BLK_DCAM_UID_PXL_ASBM_DCAM_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_pxl_asbs_dcam[] = {
	GOUT_BLK_DCAM_UID_PXL_ASBS_DCAM_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_sysreg_dcam[] = {
	GOUT_BLK_DCAM_UID_SYSREG_DCAM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_dcam[] = {
	GOUT_BLK_DCAM_UID_XIU_P_DCAM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ad_apb_decon0[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DECON0_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DECON0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_decon0_secure[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DECON0_SECURE_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DECON0_SECURE_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpp[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DPP_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DPP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpp_secure[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DPP_SECURE_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DPP_SECURE_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpu_dma[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DPU_DMA_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DPU_DMA_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpu_dma_secure[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DPU_DMA_SECURE_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DPU_DMA_SECURE_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_dpu_wb_mux[] = {
	GOUT_BLK_DPU0_UID_AD_APB_DPU_WB_MUX_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU0_UID_AD_APB_DPU_WB_MUX_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_mipi_dsim0[] = {
	GOUT_BLK_DPU0_UID_AD_APB_MIPI_DSIM0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_mipi_dsim1[] = {
	GOUT_BLK_DPU0_UID_AD_APB_MIPI_DSIM1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_dpu0p0[] = {
	GOUT_BLK_DPU0_UID_AXI2APB_DPU0P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_dpu0p1[] = {
	GOUT_BLK_DPU0_UID_AXI2APB_DPU0P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_dpud0[] = {
	GOUT_BLK_DPU0_UID_BTM_DPUD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_DPU0_UID_BTM_DPUD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_dpud1[] = {
	GOUT_BLK_DPU0_UID_BTM_DPUD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_DPU0_UID_BTM_DPUD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_dpud2[] = {
	GOUT_BLK_DPU0_UID_BTM_DPUD2_IPCLKPORT_I_ACLK,
	GOUT_BLK_DPU0_UID_BTM_DPUD2_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_decon0[] = {
	GOUT_BLK_DPU0_UID_DECON0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_dpp[] = {
	GOUT_BLK_DPU0_UID_DPP_IPCLKPORT_DPP_G0_ACLK,
	GOUT_BLK_DPU0_UID_DPP_IPCLKPORT_DPP_G1_ACLK,
	GOUT_BLK_DPU0_UID_DPP_IPCLKPORT_DPP_VGR_ACLK,
};
enum clk_id cmucal_vclk_dpu0_cmu_dpu0[] = {
	CLK_BLK_DPU0_UID_DPU0_CMU_DPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dpu_dma[] = {
	GOUT_BLK_DPU0_UID_DPU_DMA_IPCLKPORT_ACLK_CH0,
	GOUT_BLK_DPU0_UID_DPU_DMA_IPCLKPORT_ACLK_CH1,
	GOUT_BLK_DPU0_UID_DPU_DMA_IPCLKPORT_ACLK_CH2,
	GOUT_BLK_DPU0_UID_DPU_DMA_IPCLKPORT_ACLK_CH_INTF,
	GOUT_BLK_DPU0_UID_DPU_DMA_IPCLKPORT_ACLK_SFR,
	GOUT_BLK_DPU0_UID_DPU_DMA_IPCLKPORT_ACLK_WB,
};
enum clk_id cmucal_vclk_dpu_wb_mux[] = {
	GOUT_BLK_DPU0_UID_DPU_WB_MUX_IPCLKPORT_DPP_G1_ACLK,
};
enum clk_id cmucal_vclk_lhm_axi_p0_dpu[] = {
	GOUT_BLK_DPU0_UID_LHM_AXI_P0_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d0_dpu[] = {
	GOUT_BLK_DPU0_UID_LHS_AXI_D0_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d1_dpu[] = {
	GOUT_BLK_DPU0_UID_LHS_AXI_D1_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d2_dpu[] = {
	GOUT_BLK_DPU0_UID_LHS_AXI_D2_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_usbtv[] = {
	GOUT_BLK_DPU0_UID_LHS_AXI_D_USBTV_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_dpu0[] = {
	GOUT_BLK_DPU0_UID_PMU_DPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_dpud0[] = {
	GOUT_BLK_DPU0_UID_BCM_DPUD0_IPCLKPORT_ACLK,
	GOUT_BLK_DPU0_UID_BCM_DPUD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_dpud1[] = {
	GOUT_BLK_DPU0_UID_BCM_DPUD1_IPCLKPORT_ACLK,
	GOUT_BLK_DPU0_UID_BCM_DPUD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_dpud2[] = {
	GOUT_BLK_DPU0_UID_BCM_DPUD2_IPCLKPORT_ACLK,
	GOUT_BLK_DPU0_UID_BCM_DPUD2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysmmu_dpud0[] = {
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD0_IPCLKPORT_ACLK,
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD0_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD0_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysmmu_dpud1[] = {
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD1_IPCLKPORT_ACLK,
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD1_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD1_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysmmu_dpud2[] = {
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD2_IPCLKPORT_ACLK,
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD2_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_DPU0_UID_SYSMMU_DPUD2_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysreg_dpu0[] = {
	GOUT_BLK_DPU0_UID_SYSREG_DPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p0_dpu[] = {
	GOUT_BLK_DPU0_UID_XIU_P0_DPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ad_apb_decon1[] = {
	GOUT_BLK_DPU1_UID_AD_APB_DECON1_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU1_UID_AD_APB_DECON1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ad_apb_decon2[] = {
	GOUT_BLK_DPU1_UID_AD_APB_DECON2_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU1_UID_AD_APB_DECON2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_dpu1p0[] = {
	GOUT_BLK_DPU1_UID_AXI2APB_DPU1P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_decon1[] = {
	GOUT_BLK_DPU1_UID_DECON1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_decon2[] = {
	GOUT_BLK_DPU1_UID_DECON2_IPCLKPORT_ACLK,
	GOUT_BLK_DPU1_UID_DECON2_IPCLKPORT_VCLK,
};
enum clk_id cmucal_vclk_dpu1_cmu_dpu1[] = {
	CLK_BLK_DPU1_UID_DPU1_CMU_DPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dpu1[] = {
	GOUT_BLK_DPU1_UID_LHM_AXI_P_DPU1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_dpu1[] = {
	GOUT_BLK_DPU1_UID_PMU_DPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_dpu1[] = {
	GOUT_BLK_DPU1_UID_SYSREG_DPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ad_apb_score[] = {
	GOUT_BLK_DSP_UID_AD_APB_SCORE_IPCLKPORT_PCLKM,
	GOUT_BLK_DSP_UID_AD_APB_SCORE_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_dsp[] = {
	GOUT_BLK_DSP_UID_AXI2APB_DSP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_score[] = {
	GOUT_BLK_DSP_UID_BTM_SCORE_IPCLKPORT_I_ACLK,
	GOUT_BLK_DSP_UID_BTM_SCORE_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dsp_cmu_dsp[] = {
	CLK_BLK_DSP_UID_DSP_CMU_DSP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_ivadsp[] = {
	GOUT_BLK_DSP_UID_LHM_AXI_D_IVADSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_vpudsp[] = {
	GOUT_BLK_DSP_UID_LHM_AXI_D_VPUDSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dsp[] = {
	GOUT_BLK_DSP_UID_LHM_AXI_P_DSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_ivadsp[] = {
	GOUT_BLK_DSP_UID_LHM_AXI_P_IVADSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d_dsp[] = {
	GOUT_BLK_DSP_UID_LHS_ACEL_D_DSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_dspiva[] = {
	GOUT_BLK_DSP_UID_LHS_AXI_P_DSPIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_dspvpu[] = {
	GOUT_BLK_DSP_UID_LHS_AXI_P_DSPVPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_dsp[] = {
	GOUT_BLK_DSP_UID_PMU_DSP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_score[] = {
	GOUT_BLK_DSP_UID_BCM_SCORE_IPCLKPORT_ACLK,
	GOUT_BLK_DSP_UID_BCM_SCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_score[] = {
	GOUT_BLK_DSP_UID_SCORE_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_smmu_score[] = {
	GOUT_BLK_DSP_UID_SMMU_SCORE_IPCLKPORT_ACLK,
	GOUT_BLK_DSP_UID_SMMU_SCORE_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_DSP_UID_SMMU_SCORE_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysreg_dsp[] = {
	GOUT_BLK_DSP_UID_SYSREG_DSP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wrap2_conv_dsp[] = {
	GOUT_BLK_DSP_UID_WRAP2_CONV_DSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_xiu_d_dsp0[] = {
	GOUT_BLK_DSP_UID_XIU_D_DSP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_d_dsp1[] = {
	GOUT_BLK_DSP_UID_XIU_D_DSP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_dsp[] = {
	GOUT_BLK_DSP_UID_XIU_P_DSP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ahbbr_fsys0[] = {
	GOUT_BLK_FSYS0_UID_AHBBR_FSYS0_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_axi2ahb_fsys0[] = {
	GOUT_BLK_FSYS0_UID_AXI2AHB_FSYS0_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_axi2ahb_usb_fsys0[] = {
	GOUT_BLK_FSYS0_UID_AXI2AHB_USB_FSYS0_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_axi2apb_fsys0[] = {
	GOUT_BLK_FSYS0_UID_AXI2APB_FSYS0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_fsys0[] = {
	GOUT_BLK_FSYS0_UID_BTM_FSYS0_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS0_UID_BTM_FSYS0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_dp_link[] = {
	GOUT_BLK_FSYS0_UID_DP_LINK_IPCLKPORT_I_GTC_EXT_CLK,
	GOUT_BLK_FSYS0_UID_DP_LINK_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_etr_miu[] = {
	GOUT_BLK_FSYS0_UID_ETR_MIU_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS0_UID_ETR_MIU_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_fsys0_cmu_fsys0[] = {
	CLK_BLK_FSYS0_UID_FSYS0_CMU_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gpio_fsys0[] = {
	GOUT_BLK_FSYS0_UID_GPIO_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_usbtv[] = {
	GOUT_BLK_FSYS0_UID_LHM_AXI_D_USBTV_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_g_etr[] = {
	GOUT_BLK_FSYS0_UID_LHM_AXI_G_ETR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_fsys0[] = {
	GOUT_BLK_FSYS0_UID_LHM_AXI_P_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d_fsys0[] = {
	GOUT_BLK_FSYS0_UID_LHS_ACEL_D_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mmc_embd[] = {
	GOUT_BLK_FSYS0_UID_MMC_EMBD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS0_UID_MMC_EMBD_IPCLKPORT_SDCLKIN,
};
enum clk_id cmucal_vclk_pmu_fsys0[] = {
	GOUT_BLK_FSYS0_UID_PMU_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_fsys0[] = {
	GOUT_BLK_FSYS0_UID_BCM_FSYS0_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS0_UID_BCM_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_fsys0[] = {
	GOUT_BLK_FSYS0_UID_SYSREG_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ufs_embd[] = {
	GOUT_BLK_FSYS0_UID_UFS_EMBD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS0_UID_UFS_EMBD_IPCLKPORT_I_CLK_UNIPRO,
	GOUT_BLK_FSYS0_UID_UFS_EMBD_IPCLKPORT_I_FMP_CLK,
};
enum clk_id cmucal_vclk_usbtv[] = {
	GOUT_BLK_FSYS0_UID_USBTV_IPCLKPORT_I_USB30DRD_ACLK,
	GOUT_BLK_FSYS0_UID_USBTV_IPCLKPORT_I_USB30DRD_REF_CLK,
	GOUT_BLK_FSYS0_UID_USBTV_IPCLKPORT_I_USB30DRD_SUSPEND_CLK,
	GOUT_BLK_FSYS0_UID_USBTV_IPCLKPORT_I_USBTVH_AHB_CLK,
	GOUT_BLK_FSYS0_UID_USBTV_IPCLKPORT_I_USBTVH_CORE_CLK,
	GOUT_BLK_FSYS0_UID_USBTV_IPCLKPORT_I_USBTVH_XIU_CLK,
};
enum clk_id cmucal_vclk_us_d_fsys0_usb[] = {
	GOUT_BLK_FSYS0_UID_US_D_FSYS0_USB_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_xiu_d_fsys0[] = {
	GOUT_BLK_FSYS0_UID_XIU_D_FSYS0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_d_fsys0_usb[] = {
	GOUT_BLK_FSYS0_UID_XIU_D_FSYS0_USB_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_fsys0[] = {
	GOUT_BLK_FSYS0_UID_XIU_P_FSYS0_IPCLKPORT_ACLK,
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
enum clk_id cmucal_vclk_pcie[] = {
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_dbi_aclk_0,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_dbi_aclk_1,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_ieee1500_wrapper_for_pcie_phy_lc_x2_inst_0_i_scl_apb_pclk,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_mstr_aclk_0,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_mstr_aclk_1,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_pcie_sub_ctrl_inst_0_i_driver_apb_clk,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_pcie_sub_ctrl_inst_1_i_driver_apb_clk,
	CLK_BLK_FSYS1_UID_PCIE_IPCLKPORT_phy_ref_clk_in,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_pipe2_digital_x2_wrap_inst_0_i_apb_pclk_scl,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_slv_aclk_0,
	GOUT_BLK_FSYS1_UID_PCIE_IPCLKPORT_slv_aclk_1,
};
enum clk_id cmucal_vclk_pmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_PMU_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_fsys1[] = {
	GOUT_BLK_FSYS1_UID_BCM_FSYS1_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS1_UID_BCM_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_rtic[] = {
	GOUT_BLK_FSYS1_UID_RTIC_IPCLKPORT_i_ACLK,
	GOUT_BLK_FSYS1_UID_RTIC_IPCLKPORT_i_PCLK,
};
enum clk_id cmucal_vclk_sss[] = {
	GOUT_BLK_FSYS1_UID_SSS_IPCLKPORT_i_ACLK,
	GOUT_BLK_FSYS1_UID_SSS_IPCLKPORT_i_PCLK,
};
enum clk_id cmucal_vclk_sysreg_fsys1[] = {
	GOUT_BLK_FSYS1_UID_SYSREG_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_toe_wifi0[] = {
	GOUT_BLK_FSYS1_UID_TOE_WIFI0_IPCLKPORT_i_CLK,
};
enum clk_id cmucal_vclk_toe_wifi1[] = {
	GOUT_BLK_FSYS1_UID_TOE_WIFI1_IPCLKPORT_i_CLK,
};
enum clk_id cmucal_vclk_ufs_card[] = {
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_CLK_UNIPRO,
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_FMP_CLK,
};
enum clk_id cmucal_vclk_xiu_d_fsys1[] = {
	GOUT_BLK_FSYS1_UID_XIU_D_FSYS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_fsys1[] = {
	GOUT_BLK_FSYS1_UID_XIU_P_FSYS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_as_p_g2dp[] = {
	GOUT_BLK_G2D_UID_AS_P_G2DP_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_G2DP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_as_p_jpegp[] = {
	GOUT_BLK_G2D_UID_AS_P_JPEGP_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_JPEGP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_as_p_m2mscalerp[] = {
	GOUT_BLK_G2D_UID_AS_P_M2MSCALERP_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_M2MSCALERP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_g2dp0[] = {
	GOUT_BLK_G2D_UID_AXI2APB_G2DP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_g2dp1[] = {
	GOUT_BLK_G2D_UID_AXI2APB_G2DP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_g2dd0[] = {
	GOUT_BLK_G2D_UID_BTM_G2DD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_G2D_UID_BTM_G2DD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_g2dd1[] = {
	GOUT_BLK_G2D_UID_BTM_G2DD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_G2D_UID_BTM_G2DD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_g2dd2[] = {
	GOUT_BLK_G2D_UID_BTM_G2DD2_IPCLKPORT_I_ACLK,
	GOUT_BLK_G2D_UID_BTM_G2DD2_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_g2d[] = {
	GOUT_BLK_G2D_UID_G2D_IPCLKPORT_Clk,
};
enum clk_id cmucal_vclk_g2d_cmu_g2d[] = {
	CLK_BLK_G2D_UID_G2D_CMU_G2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_jpeg[] = {
	GOUT_BLK_G2D_UID_JPEG_IPCLKPORT_I_SMFC_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_g2d[] = {
	GOUT_BLK_G2D_UID_LHM_AXI_P_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d0_g2d[] = {
	GOUT_BLK_G2D_UID_LHS_ACEL_D0_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d1_g2d[] = {
	GOUT_BLK_G2D_UID_LHS_ACEL_D1_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d2_g2d[] = {
	GOUT_BLK_G2D_UID_LHS_ACEL_D2_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_m2mscaler[] = {
	GOUT_BLK_G2D_UID_M2MSCALER_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_pmu_g2d[] = {
	GOUT_BLK_G2D_UID_PMU_G2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g2dd0[] = {
	GOUT_BLK_G2D_UID_BCM_G2DD0_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_BCM_G2DD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g2dd1[] = {
	GOUT_BLK_G2D_UID_BCM_G2DD1_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_BCM_G2DD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_g2dd2[] = {
	GOUT_BLK_G2D_UID_BCM_G2DD2_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_BCM_G2DD2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_qe_jpeg[] = {
	GOUT_BLK_G2D_UID_QE_JPEG_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_QE_JPEG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_qe_m2mscaler[] = {
	GOUT_BLK_G2D_UID_QE_M2MSCALER_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_QE_M2MSCALER_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_smmu_g2dd0[] = {
	GOUT_BLK_G2D_UID_SMMU_G2DD0_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_SMMU_G2DD0_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_G2D_UID_SMMU_G2DD0_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_smmu_g2dd1[] = {
	GOUT_BLK_G2D_UID_SMMU_G2DD1_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_SMMU_G2DD1_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_G2D_UID_SMMU_G2DD1_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_smmu_g2dd2[] = {
	GOUT_BLK_G2D_UID_SMMU_G2DD2_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_SMMU_G2DD2_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_G2D_UID_SMMU_G2DD2_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysreg_g2d[] = {
	GOUT_BLK_G2D_UID_SYSREG_G2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_d_g2d[] = {
	GOUT_BLK_G2D_UID_XIU_D_G2D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_g2d[] = {
	GOUT_BLK_G2D_UID_XIU_P_G2D_IPCLKPORT_ACLK,
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
enum clk_id cmucal_vclk_hpm_g3d[] = {
	CLK_BLK_G3D_UID_HPM_G3D_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_g3d[] = {
	GOUT_BLK_G3D_UID_LHM_AXI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_g3dsfr[] = {
	GOUT_BLK_G3D_UID_LHS_AXI_G3DSFR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_g3d[] = {
	GOUT_BLK_G3D_UID_PMU_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_g3d[] = {
	GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_g3d[] = {
	GOUT_BLK_G3D_UID_XIU_P_G3D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_imem[] = {
	GOUT_BLK_IMEM_UID_AXI2APB_IMEM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_imem_cmu_imem[] = {
	CLK_BLK_IMEM_UID_IMEM_CMU_IMEM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_intmem[] = {
	GOUT_BLK_IMEM_UID_IntMEM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_imem[] = {
	GOUT_BLK_IMEM_UID_LHM_AXI_P_IMEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_imem[] = {
	GOUT_BLK_IMEM_UID_PMU_IMEM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_imem[] = {
	GOUT_BLK_IMEM_UID_SYSREG_IMEM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_p_imem[] = {
	GOUT_BLK_IMEM_UID_XIU_P_IMEM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_isphq_cmu_isphq[] = {
	CLK_BLK_ISPHQ_UID_ISPHQ_CMU_ISPHQ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_isp_ewgen_isphq[] = {
	GOUT_BLK_ISPHQ_UID_ISP_EWGEN_ISPHQ_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_is_isphq[] = {
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_3AA_PCLKM,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_3AA_PCLKS,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_ISPHQ_PCLKM,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_ISPHQ_PCLKS,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_AXI2APB_BRIDGE_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_QE_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_XIU_D_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_QE_3AA_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_QE_3AA_PCLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_QE_ISPHQ_PCLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_TAA_ACLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_isphq[] = {
	GOUT_BLK_ISPHQ_UID_LHM_AXI_P_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_ld_isphq[] = {
	GOUT_BLK_ISPHQ_UID_LHS_AXI_LD_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_isphq[] = {
	GOUT_BLK_ISPHQ_UID_PMU_ISPHQ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_isphq[] = {
	GOUT_BLK_ISPHQ_UID_SYSREG_ISPHQ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_btm_isplp[] = {
	GOUT_BLK_ISPLP_UID_BTM_ISPLP_IPCLKPORT_I_ACLK,
	GOUT_BLK_ISPLP_UID_BTM_ISPLP_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_isplp_cmu_isplp[] = {
	CLK_BLK_ISPLP_UID_ISPLP_CMU_ISPLP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_isp_ewgen_isplp[] = {
	GOUT_BLK_ISPLP_UID_ISP_EWGEN_ISPLP_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_is_isplp[] = {
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_3AAW_PCLKM,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_3AAW_PCLKS,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_ISPLP_PCLKM,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_ISPLP_PCLKS,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_AXI2APB_BRIDGE_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_XIU_D_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_SMMU_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_BCM_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_BCM_ISPLP_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_3AAW_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_3AAW_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_ISPLP_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_SMMU_ISPLP_PCLK_NONSECURE,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_SMMU_ISPLP_PCLK_SECURE,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_TAAW_ACLK,
};
enum clk_id cmucal_vclk_lhm_axi_ld_isphq[] = {
	GOUT_BLK_ISPLP_UID_LHM_AXI_LD_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_isplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_AXI_P_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_isplp[] = {
	GOUT_BLK_ISPLP_UID_LHS_AXI_D_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_isplp[] = {
	GOUT_BLK_ISPLP_UID_PMU_ISPLP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_isplp[] = {
	GOUT_BLK_ISPLP_UID_SYSREG_ISPLP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ad_apb_iva[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA_IPCLKPORT_PCLKM,
	GOUT_BLK_IVA_UID_AD_APB_IVA_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_2m_iva[] = {
	GOUT_BLK_IVA_UID_AXI2APB_2M_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_iva[] = {
	GOUT_BLK_IVA_UID_AXI2APB_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_iva[] = {
	GOUT_BLK_IVA_UID_BTM_IVA_IPCLKPORT_I_ACLK,
	GOUT_BLK_IVA_UID_BTM_IVA_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_iva[] = {
	GOUT_BLK_IVA_UID_IVA_IPCLKPORT_clk_a,
};
enum clk_id cmucal_vclk_iva_cmu_iva[] = {
	CLK_BLK_IVA_UID_IVA_CMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_iva_intmem[] = {
	GOUT_BLK_IVA_UID_IVA_IntMEM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_ivasc[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_D_IVASC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dspiva[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_P_DSPIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_iva[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_P_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d_iva[] = {
	GOUT_BLK_IVA_UID_LHS_ACEL_D_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_ivadsp[] = {
	GOUT_BLK_IVA_UID_LHS_AXI_D_IVADSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_ivadsp[] = {
	GOUT_BLK_IVA_UID_LHS_AXI_P_IVADSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_iva[] = {
	GOUT_BLK_IVA_UID_PMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_iva[] = {
	GOUT_BLK_IVA_UID_BCM_IVA_IPCLKPORT_ACLK,
	GOUT_BLK_IVA_UID_BCM_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_smmu_iva[] = {
	GOUT_BLK_IVA_UID_SMMU_IVA_IPCLKPORT_ACLK,
	GOUT_BLK_IVA_UID_SMMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_iva[] = {
	GOUT_BLK_IVA_UID_SYSREG_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_d_iva[] = {
	GOUT_BLK_IVA_UID_XIU_D_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_iva[] = {
	GOUT_BLK_IVA_UID_XIU_P_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_as_p_mfcp[] = {
	GOUT_BLK_MFC_UID_AS_P_MFCP_IPCLKPORT_PCLKM,
	GOUT_BLK_MFC_UID_AS_P_MFCP_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_mfc[] = {
	GOUT_BLK_MFC_UID_AXI2APB_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_mfcd0[] = {
	GOUT_BLK_MFC_UID_BTM_MFCD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_MFC_UID_BTM_MFCD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_btm_mfcd1[] = {
	GOUT_BLK_MFC_UID_BTM_MFCD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_MFC_UID_BTM_MFCD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_mfc[] = {
	GOUT_BLK_MFC_UID_LHM_AXI_P_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d0_mfc[] = {
	GOUT_BLK_MFC_UID_LHS_AXI_D0_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d1_mfc[] = {
	GOUT_BLK_MFC_UID_LHS_AXI_D1_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mfc[] = {
	GOUT_BLK_MFC_UID_MFC_IPCLKPORT_Clk,
};
enum clk_id cmucal_vclk_mfc_cmu_mfc[] = {
	CLK_BLK_MFC_UID_MFC_CMU_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_mfc[] = {
	GOUT_BLK_MFC_UID_PMU_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_mfcd0[] = {
	GOUT_BLK_MFC_UID_BCM_MFCD0_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_BCM_MFCD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_mfcd1[] = {
	GOUT_BLK_MFC_UID_BCM_MFCD1_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_BCM_MFCD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_smmu_mfcd0[] = {
	GOUT_BLK_MFC_UID_SMMU_MFCD0_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_SMMU_MFCD0_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_MFC_UID_SMMU_MFCD0_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_smmu_mfcd1[] = {
	GOUT_BLK_MFC_UID_SMMU_MFCD1_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_SMMU_MFCD1_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_MFC_UID_SMMU_MFCD1_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysreg_mfc[] = {
	GOUT_BLK_MFC_UID_SYSREG_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_ddrphy[] = {
	GOUT_BLK_MIF_UID_APBBR_DDRPHY_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmc[] = {
	GOUT_BLK_MIF_UID_APBBR_DMC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmctz[] = {
	GOUT_BLK_MIF_UID_APBBR_DMCTZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_mif[] = {
	GOUT_BLK_MIF_UID_AXI2APB_MIF_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_hpmmif[] = {
	GOUT_BLK_MIF_UID_BUSIF_HPMMIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ddrphy[] = {
	GOUT_BLK_MIF_UID_DDRPHY_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dmc[] = {
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK2,
	CLK_BLK_MIF_UID_DMC_IPCLKPORT_soc_clk,
};
enum clk_id cmucal_vclk_hpm_mif[] = {
	CLK_BLK_MIF_UID_HPM_MIF_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_mif[] = {
	GOUT_BLK_MIF_UID_LHM_AXI_P_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_pscdc_d_mif[] = {
	CLK_BLK_MIF_UID_LHM_PSCDC_D_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mif_cmu_mif[] = {
	CLK_BLK_MIF_UID_MIF_CMU_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_mif[] = {
	GOUT_BLK_MIF_UID_PMU_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_debug[] = {
	CLK_BLK_MIF_UID_BCMPPC_DEBUG_IPCLKPORT_ACLK,
	GOUT_BLK_MIF_UID_BCMPPC_DEBUG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_dvfs[] = {
	GOUT_BLK_MIF_UID_BCMPPC_DVFS_IPCLKPORT_PCLK,
	CLK_BLK_MIF_UID_BCMPPC_DVFS_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_sysreg_mif[] = {
	GOUT_BLK_MIF_UID_SYSREG_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_ddrphy1[] = {
	GOUT_BLK_MIF1_UID_APBBR_DDRPHY1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmc1[] = {
	GOUT_BLK_MIF1_UID_APBBR_DMC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmctz1[] = {
	GOUT_BLK_MIF1_UID_APBBR_DMCTZ1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_mif1[] = {
	GOUT_BLK_MIF1_UID_AXI2APB_MIF1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_hpmmif1[] = {
	GOUT_BLK_MIF1_UID_BUSIF_HPMMIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ddrphy1[] = {
	GOUT_BLK_MIF1_UID_DDRPHY1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dmc1[] = {
	GOUT_BLK_MIF1_UID_DMC1_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF1_UID_DMC1_IPCLKPORT_PCLK2,
	CLK_BLK_MIF1_UID_DMC1_IPCLKPORT_soc_clk,
};
enum clk_id cmucal_vclk_hpm_mif1[] = {
	CLK_BLK_MIF1_UID_HPM_MIF1_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_mif1[] = {
	GOUT_BLK_MIF1_UID_LHM_AXI_P_MIF1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_pscdc_d_mif1[] = {
	CLK_BLK_MIF1_UID_LHM_PSCDC_D_MIF1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mif1_cmu_mif1[] = {
	CLK_BLK_MIF1_UID_MIF1_CMU_MIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_mif1[] = {
	GOUT_BLK_MIF1_UID_PMU_MIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_debug1[] = {
	CLK_BLK_MIF1_UID_BCMPPC_DEBUG1_IPCLKPORT_ACLK,
	GOUT_BLK_MIF1_UID_BCMPPC_DEBUG1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_dvfs1[] = {
	GOUT_BLK_MIF1_UID_BCMPPC_DVFS1_IPCLKPORT_PCLK,
	CLK_BLK_MIF1_UID_BCMPPC_DVFS1_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_sysreg_mif1[] = {
	GOUT_BLK_MIF1_UID_SYSREG_MIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_ddrphy2[] = {
	GOUT_BLK_MIF2_UID_APBBR_DDRPHY2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmc2[] = {
	GOUT_BLK_MIF2_UID_APBBR_DMC2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmctz2[] = {
	GOUT_BLK_MIF2_UID_APBBR_DMCTZ2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_mif2[] = {
	GOUT_BLK_MIF2_UID_AXI2APB_MIF2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_hpmmif2[] = {
	GOUT_BLK_MIF2_UID_BUSIF_HPMMIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ddrphy2[] = {
	GOUT_BLK_MIF2_UID_DDRPHY2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dmc2[] = {
	GOUT_BLK_MIF2_UID_DMC2_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF2_UID_DMC2_IPCLKPORT_PCLK2,
	CLK_BLK_MIF2_UID_DMC2_IPCLKPORT_soc_clk,
};
enum clk_id cmucal_vclk_hpm_mif2[] = {
	CLK_BLK_MIF2_UID_HPM_MIF2_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_mif2[] = {
	GOUT_BLK_MIF2_UID_LHM_AXI_P_MIF2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_pscdc_d_mif2[] = {
	CLK_BLK_MIF2_UID_LHM_PSCDC_D_MIF2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mif2_cmu_mif2[] = {
	CLK_BLK_MIF2_UID_MIF2_CMU_MIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_mif2[] = {
	GOUT_BLK_MIF2_UID_PMU_MIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_debug2[] = {
	CLK_BLK_MIF2_UID_BCMPPC_DEBUG2_IPCLKPORT_ACLK,
	GOUT_BLK_MIF2_UID_BCMPPC_DEBUG2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_dvfs2[] = {
	GOUT_BLK_MIF2_UID_BCMPPC_DVFS2_IPCLKPORT_PCLK,
	CLK_BLK_MIF2_UID_BCMPPC_DVFS2_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_sysreg_mif2[] = {
	GOUT_BLK_MIF2_UID_SYSREG_MIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_ddrphy3[] = {
	GOUT_BLK_MIF3_UID_APBBR_DDRPHY3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmc3[] = {
	GOUT_BLK_MIF3_UID_APBBR_DMC3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_apbbr_dmctz3[] = {
	GOUT_BLK_MIF3_UID_APBBR_DMCTZ3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_mif3[] = {
	GOUT_BLK_MIF3_UID_AXI2APB_MIF3_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_busif_hpmmif3[] = {
	GOUT_BLK_MIF3_UID_BUSIF_HPMMIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ddrphy3[] = {
	GOUT_BLK_MIF3_UID_DDRPHY3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dmc3[] = {
	GOUT_BLK_MIF3_UID_DMC3_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF3_UID_DMC3_IPCLKPORT_PCLK2,
	CLK_BLK_MIF3_UID_DMC3_IPCLKPORT_soc_clk,
};
enum clk_id cmucal_vclk_hpm_mif3[] = {
	CLK_BLK_MIF3_UID_HPM_MIF3_IPCLKPORT_hpm_targetclk_c,
};
enum clk_id cmucal_vclk_lhm_axi_p_mif3[] = {
	GOUT_BLK_MIF3_UID_LHM_AXI_P_MIF3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_pscdc_d_mif3[] = {
	CLK_BLK_MIF3_UID_LHM_PSCDC_D_MIF3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mif3_cmu_mif3[] = {
	CLK_BLK_MIF3_UID_MIF3_CMU_MIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_mif3[] = {
	GOUT_BLK_MIF3_UID_PMU_MIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_debug3[] = {
	CLK_BLK_MIF3_UID_BCMPPC_DEBUG3_IPCLKPORT_ACLK,
	GOUT_BLK_MIF3_UID_BCMPPC_DEBUG3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcmppc_dvfs3[] = {
	GOUT_BLK_MIF3_UID_BCMPPC_DVFS3_IPCLKPORT_PCLK,
	CLK_BLK_MIF3_UID_BCMPPC_DVFS3_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_sysreg_mif3[] = {
	GOUT_BLK_MIF3_UID_SYSREG_MIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_axi2apb_peric0[] = {
	GOUT_BLK_PERIC0_UID_AXI2APB_PERIC0_IPCLKPORT_ACLK,
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
enum clk_id cmucal_vclk_pmu_peric0[] = {
	GOUT_BLK_PERIC0_UID_PMU_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pwm[] = {
	GOUT_BLK_PERIC0_UID_PWM_IPCLKPORT_i_PCLK_S0,
};
enum clk_id cmucal_vclk_speedy2_tsp[] = {
	GOUT_BLK_PERIC0_UID_SPEEDY2_TSP_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_sysreg_peric0[] = {
	GOUT_BLK_PERIC0_UID_SYSREG_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_uart_dbg[] = {
	GOUT_BLK_PERIC0_UID_UART_DBG_IPCLKPORT_EXT_UCLK,
	GOUT_BLK_PERIC0_UID_UART_DBG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi00[] = {
	GOUT_BLK_PERIC0_UID_USI00_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC0_UID_USI00_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi01[] = {
	GOUT_BLK_PERIC0_UID_USI01_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC0_UID_USI01_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi02[] = {
	GOUT_BLK_PERIC0_UID_USI02_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC0_UID_USI02_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi03[] = {
	GOUT_BLK_PERIC0_UID_USI03_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC0_UID_USI03_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_axi2apb_peric1p0[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_peric1p1[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_peric1p2[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_gpio_peric1[] = {
	GOUT_BLK_PERIC1_UID_GPIO_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_hsi2c_cam0[] = {
	GOUT_BLK_PERIC1_UID_HSI2C_CAM0_IPCLKPORT_iPCLK,
};
enum clk_id cmucal_vclk_hsi2c_cam1[] = {
	GOUT_BLK_PERIC1_UID_HSI2C_CAM1_IPCLKPORT_iPCLK,
};
enum clk_id cmucal_vclk_hsi2c_cam2[] = {
	GOUT_BLK_PERIC1_UID_HSI2C_CAM2_IPCLKPORT_iPCLK,
};
enum clk_id cmucal_vclk_hsi2c_cam3[] = {
	GOUT_BLK_PERIC1_UID_HSI2C_CAM3_IPCLKPORT_iPCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_peric1[] = {
	GOUT_BLK_PERIC1_UID_LHM_AXI_P_PERIC1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_peric1_cmu_peric1[] = {
	CLK_BLK_PERIC1_UID_PERIC1_CMU_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pmu_peric1[] = {
	GOUT_BLK_PERIC1_UID_PMU_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_speedy2_ddi[] = {
	GOUT_BLK_PERIC1_UID_SPEEDY2_DDI_IPCLKPORT_clk,
	GOUT_BLK_PERIC1_UID_SPEEDY2_DDI_IPCLKPORT_sclk,
};
enum clk_id cmucal_vclk_speedy2_ddi1[] = {
	GOUT_BLK_PERIC1_UID_SPEEDY2_DDI1_IPCLKPORT_clk,
	GOUT_BLK_PERIC1_UID_SPEEDY2_DDI1_IPCLKPORT_sclk,
};
enum clk_id cmucal_vclk_speedy2_ddi2[] = {
	GOUT_BLK_PERIC1_UID_SPEEDY2_DDI2_IPCLKPORT_clk,
	GOUT_BLK_PERIC1_UID_SPEEDY2_DDI2_IPCLKPORT_sclk,
};
enum clk_id cmucal_vclk_speedy2_tsp1[] = {
	GOUT_BLK_PERIC1_UID_SPEEDY2_TSP1_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_speedy2_tsp2[] = {
	GOUT_BLK_PERIC1_UID_SPEEDY2_TSP2_IPCLKPORT_clk,
};
enum clk_id cmucal_vclk_spi_cam0[] = {
	GOUT_BLK_PERIC1_UID_SPI_CAM0_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_SPI_CAM0_IPCLKPORT_SPI_EXT_CLK,
};
enum clk_id cmucal_vclk_spi_cam1[] = {
	GOUT_BLK_PERIC1_UID_SPI_CAM1_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_SPI_CAM1_IPCLKPORT_SPI_EXT_CLK,
};
enum clk_id cmucal_vclk_sysreg_peric1[] = {
	GOUT_BLK_PERIC1_UID_SYSREG_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_uart_bt[] = {
	GOUT_BLK_PERIC1_UID_UART_BT_IPCLKPORT_EXT_UCLK,
	GOUT_BLK_PERIC1_UID_UART_BT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_usi04[] = {
	GOUT_BLK_PERIC1_UID_USI04_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI04_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi05[] = {
	GOUT_BLK_PERIC1_UID_USI05_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI05_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi06[] = {
	GOUT_BLK_PERIC1_UID_USI06_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI06_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi07[] = {
	GOUT_BLK_PERIC1_UID_USI07_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI07_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi08[] = {
	GOUT_BLK_PERIC1_UID_USI08_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI08_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi09[] = {
	GOUT_BLK_PERIC1_UID_USI09_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI09_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi10[] = {
	GOUT_BLK_PERIC1_UID_USI10_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI10_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi11[] = {
	GOUT_BLK_PERIC1_UID_USI11_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI11_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi12[] = {
	GOUT_BLK_PERIC1_UID_USI12_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI12_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_usi13[] = {
	GOUT_BLK_PERIC1_UID_USI13_IPCLKPORT_i_PCLK,
	GOUT_BLK_PERIC1_UID_USI13_IPCLKPORT_i_SCLK_USI,
};
enum clk_id cmucal_vclk_xiu_p_peric1[] = {
	GOUT_BLK_PERIC1_UID_XIU_P_PERIC1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ad_axi_p_peris[] = {
	GOUT_BLK_PERIS_UID_AD_AXI_P_PERIS_IPCLKPORT_ACLKM,
	GOUT_BLK_PERIS_UID_AD_AXI_P_PERIS_IPCLKPORT_ACLKS,
};
enum clk_id cmucal_vclk_axi2apb_perisp0[] = {
	GOUT_BLK_PERIS_UID_AXI2APB_PERISP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_perisp1[] = {
	GOUT_BLK_PERIS_UID_AXI2APB_PERISP1_IPCLKPORT_ACLK,
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
enum clk_id cmucal_vclk_pmu_peris[] = {
	GOUT_BLK_PERIS_UID_PMU_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_peris[] = {
	GOUT_BLK_PERIS_UID_SYSREG_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc00[] = {
	GOUT_BLK_PERIS_UID_TZPC00_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc01[] = {
	GOUT_BLK_PERIS_UID_TZPC01_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc02[] = {
	GOUT_BLK_PERIS_UID_TZPC02_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc03[] = {
	GOUT_BLK_PERIS_UID_TZPC03_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc04[] = {
	GOUT_BLK_PERIS_UID_TZPC04_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc05[] = {
	GOUT_BLK_PERIS_UID_TZPC05_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc06[] = {
	GOUT_BLK_PERIS_UID_TZPC06_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc07[] = {
	GOUT_BLK_PERIS_UID_TZPC07_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc08[] = {
	GOUT_BLK_PERIS_UID_TZPC08_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc09[] = {
	GOUT_BLK_PERIS_UID_TZPC09_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc10[] = {
	GOUT_BLK_PERIS_UID_TZPC10_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc11[] = {
	GOUT_BLK_PERIS_UID_TZPC11_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc12[] = {
	GOUT_BLK_PERIS_UID_TZPC12_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc13[] = {
	GOUT_BLK_PERIS_UID_TZPC13_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc14[] = {
	GOUT_BLK_PERIS_UID_TZPC14_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_tzpc15[] = {
	GOUT_BLK_PERIS_UID_TZPC15_IPCLKPORT_PCLK,
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
enum clk_id cmucal_vclk_ad_apb_srdz[] = {
	GOUT_BLK_SRDZ_UID_AD_APB_SRDZ_IPCLKPORT_PCLKM,
	GOUT_BLK_SRDZ_UID_AD_APB_SRDZ_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_axi2apb_srdz[] = {
	GOUT_BLK_SRDZ_UID_AXI2APB_SRDZ_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2apb_srdz_sys[] = {
	GOUT_BLK_SRDZ_UID_AXI2APB_SRDZ_SYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_srdz[] = {
	GOUT_BLK_SRDZ_UID_BTM_SRDZ_IPCLKPORT_I_ACLK,
	GOUT_BLK_SRDZ_UID_BTM_SRDZ_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_lhm_atb_dcamsrdz[] = {
	GOUT_BLK_SRDZ_UID_LHM_ATB_DCAMSRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_d_dcamsrdz[] = {
	GOUT_BLK_SRDZ_UID_LHM_AXI_D_DCAMSRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_srdz[] = {
	GOUT_BLK_SRDZ_UID_LHM_AXI_P_SRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_atb_srdzcam[] = {
	GOUT_BLK_SRDZ_UID_LHS_ATB_SRDZCAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_srdz[] = {
	GOUT_BLK_SRDZ_UID_LHS_AXI_D_SRDZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_p_srdzdcam[] = {
	GOUT_BLK_SRDZ_UID_LHS_AXI_P_SRDZDCAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_srdz[] = {
	GOUT_BLK_SRDZ_UID_PMU_SRDZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_srdz[] = {
	GOUT_BLK_SRDZ_UID_BCM_SRDZ_IPCLKPORT_ACLK,
	GOUT_BLK_SRDZ_UID_BCM_SRDZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_pxl_asbm_1to2_srdz[] = {
	GOUT_BLK_SRDZ_UID_PXL_ASBM_1to2_SRDZ_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_pxl_asbs_srdz[] = {
	GOUT_BLK_SRDZ_UID_PXL_ASBS_SRDZ_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_smmu_srdz[] = {
	GOUT_BLK_SRDZ_UID_SMMU_SRDZ_IPCLKPORT_ACLK,
	GOUT_BLK_SRDZ_UID_SMMU_SRDZ_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_SRDZ_UID_SMMU_SRDZ_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_srdz[] = {
	GOUT_BLK_SRDZ_UID_SRDZ_IPCLKPORT_Clk,
};
enum clk_id cmucal_vclk_srdz_cmu_srdz[] = {
	CLK_BLK_SRDZ_UID_SRDZ_CMU_SRDZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_srdz[] = {
	GOUT_BLK_SRDZ_UID_SYSREG_SRDZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_d_srdz[] = {
	GOUT_BLK_SRDZ_UID_XIU_D_SRDZ_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_srdz[] = {
	GOUT_BLK_SRDZ_UID_XIU_P_SRDZ_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_axi2ahb_vpu[] = {
	GOUT_BLK_VPU_UID_AXI2AHB_VPU_IPCLKPORT_aclk,
};
enum clk_id cmucal_vclk_axi2apb_vpu[] = {
	GOUT_BLK_VPU_UID_AXI2APB_VPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_btm_vpu[] = {
	GOUT_BLK_VPU_UID_BTM_VPU_IPCLKPORT_I_ACLK,
	GOUT_BLK_VPU_UID_BTM_VPU_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_dspvpu[] = {
	GOUT_BLK_VPU_UID_LHM_AXI_P_DSPVPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_vpu[] = {
	GOUT_BLK_VPU_UID_LHM_AXI_P_VPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_acel_d_vpu[] = {
	GOUT_BLK_VPU_UID_LHS_ACEL_D_VPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_vpudsp[] = {
	GOUT_BLK_VPU_UID_LHS_AXI_D_VPUDSP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_pmu_vpu[] = {
	GOUT_BLK_VPU_UID_PMU_VPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_bcm_vpu[] = {
	GOUT_BLK_VPU_UID_BCM_VPU_IPCLKPORT_ACLK,
	GOUT_BLK_VPU_UID_BCM_VPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_smmu_vpu[] = {
	GOUT_BLK_VPU_UID_SMMU_VPU_IPCLKPORT_ACLK,
	GOUT_BLK_VPU_UID_SMMU_VPU_IPCLKPORT_PCLK_NONSECURE,
	GOUT_BLK_VPU_UID_SMMU_VPU_IPCLKPORT_PCLK_SECURE,
};
enum clk_id cmucal_vclk_sysreg_vpu[] = {
	GOUT_BLK_VPU_UID_SYSREG_VPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_vpu[] = {
	GOUT_BLK_VPU_UID_VPU_IPCLKPORT_Clk,
	GOUT_BLK_VPU_UID_VPU_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_vpu_cmu_vpu[] = {
	CLK_BLK_VPU_UID_VPU_CMU_VPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_xiu_d_vpu[] = {
	GOUT_BLK_VPU_UID_XIU_D_VPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_xiu_p_vpu[] = {
	GOUT_BLK_VPU_UID_XIU_P_VPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_dmic_ahb[] = {
	GOUT_BLK_VTS_UID_DMIC_AHB_IPCLKPORT_HCLK,
	GOUT_BLK_VTS_UID_DMIC_AHB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_dmic_if[] = {
	GOUT_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMICIF_CLK,
	CLK_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_CLK,
	GOUT_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_DIV2_CLK,
	GOUT_BLK_VTS_UID_DMIC_IF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_gpio_vts[] = {
	GOUT_BLK_VTS_UID_GPIO_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_lhm_axi_p_vts[] = {
	GOUT_BLK_VTS_UID_LHM_AXI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_lhs_axi_d_vts[] = {
	GOUT_BLK_VTS_UID_LHS_AXI_D_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_mailbox_vts2ap[] = {
	GOUT_BLK_VTS_UID_MAILBOX_VTS2AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_sysreg_vts[] = {
	GOUT_BLK_VTS_UID_SYSREG_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_vts[] = {
	GOUT_BLK_VTS_UID_VTS_IPCLKPORT_ACLK_CPU,
	GOUT_BLK_VTS_UID_VTS_IPCLKPORT_ACLK_SYS,
};
enum clk_id cmucal_vclk_vts_cmu_vts[] = {
	CLK_BLK_VTS_UID_VTS_CMU_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_wdt_vts[] = {
	GOUT_BLK_VTS_UID_WDT_VTS_IPCLKPORT_PCLK,
};

/* Switching LUT */
/* -1 is the Value of EMPTY_CLK_ID */
struct switch_lut spl_clk_abox_cpu_aclk_blk_cmu_lut[] = {
	{1066000, 0, 0},
	{667333, 3, 0},
	{400000, 2, 1},
};
struct switch_lut spl_clk_cpucl0_cmuref_blk_cmu_lut[] = {
	{1066000, 0, 0},
};
struct switch_lut div_clk_cpucl1_cmuref_blk_cmu_lut[] = {
	{1066000, 0, 0},
	{932750, 1, 0},
	{533000, 0, 1},
	{266500, 0, 3},
};
struct switch_lut gate_clk_g3d_agpu_blk_cmu_lut[] = {
	{800000, -1, 0},
	{400000, -1, 1},
	{266666, -1, 2},
};
struct switch_lut occ_mif_cmuref_blk_cmu_lut[] = {
	{2132000, 0, -1},
	{1865500, 1, -1},
	{932750, 3, -1},
};

/* DVFS LUT */
struct vclk_lut vddi_lut[] = {
	{400000, vddi_l0_lut_params},
	{300000, vddi_l1_lut_params},
	{200000, vddi_l2_lut_params},
	{100000, vddi_l3_lut_params},
};
struct vclk_lut vdd_mif_lut[] = {
	{4264000, vdd_mif_od_lut_params},
	{3731000, vdd_mif_nm_lut_params},
	{2576600, vdd_mif_ud_lut_params},
	{1418000, vdd_mif_sud_lut_params},
};
struct vclk_lut vdd_g3d_lut[] = {
	{860000, vdd_g3d_nm_lut_params},
	{600000, vdd_g3d_ud_lut_params},
	{300000, vdd_g3d_sud_lut_params},
};
struct vclk_lut vdd_mngs_lut[] = {
	{2860000, vdd_mngs_sod_lut_params},
	{2160167, vdd_mngs_od_lut_params},
	{1924000, vdd_mngs_nm_lut_params},
	{1069900, vdd_mngs_ud_lut_params},
	{400000, vdd_mngs_sud_lut_params},
};
struct vclk_lut vdd_apollo_lut[] = {
	{1906666, vdd_apollo_sod_lut_params},
	{1450000, vdd_apollo_od_lut_params},
	{970666, vdd_apollo_nm_lut_params},
	{600000, vdd_apollo_ud_lut_params},
	{300000, vdd_apollo_sud_lut_params},
};
struct vclk_lut dfs_abox_lut[] = {
	{1179648, dfs_abox_nm_lut_params},
	{589824, dfs_abox_ud_lut_params},
	{393216, dfs_abox_sud_lut_params},
	{98304, dfs_abox_ssud_lut_params},
};

/* SPECIAL LUT */
struct vclk_lut spl_clk_abox_uaif0_blk_abox_lut[] = {
	{25615, spl_clk_abox_uaif0_blk_abox_ud_lut_params},
	{25000, spl_clk_abox_uaif0_blk_abox_nm_lut_params},
};
struct vclk_lut spl_clk_abox_uaif2_blk_abox_lut[] = {
	{25615, spl_clk_abox_uaif2_blk_abox_ud_lut_params},
	{25000, spl_clk_abox_uaif2_blk_abox_nm_lut_params},
};
struct vclk_lut spl_clk_abox_uaif4_blk_abox_lut[] = {
	{25615, spl_clk_abox_uaif4_blk_abox_ud_lut_params},
	{25000, spl_clk_abox_uaif4_blk_abox_nm_lut_params},
};
struct vclk_lut div_clk_abox_dmic_blk_abox_lut[] = {
	{25615, div_clk_abox_dmic_blk_abox_ud_lut_params},
	{25000, div_clk_abox_dmic_blk_abox_nm_lut_params},
};
struct vclk_lut spl_clk_abox_cpu_pclkdbg_blk_abox_lut[] = {
	{150000, spl_clk_abox_cpu_pclkdbg_blk_abox_nm_lut_params},
	{83250, spl_clk_abox_cpu_pclkdbg_blk_abox_ud_lut_params},
	{50000, spl_clk_abox_cpu_pclkdbg_blk_abox_sud_lut_params},
};
struct vclk_lut spl_clk_abox_uaif1_blk_abox_lut[] = {
	{25615, spl_clk_abox_uaif1_blk_abox_ud_lut_params},
	{25000, spl_clk_abox_uaif1_blk_abox_nm_lut_params},
};
struct vclk_lut spl_clk_abox_uaif3_blk_abox_lut[] = {
	{25615, spl_clk_abox_uaif3_blk_abox_ud_lut_params},
	{25000, spl_clk_abox_uaif3_blk_abox_nm_lut_params},
};
struct vclk_lut spl_clk_abox_cpu_aclk_blk_abox_lut[] = {
	{600000, spl_clk_abox_cpu_aclk_blk_abox_nm_lut_params},
	{333000, spl_clk_abox_cpu_aclk_blk_abox_ud_lut_params},
	{200000, spl_clk_abox_cpu_aclk_blk_abox_sud_lut_params},
};
struct vclk_lut spl_clk_fsys1_mmc_card_blk_cmu_lut[] = {
	{800000, spl_clk_fsys1_mmc_card_blk_cmu_nm_lut_params},
	{400000, spl_clk_fsys1_mmc_card_blk_cmu_mmc_l1_lut_params},
	{200000, spl_clk_fsys1_mmc_card_blk_cmu_mmc_l2_lut_params},
	{100000, spl_clk_fsys1_mmc_card_blk_cmu_mmc_l3_lut_params},
};
struct vclk_lut spl_clk_peric0_usi00_blk_cmu_lut[] = {
	{200000, spl_clk_peric0_usi00_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric0_usi00_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric0_usi00_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric0_usi00_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric0_usi00_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric0_usi00_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric0_usi03_blk_cmu_lut[] = {
	{200000, spl_clk_peric0_usi03_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric0_usi03_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric0_usi03_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric0_usi03_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric0_usi03_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric0_usi03_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_uart_bt_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_uart_bt_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_peric1_usi06_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi06_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi06_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi06_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi06_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi06_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi06_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi09_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi09_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi09_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi09_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi09_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi09_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi09_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi12_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi12_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi12_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi12_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi12_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi12_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi12_blk_cmu_l5_lut_params},
};
struct vclk_lut occ_cmu_cmuref_blk_cmu_lut[] = {
	{400000, occ_cmu_cmuref_blk_cmu_nm_lut_params},
	{266500, occ_cmu_cmuref_blk_cmu_sud_lut_params},
};
struct vclk_lut clkcmu_cis_clk0_blk_cmu_lut[] = {
	{100000, clkcmu_cis_clk0_blk_cmu_l2_lut_params},
	{26000, clkcmu_cis_clk0_blk_cmu_l1_lut_params},
};
struct vclk_lut clkcmu_cis_clk2_blk_cmu_lut[] = {
	{100000, clkcmu_cis_clk2_blk_cmu_l2_lut_params},
	{26000, clkcmu_cis_clk2_blk_cmu_l1_lut_params},
};
struct vclk_lut clkcmu_hpm_blk_cmu_lut[] = {
	{466375, clkcmu_hpm_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_fsys0_dpgtc_blk_cmu_lut[] = {
	{133250, spl_clk_fsys0_dpgtc_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_fsys0_mmc_embd_blk_cmu_lut[] = {
	{800000, spl_clk_fsys0_mmc_embd_blk_cmu_nm_lut_params},
	{400000, spl_clk_fsys0_mmc_embd_blk_cmu_l1_lut_params},
	{200000, spl_clk_fsys0_mmc_embd_blk_cmu_l2_lut_params},
	{100000, spl_clk_fsys0_mmc_embd_blk_cmu_l3_lut_params},
};
struct vclk_lut spl_clk_fsys0_ufs_embd_blk_cmu_lut[] = {
	{177667, spl_clk_fsys0_ufs_embd_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_fsys0_usbdrd30_blk_cmu_lut[] = {
	{66625, spl_clk_fsys0_usbdrd30_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_fsys1_ufs_card_blk_cmu_lut[] = {
	{177667, spl_clk_fsys1_ufs_card_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_peric0_uart_dbg_blk_cmu_lut[] = {
	{200000, spl_clk_peric0_uart_dbg_blk_cmu_nm_lut_params},
};
struct vclk_lut spl_clk_peric0_usi01_blk_cmu_lut[] = {
	{200000, spl_clk_peric0_usi01_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric0_usi01_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric0_usi01_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric0_usi01_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric0_usi01_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric0_usi01_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric0_usi02_blk_cmu_lut[] = {
	{200000, spl_clk_peric0_usi02_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric0_usi02_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric0_usi02_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric0_usi02_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric0_usi02_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric0_usi02_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_spi_cam0_blk_cmu_lut[] = {
	{100000, spl_clk_peric1_spi_cam0_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_spi_cam0_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_spi_cam0_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_spi_cam0_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_spi_cam0_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_spi_cam1_blk_cmu_lut[] = {
	{100000, spl_clk_peric1_spi_cam1_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_spi_cam1_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_spi_cam1_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_spi_cam1_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_spi_cam1_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi04_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi04_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi04_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi04_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi04_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi04_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi04_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi05_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi05_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi05_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi05_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi05_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi05_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi05_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi07_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi07_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi07_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi07_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi07_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi07_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi07_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi08_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi08_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi08_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi08_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi08_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi08_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi08_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi10_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi10_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi10_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi10_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi10_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi10_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi10_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi11_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi11_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi11_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi11_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi11_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi11_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi11_blk_cmu_l5_lut_params},
};
struct vclk_lut spl_clk_peric1_usi13_blk_cmu_lut[] = {
	{200000, spl_clk_peric1_usi13_blk_cmu_nm_lut_params},
	{100000, spl_clk_peric1_usi13_blk_cmu_l1_lut_params},
	{50000, spl_clk_peric1_usi13_blk_cmu_l2_lut_params},
	{26000, spl_clk_peric1_usi13_blk_cmu_l4_lut_params},
	{25000, spl_clk_peric1_usi13_blk_cmu_l3_lut_params},
	{13000, spl_clk_peric1_usi13_blk_cmu_l5_lut_params},
};
struct vclk_lut clkcmu_cis_clk1_blk_cmu_lut[] = {
	{100000, clkcmu_cis_clk1_blk_cmu_l2_lut_params},
	{26000, clkcmu_cis_clk1_blk_cmu_l1_lut_params},
};
struct vclk_lut clkcmu_cis_clk3_blk_cmu_lut[] = {
	{100000, clkcmu_cis_clk3_blk_cmu_l2_lut_params},
	{26000, clkcmu_cis_clk3_blk_cmu_l1_lut_params},
};
struct vclk_lut occ_clk_fsys1_pcie_blk_cmu_lut[] = {
	{800000, occ_clk_fsys1_pcie_blk_cmu_nm_lut_params},
};
struct vclk_lut div_clk_cluster0_atclk_blk_cpucl0_lut[] = {
	{664950, div_clk_cluster0_atclk_blk_cpucl0_sod_lut_params},
	{540042, div_clk_cluster0_atclk_blk_cpucl0_od_lut_params},
	{424938, div_clk_cluster0_atclk_blk_cpucl0_nm_lut_params},
	{267475, div_clk_cluster0_atclk_blk_cpucl0_ud_lut_params},
	{100000, div_clk_cluster0_atclk_blk_cpucl0_sud_lut_params},
};
struct vclk_lut spl_clk_cpucl0_cmuref_blk_cpucl0_lut[] = {
	{1329900, spl_clk_cpucl0_cmuref_blk_cpucl0_sod_lut_params},
	{1080083, spl_clk_cpucl0_cmuref_blk_cpucl0_od_lut_params},
	{849875, spl_clk_cpucl0_cmuref_blk_cpucl0_nm_lut_params},
	{534950, spl_clk_cpucl0_cmuref_blk_cpucl0_ud_lut_params},
	{200000, spl_clk_cpucl0_cmuref_blk_cpucl0_sud_lut_params},
};
struct vclk_lut div_clk_cpucl0_pclkdbg_blk_cpucl0_lut[] = {
	{332475, div_clk_cpucl0_pclkdbg_blk_cpucl0_sod_lut_params},
	{270021, div_clk_cpucl0_pclkdbg_blk_cpucl0_od_lut_params},
	{212469, div_clk_cpucl0_pclkdbg_blk_cpucl0_nm_lut_params},
	{133738, div_clk_cpucl0_pclkdbg_blk_cpucl0_ud_lut_params},
	{50000, div_clk_cpucl0_pclkdbg_blk_cpucl0_sud_lut_params},
};
struct vclk_lut div_clk_cluster1_cntclk_blk_cpucl1_lut[] = {
	{450000, div_clk_cluster1_cntclk_blk_cpucl1_sod_lut_params},
	{362500, div_clk_cluster1_cntclk_blk_cpucl1_od_lut_params},
	{262438, div_clk_cluster1_cntclk_blk_cpucl1_nm_lut_params},
	{150000, div_clk_cluster1_cntclk_blk_cpucl1_ud_lut_params},
	{75000, div_clk_cluster1_cntclk_blk_cpucl1_sud_lut_params},
};
struct vclk_lut div_clk_cpucl1_cmuref_blk_cpucl1_lut[] = {
	{900000, div_clk_cpucl1_cmuref_blk_cpucl1_sod_lut_params},
	{725000, div_clk_cpucl1_cmuref_blk_cpucl1_od_lut_params},
	{524875, div_clk_cpucl1_cmuref_blk_cpucl1_nm_lut_params},
	{300000, div_clk_cpucl1_cmuref_blk_cpucl1_ud_lut_params},
	{150000, div_clk_cpucl1_cmuref_blk_cpucl1_sud_lut_params},
};
struct vclk_lut div_clk_cluster1_atclk_blk_cpucl1_lut[] = {
	{450000, div_clk_cluster1_atclk_blk_cpucl1_sod_lut_params},
	{362500, div_clk_cluster1_atclk_blk_cpucl1_od_lut_params},
	{262438, div_clk_cluster1_atclk_blk_cpucl1_nm_lut_params},
	{150000, div_clk_cluster1_atclk_blk_cpucl1_ud_lut_params},
	{75000, div_clk_cluster1_atclk_blk_cpucl1_sud_lut_params},
};
struct vclk_lut div_clk_cluster1_pclkdbg_blk_cpucl1_lut[] = {
	{225000, div_clk_cluster1_pclkdbg_blk_cpucl1_sod_lut_params},
	{181250, div_clk_cluster1_pclkdbg_blk_cpucl1_od_lut_params},
	{131219, div_clk_cluster1_pclkdbg_blk_cpucl1_nm_lut_params},
	{75000, div_clk_cluster1_pclkdbg_blk_cpucl1_ud_lut_params},
	{37500, div_clk_cluster1_pclkdbg_blk_cpucl1_sud_lut_params},
};
struct vclk_lut spl_clk_dpu1_decon2_blk_dpu1_lut[] = {
	{600000, spl_clk_dpu1_decon2_blk_dpu1_nm_lut_params},
	{175000, spl_clk_dpu1_decon2_blk_dpu1_sud_lut_params},
};
struct vclk_lut occ_mif_cmuref_blk_mif_lut[] = {
	{26000, occ_mif_cmuref_blk_mif_nm_lut_params},
};
struct vclk_lut occ_mif1_cmuref_blk_mif1_lut[] = {
	{26000, occ_mif1_cmuref_blk_mif1_nm_lut_params},
};
struct vclk_lut occ_mif2_cmuref_blk_mif2_lut[] = {
	{26000, occ_mif2_cmuref_blk_mif2_nm_lut_params},
};
struct vclk_lut occ_mif3_cmuref_blk_mif3_lut[] = {
	{26000, occ_mif3_cmuref_blk_mif3_nm_lut_params},
};
struct vclk_lut spl_clk_vts_dmicif_div2_blk_vts_lut[] = {
	{2048, spl_clk_vts_dmicif_div2_blk_vts_nm_lut_params},
};
struct vclk_lut div_clk_vts_dmic_blk_vts_lut[] = {
	{4096, div_clk_vts_dmic_blk_vts_nm_lut_params},
};

/* COMMON LUT */
struct vclk_lut blk_abox_lut[] = {
	{1179648, blk_abox_lut_params},
};
struct vclk_lut blk_bus1_lut[] = {
	{266500, blk_bus1_lut_params},
};
struct vclk_lut blk_busc_lut[] = {
	{266500, blk_busc_lut_params},
};
struct vclk_lut blk_cam_lut[] = {
	{266500, blk_cam_lut_params},
};
struct vclk_lut blk_cmu_lut[] = {
	{2132000, blk_cmu_lut_params},
};
struct vclk_lut blk_core_lut[] = {
	{355333, blk_core_lut_params},
};
struct vclk_lut blk_cpucl0_lut[] = {
	{212469, blk_cpucl0_lut_params},
};
struct vclk_lut blk_cpucl1_lut[] = {
	{131219, blk_cpucl1_lut_params},
};
struct vclk_lut blk_dbg_lut[] = {
	{200000, blk_dbg_lut_params},
};
struct vclk_lut blk_dcam_lut[] = {
	{222444, blk_dcam_lut_params},
};
struct vclk_lut blk_dpu0_lut[] = {
	{157500, blk_dpu0_lut_params},
};
struct vclk_lut blk_dsp_lut[] = {
	{266500, blk_dsp_lut_params},
};
struct vclk_lut blk_g2d_lut[] = {
	{266500, blk_g2d_lut_params},
};
struct vclk_lut blk_g3d_lut[] = {
	{215000, blk_g3d_lut_params},
};
struct vclk_lut blk_isphq_lut[] = {
	{266500, blk_isphq_lut_params},
};
struct vclk_lut blk_isplp_lut[] = {
	{266500, blk_isplp_lut_params},
};
struct vclk_lut blk_iva_lut[] = {
	{266500, blk_iva_lut_params},
};
struct vclk_lut blk_mfc_lut[] = {
	{166833, blk_mfc_lut_params},
};
struct vclk_lut blk_mif_lut[] = {
	{233188, blk_mif_lut_params},
};
struct vclk_lut blk_mif1_lut[] = {
	{1865500, blk_mif1_lut_params},
};
struct vclk_lut blk_mif2_lut[] = {
	{1865500, blk_mif2_lut_params},
};
struct vclk_lut blk_mif3_lut[] = {
	{1865500, blk_mif3_lut_params},
};
struct vclk_lut blk_srdz_lut[] = {
	{266500, blk_srdz_lut_params},
};
struct vclk_lut blk_vpu_lut[] = {
	{266500, blk_vpu_lut_params},
};
struct vclk_lut blk_vts_lut[] = {
	{49152, blk_vts_lut_params},
};
/*=================VCLK Switch list================================*/

struct vclk_switch vclk_switch_blk_abox[] = {
	{MUX_CLK_ABOX_CPU, MUX_CLKCMU_ABOX_CPUABOX, CLKCMU_ABOX_CPUABOX, GATE_CLKCMU_ABOX_CPUABOX, MUX_CLKCMU_ABOX_CPUABOX_USER, spl_clk_abox_cpu_aclk_blk_cmu_lut, 3},
};
struct vclk_switch vclk_switch_blk_cpucl0[] = {
	{MUX_CLK_CPUCL0_PLL, MUX_CLKCMU_CPUCL0_SWITCH, CLKCMU_CPUCL0_SWITCH, GATE_CLKCMU_CPUCL0_SWITCH, MUX_CLKCMU_CPUCL0_SWITCH_USER, spl_clk_cpucl0_cmuref_blk_cmu_lut, 1},
};
struct vclk_switch vclk_switch_blk_cpucl1[] = {
	{MUX_CLK_CPUCL1_PLL, MUX_CLKCMU_CPUCL1_SWITCH, CLKCMU_CPUCL1_SWITCH, GATE_CLKCMU_CPUCL1_SWITCH, MUX_CLKCMU_CPUCL1_SWITCH_USER, div_clk_cpucl1_cmuref_blk_cmu_lut, 4},
};
struct vclk_switch vclk_switch_blk_g3d[] = {
	{MUX_CLK_G3D_BUSD, EMPTY_CLK_ID, CLKCMU_G3D_SWITCH, GATE_CLKCMU_G3D_SWITCH, MUX_CLKCMU_G3D_SWITCH_USER, gate_clk_g3d_agpu_blk_cmu_lut, 3},
};
struct vclk_switch vclk_switch_blk_mif[] = {
	{CLKMUX_MIF_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, occ_mif_cmuref_blk_cmu_lut, 3},
};
struct vclk_switch vclk_switch_blk_mif1[] = {
	{CLKMUX_MIF1_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, occ_mif_cmuref_blk_cmu_lut, 3},
};
struct vclk_switch vclk_switch_blk_mif2[] = {
	{CLKMUX_MIF2_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, occ_mif_cmuref_blk_cmu_lut, 3},
};
struct vclk_switch vclk_switch_blk_mif3[] = {
	{CLKMUX_MIF3_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, CLKCMU_MIF_SWITCH, EMPTY_CLK_ID, occ_mif_cmuref_blk_cmu_lut, 3},
};

/*=================VCLK list================================*/

struct vclk cmucal_vclk_list[] = {

/* DVFS VCLK */
	CMUCAL_VCLK(VCLK_VDDI, vddi_lut, cmucal_vclk_vddi, NULL, NULL),
	CMUCAL_VCLK(VCLK_VDD_MIF, vdd_mif_lut, cmucal_vclk_vdd_mif, NULL, vclk_switch_blk_mif),
	CMUCAL_VCLK(VCLK_VDD_G3D, vdd_g3d_lut, cmucal_vclk_vdd_g3d, NULL, vclk_switch_blk_g3d),
	CMUCAL_VCLK(VCLK_VDD_MNGS, vdd_mngs_lut, cmucal_vclk_vdd_mngs, NULL, vclk_switch_blk_cpucl0),
	CMUCAL_VCLK(VCLK_VDD_APOLLO, vdd_apollo_lut, cmucal_vclk_vdd_apollo, NULL, vclk_switch_blk_cpucl1),
	CMUCAL_VCLK(VCLK_DFS_ABOX, dfs_abox_lut, cmucal_vclk_dfs_abox, NULL, NULL),

/* SPECIAL VCLK */
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_UAIF0_BLK_ABOX, spl_clk_abox_uaif0_blk_abox_lut, cmucal_vclk_spl_clk_abox_uaif0_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_UAIF2_BLK_ABOX, spl_clk_abox_uaif2_blk_abox_lut, cmucal_vclk_spl_clk_abox_uaif2_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_UAIF4_BLK_ABOX, spl_clk_abox_uaif4_blk_abox_lut, cmucal_vclk_spl_clk_abox_uaif4_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_ABOX_DMIC_BLK_ABOX, div_clk_abox_dmic_blk_abox_lut, cmucal_vclk_div_clk_abox_dmic_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_CPU_PCLKDBG_BLK_ABOX, spl_clk_abox_cpu_pclkdbg_blk_abox_lut, cmucal_vclk_spl_clk_abox_cpu_pclkdbg_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_UAIF1_BLK_ABOX, spl_clk_abox_uaif1_blk_abox_lut, cmucal_vclk_spl_clk_abox_uaif1_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_UAIF3_BLK_ABOX, spl_clk_abox_uaif3_blk_abox_lut, cmucal_vclk_spl_clk_abox_uaif3_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_ABOX_CPU_ACLK_BLK_ABOX, spl_clk_abox_cpu_aclk_blk_abox_lut, cmucal_vclk_spl_clk_abox_cpu_aclk_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_FSYS1_MMC_CARD_BLK_CMU, spl_clk_fsys1_mmc_card_blk_cmu_lut, cmucal_vclk_spl_clk_fsys1_mmc_card_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC0_USI00_BLK_CMU, spl_clk_peric0_usi00_blk_cmu_lut, cmucal_vclk_spl_clk_peric0_usi00_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC0_USI03_BLK_CMU, spl_clk_peric0_usi03_blk_cmu_lut, cmucal_vclk_spl_clk_peric0_usi03_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_UART_BT_BLK_CMU, spl_clk_peric1_uart_bt_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_uart_bt_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI06_BLK_CMU, spl_clk_peric1_usi06_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi06_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI09_BLK_CMU, spl_clk_peric1_usi09_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi09_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI12_BLK_CMU, spl_clk_peric1_usi12_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi12_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_CMU_CMUREF_BLK_CMU, occ_cmu_cmuref_blk_cmu_lut, cmucal_vclk_occ_cmu_cmuref_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK0_BLK_CMU, clkcmu_cis_clk0_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk0_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK2_BLK_CMU, clkcmu_cis_clk2_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk2_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_HPM_BLK_CMU, clkcmu_hpm_blk_cmu_lut, cmucal_vclk_clkcmu_hpm_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_FSYS0_DPGTC_BLK_CMU, spl_clk_fsys0_dpgtc_blk_cmu_lut, cmucal_vclk_spl_clk_fsys0_dpgtc_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_FSYS0_MMC_EMBD_BLK_CMU, spl_clk_fsys0_mmc_embd_blk_cmu_lut, cmucal_vclk_spl_clk_fsys0_mmc_embd_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_FSYS0_UFS_EMBD_BLK_CMU, spl_clk_fsys0_ufs_embd_blk_cmu_lut, cmucal_vclk_spl_clk_fsys0_ufs_embd_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_FSYS0_USBDRD30_BLK_CMU, spl_clk_fsys0_usbdrd30_blk_cmu_lut, cmucal_vclk_spl_clk_fsys0_usbdrd30_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_FSYS1_UFS_CARD_BLK_CMU, spl_clk_fsys1_ufs_card_blk_cmu_lut, cmucal_vclk_spl_clk_fsys1_ufs_card_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC0_UART_DBG_BLK_CMU, spl_clk_peric0_uart_dbg_blk_cmu_lut, cmucal_vclk_spl_clk_peric0_uart_dbg_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC0_USI01_BLK_CMU, spl_clk_peric0_usi01_blk_cmu_lut, cmucal_vclk_spl_clk_peric0_usi01_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC0_USI02_BLK_CMU, spl_clk_peric0_usi02_blk_cmu_lut, cmucal_vclk_spl_clk_peric0_usi02_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_SPI_CAM0_BLK_CMU, spl_clk_peric1_spi_cam0_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_spi_cam0_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_SPI_CAM1_BLK_CMU, spl_clk_peric1_spi_cam1_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_spi_cam1_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI04_BLK_CMU, spl_clk_peric1_usi04_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi04_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI05_BLK_CMU, spl_clk_peric1_usi05_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi05_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI07_BLK_CMU, spl_clk_peric1_usi07_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi07_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI08_BLK_CMU, spl_clk_peric1_usi08_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi08_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI10_BLK_CMU, spl_clk_peric1_usi10_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi10_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI11_BLK_CMU, spl_clk_peric1_usi11_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi11_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_PERIC1_USI13_BLK_CMU, spl_clk_peric1_usi13_blk_cmu_lut, cmucal_vclk_spl_clk_peric1_usi13_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK1_BLK_CMU, clkcmu_cis_clk1_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk1_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK3_BLK_CMU, clkcmu_cis_clk3_blk_cmu_lut, cmucal_vclk_clkcmu_cis_clk3_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_CLK_FSYS1_PCIE_BLK_CMU, occ_clk_fsys1_pcie_blk_cmu_lut, cmucal_vclk_occ_clk_fsys1_pcie_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER0_ATCLK_BLK_CPUCL0, div_clk_cluster0_atclk_blk_cpucl0_lut, cmucal_vclk_div_clk_cluster0_atclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_CPUCL0_CMUREF_BLK_CPUCL0, spl_clk_cpucl0_cmuref_blk_cpucl0_lut, cmucal_vclk_spl_clk_cpucl0_cmuref_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL0_PCLKDBG_BLK_CPUCL0, div_clk_cpucl0_pclkdbg_blk_cpucl0_lut, cmucal_vclk_div_clk_cpucl0_pclkdbg_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER1_CNTCLK_BLK_CPUCL1, div_clk_cluster1_cntclk_blk_cpucl1_lut, cmucal_vclk_div_clk_cluster1_cntclk_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL1_CMUREF_BLK_CPUCL1, div_clk_cpucl1_cmuref_blk_cpucl1_lut, cmucal_vclk_div_clk_cpucl1_cmuref_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER1_ATCLK_BLK_CPUCL1, div_clk_cluster1_atclk_blk_cpucl1_lut, cmucal_vclk_div_clk_cluster1_atclk_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER1_PCLKDBG_BLK_CPUCL1, div_clk_cluster1_pclkdbg_blk_cpucl1_lut, cmucal_vclk_div_clk_cluster1_pclkdbg_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_DPU1_DECON2_BLK_DPU1, spl_clk_dpu1_decon2_blk_dpu1_lut, cmucal_vclk_spl_clk_dpu1_decon2_blk_dpu1, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_MIF_CMUREF_BLK_MIF, occ_mif_cmuref_blk_mif_lut, cmucal_vclk_occ_mif_cmuref_blk_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_MIF1_CMUREF_BLK_MIF1, occ_mif1_cmuref_blk_mif1_lut, cmucal_vclk_occ_mif1_cmuref_blk_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_MIF2_CMUREF_BLK_MIF2, occ_mif2_cmuref_blk_mif2_lut, cmucal_vclk_occ_mif2_cmuref_blk_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_OCC_MIF3_CMUREF_BLK_MIF3, occ_mif3_cmuref_blk_mif3_lut, cmucal_vclk_occ_mif3_cmuref_blk_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPL_CLK_VTS_DMICIF_DIV2_BLK_VTS, spl_clk_vts_dmicif_div2_blk_vts_lut, cmucal_vclk_spl_clk_vts_dmicif_div2_blk_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_VTS_DMIC_BLK_VTS, div_clk_vts_dmic_blk_vts_lut, cmucal_vclk_div_clk_vts_dmic_blk_vts, NULL, NULL),

/* COMMON VCLK */
	CMUCAL_VCLK(VCLK_BLK_ABOX, blk_abox_lut, cmucal_vclk_blk_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_BUS1, blk_bus1_lut, cmucal_vclk_blk_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_BUSC, blk_busc_lut, cmucal_vclk_blk_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CAM, blk_cam_lut, cmucal_vclk_blk_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CMU, blk_cmu_lut, cmucal_vclk_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CORE, blk_core_lut, cmucal_vclk_blk_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CPUCL0, blk_cpucl0_lut, cmucal_vclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CPUCL1, blk_cpucl1_lut, cmucal_vclk_blk_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DBG, blk_dbg_lut, cmucal_vclk_blk_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DCAM, blk_dcam_lut, cmucal_vclk_blk_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DPU0, blk_dpu0_lut, cmucal_vclk_blk_dpu0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DSP, blk_dsp_lut, cmucal_vclk_blk_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G2D, blk_g2d_lut, cmucal_vclk_blk_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G3D, blk_g3d_lut, cmucal_vclk_blk_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPHQ, blk_isphq_lut, cmucal_vclk_blk_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPLP, blk_isplp_lut, cmucal_vclk_blk_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_IVA, blk_iva_lut, cmucal_vclk_blk_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MFC, blk_mfc_lut, cmucal_vclk_blk_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MIF, blk_mif_lut, cmucal_vclk_blk_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MIF1, blk_mif1_lut, cmucal_vclk_blk_mif1, NULL, vclk_switch_blk_mif1),
	CMUCAL_VCLK(VCLK_BLK_MIF2, blk_mif2_lut, cmucal_vclk_blk_mif2, NULL, vclk_switch_blk_mif2),
	CMUCAL_VCLK(VCLK_BLK_MIF3, blk_mif3_lut, cmucal_vclk_blk_mif3, NULL, vclk_switch_blk_mif3),
	CMUCAL_VCLK(VCLK_BLK_SRDZ, blk_srdz_lut, cmucal_vclk_blk_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_VPU, blk_vpu_lut, cmucal_vclk_blk_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_VTS, blk_vts_lut, cmucal_vclk_blk_vts, NULL, NULL),

/* GATING VCLK */
	CMUCAL_VCLK(VCLK_ABOX_CMU_ABOX, NULL, cmucal_vclk_abox_cmu_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_ABOX_DAP, NULL, cmucal_vclk_abox_dap, NULL, NULL),
	CMUCAL_VCLK(VCLK_ABOX_TOP, NULL, cmucal_vclk_abox_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI_US_32to128, NULL, cmucal_vclk_axi_us_32to128, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_ABOX, NULL, cmucal_vclk_btm_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_DFTMUX_ABOX, NULL, cmucal_vclk_dftmux_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMIC, NULL, cmucal_vclk_dmic, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_ABOX, NULL, cmucal_vclk_gpio_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_ABOX, NULL, cmucal_vclk_lhm_axi_p_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_ABOX, NULL, cmucal_vclk_lhs_atb_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_ABOX, NULL, cmucal_vclk_lhs_axi_d_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERI_AXI_ASB, NULL, cmucal_vclk_peri_axi_asb, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_ABOX, NULL, cmucal_vclk_pmu_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_ABOX, NULL, cmucal_vclk_bcm_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_ABOX, NULL, cmucal_vclk_smmu_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_ABOX, NULL, cmucal_vclk_sysreg_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_ABOX, NULL, cmucal_vclk_trex_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_ABOXCPU, NULL, cmucal_vclk_wdt_aboxcpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_WRAP2_CONV_ABOX, NULL, cmucal_vclk_wrap2_conv_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_APM, NULL, cmucal_vclk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_APM_CMU_APM, NULL, cmucal_vclk_apm_cmu_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_ALIVE, NULL, cmucal_vclk_lhm_axi_p_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_ALIVE, NULL, cmucal_vclk_lhs_axi_d_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2AP, NULL, cmucal_vclk_mailbox_apm2ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2CP, NULL, cmucal_vclk_mailbox_apm2cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_APM2GNSS, NULL, cmucal_vclk_mailbox_apm2gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_SCAN2AXI, NULL, cmucal_vclk_scan2axi, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_APM, NULL, cmucal_vclk_sysreg_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_APM, NULL, cmucal_vclk_wdt_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUS1, NULL, cmucal_vclk_axi2apb_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUS1_TREX, NULL, cmucal_vclk_axi2apb_bus1_trex, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUS1_CMU_BUS1, NULL, cmucal_vclk_bus1_cmu_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D_FSYS1, NULL, cmucal_vclk_lhm_acel_d_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_ALIVE, NULL, cmucal_vclk_lhm_axi_d_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_GNSS, NULL, cmucal_vclk_lhm_axi_d_gnss, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_ALIVE, NULL, cmucal_vclk_lhs_axi_p_alive, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_FSYS1, NULL, cmucal_vclk_lhs_axi_p_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_BUS1, NULL, cmucal_vclk_pmu_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_BUS1, NULL, cmucal_vclk_sysreg_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_D_BUS1, NULL, cmucal_vclk_trex_d_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P_BUS1, NULL, cmucal_vclk_trex_p_bus1, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADCIF_BUSC, NULL, cmucal_vclk_adcif_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_HSI2CDF, NULL, cmucal_vclk_ad_apb_hsi2cdf, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_PDMA0, NULL, cmucal_vclk_ad_apb_pdma0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SPDMA, NULL, cmucal_vclk_ad_apb_spdma, NULL, NULL),
	CMUCAL_VCLK(VCLK_ASYNCSFR_WR_SCI, NULL, cmucal_vclk_asyncsfr_wr_sci, NULL, NULL),
	CMUCAL_VCLK(VCLK_ASYNCSFR_WR_SMC, NULL, cmucal_vclk_asyncsfr_wr_smc, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUSCP0, NULL, cmucal_vclk_axi2apb_buscp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUSCP1, NULL, cmucal_vclk_axi2apb_buscp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_BUSC_TDP, NULL, cmucal_vclk_axi2apb_busc_tdp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSC_CMU_BUSC, NULL, cmucal_vclk_busc_cmu_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_CMUTOPC, NULL, cmucal_vclk_busif_cmutopc, NULL, NULL),
	CMUCAL_VCLK(VCLK_GNSSMBOX, NULL, cmucal_vclk_gnssmbox, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_BUSC, NULL, cmucal_vclk_gpio_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_HSI2CDF, NULL, cmucal_vclk_hsi2cdf, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D0_G2D, NULL, cmucal_vclk_lhm_acel_d0_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D1_G2D, NULL, cmucal_vclk_lhm_acel_d1_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D2_G2D, NULL, cmucal_vclk_lhm_acel_d2_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D_DSP, NULL, cmucal_vclk_lhm_acel_d_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D_FSYS0, NULL, cmucal_vclk_lhm_acel_d_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D_IVA, NULL, cmucal_vclk_lhm_acel_d_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACEL_D_VPU, NULL, cmucal_vclk_lhm_acel_d_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D0_CAM, NULL, cmucal_vclk_lhm_axi_d0_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D0_DPU, NULL, cmucal_vclk_lhm_axi_d0_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D0_MFC, NULL, cmucal_vclk_lhm_axi_d0_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D1_CAM, NULL, cmucal_vclk_lhm_axi_d1_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D1_DPU, NULL, cmucal_vclk_lhm_axi_d1_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D1_MFC, NULL, cmucal_vclk_lhm_axi_d1_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D2_DPU, NULL, cmucal_vclk_lhm_axi_d2_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_ABOX, NULL, cmucal_vclk_lhm_axi_d_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_ISPLP, NULL, cmucal_vclk_lhm_axi_d_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_SRDZ, NULL, cmucal_vclk_lhm_axi_d_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_VTS, NULL, cmucal_vclk_lhm_axi_d_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_G_CSSYS, NULL, cmucal_vclk_lhm_axi_g_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_IVASC, NULL, cmucal_vclk_lhs_axi_d_ivasc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P0_DPU, NULL, cmucal_vclk_lhs_axi_p0_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P1_DPU, NULL, cmucal_vclk_lhs_axi_p1_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_ABOX, NULL, cmucal_vclk_lhs_axi_p_abox, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CAM, NULL, cmucal_vclk_lhs_axi_p_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_DSP, NULL, cmucal_vclk_lhs_axi_p_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_FSYS0, NULL, cmucal_vclk_lhs_axi_p_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_G2D, NULL, cmucal_vclk_lhs_axi_p_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_ISPHQ, NULL, cmucal_vclk_lhs_axi_p_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_ISPLP, NULL, cmucal_vclk_lhs_axi_p_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_IVA, NULL, cmucal_vclk_lhs_axi_p_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_MFC, NULL, cmucal_vclk_lhs_axi_p_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_MIF0, NULL, cmucal_vclk_lhs_axi_p_mif0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_MIF1, NULL, cmucal_vclk_lhs_axi_p_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_MIF2, NULL, cmucal_vclk_lhs_axi_p_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_MIF3, NULL, cmucal_vclk_lhs_axi_p_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_PERIC0, NULL, cmucal_vclk_lhs_axi_p_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_PERIC1, NULL, cmucal_vclk_lhs_axi_p_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_PERIS, NULL, cmucal_vclk_lhs_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_SRDZ, NULL, cmucal_vclk_lhs_axi_p_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_VPU, NULL, cmucal_vclk_lhs_axi_p_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_VTS, NULL, cmucal_vclk_lhs_axi_p_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_MBOX, NULL, cmucal_vclk_mbox, NULL, NULL),
	CMUCAL_VCLK(VCLK_PDMA0, NULL, cmucal_vclk_pdma0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_BUSC, NULL, cmucal_vclk_pmu_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_SECMBOX, NULL, cmucal_vclk_secmbox, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPDMA, NULL, cmucal_vclk_spdma, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY, NULL, cmucal_vclk_speedy, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY_BATCHER_WRAP, NULL, cmucal_vclk_speedy_batcher_wrap, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_BUSC, NULL, cmucal_vclk_sysreg_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_D_BUSC, NULL, cmucal_vclk_trex_d_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P_BUSC, NULL, cmucal_vclk_trex_p_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_CAMD0, NULL, cmucal_vclk_btm_camd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_CAMD1, NULL, cmucal_vclk_btm_camd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CAM_CMU_CAM, NULL, cmucal_vclk_cam_cmu_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_ISP_EWGEN_CAM, NULL, cmucal_vclk_isp_ewgen_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_IS_CAM, NULL, cmucal_vclk_is_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_SRDZCAM, NULL, cmucal_vclk_lhm_atb_srdzcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CAM, NULL, cmucal_vclk_lhm_axi_p_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D0_CAM, NULL, cmucal_vclk_lhs_axi_d0_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D1_CAM, NULL, cmucal_vclk_lhs_axi_d1_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_CAM, NULL, cmucal_vclk_pmu_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CAM, NULL, cmucal_vclk_sysreg_cam, NULL, NULL),
	CMUCAL_VCLK(VCLK_ADM_APB_G_BDU, NULL, cmucal_vclk_adm_apb_g_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_CCI, NULL, cmucal_vclk_apbbr_cci, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CORE, NULL, cmucal_vclk_axi2apb_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CORE_TP, NULL, cmucal_vclk_axi2apb_core_tp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BDU, NULL, cmucal_vclk_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMCORE, NULL, cmucal_vclk_busif_hpmcore, NULL, NULL),
	CMUCAL_VCLK(VCLK_CCI, NULL, cmucal_vclk_cci, NULL, NULL),
	CMUCAL_VCLK(VCLK_CORE_CMU_CORE, NULL, cmucal_vclk_core_cmu_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CORE, NULL, cmucal_vclk_hpm_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACE_D0_G3D, NULL, cmucal_vclk_lhm_ace_d0_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACE_D1_G3D, NULL, cmucal_vclk_lhm_ace_d1_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACE_D2_G3D, NULL, cmucal_vclk_lhm_ace_d2_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACE_D3_G3D, NULL, cmucal_vclk_lhm_ace_d3_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ACE_D_CPUCL1, NULL, cmucal_vclk_lhm_ace_d_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_CP, NULL, cmucal_vclk_lhm_axi_d_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CP, NULL, cmucal_vclk_lhm_axi_p_cp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_CPACE_D_CPUCL0, NULL, cmucal_vclk_lhm_cpace_d_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_DBG_G0_DMC, NULL, cmucal_vclk_lhm_dbg_g0_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_DBG_G1_DMC, NULL, cmucal_vclk_lhm_dbg_g1_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_DBG_G2_DMC, NULL, cmucal_vclk_lhm_dbg_g2_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_DBG_G3_DMC, NULL, cmucal_vclk_lhm_dbg_g3_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_T_BDU, NULL, cmucal_vclk_lhs_atb_t_bdu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CPUCL0, NULL, cmucal_vclk_lhs_axi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_CPUCL1, NULL, cmucal_vclk_lhs_axi_p_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_DBG, NULL, cmucal_vclk_lhs_axi_p_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_G3D, NULL, cmucal_vclk_lhs_axi_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_IMEM, NULL, cmucal_vclk_lhs_axi_p_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_PSCDC_D_MIF0, NULL, cmucal_vclk_lhs_pscdc_d_mif0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_PSCDC_D_MIF1, NULL, cmucal_vclk_lhs_pscdc_d_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_PSCDC_D_MIF2, NULL, cmucal_vclk_lhs_pscdc_d_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_PSCDC_D_MIF3, NULL, cmucal_vclk_lhs_pscdc_d_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_MPACE2AXI_0, NULL, cmucal_vclk_mpace2axi_0, NULL, NULL),
	CMUCAL_VCLK(VCLK_MPACE2AXI_1, NULL, cmucal_vclk_mpace2axi_1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_CORE, NULL, cmucal_vclk_pmu_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_PPCFW_G3D, NULL, cmucal_vclk_ppcfw_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_CCI, NULL, cmucal_vclk_bcmppc_cci, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_CPUCL0, NULL, cmucal_vclk_bcm_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_CPUCL1, NULL, cmucal_vclk_bcm_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G3D0, NULL, cmucal_vclk_bcm_g3d0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G3D1, NULL, cmucal_vclk_bcm_g3d1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G3D2, NULL, cmucal_vclk_bcm_g3d2, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G3D3, NULL, cmucal_vclk_bcm_g3d3, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CORE, NULL, cmucal_vclk_sysreg_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_D_CORE, NULL, cmucal_vclk_trex_d_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P0_CORE, NULL, cmucal_vclk_trex_p0_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_TREX_P1_CORE, NULL, cmucal_vclk_trex_p1_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CPUCL0, NULL, cmucal_vclk_axi2apb_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_DROOPDETECTOR_CPUCL0, NULL, cmucal_vclk_busif_droopdetector_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMCPUCL0, NULL, cmucal_vclk_busif_hpmcpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CPUCL0_CMU_CPUCL0, NULL, cmucal_vclk_cpucl0_cmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DROOP_DETECTOR_CPUCL0_GRP0, NULL, cmucal_vclk_droop_detector_cpucl0_grp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DROOP_DETECTOR_CPUCL0_GRP1, NULL, cmucal_vclk_droop_detector_cpucl0_grp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CPUCL0, NULL, cmucal_vclk_hpm_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CPUCL0, NULL, cmucal_vclk_lhm_axi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_CPUCL0, NULL, cmucal_vclk_pmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CPUCL0, NULL, cmucal_vclk_sysreg_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CPUCL1, NULL, cmucal_vclk_axi2apb_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMCPUCL1, NULL, cmucal_vclk_busif_hpmcpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CPUCL1_CMU_CPUCL1, NULL, cmucal_vclk_cpucl1_cmu_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_CPUCL1, NULL, cmucal_vclk_hpm_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_CPUCL1, NULL, cmucal_vclk_lhm_axi_p_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_CPUCL1, NULL, cmucal_vclk_pmu_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_CPUCL1, NULL, cmucal_vclk_sysreg_cpucl1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_CORESIGHT, NULL, cmucal_vclk_axi2apb_coresight, NULL, NULL),
	CMUCAL_VCLK(VCLK_CSSYS, NULL, cmucal_vclk_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_DBG_CMU_DBG, NULL, cmucal_vclk_dbg_cmu_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_DUMPPC_CPUCL0, NULL, cmucal_vclk_dumppc_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DUMPPC_CPUCL1, NULL, cmucal_vclk_dumppc_cpucl1, NULL, NULL),
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
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DBG, NULL, cmucal_vclk_lhm_axi_p_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G_CSSYS, NULL, cmucal_vclk_lhs_axi_g_cssys, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G_ETR, NULL, cmucal_vclk_lhs_axi_g_etr, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_DBG, NULL, cmucal_vclk_pmu_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_SECJTAG, NULL, cmucal_vclk_secjtag, NULL, NULL),
	CMUCAL_VCLK(VCLK_STM_TXACTOR, NULL, cmucal_vclk_stm_txactor, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DBG, NULL, cmucal_vclk_sysreg_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DCAM, NULL, cmucal_vclk_ad_apb_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DCAM, NULL, cmucal_vclk_axi2apb_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DCAM_SYS, NULL, cmucal_vclk_axi2apb_dcam_sys, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DCAM, NULL, cmucal_vclk_btm_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_DCAM_CMU_DCAM, NULL, cmucal_vclk_dcam_cmu_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_DCP, NULL, cmucal_vclk_dcp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_SRDZDCAM, NULL, cmucal_vclk_lhm_axi_p_srdzdcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_DCAMSRDZ, NULL, cmucal_vclk_lhs_atb_dcamsrdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_DCAMSRDZ, NULL, cmucal_vclk_lhs_axi_d_dcamsrdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_DCAM, NULL, cmucal_vclk_pmu_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_DCAM, NULL, cmucal_vclk_bcm_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_PXL_ASBM_DCAM, NULL, cmucal_vclk_pxl_asbm_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_PXL_ASBS_DCAM, NULL, cmucal_vclk_pxl_asbs_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DCAM, NULL, cmucal_vclk_sysreg_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_DCAM, NULL, cmucal_vclk_xiu_p_dcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON0, NULL, cmucal_vclk_ad_apb_decon0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON0_SECURE, NULL, cmucal_vclk_ad_apb_decon0_secure, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPP, NULL, cmucal_vclk_ad_apb_dpp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPP_SECURE, NULL, cmucal_vclk_ad_apb_dpp_secure, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPU_DMA, NULL, cmucal_vclk_ad_apb_dpu_dma, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPU_DMA_SECURE, NULL, cmucal_vclk_ad_apb_dpu_dma_secure, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DPU_WB_MUX, NULL, cmucal_vclk_ad_apb_dpu_wb_mux, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_MIPI_DSIM0, NULL, cmucal_vclk_ad_apb_mipi_dsim0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_MIPI_DSIM1, NULL, cmucal_vclk_ad_apb_mipi_dsim1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DPU0P0, NULL, cmucal_vclk_axi2apb_dpu0p0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DPU0P1, NULL, cmucal_vclk_axi2apb_dpu0p1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DPUD0, NULL, cmucal_vclk_btm_dpud0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DPUD1, NULL, cmucal_vclk_btm_dpud1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_DPUD2, NULL, cmucal_vclk_btm_dpud2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DECON0, NULL, cmucal_vclk_decon0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DPP, NULL, cmucal_vclk_dpp, NULL, NULL),
	CMUCAL_VCLK(VCLK_DPU0_CMU_DPU0, NULL, cmucal_vclk_dpu0_cmu_dpu0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DPU_DMA, NULL, cmucal_vclk_dpu_dma, NULL, NULL),
	CMUCAL_VCLK(VCLK_DPU_WB_MUX, NULL, cmucal_vclk_dpu_wb_mux, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P0_DPU, NULL, cmucal_vclk_lhm_axi_p0_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D0_DPU, NULL, cmucal_vclk_lhs_axi_d0_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D1_DPU, NULL, cmucal_vclk_lhs_axi_d1_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D2_DPU, NULL, cmucal_vclk_lhs_axi_d2_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_USBTV, NULL, cmucal_vclk_lhs_axi_d_usbtv, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_DPU0, NULL, cmucal_vclk_pmu_dpu0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_DPUD0, NULL, cmucal_vclk_bcm_dpud0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_DPUD1, NULL, cmucal_vclk_bcm_dpud1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_DPUD2, NULL, cmucal_vclk_bcm_dpud2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSMMU_DPUD0, NULL, cmucal_vclk_sysmmu_dpud0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSMMU_DPUD1, NULL, cmucal_vclk_sysmmu_dpud1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSMMU_DPUD2, NULL, cmucal_vclk_sysmmu_dpud2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DPU0, NULL, cmucal_vclk_sysreg_dpu0, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P0_DPU, NULL, cmucal_vclk_xiu_p0_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON1, NULL, cmucal_vclk_ad_apb_decon1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_DECON2, NULL, cmucal_vclk_ad_apb_decon2, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DPU1P0, NULL, cmucal_vclk_axi2apb_dpu1p0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DECON1, NULL, cmucal_vclk_decon1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DECON2, NULL, cmucal_vclk_decon2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DPU1_CMU_DPU1, NULL, cmucal_vclk_dpu1_cmu_dpu1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DPU1, NULL, cmucal_vclk_lhm_axi_p_dpu1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_DPU1, NULL, cmucal_vclk_pmu_dpu1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DPU1, NULL, cmucal_vclk_sysreg_dpu1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SCORE, NULL, cmucal_vclk_ad_apb_score, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_DSP, NULL, cmucal_vclk_axi2apb_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_SCORE, NULL, cmucal_vclk_btm_score, NULL, NULL),
	CMUCAL_VCLK(VCLK_DSP_CMU_DSP, NULL, cmucal_vclk_dsp_cmu_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_IVADSP, NULL, cmucal_vclk_lhm_axi_d_ivadsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_VPUDSP, NULL, cmucal_vclk_lhm_axi_d_vpudsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DSP, NULL, cmucal_vclk_lhm_axi_p_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_IVADSP, NULL, cmucal_vclk_lhm_axi_p_ivadsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D_DSP, NULL, cmucal_vclk_lhs_acel_d_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_DSPIVA, NULL, cmucal_vclk_lhs_axi_p_dspiva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_DSPVPU, NULL, cmucal_vclk_lhs_axi_p_dspvpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_DSP, NULL, cmucal_vclk_pmu_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_SCORE, NULL, cmucal_vclk_bcm_score, NULL, NULL),
	CMUCAL_VCLK(VCLK_SCORE, NULL, cmucal_vclk_score, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_SCORE, NULL, cmucal_vclk_smmu_score, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_DSP, NULL, cmucal_vclk_sysreg_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_WRAP2_CONV_DSP, NULL, cmucal_vclk_wrap2_conv_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_DSP0, NULL, cmucal_vclk_xiu_d_dsp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_DSP1, NULL, cmucal_vclk_xiu_d_dsp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_DSP, NULL, cmucal_vclk_xiu_p_dsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AHBBR_FSYS0, NULL, cmucal_vclk_ahbbr_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2AHB_FSYS0, NULL, cmucal_vclk_axi2ahb_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2AHB_USB_FSYS0, NULL, cmucal_vclk_axi2ahb_usb_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_FSYS0, NULL, cmucal_vclk_axi2apb_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_FSYS0, NULL, cmucal_vclk_btm_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DP_LINK, NULL, cmucal_vclk_dp_link, NULL, NULL),
	CMUCAL_VCLK(VCLK_ETR_MIU, NULL, cmucal_vclk_etr_miu, NULL, NULL),
	CMUCAL_VCLK(VCLK_FSYS0_CMU_FSYS0, NULL, cmucal_vclk_fsys0_cmu_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_FSYS0, NULL, cmucal_vclk_gpio_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_USBTV, NULL, cmucal_vclk_lhm_axi_d_usbtv, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_G_ETR, NULL, cmucal_vclk_lhm_axi_g_etr, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_FSYS0, NULL, cmucal_vclk_lhm_axi_p_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D_FSYS0, NULL, cmucal_vclk_lhs_acel_d_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_MMC_EMBD, NULL, cmucal_vclk_mmc_embd, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_FSYS0, NULL, cmucal_vclk_pmu_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_FSYS0, NULL, cmucal_vclk_bcm_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_FSYS0, NULL, cmucal_vclk_sysreg_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_UFS_EMBD, NULL, cmucal_vclk_ufs_embd, NULL, NULL),
	CMUCAL_VCLK(VCLK_USBTV, NULL, cmucal_vclk_usbtv, NULL, NULL),
	CMUCAL_VCLK(VCLK_US_D_FSYS0_USB, NULL, cmucal_vclk_us_d_fsys0_usb, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_FSYS0, NULL, cmucal_vclk_xiu_d_fsys0, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_FSYS0_USB, NULL, cmucal_vclk_xiu_d_fsys0_usb, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_FSYS0, NULL, cmucal_vclk_xiu_p_fsys0, NULL, NULL),
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
	CMUCAL_VCLK(VCLK_PCIE, NULL, cmucal_vclk_pcie, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_FSYS1, NULL, cmucal_vclk_pmu_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_FSYS1, NULL, cmucal_vclk_bcm_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_RTIC, NULL, cmucal_vclk_rtic, NULL, NULL),
	CMUCAL_VCLK(VCLK_SSS, NULL, cmucal_vclk_sss, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_FSYS1, NULL, cmucal_vclk_sysreg_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_TOE_WIFI0, NULL, cmucal_vclk_toe_wifi0, NULL, NULL),
	CMUCAL_VCLK(VCLK_TOE_WIFI1, NULL, cmucal_vclk_toe_wifi1, NULL, NULL),
	CMUCAL_VCLK(VCLK_UFS_CARD, NULL, cmucal_vclk_ufs_card, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_FSYS1, NULL, cmucal_vclk_xiu_d_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_FSYS1, NULL, cmucal_vclk_xiu_p_fsys1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AS_P_G2DP, NULL, cmucal_vclk_as_p_g2dp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AS_P_JPEGP, NULL, cmucal_vclk_as_p_jpegp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AS_P_M2MSCALERP, NULL, cmucal_vclk_as_p_m2mscalerp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_G2DP0, NULL, cmucal_vclk_axi2apb_g2dp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_G2DP1, NULL, cmucal_vclk_axi2apb_g2dp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_G2DD0, NULL, cmucal_vclk_btm_g2dd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_G2DD1, NULL, cmucal_vclk_btm_g2dd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_G2DD2, NULL, cmucal_vclk_btm_g2dd2, NULL, NULL),
	CMUCAL_VCLK(VCLK_G2D, NULL, cmucal_vclk_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_G2D_CMU_G2D, NULL, cmucal_vclk_g2d_cmu_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_JPEG, NULL, cmucal_vclk_jpeg, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_G2D, NULL, cmucal_vclk_lhm_axi_p_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D0_G2D, NULL, cmucal_vclk_lhs_acel_d0_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D1_G2D, NULL, cmucal_vclk_lhs_acel_d1_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D2_G2D, NULL, cmucal_vclk_lhs_acel_d2_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_M2MSCALER, NULL, cmucal_vclk_m2mscaler, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_G2D, NULL, cmucal_vclk_pmu_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G2DD0, NULL, cmucal_vclk_bcm_g2dd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G2DD1, NULL, cmucal_vclk_bcm_g2dd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_G2DD2, NULL, cmucal_vclk_bcm_g2dd2, NULL, NULL),
	CMUCAL_VCLK(VCLK_QE_JPEG, NULL, cmucal_vclk_qe_jpeg, NULL, NULL),
	CMUCAL_VCLK(VCLK_QE_M2MSCALER, NULL, cmucal_vclk_qe_m2mscaler, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_G2DD0, NULL, cmucal_vclk_smmu_g2dd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_G2DD1, NULL, cmucal_vclk_smmu_g2dd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_G2DD2, NULL, cmucal_vclk_smmu_g2dd2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_G2D, NULL, cmucal_vclk_sysreg_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_G2D, NULL, cmucal_vclk_xiu_d_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_G2D, NULL, cmucal_vclk_xiu_p_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_G3D, NULL, cmucal_vclk_axi2apb_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMG3D, NULL, cmucal_vclk_busif_hpmg3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_G3D_CMU_G3D, NULL, cmucal_vclk_g3d_cmu_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_G3D, NULL, cmucal_vclk_hpm_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_G3D, NULL, cmucal_vclk_lhm_axi_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_G3DSFR, NULL, cmucal_vclk_lhs_axi_g3dsfr, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_G3D, NULL, cmucal_vclk_pmu_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_G3D, NULL, cmucal_vclk_sysreg_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_G3D, NULL, cmucal_vclk_xiu_p_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_IMEM, NULL, cmucal_vclk_axi2apb_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IMEM_CMU_IMEM, NULL, cmucal_vclk_imem_cmu_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_IntMEM, NULL, cmucal_vclk_intmem, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_IMEM, NULL, cmucal_vclk_lhm_axi_p_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_IMEM, NULL, cmucal_vclk_pmu_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_IMEM, NULL, cmucal_vclk_sysreg_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_IMEM, NULL, cmucal_vclk_xiu_p_imem, NULL, NULL),
	CMUCAL_VCLK(VCLK_ISPHQ_CMU_ISPHQ, NULL, cmucal_vclk_isphq_cmu_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_ISP_EWGEN_ISPHQ, NULL, cmucal_vclk_isp_ewgen_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_IS_ISPHQ, NULL, cmucal_vclk_is_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_ISPHQ, NULL, cmucal_vclk_lhm_axi_p_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_LD_ISPHQ, NULL, cmucal_vclk_lhs_axi_ld_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_ISPHQ, NULL, cmucal_vclk_pmu_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_ISPHQ, NULL, cmucal_vclk_sysreg_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_ISPLP, NULL, cmucal_vclk_btm_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_ISPLP_CMU_ISPLP, NULL, cmucal_vclk_isplp_cmu_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_ISP_EWGEN_ISPLP, NULL, cmucal_vclk_isp_ewgen_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_IS_ISPLP, NULL, cmucal_vclk_is_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_LD_ISPHQ, NULL, cmucal_vclk_lhm_axi_ld_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_ISPLP, NULL, cmucal_vclk_lhm_axi_p_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_ISPLP, NULL, cmucal_vclk_lhs_axi_d_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_ISPLP, NULL, cmucal_vclk_pmu_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_ISPLP, NULL, cmucal_vclk_sysreg_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_IVA, NULL, cmucal_vclk_ad_apb_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_2M_IVA, NULL, cmucal_vclk_axi2apb_2m_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_IVA, NULL, cmucal_vclk_axi2apb_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_IVA, NULL, cmucal_vclk_btm_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_IVA, NULL, cmucal_vclk_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_IVA_CMU_IVA, NULL, cmucal_vclk_iva_cmu_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_IVA_IntMEM, NULL, cmucal_vclk_iva_intmem, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_IVASC, NULL, cmucal_vclk_lhm_axi_d_ivasc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DSPIVA, NULL, cmucal_vclk_lhm_axi_p_dspiva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_IVA, NULL, cmucal_vclk_lhm_axi_p_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D_IVA, NULL, cmucal_vclk_lhs_acel_d_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_IVADSP, NULL, cmucal_vclk_lhs_axi_d_ivadsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_IVADSP, NULL, cmucal_vclk_lhs_axi_p_ivadsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_IVA, NULL, cmucal_vclk_pmu_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_IVA, NULL, cmucal_vclk_bcm_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_IVA, NULL, cmucal_vclk_smmu_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_IVA, NULL, cmucal_vclk_sysreg_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_IVA, NULL, cmucal_vclk_xiu_d_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_IVA, NULL, cmucal_vclk_xiu_p_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_AS_P_MFCP, NULL, cmucal_vclk_as_p_mfcp, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_MFC, NULL, cmucal_vclk_axi2apb_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_MFCD0, NULL, cmucal_vclk_btm_mfcd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_MFCD1, NULL, cmucal_vclk_btm_mfcd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_MFC, NULL, cmucal_vclk_lhm_axi_p_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D0_MFC, NULL, cmucal_vclk_lhs_axi_d0_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D1_MFC, NULL, cmucal_vclk_lhs_axi_d1_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_MFC, NULL, cmucal_vclk_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_MFC_CMU_MFC, NULL, cmucal_vclk_mfc_cmu_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_MFC, NULL, cmucal_vclk_pmu_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_MFCD0, NULL, cmucal_vclk_bcm_mfcd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_MFCD1, NULL, cmucal_vclk_bcm_mfcd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_MFCD0, NULL, cmucal_vclk_smmu_mfcd0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_MFCD1, NULL, cmucal_vclk_smmu_mfcd1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_MFC, NULL, cmucal_vclk_sysreg_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DDRPHY, NULL, cmucal_vclk_apbbr_ddrphy, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMC, NULL, cmucal_vclk_apbbr_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMCTZ, NULL, cmucal_vclk_apbbr_dmctz, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_MIF, NULL, cmucal_vclk_axi2apb_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMMIF, NULL, cmucal_vclk_busif_hpmmif, NULL, NULL),
	CMUCAL_VCLK(VCLK_DDRPHY, NULL, cmucal_vclk_ddrphy, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMC, NULL, cmucal_vclk_dmc, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_MIF, NULL, cmucal_vclk_hpm_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_MIF, NULL, cmucal_vclk_lhm_axi_p_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_PSCDC_D_MIF, NULL, cmucal_vclk_lhm_pscdc_d_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_MIF_CMU_MIF, NULL, cmucal_vclk_mif_cmu_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_MIF, NULL, cmucal_vclk_pmu_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DEBUG, NULL, cmucal_vclk_bcmppc_debug, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DVFS, NULL, cmucal_vclk_bcmppc_dvfs, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_MIF, NULL, cmucal_vclk_sysreg_mif, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DDRPHY1, NULL, cmucal_vclk_apbbr_ddrphy1, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMC1, NULL, cmucal_vclk_apbbr_dmc1, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMCTZ1, NULL, cmucal_vclk_apbbr_dmctz1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_MIF1, NULL, cmucal_vclk_axi2apb_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMMIF1, NULL, cmucal_vclk_busif_hpmmif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DDRPHY1, NULL, cmucal_vclk_ddrphy1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMC1, NULL, cmucal_vclk_dmc1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_MIF1, NULL, cmucal_vclk_hpm_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_MIF1, NULL, cmucal_vclk_lhm_axi_p_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_PSCDC_D_MIF1, NULL, cmucal_vclk_lhm_pscdc_d_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_MIF1_CMU_MIF1, NULL, cmucal_vclk_mif1_cmu_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_MIF1, NULL, cmucal_vclk_pmu_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DEBUG1, NULL, cmucal_vclk_bcmppc_debug1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DVFS1, NULL, cmucal_vclk_bcmppc_dvfs1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_MIF1, NULL, cmucal_vclk_sysreg_mif1, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DDRPHY2, NULL, cmucal_vclk_apbbr_ddrphy2, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMC2, NULL, cmucal_vclk_apbbr_dmc2, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMCTZ2, NULL, cmucal_vclk_apbbr_dmctz2, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_MIF2, NULL, cmucal_vclk_axi2apb_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMMIF2, NULL, cmucal_vclk_busif_hpmmif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DDRPHY2, NULL, cmucal_vclk_ddrphy2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMC2, NULL, cmucal_vclk_dmc2, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_MIF2, NULL, cmucal_vclk_hpm_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_MIF2, NULL, cmucal_vclk_lhm_axi_p_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_PSCDC_D_MIF2, NULL, cmucal_vclk_lhm_pscdc_d_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_MIF2_CMU_MIF2, NULL, cmucal_vclk_mif2_cmu_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_MIF2, NULL, cmucal_vclk_pmu_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DEBUG2, NULL, cmucal_vclk_bcmppc_debug2, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DVFS2, NULL, cmucal_vclk_bcmppc_dvfs2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_MIF2, NULL, cmucal_vclk_sysreg_mif2, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DDRPHY3, NULL, cmucal_vclk_apbbr_ddrphy3, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMC3, NULL, cmucal_vclk_apbbr_dmc3, NULL, NULL),
	CMUCAL_VCLK(VCLK_APBBR_DMCTZ3, NULL, cmucal_vclk_apbbr_dmctz3, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_MIF3, NULL, cmucal_vclk_axi2apb_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_HPMMIF3, NULL, cmucal_vclk_busif_hpmmif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_DDRPHY3, NULL, cmucal_vclk_ddrphy3, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMC3, NULL, cmucal_vclk_dmc3, NULL, NULL),
	CMUCAL_VCLK(VCLK_HPM_MIF3, NULL, cmucal_vclk_hpm_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_MIF3, NULL, cmucal_vclk_lhm_axi_p_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_PSCDC_D_MIF3, NULL, cmucal_vclk_lhm_pscdc_d_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_MIF3_CMU_MIF3, NULL, cmucal_vclk_mif3_cmu_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_MIF3, NULL, cmucal_vclk_pmu_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DEBUG3, NULL, cmucal_vclk_bcmppc_debug3, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCMPPC_DVFS3, NULL, cmucal_vclk_bcmppc_dvfs3, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_MIF3, NULL, cmucal_vclk_sysreg_mif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC0, NULL, cmucal_vclk_axi2apb_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_PERIC0, NULL, cmucal_vclk_gpio_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_PERIC0, NULL, cmucal_vclk_lhm_axi_p_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERIC0_CMU_PERIC0, NULL, cmucal_vclk_peric0_cmu_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_PERIC0, NULL, cmucal_vclk_pmu_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_PWM, NULL, cmucal_vclk_pwm, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY2_TSP, NULL, cmucal_vclk_speedy2_tsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_PERIC0, NULL, cmucal_vclk_sysreg_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_UART_DBG, NULL, cmucal_vclk_uart_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI00, NULL, cmucal_vclk_usi00, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI01, NULL, cmucal_vclk_usi01, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI02, NULL, cmucal_vclk_usi02, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI03, NULL, cmucal_vclk_usi03, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC1P0, NULL, cmucal_vclk_axi2apb_peric1p0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC1P1, NULL, cmucal_vclk_axi2apb_peric1p1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERIC1P2, NULL, cmucal_vclk_axi2apb_peric1p2, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_PERIC1, NULL, cmucal_vclk_gpio_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HSI2C_CAM0, NULL, cmucal_vclk_hsi2c_cam0, NULL, NULL),
	CMUCAL_VCLK(VCLK_HSI2C_CAM1, NULL, cmucal_vclk_hsi2c_cam1, NULL, NULL),
	CMUCAL_VCLK(VCLK_HSI2C_CAM2, NULL, cmucal_vclk_hsi2c_cam2, NULL, NULL),
	CMUCAL_VCLK(VCLK_HSI2C_CAM3, NULL, cmucal_vclk_hsi2c_cam3, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_PERIC1, NULL, cmucal_vclk_lhm_axi_p_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERIC1_CMU_PERIC1, NULL, cmucal_vclk_peric1_cmu_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_PERIC1, NULL, cmucal_vclk_pmu_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY2_DDI, NULL, cmucal_vclk_speedy2_ddi, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY2_DDI1, NULL, cmucal_vclk_speedy2_ddi1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY2_DDI2, NULL, cmucal_vclk_speedy2_ddi2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY2_TSP1, NULL, cmucal_vclk_speedy2_tsp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPEEDY2_TSP2, NULL, cmucal_vclk_speedy2_tsp2, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPI_CAM0, NULL, cmucal_vclk_spi_cam0, NULL, NULL),
	CMUCAL_VCLK(VCLK_SPI_CAM1, NULL, cmucal_vclk_spi_cam1, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_PERIC1, NULL, cmucal_vclk_sysreg_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_UART_BT, NULL, cmucal_vclk_uart_bt, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI04, NULL, cmucal_vclk_usi04, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI05, NULL, cmucal_vclk_usi05, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI06, NULL, cmucal_vclk_usi06, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI07, NULL, cmucal_vclk_usi07, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI08, NULL, cmucal_vclk_usi08, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI09, NULL, cmucal_vclk_usi09, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI10, NULL, cmucal_vclk_usi10, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI11, NULL, cmucal_vclk_usi11, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI12, NULL, cmucal_vclk_usi12, NULL, NULL),
	CMUCAL_VCLK(VCLK_USI13, NULL, cmucal_vclk_usi13, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_PERIC1, NULL, cmucal_vclk_xiu_p_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_AXI_P_PERIS, NULL, cmucal_vclk_ad_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERISP0, NULL, cmucal_vclk_axi2apb_perisp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_PERISP1, NULL, cmucal_vclk_axi2apb_perisp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BUSIF_TMU, NULL, cmucal_vclk_busif_tmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_GIC, NULL, cmucal_vclk_gic, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_PERIS, NULL, cmucal_vclk_lhm_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_MCT, NULL, cmucal_vclk_mct, NULL, NULL),
	CMUCAL_VCLK(VCLK_OTP_CON_BIRA, NULL, cmucal_vclk_otp_con_bira, NULL, NULL),
	CMUCAL_VCLK(VCLK_OTP_CON_TOP, NULL, cmucal_vclk_otp_con_top, NULL, NULL),
	CMUCAL_VCLK(VCLK_PERIS_CMU_PERIS, NULL, cmucal_vclk_peris_cmu_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_PERIS, NULL, cmucal_vclk_pmu_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_PERIS, NULL, cmucal_vclk_sysreg_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC00, NULL, cmucal_vclk_tzpc00, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC01, NULL, cmucal_vclk_tzpc01, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC02, NULL, cmucal_vclk_tzpc02, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC03, NULL, cmucal_vclk_tzpc03, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC04, NULL, cmucal_vclk_tzpc04, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC05, NULL, cmucal_vclk_tzpc05, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC06, NULL, cmucal_vclk_tzpc06, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC07, NULL, cmucal_vclk_tzpc07, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC08, NULL, cmucal_vclk_tzpc08, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC09, NULL, cmucal_vclk_tzpc09, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC10, NULL, cmucal_vclk_tzpc10, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC11, NULL, cmucal_vclk_tzpc11, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC12, NULL, cmucal_vclk_tzpc12, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC13, NULL, cmucal_vclk_tzpc13, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC14, NULL, cmucal_vclk_tzpc14, NULL, NULL),
	CMUCAL_VCLK(VCLK_TZPC15, NULL, cmucal_vclk_tzpc15, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_CLUSTER0, NULL, cmucal_vclk_wdt_cluster0, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_CLUSTER1, NULL, cmucal_vclk_wdt_cluster1, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_PERIS, NULL, cmucal_vclk_xiu_p_peris, NULL, NULL),
	CMUCAL_VCLK(VCLK_AD_APB_SRDZ, NULL, cmucal_vclk_ad_apb_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_SRDZ, NULL, cmucal_vclk_axi2apb_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_SRDZ_SYS, NULL, cmucal_vclk_axi2apb_srdz_sys, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_SRDZ, NULL, cmucal_vclk_btm_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_ATB_DCAMSRDZ, NULL, cmucal_vclk_lhm_atb_dcamsrdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_D_DCAMSRDZ, NULL, cmucal_vclk_lhm_axi_d_dcamsrdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_SRDZ, NULL, cmucal_vclk_lhm_axi_p_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ATB_SRDZCAM, NULL, cmucal_vclk_lhs_atb_srdzcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_SRDZ, NULL, cmucal_vclk_lhs_axi_d_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_P_SRDZDCAM, NULL, cmucal_vclk_lhs_axi_p_srdzdcam, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_SRDZ, NULL, cmucal_vclk_pmu_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_SRDZ, NULL, cmucal_vclk_bcm_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_PXL_ASBM_1to2_SRDZ, NULL, cmucal_vclk_pxl_asbm_1to2_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_PXL_ASBS_SRDZ, NULL, cmucal_vclk_pxl_asbs_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_SRDZ, NULL, cmucal_vclk_smmu_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_SRDZ, NULL, cmucal_vclk_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_SRDZ_CMU_SRDZ, NULL, cmucal_vclk_srdz_cmu_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_SRDZ, NULL, cmucal_vclk_sysreg_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_SRDZ, NULL, cmucal_vclk_xiu_d_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_SRDZ, NULL, cmucal_vclk_xiu_p_srdz, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2AHB_VPU, NULL, cmucal_vclk_axi2ahb_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_AXI2APB_VPU, NULL, cmucal_vclk_axi2apb_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BTM_VPU, NULL, cmucal_vclk_btm_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_DSPVPU, NULL, cmucal_vclk_lhm_axi_p_dspvpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_VPU, NULL, cmucal_vclk_lhm_axi_p_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_ACEL_D_VPU, NULL, cmucal_vclk_lhs_acel_d_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_VPUDSP, NULL, cmucal_vclk_lhs_axi_d_vpudsp, NULL, NULL),
	CMUCAL_VCLK(VCLK_PMU_VPU, NULL, cmucal_vclk_pmu_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BCM_VPU, NULL, cmucal_vclk_bcm_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SMMU_VPU, NULL, cmucal_vclk_smmu_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_VPU, NULL, cmucal_vclk_sysreg_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_VPU, NULL, cmucal_vclk_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_VPU_CMU_VPU, NULL, cmucal_vclk_vpu_cmu_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_D_VPU, NULL, cmucal_vclk_xiu_d_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_XIU_P_VPU, NULL, cmucal_vclk_xiu_p_vpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMIC_AHB, NULL, cmucal_vclk_dmic_ahb, NULL, NULL),
	CMUCAL_VCLK(VCLK_DMIC_IF, NULL, cmucal_vclk_dmic_if, NULL, NULL),
	CMUCAL_VCLK(VCLK_GPIO_VTS, NULL, cmucal_vclk_gpio_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHM_AXI_P_VTS, NULL, cmucal_vclk_lhm_axi_p_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_LHS_AXI_D_VTS, NULL, cmucal_vclk_lhs_axi_d_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_MAILBOX_VTS2AP, NULL, cmucal_vclk_mailbox_vts2ap, NULL, NULL),
	CMUCAL_VCLK(VCLK_SYSREG_VTS, NULL, cmucal_vclk_sysreg_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_VTS, NULL, cmucal_vclk_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_VTS_CMU_VTS, NULL, cmucal_vclk_vts_cmu_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_WDT_VTS, NULL, cmucal_vclk_wdt_vts, NULL, NULL),
};
unsigned int cmucal_vclk_size = 682;

