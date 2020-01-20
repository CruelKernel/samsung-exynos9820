
enum acpm_dvfs_id {
	dvfs_mif = ACPM_VCLK_TYPE,
	dvfs_int,
	dvfs_cpucl0,
	dvfs_cpucl1,
	dvfs_g3d,
	dvfs_intcam,
	dvfs_fsys0,
	dvfs_cam,
	dvfs_disp_evt1,
	dvfs_aud,
	dvfs_iva,
	dvfs_score,
	dvfs_cp,
};

struct vclk acpm_vclk_list[] = {
	CMUCAL_ACPM_VCLK(dvfs_mif, NULL, NULL, NULL, NULL, MARGIN_MIF),
	CMUCAL_ACPM_VCLK(dvfs_int, NULL, NULL, NULL, NULL, MARGIN_INT),
	CMUCAL_ACPM_VCLK(dvfs_cpucl0, NULL, NULL, NULL, NULL, MARGIN_LIT),
	CMUCAL_ACPM_VCLK(dvfs_cpucl1, NULL, NULL, NULL, NULL, MARGIN_BIG),
	CMUCAL_ACPM_VCLK(dvfs_g3d, NULL, NULL, NULL, NULL, MARGIN_G3D),
	CMUCAL_ACPM_VCLK(dvfs_intcam, NULL, NULL, NULL, NULL, MARGIN_INTCAM),
	CMUCAL_ACPM_VCLK(dvfs_fsys0, NULL, NULL, NULL, NULL, MARGIN_FSYS0),
	CMUCAL_ACPM_VCLK(dvfs_cam, NULL, NULL, NULL, NULL, MARGIN_CAM),
	CMUCAL_ACPM_VCLK(dvfs_disp_evt1, NULL, NULL, NULL, NULL, MARGIN_DISP),
	CMUCAL_ACPM_VCLK(dvfs_aud, NULL, NULL, NULL, NULL, MARGIN_AUD),
	CMUCAL_ACPM_VCLK(dvfs_iva, NULL, NULL, NULL, NULL, MARGIN_IVA),
	CMUCAL_ACPM_VCLK(dvfs_score, NULL, NULL, NULL, NULL, MARGIN_SCORE),
	CMUCAL_ACPM_VCLK(dvfs_cp, NULL, NULL, NULL, NULL, MARGIN_CP),
};

unsigned int acpm_vclk_size = ARRAY_SIZE(acpm_vclk_list);
