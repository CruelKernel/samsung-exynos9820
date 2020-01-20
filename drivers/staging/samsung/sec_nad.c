/* sec_nad.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/sec_sysfs.h>
#include <linux/sec_nad.h>
#include <linux/fs.h>

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sec_debug.h>




#define NAD_PRINT(format, ...) printk("[NAD] " format, ##__VA_ARGS__)
#define NAD_DEBUG


#if defined(CONFIG_SEC_FACTORY)
#if defined(CONFIG_SEC_NAD_LOG)
static char nad_log_buffer[NAD_LOG_SIZE];
#endif

static void sec_nad_param_update(struct work_struct *work)
{
	int ret = -1;
	struct file *fp;
#if defined(CONFIG_SEC_NAD_LOG)
	struct file *fp2;
#endif	
	struct sec_nad_param *param_data =
		container_of(work, struct sec_nad_param, sec_nad_work);

	NAD_PRINT("%s: set param at %s\n", __func__, param_data->state ? "write" : "read");

	fp = filp_open(NAD_PARAM_NAME, O_RDWR | O_SYNC, 0);
	if (IS_ERR(fp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(fp));

		/* Rerun workqueue when nad_refer read fail */
		if (param_data->retry_cnt > param_data->curr_cnt) {
			NAD_PRINT("retry count : %d\n", param_data->curr_cnt++);
			schedule_delayed_work(&sec_nad_param_data.sec_nad_delay_work, HZ * 1);
		}
		return;
	}
	ret = fp->f_op->llseek(fp, -param_data->offset, SEEK_END);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}

#if defined(CONFIG_SEC_NAD_LOG)	
	fp2 = filp_open(NAD_UFS_TEST_BLOCK, O_RDWR | O_SYNC, 0);
	if (IS_ERR(fp2)) {
		pr_err("%s: NAD_UFS_TEST_BLOCK filp_open error %ld\n", __func__, PTR_ERR(fp2));
		return;
	}
	
	ret = fp2->f_op->llseek(fp2, NAD_LOG_OFFSET, SEEK_SET);
	if (ret < 0) {
		pr_err("%s: llseek error %d!\n", __func__, ret);
		goto close_fp_out;
	}
	
	NAD_PRINT("%s: NAD_LOG_OFFSET %d\n", __func__, ret);	
#endif	

	switch (param_data->state) {
	case NAD_PARAM_WRITE:
		ret = vfs_write(fp, (char *)&sec_nad_env, sizeof(struct nad_env), &(fp->f_pos));

		if (ret < 0)
			pr_err("%s: write error! %d\n", __func__, ret);

#ifdef NAD_DEBUG
			NAD_PRINT(
				"NAD factory : %s\n"
				"NAD result : %s\n"
				"NAD data : 0x%x\n"
				"NAD ACAT : %s\n"
				"NAD ACAT result : %s\n"
				"NAD ACAT data : 0x%x\n"
				"NAD ACAT loop count : %d\n"
				"NAD ACAT real count : %d\n"
				"NAD DRAM test need : %s\n"
				"NAD DRAM test result : 0x%x\n"
				"NAD DRAM test fail data : 0x%x\n"
				"NAD DRAM test fail address : 0x%08lx\n"
				"NAD status : %d\n"
				"NAD ACAT skip fail : %d\n"
				"NAD unlimited loop : %d\n", sec_nad_env.nad_factory,
				sec_nad_env.nad_result,
				sec_nad_env.nad_data,
				sec_nad_env.nad_acat,
				sec_nad_env.nad_acat_result,
				sec_nad_env.nad_acat_data,
				sec_nad_env.nad_acat_loop_count,
				sec_nad_env.nad_acat_real_count,
				sec_nad_env.nad_dram_test_need,
				sec_nad_env.nad_dram_test_result,
				sec_nad_env.nad_dram_fail_data,
				sec_nad_env.nad_dram_fail_address,
				sec_nad_env.current_nad_status,
				sec_nad_env.nad_acat_skip_fail,
				sec_nad_env.unlimited_loop);
#if defined(CONFIG_SEC_NAD_X)			
				NAD_PRINT("NAD X : %s\n", sec_nad_env.nad_extend);	
				NAD_PRINT("sec_nad_env total size : %d\n", sizeof(sec_nad_env));

				NAD_PRINT(			
				"NADX : %s\n"
				"NADX result : %s\n"
				"NADX data : 0x%x\n"
				"NADX nadx_is_excuted : %d\n", sec_nad_env.nad_extend,
				sec_nad_env.nad_extend_result,
				sec_nad_env.nad_extend_data,
				sec_nad_env.nadx_is_excuted);	
#endif				

#endif
		break;
	case NAD_PARAM_READ:
		ret = vfs_read(fp, (char *)&sec_nad_env, sizeof(struct nad_env), &(fp->f_pos));

		if (ret < 0)
			pr_err("%s: read error! %d\n", __func__, ret);
#ifdef NAD_DEBUG
			NAD_PRINT(
				"NAD factory : %s\n"
				"NAD result : %s\n"
				"NAD data : 0x%x\n"
				"NAD ACAT : %s\n"
				"NAD ACAT result : %s\n"
				"NAD ACAT data : 0x%x\n"
				"NAD ACAT loop count : %d\n"
				"NAD ACAT real count : %d\n"
				"NAD DRAM test need : %s\n"
				"NAD DRAM test result : 0x%x\n"
				"NAD DRAM test fail data : 0x%x\n"
				"NAD DRAM test fail address : 0x%08lx\n"
				"NAD status : %d\n"
				"NAD ACAT skip fail : %d\n"
				"NAD unlimited loop : %d\n", sec_nad_env.nad_factory,
				sec_nad_env.nad_result,
				sec_nad_env.nad_data,
				sec_nad_env.nad_acat,
				sec_nad_env.nad_acat_result,
				sec_nad_env.nad_acat_data,
				sec_nad_env.nad_acat_loop_count,
				sec_nad_env.nad_acat_real_count,
				sec_nad_env.nad_dram_test_need,
				sec_nad_env.nad_dram_test_result,
				sec_nad_env.nad_dram_fail_data,
				sec_nad_env.nad_dram_fail_address,
				sec_nad_env.current_nad_status,
				sec_nad_env.nad_acat_skip_fail,
				sec_nad_env.unlimited_loop);

#endif

#if defined(CONFIG_SEC_NAD_X)			
				NAD_PRINT("NAD X : %s\n", sec_nad_env.nad_extend);	
				NAD_PRINT("sec_nad_env total size : %d\n", sizeof(sec_nad_env));

				NAD_PRINT(			
				"NADX : %s\n"
				"NADX result : %s\n"
				"NADX data : 0x%x\n"
				"NADX nadx_is_excuted : %d\n", sec_nad_env.nad_extend,
				sec_nad_env.nad_extend_result,
				sec_nad_env.nad_extend_data,
				sec_nad_env.nadx_is_excuted);	
#endif	

#if defined(CONFIG_SEC_NAD_LOG)
		ret = vfs_read(fp2, (char *)nad_log_buffer, NAD_LOG_SIZE, &(fp2->f_pos));
		if (ret < 0)
			pr_err("%s: read error! %d\n", __func__, ret);
			
		NAD_PRINT("%s: NAD_LOG %s\n", __func__, nad_log_buffer);
#endif			
		break;
	}
