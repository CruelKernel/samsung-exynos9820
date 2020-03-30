/*
 *sec_hw_param.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sec_debug.h>
#include <linux/sec_ext.h>
#include <linux/sec_class.h>
#include <linux/uaccess.h>
#include <linux/soc/samsung/exynos-soc.h>
#include <soc/samsung/exynos-pm.h>
#include <linux/io.h>
#include <linux/thermal.h>

#define MAX_DDR_VENDOR 16
#define LPDDR_BASE      0x02062c00
#define DATA_SIZE 1024
#define LOT_STRING_LEN 5

/*
 * LPDDR4 (JESD209-4) MR5 Manufacturer ID
 * 0000 0000B : Reserved
 * 0000 0001B : Samsung
 * 0000 0101B : Nanya
 * 0000 0110B : SK hynix
 * 0000 1000B : Winbond
 * 0000 1001B : ESMT
 * 1111 1111B : Micron
 * All others : Reserved
 */
static char *lpddr4_manufacture_name[MAX_DDR_VENDOR] = {
	"NA",
	"SEC", /* Samsung */
	"NA",
	"NA",
	"NA",
	"NAN", /* Nanya */
	"HYN", /* SK hynix */
	"NA",
	"WIN", /* Winbond */
	"ESM", /* ESMT */
	"NA",
	"NA",
	"NA",
	"NA",
	"NA",
	"MIC", /* Micron */
};

static unsigned int sec_hw_rev;
static unsigned int chipid_fail_cnt;
static unsigned int lpi_timeout_cnt;
static unsigned int cache_err_cnt;
static unsigned int codediff_cnt;
#if defined(CONFIG_SAMSUNG_VST_CAL)
static unsigned int vst_result;
#endif
static unsigned long pcb_offset;
static unsigned long smd_offset;
static unsigned int lpddr4_size;
static char warranty = 'D';

#if 1	/* DDR training result structure */
#define MK_DDR_TRN_DATA_BASE 0x02062000
#define NUM_OF_CH                       (4)
#define NUM_OF_TRN_OFFSET_INFO          (2)
#define NUM_OF_TRN_DLL_INFO             (1)
#define NUM_OF_TRN_GATE_INFO            (4)
#define NUM_OF_TRN_RD_DESKEW_INFO       (9)
#define NUM_OF_TRN_RD_DESKEWQ_INFO      (2)
#define NUM_OF_TRN_WR_DESKEW_INFO       (9)
#define NUM_OF_TRN_INFO                 (0)
enum phy_rank_info {
	PHY_RANK_0,
	PHY_RANK_1,
	PHY_RANK_ALL,
};

enum phy_byte_info {
	PHY_BYTE_0,
	PHY_BYTE_1,
	PHY_BYTE_ALL,
};

struct phy_trn_cbt_info_t {
	unsigned char ca[6];
	unsigned char ck;
	unsigned char cs[2];
	unsigned char cke[2];

};

struct phy_trn_read_dqs_info_t {
	unsigned char center[PHY_BYTE_ALL];
	unsigned char left[PHY_BYTE_ALL];
};

struct phy_trn_wr_lvl_info_t {
	unsigned short code[PHY_BYTE_ALL];
};

struct phy_trn_gate_info_t {
	unsigned short center[PHY_BYTE_ALL];
	unsigned char  cycle[PHY_BYTE_ALL];
};

struct phy_trn_read_info_t {
	unsigned short deskewc[NUM_OF_TRN_RD_DESKEW_INFO][PHY_BYTE_ALL];
	unsigned short deskewl[NUM_OF_TRN_RD_DESKEW_INFO][PHY_BYTE_ALL];
	struct phy_trn_read_dqs_info_t  dqs;
};

struct phy_trn_write_info_t {
	unsigned short deskewc[NUM_OF_TRN_WR_DESKEW_INFO][PHY_BYTE_ALL];
	unsigned short deskewl[NUM_OF_TRN_WR_DESKEW_INFO][PHY_BYTE_ALL];

};

struct phy_trn_all_level_deskew_offset_info_t {
	signed char offset[PHY_BYTE_ALL];

};

