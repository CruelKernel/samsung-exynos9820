#include "../cmucal.h"
#include "cmucal-node.h"
#include "cmucal-vclk.h"
#include "cmucal-vclklut.h"

/* DVFS VCLK -> Clock Node List */
enum clk_id cmucal_vclk_vddi[] = {
	CLKCMU_MFC_MFC,
	MUX_CLKCMU_MFC_MFC,
	MUX_CLKCMU_HPM,
	CLKCMU_CMU_BOOST,
	MUX_CLKCMU_CMU_BOOST,
	CLKCMU_G3D_SWITCH,
	CLKCMU_BUSC_BUS,
	MUX_CLKCMU_BUSC_BUS,
	CLKCMU_G2D_G2D,
	MUX_CLKCMU_G2D_G2D,
	CLKCMU_DSPM_BUS,
	MUX_CLKCMU_DSPM_BUS,
	CLKCMU_CPUCL0_SWITCH,
	MUX_CLKCMU_CPUCL0_SWITCH,
	CLKCMU_ISPPRE_BUS,
	MUX_CLKCMU_ISPPRE_BUS,
	CLKCMU_ISPLP_BUS,
	MUX_CLKCMU_ISPLP_BUS,
	CLKCMU_ISPHQ_BUS,
	MUX_CLKCMU_ISPHQ_BUS,
	CLKCMU_FSYS0_BUS,
	MUX_CLKCMU_FSYS0_BUS,
	CLKCMU_IVA_BUS,
	MUX_CLKCMU_IVA_BUS,
	DIV_CLK_CMU_CMUREF,
	CLKCMU_NPU0_BUS,
	MUX_CLKCMU_NPU0_BUS,
	CLKCMU_MFC_WFD,
	MUX_CLKCMU_MFC_WFD,
	CLKCMU_NPU1_BUS,
	MUX_CLKCMU_NPU1_BUS,
	CLKCMU_ISPLP_GDC,
	MUX_CLKCMU_ISPLP_GDC,
	CLKCMU_DSPS_AUD,
	MUX_CLKCMU_DSPS_AUD,
	CLKCMU_DSPM_BUS_OTF,
	MUX_CLKCMU_DSPM_BUS_OTF,
	CLKCMU_VRA2_BUS,
	MUX_CLKCMU_VRA2_BUS,
	CLKCMU_DPU_BUS,
	DIV_CLKCMU_DPU,
	MUX_CLKCMU_DPU,
	PLL_G3D,
	MUX_CLKCMU_NPU0_CPU,
	CLKCMU_VRA2_STR,
	MUX_CLKCMU_VRA2_STR,
	DIV_CLK_FSYS1_BUS,
	PLL_MMC,
	CLKCMU_CORE_BUS,
	MUX_CLKCMU_CORE_BUS,
	CLKCMU_G2D_MSCL,
	MUX_CLKCMU_G2D_MSCL,
	CLKCMU_CPUCL0_DBG_BUS,
	MUX_CLKCMU_CPUCL0_DBG_BUS,
	CLKCMU_MIF_BUSP,
	MUX_CLKCMU_MIF_BUSP,
	CLKCMU_FSYS1_UFS_CARD,
	MUX_CLKCMU_FSYS1_UFS_CARD,
	CLKCMU_PERIC0_IP,
	CLKCMU_PERIC1_IP,
	DIV_CLKCMU_DPU_BUS,
	MUX_CLKCMU_DPU_BUS,
};
enum clk_id cmucal_vclk_vdd_mif[] = {
	PLL_MIF,
	PLL_MIF_S2D,
};
enum clk_id cmucal_vclk_vdd_cam[] = {
	DIV_CLK_AUD_PLL,
	DIV_CLK_AUD_UAIF0,
	DIV_CLK_AUD_UAIF1,
	DIV_CLK_AUD_UAIF2,
	DIV_CLK_AUD_UAIF3,
	DIV_CLK_AUD_DMIC,
	DIV_CLK_AUD_CNT,
	DIV_CLK_AUD_MCLK,
	DIV_CLK_AUD_AUDIF,
	DIV_CLK_AUD_BUS,
	PLL_AUD0,
	DIV_CLK_NPU0_CPU,
	DIV_CLK_NPU0_BUSP,
};
enum clk_id cmucal_vclk_vdd_cheetah[] = {
	DIV_CLK_CLUSTER2_ACLK,
	PLL_CPUCL2,
};
enum clk_id cmucal_vclk_vdd_ananke[] = {
	DIV_CLK_CLUSTER0_ACLK,
	PLL_CPUCL0,
};
enum clk_id cmucal_vclk_vdd_prometheus[] = {
	PLL_CPUCL1,
};
/* SPECIAL VCLK -> Clock Node List */
enum clk_id cmucal_vclk_mux_clk_aud_uaif3[] = {
	MUX_CLK_AUD_UAIF3,
	MUX_CLK_AUD_UAIF2,
	MUX_CLK_AUD_UAIF1,
	MUX_CLK_AUD_UAIF0,
	MUX_CLK_AUD_DSIF,
	DIV_CLK_AUD_DSIF,
	PLL_AUD1,
	DIV_CLK_AUD_CPU_PCLKDBG,
	DIV_CLK_AUD_CPU_ACLK,
};
enum clk_id cmucal_vclk_mux_buc_cmuref[] = {
	MUX_BUC_CMUREF,
};
enum clk_id cmucal_vclk_mux_clk_cmgp_adc[] = {
	MUX_CLK_CMGP_ADC,
	DIV_CLK_CMGP_ADC,
};
enum clk_id cmucal_vclk_clkcmu_cmgp_bus[] = {
	CLKCMU_CMGP_BUS,
	MUX_CLKCMU_CMGP_BUS,
};
enum clk_id cmucal_vclk_clkcmu_apm_bus[] = {
	CLKCMU_APM_BUS,
	MUX_CLKCMU_APM_BUS,
};
enum clk_id cmucal_vclk_mux_cmu_cmuref[] = {
	MUX_CMU_CMUREF,
	MUX_CLK_CMU_CMUREF,
};
enum clk_id cmucal_vclk_mux_core_cmuref[] = {
	MUX_CORE_CMUREF,
};
enum clk_id cmucal_vclk_mux_cpucl0_cmuref[] = {
	MUX_CPUCL0_CMUREF,
};
enum clk_id cmucal_vclk_mux_cpucl1_cmuref[] = {
	MUX_CPUCL1_CMUREF,
};
enum clk_id cmucal_vclk_mux_cpucl2_cmuref[] = {
	MUX_CPUCL2_CMUREF,
};
enum clk_id cmucal_vclk_mux_mif_cmuref[] = {
	MUX_MIF_CMUREF,
};
enum clk_id cmucal_vclk_mux_mif1_cmuref[] = {
	MUX_MIF1_CMUREF,
};
enum clk_id cmucal_vclk_mux_mif2_cmuref[] = {
	MUX_MIF2_CMUREF,
};
enum clk_id cmucal_vclk_mux_mif3_cmuref[] = {
	MUX_MIF3_CMUREF,
};
enum clk_id cmucal_vclk_clkcmu_fsys0_dpgtc[] = {
	CLKCMU_FSYS0_DPGTC,
	MUX_CLKCMU_FSYS0_DPGTC,
};
enum clk_id cmucal_vclk_mux_clkcmu_fsys0_pcie[] = {
	MUX_CLKCMU_FSYS0_PCIE,
};
enum clk_id cmucal_vclk_mux_clkcmu_fsys0a_usbdp_debug[] = {
	MUX_CLKCMU_FSYS0A_USBDP_DEBUG,
};
enum clk_id cmucal_vclk_mux_clkcmu_fsys1_pcie[] = {
	MUX_CLKCMU_FSYS1_PCIE,
};
enum clk_id cmucal_vclk_clkcmu_fsys1_ufs_embd[] = {
	CLKCMU_FSYS1_UFS_EMBD,
	MUX_CLKCMU_FSYS1_UFS_EMBD,
};
enum clk_id cmucal_vclk_div_clk_i2c_cmgp0[] = {
	DIV_CLK_I2C_CMGP0,
	MUX_CLK_I2C_CMGP0,
};
enum clk_id cmucal_vclk_div_clk_usi_cmgp1[] = {
	DIV_CLK_USI_CMGP1,
	MUX_CLK_USI_CMGP1,
};
enum clk_id cmucal_vclk_div_clk_usi_cmgp0[] = {
	DIV_CLK_USI_CMGP0,
	MUX_CLK_USI_CMGP0,
};
enum clk_id cmucal_vclk_div_clk_usi_cmgp2[] = {
	DIV_CLK_USI_CMGP2,
	MUX_CLK_USI_CMGP2,
};
enum clk_id cmucal_vclk_div_clk_usi_cmgp3[] = {
	DIV_CLK_USI_CMGP3,
	MUX_CLK_USI_CMGP3,
};
enum clk_id cmucal_vclk_div_clk_i2c_cmgp1[] = {
	DIV_CLK_I2C_CMGP1,
	MUX_CLK_I2C_CMGP1,
};
enum clk_id cmucal_vclk_div_clk_i2c_cmgp2[] = {
	DIV_CLK_I2C_CMGP2,
	MUX_CLK_I2C_CMGP2,
};
enum clk_id cmucal_vclk_div_clk_i2c_cmgp3[] = {
	DIV_CLK_I2C_CMGP3,
	MUX_CLK_I2C_CMGP3,
};
enum clk_id cmucal_vclk_clkcmu_hpm[] = {
	CLKCMU_HPM,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk0[] = {
	CLKCMU_CIS_CLK0,
	MUX_CLKCMU_CIS_CLK0,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk1[] = {
	CLKCMU_CIS_CLK1,
	MUX_CLKCMU_CIS_CLK1,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk2[] = {
	CLKCMU_CIS_CLK2,
	MUX_CLKCMU_CIS_CLK2,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk3[] = {
	CLKCMU_CIS_CLK3,
	MUX_CLKCMU_CIS_CLK3,
};
enum clk_id cmucal_vclk_clkcmu_cis_clk4[] = {
	CLKCMU_CIS_CLK4,
	MUX_CLKCMU_CIS_CLK4,
};
enum clk_id cmucal_vclk_div_clk_cpucl0_cmuref[] = {
	DIV_CLK_CPUCL0_CMUREF,
};
enum clk_id cmucal_vclk_div_clk_cluster0_periphclk[] = {
	DIV_CLK_CLUSTER0_PERIPHCLK,
};
enum clk_id cmucal_vclk_div_clk_cpucl1_cmuref[] = {
	DIV_CLK_CPUCL1_CMUREF,
};
enum clk_id cmucal_vclk_div_clk_cpucl2_cmuref[] = {
	DIV_CLK_CPUCL2_CMUREF,
};
enum clk_id cmucal_vclk_div_clk_cluster2_atclk[] = {
	DIV_CLK_CLUSTER2_ATCLK,
	DIV_CLK_CPUCL2_PCLKDBG,
	DIV_CLK_CLUSTER2_AMCLK,
};
enum clk_id cmucal_vclk_div_clk_fsys1_mmc_card[] = {
	DIV_CLK_FSYS1_MMC_CARD,
};
enum clk_id cmucal_vclk_div_clk_mif_pre[] = {
	DIV_CLK_MIF_PRE,
};
enum clk_id cmucal_vclk_div_clk_mif1_pre[] = {
	DIV_CLK_MIF1_PRE,
};
enum clk_id cmucal_vclk_div_clk_mif2_pre[] = {
	DIV_CLK_MIF2_PRE,
};
enum clk_id cmucal_vclk_div_clk_mif3_pre[] = {
	DIV_CLK_MIF3_PRE,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi00_usi[] = {
	DIV_CLK_PERIC0_USI00_USI,
	MUX_CLKCMU_PERIC0_USI00_USI_USER,
};
enum clk_id cmucal_vclk_mux_clkcmu_peric0_ip[] = {
	MUX_CLKCMU_PERIC0_IP,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi01_usi[] = {
	DIV_CLK_PERIC0_USI01_USI,
	MUX_CLKCMU_PERIC0_USI01_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi02_usi[] = {
	DIV_CLK_PERIC0_USI02_USI,
	MUX_CLKCMU_PERIC0_USI02_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi03_usi[] = {
	DIV_CLK_PERIC0_USI03_USI,
	MUX_CLKCMU_PERIC0_USI03_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi04_usi[] = {
	DIV_CLK_PERIC0_USI04_USI,
	MUX_CLKCMU_PERIC0_USI04_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi05_usi[] = {
	DIV_CLK_PERIC0_USI05_USI,
	MUX_CLKCMU_PERIC0_USI05_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_uart_dbg[] = {
	DIV_CLK_PERIC0_UART_DBG,
	MUX_CLKCMU_PERIC0_UART_DBG,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi12_usi[] = {
	DIV_CLK_PERIC0_USI12_USI,
	MUX_CLKCMU_PERIC0_USI12_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi13_usi[] = {
	DIV_CLK_PERIC0_USI13_USI,
	MUX_CLKCMU_PERIC0_USE13_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi14_usi[] = {
	DIV_CLK_PERIC0_USI14_USI,
	MUX_CLKCMU_PERIC0_USI14_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric0_usi15_usi[] = {
	DIV_CLK_PERIC0_USI15_USI,
	MUX_CLKCMU_PERIC0_USI15_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_uart_bt[] = {
	DIV_CLK_PERIC1_UART_BT,
	MUX_CLKCMU_PERIC1_UART_BT_USER,
};
enum clk_id cmucal_vclk_mux_clkcmu_peric1_ip[] = {
	MUX_CLKCMU_PERIC1_IP,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi06_usi[] = {
	DIV_CLK_PERIC1_USI06_USI,
	MUX_CLKCMU_PERIC1_USI06_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi07_usi[] = {
	DIV_CLK_PERIC1_USI07_USI,
	MUX_CLKCMU_PERIC1_USI07_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi08_usi[] = {
	DIV_CLK_PERIC1_USI08_USI,
	MUX_CLKCMU_PERIC1_USI08_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_i2c_cam0[] = {
	DIV_CLK_PERIC1_I2C_CAM0,
	MUX_CLKCMU_PERIC1_I2C_CAM0_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_i2c_cam1[] = {
	DIV_CLK_PERIC1_I2C_CAM1,
	MUX_CLKCMU_PERIC1_I2C_CAM1_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_i2c_cam2[] = {
	DIV_CLK_PERIC1_I2C_CAM2,
	MUX_CLKCMU_PERIC1_I2C_CAM2_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_i2c_cam3[] = {
	DIV_CLK_PERIC1_I2C_CAM3,
	MUX_CLKCMU_PERIC1_I2C_CAM3_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_spi_cam0[] = {
	DIV_CLK_PERIC1_SPI_CAM0,
	MUX_CLKCMU_PERIC1_SPI_CAM0_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi09_usi[] = {
	DIV_CLK_PERIC1_USI09_USI,
	MUX_CLKCMU_PERIC1_USI09_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi10_usi[] = {
	DIV_CLK_PERIC1_USI10_USI,
	MUX_CLKCMU_PERIC1_USI10_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi11_usi[] = {
	DIV_CLK_PERIC1_USI11_USI,
	MUX_CLKCMU_PERIC1_USI11_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi16_usi[] = {
	DIV_CLK_PERIC1_USI16_USI,
	MUX_CLKCMU_PERIC1_USI16_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_peric1_usi17_usi[] = {
	DIV_CLK_PERIC1_USI17_USI,
	MUX_CLKCMU_PERIC1_USI17_USI_USER,
};
enum clk_id cmucal_vclk_div_clk_vts_dmic[] = {
	DIV_CLK_VTS_DMIC,
};
/* COMMON VCLK -> Clock Node List */
enum clk_id cmucal_vclk_blk_cmu[] = {
	PLL_SHARED1_DIV3,
	CLKCMU_FSYS0A_BUS,
	MUX_CLKCMU_FSYS0A_BUS,
	PLL_SHARED1_DIV4,
	PLL_SHARED1_DIV2,
	MUX_PLL_SHARED1,
	PLL_SHARED1,
	CLKCMU_FSYS0A_USB31DRD,
	MUX_CLKCMU_FSYS0A_USB31DRD,
	PLL_SHARED4_DIV2,
	PLL_SHARED4,
	PLL_SHARED3_DIV2,
	PLL_SHARED3,
	CLKCMU_PERIC1_BUS,
	MUX_CLKCMU_PERIC1_BUS,
	CLKCMU_PERIC0_BUS,
	MUX_CLKCMU_PERIC0_BUS,
	CLKCMU_PERIS_BUS,
	MUX_CLKCMU_PERIS_BUS,
	PLL_SHARED2_DIV2,
	PLL_SHARED2,
	PLL_SHARED0_DIV3,
	PLL_SHARED0_DIV4,
	PLL_SHARED0_DIV2,
	MUX_PLL_SHARED0,
	PLL_SHARED0,
	APLL_SHARED1,
	APLL_SHARED0,
};
enum clk_id cmucal_vclk_blk_cpucl0[] = {
	DIV_CLK_CLUSTER0_ATCLK,
	DIV_CLK_CLUSTER0_PCLKDBG,
	DIV_CLK_CPUCL0_PCLK,
	MUX_CLK_CPUCL0_PLL,
	MUX_PLL_CPUCL0,
	APLL_CPUCL0,
	DIV_CLK_CPUCL0_DBG_PCLKDBG,
	DIV_CLK_CPUCL0_DBG_BUS,
};
enum clk_id cmucal_vclk_blk_mif1[] = {
	PLL_MIF1,
};
enum clk_id cmucal_vclk_blk_mif2[] = {
	PLL_MIF2,
};
enum clk_id cmucal_vclk_blk_mif3[] = {
	PLL_MIF3,
};
enum clk_id cmucal_vclk_blk_apm[] = {
	DIV_CLK_APM_BUS,
	MUX_CLK_APM_BUS,
	CLKCMU_VTS_BUS,
	MUX_CLKCMU_VTS_BUS,
};
enum clk_id cmucal_vclk_blk_dsps[] = {
	DIV_CLK_DSPS_BUSP,
	MUX_CLK_DSPS_BUS,
};
enum clk_id cmucal_vclk_blk_g3d[] = {
	DIV_CLK_G3D_BUSP,
	MUX_CLK_G3D_BUSD,
};
enum clk_id cmucal_vclk_blk_s2d[] = {
	MUX_CLK_S2D_CORE,
	CLKCMU_MIF_DDRPHY2X_S2D,
};
enum clk_id cmucal_vclk_blk_vts[] = {
	DIV_CLK_VTS_BUS,
	MUX_CLK_VTS_BUS,
	DIV_CLK_VTS_DMIC_DIV2,
	DIV_CLK_VTS_DMIC_IF,
};
enum clk_id cmucal_vclk_blk_aud[] = {
	DIV_CLK_AUD_CPU_ATCLK,
	DIV_CLK_AUD_BUSP,
};
enum clk_id cmucal_vclk_blk_busc[] = {
	DIV_CLK_BUSC_BUSP,
};
enum clk_id cmucal_vclk_blk_cmgp[] = {
	DIV_CLK_CMGP_BUS,
};
enum clk_id cmucal_vclk_blk_core[] = {
	DIV_CLK_CORE_BUSP,
};
enum clk_id cmucal_vclk_blk_cpucl2[] = {
	DIV_CLK_CPUCL2_PCLK,
};
enum clk_id cmucal_vclk_blk_dpu[] = {
	DIV_CLK_DPU_BUSP,
};
enum clk_id cmucal_vclk_blk_dspm[] = {
	DIV_CLK_DSPM_BUSP,
};
enum clk_id cmucal_vclk_blk_g2d[] = {
	DIV_CLK_G2D_BUSP,
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
	DIV_CLK_IVA_DEBUG,
};
enum clk_id cmucal_vclk_blk_mfc[] = {
	DIV_CLK_MFC_BUSP,
};
enum clk_id cmucal_vclk_blk_npu1[] = {
	DIV_CLK_NPU1_BUSP,
};
enum clk_id cmucal_vclk_blk_peric0[] = {
	DIV_CLK_PERIC0_USI_I2C,
};
enum clk_id cmucal_vclk_blk_peric1[] = {
	DIV_CLK_PERIC1_USI_I2C,
};
enum clk_id cmucal_vclk_blk_vra2[] = {
	DIV_CLK_VRA2_BUSP,
};
/* GATE VCLK -> Clock Node List */
enum clk_id cmucal_vclk_ip_lhs_axi_d_apm[] = {
	GOUT_BLK_APM_UID_LHS_AXI_D_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_apm[] = {
	GOUT_BLK_APM_UID_LHM_AXI_P_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_wdt_apm[] = {
	GOUT_BLK_APM_UID_WDT_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_apm[] = {
	GOUT_BLK_APM_UID_SYSREG_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_ap[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM_AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_pmu_alive[] = {
	GOUT_BLK_APM_UID_APBIF_PMU_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_intmem[] = {
	GOUT_BLK_APM_UID_INTMEM_IPCLKPORT_ACLK,
	GOUT_BLK_APM_UID_INTMEM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_c_modem[] = {
	GOUT_BLK_APM_UID_LHM_AXI_C_MODEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g_scan2dram[] = {
	GOUT_BLK_APM_UID_LHS_AXI_G_SCAN2DRAM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_pmu_intr_gen[] = {
	GOUT_BLK_APM_UID_PMU_INTR_GEN_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pem[] = {
	GOUT_BLK_APM_UID_PEM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_speedy_apm[] = {
	GOUT_BLK_APM_UID_SPEEDY_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_dp_apm[] = {
	GOUT_BLK_APM_UID_XIU_DP_APM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_apm_cmu_apm[] = {
	CLK_BLK_APM_UID_APM_CMU_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_apm[] = {
	GOUT_BLK_APM_UID_VGEN_LITE_APM_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_grebeintegration[] = {
	GOUT_BLK_APM_UID_GREBEINTEGRATION_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_apbif_gpio_alive[] = {
	GOUT_BLK_APM_UID_APBIF_GPIO_ALIVE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbif_top_rtc[] = {
	GOUT_BLK_APM_UID_APBIF_TOP_RTC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP_CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_cp_s[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP_CP_S_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_grebeintegration_dbgcore[] = {
	GOUT_BLK_APM_UID_GREBEINTEGRATION_DBGCORE_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_dtzpc_apm[] = {
	GOUT_BLK_APM_UID_DTZPC_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_c_vts[] = {
	GOUT_BLK_APM_UID_LHM_AXI_C_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_vts[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_dbgcore[] = {
	GOUT_BLK_APM_UID_MAILBOX_AP_DBGCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_lp_vts[] = {
	GOUT_BLK_APM_UID_LHS_AXI_LP_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mailbox_apm_cp[] = {
	GOUT_BLK_APM_UID_MAILBOX_APM_CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g_dbgcore[] = {
	GOUT_BLK_APM_UID_LHS_AXI_G_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_apbif_rtc[] = {
	GOUT_BLK_APM_UID_APBIF_RTC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_c_cmgp[] = {
	GOUT_BLK_APM_UID_LHS_AXI_C_CMGP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_speedy_sub_apm[] = {
	GOUT_BLK_APM_UID_SPEEDY_SUB_APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_aud_cmu_aud[] = {
	CLK_BLK_AUD_UID_AUD_CMU_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_aud[] = {
	GOUT_BLK_AUD_UID_LHS_AXI_D_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_aud[] = {
	GOUT_BLK_AUD_UID_PPMU_AUD_IPCLKPORT_ACLK,
	GOUT_BLK_AUD_UID_PPMU_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_aud[] = {
	GOUT_BLK_AUD_UID_SYSREG_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_abox[] = {
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF0,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF1,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF3,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_DSIF,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_CCLK_ASB,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_UAIF2,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_ACLK,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_CCLK_CA32,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_CCLK_DAP,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_ACLK_IRQ,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_ACLK_IRQ,
	GOUT_BLK_AUD_UID_ABOX_IPCLKPORT_BCLK_CNT,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t0_aud[] = {
	GOUT_BLK_AUD_UID_LHS_ATB_T0_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_gpio_aud[] = {
	GOUT_BLK_AUD_UID_GPIO_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi_us_32to128[] = {
	GOUT_BLK_AUD_UID_AXI_US_32TO128_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_btm_aud[] = {
	GOUT_BLK_AUD_UID_BTM_AUD_IPCLKPORT_I_ACLK,
	GOUT_BLK_AUD_UID_BTM_AUD_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_peri_axi_asb[] = {
	GOUT_BLK_AUD_UID_PERI_AXI_ASB_IPCLKPORT_ACLKM,
	GOUT_BLK_AUD_UID_PERI_AXI_ASB_IPCLKPORT_ACLKS,
	GOUT_BLK_AUD_UID_PERI_AXI_ASB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_aud[] = {
	GOUT_BLK_AUD_UID_LHM_AXI_P_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_wdt_aud[] = {
	GOUT_BLK_AUD_UID_WDT_AUD_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmic[] = {
	CLK_BLK_AUD_UID_DMIC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_trex_aud[] = {
	GOUT_BLK_AUD_UID_TREX_AUD_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_dftmux_aud[] = {
	CLK_BLK_AUD_UID_DFTMUX_AUD_IPCLKPORT_AUD_CODEC_MCLK,
};
enum clk_id cmucal_vclk_ip_smmu_aud[] = {
	GOUT_BLK_AUD_UID_SMMU_AUD_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_wrap2_conv_aud[] = {
	GOUT_BLK_AUD_UID_WRAP2_CONV_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_aud[] = {
	GOUT_BLK_AUD_UID_XIU_P_AUD_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_smmu_aud[] = {
	GOUT_BLK_AUD_UID_AD_APB_SMMU_AUD_IPCLKPORT_PCLKS,
	GOUT_BLK_AUD_UID_AD_APB_SMMU_AUD_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_axi2apb_aud[] = {
	GOUT_BLK_AUD_UID_AXI2APB_AUD_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_smmu_aud_s[] = {
	GOUT_BLK_AUD_UID_AD_APB_SMMU_AUD_S_IPCLKPORT_PCLKS,
	GOUT_BLK_AUD_UID_AD_APB_SMMU_AUD_S_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t1_aud[] = {
	GOUT_BLK_AUD_UID_LHS_ATB_T1_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_aud[] = {
	GOUT_BLK_AUD_UID_VGEN_LITE_AUD_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_busc_cmu_busc[] = {
	CLK_BLK_BUSC_UID_BUSC_CMU_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_buscp0[] = {
	GOUT_BLK_BUSC_UID_AXI2APB_BUSCP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_busc_tdp[] = {
	GOUT_BLK_BUSC_UID_AXI2APB_BUSC_TDP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_busc[] = {
	GOUT_BLK_BUSC_UID_SYSREG_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_busif_cmutopc[] = {
	GOUT_BLK_BUSC_UID_BUSIF_CMUTOPC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_trex_d0_busc[] = {
	GOUT_BLK_BUSC_UID_TREX_D0_BUSC_IPCLKPORT_PCLK,
	GOUT_BLK_BUSC_UID_TREX_D0_BUSC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_trex_p_busc[] = {
	GOUT_BLK_BUSC_UID_TREX_P_BUSC_IPCLKPORT_PCLK,
	GOUT_BLK_BUSC_UID_TREX_P_BUSC_IPCLKPORT_ACLK_BUSC,
	GOUT_BLK_BUSC_UID_TREX_P_BUSC_IPCLKPORT_PCLK_BUSC,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_mif0[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_mif1[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_mif2[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_mif3[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MIF3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_peris[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_PERIS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_peric0[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_PERIC0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_peric1[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_PERIC1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_asyncsfr_wr_smc[] = {
	GOUT_BLK_BUSC_UID_ASYNCSFR_WR_SMC_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_ivasc[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_D_IVASC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d0_g2d[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D0_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d1_g2d[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D1_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d2_g2d[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D2_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d_fsys0[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d_iva[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d_npu[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_NPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d0_dpu[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D0_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d0_mfc[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D0_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_isppre[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_ISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d1_dpu[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D1_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d1_mfc[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D1_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d2_dpu[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D2_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d0_isplp[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D0_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_dpu[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_isppre[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_ISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_dspm[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_fsys0[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_g2d[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_isphq[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_isplp[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_iva[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_mfc[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d_fsys1[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_apm[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d1_isplp[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D1_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_fsys1[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sirex[] = {
	GOUT_BLK_BUSC_UID_SIREX_IPCLKPORT_I_PCLK,
	GOUT_BLK_BUSC_UID_SIREX_IPCLKPORT_I_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d0_dspm[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D0_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_acel_d1_dspm[] = {
	GOUT_BLK_BUSC_UID_LHM_ACEL_D1_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_isphq[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_trex_rb_busc[] = {
	GOUT_BLK_BUSC_UID_TREX_RB_BUSC_IPCLKPORT_CLK,
	GOUT_BLK_BUSC_UID_TREX_RB_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppfw[] = {
	GOUT_BLK_BUSC_UID_PPFW_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_wrap2_conv_busc[] = {
	GOUT_BLK_BUSC_UID_WRAP2_CONV_BUSC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_vgen_pdma0[] = {
	GOUT_BLK_BUSC_UID_VGEN_PDMA0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_busc[] = {
	GOUT_BLK_BUSC_UID_VGEN_LITE_BUSC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_hpm_busc[] = {
	CLK_BLK_BUSC_UID_HPM_BUSC_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_busif_hpmbusc[] = {
	GOUT_BLK_BUSC_UID_BUSIF_HPMBUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pdma0[] = {
	GOUT_BLK_BUSC_UID_PDMA0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sbic[] = {
	GOUT_BLK_BUSC_UID_SBIC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_spdma[] = {
	GOUT_BLK_BUSC_UID_SPDMA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_dit[] = {
	GOUT_BLK_BUSC_UID_AD_APB_DIT_IPCLKPORT_PCLKM,
	GOUT_BLK_BUSC_UID_AD_APB_DIT_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_dit[] = {
	GOUT_BLK_BUSC_UID_DIT_IPCLKPORT_ICLKL2A,
};
enum clk_id cmucal_vclk_ip_d_tzpc_busc[] = {
	GOUT_BLK_BUSC_UID_D_TZPC_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_npu[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_NPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mmcache[] = {
	GOUT_BLK_BUSC_UID_MMCACHE_IPCLKPORT_ACLK,
	GOUT_BLK_BUSC_UID_MMCACHE_IPCLKPORT_PCLK,
	GOUT_BLK_BUSC_UID_MMCACHE_IPCLKPORT_PCLK_SECURE,
	GOUT_BLK_BUSC_UID_MMCACHE_IPCLKPORT_PCLK_SECURE,
	GOUT_BLK_BUSC_UID_MMCACHE_IPCLKPORT_PCLK_PPFW,
	GOUT_BLK_BUSC_UID_MMCACHE_IPCLKPORT_PCLK_PPFW,
};
enum clk_id cmucal_vclk_ip_trex_d1_busc[] = {
	GOUT_BLK_BUSC_UID_TREX_D1_BUSC_IPCLKPORT_ACLK,
	GOUT_BLK_BUSC_UID_TREX_D1_BUSC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_buscp1[] = {
	GOUT_BLK_BUSC_UID_AXI2APB_BUSCP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_aud[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_aud[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_dbg_g_busc[] = {
	GOUT_BLK_BUSC_UID_LHS_DBG_G_BUSC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_vts[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_vts[] = {
	GOUT_BLK_BUSC_UID_LHS_AXI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_qe_spdma[] = {
	GOUT_BLK_BUSC_UID_QE_SPDMA_IPCLKPORT_ACLK,
	GOUT_BLK_BUSC_UID_QE_SPDMA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_pdma0[] = {
	GOUT_BLK_BUSC_UID_QE_PDMA0_IPCLKPORT_ACLK,
	GOUT_BLK_BUSC_UID_QE_PDMA0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_busc[] = {
	GOUT_BLK_BUSC_UID_XIU_D_BUSC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_baaw_p_vts[] = {
	GOUT_BLK_BUSC_UID_BAAW_P_VTS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_axi_us_64to128[] = {
	GOUT_BLK_BUSC_UID_AXI_US_64TO128_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_baaw_p_npu[] = {
	GOUT_BLK_BUSC_UID_BAAW_P_NPU_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_vra2[] = {
	GOUT_BLK_BUSC_UID_LHM_AXI_D_VRA2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_cmgp_cmu_cmgp[] = {
	CLK_BLK_CMGP_UID_CMGP_CMU_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_adc_cmgp[] = {
	GOUT_BLK_CMGP_UID_ADC_CMGP_IPCLKPORT_PCLK_S0,
	GOUT_BLK_CMGP_UID_ADC_CMGP_IPCLKPORT_PCLK_S1,
};
enum clk_id cmucal_vclk_ip_gpio_cmgp[] = {
	GOUT_BLK_CMGP_UID_GPIO_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp0[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP0_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP0_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp1[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP1_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP1_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp2[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP2_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP2_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cmgp3[] = {
	GOUT_BLK_CMGP_UID_I2C_CMGP3_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_I2C_CMGP3_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cmgp[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp0[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP0_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP0_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp1[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP1_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP1_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp2[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP2_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP2_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi_cmgp3[] = {
	GOUT_BLK_CMGP_UID_USI_CMGP3_IPCLKPORT_PCLK,
	GOUT_BLK_CMGP_UID_USI_CMGP3_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cmgp2cp[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2CP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cmgp2pmu_ap[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2PMU_AP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dtzpc_cmgp[] = {
	GOUT_BLK_CMGP_UID_DTZPC_CMGP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_c_cmgp[] = {
	GOUT_BLK_CMGP_UID_LHM_AXI_C_CMGP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cmgp2apm[] = {
	GOUT_BLK_CMGP_UID_SYSREG_CMGP2APM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_core_cmu_core[] = {
	CLK_BLK_CORE_UID_CORE_CMU_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_core[] = {
	GOUT_BLK_CORE_UID_SYSREG_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_core_0[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_mpace2axi_0[] = {
	GOUT_BLK_CORE_UID_MPACE2AXI_0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_mpace2axi_1[] = {
	GOUT_BLK_CORE_UID_MPACE2AXI_1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppc_debug_cci[] = {
	GOUT_BLK_CORE_UID_PPC_DEBUG_CCI_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_DEBUG_CCI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_trex_p0_core[] = {
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_ACLK_CORE,
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_PCLK,
	GOUT_BLK_CORE_UID_TREX_P0_CORE_IPCLKPORT_PCLK_CORE,
};
enum clk_id cmucal_vclk_ip_ppmu_cpucl2_0[] = {
	GOUT_BLK_CORE_UID_PPMU_CPUCL2_0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPMU_CPUCL2_0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_dbg_g0_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G0_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_dbg_g1_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G1_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_dbg_g2_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G2_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_dbg_g3_dmc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G3_DMC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t_bdu[] = {
	GOUT_BLK_CORE_UID_LHS_ATB_T_BDU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_adm_apb_g_bdu[] = {
	GOUT_BLK_CORE_UID_ADM_APB_G_BDU_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_bdu[] = {
	GOUT_BLK_CORE_UID_BDU_IPCLKPORT_I_CLK,
	GOUT_BLK_CORE_UID_BDU_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_trex_p1_core[] = {
	GOUT_BLK_CORE_UID_TREX_P1_CORE_IPCLKPORT_PCLK,
	GOUT_BLK_CORE_UID_TREX_P1_CORE_IPCLKPORT_PCLK_CORE,
};
enum clk_id cmucal_vclk_ip_axi2apb_core_tp[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_TP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppfw_g3d[] = {
	GOUT_BLK_CORE_UID_PPFW_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_g3d[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_cpucl0[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_cpucl2[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CPUCL2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d0_cp[] = {
	GOUT_BLK_CORE_UID_LHM_AXI_D0_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ace_d0_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D0_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ace_d1_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D1_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ace_d2_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D2_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ace_d3_g3d[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D3_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_trex_d_core[] = {
	GOUT_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_PCLK,
	GOUT_BLK_CORE_UID_TREX_D_CORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_hpm_core[] = {
	CLK_BLK_CORE_UID_HPM_CORE_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_busif_hpmcore[] = {
	GOUT_BLK_CORE_UID_BUSIF_HPMCORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_bps_d0_g3d[] = {
	GOUT_BLK_CORE_UID_BPS_D0_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_bps_d1_g3d[] = {
	GOUT_BLK_CORE_UID_BPS_D1_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_bps_d2_g3d[] = {
	GOUT_BLK_CORE_UID_BPS_D2_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_bps_d3_g3d[] = {
	GOUT_BLK_CORE_UID_BPS_D3_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppcfw_g3d[] = {
	GOUT_BLK_CORE_UID_PPCFW_G3D_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPCFW_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_cp[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_apb_async_ppfw_g3d[] = {
	GOUT_BLK_CORE_UID_APB_ASYNC_PPFW_G3D_IPCLKPORT_PCLKS,
	GOUT_BLK_CORE_UID_APB_ASYNC_PPFW_G3D_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_baaw_cp[] = {
	GOUT_BLK_CORE_UID_BAAW_CP_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_bps_p_g3d[] = {
	GOUT_BLK_CORE_UID_BPS_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_apm[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_P_APM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_cpucl2_1[] = {
	GOUT_BLK_CORE_UID_PPMU_CPUCL2_1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPMU_CPUCL2_1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_core[] = {
	GOUT_BLK_CORE_UID_D_TZPC_CORE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_core_1[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_core[] = {
	GOUT_BLK_CORE_UID_XIU_P_CORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppc_cpucl2_0[] = {
	GOUT_BLK_CORE_UID_PPC_CPUCL2_0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_CPUCL2_0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_cpucl2_1[] = {
	GOUT_BLK_CORE_UID_PPC_CPUCL2_1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_CPUCL2_1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_g3d0[] = {
	GOUT_BLK_CORE_UID_PPC_G3D0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_G3D0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_g3d1[] = {
	GOUT_BLK_CORE_UID_PPC_G3D1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_G3D1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_g3d2[] = {
	GOUT_BLK_CORE_UID_PPC_G3D2_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_G3D2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_g3d3[] = {
	GOUT_BLK_CORE_UID_PPC_G3D3_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_G3D3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_irps0[] = {
	GOUT_BLK_CORE_UID_PPC_IRPS0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_IRPS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_irps1[] = {
	GOUT_BLK_CORE_UID_PPC_IRPS1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_IRPS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d1_cp[] = {
	GOUT_BLK_CORE_UID_LHM_AXI_D1_CP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_l_core[] = {
	GOUT_BLK_CORE_UID_LHS_AXI_L_CORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_core_2[] = {
	GOUT_BLK_CORE_UID_AXI2APB_CORE_2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_l_core[] = {
	GOUT_BLK_CORE_UID_LHM_AXI_L_CORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ace_d0_cluster0[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ace_d1_cluster0[] = {
	GOUT_BLK_CORE_UID_LHM_ACE_D1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppc_cpucl0_0[] = {
	GOUT_BLK_CORE_UID_PPC_CPUCL0_0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_CPUCL0_0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppc_cpucl0_1[] = {
	GOUT_BLK_CORE_UID_PPC_CPUCL0_1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPC_CPUCL0_1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_cpucl0_0[] = {
	GOUT_BLK_CORE_UID_PPMU_CPUCL0_0_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPMU_CPUCL0_0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_cpucl0_1[] = {
	GOUT_BLK_CORE_UID_PPMU_CPUCL0_1_IPCLKPORT_ACLK,
	GOUT_BLK_CORE_UID_PPMU_CPUCL0_1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_dbg_g_busc[] = {
	GOUT_BLK_CORE_UID_LHM_DBG_G_BUSC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mpace_asb_d0_mif[] = {
	GOUT_BLK_CORE_UID_MPACE_ASB_D0_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mpace_asb_d1_mif[] = {
	GOUT_BLK_CORE_UID_MPACE_ASB_D1_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mpace_asb_d2_mif[] = {
	GOUT_BLK_CORE_UID_MPACE_ASB_D2_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mpace_asb_d3_mif[] = {
	GOUT_BLK_CORE_UID_MPACE_ASB_D3_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_axi_asb_cssys[] = {
	GOUT_BLK_CORE_UID_AXI_ASB_CSSYS_IPCLKPORT_PCLK,
	GOUT_BLK_CORE_UID_AXI_ASB_CSSYS_IPCLKPORT_ACLKM,
	GOUT_BLK_CORE_UID_AXI_ASB_CSSYS_IPCLKPORT_ACLKS,
};
enum clk_id cmucal_vclk_ip_lhm_axi_g_cssys[] = {
	GOUT_BLK_CORE_UID_LHM_AXI_G_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_cci[] = {
	CLK_BLK_CORE_UID_CCI_IPCLKPORT_PCLK,
	CLK_BLK_CORE_UID_CCI_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_AXI2APB_CPUCL0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_SYSREG_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmcpucl0[] = {
	GOUT_BLK_CPUCL0_UID_BUSIF_HPMCPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_cssys[] = {
	GOUT_BLK_CPUCL0_UID_CSSYS_IPCLKPORT_PCLKDBG,
	GOUT_BLK_CPUCL0_UID_CSSYS_IPCLKPORT_ATCLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t0_aud[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T0_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t_bdu[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T_BDU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t0_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t0_cluster2[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T0_CLUSTER2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t1_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t1_cluster2[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T1_CLUSTER2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t2_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T2_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t3_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T3_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_secjtag[] = {
	GOUT_BLK_CPUCL0_UID_SECJTAG_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_P_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ace_d0_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ACE_D0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t0_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T0_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t1_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t2_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T2_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t3_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T3_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_adm_apb_g_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_ADM_APB_G_CLUSTER0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_cpucl0_cmu_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_CPUCL0_CMU_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_cpucl0[] = {
	CLK_BLK_CPUCL0_UID_CPUCL0_IPCLKPORT_PERIPHCLK,
	CLK_BLK_CPUCL0_UID_CPUCL0_IPCLKPORT_SCLK,
	CLK_BLK_CPUCL0_UID_CPUCL0_IPCLKPORT_PCLK,
	CLK_BLK_CPUCL0_UID_CPUCL0_IPCLKPORT_ATCLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t4_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T4_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t5_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T5_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ace_d1_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ACE_D1_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t4_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T4_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_t5_cluster0[] = {
	GOUT_BLK_CPUCL0_UID_LHS_ATB_T5_CLUSTER0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_D_TZPC_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_t1_aud[] = {
	GOUT_BLK_CPUCL0_UID_LHM_ATB_T1_AUD_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g_int_cssys[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_G_INT_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_g_int_cssys[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_G_INT_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g_int_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_G_INT_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_g_int_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_G_INT_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_XIU_P_CPUCL0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_dp_cssys[] = {
	GOUT_BLK_CPUCL0_UID_XIU_DP_CSSYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_trex_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_TREX_CPUCL0_IPCLKPORT_CLK,
	GOUT_BLK_CPUCL0_UID_TREX_CPUCL0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi_us_32to64_g_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_AXI_US_32TO64_G_DBGCORE_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g_cssys[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_G_CSSYS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_hpm_cpucl0_1[] = {
	CLK_BLK_CPUCL0_UID_HPM_CPUCL0_1_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_hpm_cpucl0_0[] = {
	CLK_BLK_CPUCL0_UID_HPM_CPUCL0_0_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_apb_async_p_cssys_0[] = {
	GOUT_BLK_CPUCL0_UID_APB_ASYNC_P_CSSYS_0_IPCLKPORT_PCLKS,
	GOUT_BLK_CPUCL0_UID_APB_ASYNC_P_CSSYS_0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g_int_etr[] = {
	GOUT_BLK_CPUCL0_UID_LHS_AXI_G_INT_ETR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_g_dbgcore[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_G_DBGCORE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_g_int_etr[] = {
	GOUT_BLK_CPUCL0_UID_LHM_AXI_G_INT_ETR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_p_cssys[] = {
	GOUT_BLK_CPUCL0_UID_AXI2APB_P_CSSYS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_bps_cpucl0[] = {
	GOUT_BLK_CPUCL0_UID_BPS_CPUCL0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_cpucl1_cmu_cpucl1[] = {
	CLK_BLK_CPUCL1_UID_CPUCL1_CMU_CPUCL1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_cpucl1[] = {
	CLK_BLK_CPUCL1_UID_CPUCL1_IPCLKPORT_SCLK,
};
enum clk_id cmucal_vclk_ip_cpucl2_cmu_cpucl2[] = {
	CLK_BLK_CPUCL2_UID_CPUCL2_CMU_CPUCL2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_cpucl2[] = {
	GOUT_BLK_CPUCL2_UID_SYSREG_CPUCL2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmcpucl2[] = {
	GOUT_BLK_CPUCL2_UID_BUSIF_HPMCPUCL2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_cpucl2_0[] = {
	CLK_BLK_CPUCL2_UID_HPM_CPUCL2_0_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_cluster2[] = {
	CLK_BLK_CPUCL2_UID_CLUSTER2_IPCLKPORT_PCLKDBG,
};
enum clk_id cmucal_vclk_ip_axi2apb_cpucl2[] = {
	GOUT_BLK_CPUCL2_UID_AXI2APB_CPUCL2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_cpucl2[] = {
	GOUT_BLK_CPUCL2_UID_LHM_AXI_P_CPUCL2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_hpm_cpucl2_1[] = {
	CLK_BLK_CPUCL2_UID_HPM_CPUCL2_1_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_hpm_cpucl2_2[] = {
	CLK_BLK_CPUCL2_UID_HPM_CPUCL2_2_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_d_tzpc_cpucl2[] = {
	GOUT_BLK_CPUCL2_UID_D_TZPC_CPUCL2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dpu_cmu_dpu[] = {
	CLK_BLK_DPU_UID_DPU_CMU_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_dpud0[] = {
	GOUT_BLK_DPU_UID_BTM_DPUD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_DPU_UID_BTM_DPUD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_dpud1[] = {
	GOUT_BLK_DPU_UID_BTM_DPUD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_DPU_UID_BTM_DPUD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_dpu[] = {
	GOUT_BLK_DPU_UID_SYSREG_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_dpup1[] = {
	GOUT_BLK_DPU_UID_AXI2APB_DPUP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_dpup0[] = {
	GOUT_BLK_DPU_UID_AXI2APB_DPUP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_dpud0[] = {
	GOUT_BLK_DPU_UID_SYSMMU_DPUD0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_dpu[] = {
	GOUT_BLK_DPU_UID_LHM_AXI_P_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d1_dpu[] = {
	GOUT_BLK_DPU_UID_LHS_AXI_D1_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_dpu[] = {
	GOUT_BLK_DPU_UID_XIU_P_DPU_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_decon0[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON0_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_DECON0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_apb_decon1[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON1_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_DECON1_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_apb_mipi_dsim1[] = {
	GOUT_BLK_DPU_UID_AD_APB_MIPI_DSIM1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_dpp[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPP_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_DPP_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d2_dpu[] = {
	GOUT_BLK_DPU_UID_LHS_AXI_D2_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_btm_dpud2[] = {
	GOUT_BLK_DPU_UID_BTM_DPUD2_IPCLKPORT_I_ACLK,
	GOUT_BLK_DPU_UID_BTM_DPUD2_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_dpud2[] = {
	GOUT_BLK_DPU_UID_SYSMMU_DPUD2_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_dpu_dma[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPU_DMA_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU_UID_AD_APB_DPU_DMA_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_dpu_wb_mux[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPU_WB_MUX_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU_UID_AD_APB_DPU_WB_MUX_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_sysmmu_dpud1[] = {
	GOUT_BLK_DPU_UID_SYSMMU_DPUD1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_dpud0[] = {
	GOUT_BLK_DPU_UID_PPMU_DPUD0_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_PPMU_DPUD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_dpud1[] = {
	GOUT_BLK_DPU_UID_PPMU_DPUD1_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_PPMU_DPUD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_dpud2[] = {
	GOUT_BLK_DPU_UID_PPMU_DPUD2_IPCLKPORT_ACLK,
	GOUT_BLK_DPU_UID_PPMU_DPUD2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_mipi_dsim0[] = {
	GOUT_BLK_DPU_UID_AD_APB_MIPI_DSIM0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_decon2[] = {
	GOUT_BLK_DPU_UID_AD_APB_DECON2_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_DECON2_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_apb_sysmmu_dpud0[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD0_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_sysmmu_dpud0_s[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD0_S_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD0_S_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_sysmmu_dpud1[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD1_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD1_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_apb_sysmmu_dpud1_s[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD1_S_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD1_S_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_sysmmu_dpud2[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD2_IPCLKPORT_PCLKM,
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_sysmmu_dpud2_s[] = {
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD2_S_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_SYSMMU_DPUD2_S_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_dpu[] = {
	GOUT_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DPU_WB_MUX,
	GOUT_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DECON,
	GOUT_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DMA,
	GOUT_BLK_DPU_UID_DPU_IPCLKPORT_ACLK_DPP,
};
enum clk_id cmucal_vclk_ip_wrapper_for_s5i6280_hsi_dcphy_combo_top[] = {
	GOUT_BLK_DPU_UID_WRAPPER_FOR_S5I6280_HSI_DCPHY_COMBO_TOP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_dpu_dma_pgen[] = {
	GOUT_BLK_DPU_UID_AD_APB_DPU_DMA_PGEN_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_DPU_DMA_PGEN_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d0_dpu[] = {
	GOUT_BLK_DPU_UID_LHS_AXI_D0_DPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_dpu[] = {
	GOUT_BLK_DPU_UID_D_TZPC_DPU_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_mcd[] = {
	GOUT_BLK_DPU_UID_AD_APB_MCD_IPCLKPORT_PCLKS,
	GOUT_BLK_DPU_UID_AD_APB_MCD_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_dspm_cmu_dspm[] = {
	CLK_BLK_DSPM_UID_DSPM_CMU_DSPM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_dspm[] = {
	GOUT_BLK_DSPM_UID_SYSREG_DSPM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_dspm[] = {
	GOUT_BLK_DSPM_UID_AXI2APB_DSPM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmu_dspm0[] = {
	GOUT_BLK_DSPM_UID_PPMU_DSPM0_IPCLKPORT_ACLK,
	GOUT_BLK_DSPM_UID_PPMU_DSPM0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_dspm0[] = {
	GOUT_BLK_DSPM_UID_SYSMMU_DSPM0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_btm_dspm0[] = {
	GOUT_BLK_DSPM_UID_BTM_DSPM0_IPCLKPORT_I_ACLK,
	GOUT_BLK_DSPM_UID_BTM_DSPM0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_dspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_P_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d0_dspm[] = {
	GOUT_BLK_DSPM_UID_LHS_ACEL_D0_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_ivadspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_P_IVADSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_dspmiva[] = {
	GOUT_BLK_DSPM_UID_LHS_AXI_P_DSPMIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_wrap2_conv_dspm[] = {
	GOUT_BLK_DSPM_UID_WRAP2_CONV_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_dspm0[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM0_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_dspm1[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM1_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_ad_apb_dspm3[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM3_IPCLKPORT_PCLKS,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM3_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_axi_dspm0[] = {
	GOUT_BLK_DSPM_UID_AD_AXI_DSPM0_IPCLKPORT_ACLKS,
	GOUT_BLK_DSPM_UID_AD_AXI_DSPM0_IPCLKPORT_ACLKM,
};
enum clk_id cmucal_vclk_ip_btm_dspm1[] = {
	GOUT_BLK_DSPM_UID_BTM_DSPM1_IPCLKPORT_I_ACLK,
	GOUT_BLK_DSPM_UID_BTM_DSPM1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d1_dspm[] = {
	GOUT_BLK_DSPM_UID_LHS_ACEL_D1_DSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_dspmdsps[] = {
	GOUT_BLK_DSPM_UID_LHS_AXI_P_DSPMDSPS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_dspm1[] = {
	GOUT_BLK_DSPM_UID_PPMU_DSPM1_IPCLKPORT_ACLK,
	GOUT_BLK_DSPM_UID_PPMU_DSPM1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_dspm1[] = {
	GOUT_BLK_DSPM_UID_SYSMMU_DSPM1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_adm_apb_dspm[] = {
	GOUT_BLK_DSPM_UID_ADM_APB_DSPM_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d0_dspsdspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AXI_D0_DSPSDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_dspm[] = {
	GOUT_BLK_DSPM_UID_XIU_P_DSPM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_dspm[] = {
	GOUT_BLK_DSPM_UID_VGEN_LITE_DSPM_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_dspm2[] = {
	GOUT_BLK_DSPM_UID_AD_APB_DSPM2_IPCLKPORT_PCLKM,
	GOUT_BLK_DSPM_UID_AD_APB_DSPM2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_score_ts_ii[] = {
	GOUT_BLK_DSPM_UID_SCORE_TS_II_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_dspm[] = {
	GOUT_BLK_DSPM_UID_D_TZPC_DSPM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_isppredspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AST_ISPPREDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_isplpdspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AST_ISPLPDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_isphqdspm[] = {
	GOUT_BLK_DSPM_UID_LHM_AST_ISPHQDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_dspmisppre[] = {
	GOUT_BLK_DSPM_UID_LHS_AST_DSPMISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_dspmisplp[] = {
	GOUT_BLK_DSPM_UID_LHS_AST_DSPMISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_dspm[] = {
	GOUT_BLK_DSPM_UID_XIU_D_DSPM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_baaw_dspm[] = {
	GOUT_BLK_DSPM_UID_BAAW_DSPM_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_dspmnpu0[] = {
	GOUT_BLK_DSPM_UID_LHS_AXI_D_DSPMNPU0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_dsps_cmu_dsps[] = {
	CLK_BLK_DSPS_UID_DSPS_CMU_DSPS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_dsps[] = {
	GOUT_BLK_DSPS_UID_AXI2APB_DSPS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_dspmdsps[] = {
	GOUT_BLK_DSPS_UID_LHM_AXI_P_DSPMDSPS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_dsps[] = {
	GOUT_BLK_DSPS_UID_SYSREG_DSPS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_dspsiva[] = {
	GOUT_BLK_DSPS_UID_LHS_AXI_D_DSPSIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d0_dspsdspm[] = {
	GOUT_BLK_DSPS_UID_LHS_AXI_D0_DSPSDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_score_baron[] = {
	GOUT_BLK_DSPS_UID_SCORE_BARON_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_ivadsps[] = {
	GOUT_BLK_DSPS_UID_LHM_AXI_D_IVADSPS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_dsps[] = {
	GOUT_BLK_DSPS_UID_D_TZPC_DSPS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_dsps[] = {
	GOUT_BLK_DSPS_UID_VGEN_LITE_DSPS_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_fsys0_cmu_fsys0[] = {
	CLK_BLK_FSYS0_UID_FSYS0_CMU_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d_fsys0[] = {
	GOUT_BLK_FSYS0_UID_LHS_ACEL_D_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_fsys0[] = {
	GOUT_BLK_FSYS0_UID_LHM_AXI_P_FSYS0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_gpio_fsys0[] = {
	GOUT_BLK_FSYS0_UID_GPIO_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_fsys0[] = {
	GOUT_BLK_FSYS0_UID_SYSREG_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_fsys0[] = {
	GOUT_BLK_FSYS0_UID_XIU_D_FSYS0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_btm_fsys0[] = {
	GOUT_BLK_FSYS0_UID_BTM_FSYS0_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS0_UID_BTM_FSYS0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_dp_link[] = {
	GOUT_BLK_FSYS0_UID_DP_LINK_IPCLKPORT_I_PCLK,
	GOUT_BLK_FSYS0_UID_DP_LINK_IPCLKPORT_I_DP_GTC_CLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_fsys0[] = {
	GOUT_BLK_FSYS0_UID_VGEN_LITE_FSYS0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_usb[] = {
	GOUT_BLK_FSYS0_UID_LHM_AXI_D_USB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_usb[] = {
	GOUT_BLK_FSYS0_UID_LHS_AXI_P_USB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_fsys0[] = {
	GOUT_BLK_FSYS0_UID_PPMU_FSYS0_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS0_UID_PPMU_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_pcie_gen3a[] = {
	GOUT_BLK_FSYS0_UID_SYSMMU_PCIE_GEN3A_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_pcie_gen3b[] = {
	GOUT_BLK_FSYS0_UID_SYSMMU_PCIE_GEN3B_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p0_fsys0[] = {
	GOUT_BLK_FSYS0_UID_XIU_P0_FSYS0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_pcie_gen3[] = {
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_PIPE_PAL_PCIE_INST_0_I_APB_PCLK,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_G3X2_DWC_PCIE_DM_INST_0_DBI_ACLK_UG,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_G3X2_DWC_PCIE_DM_INST_0_MSTR_ACLK_UG,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_PCIE_SUB_CTRL_INST_1_I_DRIVER_APB_CLK,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_IEEE1500_WRAPPER_FOR_QCHANNEL_WRAPPER_FOR_PCIE_PHY_TOP_X2_INST_0_I_PLL_REF_CLK_IN_SYSPLL,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_PCIE_SUB_CTRL_INST_1_PHY_REFCLK_IN,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_G3X2_DWC_PCIE_DM_INST_0_SLV_ACLK_UG,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_G3X1_DWC_PCIE_DM_INST_0_DBI_ACLK_UG,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_G3X1_DWC_PCIE_DM_INST_0_MSTR_ACLK_UG,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_PCIE_SUB_CTRL_INST_0_I_DRIVER_APB_CLK,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_PCIE_SUB_CTRL_INST_0_PHY_REFCLK_IN,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_MK_G3X1_DWC_PCIE_DM_INST_0_SLV_ACLK_UG,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_IEEE1500_WRAPPER_FOR_QCHANNEL_WRAPPER_FOR_PCIE_PHY_TOP_X2_INST_0_I_APB_PCLK_0,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_IEEE1500_WRAPPER_FOR_QCHANNEL_WRAPPER_FOR_PCIE_PHY_TOP_X2_INST_0_I_LN0_APB_PCLK_0,
	GOUT_BLK_FSYS0_UID_PCIE_GEN3_IPCLKPORT_IEEE1500_WRAPPER_FOR_QCHANNEL_WRAPPER_FOR_PCIE_PHY_TOP_X2_INST_0_I_LN1_APB_PCLK_0,
};
enum clk_id cmucal_vclk_ip_pcie_ia_gen3a[] = {
	GOUT_BLK_FSYS0_UID_PCIE_IA_GEN3A_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_pcie_ia_gen3b[] = {
	GOUT_BLK_FSYS0_UID_PCIE_IA_GEN3B_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_fsys0[] = {
	GOUT_BLK_FSYS0_UID_D_TZPC_FSYS0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_fsys0a_cmu_fsys0a[] = {
	CLK_BLK_FSYS0A_UID_FSYS0A_CMU_FSYS0A_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usb31drd[] = {
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_I_USBDPPHY_REF_SOC_PLL,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_ACLK_BUS,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_ACLK_BUS_PHYCTRL,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_ACLK_BUS_PHYCTRL,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_ACLK_PHYCTRL,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_BUS_CLK_EARLY,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_I_USB31DRD_REF_CLK_40,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_I_USBDPPHY_SCL_APB_PCLK,
	GOUT_BLK_FSYS0A_UID_USB31DRD_IPCLKPORT_I_USBPCS_APB_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_usb[] = {
	GOUT_BLK_FSYS0A_UID_LHM_AXI_P_USB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_usb[] = {
	GOUT_BLK_FSYS0A_UID_LHS_AXI_D_USB_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_fsys1_cmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_FSYS1_CMU_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mmc_card[] = {
	GOUT_BLK_FSYS1_UID_MMC_CARD_IPCLKPORT_SDCLKIN,
	GOUT_BLK_FSYS1_UID_MMC_CARD_IPCLKPORT_I_ACLK,
};
enum clk_id cmucal_vclk_ip_pcie_gen2[] = {
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_IEEE1500_WRAPPER_FOR_PCIEG2_PHY_X1_INST_0_I_SCL_APB_PCLK,
	CLK_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_PHY_REFCLK_IN,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_SLV_ACLK,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_DBI_ACLK,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_PCIE_SUB_CTRL_INST_0_I_DRIVER_APB_CLK,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_PIPE2_DIGITAL_X1_WRAP_INST_0_I_APB_PCLK_SCL,
	GOUT_BLK_FSYS1_UID_PCIE_GEN2_IPCLKPORT_MSTR_ACLK,
};
enum clk_id cmucal_vclk_ip_sss[] = {
	GOUT_BLK_FSYS1_UID_SSS_IPCLKPORT_I_PCLK,
	GOUT_BLK_FSYS1_UID_SSS_IPCLKPORT_I_ACLK,
};
enum clk_id cmucal_vclk_ip_rtic[] = {
	GOUT_BLK_FSYS1_UID_RTIC_IPCLKPORT_I_PCLK,
	GOUT_BLK_FSYS1_UID_RTIC_IPCLKPORT_I_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_fsys1[] = {
	GOUT_BLK_FSYS1_UID_SYSREG_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_gpio_fsys1[] = {
	GOUT_BLK_FSYS1_UID_GPIO_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d_fsys1[] = {
	GOUT_BLK_FSYS1_UID_LHS_ACEL_D_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_fsys1[] = {
	GOUT_BLK_FSYS1_UID_LHM_AXI_P_FSYS1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_fsys1[] = {
	GOUT_BLK_FSYS1_UID_XIU_D_FSYS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_fsys1[] = {
	GOUT_BLK_FSYS1_UID_XIU_P_FSYS1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_PPMU_FSYS1_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS1_UID_PPMU_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_fsys1[] = {
	GOUT_BLK_FSYS1_UID_BTM_FSYS1_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_BTM_FSYS1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_ufs_card[] = {
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_CLK_UNIPRO,
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_UFS_CARD_IPCLKPORT_I_FMP_CLK,
};
enum clk_id cmucal_vclk_ip_adm_ahb_sss[] = {
	GOUT_BLK_FSYS1_UID_ADM_AHB_SSS_IPCLKPORT_HCLKM,
};
enum clk_id cmucal_vclk_ip_sysmmu_fsys1[] = {
	GOUT_BLK_FSYS1_UID_SYSMMU_FSYS1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_fsys1[] = {
	GOUT_BLK_FSYS1_UID_VGEN_LITE_FSYS1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_pcie_ia_gen2[] = {
	GOUT_BLK_FSYS1_UID_PCIE_IA_GEN2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_fsys1[] = {
	GOUT_BLK_FSYS1_UID_D_TZPC_FSYS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ufs_embd[] = {
	GOUT_BLK_FSYS1_UID_UFS_EMBD_IPCLKPORT_I_ACLK,
	GOUT_BLK_FSYS1_UID_UFS_EMBD_IPCLKPORT_I_FMP_CLK,
	GOUT_BLK_FSYS1_UID_UFS_EMBD_IPCLKPORT_I_CLK_UNIPRO,
};
enum clk_id cmucal_vclk_ip_puf[] = {
	GOUT_BLK_FSYS1_UID_PUF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_qe_rtic[] = {
	GOUT_BLK_FSYS1_UID_QE_RTIC_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS1_UID_QE_RTIC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_sss[] = {
	GOUT_BLK_FSYS1_UID_QE_SSS_IPCLKPORT_ACLK,
	GOUT_BLK_FSYS1_UID_QE_SSS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_baaw_sss[] = {
	GOUT_BLK_FSYS1_UID_BAAW_SSS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_g2d_cmu_g2d[] = {
	CLK_BLK_G2D_UID_G2D_CMU_G2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_g2dd0[] = {
	GOUT_BLK_G2D_UID_PPMU_G2DD0_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_PPMU_G2DD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_g2dd1[] = {
	GOUT_BLK_G2D_UID_PPMU_G2DD1_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_PPMU_G2DD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_g2dd0[] = {
	GOUT_BLK_G2D_UID_SYSMMU_G2DD0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_g2d[] = {
	GOUT_BLK_G2D_UID_SYSREG_G2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d0_g2d[] = {
	GOUT_BLK_G2D_UID_LHS_ACEL_D0_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d1_g2d[] = {
	GOUT_BLK_G2D_UID_LHS_ACEL_D1_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_g2d[] = {
	GOUT_BLK_G2D_UID_LHM_AXI_P_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_as_p_g2d[] = {
	GOUT_BLK_G2D_UID_AS_P_G2D_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_G2D_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_axi2apb_g2dp0[] = {
	GOUT_BLK_G2D_UID_AXI2APB_G2DP0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_btm_g2dd0[] = {
	GOUT_BLK_G2D_UID_BTM_G2DD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_G2D_UID_BTM_G2DD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_g2dd1[] = {
	GOUT_BLK_G2D_UID_BTM_G2DD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_G2D_UID_BTM_G2DD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_g2d[] = {
	GOUT_BLK_G2D_UID_XIU_P_G2D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_g2dp1[] = {
	GOUT_BLK_G2D_UID_AXI2APB_G2DP1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_btm_g2dd2[] = {
	GOUT_BLK_G2D_UID_BTM_G2DD2_IPCLKPORT_I_ACLK,
	GOUT_BLK_G2D_UID_BTM_G2DD2_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_jpeg[] = {
	GOUT_BLK_G2D_UID_QE_JPEG_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_QE_JPEG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_mscl[] = {
	GOUT_BLK_G2D_UID_QE_MSCL_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_QE_MSCL_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_g2dd2[] = {
	GOUT_BLK_G2D_UID_SYSMMU_G2DD2_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_g2dd2[] = {
	GOUT_BLK_G2D_UID_PPMU_G2DD2_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_PPMU_G2DD2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d2_g2d[] = {
	GOUT_BLK_G2D_UID_LHS_ACEL_D2_G2D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_as_p_jpeg[] = {
	GOUT_BLK_G2D_UID_AS_P_JPEG_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_JPEG_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_xiu_d_g2d[] = {
	GOUT_BLK_G2D_UID_XIU_D_G2D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_as_p_mscl[] = {
	GOUT_BLK_G2D_UID_AS_P_MSCL_IPCLKPORT_PCLKS,
	GOUT_BLK_G2D_UID_AS_P_MSCL_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_as_p_astc[] = {
	GOUT_BLK_G2D_UID_AS_P_ASTC_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_ASTC_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_as_p_sysmmu_ns_g2dd0[] = {
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_NS_G2DD0_IPCLKPORT_PCLKS,
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_NS_G2DD0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_as_p_sysmmu_ns_g2dd2[] = {
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_NS_G2DD2_IPCLKPORT_PCLKS,
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_NS_G2DD2_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_as_p_sysmmu_s_g2dd0[] = {
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_S_G2DD0_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_S_G2DD0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_as_p_sysmmu_s_g2dd2[] = {
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_S_G2DD2_IPCLKPORT_PCLKS,
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_S_G2DD2_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_qe_astc[] = {
	GOUT_BLK_G2D_UID_QE_ASTC_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_QE_ASTC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_g2d[] = {
	GOUT_BLK_G2D_UID_VGEN_LITE_G2D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_g2d[] = {
	GOUT_BLK_G2D_UID_G2D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_as_p_sysmmu_ns_g2dd1[] = {
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_NS_G2DD1_IPCLKPORT_PCLKS,
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_NS_G2DD1_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_as_p_sysmmu_s_g2dd1[] = {
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_S_G2DD1_IPCLKPORT_PCLKS,
	GOUT_BLK_G2D_UID_AS_P_SYSMMU_S_G2DD1_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_sysmmu_g2dd1[] = {
	GOUT_BLK_G2D_UID_SYSMMU_G2DD1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_jpeg[] = {
	GOUT_BLK_G2D_UID_JPEG_IPCLKPORT_I_SMFC_CLK,
};
enum clk_id cmucal_vclk_ip_mscl[] = {
	GOUT_BLK_G2D_UID_MSCL_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_astc[] = {
	GOUT_BLK_G2D_UID_ASTC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_as_p_jsqz[] = {
	GOUT_BLK_G2D_UID_AS_P_JSQZ_IPCLKPORT_PCLKM,
	GOUT_BLK_G2D_UID_AS_P_JSQZ_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_qe_jsqz[] = {
	GOUT_BLK_G2D_UID_QE_JSQZ_IPCLKPORT_ACLK,
	GOUT_BLK_G2D_UID_QE_JSQZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_g2d[] = {
	GOUT_BLK_G2D_UID_D_TZPC_G2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_jsqz[] = {
	GOUT_BLK_G2D_UID_JSQZ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_g3d[] = {
	GOUT_BLK_G3D_UID_XIU_P_G3D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_g3d[] = {
	GOUT_BLK_G3D_UID_LHM_AXI_P_G3D_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmg3d[] = {
	GOUT_BLK_G3D_UID_BUSIF_HPMG3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_g3d0[] = {
	CLK_BLK_G3D_UID_HPM_G3D0_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_sysreg_g3d[] = {
	GOUT_BLK_G3D_UID_SYSREG_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_g3d_cmu_g3d[] = {
	CLK_BLK_G3D_UID_G3D_CMU_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_g3dsfr[] = {
	GOUT_BLK_G3D_UID_LHS_AXI_G3DSFR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_g3d[] = {
	GOUT_BLK_G3D_UID_VGEN_LITE_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_gpu[] = {
	CLK_BLK_G3D_UID_GPU_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_g3d[] = {
	GOUT_BLK_G3D_UID_AXI2APB_G3D_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_g3dsfr[] = {
	GOUT_BLK_G3D_UID_LHM_AXI_G3DSFR_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_gray2bin_g3d[] = {
	GOUT_BLK_G3D_UID_GRAY2BIN_G3D_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_g3d[] = {
	GOUT_BLK_G3D_UID_D_TZPC_G3D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_asb_g3d[] = {
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_PPMU_G3D0_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_PPMU_G3D1_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_PPMU_G3D2_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_PPMU_G3D3_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_QE_G3D0_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_QE_G3D1_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_QE_G3D2_PCLK,
	GOUT_BLK_G3D_UID_ASB_G3D_IPCLKPORT_QE_G3D3_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_isphq[] = {
	GOUT_BLK_ISPHQ_UID_LHM_AXI_P_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_isphq[] = {
	GOUT_BLK_ISPHQ_UID_LHS_AXI_D_ISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_is_isphq[] = {
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_AXI2APB_BRIDGE_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_ISPHQ_PCLKM,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_ISPHQ_PCLKS,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_SYSMMU_PCLKM_NONSECURE,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_SYSMMU_PCLKM_SECURE,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_SYSMMU_PCLKS_NONSECURE,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_APB_ASYNC_TOP_SYSMMU_PCLKS_SECURE,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_SYSMMU_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_PPMU_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_PPMU_ISPHQ_PCLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_VGEN_LITE_ISPHQ_PCLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_ISPHQ_ACLK,
	GOUT_BLK_ISPHQ_UID_IS_ISPHQ_IPCLKPORT_ISPHQ_C2COM_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_isphq[] = {
	GOUT_BLK_ISPHQ_UID_SYSREG_ISPHQ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_isphq_cmu_isphq[] = {
	CLK_BLK_ISPHQ_UID_ISPHQ_CMU_ISPHQ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_isppreisphq[] = {
	GOUT_BLK_ISPHQ_UID_LHM_ATB_ISPPREISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_isphqisplp[] = {
	GOUT_BLK_ISPHQ_UID_LHS_ATB_ISPHQISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_btm_isphq[] = {
	GOUT_BLK_ISPHQ_UID_BTM_ISPHQ_IPCLKPORT_I_ACLK,
	GOUT_BLK_ISPHQ_UID_BTM_ISPHQ_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_vo_isplpisphq[] = {
	GOUT_BLK_ISPHQ_UID_LHM_ATB_VO_ISPLPISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_vo_isphqisppre[] = {
	GOUT_BLK_ISPHQ_UID_LHS_AST_VO_ISPHQISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_isphq[] = {
	GOUT_BLK_ISPHQ_UID_D_TZPC_ISPHQ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_isphqdspm[] = {
	GOUT_BLK_ISPHQ_UID_LHS_AST_ISPHQDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_isplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_AXI_P_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d0_isplp[] = {
	GOUT_BLK_ISPLP_UID_LHS_AXI_D0_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_btm_isplp0[] = {
	GOUT_BLK_ISPLP_UID_BTM_ISPLP0_IPCLKPORT_I_ACLK,
	GOUT_BLK_ISPLP_UID_BTM_ISPLP0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_is_isplp[] = {
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_XIU_D_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_SYSMMU_ISPLP0_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_PPMU_ISPLP0_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_ISPLP_PCLKS,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_PPMU_ISPLP0_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_ISPLP_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_PPMU_ISPLP1_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_PPMU_ISPLP1_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_SYSMMU_ISPLP1_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_ISPLP_PCLKM,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_XIU_P_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_VGEN_LITE_ISPLP_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_GDC_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_QE_GDC_PCLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_ISPLP_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_ISPLP_C2CLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_MC_SCALER_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_GDC_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_XIU_ASYNCM_GDC_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_XIU_ASYNCS_GDC_ACLK,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_GDC_PCLKM,
	GOUT_BLK_ISPLP_UID_IS_ISPLP_IPCLKPORT_APB_ASYNC_TOP_GDC_PCLKS,
};
enum clk_id cmucal_vclk_ip_sysreg_isplp[] = {
	GOUT_BLK_ISPLP_UID_SYSREG_ISPLP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_isplp_cmu_isplp[] = {
	CLK_BLK_ISPLP_UID_ISPLP_CMU_ISPLP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_isplp1[] = {
	GOUT_BLK_ISPLP_UID_BTM_ISPLP1_IPCLKPORT_I_ACLK,
	GOUT_BLK_ISPLP_UID_BTM_ISPLP1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d1_isplp[] = {
	GOUT_BLK_ISPLP_UID_LHS_AXI_D1_ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_isphqisplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_ATB_ISPHQISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_vo_isppreisplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_AST_VO_ISPPREISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_atb_isppreisplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_ATB_ISPPREISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_vo_isplpisphq[] = {
	GOUT_BLK_ISPLP_UID_LHS_ATB_VO_ISPLPISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_isplp[] = {
	GOUT_BLK_ISPLP_UID_D_TZPC_ISPLP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_isplpdspm[] = {
	GOUT_BLK_ISPLP_UID_LHS_AST_ISPLPDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_dspmisplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_AST_DSPMISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_isplpvra2[] = {
	GOUT_BLK_ISPLP_UID_LHS_AXI_P_ISPLPVRA2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_vra2isplp[] = {
	GOUT_BLK_ISPLP_UID_LHM_AXI_D_VRA2ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_is_isppre[] = {
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_APB_ASYNC_TOP_3AA0_PCLKS,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_PPMU_ISPPRE,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_QE_PDP_TOP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_QE_3AA0,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_QE_PDP_TOP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_QE_3AA0,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_SYSMMU_ISPPRE,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_XIU_D_ISPPRE,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_QE_3AA1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_QE_3AA1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_APB_ASYNC_TOP_3AA0_PCLKM,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_VGEN_LITE_ISPPRE,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_3AA0,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_3AA1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_PDP_TOP_CORE_TOP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_PDP_TOP_DMA,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_QE_CSISX4_PDP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_VGEN_LITE_ISPPRE1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_VGEN_LITE_ISPPRE1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_QE_CSISX4_PDP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSIS0,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSIS1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSIS2,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSIS3,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSIS3_1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSIS3_1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_CSISX4_PDP_DMA,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_XIU_P_ISPPRE,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_PPMU_ISPPRE,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_VGEN_LITE_ISPPRE2,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_VGEN_LITE_ISPPRE2,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_VPP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_C2CLK_CSISX4_PDP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_C2CLK_3AA0,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_C2CLK_3AA1,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_C2CLK_AGENT,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_QE_VPP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_PCLK_QE_VPP,
	GOUT_BLK_ISPPRE_UID_IS_ISPPRE_IPCLKPORT_ACLK_PDP_TOP_RDMA,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_isppre[] = {
	GOUT_BLK_ISPPRE_UID_LHS_AXI_D_ISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_btm_isppre[] = {
	GOUT_BLK_ISPPRE_UID_BTM_ISPPRE_IPCLKPORT_I_ACLK,
	GOUT_BLK_ISPPRE_UID_BTM_ISPPRE_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_isppre[] = {
	GOUT_BLK_ISPPRE_UID_LHM_AXI_P_ISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_isppre[] = {
	GOUT_BLK_ISPPRE_UID_SYSREG_ISPPRE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_isppre_cmu_isppre[] = {
	CLK_BLK_ISPPRE_UID_ISPPRE_CMU_ISPPRE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_isppreisplp[] = {
	GOUT_BLK_ISPPRE_UID_LHS_ATB_ISPPREISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_atb_isppreisphq[] = {
	GOUT_BLK_ISPPRE_UID_LHS_ATB_ISPPREISPHQ_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_isppre[] = {
	GOUT_BLK_ISPPRE_UID_D_TZPC_ISPPRE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_isppredspm[] = {
	GOUT_BLK_ISPPRE_UID_LHS_AST_ISPPREDSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_dspmisppre[] = {
	GOUT_BLK_ISPPRE_UID_LHM_AST_DSPMISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmisppre[] = {
	GOUT_BLK_ISPPRE_UID_BUSIF_HPMISPPRE_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_isppre[] = {
	CLK_BLK_ISPPRE_UID_HPM_ISPPRE_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_d_tzpc_isppre1[] = {
	GOUT_BLK_ISPPRE_UID_D_TZPC_ISPPRE1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_vo_isppreisplp[] = {
	GOUT_BLK_ISPPRE_UID_LHS_AST_VO_ISPPREISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_vo_isphqisppre[] = {
	GOUT_BLK_ISPPRE_UID_LHM_AST_VO_ISPHQISPPRE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_iva_cmu_iva[] = {
	CLK_BLK_IVA_UID_IVA_CMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d_iva[] = {
	GOUT_BLK_IVA_UID_LHS_ACEL_D_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_ivadsps[] = {
	GOUT_BLK_IVA_UID_LHS_AXI_D_IVADSPS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_ivadspm[] = {
	GOUT_BLK_IVA_UID_LHS_AXI_P_IVADSPM_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_dspmiva[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_P_DSPMIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_iva[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_P_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_btm_iva[] = {
	GOUT_BLK_IVA_UID_BTM_IVA_IPCLKPORT_I_ACLK,
	GOUT_BLK_IVA_UID_BTM_IVA_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_iva[] = {
	GOUT_BLK_IVA_UID_PPMU_IVA_IPCLKPORT_ACLK,
	GOUT_BLK_IVA_UID_PPMU_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_iva[] = {
	GOUT_BLK_IVA_UID_SYSMMU_IVA_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_iva[] = {
	GOUT_BLK_IVA_UID_XIU_P_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_iva0[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA0_IPCLKPORT_PCLKS,
	GOUT_BLK_IVA_UID_AD_APB_IVA0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_axi2apb_2m_iva[] = {
	GOUT_BLK_IVA_UID_AXI2APB_2M_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_iva[] = {
	GOUT_BLK_IVA_UID_AXI2APB_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_iva[] = {
	GOUT_BLK_IVA_UID_SYSREG_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_ivasc[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_D_IVASC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_adm_dap_iva[] = {
	GOUT_BLK_IVA_UID_ADM_DAP_IVA_IPCLKPORT_DAPCLKM,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_dspsiva[] = {
	GOUT_BLK_IVA_UID_LHM_AXI_D_DSPSIVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ad_apb_iva1[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA1_IPCLKPORT_PCLKS,
	GOUT_BLK_IVA_UID_AD_APB_IVA1_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_ad_apb_iva2[] = {
	GOUT_BLK_IVA_UID_AD_APB_IVA2_IPCLKPORT_PCLKM,
	GOUT_BLK_IVA_UID_AD_APB_IVA2_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_vgen_lite_iva[] = {
	GOUT_BLK_IVA_UID_VGEN_LITE_IVA_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_iva[] = {
	GOUT_BLK_IVA_UID_IVA_IPCLKPORT_CLK_A,
	GOUT_BLK_IVA_UID_IVA_IPCLKPORT_DAP_CLK,
};
enum clk_id cmucal_vclk_ip_iva_intmem[] = {
	GOUT_BLK_IVA_UID_IVA_INTMEM_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_d0_iva[] = {
	GOUT_BLK_IVA_UID_XIU_D0_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_d1_iva[] = {
	GOUT_BLK_IVA_UID_XIU_D1_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_iva[] = {
	GOUT_BLK_IVA_UID_D_TZPC_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_d2_iva[] = {
	GOUT_BLK_IVA_UID_XIU_D2_IVA_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_trex_rb1_iva[] = {
	GOUT_BLK_IVA_UID_TREX_RB1_IVA_IPCLKPORT_CLK,
	GOUT_BLK_IVA_UID_TREX_RB1_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_iva[] = {
	GOUT_BLK_IVA_UID_QE_IVA_IPCLKPORT_ACLK,
	GOUT_BLK_IVA_UID_QE_IVA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wrap2_conv_iva[] = {
	GOUT_BLK_IVA_UID_WRAP2_CONV_IVA_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mfc_cmu_mfc[] = {
	CLK_BLK_MFC_UID_MFC_CMU_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_as_apb_mfc[] = {
	GOUT_BLK_MFC_UID_AS_APB_MFC_IPCLKPORT_PCLKS,
	GOUT_BLK_MFC_UID_AS_APB_MFC_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_axi2apb_mfc[] = {
	GOUT_BLK_MFC_UID_AXI2APB_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mfc[] = {
	GOUT_BLK_MFC_UID_SYSREG_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d0_mfc[] = {
	GOUT_BLK_MFC_UID_LHS_AXI_D0_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d1_mfc[] = {
	GOUT_BLK_MFC_UID_LHS_AXI_D1_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_mfc[] = {
	GOUT_BLK_MFC_UID_LHM_AXI_P_MFC_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_mfcd0[] = {
	GOUT_BLK_MFC_UID_SYSMMU_MFCD0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_mfcd1[] = {
	GOUT_BLK_MFC_UID_SYSMMU_MFCD1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_mfcd0[] = {
	GOUT_BLK_MFC_UID_PPMU_MFCD0_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_PPMU_MFCD0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_mfcd1[] = {
	GOUT_BLK_MFC_UID_PPMU_MFCD1_IPCLKPORT_ACLK,
	GOUT_BLK_MFC_UID_PPMU_MFCD1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_mfcd0[] = {
	GOUT_BLK_MFC_UID_BTM_MFCD0_IPCLKPORT_I_ACLK,
	GOUT_BLK_MFC_UID_BTM_MFCD0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_btm_mfcd1[] = {
	GOUT_BLK_MFC_UID_BTM_MFCD1_IPCLKPORT_I_ACLK,
	GOUT_BLK_MFC_UID_BTM_MFCD1_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_as_apb_sysmmu_ns_mfcd0[] = {
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_NS_MFCD0_IPCLKPORT_PCLKM,
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_NS_MFCD0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_as_apb_sysmmu_ns_mfcd1[] = {
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_NS_MFCD1_IPCLKPORT_PCLKM,
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_NS_MFCD1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_as_apb_sysmmu_s_mfcd0[] = {
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_S_MFCD0_IPCLKPORT_PCLKS,
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_S_MFCD0_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_as_apb_sysmmu_s_mfcd1[] = {
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_S_MFCD1_IPCLKPORT_PCLKM,
	GOUT_BLK_MFC_UID_AS_APB_SYSMMU_S_MFCD1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_as_apb_wfd_ns[] = {
	GOUT_BLK_MFC_UID_AS_APB_WFD_NS_IPCLKPORT_PCLKS,
	GOUT_BLK_MFC_UID_AS_APB_WFD_NS_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_as_axi_wfd[] = {
	GOUT_BLK_MFC_UID_AS_AXI_WFD_IPCLKPORT_ACLKS,
	GOUT_BLK_MFC_UID_AS_AXI_WFD_IPCLKPORT_ACLKM,
};
enum clk_id cmucal_vclk_ip_ppmu_mfcd2[] = {
	GOUT_BLK_MFC_UID_PPMU_MFCD2_IPCLKPORT_PCLK,
	GOUT_BLK_MFC_UID_PPMU_MFCD2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_mfc[] = {
	GOUT_BLK_MFC_UID_XIU_D_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_as_apb_wfd_s[] = {
	GOUT_BLK_MFC_UID_AS_APB_WFD_S_IPCLKPORT_PCLKS,
	GOUT_BLK_MFC_UID_AS_APB_WFD_S_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_vgen_mfc[] = {
	GOUT_BLK_MFC_UID_VGEN_MFC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_mfc[] = {
	GOUT_BLK_MFC_UID_MFC_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_wfd[] = {
	GOUT_BLK_MFC_UID_WFD_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lh_atb_mfc[] = {
	GOUT_BLK_MFC_UID_LH_ATB_MFC_IPCLKPORT_I_CLK_MI,
	GOUT_BLK_MFC_UID_LH_ATB_MFC_IPCLKPORT_I_CLK_SI,
};
enum clk_id cmucal_vclk_ip_d_tzpc_mfc[] = {
	GOUT_BLK_MFC_UID_D_TZPC_MFC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mif_cmu_mif[] = {
	CLK_BLK_MIF_UID_MIF_CMU_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ddrphy[] = {
	GOUT_BLK_MIF_UID_DDRPHY_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mif[] = {
	GOUT_BLK_MIF_UID_SYSREG_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmmif[] = {
	GOUT_BLK_MIF_UID_BUSIF_HPMMIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_mif[] = {
	GOUT_BLK_MIF_UID_LHM_AXI_P_MIF_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_mif[] = {
	GOUT_BLK_MIF_UID_AXI2APB_MIF_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppc_dvfs[] = {
	GOUT_BLK_MIF_UID_PPC_DVFS_IPCLKPORT_PCLK,
	CLK_BLK_MIF_UID_PPC_DVFS_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppc_debug[] = {
	GOUT_BLK_MIF_UID_PPC_DEBUG_IPCLKPORT_PCLK,
	CLK_BLK_MIF_UID_PPC_DEBUG_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_apbbr_ddrphy[] = {
	GOUT_BLK_MIF_UID_APBBR_DDRPHY_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmc[] = {
	GOUT_BLK_MIF_UID_APBBR_DMC_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmctz[] = {
	GOUT_BLK_MIF_UID_APBBR_DMCTZ_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_mif[] = {
	CLK_BLK_MIF_UID_HPM_MIF_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_dmc[] = {
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF_UID_DMC_IPCLKPORT_PCLK2,
	CLK_BLK_MIF_UID_DMC_IPCLKPORT_SOC_CLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppc_debug[] = {
	GOUT_BLK_MIF_UID_QCH_ADAPTER_PPC_DEBUG_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppc_dvfs[] = {
	GOUT_BLK_MIF_UID_QCH_ADAPTER_PPC_DVFS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_mif[] = {
	GOUT_BLK_MIF_UID_D_TZPC_MIF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_mif1[] = {
	CLK_BLK_MIF1_UID_HPM_MIF1_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_mif1_cmu_mif1[] = {
	CLK_BLK_MIF1_UID_MIF1_CMU_MIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_ddrphy1[] = {
	GOUT_BLK_MIF1_UID_APBBR_DDRPHY1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmc1[] = {
	GOUT_BLK_MIF1_UID_APBBR_DMC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmctz1[] = {
	GOUT_BLK_MIF1_UID_APBBR_DMCTZ1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_mif1[] = {
	GOUT_BLK_MIF1_UID_AXI2APB_MIF1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmmif1[] = {
	GOUT_BLK_MIF1_UID_BUSIF_HPMMIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ddrphy1[] = {
	GOUT_BLK_MIF1_UID_DDRPHY1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmc1[] = {
	GOUT_BLK_MIF1_UID_DMC1_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF1_UID_DMC1_IPCLKPORT_PCLK2,
	CLK_BLK_MIF1_UID_DMC1_IPCLKPORT_SOC_CLK,
	CLK_BLK_MIF1_UID_DMC1_IPCLKPORT_SOC_MPACE_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_mif1[] = {
	GOUT_BLK_MIF1_UID_LHM_AXI_P_MIF1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmuppc_debug1[] = {
	GOUT_BLK_MIF1_UID_PPMUPPC_DEBUG1_IPCLKPORT_PCLK,
	CLK_BLK_MIF1_UID_PPMUPPC_DEBUG1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmuppc_dvfs1[] = {
	GOUT_BLK_MIF1_UID_PPMUPPC_DVFS1_IPCLKPORT_PCLK,
	CLK_BLK_MIF1_UID_PPMUPPC_DVFS1_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mif1[] = {
	GOUT_BLK_MIF1_UID_SYSREG_MIF1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppmuppc_debug1[] = {
	GOUT_BLK_MIF1_UID_QCH_ADAPTER_PPMUPPC_DEBUG1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppmuppc_dvfs1[] = {
	GOUT_BLK_MIF1_UID_QCH_ADAPTER_PPMUPPC_DVFS1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_mif2[] = {
	CLK_BLK_MIF2_UID_HPM_MIF2_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_apbbr_ddrphy2[] = {
	GOUT_BLK_MIF2_UID_APBBR_DDRPHY2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmc2[] = {
	GOUT_BLK_MIF2_UID_APBBR_DMC2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmctz2[] = {
	GOUT_BLK_MIF2_UID_APBBR_DMCTZ2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_mif2[] = {
	GOUT_BLK_MIF2_UID_AXI2APB_MIF2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmmif2[] = {
	GOUT_BLK_MIF2_UID_BUSIF_HPMMIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ddrphy2[] = {
	GOUT_BLK_MIF2_UID_DDRPHY2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmc2[] = {
	GOUT_BLK_MIF2_UID_DMC2_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF2_UID_DMC2_IPCLKPORT_PCLK2,
	CLK_BLK_MIF2_UID_DMC2_IPCLKPORT_SOC_CLK,
	CLK_BLK_MIF2_UID_DMC2_IPCLKPORT_SOC_MPACE_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_mif2[] = {
	GOUT_BLK_MIF2_UID_LHM_AXI_P_MIF2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmuppc_debug2[] = {
	GOUT_BLK_MIF2_UID_PPMUPPC_DEBUG2_IPCLKPORT_PCLK,
	CLK_BLK_MIF2_UID_PPMUPPC_DEBUG2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmuppc_dvfs2[] = {
	GOUT_BLK_MIF2_UID_PPMUPPC_DVFS2_IPCLKPORT_PCLK,
	CLK_BLK_MIF2_UID_PPMUPPC_DVFS2_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mif2[] = {
	GOUT_BLK_MIF2_UID_SYSREG_MIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppmuppc_debug2[] = {
	GOUT_BLK_MIF2_UID_QCH_ADAPTER_PPMUPPC_DEBUG2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppmuppc_dvfs2[] = {
	GOUT_BLK_MIF2_UID_QCH_ADAPTER_PPMUPPC_DVFS2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mif2_cmu_mif2[] = {
	CLK_BLK_MIF2_UID_MIF2_CMU_MIF2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_hpm_mif3[] = {
	CLK_BLK_MIF3_UID_HPM_MIF3_IPCLKPORT_HPM_TARGETCLK_C,
};
enum clk_id cmucal_vclk_ip_apbbr_ddrphy3[] = {
	GOUT_BLK_MIF3_UID_APBBR_DDRPHY3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmc3[] = {
	GOUT_BLK_MIF3_UID_APBBR_DMC3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apbbr_dmctz3[] = {
	GOUT_BLK_MIF3_UID_APBBR_DMCTZ3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_mif3[] = {
	GOUT_BLK_MIF3_UID_AXI2APB_MIF3_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_busif_hpmmif3[] = {
	GOUT_BLK_MIF3_UID_BUSIF_HPMMIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ddrphy3[] = {
	GOUT_BLK_MIF3_UID_DDRPHY3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmc3[] = {
	GOUT_BLK_MIF3_UID_DMC3_IPCLKPORT_PCLK1,
	GOUT_BLK_MIF3_UID_DMC3_IPCLKPORT_PCLK2,
	CLK_BLK_MIF3_UID_DMC3_IPCLKPORT_SOC_CLK,
	CLK_BLK_MIF3_UID_DMC3_IPCLKPORT_SOC_MPACE_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_mif3[] = {
	GOUT_BLK_MIF3_UID_LHM_AXI_P_MIF3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmuppc_debug3[] = {
	GOUT_BLK_MIF3_UID_PPMUPPC_DEBUG3_IPCLKPORT_PCLK,
	CLK_BLK_MIF3_UID_PPMUPPC_DEBUG3_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_ppmuppc_dvfs3[] = {
	GOUT_BLK_MIF3_UID_PPMUPPC_DVFS3_IPCLKPORT_PCLK,
	CLK_BLK_MIF3_UID_PPMUPPC_DVFS3_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_mif3[] = {
	GOUT_BLK_MIF3_UID_SYSREG_MIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_mif3_cmu_mif3[] = {
	CLK_BLK_MIF3_UID_MIF3_CMU_MIF3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppmuppc_debug3[] = {
	GOUT_BLK_MIF3_UID_QCH_ADAPTER_PPMUPPC_DEBUG3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qch_adapter_ppmuppc_dvfs3[] = {
	GOUT_BLK_MIF3_UID_QCH_ADAPTER_PPMUPPC_DVFS3_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_acel_d_npu[] = {
	GOUT_BLK_NPU0_UID_LHS_ACEL_D_NPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_p_npu1[] = {
	GOUT_BLK_NPU0_UID_LHS_AXI_P_NPU1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_npu0_cmu_npu0[] = {
	CLK_BLK_NPU0_UID_NPU0_CMU_NPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_apb_async_si0[] = {
	GOUT_BLK_NPU0_UID_APB_ASYNC_SI0_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_apb_async_smmu_ns[] = {
	GOUT_BLK_NPU0_UID_APB_ASYNC_SMMU_NS_IPCLKPORT_PCLKS,
	GOUT_BLK_NPU0_UID_APB_ASYNC_SMMU_NS_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_axi2apb_npu0[] = {
	GOUT_BLK_NPU0_UID_AXI2APB_NPU0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_btm_npu0[] = {
	GOUT_BLK_NPU0_UID_BTM_NPU0_IPCLKPORT_I_ACLK,
	GOUT_BLK_NPU0_UID_BTM_NPU0_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_npu0[] = {
	GOUT_BLK_NPU0_UID_D_TZPC_NPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_0[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_1[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_2[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_3[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_4[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_4_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_5[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_5_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_6[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_6_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud1_d1_7[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_D_NPUD1_D1_7_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_p_npu1_done[] = {
	GOUT_BLK_NPU0_UID_LHM_AST_P_NPU1_DONE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_dspmnpu0[] = {
	GOUT_BLK_NPU0_UID_LHM_AXI_D_DSPMNPU0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_npu[] = {
	GOUT_BLK_NPU0_UID_LHM_AXI_P_NPU_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_0[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_1[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_2[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_3[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_4[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_4_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_5[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_5_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_6[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_6_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud0_d1_7[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_D_NPUD0_D1_7_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_p_npud1_setreg[] = {
	GOUT_BLK_NPU0_UID_LHS_AST_P_NPUD1_SETREG_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_idpsram1[] = {
	GOUT_BLK_NPU0_UID_LHS_AXI_D_IDPSRAM1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_idpsram3[] = {
	GOUT_BLK_NPU0_UID_LHS_AXI_D_IDPSRAM3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_npuc[] = {
	GOUT_BLK_NPU0_UID_NPUC_IPCLKPORT_I_PCLK,
	GOUT_BLK_NPU0_UID_NPUC_IPCLKPORT_I_CM7_CLKIN,
	GOUT_BLK_NPU0_UID_NPUC_IPCLKPORT_I_ACLK,
};
enum clk_id cmucal_vclk_ip_npud_unit0[] = {
	GOUT_BLK_NPU0_UID_NPUD_UNIT0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_cpudma[] = {
	GOUT_BLK_NPU0_UID_PPMU_CPUDMA_IPCLKPORT_ACLK,
	GOUT_BLK_NPU0_UID_PPMU_CPUDMA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_rfm[] = {
	GOUT_BLK_NPU0_UID_PPMU_RFM_IPCLKPORT_ACLK,
	GOUT_BLK_NPU0_UID_PPMU_RFM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_cpudma[] = {
	GOUT_BLK_NPU0_UID_QE_CPUDMA_IPCLKPORT_ACLK,
	GOUT_BLK_NPU0_UID_QE_CPUDMA_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_qe_rfm[] = {
	GOUT_BLK_NPU0_UID_QE_RFM_IPCLKPORT_ACLK,
	GOUT_BLK_NPU0_UID_QE_RFM_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_smmu_npu0[] = {
	GOUT_BLK_NPU0_UID_SMMU_NPU0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_npu0[] = {
	GOUT_BLK_NPU0_UID_SYSREG_NPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_xiu_d_npu0[] = {
	GOUT_BLK_NPU0_UID_XIU_D_NPU0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_apb_async_smmu_s[] = {
	GOUT_BLK_NPU0_UID_APB_ASYNC_SMMU_S_IPCLKPORT_PCLKM,
	GOUT_BLK_NPU0_UID_APB_ASYNC_SMMU_S_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_vgen_lite_npu0[] = {
	GOUT_BLK_NPU0_UID_VGEN_LITE_NPU0_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_npu0[] = {
	GOUT_BLK_NPU0_UID_PPMU_NPU0_IPCLKPORT_ACLK,
	GOUT_BLK_NPU0_UID_PPMU_NPU0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_npu0_ppc_wrapper[] = {
	GOUT_BLK_NPU0_UID_NPU0_PPC_WRAPPER_IPCLKPORT_ACLK,
	GOUT_BLK_NPU0_UID_NPU0_PPC_WRAPPER_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_npu1_cmu_npu1[] = {
	CLK_BLK_NPU1_UID_NPU1_CMU_NPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_0[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_npu1[] = {
	GOUT_BLK_NPU1_UID_LHM_AXI_P_NPU1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_apb_async_si1[] = {
	GOUT_BLK_NPU1_UID_APB_ASYNC_SI1_IPCLKPORT_PCLKS,
};
enum clk_id cmucal_vclk_ip_axi2apb_npu1[] = {
	GOUT_BLK_NPU1_UID_AXI2APB_NPU1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_npu1[] = {
	GOUT_BLK_NPU1_UID_D_TZPC_NPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_1[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_2[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_3[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_4[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_4_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_5[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_5_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_6[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_6_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_d_npud0_d1_7[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_D_NPUD0_D1_7_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_ast_p_npud1_setreg[] = {
	GOUT_BLK_NPU1_UID_LHM_AST_P_NPUD1_SETREG_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_idpsram1[] = {
	GOUT_BLK_NPU1_UID_LHM_AXI_D_IDPSRAM1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_d_idpsram3[] = {
	GOUT_BLK_NPU1_UID_LHM_AXI_D_IDPSRAM3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_0[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_1[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_2[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_3[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_3_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_4[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_4_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_5[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_5_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_6[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_6_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_d_npud1_d1_7[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_D_NPUD1_D1_7_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_sysreg_npu1[] = {
	GOUT_BLK_NPU1_UID_SYSREG_NPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_ast_p_npu1_done[] = {
	GOUT_BLK_NPU1_UID_LHS_AST_P_NPU1_DONE_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_npud_unit1[] = {
	GOUT_BLK_NPU1_UID_NPUD_UNIT1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_ppmu_npu1[] = {
	GOUT_BLK_NPU1_UID_PPMU_NPU1_IPCLKPORT_ACLK,
	GOUT_BLK_NPU1_UID_PPMU_NPU1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_npu1_ppc_wrapper[] = {
	GOUT_BLK_NPU1_UID_NPU1_PPC_WRAPPER_IPCLKPORT_ACLK,
	GOUT_BLK_NPU1_UID_NPU1_PPC_WRAPPER_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_gpio_peric0[] = {
	GOUT_BLK_PERIC0_UID_GPIO_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_pwm[] = {
	GOUT_BLK_PERIC0_UID_PWM_IPCLKPORT_I_PCLK_S0,
};
enum clk_id cmucal_vclk_ip_sysreg_peric0[] = {
	GOUT_BLK_PERIC0_UID_SYSREG_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi00_usi[] = {
	GOUT_BLK_PERIC0_UID_USI00_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI00_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi01_usi[] = {
	GOUT_BLK_PERIC0_UID_USI01_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI01_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi02_usi[] = {
	GOUT_BLK_PERIC0_UID_USI02_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI02_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi03_usi[] = {
	GOUT_BLK_PERIC0_UID_USI03_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI03_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_peric0p0[] = {
	GOUT_BLK_PERIC0_UID_AXI2APB_PERIC0P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_peric0_cmu_peric0[] = {
	CLK_BLK_PERIC0_UID_PERIC0_CMU_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi04_usi[] = {
	GOUT_BLK_PERIC0_UID_USI04_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI04_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_peric0p1[] = {
	GOUT_BLK_PERIC0_UID_AXI2APB_PERIC0P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_usi05_usi[] = {
	GOUT_BLK_PERIC0_UID_USI05_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI05_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi00_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI00_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI00_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi01_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI01_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI01_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi02_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI02_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI02_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi03_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI03_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI03_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi04_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI04_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI04_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi05_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI05_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI05_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_uart_dbg[] = {
	GOUT_BLK_PERIC0_UID_UART_DBG_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_UART_DBG_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_peric0[] = {
	GOUT_BLK_PERIC0_UID_XIU_P_PERIC0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_peric0[] = {
	GOUT_BLK_PERIC0_UID_LHM_AXI_P_PERIC0_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_usi12_usi[] = {
	GOUT_BLK_PERIC0_UID_USI12_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI12_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi12_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI12_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI12_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi13_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI13_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI13_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi13_usi[] = {
	GOUT_BLK_PERIC0_UID_USI13_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI13_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi14_usi[] = {
	GOUT_BLK_PERIC0_UID_USI14_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI14_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi14_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI14_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC0_UID_USI14_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_peric0[] = {
	GOUT_BLK_PERIC0_UID_D_TZPC_PERIC0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi15_i2c[] = {
	GOUT_BLK_PERIC0_UID_USI15_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI15_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi15_usi[] = {
	GOUT_BLK_PERIC0_UID_USI15_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC0_UID_USI15_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_peric1p1[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_gpio_peric1[] = {
	GOUT_BLK_PERIC1_UID_GPIO_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_peric1[] = {
	GOUT_BLK_PERIC1_UID_SYSREG_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_uart_bt[] = {
	GOUT_BLK_PERIC1_UID_UART_BT_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_UART_BT_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cam1[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM1_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM1_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cam2[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM2_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM2_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cam3[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM3_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM3_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi06_usi[] = {
	GOUT_BLK_PERIC1_UID_USI06_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI06_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi07_usi[] = {
	GOUT_BLK_PERIC1_UID_USI07_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI07_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi08_usi[] = {
	GOUT_BLK_PERIC1_UID_USI08_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI08_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_i2c_cam0[] = {
	GOUT_BLK_PERIC1_UID_I2C_CAM0_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_I2C_CAM0_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_peric1[] = {
	GOUT_BLK_PERIC1_UID_XIU_P_PERIC1_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_peric1p0[] = {
	GOUT_BLK_PERIC1_UID_AXI2APB_PERIC1P0_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_peric1_cmu_peric1[] = {
	CLK_BLK_PERIC1_UID_PERIC1_CMU_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_spi_cam0[] = {
	GOUT_BLK_PERIC1_UID_SPI_CAM0_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_SPI_CAM0_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi09_usi[] = {
	GOUT_BLK_PERIC1_UID_USI09_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI09_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi06_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI06_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI06_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi10_usi[] = {
	GOUT_BLK_PERIC1_UID_USI10_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI10_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi07_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI07_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI07_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi08_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI08_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI08_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi09_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI09_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI09_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi10_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI10_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI10_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_peric1[] = {
	GOUT_BLK_PERIC1_UID_LHM_AXI_P_PERIC1_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_usi11_usi[] = {
	GOUT_BLK_PERIC1_UID_USI11_USI_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI11_USI_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_usi11_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI11_I2C_IPCLKPORT_PCLK,
	GOUT_BLK_PERIC1_UID_USI11_I2C_IPCLKPORT_IPCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_peric1[] = {
	GOUT_BLK_PERIC1_UID_D_TZPC_PERIC1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_i3c[] = {
	GOUT_BLK_PERIC1_UID_I3C_IPCLKPORT_I_PCLK,
	GOUT_BLK_PERIC1_UID_I3C_IPCLKPORT_I_SCLK,
};
enum clk_id cmucal_vclk_ip_usi16_usi[] = {
	GOUT_BLK_PERIC1_UID_USI16_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI16_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi17_usi[] = {
	GOUT_BLK_PERIC1_UID_USI17_USI_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI17_USI_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_usi16_i3c[] = {
	GOUT_BLK_PERIC1_UID_USI16_I3C_IPCLKPORT_I_SCLK,
	GOUT_BLK_PERIC1_UID_USI16_I3C_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_usi17_i2c[] = {
	GOUT_BLK_PERIC1_UID_USI17_I2C_IPCLKPORT_IPCLK,
	GOUT_BLK_PERIC1_UID_USI17_I2C_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_axi2apb_perisp[] = {
	GOUT_BLK_PERIS_UID_AXI2APB_PERISP_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_xiu_p_peris[] = {
	GOUT_BLK_PERIS_UID_XIU_P_PERIS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sysreg_peris[] = {
	GOUT_BLK_PERIS_UID_SYSREG_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_cluster2[] = {
	GOUT_BLK_PERIS_UID_WDT_CLUSTER2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_cluster0[] = {
	GOUT_BLK_PERIS_UID_WDT_CLUSTER0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_peris_cmu_peris[] = {
	CLK_BLK_PERIS_UID_PERIS_CMU_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ad_axi_p_peris[] = {
	GOUT_BLK_PERIS_UID_AD_AXI_P_PERIS_IPCLKPORT_ACLKS,
	GOUT_BLK_PERIS_UID_AD_AXI_P_PERIS_IPCLKPORT_ACLKM,
};
enum clk_id cmucal_vclk_ip_otp_con_bira[] = {
	GOUT_BLK_PERIS_UID_OTP_CON_BIRA_IPCLKPORT_PCLK,
	CLK_BLK_PERIS_UID_OTP_CON_BIRA_IPCLKPORT_I_OSCCLK,
};
enum clk_id cmucal_vclk_ip_gic[] = {
	GOUT_BLK_PERIS_UID_GIC_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_peris[] = {
	GOUT_BLK_PERIS_UID_LHM_AXI_P_PERIS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_mct[] = {
	GOUT_BLK_PERIS_UID_MCT_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_otp_con_top[] = {
	GOUT_BLK_PERIS_UID_OTP_CON_TOP_IPCLKPORT_PCLK,
	CLK_BLK_PERIS_UID_OTP_CON_TOP_IPCLKPORT_I_OSCCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_peris[] = {
	GOUT_BLK_PERIS_UID_D_TZPC_PERIS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_tmu_sub[] = {
	GOUT_BLK_PERIS_UID_TMU_SUB_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_tmu_top[] = {
	GOUT_BLK_PERIS_UID_TMU_TOP_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_otp_con_bisr[] = {
	CLK_BLK_PERIS_UID_OTP_CON_BISR_IPCLKPORT_I_OSCCLK,
	GOUT_BLK_PERIS_UID_OTP_CON_BISR_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_s2d_cmu_s2d[] = {
	CLK_BLK_S2D_UID_S2D_CMU_S2D_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vra2_cmu_vra2[] = {
	CLK_BLK_VRA2_UID_VRA2_CMU_VRA2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_as_apb_vra2[] = {
	GOUT_BLK_VRA2_UID_AS_APB_VRA2_IPCLKPORT_PCLKS,
	GOUT_BLK_VRA2_UID_AS_APB_VRA2_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_axi2apb_vra2[] = {
	GOUT_BLK_VRA2_UID_AXI2APB_VRA2_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_vra2[] = {
	GOUT_BLK_VRA2_UID_D_TZPC_VRA2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_isplpvra2[] = {
	GOUT_BLK_VRA2_UID_LHM_AXI_P_ISPLPVRA2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_vra2isplp[] = {
	GOUT_BLK_VRA2_UID_LHS_AXI_D_VRA2ISPLP_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_qe_vra2[] = {
	GOUT_BLK_VRA2_UID_QE_VRA2_IPCLKPORT_ACLK,
	GOUT_BLK_VRA2_UID_QE_VRA2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_vra2[] = {
	GOUT_BLK_VRA2_UID_SYSREG_VRA2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite_vra2[] = {
	GOUT_BLK_VRA2_UID_VGEN_LITE_VRA2_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_vra2[] = {
	GOUT_BLK_VRA2_UID_VRA2_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_as_apb_str[] = {
	GOUT_BLK_VRA2_UID_AS_APB_STR_IPCLKPORT_PCLKS,
	GOUT_BLK_VRA2_UID_AS_APB_STR_IPCLKPORT_PCLKM,
};
enum clk_id cmucal_vclk_ip_btm_vra2[] = {
	GOUT_BLK_VRA2_UID_BTM_VRA2_IPCLKPORT_I_ACLK,
	GOUT_BLK_VRA2_UID_BTM_VRA2_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_ppmu_vra2[] = {
	GOUT_BLK_VRA2_UID_PPMU_VRA2_IPCLKPORT_ACLK,
	GOUT_BLK_VRA2_UID_PPMU_VRA2_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysmmu_vra2[] = {
	GOUT_BLK_VRA2_UID_SYSMMU_VRA2_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_str[] = {
	GOUT_BLK_VRA2_UID_STR_IPCLKPORT_I_STR_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_vra2[] = {
	GOUT_BLK_VRA2_UID_LHS_AXI_D_VRA2_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_dmic_if[] = {
	GOUT_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_IF_CLK,
	CLK_BLK_VTS_UID_DMIC_IF_IPCLKPORT_DMIC_IF_DIV2_CLK,
	GOUT_BLK_VTS_UID_DMIC_IF_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_sysreg_vts[] = {
	GOUT_BLK_VTS_UID_SYSREG_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vts_cmu_vts[] = {
	CLK_BLK_VTS_UID_VTS_CMU_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_ahb_busmatrix[] = {
	GOUT_BLK_VTS_UID_AHB_BUSMATRIX_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_lhm_axi_p_vts[] = {
	GOUT_BLK_VTS_UID_LHM_AXI_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_gpio_vts[] = {
	GOUT_BLK_VTS_UID_GPIO_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_wdt_vts[] = {
	GOUT_BLK_VTS_UID_WDT_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmic_ahb0[] = {
	GOUT_BLK_VTS_UID_DMIC_AHB0_IPCLKPORT_HCLK,
	GOUT_BLK_VTS_UID_DMIC_AHB0_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmic_ahb1[] = {
	GOUT_BLK_VTS_UID_DMIC_AHB1_IPCLKPORT_HCLK,
	GOUT_BLK_VTS_UID_DMIC_AHB1_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_c_vts[] = {
	GOUT_BLK_VTS_UID_LHS_AXI_C_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_asyncinterrupt[] = {
	GOUT_BLK_VTS_UID_ASYNCINTERRUPT_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_hwacg_sys_dmic0[] = {
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC0_IPCLKPORT_HCLK_BUS,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC0_IPCLKPORT_HCLK_BUS,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC0_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_hwacg_sys_dmic1[] = {
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC1_IPCLKPORT_HCLK_BUS,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC1_IPCLKPORT_HCLK_BUS,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC1_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_ss_vts_glue[] = {
	GOUT_BLK_VTS_UID_SS_VTS_GLUE_IPCLKPORT_ACLK_CPU,
};
enum clk_id cmucal_vclk_ip_cortexm4integration[] = {
	GOUT_BLK_VTS_UID_CORTEXM4INTEGRATION_IPCLKPORT_FCLK,
};
enum clk_id cmucal_vclk_ip_u_dmic_clk_mux[] = {
	CLK_BLK_VTS_UID_U_DMIC_CLK_MUX_IPCLKPORT_D0,
};
enum clk_id cmucal_vclk_ip_lhm_axi_lp_vts[] = {
	GOUT_BLK_VTS_UID_LHM_AXI_LP_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_lhs_axi_d_vts[] = {
	GOUT_BLK_VTS_UID_LHS_AXI_D_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_baaw_c_vts[] = {
	GOUT_BLK_VTS_UID_BAAW_C_VTS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_d_tzpc_vts[] = {
	GOUT_BLK_VTS_UID_D_TZPC_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_vgen_lite[] = {
	GOUT_BLK_VTS_UID_VGEN_LITE_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_bps_lp_vts[] = {
	GOUT_BLK_VTS_UID_BPS_LP_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_bps_p_vts[] = {
	GOUT_BLK_VTS_UID_BPS_P_VTS_IPCLKPORT_I_CLK,
};
enum clk_id cmucal_vclk_ip_xhb_lp_vts[] = {
	GOUT_BLK_VTS_UID_XHB_LP_VTS_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_xhb_p_vts[] = {
	GOUT_BLK_VTS_UID_XHB_P_VTS_IPCLKPORT_CLK,
};
enum clk_id cmucal_vclk_ip_sweeper_c_vts[] = {
	GOUT_BLK_VTS_UID_SWEEPER_C_VTS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_sweeper_d_vts[] = {
	GOUT_BLK_VTS_UID_SWEEPER_D_VTS_IPCLKPORT_ACLK,
};
enum clk_id cmucal_vclk_ip_baaw_d_vts[] = {
	GOUT_BLK_VTS_UID_BAAW_D_VTS_IPCLKPORT_I_PCLK,
};
enum clk_id cmucal_vclk_ip_mailbox_abox_vts[] = {
	GOUT_BLK_VTS_UID_MAILBOX_ABOX_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_dmic_ahb2[] = {
	GOUT_BLK_VTS_UID_DMIC_AHB2_IPCLKPORT_PCLK,
	GOUT_BLK_VTS_UID_DMIC_AHB2_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_dmic_ahb3[] = {
	GOUT_BLK_VTS_UID_DMIC_AHB3_IPCLKPORT_PCLK,
	GOUT_BLK_VTS_UID_DMIC_AHB3_IPCLKPORT_HCLK,
};
enum clk_id cmucal_vclk_ip_hwacg_sys_dmic2[] = {
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC2_IPCLKPORT_HCLK,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC2_IPCLKPORT_HCLK_BUS,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC2_IPCLKPORT_HCLK_BUS,
};
enum clk_id cmucal_vclk_ip_hwacg_sys_dmic3[] = {
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC3_IPCLKPORT_HCLK,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC3_IPCLKPORT_HCLK_BUS,
	GOUT_BLK_VTS_UID_HWACG_SYS_DMIC3_IPCLKPORT_HCLK_BUS,
};
enum clk_id cmucal_vclk_ip_dmic_if_3rd[] = {
	GOUT_BLK_VTS_UID_DMIC_IF_3RD_IPCLKPORT_PCLK,
	GOUT_BLK_VTS_UID_DMIC_IF_3RD_IPCLKPORT_DMIC_IF_CLK,
	GOUT_BLK_VTS_UID_DMIC_IF_3RD_IPCLKPORT_DMIC_IF_DIV2_CLK,
};
enum clk_id cmucal_vclk_ip_mailbox_ap_vts[] = {
	GOUT_BLK_VTS_UID_MAILBOX_AP_VTS_IPCLKPORT_PCLK,
};
enum clk_id cmucal_vclk_ip_timer[] = {
	GOUT_BLK_VTS_UID_TIMER_IPCLKPORT_PCLK,
};

/* DVFS VCLK -> LUT List */
struct vclk_lut cmucal_vclk_vddi_lut[] = {
	{860000, vddi_nm_lut_params},
	{860000, vddi_od_lut_params},
	{650000, vddi_ud_lut_params},
	{320000, vddi_sud_lut_params},
	{160000, vddi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_vdd_mif_lut[] = {
	{4264000, vdd_mif_od_lut_params},
	{2687750, vdd_mif_ud_lut_params},
	{1420000, vdd_mif_sud_lut_params},
	{842000, vdd_mif_uud_lut_params},
};
struct vclk_lut cmucal_vclk_vdd_cam_lut[] = {
	{1200000, vdd_cam_nm_lut_params},
	{1200000, vdd_cam_od_lut_params},
	{1200000, vdd_cam_sud_lut_params},
	{1200000, vdd_cam_ud_lut_params},
	{600000, vdd_cam_uud_lut_params},
};
struct vclk_lut cmucal_vclk_vdd_cheetah_lut[] = {
	{2850250, vdd_cheetah_sod_lut_params},
	{2224857, vdd_cheetah_od_lut_params},
	{1834857, vdd_cheetah_nm_lut_params},
	{1100000, vdd_cheetah_ud_lut_params},
};
struct vclk_lut cmucal_vclk_vdd_ananke_lut[] = {
	{1950000, vdd_ananke_sod_lut_params},
	{1599000, vdd_ananke_od_lut_params},
	{1150500, vdd_ananke_nm_lut_params},
	{650000, vdd_ananke_ud_lut_params},
	{349917, vdd_ananke_sud_lut_params},
	{175000, vdd_ananke_uud_lut_params},
};
struct vclk_lut cmucal_vclk_vdd_prometheus_lut[] = {
	{2398500, vdd_prometheus_sod_lut_params},
	{1800000, vdd_prometheus_od_lut_params},
	{1400000, vdd_prometheus_nm_lut_params},
	{800000, vdd_prometheus_ud_lut_params},
	{466000, vdd_prometheus_sud_lut_params},
};

/* SPECIAL VCLK -> LUT List */
struct vclk_lut cmucal_vclk_mux_clk_aud_uaif3_lut[] = {
	{600000, mux_clk_aud_uaif3_nm_lut_params},
	{600000, mux_clk_aud_uaif3_od_lut_params},
	{466375, mux_clk_aud_uaif3_uud_lut_params},
	{300000, mux_clk_aud_uaif3_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_buc_cmuref_lut[] = {
	{26000, mux_buc_cmuref_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clk_cmgp_adc_lut[] = {
	{28571, mux_clk_cmgp_adc_nm_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cmgp_bus_lut[] = {
	{400000, clkcmu_cmgp_bus_nm_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_apm_bus_lut[] = {
	{355333, clkcmu_apm_bus_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_cmu_cmuref_lut[] = {
	{533000, mux_cmu_cmuref_ud_lut_params},
	{266500, mux_cmu_cmuref_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_core_cmuref_lut[] = {
	{26000, mux_core_cmuref_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_cpucl0_cmuref_lut[] = {
	{26000, mux_cpucl0_cmuref_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_cpucl1_cmuref_lut[] = {
	{26000, mux_cpucl1_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_cpucl2_cmuref_lut[] = {
	{26000, mux_cpucl2_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_mif_cmuref_lut[] = {
	{233188, mux_mif_cmuref_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_mif1_cmuref_lut[] = {
	{26000, mux_mif1_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_mif2_cmuref_lut[] = {
	{26000, mux_mif2_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_mif3_cmuref_lut[] = {
	{26000, mux_mif3_cmuref_ud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_fsys0_dpgtc_lut[] = {
	{133250, clkcmu_fsys0_dpgtc_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clkcmu_fsys0_pcie_lut[] = {
	{800000, mux_clkcmu_fsys0_pcie_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clkcmu_fsys0a_usbdp_debug_lut[] = {
	{800000, mux_clkcmu_fsys0a_usbdp_debug_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clkcmu_fsys1_pcie_lut[] = {
	{800000, mux_clkcmu_fsys1_pcie_uud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_fsys1_ufs_embd_lut[] = {
	{177667, clkcmu_fsys1_ufs_embd_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_i2c_cmgp0_lut[] = {
	{400000, div_clk_i2c_cmgp0_nm_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_usi_cmgp1_lut[] = {
	{400000, div_clk_usi_cmgp1_nm_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_usi_cmgp0_lut[] = {
	{400000, div_clk_usi_cmgp0_nm_lut_params},
	{4000, div_clk_usi_cmgp0_4m_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_usi_cmgp2_lut[] = {
	{400000, div_clk_usi_cmgp2_nm_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_usi_cmgp3_lut[] = {
	{400000, div_clk_usi_cmgp3_nm_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_i2c_cmgp1_lut[] = {
	{400000, div_clk_i2c_cmgp1_nm_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_i2c_cmgp2_lut[] = {
	{400000, div_clk_i2c_cmgp2_nm_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_i2c_cmgp3_lut[] = {
	{400000, div_clk_i2c_cmgp3_nm_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_hpm_lut[] = {
	{621833, clkcmu_hpm_od_lut_params},
	{400000, clkcmu_hpm_uud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cis_clk0_lut[] = {
	{100000, clkcmu_cis_clk0_uud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cis_clk1_lut[] = {
	{100000, clkcmu_cis_clk1_uud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cis_clk2_lut[] = {
	{100000, clkcmu_cis_clk2_uud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cis_clk3_lut[] = {
	{100000, clkcmu_cis_clk3_uud_lut_params},
};
struct vclk_lut cmucal_vclk_clkcmu_cis_clk4_lut[] = {
	{100000, clkcmu_cis_clk4_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cpucl0_cmuref_lut[] = {
	{975000, div_clk_cpucl0_cmuref_sod_lut_params},
	{799500, div_clk_cpucl0_cmuref_od_lut_params},
	{575250, div_clk_cpucl0_cmuref_nm_lut_params},
	{325000, div_clk_cpucl0_cmuref_ud_lut_params},
	{174958, div_clk_cpucl0_cmuref_sud_lut_params},
	{87500, div_clk_cpucl0_cmuref_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cluster0_periphclk_lut[] = {
	{975000, div_clk_cluster0_periphclk_sod_lut_params},
	{799500, div_clk_cluster0_periphclk_od_lut_params},
	{575250, div_clk_cluster0_periphclk_nm_lut_params},
	{325000, div_clk_cluster0_periphclk_ud_lut_params},
	{174958, div_clk_cluster0_periphclk_sud_lut_params},
	{87500, div_clk_cluster0_periphclk_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cpucl1_cmuref_lut[] = {
	{1199250, div_clk_cpucl1_cmuref_sod_lut_params},
	{900000, div_clk_cpucl1_cmuref_od_lut_params},
	{700000, div_clk_cpucl1_cmuref_nm_lut_params},
	{400000, div_clk_cpucl1_cmuref_ud_lut_params},
	{233000, div_clk_cpucl1_cmuref_sud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cpucl2_cmuref_lut[] = {
	{1425125, div_clk_cpucl2_cmuref_sod_lut_params},
	{1112429, div_clk_cpucl2_cmuref_od_lut_params},
	{917429, div_clk_cpucl2_cmuref_nm_lut_params},
	{550000, div_clk_cpucl2_cmuref_ud_lut_params},
	{266500, div_clk_cpucl2_cmuref_sud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_cluster2_atclk_lut[] = {
	{1425125, div_clk_cluster2_atclk_sod_lut_params},
	{1112429, div_clk_cluster2_atclk_od_lut_params},
	{917429, div_clk_cluster2_atclk_nm_lut_params},
	{550000, div_clk_cluster2_atclk_ud_lut_params},
	{266500, div_clk_cluster2_atclk_sud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_fsys1_mmc_card_lut[] = {
	{808000, div_clk_fsys1_mmc_card_ud_lut_params},
	{404000, div_clk_fsys1_mmc_card_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_mif_pre_lut[] = {
	{533000, div_clk_mif_pre_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_mif1_pre_lut[] = {
	{2132000, div_clk_mif1_pre_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_mif2_pre_lut[] = {
	{2132000, div_clk_mif2_pre_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_mif3_pre_lut[] = {
	{2132000, div_clk_mif3_pre_ud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi00_usi_lut[] = {
	{400000, div_clk_peric0_usi00_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clkcmu_peric0_ip_lut[] = {
	{400000, mux_clkcmu_peric0_ip_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi01_usi_lut[] = {
	{400000, div_clk_peric0_usi01_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi02_usi_lut[] = {
	{400000, div_clk_peric0_usi02_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi03_usi_lut[] = {
	{400000, div_clk_peric0_usi03_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi04_usi_lut[] = {
	{400000, div_clk_peric0_usi04_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi05_usi_lut[] = {
	{400000, div_clk_peric0_usi05_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_uart_dbg_lut[] = {
	{200000, div_clk_peric0_uart_dbg_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi12_usi_lut[] = {
	{400000, div_clk_peric0_usi12_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi13_usi_lut[] = {
	{400000, div_clk_peric0_usi13_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi14_usi_lut[] = {
	{400000, div_clk_peric0_usi14_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric0_usi15_usi_lut[] = {
	{400000, div_clk_peric0_usi15_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_uart_bt_lut[] = {
	{200000, div_clk_peric1_uart_bt_uud_lut_params},
};
struct vclk_lut cmucal_vclk_mux_clkcmu_peric1_ip_lut[] = {
	{400000, mux_clkcmu_peric1_ip_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi06_usi_lut[] = {
	{400000, div_clk_peric1_usi06_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi07_usi_lut[] = {
	{400000, div_clk_peric1_usi07_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi08_usi_lut[] = {
	{400000, div_clk_peric1_usi08_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_i2c_cam0_lut[] = {
	{200000, div_clk_peric1_i2c_cam0_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_i2c_cam1_lut[] = {
	{200000, div_clk_peric1_i2c_cam1_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_i2c_cam2_lut[] = {
	{200000, div_clk_peric1_i2c_cam2_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_i2c_cam3_lut[] = {
	{200000, div_clk_peric1_i2c_cam3_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_spi_cam0_lut[] = {
	{400000, div_clk_peric1_spi_cam0_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi09_usi_lut[] = {
	{400000, div_clk_peric1_usi09_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi10_usi_lut[] = {
	{400000, div_clk_peric1_usi10_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi11_usi_lut[] = {
	{400000, div_clk_peric1_usi11_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi16_usi_lut[] = {
	{400000, div_clk_peric1_usi16_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_peric1_usi17_usi_lut[] = {
	{400000, div_clk_peric1_usi17_usi_uud_lut_params},
};
struct vclk_lut cmucal_vclk_div_clk_vts_dmic_lut[] = {
	{60000, div_clk_vts_dmic_nm_lut_params},
};

struct vclk_lut cmucal_vclk_div_clk_pericx_usixx_usi_lut[] = {
	{200000, div_clk_peric_200_lut_params},
	{100000, div_clk_peric_100_lut_params},
	{50000, div_clk_peric_50_lut_params},
	{26000, div_clk_peric_26_lut_params},
	{25000, div_clk_peric_25_lut_params},
	{13000, div_clk_peric_13_lut_params},
	{8600, div_clk_peric_8_lut_params},
	{6500, div_clk_peric_6_lut_params},
};

/* COMMON VCLK -> LUT List */
struct vclk_lut cmucal_vclk_blk_cmu_lut[] = {
	{710667, blk_cmu_ud_lut_params},
	{710667, blk_cmu_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_cpucl0_lut[] = {
	{487500, blk_cpucl0_sod_lut_params},
	{400000, blk_cpucl0_nm_lut_params},
	{400000, blk_cpucl0_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_mif1_lut[] = {
	{26000, blk_mif1_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_mif2_lut[] = {
	{26000, blk_mif2_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_mif3_lut[] = {
	{26000, blk_mif3_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_apm_lut[] = {
	{400000, blk_apm_nm_lut_params},
};
struct vclk_lut cmucal_vclk_blk_dsps_lut[] = {
	{266500, blk_dsps_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_g3d_lut[] = {
	{215000, blk_g3d_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_s2d_lut[] = {
	{100000, blk_s2d_od_lut_params},
	{106, blk_s2d_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_vts_lut[] = {
	{400000, blk_vts_nm_lut_params},
};
struct vclk_lut cmucal_vclk_blk_aud_lut[] = {
	{600000, blk_aud_nm_lut_params},
	{600000, blk_aud_od_lut_params},
	{466375, blk_aud_uud_lut_params},
	{300000, blk_aud_ud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_busc_lut[] = {
	{266500, blk_busc_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_cmgp_lut[] = {
	{400000, blk_cmgp_nm_lut_params},
};
struct vclk_lut cmucal_vclk_blk_core_lut[] = {
	{355333, blk_core_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_cpucl2_lut[] = {
	{356281, blk_cpucl2_sod_lut_params},
	{278107, blk_cpucl2_od_lut_params},
	{229357, blk_cpucl2_nm_lut_params},
	{137500, blk_cpucl2_ud_lut_params},
	{66625, blk_cpucl2_sud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_dpu_lut[] = {
	{160000, blk_dpu_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_dspm_lut[] = {
	{266500, blk_dspm_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_g2d_lut[] = {
	{266500, blk_g2d_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_isphq_lut[] = {
	{333000, blk_isphq_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_isplp_lut[] = {
	{333000, blk_isplp_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_isppre_lut[] = {
	{333000, blk_isppre_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_iva_lut[] = {
	{266500, blk_iva_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_mfc_lut[] = {
	{166500, blk_mfc_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_npu1_lut[] = {
	{200000, blk_npu1_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_peric0_lut[] = {
	{200000, blk_peric0_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_peric1_lut[] = {
	{200000, blk_peric1_uud_lut_params},
};
struct vclk_lut cmucal_vclk_blk_vra2_lut[] = {
	{400000, blk_vra2_uud_lut_params},
};

/* Switch VCLK -> LUT Parameter List */
struct switch_lut mux_clk_aud_cpu_lut[] = {
	{1066000, 0, 0},
	{932750, 1, 0},
	{533000, 4, 0},
	{333000, 3, 1},
	{166500, 3, 3},
};
struct switch_lut mux_clk_cpucl1_pll_lut[] = {
	{1066000, 0, 0},
	{800000, 2, 0},
	{400000, 2, 1},
};
struct switch_lut mux_clk_cpucl2_pll_lut[] = {
	{1066000, 0, 0},
	{932750, 1, 0},
	{533000, 0, 1},
};
struct switch_lut mux_clk_fsys1_bus_lut[] = {
	{333000, 2, -1},
};
struct switch_lut mux_clk_fsys1_mmc_card_lut[] = {
	{800000, 1, -1},
	{26000, 0, -1},
};
struct switch_lut clkmux_mif_ddrphy2x_lut[] = {
	{2132000, 0, -1},
	{1865500, 1, -1},
	{1066000, 2, -1},
};
struct switch_lut clkmux_mif1_ddrphy2x_lut[] = {
	{2132000, 0, -1},
	{1865500, 1, -1},
	{1066000, 2, -1},
};
struct switch_lut clkmux_mif2_ddrphy2x_lut[] = {
	{2132000, 0, -1},
	{1865500, 1, -1},
	{1066000, 2, -1},
};
struct switch_lut clkmux_mif3_ddrphy2x_lut[] = {
	{2132000, 0, -1},
	{1865500, 1, -1},
	{1066000, 2, -1},
};
/*================================ SWPLL List =================================*/
struct vclk_switch switch_vddi[] = {
	{MUX_CLK_FSYS1_BUS, MUX_CLKCMU_FSYS1_BUS, EMPTY_CAL_ID, CLKCMU_FSYS1_BUS, MUX_CLKCMU_FSYS1_BUS_USER, mux_clk_fsys1_bus_lut, 1},
	{MUX_CLK_FSYS1_MMC_CARD, MUX_CLKCMU_FSYS1_MMC_CARD, EMPTY_CAL_ID, CLKCMU_FSYS1_MMC_CARD, MUX_CLKCMU_FSYS1_MMC_CARD_USER, mux_clk_fsys1_mmc_card_lut, 2},
};
struct vclk_switch switch_vdd_mif[] = {
	{CLKMUX_MIF_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, clkmux_mif_ddrphy2x_lut, 3},
};
struct vclk_switch switch_vdd_cam[] = {
	{MUX_CLK_AUD_CPU, MUX_CLKCMU_AUD_CPU, CLKCMU_AUD_CPU, GATE_CLKCMU_AUD_CPU, MUX_CLKCMU_AUD_CPU_USER, mux_clk_aud_cpu_lut, 5},
};
struct vclk_switch switch_vdd_cheetah[] = {
	{MUX_CLK_CPUCL2_PLL, MUX_CLKCMU_CPUCL2_SWITCH, CLKCMU_CPUCL2_SWITCH, GATE_CLKCMU_CPUCL2_SWITCH, MUX_CLKCMU_CPUCL2_SWITCH_USER, mux_clk_cpucl2_pll_lut, 3},
};
struct vclk_switch switch_vdd_prometheus[] = {
	{MUX_CLK_CPUCL1_PLL, MUX_CLKCMU_CPUCL1_SWITCH, CLKCMU_CPUCL1_SWITCH, GATE_CLKCMU_CPUCL1_SWITCH, MUX_CLKCMU_CPUCL1_SWITCH_USER, mux_clk_cpucl1_pll_lut, 3},
};
struct vclk_switch switch_blk_mif1[] = {
	{CLKMUX_MIF1_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, clkmux_mif1_ddrphy2x_lut, 3},
};
struct vclk_switch switch_blk_mif2[] = {
	{CLKMUX_MIF2_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, clkmux_mif2_ddrphy2x_lut, 3},
};
struct vclk_switch switch_blk_mif3[] = {
	{CLKMUX_MIF3_DDRPHY2X, MUX_CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, CLKCMU_MIF_SWITCH, EMPTY_CAL_ID, clkmux_mif3_ddrphy2x_lut, 3},
};

/*================================ VCLK List =================================*/
unsigned int cmucal_vclk_size = 925;
struct vclk cmucal_vclk_list[] = {

/* DVFS VCLK*/
	CMUCAL_VCLK(VCLK_VDDI, cmucal_vclk_vddi_lut, cmucal_vclk_vddi, NULL, switch_vddi),
	CMUCAL_VCLK(VCLK_VDD_MIF, cmucal_vclk_vdd_mif_lut, cmucal_vclk_vdd_mif, NULL, switch_vdd_mif),
	CMUCAL_VCLK(VCLK_VDD_CAM, cmucal_vclk_vdd_cam_lut, cmucal_vclk_vdd_cam, NULL, switch_vdd_cam),
	CMUCAL_VCLK(VCLK_VDD_CHEETAH, cmucal_vclk_vdd_cheetah_lut, cmucal_vclk_vdd_cheetah, NULL, switch_vdd_cheetah),
	CMUCAL_VCLK(VCLK_VDD_ANANKE, cmucal_vclk_vdd_ananke_lut, cmucal_vclk_vdd_ananke, NULL, NULL),
	CMUCAL_VCLK(VCLK_VDD_PROMETHEUS, cmucal_vclk_vdd_prometheus_lut, cmucal_vclk_vdd_prometheus, NULL, switch_vdd_prometheus),

/* SPECIAL VCLK*/
	CMUCAL_VCLK(VCLK_MUX_CLK_AUD_UAIF3, cmucal_vclk_mux_clk_aud_uaif3_lut, cmucal_vclk_mux_clk_aud_uaif3, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_BUC_CMUREF, cmucal_vclk_mux_buc_cmuref_lut, cmucal_vclk_mux_buc_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLK_CMGP_ADC, cmucal_vclk_mux_clk_cmgp_adc_lut, cmucal_vclk_mux_clk_cmgp_adc, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CMGP_BUS, cmucal_vclk_clkcmu_cmgp_bus_lut, cmucal_vclk_clkcmu_cmgp_bus, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_APM_BUS, cmucal_vclk_clkcmu_apm_bus_lut, cmucal_vclk_clkcmu_apm_bus, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CMU_CMUREF, cmucal_vclk_mux_cmu_cmuref_lut, cmucal_vclk_mux_cmu_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CORE_CMUREF, cmucal_vclk_mux_core_cmuref_lut, cmucal_vclk_mux_core_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CPUCL0_CMUREF, cmucal_vclk_mux_cpucl0_cmuref_lut, cmucal_vclk_mux_cpucl0_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CPUCL1_CMUREF, cmucal_vclk_mux_cpucl1_cmuref_lut, cmucal_vclk_mux_cpucl1_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CPUCL2_CMUREF, cmucal_vclk_mux_cpucl2_cmuref_lut, cmucal_vclk_mux_cpucl2_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_MIF_CMUREF, cmucal_vclk_mux_mif_cmuref_lut, cmucal_vclk_mux_mif_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_MIF1_CMUREF, cmucal_vclk_mux_mif1_cmuref_lut, cmucal_vclk_mux_mif1_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_MIF2_CMUREF, cmucal_vclk_mux_mif2_cmuref_lut, cmucal_vclk_mux_mif2_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_MIF3_CMUREF, cmucal_vclk_mux_mif3_cmuref_lut, cmucal_vclk_mux_mif3_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_FSYS0_DPGTC, cmucal_vclk_clkcmu_fsys0_dpgtc_lut, cmucal_vclk_clkcmu_fsys0_dpgtc, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLKCMU_FSYS0_PCIE, cmucal_vclk_mux_clkcmu_fsys0_pcie_lut, cmucal_vclk_mux_clkcmu_fsys0_pcie, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLKCMU_FSYS0A_USBDP_DEBUG, cmucal_vclk_mux_clkcmu_fsys0a_usbdp_debug_lut, cmucal_vclk_mux_clkcmu_fsys0a_usbdp_debug, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLKCMU_FSYS1_PCIE, cmucal_vclk_mux_clkcmu_fsys1_pcie_lut, cmucal_vclk_mux_clkcmu_fsys1_pcie, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_FSYS1_UFS_EMBD, cmucal_vclk_clkcmu_fsys1_ufs_embd_lut, cmucal_vclk_clkcmu_fsys1_ufs_embd, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_I2C_CMGP0, cmucal_vclk_div_clk_i2c_cmgp0_lut, cmucal_vclk_div_clk_i2c_cmgp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_USI_CMGP1, cmucal_vclk_div_clk_usi_cmgp1_lut, cmucal_vclk_div_clk_usi_cmgp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_USI_CMGP0, cmucal_vclk_div_clk_usi_cmgp0_lut, cmucal_vclk_div_clk_usi_cmgp0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_USI_CMGP2, cmucal_vclk_div_clk_usi_cmgp2_lut, cmucal_vclk_div_clk_usi_cmgp2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_USI_CMGP3, cmucal_vclk_div_clk_usi_cmgp3_lut, cmucal_vclk_div_clk_usi_cmgp3, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_I2C_CMGP1, cmucal_vclk_div_clk_i2c_cmgp1_lut, cmucal_vclk_div_clk_i2c_cmgp1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_I2C_CMGP2, cmucal_vclk_div_clk_i2c_cmgp2_lut, cmucal_vclk_div_clk_i2c_cmgp2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_I2C_CMGP3, cmucal_vclk_div_clk_i2c_cmgp3_lut, cmucal_vclk_div_clk_i2c_cmgp3, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_HPM, cmucal_vclk_clkcmu_hpm_lut, cmucal_vclk_clkcmu_hpm, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK0, cmucal_vclk_clkcmu_cis_clk0_lut, cmucal_vclk_clkcmu_cis_clk0, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK1, cmucal_vclk_clkcmu_cis_clk1_lut, cmucal_vclk_clkcmu_cis_clk1, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK2, cmucal_vclk_clkcmu_cis_clk2_lut, cmucal_vclk_clkcmu_cis_clk2, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK3, cmucal_vclk_clkcmu_cis_clk3_lut, cmucal_vclk_clkcmu_cis_clk3, NULL, NULL),
	CMUCAL_VCLK(VCLK_CLKCMU_CIS_CLK4, cmucal_vclk_clkcmu_cis_clk4_lut, cmucal_vclk_clkcmu_cis_clk4, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL0_CMUREF, cmucal_vclk_div_clk_cpucl0_cmuref_lut, cmucal_vclk_div_clk_cpucl0_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER0_PERIPHCLK, cmucal_vclk_div_clk_cluster0_periphclk_lut, cmucal_vclk_div_clk_cluster0_periphclk, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL1_CMUREF, cmucal_vclk_div_clk_cpucl1_cmuref_lut, cmucal_vclk_div_clk_cpucl1_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CPUCL2_CMUREF, cmucal_vclk_div_clk_cpucl2_cmuref_lut, cmucal_vclk_div_clk_cpucl2_cmuref, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_CLUSTER2_ATCLK, cmucal_vclk_div_clk_cluster2_atclk_lut, cmucal_vclk_div_clk_cluster2_atclk, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_FSYS1_MMC_CARD, cmucal_vclk_div_clk_fsys1_mmc_card_lut, cmucal_vclk_div_clk_fsys1_mmc_card, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_MIF_PRE, cmucal_vclk_div_clk_mif_pre_lut, cmucal_vclk_div_clk_mif_pre, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_MIF1_PRE, cmucal_vclk_div_clk_mif1_pre_lut, cmucal_vclk_div_clk_mif1_pre, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_MIF2_PRE, cmucal_vclk_div_clk_mif2_pre_lut, cmucal_vclk_div_clk_mif2_pre, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_MIF3_PRE, cmucal_vclk_div_clk_mif3_pre_lut, cmucal_vclk_div_clk_mif3_pre, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI00_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi00_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLKCMU_PERIC0_IP, cmucal_vclk_mux_clkcmu_peric0_ip_lut, cmucal_vclk_mux_clkcmu_peric0_ip, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI01_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi01_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI02_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi02_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI03_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi03_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI04_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi04_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI05_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi05_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_UART_DBG, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_uart_dbg, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI12_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi12_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI13_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi13_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI14_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi14_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC0_USI15_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric0_usi15_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_UART_BT, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_uart_bt, NULL, NULL),
	CMUCAL_VCLK(VCLK_MUX_CLKCMU_PERIC1_IP, cmucal_vclk_mux_clkcmu_peric1_ip_lut, cmucal_vclk_mux_clkcmu_peric1_ip, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI06_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi06_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI07_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi07_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI08_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi08_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_I2C_CAM0, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_i2c_cam0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_I2C_CAM1, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_i2c_cam1, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_I2C_CAM2, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_i2c_cam2, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_I2C_CAM3, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_i2c_cam3, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_SPI_CAM0, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_spi_cam0, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI09_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi09_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI10_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi10_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI11_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi11_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI16_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi16_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_PERIC1_USI17_USI, cmucal_vclk_div_clk_pericx_usixx_usi_lut, cmucal_vclk_div_clk_peric1_usi17_usi, NULL, NULL),
	CMUCAL_VCLK(VCLK_DIV_CLK_VTS_DMIC, cmucal_vclk_div_clk_vts_dmic_lut, cmucal_vclk_div_clk_vts_dmic, NULL, NULL),

/* COMMON VCLK*/
	CMUCAL_VCLK(VCLK_BLK_CMU, cmucal_vclk_blk_cmu_lut, cmucal_vclk_blk_cmu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CPUCL0, cmucal_vclk_blk_cpucl0_lut, cmucal_vclk_blk_cpucl0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MIF1, cmucal_vclk_blk_mif1_lut, cmucal_vclk_blk_mif1, NULL, switch_blk_mif1),
	CMUCAL_VCLK(VCLK_BLK_MIF2, cmucal_vclk_blk_mif2_lut, cmucal_vclk_blk_mif2, NULL, switch_blk_mif2),
	CMUCAL_VCLK(VCLK_BLK_MIF3, cmucal_vclk_blk_mif3_lut, cmucal_vclk_blk_mif3, NULL, switch_blk_mif3),
	CMUCAL_VCLK(VCLK_BLK_APM, cmucal_vclk_blk_apm_lut, cmucal_vclk_blk_apm, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DSPS, cmucal_vclk_blk_dsps_lut, cmucal_vclk_blk_dsps, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G3D, cmucal_vclk_blk_g3d_lut, cmucal_vclk_blk_g3d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_S2D, cmucal_vclk_blk_s2d_lut, cmucal_vclk_blk_s2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_VTS, cmucal_vclk_blk_vts_lut, cmucal_vclk_blk_vts, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_AUD, cmucal_vclk_blk_aud_lut, cmucal_vclk_blk_aud, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_BUSC, cmucal_vclk_blk_busc_lut, cmucal_vclk_blk_busc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CMGP, cmucal_vclk_blk_cmgp_lut, cmucal_vclk_blk_cmgp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CORE, cmucal_vclk_blk_core_lut, cmucal_vclk_blk_core, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_CPUCL2, cmucal_vclk_blk_cpucl2_lut, cmucal_vclk_blk_cpucl2, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DPU, cmucal_vclk_blk_dpu_lut, cmucal_vclk_blk_dpu, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_DSPM, cmucal_vclk_blk_dspm_lut, cmucal_vclk_blk_dspm, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_G2D, cmucal_vclk_blk_g2d_lut, cmucal_vclk_blk_g2d, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPHQ, cmucal_vclk_blk_isphq_lut, cmucal_vclk_blk_isphq, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPLP, cmucal_vclk_blk_isplp_lut, cmucal_vclk_blk_isplp, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_ISPPRE, cmucal_vclk_blk_isppre_lut, cmucal_vclk_blk_isppre, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_IVA, cmucal_vclk_blk_iva_lut, cmucal_vclk_blk_iva, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_MFC, cmucal_vclk_blk_mfc_lut, cmucal_vclk_blk_mfc, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_NPU1, cmucal_vclk_blk_npu1_lut, cmucal_vclk_blk_npu1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_PERIC0, cmucal_vclk_blk_peric0_lut, cmucal_vclk_blk_peric0, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_PERIC1, cmucal_vclk_blk_peric1_lut, cmucal_vclk_blk_peric1, NULL, NULL),
	CMUCAL_VCLK(VCLK_BLK_VRA2, cmucal_vclk_blk_vra2_lut, cmucal_vclk_blk_vra2, NULL, NULL),

/* GATE VCLK*/
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_APM, NULL, cmucal_vclk_ip_lhs_axi_d_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_APM, NULL, cmucal_vclk_ip_lhm_axi_p_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WDT_APM, NULL, cmucal_vclk_ip_wdt_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_APM, NULL, cmucal_vclk_ip_sysreg_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_APM_AP, NULL, cmucal_vclk_ip_mailbox_apm_ap, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBIF_PMU_ALIVE, NULL, cmucal_vclk_ip_apbif_pmu_alive, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_INTMEM, NULL, cmucal_vclk_ip_intmem, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_C_MODEM, NULL, cmucal_vclk_ip_lhm_axi_c_modem, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G_SCAN2DRAM, NULL, cmucal_vclk_ip_lhs_axi_g_scan2dram, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PMU_INTR_GEN, NULL, cmucal_vclk_ip_pmu_intr_gen, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PEM, NULL, cmucal_vclk_ip_pem, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SPEEDY_APM, NULL, cmucal_vclk_ip_speedy_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_DP_APM, NULL, cmucal_vclk_ip_xiu_dp_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APM_CMU_APM, NULL, cmucal_vclk_ip_apm_cmu_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_APM, NULL, cmucal_vclk_ip_vgen_lite_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GREBEINTEGRATION, NULL, cmucal_vclk_ip_grebeintegration, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBIF_GPIO_ALIVE, NULL, cmucal_vclk_ip_apbif_gpio_alive, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBIF_TOP_RTC, NULL, cmucal_vclk_ip_apbif_top_rtc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_AP_CP, NULL, cmucal_vclk_ip_mailbox_ap_cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_AP_CP_S, NULL, cmucal_vclk_ip_mailbox_ap_cp_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GREBEINTEGRATION_DBGCORE, NULL, cmucal_vclk_ip_grebeintegration_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DTZPC_APM, NULL, cmucal_vclk_ip_dtzpc_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_C_VTS, NULL, cmucal_vclk_ip_lhm_axi_c_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_APM_VTS, NULL, cmucal_vclk_ip_mailbox_apm_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_AP_DBGCORE, NULL, cmucal_vclk_ip_mailbox_ap_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_LP_VTS, NULL, cmucal_vclk_ip_lhs_axi_lp_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_APM_CP, NULL, cmucal_vclk_ip_mailbox_apm_cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G_DBGCORE, NULL, cmucal_vclk_ip_lhs_axi_g_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBIF_RTC, NULL, cmucal_vclk_ip_apbif_rtc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_C_CMGP, NULL, cmucal_vclk_ip_lhs_axi_c_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SPEEDY_SUB_APM, NULL, cmucal_vclk_ip_speedy_sub_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AUD_CMU_AUD, NULL, cmucal_vclk_ip_aud_cmu_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_AUD, NULL, cmucal_vclk_ip_lhs_axi_d_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_AUD, NULL, cmucal_vclk_ip_ppmu_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_AUD, NULL, cmucal_vclk_ip_sysreg_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ABOX, NULL, cmucal_vclk_ip_abox, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T0_AUD, NULL, cmucal_vclk_ip_lhs_atb_t0_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_AUD, NULL, cmucal_vclk_ip_gpio_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI_US_32TO128, NULL, cmucal_vclk_ip_axi_us_32to128, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_AUD, NULL, cmucal_vclk_ip_btm_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PERI_AXI_ASB, NULL, cmucal_vclk_ip_peri_axi_asb, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_AUD, NULL, cmucal_vclk_ip_lhm_axi_p_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WDT_AUD, NULL, cmucal_vclk_ip_wdt_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC, NULL, cmucal_vclk_ip_dmic, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_AUD, NULL, cmucal_vclk_ip_trex_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DFTMUX_AUD, NULL, cmucal_vclk_ip_dftmux_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SMMU_AUD, NULL, cmucal_vclk_ip_smmu_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WRAP2_CONV_AUD, NULL, cmucal_vclk_ip_wrap2_conv_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_AUD, NULL, cmucal_vclk_ip_xiu_p_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SMMU_AUD, NULL, cmucal_vclk_ip_ad_apb_smmu_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_AUD, NULL, cmucal_vclk_ip_axi2apb_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SMMU_AUD_S, NULL, cmucal_vclk_ip_ad_apb_smmu_aud_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T1_AUD, NULL, cmucal_vclk_ip_lhs_atb_t1_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_AUD, NULL, cmucal_vclk_ip_vgen_lite_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSC_CMU_BUSC, NULL, cmucal_vclk_ip_busc_cmu_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_BUSCP0, NULL, cmucal_vclk_ip_axi2apb_buscp0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_BUSC_TDP, NULL, cmucal_vclk_ip_axi2apb_busc_tdp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_BUSC, NULL, cmucal_vclk_ip_sysreg_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_CMUTOPC, NULL, cmucal_vclk_ip_busif_cmutopc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_D0_BUSC, NULL, cmucal_vclk_ip_trex_d0_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_P_BUSC, NULL, cmucal_vclk_ip_trex_p_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_MIF0, NULL, cmucal_vclk_ip_lhs_axi_p_mif0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_MIF1, NULL, cmucal_vclk_ip_lhs_axi_p_mif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_MIF2, NULL, cmucal_vclk_ip_lhs_axi_p_mif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_MIF3, NULL, cmucal_vclk_ip_lhs_axi_p_mif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_PERIS, NULL, cmucal_vclk_ip_lhs_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_PERIC0, NULL, cmucal_vclk_ip_lhs_axi_p_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_PERIC1, NULL, cmucal_vclk_ip_lhs_axi_p_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ASYNCSFR_WR_SMC, NULL, cmucal_vclk_ip_asyncsfr_wr_smc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_IVASC, NULL, cmucal_vclk_ip_lhs_axi_d_ivasc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D0_G2D, NULL, cmucal_vclk_ip_lhm_acel_d0_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D1_G2D, NULL, cmucal_vclk_ip_lhm_acel_d1_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D2_G2D, NULL, cmucal_vclk_ip_lhm_acel_d2_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D_FSYS0, NULL, cmucal_vclk_ip_lhm_acel_d_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D_IVA, NULL, cmucal_vclk_ip_lhm_acel_d_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D_NPU, NULL, cmucal_vclk_ip_lhm_acel_d_npu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D0_DPU, NULL, cmucal_vclk_ip_lhm_axi_d0_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D0_MFC, NULL, cmucal_vclk_ip_lhm_axi_d0_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_ISPPRE, NULL, cmucal_vclk_ip_lhm_axi_d_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D1_DPU, NULL, cmucal_vclk_ip_lhm_axi_d1_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D1_MFC, NULL, cmucal_vclk_ip_lhm_axi_d1_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D2_DPU, NULL, cmucal_vclk_ip_lhm_axi_d2_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D0_ISPLP, NULL, cmucal_vclk_ip_lhm_axi_d0_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_DPU, NULL, cmucal_vclk_ip_lhs_axi_p_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_ISPPRE, NULL, cmucal_vclk_ip_lhs_axi_p_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_DSPM, NULL, cmucal_vclk_ip_lhs_axi_p_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_FSYS0, NULL, cmucal_vclk_ip_lhs_axi_p_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_G2D, NULL, cmucal_vclk_ip_lhs_axi_p_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_ISPHQ, NULL, cmucal_vclk_ip_lhs_axi_p_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_ISPLP, NULL, cmucal_vclk_ip_lhs_axi_p_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_IVA, NULL, cmucal_vclk_ip_lhs_axi_p_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_MFC, NULL, cmucal_vclk_ip_lhs_axi_p_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D_FSYS1, NULL, cmucal_vclk_ip_lhm_acel_d_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_APM, NULL, cmucal_vclk_ip_lhm_axi_d_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D1_ISPLP, NULL, cmucal_vclk_ip_lhm_axi_d1_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_FSYS1, NULL, cmucal_vclk_ip_lhs_axi_p_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SIREX, NULL, cmucal_vclk_ip_sirex, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D0_DSPM, NULL, cmucal_vclk_ip_lhm_acel_d0_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACEL_D1_DSPM, NULL, cmucal_vclk_ip_lhm_acel_d1_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_ISPHQ, NULL, cmucal_vclk_ip_lhm_axi_d_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_RB_BUSC, NULL, cmucal_vclk_ip_trex_rb_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPFW, NULL, cmucal_vclk_ip_ppfw, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WRAP2_CONV_BUSC, NULL, cmucal_vclk_ip_wrap2_conv_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_PDMA0, NULL, cmucal_vclk_ip_vgen_pdma0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_BUSC, NULL, cmucal_vclk_ip_vgen_lite_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_BUSC, NULL, cmucal_vclk_ip_hpm_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMBUSC, NULL, cmucal_vclk_ip_busif_hpmbusc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PDMA0, NULL, cmucal_vclk_ip_pdma0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SBIC, NULL, cmucal_vclk_ip_sbic, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SPDMA, NULL, cmucal_vclk_ip_spdma, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DIT, NULL, cmucal_vclk_ip_ad_apb_dit, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DIT, NULL, cmucal_vclk_ip_dit, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_BUSC, NULL, cmucal_vclk_ip_d_tzpc_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_NPU, NULL, cmucal_vclk_ip_lhs_axi_p_npu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MMCACHE, NULL, cmucal_vclk_ip_mmcache, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_D1_BUSC, NULL, cmucal_vclk_ip_trex_d1_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_BUSCP1, NULL, cmucal_vclk_ip_axi2apb_buscp1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_AUD, NULL, cmucal_vclk_ip_lhm_axi_d_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_AUD, NULL, cmucal_vclk_ip_lhs_axi_p_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_DBG_G_BUSC, NULL, cmucal_vclk_ip_lhs_dbg_g_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_VTS, NULL, cmucal_vclk_ip_lhm_axi_d_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_VTS, NULL, cmucal_vclk_ip_lhs_axi_p_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_SPDMA, NULL, cmucal_vclk_ip_qe_spdma, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_PDMA0, NULL, cmucal_vclk_ip_qe_pdma0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_BUSC, NULL, cmucal_vclk_ip_xiu_d_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_P_VTS, NULL, cmucal_vclk_ip_baaw_p_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI_US_64TO128, NULL, cmucal_vclk_ip_axi_us_64to128, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_P_NPU, NULL, cmucal_vclk_ip_baaw_p_npu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_VRA2, NULL, cmucal_vclk_ip_lhm_axi_d_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CMGP_CMU_CMGP, NULL, cmucal_vclk_ip_cmgp_cmu_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ADC_CMGP, NULL, cmucal_vclk_ip_adc_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_CMGP, NULL, cmucal_vclk_ip_gpio_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CMGP0, NULL, cmucal_vclk_ip_i2c_cmgp0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CMGP1, NULL, cmucal_vclk_ip_i2c_cmgp1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CMGP2, NULL, cmucal_vclk_ip_i2c_cmgp2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CMGP3, NULL, cmucal_vclk_ip_i2c_cmgp3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CMGP, NULL, cmucal_vclk_ip_sysreg_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI_CMGP0, NULL, cmucal_vclk_ip_usi_cmgp0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI_CMGP1, NULL, cmucal_vclk_ip_usi_cmgp1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI_CMGP2, NULL, cmucal_vclk_ip_usi_cmgp2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI_CMGP3, NULL, cmucal_vclk_ip_usi_cmgp3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CMGP2CP, NULL, cmucal_vclk_ip_sysreg_cmgp2cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CMGP2PMU_AP, NULL, cmucal_vclk_ip_sysreg_cmgp2pmu_ap, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DTZPC_CMGP, NULL, cmucal_vclk_ip_dtzpc_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_C_CMGP, NULL, cmucal_vclk_ip_lhm_axi_c_cmgp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CMGP2APM, NULL, cmucal_vclk_ip_sysreg_cmgp2apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CORE_CMU_CORE, NULL, cmucal_vclk_ip_core_cmu_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CORE, NULL, cmucal_vclk_ip_sysreg_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_CORE_0, NULL, cmucal_vclk_ip_axi2apb_core_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MPACE2AXI_0, NULL, cmucal_vclk_ip_mpace2axi_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MPACE2AXI_1, NULL, cmucal_vclk_ip_mpace2axi_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_DEBUG_CCI, NULL, cmucal_vclk_ip_ppc_debug_cci, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_P0_CORE, NULL, cmucal_vclk_ip_trex_p0_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_CPUCL2_0, NULL, cmucal_vclk_ip_ppmu_cpucl2_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_DBG_G0_DMC, NULL, cmucal_vclk_ip_lhm_dbg_g0_dmc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_DBG_G1_DMC, NULL, cmucal_vclk_ip_lhm_dbg_g1_dmc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_DBG_G2_DMC, NULL, cmucal_vclk_ip_lhm_dbg_g2_dmc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_DBG_G3_DMC, NULL, cmucal_vclk_ip_lhm_dbg_g3_dmc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T_BDU, NULL, cmucal_vclk_ip_lhs_atb_t_bdu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ADM_APB_G_BDU, NULL, cmucal_vclk_ip_adm_apb_g_bdu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BDU, NULL, cmucal_vclk_ip_bdu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_P1_CORE, NULL, cmucal_vclk_ip_trex_p1_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_CORE_TP, NULL, cmucal_vclk_ip_axi2apb_core_tp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPFW_G3D, NULL, cmucal_vclk_ip_ppfw_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_G3D, NULL, cmucal_vclk_ip_lhs_axi_p_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_CPUCL0, NULL, cmucal_vclk_ip_lhs_axi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_CPUCL2, NULL, cmucal_vclk_ip_lhs_axi_p_cpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D0_CP, NULL, cmucal_vclk_ip_lhm_axi_d0_cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACE_D0_G3D, NULL, cmucal_vclk_ip_lhm_ace_d0_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACE_D1_G3D, NULL, cmucal_vclk_ip_lhm_ace_d1_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACE_D2_G3D, NULL, cmucal_vclk_ip_lhm_ace_d2_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACE_D3_G3D, NULL, cmucal_vclk_ip_lhm_ace_d3_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_D_CORE, NULL, cmucal_vclk_ip_trex_d_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_CORE, NULL, cmucal_vclk_ip_hpm_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMCORE, NULL, cmucal_vclk_ip_busif_hpmcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_D0_G3D, NULL, cmucal_vclk_ip_bps_d0_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_D1_G3D, NULL, cmucal_vclk_ip_bps_d1_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_D2_G3D, NULL, cmucal_vclk_ip_bps_d2_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_D3_G3D, NULL, cmucal_vclk_ip_bps_d3_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPCFW_G3D, NULL, cmucal_vclk_ip_ppcfw_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_CP, NULL, cmucal_vclk_ip_lhs_axi_p_cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APB_ASYNC_PPFW_G3D, NULL, cmucal_vclk_ip_apb_async_ppfw_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_CP, NULL, cmucal_vclk_ip_baaw_cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_P_G3D, NULL, cmucal_vclk_ip_bps_p_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_APM, NULL, cmucal_vclk_ip_lhs_axi_p_apm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_CPUCL2_1, NULL, cmucal_vclk_ip_ppmu_cpucl2_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_CORE, NULL, cmucal_vclk_ip_d_tzpc_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_CORE_1, NULL, cmucal_vclk_ip_axi2apb_core_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_CORE, NULL, cmucal_vclk_ip_xiu_p_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_CPUCL2_0, NULL, cmucal_vclk_ip_ppc_cpucl2_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_CPUCL2_1, NULL, cmucal_vclk_ip_ppc_cpucl2_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_G3D0, NULL, cmucal_vclk_ip_ppc_g3d0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_G3D1, NULL, cmucal_vclk_ip_ppc_g3d1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_G3D2, NULL, cmucal_vclk_ip_ppc_g3d2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_G3D3, NULL, cmucal_vclk_ip_ppc_g3d3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_IRPS0, NULL, cmucal_vclk_ip_ppc_irps0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_IRPS1, NULL, cmucal_vclk_ip_ppc_irps1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D1_CP, NULL, cmucal_vclk_ip_lhm_axi_d1_cp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_L_CORE, NULL, cmucal_vclk_ip_lhs_axi_l_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_CORE_2, NULL, cmucal_vclk_ip_axi2apb_core_2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_L_CORE, NULL, cmucal_vclk_ip_lhm_axi_l_core, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACE_D0_CLUSTER0, NULL, cmucal_vclk_ip_lhm_ace_d0_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ACE_D1_CLUSTER0, NULL, cmucal_vclk_ip_lhm_ace_d1_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_CPUCL0_0, NULL, cmucal_vclk_ip_ppc_cpucl0_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_CPUCL0_1, NULL, cmucal_vclk_ip_ppc_cpucl0_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_CPUCL0_0, NULL, cmucal_vclk_ip_ppmu_cpucl0_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_CPUCL0_1, NULL, cmucal_vclk_ip_ppmu_cpucl0_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_DBG_G_BUSC, NULL, cmucal_vclk_ip_lhm_dbg_g_busc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MPACE_ASB_D0_MIF, NULL, cmucal_vclk_ip_mpace_asb_d0_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MPACE_ASB_D1_MIF, NULL, cmucal_vclk_ip_mpace_asb_d1_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MPACE_ASB_D2_MIF, NULL, cmucal_vclk_ip_mpace_asb_d2_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MPACE_ASB_D3_MIF, NULL, cmucal_vclk_ip_mpace_asb_d3_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI_ASB_CSSYS, NULL, cmucal_vclk_ip_axi_asb_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_G_CSSYS, NULL, cmucal_vclk_ip_lhm_axi_g_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CCI, NULL, cmucal_vclk_ip_cci, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_CPUCL0, NULL, cmucal_vclk_ip_axi2apb_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CPUCL0, NULL, cmucal_vclk_ip_sysreg_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMCPUCL0, NULL, cmucal_vclk_ip_busif_hpmcpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CSSYS, NULL, cmucal_vclk_ip_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T0_AUD, NULL, cmucal_vclk_ip_lhm_atb_t0_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T_BDU, NULL, cmucal_vclk_ip_lhm_atb_t_bdu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T0_CLUSTER0, NULL, cmucal_vclk_ip_lhm_atb_t0_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T0_CLUSTER2, NULL, cmucal_vclk_ip_lhm_atb_t0_cluster2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T1_CLUSTER0, NULL, cmucal_vclk_ip_lhm_atb_t1_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T1_CLUSTER2, NULL, cmucal_vclk_ip_lhm_atb_t1_cluster2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T2_CLUSTER0, NULL, cmucal_vclk_ip_lhm_atb_t2_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T3_CLUSTER0, NULL, cmucal_vclk_ip_lhm_atb_t3_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SECJTAG, NULL, cmucal_vclk_ip_secjtag, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_CPUCL0, NULL, cmucal_vclk_ip_lhm_axi_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACE_D0_CLUSTER0, NULL, cmucal_vclk_ip_lhs_ace_d0_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T0_CLUSTER0, NULL, cmucal_vclk_ip_lhs_atb_t0_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T1_CLUSTER0, NULL, cmucal_vclk_ip_lhs_atb_t1_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T2_CLUSTER0, NULL, cmucal_vclk_ip_lhs_atb_t2_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T3_CLUSTER0, NULL, cmucal_vclk_ip_lhs_atb_t3_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ADM_APB_G_CLUSTER0, NULL, cmucal_vclk_ip_adm_apb_g_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CPUCL0_CMU_CPUCL0, NULL, cmucal_vclk_ip_cpucl0_cmu_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CPUCL0, NULL, cmucal_vclk_ip_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T4_CLUSTER0, NULL, cmucal_vclk_ip_lhm_atb_t4_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T5_CLUSTER0, NULL, cmucal_vclk_ip_lhm_atb_t5_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACE_D1_CLUSTER0, NULL, cmucal_vclk_ip_lhs_ace_d1_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T4_CLUSTER0, NULL, cmucal_vclk_ip_lhs_atb_t4_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_T5_CLUSTER0, NULL, cmucal_vclk_ip_lhs_atb_t5_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_CPUCL0, NULL, cmucal_vclk_ip_d_tzpc_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_T1_AUD, NULL, cmucal_vclk_ip_lhm_atb_t1_aud, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G_INT_CSSYS, NULL, cmucal_vclk_ip_lhs_axi_g_int_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_G_INT_CSSYS, NULL, cmucal_vclk_ip_lhm_axi_g_int_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G_INT_DBGCORE, NULL, cmucal_vclk_ip_lhs_axi_g_int_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_G_INT_DBGCORE, NULL, cmucal_vclk_ip_lhm_axi_g_int_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_CPUCL0, NULL, cmucal_vclk_ip_xiu_p_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_DP_CSSYS, NULL, cmucal_vclk_ip_xiu_dp_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_CPUCL0, NULL, cmucal_vclk_ip_trex_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI_US_32TO64_G_DBGCORE, NULL, cmucal_vclk_ip_axi_us_32to64_g_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G_CSSYS, NULL, cmucal_vclk_ip_lhs_axi_g_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_CPUCL0_1, NULL, cmucal_vclk_ip_hpm_cpucl0_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_CPUCL0_0, NULL, cmucal_vclk_ip_hpm_cpucl0_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APB_ASYNC_P_CSSYS_0, NULL, cmucal_vclk_ip_apb_async_p_cssys_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G_INT_ETR, NULL, cmucal_vclk_ip_lhs_axi_g_int_etr, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_G_DBGCORE, NULL, cmucal_vclk_ip_lhm_axi_g_dbgcore, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_G_INT_ETR, NULL, cmucal_vclk_ip_lhm_axi_g_int_etr, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_P_CSSYS, NULL, cmucal_vclk_ip_axi2apb_p_cssys, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_CPUCL0, NULL, cmucal_vclk_ip_bps_cpucl0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CPUCL1_CMU_CPUCL1, NULL, cmucal_vclk_ip_cpucl1_cmu_cpucl1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CPUCL1, NULL, cmucal_vclk_ip_cpucl1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CPUCL2_CMU_CPUCL2, NULL, cmucal_vclk_ip_cpucl2_cmu_cpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_CPUCL2, NULL, cmucal_vclk_ip_sysreg_cpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMCPUCL2, NULL, cmucal_vclk_ip_busif_hpmcpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_CPUCL2_0, NULL, cmucal_vclk_ip_hpm_cpucl2_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CLUSTER2, NULL, cmucal_vclk_ip_cluster2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_CPUCL2, NULL, cmucal_vclk_ip_axi2apb_cpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_CPUCL2, NULL, cmucal_vclk_ip_lhm_axi_p_cpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_CPUCL2_1, NULL, cmucal_vclk_ip_hpm_cpucl2_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_CPUCL2_2, NULL, cmucal_vclk_ip_hpm_cpucl2_2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_CPUCL2, NULL, cmucal_vclk_ip_d_tzpc_cpucl2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DPU_CMU_DPU, NULL, cmucal_vclk_ip_dpu_cmu_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_DPUD0, NULL, cmucal_vclk_ip_btm_dpud0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_DPUD1, NULL, cmucal_vclk_ip_btm_dpud1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_DPU, NULL, cmucal_vclk_ip_sysreg_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_DPUP1, NULL, cmucal_vclk_ip_axi2apb_dpup1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_DPUP0, NULL, cmucal_vclk_ip_axi2apb_dpup0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_DPUD0, NULL, cmucal_vclk_ip_sysmmu_dpud0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_DPU, NULL, cmucal_vclk_ip_lhm_axi_p_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D1_DPU, NULL, cmucal_vclk_ip_lhs_axi_d1_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_DPU, NULL, cmucal_vclk_ip_xiu_p_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DECON0, NULL, cmucal_vclk_ip_ad_apb_decon0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DECON1, NULL, cmucal_vclk_ip_ad_apb_decon1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_MIPI_DSIM1, NULL, cmucal_vclk_ip_ad_apb_mipi_dsim1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DPP, NULL, cmucal_vclk_ip_ad_apb_dpp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D2_DPU, NULL, cmucal_vclk_ip_lhs_axi_d2_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_DPUD2, NULL, cmucal_vclk_ip_btm_dpud2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_DPUD2, NULL, cmucal_vclk_ip_sysmmu_dpud2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DPU_DMA, NULL, cmucal_vclk_ip_ad_apb_dpu_dma, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DPU_WB_MUX, NULL, cmucal_vclk_ip_ad_apb_dpu_wb_mux, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_DPUD1, NULL, cmucal_vclk_ip_sysmmu_dpud1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_DPUD0, NULL, cmucal_vclk_ip_ppmu_dpud0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_DPUD1, NULL, cmucal_vclk_ip_ppmu_dpud1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_DPUD2, NULL, cmucal_vclk_ip_ppmu_dpud2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_MIPI_DSIM0, NULL, cmucal_vclk_ip_ad_apb_mipi_dsim0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DECON2, NULL, cmucal_vclk_ip_ad_apb_decon2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SYSMMU_DPUD0, NULL, cmucal_vclk_ip_ad_apb_sysmmu_dpud0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SYSMMU_DPUD0_S, NULL, cmucal_vclk_ip_ad_apb_sysmmu_dpud0_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SYSMMU_DPUD1, NULL, cmucal_vclk_ip_ad_apb_sysmmu_dpud1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SYSMMU_DPUD1_S, NULL, cmucal_vclk_ip_ad_apb_sysmmu_dpud1_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SYSMMU_DPUD2, NULL, cmucal_vclk_ip_ad_apb_sysmmu_dpud2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_SYSMMU_DPUD2_S, NULL, cmucal_vclk_ip_ad_apb_sysmmu_dpud2_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DPU, NULL, cmucal_vclk_ip_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WRAPPER_FOR_S5I6280_HSI_DCPHY_COMBO_TOP, NULL, cmucal_vclk_ip_wrapper_for_s5i6280_hsi_dcphy_combo_top, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DPU_DMA_PGEN, NULL, cmucal_vclk_ip_ad_apb_dpu_dma_pgen, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D0_DPU, NULL, cmucal_vclk_ip_lhs_axi_d0_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_DPU, NULL, cmucal_vclk_ip_d_tzpc_dpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_MCD, NULL, cmucal_vclk_ip_ad_apb_mcd, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DSPM_CMU_DSPM, NULL, cmucal_vclk_ip_dspm_cmu_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_DSPM, NULL, cmucal_vclk_ip_sysreg_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_DSPM, NULL, cmucal_vclk_ip_axi2apb_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_DSPM0, NULL, cmucal_vclk_ip_ppmu_dspm0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_DSPM0, NULL, cmucal_vclk_ip_sysmmu_dspm0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_DSPM0, NULL, cmucal_vclk_ip_btm_dspm0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_DSPM, NULL, cmucal_vclk_ip_lhm_axi_p_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D0_DSPM, NULL, cmucal_vclk_ip_lhs_acel_d0_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_IVADSPM, NULL, cmucal_vclk_ip_lhm_axi_p_ivadspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_DSPMIVA, NULL, cmucal_vclk_ip_lhs_axi_p_dspmiva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WRAP2_CONV_DSPM, NULL, cmucal_vclk_ip_wrap2_conv_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DSPM0, NULL, cmucal_vclk_ip_ad_apb_dspm0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DSPM1, NULL, cmucal_vclk_ip_ad_apb_dspm1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DSPM3, NULL, cmucal_vclk_ip_ad_apb_dspm3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_AXI_DSPM0, NULL, cmucal_vclk_ip_ad_axi_dspm0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_DSPM1, NULL, cmucal_vclk_ip_btm_dspm1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D1_DSPM, NULL, cmucal_vclk_ip_lhs_acel_d1_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_DSPMDSPS, NULL, cmucal_vclk_ip_lhs_axi_p_dspmdsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_DSPM1, NULL, cmucal_vclk_ip_ppmu_dspm1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_DSPM1, NULL, cmucal_vclk_ip_sysmmu_dspm1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ADM_APB_DSPM, NULL, cmucal_vclk_ip_adm_apb_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D0_DSPSDSPM, NULL, cmucal_vclk_ip_lhm_axi_d0_dspsdspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_DSPM, NULL, cmucal_vclk_ip_xiu_p_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_DSPM, NULL, cmucal_vclk_ip_vgen_lite_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_DSPM2, NULL, cmucal_vclk_ip_ad_apb_dspm2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SCORE_TS_II, NULL, cmucal_vclk_ip_score_ts_ii, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_DSPM, NULL, cmucal_vclk_ip_d_tzpc_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_ISPPREDSPM, NULL, cmucal_vclk_ip_lhm_ast_isppredspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_ISPLPDSPM, NULL, cmucal_vclk_ip_lhm_ast_isplpdspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_ISPHQDSPM, NULL, cmucal_vclk_ip_lhm_ast_isphqdspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_DSPMISPPRE, NULL, cmucal_vclk_ip_lhs_ast_dspmisppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_DSPMISPLP, NULL, cmucal_vclk_ip_lhs_ast_dspmisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_DSPM, NULL, cmucal_vclk_ip_xiu_d_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_DSPM, NULL, cmucal_vclk_ip_baaw_dspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_DSPMNPU0, NULL, cmucal_vclk_ip_lhs_axi_d_dspmnpu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DSPS_CMU_DSPS, NULL, cmucal_vclk_ip_dsps_cmu_dsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_DSPS, NULL, cmucal_vclk_ip_axi2apb_dsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_DSPMDSPS, NULL, cmucal_vclk_ip_lhm_axi_p_dspmdsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_DSPS, NULL, cmucal_vclk_ip_sysreg_dsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_DSPSIVA, NULL, cmucal_vclk_ip_lhs_axi_d_dspsiva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D0_DSPSDSPM, NULL, cmucal_vclk_ip_lhs_axi_d0_dspsdspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SCORE_BARON, NULL, cmucal_vclk_ip_score_baron, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_IVADSPS, NULL, cmucal_vclk_ip_lhm_axi_d_ivadsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_DSPS, NULL, cmucal_vclk_ip_d_tzpc_dsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_DSPS, NULL, cmucal_vclk_ip_vgen_lite_dsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_FSYS0_CMU_FSYS0, NULL, cmucal_vclk_ip_fsys0_cmu_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D_FSYS0, NULL, cmucal_vclk_ip_lhs_acel_d_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_FSYS0, NULL, cmucal_vclk_ip_lhm_axi_p_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_FSYS0, NULL, cmucal_vclk_ip_gpio_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_FSYS0, NULL, cmucal_vclk_ip_sysreg_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_FSYS0, NULL, cmucal_vclk_ip_xiu_d_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_FSYS0, NULL, cmucal_vclk_ip_btm_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DP_LINK, NULL, cmucal_vclk_ip_dp_link, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_FSYS0, NULL, cmucal_vclk_ip_vgen_lite_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_USB, NULL, cmucal_vclk_ip_lhm_axi_d_usb, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_USB, NULL, cmucal_vclk_ip_lhs_axi_p_usb, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_FSYS0, NULL, cmucal_vclk_ip_ppmu_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_PCIE_GEN3A, NULL, cmucal_vclk_ip_sysmmu_pcie_gen3a, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_PCIE_GEN3B, NULL, cmucal_vclk_ip_sysmmu_pcie_gen3b, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P0_FSYS0, NULL, cmucal_vclk_ip_xiu_p0_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PCIE_GEN3, NULL, cmucal_vclk_ip_pcie_gen3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PCIE_IA_GEN3A, NULL, cmucal_vclk_ip_pcie_ia_gen3a, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PCIE_IA_GEN3B, NULL, cmucal_vclk_ip_pcie_ia_gen3b, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_FSYS0, NULL, cmucal_vclk_ip_d_tzpc_fsys0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_FSYS0A_CMU_FSYS0A, NULL, cmucal_vclk_ip_fsys0a_cmu_fsys0a, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USB31DRD, NULL, cmucal_vclk_ip_usb31drd, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_USB, NULL, cmucal_vclk_ip_lhm_axi_p_usb, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_USB, NULL, cmucal_vclk_ip_lhs_axi_d_usb, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_FSYS1_CMU_FSYS1, NULL, cmucal_vclk_ip_fsys1_cmu_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MMC_CARD, NULL, cmucal_vclk_ip_mmc_card, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PCIE_GEN2, NULL, cmucal_vclk_ip_pcie_gen2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SSS, NULL, cmucal_vclk_ip_sss, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_RTIC, NULL, cmucal_vclk_ip_rtic, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_FSYS1, NULL, cmucal_vclk_ip_sysreg_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_FSYS1, NULL, cmucal_vclk_ip_gpio_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D_FSYS1, NULL, cmucal_vclk_ip_lhs_acel_d_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_FSYS1, NULL, cmucal_vclk_ip_lhm_axi_p_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_FSYS1, NULL, cmucal_vclk_ip_xiu_d_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_FSYS1, NULL, cmucal_vclk_ip_xiu_p_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_FSYS1, NULL, cmucal_vclk_ip_ppmu_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_FSYS1, NULL, cmucal_vclk_ip_btm_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_UFS_CARD, NULL, cmucal_vclk_ip_ufs_card, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ADM_AHB_SSS, NULL, cmucal_vclk_ip_adm_ahb_sss, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_FSYS1, NULL, cmucal_vclk_ip_sysmmu_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_FSYS1, NULL, cmucal_vclk_ip_vgen_lite_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PCIE_IA_GEN2, NULL, cmucal_vclk_ip_pcie_ia_gen2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_FSYS1, NULL, cmucal_vclk_ip_d_tzpc_fsys1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_UFS_EMBD, NULL, cmucal_vclk_ip_ufs_embd, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PUF, NULL, cmucal_vclk_ip_puf, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_RTIC, NULL, cmucal_vclk_ip_qe_rtic, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_SSS, NULL, cmucal_vclk_ip_qe_sss, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_SSS, NULL, cmucal_vclk_ip_baaw_sss, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_G2D_CMU_G2D, NULL, cmucal_vclk_ip_g2d_cmu_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_G2DD0, NULL, cmucal_vclk_ip_ppmu_g2dd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_G2DD1, NULL, cmucal_vclk_ip_ppmu_g2dd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_G2DD0, NULL, cmucal_vclk_ip_sysmmu_g2dd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_G2D, NULL, cmucal_vclk_ip_sysreg_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D0_G2D, NULL, cmucal_vclk_ip_lhs_acel_d0_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D1_G2D, NULL, cmucal_vclk_ip_lhs_acel_d1_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_G2D, NULL, cmucal_vclk_ip_lhm_axi_p_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_G2D, NULL, cmucal_vclk_ip_as_p_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_G2DP0, NULL, cmucal_vclk_ip_axi2apb_g2dp0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_G2DD0, NULL, cmucal_vclk_ip_btm_g2dd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_G2DD1, NULL, cmucal_vclk_ip_btm_g2dd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_G2D, NULL, cmucal_vclk_ip_xiu_p_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_G2DP1, NULL, cmucal_vclk_ip_axi2apb_g2dp1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_G2DD2, NULL, cmucal_vclk_ip_btm_g2dd2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_JPEG, NULL, cmucal_vclk_ip_qe_jpeg, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_MSCL, NULL, cmucal_vclk_ip_qe_mscl, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_G2DD2, NULL, cmucal_vclk_ip_sysmmu_g2dd2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_G2DD2, NULL, cmucal_vclk_ip_ppmu_g2dd2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D2_G2D, NULL, cmucal_vclk_ip_lhs_acel_d2_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_JPEG, NULL, cmucal_vclk_ip_as_p_jpeg, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_G2D, NULL, cmucal_vclk_ip_xiu_d_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_MSCL, NULL, cmucal_vclk_ip_as_p_mscl, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_ASTC, NULL, cmucal_vclk_ip_as_p_astc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_SYSMMU_NS_G2DD0, NULL, cmucal_vclk_ip_as_p_sysmmu_ns_g2dd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_SYSMMU_NS_G2DD2, NULL, cmucal_vclk_ip_as_p_sysmmu_ns_g2dd2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_SYSMMU_S_G2DD0, NULL, cmucal_vclk_ip_as_p_sysmmu_s_g2dd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_SYSMMU_S_G2DD2, NULL, cmucal_vclk_ip_as_p_sysmmu_s_g2dd2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_ASTC, NULL, cmucal_vclk_ip_qe_astc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_G2D, NULL, cmucal_vclk_ip_vgen_lite_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_G2D, NULL, cmucal_vclk_ip_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_SYSMMU_NS_G2DD1, NULL, cmucal_vclk_ip_as_p_sysmmu_ns_g2dd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_SYSMMU_S_G2DD1, NULL, cmucal_vclk_ip_as_p_sysmmu_s_g2dd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_G2DD1, NULL, cmucal_vclk_ip_sysmmu_g2dd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_JPEG, NULL, cmucal_vclk_ip_jpeg, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MSCL, NULL, cmucal_vclk_ip_mscl, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ASTC, NULL, cmucal_vclk_ip_astc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_P_JSQZ, NULL, cmucal_vclk_ip_as_p_jsqz, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_JSQZ, NULL, cmucal_vclk_ip_qe_jsqz, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_G2D, NULL, cmucal_vclk_ip_d_tzpc_g2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_JSQZ, NULL, cmucal_vclk_ip_jsqz, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_G3D, NULL, cmucal_vclk_ip_xiu_p_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_G3D, NULL, cmucal_vclk_ip_lhm_axi_p_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMG3D, NULL, cmucal_vclk_ip_busif_hpmg3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_G3D0, NULL, cmucal_vclk_ip_hpm_g3d0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_G3D, NULL, cmucal_vclk_ip_sysreg_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_G3D_CMU_G3D, NULL, cmucal_vclk_ip_g3d_cmu_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_G3DSFR, NULL, cmucal_vclk_ip_lhs_axi_g3dsfr, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_G3D, NULL, cmucal_vclk_ip_vgen_lite_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPU, NULL, cmucal_vclk_ip_gpu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_G3D, NULL, cmucal_vclk_ip_axi2apb_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_G3DSFR, NULL, cmucal_vclk_ip_lhm_axi_g3dsfr, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GRAY2BIN_G3D, NULL, cmucal_vclk_ip_gray2bin_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_G3D, NULL, cmucal_vclk_ip_d_tzpc_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ASB_G3D, NULL, cmucal_vclk_ip_asb_g3d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_ISPHQ, NULL, cmucal_vclk_ip_lhm_axi_p_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_ISPHQ, NULL, cmucal_vclk_ip_lhs_axi_d_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_IS_ISPHQ, NULL, cmucal_vclk_ip_is_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_ISPHQ, NULL, cmucal_vclk_ip_sysreg_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ISPHQ_CMU_ISPHQ, NULL, cmucal_vclk_ip_isphq_cmu_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_ISPPREISPHQ, NULL, cmucal_vclk_ip_lhm_atb_isppreisphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_ISPHQISPLP, NULL, cmucal_vclk_ip_lhs_atb_isphqisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_ISPHQ, NULL, cmucal_vclk_ip_btm_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_VO_ISPLPISPHQ, NULL, cmucal_vclk_ip_lhm_atb_vo_isplpisphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_VO_ISPHQISPPRE, NULL, cmucal_vclk_ip_lhs_ast_vo_isphqisppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_ISPHQ, NULL, cmucal_vclk_ip_d_tzpc_isphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_ISPHQDSPM, NULL, cmucal_vclk_ip_lhs_ast_isphqdspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_ISPLP, NULL, cmucal_vclk_ip_lhm_axi_p_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D0_ISPLP, NULL, cmucal_vclk_ip_lhs_axi_d0_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_ISPLP0, NULL, cmucal_vclk_ip_btm_isplp0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_IS_ISPLP, NULL, cmucal_vclk_ip_is_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_ISPLP, NULL, cmucal_vclk_ip_sysreg_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ISPLP_CMU_ISPLP, NULL, cmucal_vclk_ip_isplp_cmu_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_ISPLP1, NULL, cmucal_vclk_ip_btm_isplp1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D1_ISPLP, NULL, cmucal_vclk_ip_lhs_axi_d1_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_ISPHQISPLP, NULL, cmucal_vclk_ip_lhm_atb_isphqisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_VO_ISPPREISPLP, NULL, cmucal_vclk_ip_lhm_ast_vo_isppreisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_ATB_ISPPREISPLP, NULL, cmucal_vclk_ip_lhm_atb_isppreisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_VO_ISPLPISPHQ, NULL, cmucal_vclk_ip_lhs_atb_vo_isplpisphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_ISPLP, NULL, cmucal_vclk_ip_d_tzpc_isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_ISPLPDSPM, NULL, cmucal_vclk_ip_lhs_ast_isplpdspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_DSPMISPLP, NULL, cmucal_vclk_ip_lhm_ast_dspmisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_ISPLPVRA2, NULL, cmucal_vclk_ip_lhs_axi_p_isplpvra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_VRA2ISPLP, NULL, cmucal_vclk_ip_lhm_axi_d_vra2isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_IS_ISPPRE, NULL, cmucal_vclk_ip_is_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_ISPPRE, NULL, cmucal_vclk_ip_lhs_axi_d_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_ISPPRE, NULL, cmucal_vclk_ip_btm_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_ISPPRE, NULL, cmucal_vclk_ip_lhm_axi_p_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_ISPPRE, NULL, cmucal_vclk_ip_sysreg_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ISPPRE_CMU_ISPPRE, NULL, cmucal_vclk_ip_isppre_cmu_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_ISPPREISPLP, NULL, cmucal_vclk_ip_lhs_atb_isppreisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ATB_ISPPREISPHQ, NULL, cmucal_vclk_ip_lhs_atb_isppreisphq, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_ISPPRE, NULL, cmucal_vclk_ip_d_tzpc_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_ISPPREDSPM, NULL, cmucal_vclk_ip_lhs_ast_isppredspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_DSPMISPPRE, NULL, cmucal_vclk_ip_lhm_ast_dspmisppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMISPPRE, NULL, cmucal_vclk_ip_busif_hpmisppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_ISPPRE, NULL, cmucal_vclk_ip_hpm_isppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_ISPPRE1, NULL, cmucal_vclk_ip_d_tzpc_isppre1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_VO_ISPPREISPLP, NULL, cmucal_vclk_ip_lhs_ast_vo_isppreisplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_VO_ISPHQISPPRE, NULL, cmucal_vclk_ip_lhm_ast_vo_isphqisppre, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_IVA_CMU_IVA, NULL, cmucal_vclk_ip_iva_cmu_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D_IVA, NULL, cmucal_vclk_ip_lhs_acel_d_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_IVADSPS, NULL, cmucal_vclk_ip_lhs_axi_d_ivadsps, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_IVADSPM, NULL, cmucal_vclk_ip_lhs_axi_p_ivadspm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_DSPMIVA, NULL, cmucal_vclk_ip_lhm_axi_p_dspmiva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_IVA, NULL, cmucal_vclk_ip_lhm_axi_p_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_IVA, NULL, cmucal_vclk_ip_btm_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_IVA, NULL, cmucal_vclk_ip_ppmu_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_IVA, NULL, cmucal_vclk_ip_sysmmu_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_IVA, NULL, cmucal_vclk_ip_xiu_p_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_IVA0, NULL, cmucal_vclk_ip_ad_apb_iva0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_2M_IVA, NULL, cmucal_vclk_ip_axi2apb_2m_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_IVA, NULL, cmucal_vclk_ip_axi2apb_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_IVA, NULL, cmucal_vclk_ip_sysreg_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_IVASC, NULL, cmucal_vclk_ip_lhm_axi_d_ivasc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ADM_DAP_IVA, NULL, cmucal_vclk_ip_adm_dap_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_DSPSIVA, NULL, cmucal_vclk_ip_lhm_axi_d_dspsiva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_IVA1, NULL, cmucal_vclk_ip_ad_apb_iva1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_APB_IVA2, NULL, cmucal_vclk_ip_ad_apb_iva2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_IVA, NULL, cmucal_vclk_ip_vgen_lite_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_IVA, NULL, cmucal_vclk_ip_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_IVA_INTMEM, NULL, cmucal_vclk_ip_iva_intmem, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D0_IVA, NULL, cmucal_vclk_ip_xiu_d0_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D1_IVA, NULL, cmucal_vclk_ip_xiu_d1_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_IVA, NULL, cmucal_vclk_ip_d_tzpc_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D2_IVA, NULL, cmucal_vclk_ip_xiu_d2_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TREX_RB1_IVA, NULL, cmucal_vclk_ip_trex_rb1_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_IVA, NULL, cmucal_vclk_ip_qe_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WRAP2_CONV_IVA, NULL, cmucal_vclk_ip_wrap2_conv_iva, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MFC_CMU_MFC, NULL, cmucal_vclk_ip_mfc_cmu_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_MFC, NULL, cmucal_vclk_ip_as_apb_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_MFC, NULL, cmucal_vclk_ip_axi2apb_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_MFC, NULL, cmucal_vclk_ip_sysreg_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D0_MFC, NULL, cmucal_vclk_ip_lhs_axi_d0_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D1_MFC, NULL, cmucal_vclk_ip_lhs_axi_d1_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_MFC, NULL, cmucal_vclk_ip_lhm_axi_p_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_MFCD0, NULL, cmucal_vclk_ip_sysmmu_mfcd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_MFCD1, NULL, cmucal_vclk_ip_sysmmu_mfcd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_MFCD0, NULL, cmucal_vclk_ip_ppmu_mfcd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_MFCD1, NULL, cmucal_vclk_ip_ppmu_mfcd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_MFCD0, NULL, cmucal_vclk_ip_btm_mfcd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_MFCD1, NULL, cmucal_vclk_ip_btm_mfcd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_SYSMMU_NS_MFCD0, NULL, cmucal_vclk_ip_as_apb_sysmmu_ns_mfcd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_SYSMMU_NS_MFCD1, NULL, cmucal_vclk_ip_as_apb_sysmmu_ns_mfcd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_SYSMMU_S_MFCD0, NULL, cmucal_vclk_ip_as_apb_sysmmu_s_mfcd0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_SYSMMU_S_MFCD1, NULL, cmucal_vclk_ip_as_apb_sysmmu_s_mfcd1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_WFD_NS, NULL, cmucal_vclk_ip_as_apb_wfd_ns, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_AXI_WFD, NULL, cmucal_vclk_ip_as_axi_wfd, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_MFCD2, NULL, cmucal_vclk_ip_ppmu_mfcd2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_MFC, NULL, cmucal_vclk_ip_xiu_d_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_WFD_S, NULL, cmucal_vclk_ip_as_apb_wfd_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_MFC, NULL, cmucal_vclk_ip_vgen_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MFC, NULL, cmucal_vclk_ip_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WFD, NULL, cmucal_vclk_ip_wfd, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LH_ATB_MFC, NULL, cmucal_vclk_ip_lh_atb_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_MFC, NULL, cmucal_vclk_ip_d_tzpc_mfc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MIF_CMU_MIF, NULL, cmucal_vclk_ip_mif_cmu_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DDRPHY, NULL, cmucal_vclk_ip_ddrphy, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_MIF, NULL, cmucal_vclk_ip_sysreg_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMMIF, NULL, cmucal_vclk_ip_busif_hpmmif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_MIF, NULL, cmucal_vclk_ip_lhm_axi_p_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_MIF, NULL, cmucal_vclk_ip_axi2apb_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_DVFS, NULL, cmucal_vclk_ip_ppc_dvfs, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPC_DEBUG, NULL, cmucal_vclk_ip_ppc_debug, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DDRPHY, NULL, cmucal_vclk_ip_apbbr_ddrphy, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMC, NULL, cmucal_vclk_ip_apbbr_dmc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMCTZ, NULL, cmucal_vclk_ip_apbbr_dmctz, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_MIF, NULL, cmucal_vclk_ip_hpm_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMC, NULL, cmucal_vclk_ip_dmc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPC_DEBUG, NULL, cmucal_vclk_ip_qch_adapter_ppc_debug, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPC_DVFS, NULL, cmucal_vclk_ip_qch_adapter_ppc_dvfs, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_MIF, NULL, cmucal_vclk_ip_d_tzpc_mif, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_MIF1, NULL, cmucal_vclk_ip_hpm_mif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MIF1_CMU_MIF1, NULL, cmucal_vclk_ip_mif1_cmu_mif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DDRPHY1, NULL, cmucal_vclk_ip_apbbr_ddrphy1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMC1, NULL, cmucal_vclk_ip_apbbr_dmc1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMCTZ1, NULL, cmucal_vclk_ip_apbbr_dmctz1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_MIF1, NULL, cmucal_vclk_ip_axi2apb_mif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMMIF1, NULL, cmucal_vclk_ip_busif_hpmmif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DDRPHY1, NULL, cmucal_vclk_ip_ddrphy1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMC1, NULL, cmucal_vclk_ip_dmc1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_MIF1, NULL, cmucal_vclk_ip_lhm_axi_p_mif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMUPPC_DEBUG1, NULL, cmucal_vclk_ip_ppmuppc_debug1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMUPPC_DVFS1, NULL, cmucal_vclk_ip_ppmuppc_dvfs1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_MIF1, NULL, cmucal_vclk_ip_sysreg_mif1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPMUPPC_DEBUG1, NULL, cmucal_vclk_ip_qch_adapter_ppmuppc_debug1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPMUPPC_DVFS1, NULL, cmucal_vclk_ip_qch_adapter_ppmuppc_dvfs1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_MIF2, NULL, cmucal_vclk_ip_hpm_mif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DDRPHY2, NULL, cmucal_vclk_ip_apbbr_ddrphy2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMC2, NULL, cmucal_vclk_ip_apbbr_dmc2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMCTZ2, NULL, cmucal_vclk_ip_apbbr_dmctz2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_MIF2, NULL, cmucal_vclk_ip_axi2apb_mif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMMIF2, NULL, cmucal_vclk_ip_busif_hpmmif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DDRPHY2, NULL, cmucal_vclk_ip_ddrphy2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMC2, NULL, cmucal_vclk_ip_dmc2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_MIF2, NULL, cmucal_vclk_ip_lhm_axi_p_mif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMUPPC_DEBUG2, NULL, cmucal_vclk_ip_ppmuppc_debug2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMUPPC_DVFS2, NULL, cmucal_vclk_ip_ppmuppc_dvfs2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_MIF2, NULL, cmucal_vclk_ip_sysreg_mif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPMUPPC_DEBUG2, NULL, cmucal_vclk_ip_qch_adapter_ppmuppc_debug2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPMUPPC_DVFS2, NULL, cmucal_vclk_ip_qch_adapter_ppmuppc_dvfs2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MIF2_CMU_MIF2, NULL, cmucal_vclk_ip_mif2_cmu_mif2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HPM_MIF3, NULL, cmucal_vclk_ip_hpm_mif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DDRPHY3, NULL, cmucal_vclk_ip_apbbr_ddrphy3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMC3, NULL, cmucal_vclk_ip_apbbr_dmc3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APBBR_DMCTZ3, NULL, cmucal_vclk_ip_apbbr_dmctz3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_MIF3, NULL, cmucal_vclk_ip_axi2apb_mif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BUSIF_HPMMIF3, NULL, cmucal_vclk_ip_busif_hpmmif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DDRPHY3, NULL, cmucal_vclk_ip_ddrphy3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMC3, NULL, cmucal_vclk_ip_dmc3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_MIF3, NULL, cmucal_vclk_ip_lhm_axi_p_mif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMUPPC_DEBUG3, NULL, cmucal_vclk_ip_ppmuppc_debug3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMUPPC_DVFS3, NULL, cmucal_vclk_ip_ppmuppc_dvfs3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_MIF3, NULL, cmucal_vclk_ip_sysreg_mif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MIF3_CMU_MIF3, NULL, cmucal_vclk_ip_mif3_cmu_mif3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPMUPPC_DEBUG3, NULL, cmucal_vclk_ip_qch_adapter_ppmuppc_debug3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QCH_ADAPTER_PPMUPPC_DVFS3, NULL, cmucal_vclk_ip_qch_adapter_ppmuppc_dvfs3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_ACEL_D_NPU, NULL, cmucal_vclk_ip_lhs_acel_d_npu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_P_NPU1, NULL, cmucal_vclk_ip_lhs_axi_p_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPU0_CMU_NPU0, NULL, cmucal_vclk_ip_npu0_cmu_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APB_ASYNC_SI0, NULL, cmucal_vclk_ip_apb_async_si0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APB_ASYNC_SMMU_NS, NULL, cmucal_vclk_ip_apb_async_smmu_ns, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_NPU0, NULL, cmucal_vclk_ip_axi2apb_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_NPU0, NULL, cmucal_vclk_ip_btm_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_NPU0, NULL, cmucal_vclk_ip_d_tzpc_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_0, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_1, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_2, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_3, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_4, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_4, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_5, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_5, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_6, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_6, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD1_D1_7, NULL, cmucal_vclk_ip_lhm_ast_d_npud1_d1_7, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_P_NPU1_DONE, NULL, cmucal_vclk_ip_lhm_ast_p_npu1_done, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_DSPMNPU0, NULL, cmucal_vclk_ip_lhm_axi_d_dspmnpu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_NPU, NULL, cmucal_vclk_ip_lhm_axi_p_npu, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_0, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_1, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_2, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_3, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_4, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_4, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_5, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_5, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_6, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_6, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD0_D1_7, NULL, cmucal_vclk_ip_lhs_ast_d_npud0_d1_7, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_P_NPUD1_SETREG, NULL, cmucal_vclk_ip_lhs_ast_p_npud1_setreg, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_IDPSRAM1, NULL, cmucal_vclk_ip_lhs_axi_d_idpsram1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_IDPSRAM3, NULL, cmucal_vclk_ip_lhs_axi_d_idpsram3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPUC, NULL, cmucal_vclk_ip_npuc, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPUD_UNIT0, NULL, cmucal_vclk_ip_npud_unit0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_CPUDMA, NULL, cmucal_vclk_ip_ppmu_cpudma, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_RFM, NULL, cmucal_vclk_ip_ppmu_rfm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_CPUDMA, NULL, cmucal_vclk_ip_qe_cpudma, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_RFM, NULL, cmucal_vclk_ip_qe_rfm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SMMU_NPU0, NULL, cmucal_vclk_ip_smmu_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_NPU0, NULL, cmucal_vclk_ip_sysreg_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_D_NPU0, NULL, cmucal_vclk_ip_xiu_d_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APB_ASYNC_SMMU_S, NULL, cmucal_vclk_ip_apb_async_smmu_s, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_NPU0, NULL, cmucal_vclk_ip_vgen_lite_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_NPU0, NULL, cmucal_vclk_ip_ppmu_npu0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPU0_PPC_WRAPPER, NULL, cmucal_vclk_ip_npu0_ppc_wrapper, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPU1_CMU_NPU1, NULL, cmucal_vclk_ip_npu1_cmu_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_0, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_NPU1, NULL, cmucal_vclk_ip_lhm_axi_p_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_APB_ASYNC_SI1, NULL, cmucal_vclk_ip_apb_async_si1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_NPU1, NULL, cmucal_vclk_ip_axi2apb_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_NPU1, NULL, cmucal_vclk_ip_d_tzpc_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_1, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_2, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_3, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_4, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_4, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_5, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_5, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_6, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_6, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_D_NPUD0_D1_7, NULL, cmucal_vclk_ip_lhm_ast_d_npud0_d1_7, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AST_P_NPUD1_SETREG, NULL, cmucal_vclk_ip_lhm_ast_p_npud1_setreg, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_IDPSRAM1, NULL, cmucal_vclk_ip_lhm_axi_d_idpsram1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_D_IDPSRAM3, NULL, cmucal_vclk_ip_lhm_axi_d_idpsram3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_0, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_1, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_2, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_3, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_4, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_4, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_5, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_5, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_6, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_6, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_D_NPUD1_D1_7, NULL, cmucal_vclk_ip_lhs_ast_d_npud1_d1_7, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_NPU1, NULL, cmucal_vclk_ip_sysreg_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AST_P_NPU1_DONE, NULL, cmucal_vclk_ip_lhs_ast_p_npu1_done, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPUD_UNIT1, NULL, cmucal_vclk_ip_npud_unit1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_NPU1, NULL, cmucal_vclk_ip_ppmu_npu1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_NPU1_PPC_WRAPPER, NULL, cmucal_vclk_ip_npu1_ppc_wrapper, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_PERIC0, NULL, cmucal_vclk_ip_gpio_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PWM, NULL, cmucal_vclk_ip_pwm, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_PERIC0, NULL, cmucal_vclk_ip_sysreg_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI00_USI, NULL, cmucal_vclk_ip_usi00_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI01_USI, NULL, cmucal_vclk_ip_usi01_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI02_USI, NULL, cmucal_vclk_ip_usi02_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI03_USI, NULL, cmucal_vclk_ip_usi03_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_PERIC0P0, NULL, cmucal_vclk_ip_axi2apb_peric0p0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PERIC0_CMU_PERIC0, NULL, cmucal_vclk_ip_peric0_cmu_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI04_USI, NULL, cmucal_vclk_ip_usi04_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_PERIC0P1, NULL, cmucal_vclk_ip_axi2apb_peric0p1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI05_USI, NULL, cmucal_vclk_ip_usi05_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI00_I2C, NULL, cmucal_vclk_ip_usi00_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI01_I2C, NULL, cmucal_vclk_ip_usi01_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI02_I2C, NULL, cmucal_vclk_ip_usi02_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI03_I2C, NULL, cmucal_vclk_ip_usi03_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI04_I2C, NULL, cmucal_vclk_ip_usi04_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI05_I2C, NULL, cmucal_vclk_ip_usi05_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_UART_DBG, NULL, cmucal_vclk_ip_uart_dbg, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_PERIC0, NULL, cmucal_vclk_ip_xiu_p_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_PERIC0, NULL, cmucal_vclk_ip_lhm_axi_p_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI12_USI, NULL, cmucal_vclk_ip_usi12_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI12_I2C, NULL, cmucal_vclk_ip_usi12_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI13_I2C, NULL, cmucal_vclk_ip_usi13_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI13_USI, NULL, cmucal_vclk_ip_usi13_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI14_USI, NULL, cmucal_vclk_ip_usi14_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI14_I2C, NULL, cmucal_vclk_ip_usi14_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_PERIC0, NULL, cmucal_vclk_ip_d_tzpc_peric0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI15_I2C, NULL, cmucal_vclk_ip_usi15_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI15_USI, NULL, cmucal_vclk_ip_usi15_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_PERIC1P1, NULL, cmucal_vclk_ip_axi2apb_peric1p1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_PERIC1, NULL, cmucal_vclk_ip_gpio_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_PERIC1, NULL, cmucal_vclk_ip_sysreg_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_UART_BT, NULL, cmucal_vclk_ip_uart_bt, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CAM1, NULL, cmucal_vclk_ip_i2c_cam1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CAM2, NULL, cmucal_vclk_ip_i2c_cam2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CAM3, NULL, cmucal_vclk_ip_i2c_cam3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI06_USI, NULL, cmucal_vclk_ip_usi06_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI07_USI, NULL, cmucal_vclk_ip_usi07_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI08_USI, NULL, cmucal_vclk_ip_usi08_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I2C_CAM0, NULL, cmucal_vclk_ip_i2c_cam0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_PERIC1, NULL, cmucal_vclk_ip_xiu_p_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_PERIC1P0, NULL, cmucal_vclk_ip_axi2apb_peric1p0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PERIC1_CMU_PERIC1, NULL, cmucal_vclk_ip_peric1_cmu_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SPI_CAM0, NULL, cmucal_vclk_ip_spi_cam0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI09_USI, NULL, cmucal_vclk_ip_usi09_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI06_I2C, NULL, cmucal_vclk_ip_usi06_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI10_USI, NULL, cmucal_vclk_ip_usi10_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI07_I2C, NULL, cmucal_vclk_ip_usi07_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI08_I2C, NULL, cmucal_vclk_ip_usi08_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI09_I2C, NULL, cmucal_vclk_ip_usi09_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI10_I2C, NULL, cmucal_vclk_ip_usi10_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_PERIC1, NULL, cmucal_vclk_ip_lhm_axi_p_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI11_USI, NULL, cmucal_vclk_ip_usi11_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI11_I2C, NULL, cmucal_vclk_ip_usi11_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_PERIC1, NULL, cmucal_vclk_ip_d_tzpc_peric1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_I3C, NULL, cmucal_vclk_ip_i3c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI16_USI, NULL, cmucal_vclk_ip_usi16_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI17_USI, NULL, cmucal_vclk_ip_usi17_usi, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI16_I3C, NULL, cmucal_vclk_ip_usi16_i3c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_USI17_I2C, NULL, cmucal_vclk_ip_usi17_i2c, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_PERISP, NULL, cmucal_vclk_ip_axi2apb_perisp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XIU_P_PERIS, NULL, cmucal_vclk_ip_xiu_p_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_PERIS, NULL, cmucal_vclk_ip_sysreg_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WDT_CLUSTER2, NULL, cmucal_vclk_ip_wdt_cluster2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WDT_CLUSTER0, NULL, cmucal_vclk_ip_wdt_cluster0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PERIS_CMU_PERIS, NULL, cmucal_vclk_ip_peris_cmu_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AD_AXI_P_PERIS, NULL, cmucal_vclk_ip_ad_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_OTP_CON_BIRA, NULL, cmucal_vclk_ip_otp_con_bira, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GIC, NULL, cmucal_vclk_ip_gic, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_PERIS, NULL, cmucal_vclk_ip_lhm_axi_p_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MCT, NULL, cmucal_vclk_ip_mct, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_OTP_CON_TOP, NULL, cmucal_vclk_ip_otp_con_top, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_PERIS, NULL, cmucal_vclk_ip_d_tzpc_peris, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TMU_SUB, NULL, cmucal_vclk_ip_tmu_sub, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TMU_TOP, NULL, cmucal_vclk_ip_tmu_top, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_OTP_CON_BISR, NULL, cmucal_vclk_ip_otp_con_bisr, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_S2D_CMU_S2D, NULL, cmucal_vclk_ip_s2d_cmu_s2d, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VRA2_CMU_VRA2, NULL, cmucal_vclk_ip_vra2_cmu_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_VRA2, NULL, cmucal_vclk_ip_as_apb_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AXI2APB_VRA2, NULL, cmucal_vclk_ip_axi2apb_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_VRA2, NULL, cmucal_vclk_ip_d_tzpc_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_ISPLPVRA2, NULL, cmucal_vclk_ip_lhm_axi_p_isplpvra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_VRA2ISPLP, NULL, cmucal_vclk_ip_lhs_axi_d_vra2isplp, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_QE_VRA2, NULL, cmucal_vclk_ip_qe_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_VRA2, NULL, cmucal_vclk_ip_sysreg_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE_VRA2, NULL, cmucal_vclk_ip_vgen_lite_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VRA2, NULL, cmucal_vclk_ip_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AS_APB_STR, NULL, cmucal_vclk_ip_as_apb_str, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BTM_VRA2, NULL, cmucal_vclk_ip_btm_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_PPMU_VRA2, NULL, cmucal_vclk_ip_ppmu_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSMMU_VRA2, NULL, cmucal_vclk_ip_sysmmu_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_STR, NULL, cmucal_vclk_ip_str, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_VRA2, NULL, cmucal_vclk_ip_lhs_axi_d_vra2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC_IF, NULL, cmucal_vclk_ip_dmic_if, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SYSREG_VTS, NULL, cmucal_vclk_ip_sysreg_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VTS_CMU_VTS, NULL, cmucal_vclk_ip_vts_cmu_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_AHB_BUSMATRIX, NULL, cmucal_vclk_ip_ahb_busmatrix, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_P_VTS, NULL, cmucal_vclk_ip_lhm_axi_p_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_GPIO_VTS, NULL, cmucal_vclk_ip_gpio_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_WDT_VTS, NULL, cmucal_vclk_ip_wdt_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC_AHB0, NULL, cmucal_vclk_ip_dmic_ahb0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC_AHB1, NULL, cmucal_vclk_ip_dmic_ahb1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_C_VTS, NULL, cmucal_vclk_ip_lhs_axi_c_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_ASYNCINTERRUPT, NULL, cmucal_vclk_ip_asyncinterrupt, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HWACG_SYS_DMIC0, NULL, cmucal_vclk_ip_hwacg_sys_dmic0, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HWACG_SYS_DMIC1, NULL, cmucal_vclk_ip_hwacg_sys_dmic1, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SS_VTS_GLUE, NULL, cmucal_vclk_ip_ss_vts_glue, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_CORTEXM4INTEGRATION, NULL, cmucal_vclk_ip_cortexm4integration, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_U_DMIC_CLK_MUX, NULL, cmucal_vclk_ip_u_dmic_clk_mux, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHM_AXI_LP_VTS, NULL, cmucal_vclk_ip_lhm_axi_lp_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_LHS_AXI_D_VTS, NULL, cmucal_vclk_ip_lhs_axi_d_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_C_VTS, NULL, cmucal_vclk_ip_baaw_c_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_D_TZPC_VTS, NULL, cmucal_vclk_ip_d_tzpc_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_VGEN_LITE, NULL, cmucal_vclk_ip_vgen_lite, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_LP_VTS, NULL, cmucal_vclk_ip_bps_lp_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BPS_P_VTS, NULL, cmucal_vclk_ip_bps_p_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XHB_LP_VTS, NULL, cmucal_vclk_ip_xhb_lp_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_XHB_P_VTS, NULL, cmucal_vclk_ip_xhb_p_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SWEEPER_C_VTS, NULL, cmucal_vclk_ip_sweeper_c_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_SWEEPER_D_VTS, NULL, cmucal_vclk_ip_sweeper_d_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_BAAW_D_VTS, NULL, cmucal_vclk_ip_baaw_d_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_ABOX_VTS, NULL, cmucal_vclk_ip_mailbox_abox_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC_AHB2, NULL, cmucal_vclk_ip_dmic_ahb2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC_AHB3, NULL, cmucal_vclk_ip_dmic_ahb3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HWACG_SYS_DMIC2, NULL, cmucal_vclk_ip_hwacg_sys_dmic2, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_HWACG_SYS_DMIC3, NULL, cmucal_vclk_ip_hwacg_sys_dmic3, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_DMIC_IF_3RD, NULL, cmucal_vclk_ip_dmic_if_3rd, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_MAILBOX_AP_VTS, NULL, cmucal_vclk_ip_mailbox_ap_vts, NULL, NULL),
	CMUCAL_VCLK2(VCLK_IP_TIMER, NULL, cmucal_vclk_ip_timer, NULL, NULL),
};