close_fp_out:
	if (fp)
		filp_close(fp, NULL);
		
#if defined(CONFIG_SEC_NAD_LOG)		
	if (fp2)
		filp_close(fp2, NULL);
#endif


	NAD_PRINT("%s: exit %d\n", __func__, ret);
	return;
}

int sec_set_nad_param(int val)
{
	int ret = -1;

	mutex_lock(&sec_nad_param_lock);

	switch (val) {
	case NAD_PARAM_WRITE:
	case NAD_PARAM_READ:
		goto set_param;
	default:
		goto unlock_out;
	}

set_param:
	sec_nad_param_data.state = val;
	schedule_work(&sec_nad_param_data.sec_nad_work);

	/* how to determine to return success or fail ? */
	ret = 0;
unlock_out:
	mutex_unlock(&sec_nad_param_lock);
	return ret;
}

static void sec_nad_init_update(struct work_struct *work)
{
	int ret = -1;

	NAD_PRINT("%s\n", __func__);

	ret = sec_set_nad_param(NAD_PARAM_READ);
	if (ret < 0)
	pr_err("%s: read error! %d\n", __func__, ret);

}

#if defined(CONFIG_SEC_SUPPORT_VST)
static int get_vst_adjust(void)
{
	int i = 0;
	int bit = 1;
	int tmp = 0;
	int crc = 0;
	int vst_adjust;

	vst_adjust = sec_nad_env.vst_info.vst_adjust;

	NAD_PRINT("vst_adjust : 0x%x\n", vst_adjust);

	/* check crc */
	crc = vst_adjust & 0x3;
	vst_adjust = vst_adjust >> 2;

	for (i = 0; i < 30; i++) {
		if (bit & vst_adjust)
			tmp++;
		bit = bit << 1;
	}

	tmp = (tmp % 4);
	if (tmp != 0)
		tmp = 4 - tmp;

	NAD_PRINT(" crc = %d, tmp = %d\n", crc, tmp);

	if (crc != tmp)
		return 0;
	else
		return vst_adjust & NAD_VST_ADJUST_MASK;
}

static int get_vst_result(void)
{
	int vst_result;

	vst_result = sec_nad_env.vst_info.vst_result;

	NAD_PRINT("vst_result : 0x%x\n", vst_result);

	/* check magic number */
	if (((vst_result >> 8) & NAD_VST_MAGIC_MASK) == NAD_VST_MAGIC_NUM)
		return ((vst_result >> 2) & NAD_VST_RESULT_MASK);
	else
		return 0;
}
#endif

static ssize_t show_nad_stat(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int i;
	char * buf_offset;
	
	NAD_PRINT("%s\n", __func__);

#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
#if defined(CONFIG_SEC_NAD_HPM)
	if (!strncasecmp(sec_nad_env.nad_result, "FAIL", 4) && !strncasecmp(sec_nad_env.nad_second_result, "FAIL", 4)) {
	/* Both of NAD were failed. */
		return sprintf(buf, "NG_3.1_L_0x%x_0x%x_0x%x_0x%x,IT(%d),MT(%d),"
				"HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),"
				"HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),"
				"HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),"
				"HPM_G3D_L%d(%d),HPM_G3D_L%d(%d),HPM_G3D_L%d(%d),HPM_G3D_L%d(%d),"
				"HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),"
				"HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),"
				"HPM_INVALID(%d),HPM_PASS_FAIL(%d),"
				"LOTID(%s),TBL_VER(%d),BIG_G(%d),LIT_G(%d),G3D_G(%d),MIF_G(%d),"
				"HPM_BIG_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;"
				"%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"HPM_LIT_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;"
				"%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"HPM_G3D_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"HPM_MIF_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;"
				"%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"DAS(%s),BLOCK(%s),"
				"FN(%s_%s_%d_%s),FD(0x%08llx_0x%08llx_0x%08llx),"
#if defined(CONFIG_SEC_SUPPORT_VST)
				"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx),VRES(%d),VADJ(%d),FRES(%d),"
				"AVT(%d),BIG_L11(%d),BIG_L18(%d),BIG_L23(%d),"
				"MID_L6(%d),MID_L12(%d),MID_L17(%d),"
				"LIT_L4(%d),LIT_L9(%d),LIT_L14(%d),"
				"MIF_L1(%d),MIF_L5(%d),MIF_L9(%d)\n",
#else
				"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx)\n",
#endif

				sec_nad_env.nad_data,
				sec_nad_env.nad_inform2_data,
				sec_nad_env.nad_second_data,
				sec_nad_env.nad_second_inform2_data,
				sec_nad_env.nad_init_temperature,
				sec_nad_env.max_temperature,
				/* Start HPM */
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_big[0],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_big[1],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_big[2],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_big[3],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].level,
				sec_nad_env.hpm_min_diff_level_big[4],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].level,
				sec_nad_env.hpm_min_diff_level_big[5],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].level,
				sec_nad_env.hpm_min_diff_level_big[6],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].level,
				sec_nad_env.hpm_min_diff_level_big[7],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].level,
				sec_nad_env.hpm_min_diff_level_big[8],

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_lit[0],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_lit[1],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_lit[2],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_lit[3],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].level,
				sec_nad_env.hpm_min_diff_level_lit[4],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].level,
				sec_nad_env.hpm_min_diff_level_lit[5],

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_g3d[0],
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_g3d[1],
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_g3d[2],
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_g3d[3],

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_mif[0],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_mif[1],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_mif[2],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_mif[3],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].level,
				sec_nad_env.hpm_min_diff_level_mif[4],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].level,
				sec_nad_env.hpm_min_diff_level_mif[5],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].level,
				sec_nad_env.hpm_min_diff_level_mif[6],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].level,
				sec_nad_env.hpm_min_diff_level_mif[7],

				sec_nad_env.nad_hpm_info.hpm_invalid,
				sec_nad_env.nad_hpm_info.device_pass_fail,
				sec_nad_env.nad_hpm_info.LotID,
				sec_nad_env.nad_hpm_info.asv_table_ver,
				sec_nad_env.nad_hpm_info.big_grp,
				sec_nad_env.nad_hpm_info.lit_grp,
				sec_nad_env.nad_hpm_info.g3d_grp,
				sec_nad_env.nad_hpm_info.mif_grp,
				// BIG
				sec_nad_env.nad_hpm_info.hpm_info[0].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].pba_hpm_min,
				// Little
				sec_nad_env.nad_hpm_info.hpm_info[1].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].pba_hpm_min,
				//G3D
				sec_nad_env.nad_hpm_info.hpm_info[2].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].pba_hpm_min,
				//MIF
				sec_nad_env.nad_hpm_info.hpm_info[3].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].pba_hpm_min,
				/* End of HPM */
				sec_nad_env.nad_second_fail_info.das_string,
				sec_nad_env.nad_second_fail_info.block_string,
				sec_nad_env.nad_fail_info.das_string,
				sec_nad_env.nad_fail_info.block_string,
				sec_nad_env.nad_fail_info.level,
				sec_nad_env.nad_fail_info.vector_string,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].target_addr,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].read_val,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].expected_val,
				sec_nad_env.nad_second_fail_info.das_string,
				sec_nad_env.nad_second_fail_info.block_string,
				sec_nad_env.nad_second_fail_info.level,
				sec_nad_env.nad_second_fail_info.vector_string,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].target_addr,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].read_val,
