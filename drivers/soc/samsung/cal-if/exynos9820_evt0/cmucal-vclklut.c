#include "../cmucal.h"
#include "cmucal-vclklut.h"


/* DVFS VCLK -> LUT Parameter List */
unsigned int vddi_nm_lut_params[] = {
	2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 860000, 0, 2, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 
};
unsigned int vddi_od_lut_params[] = {
	1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 860000, 0, 2, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 
};
unsigned int vddi_ud_lut_params[] = {
	1, 1, 2, 2, 1, 0, 3, 0, 2, 0, 5, 3, 0, 2, 0, 2, 0, 0, 2, 0, 2, 0, 3, 3, 0, 0, 0, 3, 3, 3, 1, 2, 3, 0, 0, 3, 3, 0, 1, 2, 0, 1, 0, 0, 650000, 0, 1, 3, 2, 3, 0, 0, 1, 2, 0, 0, 0, 
};
unsigned int vddi_sud_lut_params[] = {
	3, 2, 7, 2, 1, 2, 7, 1, 2, 1, 3, 3, 2, 3, 1, 3, 1, 1, 3, 2, 3, 1, 1, 3, 1, 1, 1, 3, 3, 1, 3, 3, 3, 1, 1, 3, 3, 1, 4, 3, 1, 1, 1, 0, 320000, 1, 3, 0, 0, 0, 1, 1, 1, 2, 0, 0, 0, 
};
unsigned int vddi_uud_lut_params[] = {
	3, 5, 15, 4, 3, 5, 15, 7, 2, 3, 3, 2, 2, 2, 6, 2, 6, 6, 2, 5, 3, 3, 3, 2, 7, 1, 7, 3, 2, 1, 7, 4, 3, 7, 6, 2, 2, 7, 4, 2, 6, 0, 7, 0, 160000, 1, 3, 5, 0, 0, 0, 1, 2, 7, 1, 1, 1, 
};
unsigned int vdd_mif_od_lut_params[] = {
	4264000, 400000, 
};
unsigned int vdd_mif_ud_lut_params[] = {
	2687750, 422, 
};
unsigned int vdd_mif_sud_lut_params[] = {
	1420000, 422, 
};
unsigned int vdd_mif_uud_lut_params[] = {
	842000, 422, 
};
unsigned int vdd_cam_od_lut_params[] = {
	1200000, 23, 0, 1, 800000, 0, 3, 
};
unsigned int vdd_cam_sud_lut_params[] = {
	1200000, 23, 3, 6, 808000, 1, 3, 
};
unsigned int vdd_cam_ud_lut_params[] = {
	1200000, 23, 1, 3, 808000, 1, 3, 
};
unsigned int vdd_cam_nm_lut_params[] = {
	1032000, 19, 0, 1, 800000, 0, 3, 
};
unsigned int vdd_cam_uud_lut_params[] = {
	600000, 11, 3, 6, 120000, 7, 1, 
};
unsigned int vdd_cheetah_sod_lut_params[] = {
	2850250, 1, 
};
unsigned int vdd_cheetah_od_lut_params[] = {
	2224857, 0, 
};
unsigned int vdd_cheetah_nm_lut_params[] = {
	1834857, 0, 
};
unsigned int vdd_cheetah_ud_lut_params[] = {
	1100000, 0, 
};
unsigned int vdd_ananke_sod_lut_params[] = {
	1950000, 1, 
};
unsigned int vdd_ananke_od_lut_params[] = {
	1599000, 1, 
};
unsigned int vdd_ananke_nm_lut_params[] = {
	1150500, 0, 
};
unsigned int vdd_ananke_ud_lut_params[] = {
	650000, 0, 
};
unsigned int vdd_ananke_sud_lut_params[] = {
	349917, 0, 
};
unsigned int vdd_ananke_uud_lut_params[] = {
	175000, 0, 
};
unsigned int vdd_prometheus_sod_lut_params[] = {
	2398500, 
};
unsigned int vdd_prometheus_od_lut_params[] = {
	1800000, 
};
unsigned int vdd_prometheus_nm_lut_params[] = {
	1400000, 
};
unsigned int vdd_prometheus_ud_lut_params[] = {
	800000, 
};
unsigned int vdd_prometheus_sud_lut_params[] = {
	466000, 
};