struct phy_trn_prbs_info_t {
    short read[PHY_RANK_ALL][PHY_BYTE_ALL];
    short write[PHY_RANK_ALL][PHY_BYTE_ALL];
}; 
 

struct phy_trn_soc_vref_info_t {
	unsigned char net_lv;
	unsigned char vref;
	unsigned int average;
	unsigned char left;
	unsigned char right;

};

union mr14_t {
	volatile unsigned int data;
	struct  {
		volatile unsigned int	vref_dq : (5 - 0 + 1);
		volatile unsigned int	vr_dq : (6 - 6 + 1);
		volatile unsigned int	reserved_7 : (7 - 7 + 1);
	} bitfield;
};

struct phy_trn_memory_vref_info_t {
	unsigned char net_lv;
	unsigned char vref;
	unsigned int average;
	unsigned char left;
	unsigned char right;
	union mr14_t mr14;
};

struct phy_trn_clock_duty_info_t {
 unsigned char nmos;
 unsigned char pmos;
 unsigned int max_size;
};

struct phy_trn_data_t {
	unsigned int                      dll;
	struct phy_trn_cbt_info_t           cbt;
	struct phy_trn_wr_lvl_info_t        wr_lvl;
	struct phy_trn_gate_info_t          gate[PHY_RANK_ALL];
	struct phy_trn_read_info_t          read[PHY_RANK_ALL];//read training per rank enabled from KC
	struct phy_trn_write_info_t         write[PHY_RANK_ALL];
	struct phy_trn_all_level_deskew_offset_info_t  read_offset[12][PHY_RANK_ALL];
	struct phy_trn_all_level_deskew_offset_info_t  write_offset[12][PHY_RANK_ALL];
	struct phy_trn_prbs_info_t          prbs;
	struct phy_trn_soc_vref_info_t      soc_vref[3][PHY_RANK_ALL];
	struct phy_trn_memory_vref_info_t   memory_vref[2][PHY_RANK_ALL];
	struct phy_trn_clock_duty_info_t                clock_duty;
};
  
#endif	/* DDR training result structure */

static int __init sec_hw_param_get_hw_rev(char *arg)
{
	get_option(&arg, &sec_hw_rev);
	return 0;
}

early_param("androidboot.revision", sec_hw_param_get_hw_rev);

static int __init sec_hw_param_check_chip_id(char *arg)
{
	get_option(&arg, &chipid_fail_cnt);
	return 0;
}

early_param("sec_debug.chipidfail_cnt", sec_hw_param_check_chip_id);

static int __init sec_hw_param_check_lpi_timeout(char *arg)
{
	get_option(&arg, &lpi_timeout_cnt);
	return 0;
}

early_param("sec_debug.lpitimeout_cnt", sec_hw_param_check_lpi_timeout);

static int __init sec_hw_param_cache_error(char *arg)
{
	get_option(&arg, &cache_err_cnt);
	return 0;
}

early_param("sec_debug.cache_err_cnt", sec_hw_param_cache_error);

static int __init sec_hw_param_code_diff(char *arg)
{
	get_option(&arg, &codediff_cnt);
	return 0;
}

early_param("sec_debug.codediff_cnt", sec_hw_param_code_diff);

#if defined(CONFIG_SAMSUNG_VST_CAL)
static int __init sec_hw_param_vst_result(char *arg)
{
	get_option(&arg, &vst_result);
	return 0;
}

early_param("sec_debug.vst_result", sec_hw_param_vst_result);
#endif

static int __init sec_hw_param_pcb_offset(char *arg)
{
	pcb_offset = simple_strtoul(arg, NULL, 10);
	return 0;
}

early_param("sec_debug.pcb_offset", sec_hw_param_pcb_offset);

static int __init sec_hw_param_smd_offset(char *arg)
{
	smd_offset = simple_strtoul(arg, NULL, 10);
	return 0;
}

early_param("sec_debug.smd_offset", sec_hw_param_smd_offset);

static int __init sec_hw_param_lpddr4_size(char *arg)
{
	get_option(&arg, &lpddr4_size);
	return 0;
}

early_param("sec_debug.lpddr4_size", sec_hw_param_lpddr4_size);