#if defined(CONFIG_SEC_SUPPORT_VST)
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].expected_val,
				/* VST */
				get_vst_result(), get_vst_adjust(), sec_nad_env.vst_info.vst_f_res,
				sec_nad_env.nAsv_TABLE,
				sec_nad_env.nad_ave_current_info.current_list[5].vst_current,sec_nad_env.nad_ave_current_info.current_list[6].vst_current,sec_nad_env.nad_ave_current_info.current_list[7].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[12].vst_current,sec_nad_env.nad_ave_current_info.current_list[13].vst_current,sec_nad_env.nad_ave_current_info.current_list[14].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[19].vst_current,sec_nad_env.nad_ave_current_info.current_list[20].vst_current,sec_nad_env.nad_ave_current_info.current_list[21].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[23].vst_current,sec_nad_env.nad_ave_current_info.current_list[24].vst_current,sec_nad_env.nad_ave_current_info.current_list[25].vst_current);
#else
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].expected_val);
#endif
	} else {
		return sprintf(buf, "OK_3.1_L_0x%x_0x%x_0x%x_0x%x,IT(%d),MT(%d),"
				"HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),"
				"HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),HPM_BIG_L%d(%d),"
				"HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),HPM_LIT_L%d(%d),"
				"HPM_G3D_L%d(%d),HPM_G3D_L%d(%d),HPM_G3D_L%d(%d),HPM_G3D_L%d(%d),"
				"HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),"
				"HPM_MIF_L%d(%d),HPM_MIF_L%d(%d),"
				"HPM_INVALID(%d),HPM_PASS_FAIL(%d),"
				"LOTID(%s),TBL_VER(%d),BIG_G(%d),LIT_G(%d),G3D_G(%d),MIF_G(%d),"
				"HPM_BIG_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;"
				"%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"HPM_LIT_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;"
				"%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"HPM_G3D_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
				"HPM_MIF_DATA(%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;"
				"%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d;%d_%d_%d_%d_%d),"
#if defined(CONFIG_SEC_SUPPORT_VST)
				"OT(0x%x),TN(%d),VRES(%d),VADJ(%d),FRES(%d),"
				"AVT(%d),BIG_L11(%d),BIG_L18(%d),BIG_L23(%d),"
				"MID_L6(%d),MID_L12(%d),MID_L17(%d),"
				"LIT_L4(%d),LIT_L9(%d),LIT_L14(%d),"
				"MIF_L1(%d),MIF_L5(%d),MIF_L9(%d)\n",
#else
				"OT(0x%x),TN(%d)\n",
#endif
				sec_nad_env.nad_data,
				sec_nad_env.nad_inform2_data,
				sec_nad_env.nad_second_data,
				sec_nad_env.nad_second_inform2_data,
				sec_nad_env.nad_init_temperature,
				sec_nad_env.max_temperature,
				/* Start HPM */
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_big[0],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_big[1],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_big[2],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_big[3],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].level,
				sec_nad_env.hpm_min_diff_level_big[4],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].level,
				sec_nad_env.hpm_min_diff_level_big[5],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].level,
				sec_nad_env.hpm_min_diff_level_big[6],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].level,
				sec_nad_env.hpm_min_diff_level_big[7],
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].level,
				sec_nad_env.hpm_min_diff_level_big[8],

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_lit[0],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_lit[1],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_lit[2],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_lit[3],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].level,
				sec_nad_env.hpm_min_diff_level_lit[4],
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].level,
				sec_nad_env.hpm_min_diff_level_lit[5],

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_g3d[0],
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_g3d[1],
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_g3d[2],
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_g3d[3],

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].level,
				sec_nad_env.hpm_min_diff_level_mif[0],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].level,
				sec_nad_env.hpm_min_diff_level_mif[1],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].level,
				sec_nad_env.hpm_min_diff_level_mif[2],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].level,
				sec_nad_env.hpm_min_diff_level_mif[3],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].level,
				sec_nad_env.hpm_min_diff_level_mif[4],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].level,
				sec_nad_env.hpm_min_diff_level_mif[5],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].level,
				sec_nad_env.hpm_min_diff_level_mif[6],
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].level,
				sec_nad_env.hpm_min_diff_level_mif[7],

				sec_nad_env.nad_hpm_info.hpm_invalid,
				sec_nad_env.nad_hpm_info.device_pass_fail,
				sec_nad_env.nad_hpm_info.LotID,
				sec_nad_env.nad_hpm_info.asv_table_ver,
				sec_nad_env.nad_hpm_info.big_grp,
				sec_nad_env.nad_hpm_info.lit_grp,
				sec_nad_env.nad_hpm_info.g3d_grp,
				sec_nad_env.nad_hpm_info.mif_grp,
				// BIG
				sec_nad_env.nad_hpm_info.hpm_info[0].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[3].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[4].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[5].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[6].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[7].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].level,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[0].hpm_level_info[8].pba_hpm_min,
				// Little
				sec_nad_env.nad_hpm_info.hpm_info[1].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[3].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[4].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].level,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[1].hpm_level_info[5].pba_hpm_min,
				//G3D
				sec_nad_env.nad_hpm_info.hpm_info[2].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[2].hpm_level_info[3].pba_hpm_min,
				//MIF
				sec_nad_env.nad_hpm_info.hpm_info[3].block_pass_fail,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[0].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[1].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[2].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[3].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[4].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[5].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[6].pba_hpm_min,

				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].level,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].fused_hpm,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].pba_hpm_ave,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].pba_hpm_var,
				sec_nad_env.nad_hpm_info.hpm_info[3].hpm_level_info[7].pba_hpm_min,
				/* End of HPM */
				sec_nad_env.nad_inform3_data,
