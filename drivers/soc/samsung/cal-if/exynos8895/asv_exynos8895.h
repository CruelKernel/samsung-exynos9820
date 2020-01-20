#ifndef __ASV_EXYNOS8895_H__
#define __ASV_EXYNOS8895_H__

#include <linux/io.h>

#define ASV_TABLE_BASE	(0x10009000)
#define ID_TABLE_BASE	(0x10000000)

struct asv_tbl_info {
	unsigned mngs_asv_group:4;
	int mngs_modified_group:4;
	unsigned mngs_ssa1:4;
	unsigned mngs_ssa0:4;
	unsigned apollo_asv_group:4;
	int apollo_modified_group:4;
	unsigned apollo_ssa1:4;
	unsigned apollo_ssa0:4;

	unsigned g3d_asv_group:4;
	int g3d_modified_group:4;
	unsigned g3d_ssa1:4;
	unsigned g3d_ssa0:4;
	unsigned mif_asv_group:4;
	int mif_modified_group:4;
	unsigned mif_ssa1:4;
	unsigned mif_ssa0:4;

	unsigned int_asv_group:4;
	int int_modified_group:4;
	unsigned int_ssa1:4;
	unsigned int_ssa0:4;
	unsigned cam_disp_asv_group:4;
	int cam_disp_modified_group:4;
	unsigned cam_disp_ssa1:4;
	unsigned cam_disp_ssa0:4;

	unsigned asv_table_ver:7;
	unsigned fused_grp:1;
	unsigned reserved_0:8;
	unsigned cp_asv_group:4;
	int cp_modified_group:4;
	unsigned cp_ssa1:4;
	unsigned cp_ssa0:4;

	unsigned mngs_vthr:2;
	unsigned mngs_delta:2;
	unsigned apollo_vthr:2;
	unsigned apollo_delta:2;
	unsigned g3d_vthr1:2;
	unsigned g3d_vthr2:2;
	unsigned int_vthr:2;
	unsigned int_delta:2;
	unsigned mif_vthr:2;
	unsigned mif_delta:2;
};

struct id_tbl_info {
	unsigned reserved_0;
	unsigned reserved_1;
	unsigned reserved_2:16;
	unsigned char ids_mngs:8;
	unsigned char ids_g3d:8;
	unsigned char ids_int:8;
	unsigned char ids_others:8; /* apollo, mif, disp, cp */
	unsigned reserved_3:16;
	unsigned reserved_4:16;
	unsigned short sub_rev:4;
	unsigned short main_rev:4;
	unsigned reserved_5:8;
};

static struct asv_tbl_info *asv_tbl;
static struct id_tbl_info *id_tbl;

int asv_get_grp(unsigned int id)
{
	int grp = -1;

	if (!asv_tbl)
		return grp;

	switch (id) {
	case dvfs_mif:
		grp = asv_tbl->mif_asv_group + asv_tbl->mif_modified_group;
		break;
	case dvfs_int:
	case dvfs_intcam:
		grp = asv_tbl->int_asv_group + asv_tbl->int_modified_group;
		break;
	case dvfs_cpucl0:
		grp = asv_tbl->mngs_asv_group + asv_tbl->mngs_modified_group;
		break;
	case dvfs_cpucl1:
		grp = asv_tbl->apollo_asv_group + asv_tbl->apollo_modified_group;
		break;
	case dvfs_g3d:
		grp = asv_tbl->g3d_asv_group + asv_tbl->g3d_modified_group;
		break;
	case dvfs_cam:
	case dvfs_disp:
		grp = asv_tbl->cam_disp_asv_group + asv_tbl->cam_disp_modified_group;
		break;
	case dvs_cp:
		grp = asv_tbl->cp_asv_group + asv_tbl->cp_modified_group;
		break;
	default:
		pr_info("Un-support asv grp %d\n", id);
	}

	return grp;
}

int asv_get_ids_info(unsigned int id)
{
	int ids = 0;

	if (!id_tbl)
		return ids;

	switch (id) {
	case dvfs_cpucl0:
		ids = id_tbl->ids_mngs;
		break;
	case dvfs_g3d:
		ids = id_tbl->ids_g3d;
		break;
	case dvfs_int:
	case dvfs_intcam:
		ids = id_tbl->ids_int;
		break;
	case dvfs_cpucl1:
	case dvfs_mif:
	case dvfs_disp:
	case dvs_cp:
	case dvfs_cam:
		ids = id_tbl->ids_others;
		break;
	default:
		pr_info("Un-support ids info %d\n", id);
	}

	return ids;
}

int asv_get_table_ver(void)
{
	int ver = 0;

	if (asv_tbl)
		ver = asv_tbl->asv_table_ver;

	return ver;
}

int id_get_rev(void)
{
	int rev = 0;

	if (id_tbl)
		rev = id_tbl->main_rev;

	return rev;
}

int asv_table_init(void)
{
	asv_tbl = ioremap(ASV_TABLE_BASE, SZ_4K);
	if (!asv_tbl)
		return 0;

	pr_info("asv_table_version : %d\n", asv_tbl->asv_table_ver);
	pr_info("  mngs grp : %d\n", asv_tbl->mngs_asv_group);
	pr_info("  apollo grp : %d\n", asv_tbl->apollo_asv_group);
	pr_info("  g3d grp : %d\n", asv_tbl->g3d_asv_group);
	pr_info("  mif grp : %d\n", asv_tbl->mif_asv_group);
	pr_info("  int grp : %d\n", asv_tbl->int_asv_group);
	pr_info("  cam_disp grp : %d\n", asv_tbl->cam_disp_asv_group);
	pr_info("  cp grp : %d\n", asv_tbl->cp_asv_group);

	id_tbl = ioremap(ID_TABLE_BASE, SZ_4K);

	return asv_tbl->asv_table_ver;
}
#endif