static int __init sec_hw_param_bin(char *arg)
{
	warranty = (char)*arg;
	return 0;
}

early_param("sec_debug.bin", sec_hw_param_bin);

static u32 chipid_reverse_value(u32 value, u32 bitcnt)
{
	int tmp, ret = 0;
	int i;

	for (i = 0; i < bitcnt; i++) {
		tmp = (value >> i) & 0x1;
		ret += tmp << ((bitcnt - 1) - i);
	}

	return ret;
}

static void chipid_dec_to_36(u32 in, char *p)
{
	int mod;
	int i;

	for (i = LOT_STRING_LEN - 1; i >= 1; i--) {
		mod = in % 36;
		in /= 36;
		p[i] = (mod < 10) ? (mod + '0') : (mod - 10 + 'A');
	}

	p[0] = 'N';
	p[LOT_STRING_LEN] = '\0';
}

static char *get_dram_manufacturer(void)
{
	void *lpddr_reg;
	u64 val;
	int mr5_vendor_id = 0;

	lpddr_reg = ioremap(LPDDR_BASE, SZ_64);

	if (!lpddr_reg) {
		pr_err("failed to get i/o address lpddr_reg\n");
		return lpddr4_manufacture_name[mr5_vendor_id];
	}

	val = readq((void __iomem *)lpddr_reg);

	mr5_vendor_id = (val & 0xff00) >> 8;

	return lpddr4_manufacture_name[mr5_vendor_id];
}

static ssize_t sec_hw_param_ap_info_show(struct kobject *kobj,
					 struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;
	int reverse_id_0 = 0;
	u32 tmp = 0;
	char lot_id[LOT_STRING_LEN + 1];
	char val[32] = {0, };

	reverse_id_0 = chipid_reverse_value(exynos_soc_info.lot_id, 32);
	tmp = (reverse_id_0 >> 11) & 0x1FFFFF;
	chipid_dec_to_36(tmp, lot_id);

	info_size += snprintf(buf, DATA_SIZE, "\"HW_REV\":\"%d\",", sec_hw_rev);

	memset(val, 0, 32);
	get_bk_item_val_as_string("RSTCNT", val);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"RSTCNT\":\"%s\",", val);

	memset(val, 0, 32);
	get_bk_item_val_as_string("CHI", val);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"CHIPID_FAIL\":\"%s\",", val);

	memset(val, 0, 32);
	get_bk_item_val_as_string("LPI", val);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"LPI_TIMEOUT\":\"%s\",", val);

	memset(val, 0, 32);
	get_bk_item_val_as_string("CDI", val);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"CODE_DIFF\":\"%s\",", val);

	memset(val, 0, 32);
	get_bk_item_val_as_string("BIN", val);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"BIN\":\"%s\",", val);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"ASB\":\"%d\",", id_get_asb_ver());
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"PSITE\":\"%d\",", id_get_product_line());
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"LOT_ID\":\"%s\",", lot_id);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"CACHE_ERR\":\"%d\",", cache_err_cnt);
#if defined(CONFIG_SAMSUNG_VST_CAL)
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"VST_RESULT\":\"%d\",", vst_result);
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"VST_ADJUST\":\"%d\",", volt_vst_cal_bdata);
#endif
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"ASV_BIG\":\"%d\",", asv_ids_information(bg));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"ASV_MID\":\"%d\",", asv_ids_information(mg));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"ASV_LIT\":\"%d\",", asv_ids_information(lg));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"ASV_G3D\":\"%d\",", asv_ids_information(g3dg));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"ASV_MIF\":\"%d\",", asv_ids_information(mifg));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"IDS_BIG\":\"%d\",", asv_ids_information(bids));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"IDS_MID\":\"%d\",", asv_ids_information(mids));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"IDS_LIT\":\"%d\",", asv_ids_information(lids));
	info_size +=
		snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
				"\"IDS_G3D\":\"%d\",", asv_ids_information(gids));

	return info_size;
}