#if defined(CONFIG_SEC_SUPPORT_VST)
				sec_nad_env.nad_inform3_data & 0xFF,
				/* VST */
				get_vst_result(), get_vst_adjust(), sec_nad_env.vst_info.vst_f_res,
				sec_nad_env.nAsv_TABLE,
				sec_nad_env.nad_ave_current_info.current_list[5].vst_current,sec_nad_env.nad_ave_current_info.current_list[6].vst_current,sec_nad_env.nad_ave_current_info.current_list[7].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[12].vst_current,sec_nad_env.nad_ave_current_info.current_list[13].vst_current,sec_nad_env.nad_ave_current_info.current_list[14].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[19].vst_current,sec_nad_env.nad_ave_current_info.current_list[20].vst_current,sec_nad_env.nad_ave_current_info.current_list[21].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[23].vst_current,sec_nad_env.nad_ave_current_info.current_list[24].vst_current,sec_nad_env.nad_ave_current_info.current_list[25].vst_current);
#else
				sec_nad_env.nad_inform3_data & 0xFF);
#endif

	}
#else
	if (!strncasecmp(sec_nad_env.nad_result, "FAIL", 4) && !strncasecmp(sec_nad_env.nad_second_result, "FAIL", 4)) {
		/* Both of NAD were failed. */
				buf_offset = buf;
				buf += sprintf(buf,
			       "NG_3.1_L_0x%x_0x%x_0x%x_0x%x,IT(%d),MT(%d),DAS(%s),BLOCK(%s),"
				"FN(%s_%s_%d_%s),FD(0x%08llx_0x%08llx_0x%08llx),"
#if defined(CONFIG_SEC_SUPPORT_VST)
				"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx),VRES(%d),VADJ(%d),FRES(%d),"
				"AVT(%d),BIG_L11(%d),BIG_L16(%d),"
				"MID_L6(%d),MID_L10(%d),"
				"LIT_L4(%d),LIT_L6(%d),"
				"MIF_L1(%d),MIF_L3(%d)",
#else
				"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx)",
#endif
				sec_nad_env.nad_data,
				sec_nad_env.nad_inform2_data,
				sec_nad_env.nad_second_data,
				sec_nad_env.nad_second_inform2_data,
				sec_nad_env.nad_init_temperature,
				sec_nad_env.max_temperature,
				sec_nad_env.nad_second_fail_info.das_string,
				sec_nad_env.nad_second_fail_info.block_string,
				sec_nad_env.nad_fail_info.das_string,
				sec_nad_env.nad_fail_info.block_string,
				sec_nad_env.nad_fail_info.level,
				sec_nad_env.nad_fail_info.vector_string,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].target_addr,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].read_val,
				sec_nad_env.nad_dram_fail_information.nad_dram_fail_info[0].expected_val,
				sec_nad_env.nad_second_fail_info.das_string,
				sec_nad_env.nad_second_fail_info.block_string,
				sec_nad_env.nad_second_fail_info.level,
				sec_nad_env.nad_second_fail_info.vector_string,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].target_addr,
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].read_val,
#if defined(CONFIG_SEC_SUPPORT_VST)
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].expected_val,
				/* VST */
				get_vst_result(), get_vst_adjust(), sec_nad_env.vst_info.vst_f_res,
				sec_nad_env.nAsv_TABLE,
				sec_nad_env.nad_ave_current_info.current_list[5].vst_current,sec_nad_env.nad_ave_current_info.current_list[6].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[11].vst_current,sec_nad_env.nad_ave_current_info.current_list[12].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[17].vst_current,sec_nad_env.nad_ave_current_info.current_list[18].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[20].vst_current,sec_nad_env.nad_ave_current_info.current_list[21].vst_current);
#else
				sec_nad_env.nad_second_dram_fail_information.nad_dram_fail_info[0].expected_val);
#endif

				for (i = 0; i < sec_nad_env.nad_ave_current_info.total_num; i++) {
					if (sec_nad_env.nad_ave_current_info.current_list[i].spec_out)
						{
							buf += sprintf(buf, ",OUT_%s_L%d(%d)",nad_block_name[sec_nad_env.nad_ave_current_info.current_list[i].block], sec_nad_env.nad_ave_current_info.current_list[i].level, sec_nad_env.nad_ave_current_info.current_list[i].vst_current);
						}
				}
				
				if (sec_nad_env.last_nad_fail_status > 0) {
					buf += sprintf(buf, ",LN(%s_%s_%d_%s)",
					sec_nad_env.last_fail_data_backup.nad_fail_info.das_string,
					sec_nad_env.last_fail_data_backup.nad_fail_info.block_string,
					sec_nad_env.last_fail_data_backup.nad_fail_info.level,
					sec_nad_env.last_fail_data_backup.nad_fail_info.vector_string);
				}
				buf += sprintf(buf, "\n");
				
				return (buf - buf_offset);
	} else {
				buf_offset = buf;
				buf += sprintf(buf, "OK_3.1_L_0x%x_0x%x_0x%x_0x%x,IT(%d),MT(%d),OT(0x%x),"
#if defined(CONFIG_SEC_SUPPORT_VST)
				"TN(%d),VRES(%d),VADJ(%d),FRES(%d),"
				"AVT(%d),BIG_L11(%d),BIG_L16(%d),"
				"MID_L6(%d),MID_L10(%d),"
				"LIT_L4(%d),LIT_L6(%d),"
				"MIF_L1(%d),MIF_L3(%d)",

#else
				"TN(%d)",
#endif
				sec_nad_env.nad_data,
				sec_nad_env.nad_inform2_data,
				sec_nad_env.nad_second_data,
				sec_nad_env.nad_second_inform2_data,
				sec_nad_env.nad_init_temperature,
				sec_nad_env.max_temperature,
				sec_nad_env.nad_inform3_data,
#if defined(CONFIG_SEC_SUPPORT_VST)
				sec_nad_env.nad_inform3_data & 0xFF,
				/* VST */
				get_vst_result(), get_vst_adjust(), sec_nad_env.vst_info.vst_f_res,
				sec_nad_env.nAsv_TABLE,
				sec_nad_env.nad_ave_current_info.current_list[5].vst_current,sec_nad_env.nad_ave_current_info.current_list[6].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[11].vst_current,sec_nad_env.nad_ave_current_info.current_list[12].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[17].vst_current,sec_nad_env.nad_ave_current_info.current_list[18].vst_current,
				sec_nad_env.nad_ave_current_info.current_list[20].vst_current,sec_nad_env.nad_ave_current_info.current_list[21].vst_current);
#else
				sec_nad_env.nad_inform3_data & 0xFF);