/* SPECIAL VCLK -> LUT Parameter List */
unsigned int mux_clk_aud_uaif3_od_lut_params[] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 22579, 7, 1, 1, 
};
unsigned int mux_clk_aud_uaif3_nm_lut_params[] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 22579, 7, 1, 1, 
};
unsigned int mux_clk_aud_uaif3_uud_lut_params[] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 22579, 7, 1, 1, 
};
unsigned int mux_clk_aud_uaif3_ud_lut_params[] = {
	0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 22579, 7, 1, 1, 
};
unsigned int mux_clk_cmgp_adc_nm_lut_params[] = {
	1, 13, 
};
unsigned int clkcmu_cmgp_bus_nm_lut_params[] = {
	0, 2, 
};
unsigned int clkcmu_apm_bus_uud_lut_params[] = {
	1, 0, 
};
unsigned int mux_cmu_cmuref_ud_lut_params[] = {
	1, 0, 
};
unsigned int mux_cmu_cmuref_uud_lut_params[] = {
	1, 0, 
};
unsigned int mux_mif_cmuref_uud_lut_params[] = {
	1, 
};
unsigned int mux_mif1_cmuref_ud_lut_params[] = {
	0, 
};
unsigned int mux_mif2_cmuref_ud_lut_params[] = {
	0, 
};
unsigned int mux_mif3_cmuref_ud_lut_params[] = {
	0, 
};
unsigned int clkcmu_fsys0_dpgtc_uud_lut_params[] = {
	3, 1, 
};
unsigned int mux_clkcmu_fsys0_pcie_uud_lut_params[] = {
	1, 
};
unsigned int mux_clkcmu_fsys0a_usbdp_debug_uud_lut_params[] = {
	1, 
};
unsigned int mux_clkcmu_fsys1_pcie_uud_lut_params[] = {
	1, 
};
unsigned int clkcmu_fsys1_ufs_embd_uud_lut_params[] = {
	2, 1, 
};
unsigned int div_clk_aud_dmic_nm_lut_params[] = {
	1, 
};
unsigned int div_clk_aud_dmic_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_aud_dmic_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_aud_mclk_nm_lut_params[] = {
	0, 
};
unsigned int div_clk_aud_mclk_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_aud_mclk_ud_lut_params[] = {
	0, 
};
unsigned int div_clk_i2c_cmgp0_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_usi_cmgp1_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_usi_cmgp0_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_usi_cmgp0_4m_lut_params[] = {
	5, 0,
};
unsigned int div_clk_usi_cmgp2_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_usi_cmgp3_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_i2c_cmgp1_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_i2c_cmgp2_nm_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_i2c_cmgp3_nm_lut_params[] = {
	0, 1, 
};
unsigned int clkcmu_hpm_od_lut_params[] = {
	0, 
};
unsigned int clkcmu_hpm_ud_lut_params[] = {
	0, 
};
unsigned int clkcmu_hpm_uud_lut_params[] = {
	0, 
};
unsigned int clkcmu_cis_clk0_uud_lut_params[] = {
	3, 1, 
};
unsigned int clkcmu_cis_clk1_uud_lut_params[] = {
	3, 1, 
};
unsigned int clkcmu_cis_clk2_uud_lut_params[] = {
	3, 1, 
};
unsigned int clkcmu_cis_clk3_uud_lut_params[] = {
	3, 1, 
};
unsigned int clkcmu_cis_clk4_uud_lut_params[] = {
	3, 1, 
};
unsigned int div_clk_cpucl0_cmuref_sod_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl0_cmuref_od_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl0_cmuref_nm_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl0_cmuref_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl0_cmuref_sud_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl0_cmuref_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster0_periphclk_sod_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster0_periphclk_od_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster0_periphclk_nm_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster0_periphclk_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster0_periphclk_sud_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster0_periphclk_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl1_cmuref_sod_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl1_cmuref_od_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl1_cmuref_nm_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl1_cmuref_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl1_cmuref_sud_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl2_cmuref_sod_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl2_cmuref_od_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl2_cmuref_nm_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl2_cmuref_ud_lut_params[] = {
	1, 
};
unsigned int div_clk_cpucl2_cmuref_sud_lut_params[] = {
	1, 
};
unsigned int div_clk_cluster2_atclk_sod_lut_params[] = {
	3, 7, 1, 
};
unsigned int div_clk_cluster2_atclk_od_lut_params[] = {
	3, 7, 1, 
};
unsigned int div_clk_cluster2_atclk_nm_lut_params[] = {
	3, 7, 1, 
};
unsigned int div_clk_cluster2_atclk_ud_lut_params[] = {
	3, 7, 1, 
};
unsigned int div_clk_cluster2_atclk_sud_lut_params[] = {
	3, 7, 1, 
};
unsigned int div_clk_fsys1_mmc_card_ud_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_fsys1_mmc_card_uud_lut_params[] = {
	0, 1, 
};
unsigned int div_clk_mif_pre_uud_lut_params[] = {
	3, 
};
unsigned int div_clk_mif1_pre_ud_lut_params[] = {
	0, 
};
unsigned int div_clk_mif2_pre_ud_lut_params[] = {
	0, 
};
unsigned int div_clk_mif3_pre_ud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi00_usi_uud_lut_params[] = {
	0, 
};
unsigned int mux_clkcmu_peric0_ip_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric0_usi01_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi02_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi03_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi04_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi05_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_uart_dbg_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric0_usi12_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi13_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi14_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric0_usi15_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_uart_bt_uud_lut_params[] = {
	1, 
};
unsigned int mux_clkcmu_peric1_ip_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric1_usi06_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi07_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi08_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_i2c_cam0_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric1_i2c_cam1_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric1_i2c_cam2_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric1_i2c_cam3_uud_lut_params[] = {
	1, 
};
unsigned int div_clk_peric1_spi_cam0_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi09_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi10_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi11_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi16_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_peric1_usi17_usi_uud_lut_params[] = {
	0, 
};
unsigned int div_clk_vts_dmic_nm_lut_params[] = {
	0, 
};

