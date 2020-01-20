#include "../cmucal.h"

#include "cmucal-vclklut.h"

/*=================CMUCAL version: S5E9810================================*/

/*=================LUT in each VCLK================================*/
unsigned int vddi_nm_lut_params[] = {
	 23, 1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 0, 1, 0, 2, 0, 0, 0, 0, 2, 1, 0, 0, 1, 1, 0, 1, 0, 1, 1, 2, 2,
};
unsigned int vddi_od_lut_params[] = {
	 23, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 2, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
unsigned int vddi_sud_lut_params[] = {
	 7, 1, 2, 2, 3, 1, 1, 1, 1, 4, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 3, 1, 3, 7, 1, 1, 0, 4, 2, 1, 4, 2, 0, 1, 2, 3, 3, 0, 2, 2, 2, 0, 1, 0, 3, 2, 3, 3, 0, 3, 0, 2, 1, 1, 1, 3, 0, 3, 1, 1, 2, 2,
};
unsigned int vddi_ud_lut_params[] = {
	 12, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 3, 0, 1, 3, 0, 0, 0, 2, 0, 0, 2, 2, 4, 1, 2, 5, 3, 3, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 0, 3, 1, 3, 3, 1, 1, 3, 3, 0, 1, 1, 2, 2,
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
	 320000,
};
unsigned int vdd_g3d_ud_lut_params[] = {
	 650000,
};
unsigned int vdd_cam_nm_lut_params[] = {
	 3,
};
unsigned int vdd_cam_sud_lut_params[] = {
	 -1,
};
unsigned int vdd_cam_ud_lut_params[] = {
	 -1,
};
unsigned int spl_clk_usi_cmgp02_blk_apm_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_usi_cmgp00_blk_apm_nm_lut_params[] = {
	 0,
};
unsigned int clk_cmgp_adc_blk_apm_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_usi_cmgp03_blk_apm_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_usi_cmgp01_blk_apm_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_aud_uaif0_blk_aud_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_aud_uaif2_blk_aud_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_aud_cpu_pclkdbg_blk_aud_nm_lut_params[] = {
	 7,
};
unsigned int spl_clk_aud_cpu_pclkdbg_blk_aud_sud_lut_params[] = {
	 7,
};
unsigned int spl_clk_aud_uaif1_blk_aud_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_aud_uaif3_blk_aud_nm_lut_params[] = {
	 0, 1,
};
unsigned int div_clk_aud_dmic_blk_aud_nm_lut_params[] = {
	 0,
};
unsigned int div_clk_aud_dmic_blk_aud_sud_lut_params[] = {
	 0,
};
unsigned int div_clk_aud_dmic_blk_aud_ud_lut_params[] = {
	 0,
};
unsigned int spl_clk_chub_usi01_blk_chub_nm_lut_params[] = {
	 1, 1,
};
unsigned int spl_clk_chub_usi00_blk_chub_nm_lut_params[] = {
	 1, 1,
};
unsigned int mux_clk_chub_timer_fclk_blk_chub_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_usi_cmgp02_blk_cmgp_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_usi_cmgp00_blk_cmgp_nm_lut_params[] = {
	 0, 1,
};
unsigned int clk_cmgp_adc_blk_cmgp_nm_lut_params[] = {
	 0, 0,
};
unsigned int spl_clk_usi_cmgp01_blk_cmgp_nm_lut_params[] = {
	 0, 1,
};
unsigned int spl_clk_usi_cmgp03_blk_cmgp_nm_lut_params[] = {
	 0, 1,
};
unsigned int clk_fsys0_ufs_embd_blk_cmu_nm_lut_params[] = {
	 2, 1,
};
unsigned int clk_fsys0_ufs_embd_blk_cmu_od_lut_params[] = {
	 2, 1,
};
unsigned int clk_fsys1_mmc_card_blk_cmu_nm_lut_params[] = {
	 1,
};
unsigned int clkcmu_hpm_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clkcmu_hpm_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clkcmu_hpm_blk_cmu_ud_lut_params[] = {
	 0,
};
unsigned int spl_clk_chub_usi01_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_chub_usi01_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clkcmu_cis_clk0_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk0_blk_cmu_od_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk2_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk2_blk_cmu_od_lut_params[] = {
	 3, 1,
};
unsigned int clk_fsys0_dpgtc_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int clk_fsys0_dpgtc_blk_cmu_od_lut_params[] = {
	 3, 1,
};
unsigned int clk_fsys0_usb30drd_blk_cmu_nm_lut_params[] = {
	 7, 1,
};
unsigned int clk_fsys0_usb30drd_blk_cmu_od_lut_params[] = {
	 7, 1,
};
unsigned int mux_clkcmu_fsys0_usbdp_debug_user_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_fsys1_ufs_card_blk_cmu_nm_lut_params[] = {
	 2, 1,
};
unsigned int clk_fsys1_ufs_card_blk_cmu_od_lut_params[] = {
	 2, 1,
};
unsigned int occ_mif_cmuref_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int occ_mif_cmuref_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi01_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi01_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi04_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi04_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi13_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi13_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam1_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam1_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_spi_cam0_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_spi_cam0_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi07_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi07_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi10_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi10_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int occ_cmu_cmuref_blk_cmu_nm_lut_params[] = {
	 1,
};
unsigned int occ_cmu_cmuref_blk_cmu_od_lut_params[] = {
	 1,
};
unsigned int occ_cmu_cmuref_blk_cmu_sud_lut_params[] = {
	 1,
};
unsigned int spl_clk_chub_usi00_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_chub_usi00_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clkcmu_cis_clk1_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk1_blk_cmu_od_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk3_blk_cmu_nm_lut_params[] = {
	 3, 1,
};
unsigned int clkcmu_cis_clk3_blk_cmu_od_lut_params[] = {
	 3, 1,
};
unsigned int clk_peric0_usi00_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi00_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi05_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi05_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam0_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam0_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_uart_bt_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_uart_bt_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi09_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi09_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_uart_dbg_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_uart_dbg_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi12_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi12_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam3_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam3_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi11_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi11_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi02_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi02_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam2_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam2_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi03_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi03_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi08_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi08_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi06_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi06_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi14_usi_blk_cmu_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi14_usi_blk_cmu_od_lut_params[] = {
	 0,
};
unsigned int mux_clk_cluster0_sclk_blk_cpucl0_nm_lut_params[] = {
	 0,
};
unsigned int mux_clk_cluster0_sclk_blk_cpucl0_od_lut_params[] = {
	 0,
};
unsigned int mux_clk_cluster0_sclk_blk_cpucl0_sod_lut_params[] = {
	 0,
};
unsigned int mux_clk_cluster0_sclk_blk_cpucl0_sud_lut_params[] = {
	 0,
};
unsigned int mux_clk_cluster0_sclk_blk_cpucl0_ud_lut_params[] = {
	 0,
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
unsigned int clk_cluster0_aclkp_blk_cpucl0_nm_lut_params[] = {
	 0, 0,
};
unsigned int clk_cluster0_aclkp_blk_cpucl0_od_lut_params[] = {
	 0, 1,
};
unsigned int clk_cluster0_aclkp_blk_cpucl0_sod_lut_params[] = {
	 0, 1,
};
unsigned int clk_cluster0_aclkp_blk_cpucl0_sud_lut_params[] = {
	 0, 0,
};
unsigned int clk_cluster0_aclkp_blk_cpucl0_ud_lut_params[] = {
	 0, 0,
};
unsigned int div_clk_cluster0_periphclk_blk_cpucl0_nm_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster0_periphclk_blk_cpucl0_od_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster0_periphclk_blk_cpucl0_sod_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster0_periphclk_blk_cpucl0_sud_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster0_periphclk_blk_cpucl0_ud_lut_params[] = {
	 1,
};
unsigned int clk_cluster0_aclk_blk_cpucl0_nm_lut_params[] = {
	 0, 0,
};
unsigned int clk_cluster0_aclk_blk_cpucl0_od_lut_params[] = {
	 0, 1,
};
unsigned int clk_cluster0_aclk_blk_cpucl0_sod_lut_params[] = {
	 0, 1,
};
unsigned int clk_cluster0_aclk_blk_cpucl0_sud_lut_params[] = {
	 0, 0,
};
unsigned int clk_cluster0_aclk_blk_cpucl0_ud_lut_params[] = {
	 0, 0,
};
unsigned int div_clk_cluster1_aclk_blk_cpucl1_nm_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster1_aclk_blk_cpucl1_od_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster1_aclk_blk_cpucl1_sod_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster1_aclk_blk_cpucl1_sud_lut_params[] = {
	 1,
};
unsigned int div_clk_cluster1_aclk_blk_cpucl1_ud_lut_params[] = {
	 1,
};
unsigned int div_clk_cpucl1_pclkdbg_blk_cpucl1_nm_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl1_pclkdbg_blk_cpucl1_od_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl1_pclkdbg_blk_cpucl1_sod_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl1_pclkdbg_blk_cpucl1_sud_lut_params[] = {
	 7,
};
unsigned int div_clk_cpucl1_pclkdbg_blk_cpucl1_ud_lut_params[] = {
	 7,
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
unsigned int spl_clk_cpucl1_cmuref_blk_cpucl1_nm_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl1_cmuref_blk_cpucl1_od_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl1_cmuref_blk_cpucl1_sod_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl1_cmuref_blk_cpucl1_sud_lut_params[] = {
	 1,
};
unsigned int spl_clk_cpucl1_cmuref_blk_cpucl1_ud_lut_params[] = {
	 1,
};
unsigned int occ_mif_cmuref_blk_mif_nm_lut_params[] = {
	 1,
};
unsigned int occ_mif_cmuref_blk_mif_od_lut_params[] = {
	 1,
};
unsigned int clk_peric0_uart_dbg_blk_peric0_nm_lut_params[] = {
	 1,
};
unsigned int clk_peric0_usi01_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi03_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi05_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi13_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi04_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi02_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi12_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi00_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric0_usi14_usi_blk_peric0_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam0_blk_peric1_nm_lut_params[] = {
	 1,
};
unsigned int clk_peric1_i2c_cam2_blk_peric1_nm_lut_params[] = {
	 1,
};
unsigned int clk_peric1_spi_cam0_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi06_usi_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi08_usi_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_usi10_usi_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam1_blk_peric1_nm_lut_params[] = {
	 1,
};
unsigned int clk_peric1_usi07_usi_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_uart_bt_blk_peric1_nm_lut_params[] = {
	 1,
};
unsigned int clk_peric1_usi09_usi_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int clk_peric1_i2c_cam3_blk_peric1_nm_lut_params[] = {
	 1,
};
unsigned int clk_peric1_usi11_usi_blk_peric1_nm_lut_params[] = {
	 0,
};
unsigned int spl_clk_vts_dmic_if_div2_blk_vts_nm_lut_params[] = {
	 1,
};
unsigned int div_clk_vts_dmic_blk_vts_nm_lut_params[] = {
	 0,
};
unsigned int blk_apm_lut_params[] = {
	 -1, 0, 0,
};
unsigned int blk_aud_lut_params[] = {
	 1, 1, 1, 2, 1199999, 0, 1,
};
unsigned int blk_busc_lut_params[] = {
	 1,
};
unsigned int blk_chub_lut_params[] = {
	 1, -1, 1, 0,
};
unsigned int blk_cmgp_lut_params[] = {
	 0, 1, 0,
};
unsigned int blk_cmu_lut_params[] = {
	 1, 7, 7, 7, 640000, 0, 0, 0, 0, 0, 0, 1, 0, 0, 672000, 825999, 800000, 2132000, 1865500,
};
unsigned int blk_core_lut_params[] = {
	 2,
};
unsigned int blk_cpucl0_lut_params[] = {
	 3, 3, 3, 7, 1150500,
};
unsigned int blk_cpucl1_lut_params[] = {
	 7, 1478750,
};
unsigned int blk_dcf_lut_params[] = {
	 1,
};
unsigned int blk_dcpost_lut_params[] = {
	 1,
};
unsigned int blk_dcrd_lut_params[] = {
	 2, 1,
};
unsigned int blk_dpu_lut_params[] = {
	 3,
};
unsigned int blk_dspm_lut_params[] = {
	 1,
};
unsigned int blk_dsps_lut_params[] = {
	 1, 0,
};
unsigned int blk_g2d_lut_params[] = {
	 1,
};
unsigned int blk_g3d_lut_params[] = {
	 3, 0,
};
unsigned int blk_isphq_lut_params[] = {
	 1,
};
unsigned int blk_isplp_lut_params[] = {
	 1,
};
unsigned int blk_isppre_lut_params[] = {
	 1,
};
unsigned int blk_iva_lut_params[] = {
	 1,
};
unsigned int blk_mfc_lut_params[] = {
	 3,
};
unsigned int blk_mif_lut_params[] = {
	 3, 0,
};
unsigned int blk_peric0_lut_params[] = {
	 1,
};
unsigned int blk_peric1_lut_params[] = {
	 1,
};
unsigned int blk_s2d_lut_params[] = {
	 0, 0, 1664000,
};
unsigned int blk_vts_lut_params[] = {
	 0, 1, 0,
};