#endif



				for (i = 0; i < sec_nad_env.nad_ave_current_info.total_num; i++) {
					if (sec_nad_env.nad_ave_current_info.current_list[i].spec_out)
						{
							buf += sprintf(buf, ",OUT_%s_L%d(%d)",nad_block_name[sec_nad_env.nad_ave_current_info.current_list[i].block], sec_nad_env.nad_ave_current_info.current_list[i].level, sec_nad_env.nad_ave_current_info.current_list[i].vst_current);
						}
				}
				
				if (sec_nad_env.last_nad_fail_status > 0) {
					buf += sprintf(buf, ",LN(%s_%s_%d_%s)",
					sec_nad_env.last_fail_data_backup.nad_fail_info.das_string,
					sec_nad_env.last_fail_data_backup.nad_fail_info.block_string,
					sec_nad_env.last_fail_data_backup.nad_fail_info.level,
					sec_nad_env.last_fail_data_backup.nad_fail_info.vector_string);
				}
				buf += sprintf(buf, "\n");
				
				return (buf - buf_offset);
	}
#endif
#else
	if (!strncasecmp(sec_nad_env.nad_result, "FAIL", 4))
		return sprintf(buf, "NG_2.0_0x%x,T(%d)\n", sec_nad_env.nad_data, sec_nad_env.max_temperature);
	else
		return sprintf(buf, "OK_2.0_0x%x,T(%d)\n", sec_nad_env.nad_data, sec_nad_env.max_temperature);
#endif
}
static DEVICE_ATTR(nad_stat, S_IRUGO, show_nad_stat, NULL);

static ssize_t store_nad_erase(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = -1;

	if (!strncmp(buf, "erase", 5)) {
		strncpy(sec_nad_env.nad_factory, "GAP", 3);
#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
		strncpy(sec_nad_env.nad_second, "DUMM", 4);
#endif
#if defined(CONFIG_SEC_NAD_HPM)
		/* Initialize NAD HPM data */
		memset(&sec_nad_env.nad_hpm_info, 0, sizeof(nad_hpm));
		memset(&sec_nad_env.hpm_min_diff_level_big, 0, sizeof(sec_nad_env.hpm_min_diff_level_big));
		memset(&sec_nad_env.hpm_min_diff_level_lit, 0, sizeof(sec_nad_env.hpm_min_diff_level_lit));
		memset(&sec_nad_env.hpm_min_diff_level_g3d, 0, sizeof(sec_nad_env.hpm_min_diff_level_g3d));
		memset(&sec_nad_env.hpm_min_diff_level_mif, 0, sizeof(sec_nad_env.hpm_min_diff_level_mif));
#endif
#if defined(CONFIG_SEC_NAD_API)
		sec_nad_env.nad_api_status = 0;
#endif
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0)
		pr_err("%s: write error! %d\n", __func__, ret);

		return count;
	} else
		return count;
}

static ssize_t show_nad_erase(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	NAD_PRINT("%s\n", __func__);

	if (!strncmp(sec_nad_env.nad_factory, "GAP", 3))
		return sprintf(buf, "OK\n");
	else
		return sprintf(buf, "NG\n");
}

static DEVICE_ATTR(nad_erase, S_IRUGO | S_IWUSR, show_nad_erase, store_nad_erase);

static ssize_t show_nad_acat(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	NAD_PRINT("%s\n", __func__);

#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
#if defined(CONFIG_SEC_NAD_HPM)
	/* Check status if ACAT NAD was excuted */
	if (sec_nad_env.current_nad_status >= NAD_ACAT_FLAG) {
		if (!strncasecmp(sec_nad_env.nad_acat_result, "FAIL", 4) && !strncasecmp(sec_nad_env.nad_acat_second_result, "FAIL", 4)) {
			return sprintf(buf, "NG_3.0_L_0x%x_0x%x_0x%x_0x%x,%d,%d,HPM(NULL),%s,%s,"
					"FN(%s_%s_%d_%s),FD(0x%08llx_0x%08llx_0x%08llx),"
					"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx)\n",
					sec_nad_env.nad_acat_data,
					sec_nad_env.nad_acat_inform2_data,
					sec_nad_env.nad_acat_second_data,
					sec_nad_env.nad_acat_second_inform2_data,
					sec_nad_env.nad_acat_init_temperature,
					sec_nad_env.nad_acat_max_temperature,
					sec_nad_env.nad_acat_second_fail_info.das_string,
					sec_nad_env.nad_acat_second_fail_info.block_string,
					sec_nad_env.nad_acat_fail_info.das_string,
					sec_nad_env.nad_acat_fail_info.block_string,
					sec_nad_env.nad_acat_fail_info.level,
					sec_nad_env.nad_acat_fail_info.vector_string,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].target_addr,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].read_val,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].expected_val,
					sec_nad_env.nad_acat_second_fail_info.das_string,
					sec_nad_env.nad_acat_second_fail_info.block_string,
					sec_nad_env.nad_acat_second_fail_info.level,
					sec_nad_env.nad_acat_second_fail_info.vector_string,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].target_addr,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].read_val,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].expected_val);
		} else {
			return sprintf(buf, "OK_3.0_L_0x%x_0x%x_0x%x_0x%x,%d,%d,HPM(NULL),OT(0x%x)\n",
					sec_nad_env.nad_acat_data,
					sec_nad_env.nad_acat_inform2_data,
					sec_nad_env.nad_acat_second_data,
					sec_nad_env.nad_acat_second_inform2_data,
					sec_nad_env.nad_acat_init_temperature,
					sec_nad_env.nad_acat_max_temperature,
					sec_nad_env.nad_acat_inform3_data);
		}
	} else
		return sprintf(buf, "NO_3.0_L_NADTEST\n");