static ssize_t sec_hw_param_ddr_info_show(struct kobject *kobj,
					  struct kobj_attribute *attr,
					  char *buf)
{
	ssize_t info_size = 0;
	struct phy_trn_data_t *trn_data;
	unsigned long addr;
	unsigned int data;
	int i, j, k;
	unsigned short min_vwm, temp_vwm;
	void __iomem *dram_trn_addr;

	info_size +=
	    snprintf((char *)(buf), DATA_SIZE, "\"DDRV\":\"%s\",",
		     get_dram_manufacturer());
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"LPDDR4\":\"%dGB\",", lpddr4_size);

	dram_trn_addr = ioremap(MK_DDR_TRN_DATA_BASE, 0x4000);
	for (k = 0; k < NUM_OF_CH; k++) {
		data = readl(dram_trn_addr + 0xc60 + 4 * k);
		info_size +=
			snprintf(
				(char *)(buf + info_size),
				DATA_SIZE - info_size,
				"\"sc%dr0\":\"%x\",", k, (data >> 16) & 0xff);
		info_size +=
			snprintf(
				(char *)(buf + info_size),
				DATA_SIZE - info_size,
				"\"sc%dr1\":\"%x\",", k, data & 0xff);
	}
	for (k = 0; k < NUM_OF_CH; k++) {
		data = readl(dram_trn_addr + 0xc80 + 4 * k);
		info_size +=
			snprintf(
				(char *)(buf + info_size),
				DATA_SIZE - info_size,
				"\"dc%dr0\":\"%x\",", k, (data >> 16) & 0xff);
		info_size +=
			snprintf(
				(char *)(buf + info_size),
				DATA_SIZE - info_size,
				"\"dc%dr1\":\"%x\",", k, data & 0xff);
	}
	for (k = 0; k < NUM_OF_CH; k++) {
		addr = readl(dram_trn_addr + 0xc50 + 4 * k);

		if (addr > 0x2a000 && addr < 0x2c000) {
			trn_data =
				(struct phy_trn_data_t *)
				(dram_trn_addr + addr - 0x29000);

			for (j = 0; j < PHY_BYTE_ALL; j++) {
				for (i = 0; i < NUM_OF_TRN_RD_DESKEW_INFO; i++) {
					if (i == 0) {
						min_vwm = 2 *
							(trn_data->read[0].deskewc[i][j] -
							 trn_data->read[0].deskewl[i][j]);
					} else {
						temp_vwm = 2 *
							(trn_data->read[0].deskewc[i][j] -
							 trn_data->read[0].deskewl[i][j]);
						if (min_vwm > temp_vwm)
							min_vwm = temp_vwm;
					}
				}
				info_size +=
					snprintf(
						(char *)(buf + info_size),
						DATA_SIZE - info_size,
						"\"rc%db%d\":\"%x\",", k, j, min_vwm);
			}
			for (j = 0; j < PHY_BYTE_ALL; j++) {
				for (i = 0; i < NUM_OF_TRN_WR_DESKEW_INFO; i++) {
					if (i == 0) {
						min_vwm = 2 *
							(trn_data->write[0].deskewc[i][j] -
							 trn_data->write[0].deskewl[i][j]);
					} else {
						temp_vwm = 2 *
							(trn_data->write[0].deskewc[i][j] -
							 trn_data->write[0].deskewl[i][j]);
						if (min_vwm > temp_vwm)
							min_vwm = temp_vwm;
					}
				}
				info_size +=
					snprintf(
						(char *)(buf + info_size),
						DATA_SIZE - info_size,
						"\"wc%db%d\":\"%x\",", k, j, min_vwm);
			}
		}
	}
	iounmap(dram_trn_addr);

	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"C2D\":\"\",");
	info_size +=
	    snprintf((char *)(buf + info_size), DATA_SIZE - info_size,
		     "\"D2D\":\"\"");

	return info_size;
}

static ssize_t sec_hw_param_extra_info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	sec_debug_store_extra_info_A(buf);
	info_size = strlen(buf);

	return info_size;
}

static ssize_t sec_hw_param_extrb_info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	sec_debug_store_extra_info_B(buf);
	info_size = strlen(buf);

	return info_size;
}

static ssize_t sec_hw_param_extrc_info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	sec_debug_store_extra_info_C(buf);
	info_size = strlen(buf);

	return info_size;
}

