#include "../cmucal.h"

#include "cmucal-vclklut.h"

/*=================CMUCAL version: S5E8895================================*/

/*=================LUT in each VCLK================================*/
unsigned int vddi_l0_lut_params[] = {
	 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1,
};
unsigned int vddi_l1_lut_params[] = {
	 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1,
};
unsigned int vddi_l2_lut_params[] = {
	 4, 2, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 1, 3, 3, 3, 2, 3, 3, 3, 1,
};
unsigned int vddi_l3_lut_params[] = {
	 7, 1, 1, 1, 1, 7, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 1, 4, 3, 2, 3, 3, 2, 3, 3, 3, 3, 2, 2, 2, 0, 1, 3, 3, 3, 2, 3, 3, 3, 0,
};
unsigned int vdd_mif_nm_lut_params[] = {
	 3731000,
};
unsigned int vdd_mif_od_lut_params[] = {
	 4264000,
};
unsigned int vdd_mif_sud_lut_params[] = {
	 1418000,
};
unsigned int vdd_mif_ud_lut_params[] = {
	 2576599,
};
unsigned int vdd_g3d_nm_lut_params[] = {
	 860000,
};
unsigned int vdd_g3d_sud_lut_params[] = {
	 300000,
};
unsigned int vdd_g3d_ud_lut_params[] = {
	 600000,
};
unsigned int vdd_mngs_nm_lut_params[] = {
	 1924000, 0,
};
unsigned int vdd_mngs_od_lut_params[] = {
	 2160599, 1,
};
unsigned int vdd_mngs_sod_lut_params[] = {
	 2860000, 1,
};
unsigned int vdd_mngs_sud_lut_params[] = {
	 400000, 0,
};
unsigned int vdd_mngs_ud_lut_params[] = {
	 1066000, 0,
};
unsigned int vdd_apollo_nm_lut_params[] = {
	 970666, 0,
};
unsigned int vdd_apollo_od_lut_params[] = {
	 1449500, 0,
};
unsigned int vdd_apollo_sod_lut_params[] = {
	 1906666, 1,
};
unsigned int vdd_apollo_sud_lut_params[] = {
	 300000, 0,
};
unsigned int vdd_apollo_ud_lut_params[] = {
	 600000, 1,
};
unsigned int dfs_abox_nm_lut_params[] = {
	 1, 0,
};
unsigned int dfs_abox_sud_lut_params[] = {
	 5, 2,
};
unsigned int dfs_abox_ud_lut_params[] = {
	 3, 1,
};
unsigned int dfs_abox_ssud_lut_params[] = {
	 5, 12,
};
unsigned int spl_clk_abox_uaif0_blk_abox_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif0_blk_abox_ud_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif2_blk_abox_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif2_blk_abox_ud_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif4_blk_abox_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif4_blk_abox_ud_lut_params[] = {
	 0, 1,
};
unsigned int div_clk_abox_dmic_blk_abox_nm_lut_params[] = {
	 0,
};
unsigned int div_clk_abox_dmic_blk_abox_ud_lut_params[] = {
	 0,
};
unsigned int spl_clk_abox_cpu_pclkdbg_blk_abox_nm_lut_params[] = {
	 7,
};
unsigned int spl_clk_abox_cpu_pclkdbg_blk_abox_sud_lut_params[] = {
	 7,
};
unsigned int spl_clk_abox_cpu_pclkdbg_blk_abox_ud_lut_params[] = {
	 7,
};
unsigned int spl_clk_abox_uaif1_blk_abox_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif1_blk_abox_ud_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif3_blk_abox_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_uaif3_blk_abox_ud_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_abox_cpu_aclk_blk_abox_nm_lut_params[] = {
	 1,
};
unsigned int spl_clk_abox_cpu_aclk_blk_abox_sud_lut_params[] = {
	 1,
};
unsigned int spl_clk_abox_cpu_aclk_blk_abox_ud_lut_params[] = {
	 1,
};
unsigned int spl_clk_fsys1_mmc_card_blk_cmu_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_fsys1_mmc_card_blk_cmu_mmc_l1_lut_params[] = {
	 1, 1,
};
unsigned int spl_clk_fsys1_mmc_card_blk_cmu_mmc_l2_lut_params[] = {
	 3, 1,
};
unsigned int spl_clk_fsys1_mmc_card_blk_cmu_mmc_l3_lut_params[] = {
	 7, 1,
};
unsigned int spl_clk_peric0_usi00_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric0_usi00_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric0_usi00_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric0_usi00_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric0_usi00_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric0_usi00_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric0_usi03_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric0_usi03_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric0_usi03_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric0_usi03_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric0_usi03_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric0_usi03_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_uart_bt_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi06_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi06_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi06_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi06_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi06_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi06_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi09_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi09_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi09_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi09_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi09_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi09_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi12_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi12_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi12_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi12_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi12_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi12_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int occ_cmu_cmuref_blk_cmu_nm_lut_params[] = {
	 1,
};
unsigned int occ_cmu_cmuref_blk_cmu_sud_lut_params[] = {
	 1,
};
unsigned int clkcmu_cis_clk0_blk_cmu_l1_lut_params[] = {
	 0, 0,
};
unsigned int clkcmu_cis_clk0_blk_cmu_l2_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk2_blk_cmu_l1_lut_params[] = {
	 0, 0,
};
unsigned int clkcmu_cis_clk2_blk_cmu_l2_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk0_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_hpm_blk_cmu_nm_lut_params[] = {
	 0, 2,
};
unsigned int spl_clk_fsys0_dpgtc_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int spl_clk_fsys0_mmc_embd_blk_cmu_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_fsys0_mmc_embd_blk_cmu_l1_lut_params[] = {
	 1, 1,
};
unsigned int spl_clk_fsys0_mmc_embd_blk_cmu_l2_lut_params[] = {
	 3, 1,
};
unsigned int spl_clk_fsys0_mmc_embd_blk_cmu_l3_lut_params[] = {
	 7, 1,
};
unsigned int spl_clk_fsys0_ufs_embd_blk_cmu_nm_lut_params[] = {
	 2, 1,
};
unsigned int spl_clk_fsys0_usbdrd30_blk_cmu_nm_lut_params[] = {
	 7, 1,
};
unsigned int spl_clk_fsys1_ufs_card_blk_cmu_nm_lut_params[] = {
	 2, 1,
};
unsigned int spl_clk_peric0_uart_dbg_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric0_usi01_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric0_usi01_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric0_usi01_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric0_usi01_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric0_usi01_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric0_usi01_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric0_usi02_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric0_usi02_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric0_usi02_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric0_usi02_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric0_usi02_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric0_usi02_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_spi_cam0_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_spi_cam0_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_spi_cam0_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_spi_cam0_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_spi_cam0_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_spi_cam1_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_spi_cam1_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_spi_cam1_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_spi_cam1_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_spi_cam1_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi04_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi04_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi04_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi04_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi04_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi04_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi05_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi05_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi05_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi05_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi05_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi05_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi07_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi07_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi07_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi07_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi07_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi07_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi08_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi08_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi08_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi08_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi08_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi08_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi10_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi10_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi10_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi10_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi10_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi10_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi11_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi11_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi11_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi11_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi11_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi11_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int spl_clk_peric1_usi13_blk_cmu_l1_lut_params[] = {
	 3, 2,
};
unsigned int spl_clk_peric1_usi13_blk_cmu_l2_lut_params[] = {
	 7, 2,
};
unsigned int spl_clk_peric1_usi13_blk_cmu_l3_lut_params[] = {
	 15, 2,
};
unsigned int spl_clk_peric1_usi13_blk_cmu_l4_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_peric1_usi13_blk_cmu_l5_lut_params[] = {
	 1, 0,
};
unsigned int spl_clk_peric1_usi13_blk_cmu_nm_lut_params[] = {
	 1, 2,
};
unsigned int clkcmu_cis_clk1_blk_cmu_l1_lut_params[] = {
	 0, 0,
};
unsigned int clkcmu_cis_clk1_blk_cmu_l2_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk3_blk_cmu_l1_lut_params[] = {
	 0, 0,
};
unsigned int clkcmu_cis_clk3_blk_cmu_l2_lut_params[] = {
	 3, 1,
};
unsigned int occ_clk_fsys1_pcie_blk_cmu_nm_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster0_atclk_blk_cpucl0_nm_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster0_atclk_blk_cpucl0_od_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster0_atclk_blk_cpucl0_sod_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster0_atclk_blk_cpucl0_sud_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster0_atclk_blk_cpucl0_ud_lut_params[] = {
	 3,
};
unsigned int spl_clk_cpucl0_cmuref_blk_cpucl0_nm_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl0_cmuref_blk_cpucl0_od_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl0_cmuref_blk_cpucl0_sod_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl0_cmuref_blk_cpucl0_sud_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl0_cmuref_blk_cpucl0_ud_lut_params[] = {
	 1,
};
unsigned int div_clk_cpucl0_pclkdbg_blk_cpucl0_nm_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl0_pclkdbg_blk_cpucl0_od_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl0_pclkdbg_blk_cpucl0_sod_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl0_pclkdbg_blk_cpucl0_sud_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl0_pclkdbg_blk_cpucl0_ud_lut_params[] = {
	 7,
};
unsigned int div_clk_cluster1_cntclk_blk_cpucl1_nm_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_cntclk_blk_cpucl1_od_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_cntclk_blk_cpucl1_sod_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_cntclk_blk_cpucl1_sud_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_cntclk_blk_cpucl1_ud_lut_params[] = {
	 3,
};
unsigned int div_clk_cpucl1_cmuref_blk_cpucl1_nm_lut_params[] = {
	 1,
};
unsigned int div_clk_cpucl1_cmuref_blk_cpucl1_od_lut_params[] = {
	 1,
};
unsigned int div_clk_cpucl1_cmuref_blk_cpucl1_sod_lut_params[] = {
	 1,
};
unsigned int div_clk_cpucl1_cmuref_blk_cpucl1_sud_lut_params[] = {
	 1,
};
unsigned int div_clk_cpucl1_cmuref_blk_cpucl1_ud_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster1_atclk_blk_cpucl1_nm_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_atclk_blk_cpucl1_od_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_atclk_blk_cpucl1_sod_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_atclk_blk_cpucl1_sud_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_atclk_blk_cpucl1_ud_lut_params[] = {
	 3,
};
unsigned int div_clk_cluster1_pclkdbg_blk_cpucl1_nm_lut_params[] = {
	 7,
};
unsigned int div_clk_cluster1_pclkdbg_blk_cpucl1_od_lut_params[] = {
	 7,
};
unsigned int div_clk_cluster1_pclkdbg_blk_cpucl1_sod_lut_params[] = {
	 7,
};
unsigned int div_clk_cluster1_pclkdbg_blk_cpucl1_sud_lut_params[] = {
	 7,
};
unsigned int div_clk_cluster1_pclkdbg_blk_cpucl1_ud_lut_params[] = {
	 7,
};
unsigned int spl_clk_dpu1_decon2_blk_dpu1_nm_lut_params[] = {
	 0, 600000,
};
unsigned int spl_clk_dpu1_decon2_blk_dpu1_sud_lut_params[] = {
	 0, 175000,
};
unsigned int occ_mif_cmuref_blk_mif_nm_lut_params[] = {
	 0,
};
unsigned int occ_mif1_cmuref_blk_mif1_nm_lut_params[] = {
	 0,
};
unsigned int occ_mif2_cmuref_blk_mif2_nm_lut_params[] = {
	 0,
};
unsigned int occ_mif3_cmuref_blk_mif3_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_vts_dmicif_div2_blk_vts_nm_lut_params[] = {
	 1,
};
unsigned int div_clk_vts_dmic_blk_vts_nm_lut_params[] = {
	 0,
};
unsigned int blk_abox_lut_params[] = {
	 1, 2, 23, 1179648, 1,
};
unsigned int blk_bus1_lut_params[] = {
	 1,
};
unsigned int blk_busc_lut_params[] = {
	 1,
};
unsigned int blk_cam_lut_params[] = {
	 1,
};
unsigned int blk_cmu_lut_params[] = {
	 1, 7, 7, 7, 630000, 7, 7, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 667333, 1865500, 800000, 2132000,
};
unsigned int blk_core_lut_params[] = {
	 2,
};
unsigned int blk_cpucl0_lut_params[] = {
	 7,
};
unsigned int blk_cpucl1_lut_params[] = {
	 7,
};
unsigned int blk_dbg_lut_params[] = {
	 3,
};
unsigned int blk_dcam_lut_params[] = {
	 2,
};
unsigned int blk_dpu0_lut_params[] = {
	 3,
};
unsigned int blk_dsp_lut_params[] = {
	 1,
};
unsigned int blk_g2d_lut_params[] = {
	 1,
};
unsigned int blk_g3d_lut_params[] = {
	 3,
};
unsigned int blk_isphq_lut_params[] = {
	 1,
};
unsigned int blk_isplp_lut_params[] = {
	 1,
};
unsigned int blk_iva_lut_params[] = {
	 1,
};
unsigned int blk_mfc_lut_params[] = {
	 3,
};
unsigned int blk_mif_lut_params[] = {
	 1, 3,
};
unsigned int blk_mif1_lut_params[] = {
	 26000, 0, 0,
};
unsigned int blk_mif2_lut_params[] = {
	 26000, 0, 0,
};
unsigned int blk_mif3_lut_params[] = {
	 26000, 0, 0,
};
unsigned int blk_srdz_lut_params[] = {
	 1,
};
unsigned int blk_vpu_lut_params[] = {
	 1,
};
unsigned int blk_vts_lut_params[] = {
	 0, 0, 11,
};