#else
	/* Check status if ACAT NAD was excuted */
	if (sec_nad_env.current_nad_status >= NAD_ACAT_FLAG) {
		if (!strncasecmp(sec_nad_env.nad_acat_result, "FAIL", 4) && !strncasecmp(sec_nad_env.nad_acat_second_result, "FAIL", 4)) {
			return sprintf(buf, "NG_3.0_L_0x%x_0x%x_0x%x_0x%x,%d,%d,HPM(NULL),%s,%s,"
					"FN(%s_%s_%d_%s),FD(0x%08llx_0x%08llx_0x%08llx),"
					"FNS(%s_%s_%d_%s),FDS(0x%08llx_0x%08llx_0x%08llx)\n",
					sec_nad_env.nad_acat_data,
					sec_nad_env.nad_acat_inform2_data,
					sec_nad_env.nad_acat_second_data,
					sec_nad_env.nad_acat_second_inform2_data,
					sec_nad_env.nad_acat_init_temperature,
					sec_nad_env.nad_acat_max_temperature,
					sec_nad_env.nad_acat_second_fail_info.das_string,
					sec_nad_env.nad_acat_second_fail_info.block_string,
					sec_nad_env.nad_acat_fail_info.das_string,
					sec_nad_env.nad_acat_fail_info.block_string,
					sec_nad_env.nad_acat_fail_info.level,
					sec_nad_env.nad_acat_fail_info.vector_string,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].target_addr,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].read_val,
					sec_nad_env.nad_acat_dram_fail_information.nad_dram_fail_info[0].expected_val,
					sec_nad_env.nad_acat_second_fail_info.das_string,
					sec_nad_env.nad_acat_second_fail_info.block_string,
					sec_nad_env.nad_acat_second_fail_info.level,
					sec_nad_env.nad_acat_second_fail_info.vector_string,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].target_addr,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].read_val,
					sec_nad_env.nad_acat_second_dram_fail_information.nad_dram_fail_info[0].expected_val);
		} else {
			return sprintf(buf, "OK_3.0_L_0x%x_0x%x_0x%x_0x%x,%d,%d,HPM(NULL),OT(0x%x)\n",
					sec_nad_env.nad_acat_data,
					sec_nad_env.nad_acat_inform2_data,
					sec_nad_env.nad_acat_second_data,
					sec_nad_env.nad_acat_second_inform2_data,
					sec_nad_env.nad_acat_init_temperature,
					sec_nad_env.nad_acat_max_temperature,
					sec_nad_env.nad_acat_inform3_data);
		}
	} else
	return sprintf(buf, "NO_3.0_L_NADTEST\n");
#endif
#else
	/* Check status if ACAT NAD was excuted */
	if (sec_nad_env.current_nad_status == NAD_ACAT_FLAG) {
		if (!strncasecmp(sec_nad_env.nad_acat_result, "FAIL", 4))
			return sprintf(buf, "NG_ACAT_0x%x,T(%d)\n", sec_nad_env.nad_acat_data, sec_nad_env.nad_acat_max_temperature);
		else
			return sprintf(buf, "OK_ACAT_0x%x,T(%d)\n", sec_nad_env.nad_acat_data, sec_nad_env.nad_acat_max_temperature);
	} else
		return sprintf(buf, "NO_ACAT_NADTEST\n");
#endif
}

static ssize_t store_nad_acat(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int ret = -1;
	int idx = 0;
	char temp[NAD_BUFF_SIZE*3];
	char nad_cmd[NAD_CMD_LIST][NAD_BUFF_SIZE];
	char *nad_ptr, *string;

	NAD_PRINT("buf : %s count : %d\n", buf, (int)count);

	if ((int)count < NAD_BUFF_SIZE)
		return -EINVAL;

	/* Copy buf to nad temp */
	strncpy(temp, buf, NAD_BUFF_SIZE*3);
	string = temp;

	while (idx < NAD_CMD_LIST) {
		nad_ptr = strsep(&string, ",");
		strcpy(nad_cmd[idx++], nad_ptr);
	}

/*
* ACAT NAD Write Data format
*
* nad_acat,20,0 : ACAT Loop test (20 cycles)
* nad_acat,20,1 : ACAT Loop & DRAM test (20 cycles & 1 DRAM test)
* nad_acat,0,1  : ACAT DRAM test (1 DRAM test)
*
*/
	if (!strncmp(buf, "nad_acat", 8)) {
		/* Get NAD loop count */
		ret = sscanf(nad_cmd[1], "%d\n", &sec_nad_env.nad_acat_loop_count);
		if (ret != 1)
			return -EINVAL;

		/* case 1 : ACAT NAD */
		if (sec_nad_env.nad_acat_loop_count > 0) {
			NAD_PRINT("ACAT NAD test command.\n");

			strncpy(sec_nad_env.nad_acat, "ACAT", 4);
#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
			strncpy(sec_nad_env.nad_acat_second, "DUMM", 4);
			sec_nad_env.nad_acat_second_running_count = 0;
			sec_nad_env.acat_fail_retry_count = 0;
			sec_nad_env.nad_acat_real_count = 0;
#endif
			sec_nad_env.nad_acat_data = 0;
			sec_nad_env.nad_acat_real_count = 0;
			sec_nad_env.current_nad_status = 0;
			sec_nad_env.nad_acat_skip_fail = 0;
			sec_nad_env.unlimited_loop = 0;
			sec_nad_env.max_temperature = 0;
			sec_nad_env.nad_acat_max_temperature = 0;
		}
#if defined(CONFIG_SEC_NAD_X)		
		if(sec_nad_env.nad_acat_loop_count ==  444)
		{
			NAD_PRINT("NADX test command.\n");

			strncpy(sec_nad_env.nad_extend, "NADX", 4);
			strncpy(sec_nad_env.nad_extend_result, "NULL",4);
			sec_nad_env.nad_extend_init_temperature = 0;
			sec_nad_env.nad_extend_max_temperature = 0;	
			
			sec_nad_env.nad_extend_data = 0;
			sec_nad_env.nad_extend_inform2_data = 0;
			sec_nad_env.nad_extend_inform3_data = 0;
			sec_nad_env.current_nad_status = 0;
			
			sec_nad_env.nad_extend_real_count=0;
			sec_nad_env.nad_extend_loop_count=1;
			sec_nad_env.nad_extend_skip_fail = 0;
			
			sec_nad_env.nad_acat_loop_count = 0;
			strncpy(sec_nad_env.nad_acat, "NONE", 4);
			
#if defined(CONFIG_SEC_SUPPORT_SECOND_NAD)
			strncpy(sec_nad_env.nad_acat_second, "DUMM", 4);
			sec_nad_env.nad_acat_second_running_count = 0;
			sec_nad_env.acat_fail_retry_count = 0;
			sec_nad_env.nad_acat_real_count = 0;
#endif
			sec_nad_env.nad_acat_data = 0;
			sec_nad_env.nad_acat_real_count = 0;
			sec_nad_env.current_nad_status = 0;
			sec_nad_env.nad_acat_skip_fail = 0;
			sec_nad_env.unlimited_loop = 0;
			sec_nad_env.max_temperature = 0;
			sec_nad_env.nad_acat_max_temperature = 0;

		}
#endif
		/* case 2 : DRAM test */
		if (!strncmp(nad_cmd[2], "1", 1)) {
			NAD_PRINT("DRAM test command.\n");

			strncpy(sec_nad_env.nad_dram_test_need, "DRAM", 4);
			sec_nad_env.nad_dram_test_result = 0;
			sec_nad_env.nad_dram_fail_data = 0;
			sec_nad_env.nad_dram_fail_address = 0;
		} else
			strncpy(sec_nad_env.nad_dram_test_need, "DONE", 4);

		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0) {
			pr_err("%s: write error! %d\n", __func__, ret);
			goto err_out;
		}

		return count;
	} else
		return count;
err_out:
	return ret;
}

static DEVICE_ATTR(nad_acat, S_IRUGO | S_IWUSR,  show_nad_acat, store_nad_acat);