static ssize_t sec_hw_param_extrm_info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	sec_debug_store_extra_info_M(buf);
	info_size = strlen(buf);

	return info_size;
}

static ssize_t sec_hw_param_extrf_info_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	ssize_t info_size = 0;

	sec_debug_store_extra_info_F(buf);
	info_size = strlen(buf);

	return info_size;
}

static ssize_t sec_hw_param_pcb_info_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	unsigned char barcode[6] = {0,};
	int ret = -1;

	strncpy(barcode, buf, 5);

	ret = sec_set_param_str(pcb_offset, barcode, 5);
	if (ret < 0)
		pr_err("%s : Set Param fail. offset (%lu), data (%s)", __func__, pcb_offset, barcode);

	return count;
}

static ssize_t sec_hw_param_smd_info_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t count)
{
	unsigned char smd_date[9] = {0,};
	int ret = -1;

	strncpy(smd_date, buf, 8);

	ret = sec_set_param_str(smd_offset, smd_date, 8);
	if (ret < 0)
		pr_err("%s : Set Param fail. offset (%lu), data (%s)", __func__, smd_offset, smd_date);

	return count;
}

static ssize_t sec_hw_param_thermal_info_show(struct kobject *kobj,
					      struct kobj_attribute *attr,
					      char *buf)
{
	ssize_t info_size = 0;

	return info_size;
}

static struct kobj_attribute sec_hw_param_ap_info_attr =
	__ATTR(ap_info, 0440, sec_hw_param_ap_info_show, NULL);

static struct kobj_attribute sec_hw_param_ddr_info_attr =
	__ATTR(ddr_info, 0440, sec_hw_param_ddr_info_show, NULL);

static struct kobj_attribute sec_hw_param_extra_info_attr =
	__ATTR(extra_info, 0440, sec_hw_param_extra_info_show, NULL);

static struct kobj_attribute sec_hw_param_extrb_info_attr =
	__ATTR(extrb_info, 0440, sec_hw_param_extrb_info_show, NULL);

static struct kobj_attribute sec_hw_param_extrc_info_attr =
	__ATTR(extrc_info, 0440, sec_hw_param_extrc_info_show, NULL);

static struct kobj_attribute sec_hw_param_extrm_info_attr =
	__ATTR(extrm_info, 0440, sec_hw_param_extrm_info_show, NULL);

static struct kobj_attribute sec_hw_param_extrf_info_attr =
	__ATTR(extrf_info, 0440, sec_hw_param_extrf_info_show, NULL);

static struct kobj_attribute sec_hw_param_pcb_info_attr =
	__ATTR(pcb_info, 0660, NULL, sec_hw_param_pcb_info_store);

static struct kobj_attribute sec_hw_param_smd_info_attr =
	__ATTR(smd_info, 0660, NULL, sec_hw_param_smd_info_store);

static struct kobj_attribute sec_hw_param_thermal_info_attr =
	__ATTR(thermal_info, 0440, sec_hw_param_thermal_info_show, NULL);

static struct attribute *sec_hw_param_attributes[] = {
	&sec_hw_param_ap_info_attr.attr,
	&sec_hw_param_ddr_info_attr.attr,
	&sec_hw_param_extra_info_attr.attr,
	&sec_hw_param_extrb_info_attr.attr,
	&sec_hw_param_extrc_info_attr.attr,
	&sec_hw_param_extrm_info_attr.attr,
	&sec_hw_param_extrf_info_attr.attr,
	&sec_hw_param_pcb_info_attr.attr,
	&sec_hw_param_smd_info_attr.attr,
	&sec_hw_param_thermal_info_attr.attr,
	NULL,
};

static struct attribute_group sec_hw_param_attr_group = {
	.attrs = sec_hw_param_attributes,
};

static int __init sec_hw_param_init(void)
{
	int ret = 0;
	struct device *dev;

	dev = sec_device_create(NULL, "sec_hw_param");

	ret = sysfs_create_group(&dev->kobj, &sec_hw_param_attr_group);
	if (ret)
		pr_err("%s : could not create sysfs noden", __func__);

	return 0;
}

device_initcall(sec_hw_param_init);