/* parent clock source(DIV_CLKCMU_PERICx_IP): 200Mhz */
unsigned int div_clk_peric_200_lut_params[] = {
	0, 1,
};
unsigned int div_clk_peric_100_lut_params[] = {
	1, 1,
};
unsigned int div_clk_peric_50_lut_params[] = {
	3, 1,
};
unsigned int div_clk_peric_25_lut_params[] = {
	7, 1,
};
unsigned int div_clk_peric_26_lut_params[] = {
	0, 0,
};
unsigned int div_clk_peric_13_lut_params[] = {
	1, 0,
};
unsigned int div_clk_peric_8_lut_params[] = {
	2, 0,
};
unsigned int div_clk_peric_6_lut_params[] = {
	3, 0,
};

/* COMMON VCLK -> LUT Parameter List */
unsigned int blk_cmu_uud_lut_params[] = {
	1865500, 666000, 640000, 800000, 2132000, 1865500, 1, 0, 0, 0, 2, 0, 1, 7, 7, 1, 1, 1, 1, 1, 7, 7, 1, 2, 2, 1, 
};
unsigned int blk_mif1_ud_lut_params[] = {
	26000, 
};
unsigned int blk_mif2_ud_lut_params[] = {
	26000, 
};
unsigned int blk_mif3_ud_lut_params[] = {
	26000, 
};
unsigned int blk_apm_nm_lut_params[] = {
	2, 2, 0, 0, 
};
unsigned int blk_dsps_uud_lut_params[] = {
	0, 1, 
};
unsigned int blk_g3d_uud_lut_params[] = {
	0, 3, 
};
unsigned int blk_s2d_od_lut_params[] = {
	0, 1, 
};
unsigned int blk_s2d_uud_lut_params[] = {
	0, 1, 
};
unsigned int blk_vts_nm_lut_params[] = {
	1, 0, 1, 0, 
};
unsigned int blk_aud_od_lut_params[] = {
	1, 2, 
};
unsigned int blk_aud_nm_lut_params[] = {
	1, 2, 
};
unsigned int blk_aud_uud_lut_params[] = {
	1, 2, 
};
unsigned int blk_aud_ud_lut_params[] = {
	1, 2, 
};
unsigned int blk_busc_uud_lut_params[] = {
	1, 
};
unsigned int blk_cmgp_nm_lut_params[] = {
	0, 
};
unsigned int blk_core_uud_lut_params[] = {
	2, 
};
unsigned int blk_cpucl0_sod_lut_params[] = {
	3, 3, 3, 7, 1, 
};
unsigned int blk_cpucl0_uud_lut_params[] = {
	3, 3, 3, 7, 1, 
};
unsigned int blk_cpucl2_sod_lut_params[] = {
	7, 
};
unsigned int blk_cpucl2_od_lut_params[] = {
	7, 
};
unsigned int blk_cpucl2_nm_lut_params[] = {
	7, 
};
unsigned int blk_cpucl2_ud_lut_params[] = {
	7, 
};
unsigned int blk_cpucl2_sud_lut_params[] = {
	7, 
};
unsigned int blk_dpu_uud_lut_params[] = {
	3, 
};
unsigned int blk_dspm_uud_lut_params[] = {
	1, 
};
unsigned int blk_g2d_uud_lut_params[] = {
	1, 
};
unsigned int blk_isphq_uud_lut_params[] = {
	1, 
};
unsigned int blk_isplp_uud_lut_params[] = {
	1, 
};
unsigned int blk_isppre_uud_lut_params[] = {
	1, 
};
unsigned int blk_iva_uud_lut_params[] = {
	1, 3, 
};
unsigned int blk_mfc_uud_lut_params[] = {
	3, 
};
unsigned int blk_npu1_uud_lut_params[] = {
	3, 
};
unsigned int blk_peric0_uud_lut_params[] = {
	1, 
};
unsigned int blk_peric1_uud_lut_params[] = {
	1, 
};
unsigned int blk_vra2_uud_lut_params[] = {
	1, 
};