static ssize_t show_nad_dram(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	NAD_PRINT("%s\n", __func__);

	if (sec_nad_env.nad_dram_test_result == NAD_DRAM_TEST_FAIL)
		return sprintf(buf, "NG_DRAM_0x%08lx,0x%x\n",
				sec_nad_env.nad_dram_fail_address,
				sec_nad_env.nad_dram_fail_data);
	else if (sec_nad_env.nad_dram_test_result == NAD_DRAM_TEST_PASS)
		return sprintf(buf, "OK_DRAM\n");
	else
		return sprintf(buf, "NO_DRAMTEST\n");
}
static DEVICE_ATTR(nad_dram, S_IRUGO, show_nad_dram, NULL);

static ssize_t show_nad_all(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	return sprintf(buf,
			"NAD factory : %s\n"
			"NAD result : %s\n"
			"NAD data : 0x%x\n"
			"NAD ACAT : %s\n"
			"NAD ACAT result : %s\n"
			"NAD ACAT data : 0x%x\n"
			"NAD ACAT loop count : %d\n"
			"NAD ACAT real count : %d\n"
			"NAD DRAM test need : %s\n"
			"NAD DRAM test result : 0x%x\n"
			"NAD DRAM test fail data : 0x%x\n"
			"NAD DRAM test fail address : 0x%08lx\n"
			"NAD status : %d\n"
			"NAD ACAT skip fail : %d\n"
			"NAD unlimited loop : %d\n", sec_nad_env.nad_factory,
			sec_nad_env.nad_result,
			sec_nad_env.nad_data,
			sec_nad_env.nad_acat,
			sec_nad_env.nad_acat_result,
			sec_nad_env.nad_acat_data,
			sec_nad_env.nad_acat_loop_count,
			sec_nad_env.nad_acat_real_count,
			sec_nad_env.nad_dram_test_need,
			sec_nad_env.nad_dram_test_result,
			sec_nad_env.nad_dram_fail_data,
			sec_nad_env.nad_dram_fail_address,
			sec_nad_env.current_nad_status,
			sec_nad_env.nad_acat_skip_fail,
			sec_nad_env.unlimited_loop);
}
static DEVICE_ATTR(nad_all, S_IRUGO, show_nad_all, NULL);

#if defined(CONFIG_SEC_NAD_X)
static ssize_t show_nad_x_run(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	int ret;

	if (sec_nad_env.nadx_is_excuted == 1) {
		sec_nad_env.nadx_is_excuted = 0;

		// clear nadx executed
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
                if (ret < 0) {
                        pr_err("%s: write error! %d\n", __func__, ret);
                        return sprintf(buf, "%s\n", "RUN");
                }
		return sprintf(buf, "%s\n", "RUN");
	} else
		return sprintf(buf, "%s\n", "NORUN");

}
static DEVICE_ATTR(nad_x_run, S_IRUGO, show_nad_x_run, NULL);

static ssize_t show_nad_fac_result(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	if (!strncmp(sec_nad_env.nad_extend_result, "PASS", 4))
		return sprintf(buf, "%s\n", sec_nad_env.nad_extend_result);
	else if (!strncmp(sec_nad_env.nad_extend_result, "MAIN", 4))
		return sprintf(buf, "%s\n", "FAIL,MAIN_NAD");
	else if (!strncmp(sec_nad_env.nad_extend_result, "FAIL", 4))
		return sprintf(buf, "FAIL,NG_NADX_DAS(%s),BLOCK(%s),LEVEL(%d),VECTOR(%s),PRE(%d,%d,%d,%d,%d,%d)\n",
					sec_nad_env.nad_extend_fail_info.das_string,
					sec_nad_env.nad_extend_fail_info.block_string,
					sec_nad_env.nad_extend_fail_info.level,
					sec_nad_env.nad_extend_fail_info.vector_string,
					sec_nad_env.nad_X_Pre_Domain_Level[0],
					sec_nad_env.nad_X_Pre_Domain_Level[1],
					sec_nad_env.nad_X_Pre_Domain_Level[2],
					sec_nad_env.nad_X_Pre_Domain_Level[3],
					sec_nad_env.nad_X_Pre_Domain_Level[4],
					sec_nad_env.nad_X_Pre_Domain_Level[5]);
	else
		return sprintf(buf, "%s\n", "NG");
}

static ssize_t store_nad_fac_result(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int param_update = 0;
	int ret;

	NAD_PRINT("buf : %s count = %d\n", buf, (int)count);	

	/* check cmd */
	if (!strncmp(buf, "nadx", 4)) {
		NAD_PRINT("run NADX command.\n");
		strncpy(sec_nad_env.nad_extend, "NADX", 4);
		strncpy(sec_nad_env.nad_extend_result, "NULL",4);
		sec_nad_env.nad_extend_init_temperature = 0;
		sec_nad_env.nad_extend_max_temperature = 0;
		sec_nad_env.nad_extend_data = 0;
		sec_nad_env.nad_extend_inform2_data = 0;
		sec_nad_env.nad_extend_inform3_data = 0;
		sec_nad_env.current_nad_status = 0;
		sec_nad_env.nad_extend_real_count=0;
		sec_nad_env.nad_extend_loop_count=1;
		sec_nad_env.nad_extend_skip_fail = 0;
		param_update = 1;
	} else if (!strncmp(buf, "mainfail", 8)) {
		NAD_PRINT("update failed main nad result\n");
		strncpy(sec_nad_env.nad_extend_result, "MAIN", 4);
		param_update = 1;
	}

	if (param_update == 1) {
		ret = sec_set_nad_param(NAD_PARAM_WRITE);
		if (ret < 0) {
			pr_err("%s: write error! %d\n", __func__, ret);
			return -1;
		}
	}
	return count;
}
static DEVICE_ATTR(nad_fac_result, S_IRUGO | S_IWUSR, show_nad_fac_result, store_nad_fac_result);
#endif

static ssize_t show_nad_support(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	NAD_PRINT("%s\n", __func__);

	if (sec_nad_env.nad_support == NAD_SUPPORT_FLAG)
#if defined(CONFIG_SEC_NAD_X)
		return sprintf(buf, "SUPPORT_NADX\n");
#else
		return sprintf(buf, "SUPPORT\n");
#endif
	else
		return sprintf(buf, "NOT_SUPPORT\n");
}
static DEVICE_ATTR(nad_support, S_IRUGO, show_nad_support, NULL);

static ssize_t show_nad_version(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	unsigned int nad_project_index;
	unsigned int nad_project_index2;
	unsigned int nad_fw_version;
	char chipset_name[12];
	
	nad_project_index = (sec_nad_env.nad_data >> 28) & 0xF;
	nad_project_index2 = (sec_nad_env.nad_inform2_data >> 29) & 0xF;
	nad_fw_version = ((sec_nad_env.nad_data<<4) >> 28) & 0xF;

	memset(chipset_name, 0x0, sizeof(chipset_name));
	NAD_PRINT("%s\n", __func__);
	
	if(sec_nad_env.nad_data != 0)
	{
	    if(nad_project_index == EXYNOS7885) {
			if(nad_project_index2 == 0) {
				strcpy(chipset_name, "EXYNOS7885");
			} 
			else if (nad_project_index2 == 1) {
				strcpy(chipset_name, "EXYNOS7884");
			} 
			else if (nad_project_index2 == 2)
			{
				strcpy(chipset_name, "EXYNOS7883");
			}
		}
		else
		{
			strcpy(chipset_name, nad_chipset_name[nad_project_index]);
		}
		return sprintf(buf,"NAD_%s_Ver.0x%x\n", chipset_name, nad_fw_version);	
			
	}
	else
		return sprintf(buf,"NAD_NO_INFORMATION\n");
	
}
static DEVICE_ATTR(nad_version, S_IRUGO, show_nad_version, NULL);

#if defined(CONFIG_SEC_NAD_API)
static void make_result_data_to_string(void)
{
	int i = 0;

	NAD_PRINT("%s : api total count(%d)\n", __func__, sec_nad_env.nad_api_total_count);

	/* Make result string if array is empty */
	if (!strlen(nad_api_result_string)) {
		for (i = 0; i < sec_nad_env.nad_api_total_count; i++) {
			NAD_PRINT("%s : name(%s) result(%d)\n", __func__,
				sec_nad_env.nad_api_info[i].name, sec_nad_env.nad_api_info[i].result);
			/* Failed gpio test */
			if (sec_nad_env.nad_api_info[i].result) {
				strcat(nad_api_result_string, sec_nad_env.nad_api_info[i].name);
				strcat(nad_api_result_string, ",");
			}
		}
		nad_api_result_string[strlen(nad_api_result_string)-1] = '\0';
	}
}

static ssize_t show_nad_api(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	NAD_PRINT("%s\n", __func__);

	/* Check nad api running status */
	if (sec_nad_env.nad_api_status == MAGIC_NAD_API_SUCCESS) {
		if (sec_nad_env.nad_api_magic == MAGIC_NAD_API_SUCCESS)
			return sprintf(buf, "OK_%d\n", sec_nad_env.nad_api_total_count);
		else {
			make_result_data_to_string();
			return sprintf(buf, "NG_%d,%s\n", sec_nad_env.nad_api_total_count, nad_api_result_string);
		}
	} else
	return sprintf(buf, "NONE\n");
}
static DEVICE_ATTR(nad_api, S_IRUGO, show_nad_api, NULL);
#endif

#if defined(CONFIG_SEC_NAD_LOG)
static ssize_t sec_nad_log_read_all(struct file *file, char __user *buf,
				size_t len, loff_t *offset)
{

	loff_t pos = *offset;
	ssize_t count;

	if (pos >= NAD_LOG_SIZE)
		return 0;

	count = min(len, (size_t)(NAD_LOG_SIZE - pos));
	if (copy_to_user(buf, nad_log_buffer + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
	
}

static const struct file_operations sec_nad_log_file_ops = {
.owner = THIS_MODULE,
.read = sec_nad_log_read_all,
};
#endif
#endif




static int __init sec_nad_init(void)
{
	int ret = 0;

#if defined(CONFIG_SEC_FACTORY)

#if defined(CONFIG_SEC_NAD_LOG)
	struct proc_dir_entry *entry;
#endif

	struct device *sec_nad;

	NAD_PRINT("%s\n", __func__);

	/* Skip nad init when device goes to lp charging */
	if (lpcharge)
		return ret;

	sec_nad = sec_device_create(NULL, "sec_nad");

	if (IS_ERR(sec_nad)) {
		pr_err("%s Failed to create device(sec_nad)!\n", __func__);
		return PTR_ERR(sec_nad);
    }

	ret = device_create_file(sec_nad, &dev_attr_nad_stat);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_erase);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_acat);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_dram);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_all);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_support);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}

#if defined(CONFIG_SEC_NAD_API)
	ret = device_create_file(sec_nad, &dev_attr_nad_api);
	if (ret) {
	pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}
#endif

#if defined(CONFIG_SEC_NAD_X)
	ret = device_create_file(sec_nad, &dev_attr_nad_fac_result);
	if (ret) {
		pr_err("%s: Failed to create fac_result file\n", __func__);
		goto err_create_nad_sysfs;
	}

	ret = device_create_file(sec_nad, &dev_attr_nad_x_run);
	if (ret) {
		pr_err("%s: Failed to create fac_result file\n", __func__);
		goto err_create_nad_sysfs;
	}
#endif

	ret = device_create_file(sec_nad, &dev_attr_nad_version);
	if (ret) {
		pr_err("%s: Failed to create device file\n", __func__);
		goto err_create_nad_sysfs;
	}
#if defined(CONFIG_SEC_NAD_LOG)	
	entry = proc_create("sec_nad_log", 0440, NULL, &sec_nad_log_file_ops);
	if (!entry) {
	pr_err("%s: failed to create proc entry\n", __func__);
	return 0;
	}
	proc_set_size(entry, NAD_LOG_SIZE);
#endif	

	/* Initialize nad param struct */
	sec_nad_param_data.offset = NAD_ENV_OFFSET;
	sec_nad_param_data.state = NAD_PARAM_EMPTY;
	sec_nad_param_data.retry_cnt = NAD_RETRY_COUNT;
	sec_nad_param_data.curr_cnt = 0;

	INIT_WORK(&sec_nad_param_data.sec_nad_work, sec_nad_param_update);
	INIT_DELAYED_WORK(&sec_nad_param_data.sec_nad_delay_work, sec_nad_init_update);

#if defined(CONFIG_SEC_NAD_MANUAL_PARAM_READTIME)
	schedule_delayed_work(&sec_nad_param_data.sec_nad_delay_work, HZ * CONFIG_SEC_NAD_MANUAL_PARAM_READTIME);
#else
	schedule_delayed_work(&sec_nad_param_data.sec_nad_delay_work, HZ * 10);
#endif

	return 0;
err_create_nad_sysfs:
	sec_device_destroy(sec_nad->devt);
	return ret;
#else
	return ret;
#endif
}

static void __exit sec_nad_exit(void)
{
#if defined(CONFIG_SEC_FACTORY)
	cancel_work_sync(&sec_nad_param_data.sec_nad_work);
	cancel_delayed_work(&sec_nad_param_data.sec_nad_delay_work);
	NAD_PRINT("%s: exit\n", __func__);
#endif
}

module_init(sec_nad_init);
module_exit(sec_nad_exit);

