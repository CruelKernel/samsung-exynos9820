#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/namei.h>
#include <linux/security.h>

#include "regs/iva_base_addr.h"
#include "iva_ram_dump.h"
#include "iva_mbox.h"
#include "iva_mcu.h"
#include "iva_mem.h"
#include "iva_pmu.h"

#define CMSG_REQ_FATAL_HEADER		(0x1)
#define MAX_DUMP_PATH_SIZE		(256)

#define DFT_RAMDUMP_OUT_DIR		"/data/media/0/DCIM/"
#define DFT_RAMDUMP_OUTPUT(f)		(DFT_RAMDUMP_OUT_DIR "/" f)

#define RAMDUMP_DATE_FORMAT		"YYYYMMDD.HHMMSS"
#define RAMDUMP_FILENAME_PREFIX		"iva_ramdump"
#define RAMDUMP_FILENAME_EXT		"bin"

#define VCM_DBG_SEL_SAVE_OFFSET		(IVA_MBOX_BASE_ADDR)
#define VCM_DBG_SEL_SAVE_MAX_SIZE	(IVA_HWA_ADDR_GAP)

struct iva_mmr_skip_range_info {
	uint32_t	from;
	uint32_t	to;
};

typedef int32_t (*prepare_fn)(struct iva_dev_data *iva);

struct iva_mmr_sect_info {
	const char	*name;
	uint32_t	base;
	uint32_t	size;
	const uint32_t	skip_num;
	const struct iva_mmr_skip_range_info *skip_info;
	prepare_fn	pre_func;
};

static const struct iva_mmr_skip_range_info pmu_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x000c,
		.to	= 0x000c,
	},
	{
		.from	= 0x0034,
		.to	= 0x0034,
	},
	{
		.from	= 0x003c,
		.to	= 0x003c,
	},
	{
		.from	= 0x0054,
		.to	= 0x005c,
	},
	{
		.from	= 0x006c,
		.to	= 0xffdc,
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static const  struct iva_mmr_sect_info iva_sfr_pmu = {
	.name		= "pmu",
	.base		= IVA_PMU_BASE_ADDR,
	.size		= IVA_HWA_ADDR_GAP,
	.skip_info	= pmu_skip_range,
	.skip_num	= ARRAY_SIZE(pmu_skip_range),
};

static const  struct iva_mmr_sect_info iva_sfr_mcu = {
	.name = "mcu",
	.base = IVA_VMCU_MEM_BASE_ADDR,
#if defined(CONFIG_SOC_EXYNOS9820)
	.size = VMCU_MEM_SIZE,
#else
	.size = VMCU_MEM_SIZE,
#endif
};

#if defined(CONFIG_SOC_EXYNOS9820)
static const  struct iva_mmr_sect_info iva_sfr_mcu_extend = {
	.name = "mcu",
	.base = IVA_VMCU_MEM_BASE_ADDR,
	.size = EXTEND_VMCU_MEM_SIZE,
};
#endif

static const struct iva_mmr_skip_range_info vcm_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x000c,
		.to	= 0x000c,
	},
	{
		.from	= 0x0030,
		.to	= 0x003c,
	},
	{
		.from	= 0x00d4,
		.to	= 0xffdc,
	},
#else
	{	/* handled in vcm_sbuf section */
		.from	= IVA_VCM_SCH_BUF_OFFSET,
		.to	= IVA_VCM_SCH_BUF_OFFSET + IVA_VCM_SCH_BUF_SIZE - 1
	}, {	/* handled in vcm_cbuf section */
		.from	= IVA_VCM_CMD_BUF_OFFSET,
		.to	= IVA_VCM_CMD_BUF_OFFSET + IVA_VCM_CMD_BUF_SIZE - 1
	},
#endif

#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static int32_t iva_ram_dump_prepare_vcm_buf(struct iva_dev_data *iva)
{
	iva_pmu_reset_hwa(iva, pmu_vcm_id, false);
	iva_pmu_reset_hwa(iva, pmu_vcm_id, true);
	return 0;
}

static const struct iva_mmr_skip_range_info csc_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c,
	},
	{
		.from	= 0x0028,
		.to	= 0xeffc,
	},
	{
		.from	= 0xf018,
		.to	= 0xf01c,
	},
	{
		.from	= 0xf030,
		.to	= 0xf030,
	},
	{
		.from	= 0xf03c,
		.to	= 0xf03c,
	},
	{
		.from	= 0xf048,
		.to	= 0xf050,
	},
	{
		.from	= 0xf058,
		.to	= 0xf060,
	},
	{
		.from	= 0xf068,
		.to	= 0xf1fc,
	},
	{
		.from	= 0xf218,
		.to	= 0xf21c,
	},
	{
		.from	= 0xf22c,
		.to	= 0xffec,
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info scl_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c,
	},
	{
		.from	= 0x0040,
		.to	= 0x00fc,
	},
	{
		.from	= 0x0010c,
		.to	= 0x0010c,
	},
	{
		.from	= 0x0011c,
		.to	= 0x0011c,
	},
	{
		.from	= 0x0012c,
		.to	= 0x0012c,
	},
	{
		.from	= 0x0013c,
		.to	= 0x0013c,
	},
	{
		.from	= 0x0014c,
		.to	= 0x0014c,
	},
	{
		.from	= 0x0015c,
		.to	= 0x0015c,
	},
	{
		.from	= 0x0016c,
		.to	= 0x0016c,
	},
	{
		.from	= 0x0017c,
		.to	= 0x0017c,
	},
	{
		.from	= 0x0018c,
		.to	= 0x0018c,
	},
	{
		.from	= 0x0019c,
		.to	= 0x0019c,
	},
	{
		.from	= 0x001ac,
		.to	= 0x001ac,
	},
	{
		.from	= 0x001bc,
		.to	= 0x001bc,
	},
	{
		.from	= 0x001cc,
		.to	= 0x001cc,
	},
	{
		.from	= 0x001dc,
		.to	= 0x001dc,
	},
	{
		.from	= 0x001ec,
		.to	= 0x001ec,
	},
	{
		.from	= 0x001fc,
		.to	= 0x001fc,
	},
	{
		.from	= 0x0020c,
		.to	= 0x0020c,
	},
	{
		.from	= 0x0021c,
		.to	= 0x0021c,
	},
	{
		.from	= 0x0022c,
		.to	= 0x0022c,
	},
	{
		.from	= 0x0023c,
		.to	= 0x0023c,
	},
	{
		.from	= 0x0024c,
		.to	= 0x0024c,
	},
	{
		.from	= 0x0025c,
		.to	= 0x0025c,
	},
	{
		.from	= 0x0026c,
		.to	= 0x0026c,
	},
	{
		.from	= 0x0027c,
		.to	= 0x0027c,
	},
	{
		.from	= 0x0028c,
		.to	= 0x0028c,
	},
	{
		.from	= 0x0029c,
		.to	= 0x0029c,
	},
	{
		.from	= 0x002ac,
		.to	= 0x002ac,
	},
	{
		.from	= 0x002bc,
		.to	= 0x002bc,
	},
	{
		.from	= 0x002cc,
		.to	= 0x002cc,
	},
	{
		.from	= 0x002dc,
		.to	= 0x002dc,
	},
	{
		.from	= 0x002ec,
		.to	= 0x002ec,
	},
	{
		.from	= 0x002fc,
		.to	= 0x0effc,
	},
	{
		.from	= 0x0f018,
		.to	= 0x0f030,
	},
	{
		.from	= 0x0f03c,
		.to	= 0x0f1fc,
	},
	{
		.from	= 0x0f218,
		.to	= 0x0f224,
	},
	{
		.from	= 0x0f22c,
		.to	= 0x0ffcc,
	},
	{
		.from	= 0x0ffd4,
		.to	= 0x0ffdc,
	},
	{
		.from	= 0x0fff8,
		.to	= 0x0fffc,
	},
#endif
};

static const struct iva_mmr_skip_range_info orb_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9810)
	{ .from = 0xfff8, .to = 0xfffc }, /* last 4 bytes */
#elif defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c,
	},
	{
		.from	= 0x002c,
		.to	= 0x002c,
	},
	{
		.from	= 0x003c,
		.to	= 0x003c,
	},
	{
		.from	= 0x0064,
		.to	= 0xeffc,
	},
	{
		.from	= 0xf018,
		.to	= 0xf01c,
	},
	{
		.from	= 0xf024,
		.to	= 0xf030,
	},
	{
		.from	= 0xf038,
		.to	= 0xf03c,
	},
	{
		.from	= 0xf058,
		.to	= 0xf05c,
	},
	{
		.from	= 0xf064,
		.to	= 0xf070,
	},
	{
		.from	= 0xf078,
		.to	= 0xf07c,
	},
	{
		.from	= 0xf098,
		.to	= 0xf09c,
	},
	{
		.from	= 0xf0a4,
		.to	= 0xf0b0,
	},
	{
		.from	= 0xf0b4,
		.to	= 0xf0bc,
	},
	{
		.from	= 0xf0d8,
		.to	= 0xf0dc,
	},
	{
		.from	= 0xf0e4,
		.to	= 0xf0f0,
	},
	{
		.from	= 0xf0f8,
		.to	= 0xf1fc,
	},
	{
		.from	= 0xf218,
		.to	= 0xf21c,
	},
	{
		.from	= 0xf224,
		.to	= 0xf224,
	},
	{
		.from	= 0xf22c,
		.to	= 0xf23c,
	},
	{
		.from	= 0xf258,
		.to	= 0xf25c,
	},
	{
		.from	= 0xf264,
		.to	= 0xf264,
	},
	{
		.from	= 0xf26c,
		.to	= 0xffcf,
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc,
	},
	{
		.from	= 0xfff8,
		.to	= 0xfffc,
	},
#endif
};


static const struct iva_mmr_skip_range_info mch_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c,
	},
	{
		.from	= 0x0034,
		.to	= 0xeffc,
	},
	{
		.from	= 0xf018,
		.to	= 0xf01c,
	},
	{
		.from	= 0xf028,
		.to	= 0xf03c,
	},
	{
		.from	= 0xf058,
		.to	= 0xf05c,
	},
	{
		.from	= 0xf068,
		.to	= 0xf1fc,
	},
	{
		.from	= 0xf218,
		.to	= 0xf21c,
	},
	{
		.from	= 0xf224,
		.to	= 0xf224,
	},
	{
		.from	= 0xf22c,
		.to	= 0xffcc,
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc,
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info lmd_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9810)
	{ .from = 0xfffc, .to = 0xfffc }, /* last 4 bytes */
#elif defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x002c,
		.to	= 0xeffc,
	},
	{
		.from	= 0xf018,
		.to	= 0xf01c
	},
	{
		.from	= 0xf030,
		.to	= 0xf030
	},
	{
		.from	= 0xf03c,
		.to	= 0xf1fc
	},
	{
		.from	= 0xf218,
		.to	= 0xf21c
	},
	{
		.from	= 0xf224,
		.to	= 0xf224
	},
	{
		.from	= 0xf22c,
		.to	= 0xffec
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info lkt_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c
	},
	{
		.from	= 0x0020,
		.to	= 0x002c
	},
	{
		.from	= 0x0050,
		.to	= 0xf004
	},
	{
		.from	= 0xf010,
		.to	= 0xf01c
	},
	{
		.from	= 0xf024,
		.to	= 0xf030
	},
	{
		.from	= 0xf03c,
		.to	= 0xf03c
	},
	{
		.from	= 0xf058,
		.to	= 0xf05c
	},
	{
		.from	= 0xf064,
		.to	= 0xf084
	},
	{
		.from	= 0xf090,
		.to	= 0xf09c
	},
	{
		.from	= 0xf0a4,
		.to	= 0xf1fc
	},
	{
		.from	= 0xf218,
		.to	= 0xf224
	},
	{
		.from	= 0xf22c,
		.to	= 0xffcc
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info wig_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x000c,
		.to	= 0x000c
	},
	{
		.from	= 0x0024,
		.to	= 0x0024
	},
	{
		.from	= 0x002c,
		.to	= 0x002c
	},
	{
		.from	= 0x0074,
		.to	= 0xeffc
	},
	{
		.from	= 0xf018,
		.to	= 0xf01c
	},
	{
		.from	= 0xf028,
		.to	= 0xf1fc
	},
	{
		.from	= 0xf220,
		.to	= 0xf7fc
	},
	{
		.from	= 0xf810,
		.to	= 0xffcc
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};


static const struct iva_mmr_skip_range_info enf_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c
	},
	{
		.from	= 0x0020,
		.to	= 0x00fc
	},
	{
		.from	= 0x0114,
		.to	= 0x011c
	},
	{
		.from	= 0x0194,
		.to	= 0xeffc
	},
	{
		.from	= 0xf018,
		.to	= 0xf030
	},
	{
		.from	= 0xf03c,
		.to	= 0xf0fc
	},
	{
		.from	= 0xf118,
		.to	= 0xf130
	},
	{
		.from	= 0xf13c,
		.to	= 0xf4fc
	},
	{
		.from	= 0xf518,
		.to	= 0xf524
	},
	{
		.from	= 0xf52c,
		.to	= 0xffec
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};


#define CH0_BASE (0x200)
#define GET_CH_BASE(ch) (CH0_BASE + (ch) * 0x200)
static const struct iva_mmr_skip_range_info vdma_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x000c,
		.to	= 0x000c
	},
	{
		.from	= 0x0098,
		.to	= 0x009c
	},
	{
		.from	= 0x00b0,
		.to	= 0x01fc
	},
	/* 0~4 RO */
	{
		.from	= GET_CH_BASE(0) + 0x0008,
		.to	= GET_CH_BASE(0) + 0x000c
	},
	{
		.from	= GET_CH_BASE(0) + 0x0024,
		.to	= GET_CH_BASE(0) + 0x002c
	},
	{
		.from	= GET_CH_BASE(0) + 0x0040,
		.to	= GET_CH_BASE(0) + 0x0040
	},
	{
		.from	= GET_CH_BASE(0) + 0x0048,
		.to	= GET_CH_BASE(0) + 0x004c
	},
	{
		.from	= GET_CH_BASE(0) + 0x0068,
		.to	= GET_CH_BASE(0) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(1) + 0x0008,
		.to	= GET_CH_BASE(1) + 0x000c
	},
	{
		.from	= GET_CH_BASE(1) + 0x0024,
		.to	= GET_CH_BASE(1) + 0x002c
	},
	{
		.from	= GET_CH_BASE(1) + 0x0040,
		.to	= GET_CH_BASE(1) + 0x0040
	},
	{
		.from	= GET_CH_BASE(1) + 0x0048,
		.to	= GET_CH_BASE(1) + 0x004c
	},
	{
		.from	= GET_CH_BASE(1) + 0x0068,
		.to	= GET_CH_BASE(1) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(2) + 0x0008,
		.to	= GET_CH_BASE(2) + 0x000c
	},
	{
		.from	= GET_CH_BASE(2) + 0x0024,
		.to	= GET_CH_BASE(2) + 0x002c
	},
	{
		.from	= GET_CH_BASE(2) + 0x0040,
		.to	= GET_CH_BASE(2) + 0x0040
	},
	{
		.from	= GET_CH_BASE(2) + 0x0048,
		.to	= GET_CH_BASE(2) + 0x004c
	},
	{
		.from	= GET_CH_BASE(2) + 0x0068,
		.to	= GET_CH_BASE(2) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(3) + 0x0008,
		.to	= GET_CH_BASE(3) + 0x000c
	},
	{
		.from	= GET_CH_BASE(3) + 0x0024,
		.to	= GET_CH_BASE(3) + 0x002c
	},
	{
		.from	= GET_CH_BASE(3) + 0x0040,
		.to	= GET_CH_BASE(3) + 0x0040
	},
	{
		.from	= GET_CH_BASE(3) + 0x0048,
		.to	= GET_CH_BASE(3) + 0x004c
	},
	{
		.from	= GET_CH_BASE(3) + 0x0068,
		.to	= GET_CH_BASE(3) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(4) + 0x0008,
		.to	= GET_CH_BASE(4) + 0x000c
	},
	{
		.from	= GET_CH_BASE(4) + 0x0024,
		.to	= GET_CH_BASE(4) + 0x002c
	},
	{
		.from	= GET_CH_BASE(4) + 0x0040,
		.to	= GET_CH_BASE(4) + 0x0040
	},
	{
		.from	= GET_CH_BASE(4) + 0x0048,
		.to	= GET_CH_BASE(4) + 0x004c
	},
	{
		.from	= GET_CH_BASE(4) + 0x0068,
		.to	= GET_CH_BASE(4) + 0x01fc
	},
	{	/* RO_SPL: 0xc00 */
		.from	= GET_CH_BASE(5) + 0x0008,
		.to	= GET_CH_BASE(5) + 0x000c
	},
	{
		.from	= GET_CH_BASE(5) + 0x0024,
		.to	= GET_CH_BASE(5) + 0x002c
	},
	{
		.from	= GET_CH_BASE(5) + 0x0040,
		.to	= GET_CH_BASE(5) + 0x0040
	},
	{
		.from	= GET_CH_BASE(5) + 0x0048,
		.to	= GET_CH_BASE(5) + 0x004c
	},
	{
		.from	= GET_CH_BASE(5) + 0x0068,
		.to	= GET_CH_BASE(5) + 0x008c
	},
	{
		.from	= GET_CH_BASE(5) + 0x00a0,
		.to	= GET_CH_BASE(5) + 0x01fc
	},
	{	/* RO_YUV0: 0xE00 */
		.from	= GET_CH_BASE(6) + 0x0008,
		.to	= GET_CH_BASE(6) + 0x000c
	},
	{
		.from	= GET_CH_BASE(6) + 0x0024,
		.to	= GET_CH_BASE(6) + 0x0024
	},
	{
		.from	= GET_CH_BASE(6) + 0x0048,
		.to	= GET_CH_BASE(6) + 0x004c
	},
	{
		.from	= GET_CH_BASE(6) + 0x0068,
		.to	= GET_CH_BASE(6) + 0x0074
	},
	{
		.from	= GET_CH_BASE(6) + 0x0080,
		.to	= GET_CH_BASE(6) + 0x0080
	},
	{
		.from	= GET_CH_BASE(6) + 0x0090,
		.to	= GET_CH_BASE(6) + 0x01fc
	},
	{	/* RO_YUV1: 0x1000 */
		.from	= GET_CH_BASE(7) + 0x0008,
		.to	= GET_CH_BASE(7) + 0x000c
	},
	{
		.from	= GET_CH_BASE(7) + 0x0024,
		.to	= GET_CH_BASE(7) + 0x0024
	},
	{
		.from	= GET_CH_BASE(7) + 0x0048,
		.to	= GET_CH_BASE(7) + 0x004c
	},
	{
		.from	= GET_CH_BASE(7) + 0x0068,
		.to	= GET_CH_BASE(7) + 0x0074
	},
	{
		.from	= GET_CH_BASE(7) + 0x0080,
		.to	= GET_CH_BASE(7) + 0x0080
	},
	{
		.from	= GET_CH_BASE(7) + 0x0090,
		.to	= GET_CH_BASE(7) + 0x01fc
	},
	/* 8~11 RO */
	{
		.from	= GET_CH_BASE(8) + 0x0008,
		.to	= GET_CH_BASE(8) + 0x000c
	},
	{
		.from	= GET_CH_BASE(8) + 0x0014,
		.to	= GET_CH_BASE(8) + 0x001c
	},
	{
		.from	= GET_CH_BASE(8) + 0x0040,
		.to	= GET_CH_BASE(8) + 0x0040
	},
	{
		.from	= GET_CH_BASE(8) + 0x0048,
		.to	= GET_CH_BASE(8) + 0x006c
	},
	{
		.from	= GET_CH_BASE(8) + 0x0088,
		.to	= GET_CH_BASE(8) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(8) + 0x00b8,
		.to	= GET_CH_BASE(8) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(9) + 0x0008,
		.to	= GET_CH_BASE(9) + 0x000c
	},
	{
		.from	= GET_CH_BASE(9) + 0x0014,
		.to	= GET_CH_BASE(9) + 0x001c
	},
	{
		.from	= GET_CH_BASE(9) + 0x0040,
		.to	= GET_CH_BASE(9) + 0x0040
	},
	{
		.from	= GET_CH_BASE(9) + 0x0048,
		.to	= GET_CH_BASE(9) + 0x006c
	},
	{
		.from	= GET_CH_BASE(9) + 0x0088,
		.to	= GET_CH_BASE(9) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(9) + 0x00b8,
		.to	= GET_CH_BASE(9) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(10) + 0x0008,
		.to	= GET_CH_BASE(10) + 0x000c
	},
	{
		.from	= GET_CH_BASE(10) + 0x0014,
		.to	= GET_CH_BASE(10) + 0x001c
	},
	{
		.from	= GET_CH_BASE(10) + 0x0040,
		.to	= GET_CH_BASE(10) + 0x0040
	},
	{
		.from	= GET_CH_BASE(10) + 0x0048,
		.to	= GET_CH_BASE(10) + 0x006c
	},
	{
		.from	= GET_CH_BASE(10) + 0x0088,
		.to	= GET_CH_BASE(10) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(10) + 0x00b8,
		.to	= GET_CH_BASE(10) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(11) + 0x0008,
		.to	= GET_CH_BASE(11) + 0x000c
	},
	{
		.from	= GET_CH_BASE(11) + 0x0014,
		.to	= GET_CH_BASE(11) + 0x001c
	},
	{
		.from	= GET_CH_BASE(11) + 0x0040,
		.to	= GET_CH_BASE(11) + 0x0040
	},
	{
		.from	= GET_CH_BASE(11) + 0x0048,
		.to	= GET_CH_BASE(11) + 0x006c
	},
	{
		.from	= GET_CH_BASE(11) + 0x0088,
		.to	= GET_CH_BASE(11) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(11) + 0x00b8,
		.to	= GET_CH_BASE(11) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(12) + 0x0008,
		.to	= GET_CH_BASE(12) + 0x000c
	},
	{
		.from	= GET_CH_BASE(12) + 0x0014,
		.to	= GET_CH_BASE(12) + 0x001c
	},
	{
		.from	= GET_CH_BASE(12) + 0x0040,
		.to	= GET_CH_BASE(12) + 0x0040
	},
	{
		.from	= GET_CH_BASE(12) + 0x0048,
		.to	= GET_CH_BASE(12) + 0x006c
	},
	{
		.from	= GET_CH_BASE(12) + 0x0088,
		.to	= GET_CH_BASE(12) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(12) + 0x00b8,
		.to	= GET_CH_BASE(12) + 0x01fc
	},
	{
		.from	= GET_CH_BASE(13) + 0x0008,
		.to	= GET_CH_BASE(13) + 0x000c
	},
	{
		.from	= GET_CH_BASE(13) + 0x0014,
		.to	= GET_CH_BASE(13) + 0x001c
	},
	{
		.from	= GET_CH_BASE(13) + 0x0040,
		.to	= GET_CH_BASE(13) + 0x0040
	},
	{
		.from	= GET_CH_BASE(13) + 0x0048,
		.to	= GET_CH_BASE(13) + 0x006c
	},
	{
		.from	= GET_CH_BASE(13) + 0x0088,
		.to	= GET_CH_BASE(13) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(13) + 0x00b8,
		.to	= GET_CH_BASE(13) + 0x01fc
	},
	{	/* WO_YUV0: 0x1E00 */
		.from	= GET_CH_BASE(14) + 0x0008,
		.to	= GET_CH_BASE(14) + 0x000c
	},
	{
		.from	= GET_CH_BASE(14) + 0x0014,
		.to	= GET_CH_BASE(14) + 0x0014
	},
	{
		.from	= GET_CH_BASE(14) + 0x0048,
		.to	= GET_CH_BASE(14) + 0x006c
	},
	{
		.from	= GET_CH_BASE(14) + 0x0088,
		.to	= GET_CH_BASE(14) + 0x0094
	},
	{
		.from	= GET_CH_BASE(14) + 0x00a0,
		.to	= GET_CH_BASE(14) + 0x00a0
	},
	{
		.from	= GET_CH_BASE(14) + 0x00ac,
		.to	= GET_CH_BASE(14) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(14) + 0x00b8,
		.to	= GET_CH_BASE(14) + 0x01fc
	},
	{	/* WO_YUV1: 0x2000 */
		.from	= GET_CH_BASE(15) + 0x0008,
		.to	= GET_CH_BASE(15) + 0x000c
	},
	{
		.from	= GET_CH_BASE(15) + 0x0014,
		.to	= GET_CH_BASE(15) + 0x0014
	},
	{
		.from	= GET_CH_BASE(15) + 0x0048,
		.to	= GET_CH_BASE(15) + 0x006c
	},
	{
		.from	= GET_CH_BASE(15) + 0x0088,
		.to	= GET_CH_BASE(15) + 0x0094
	},
	{
		.from	= GET_CH_BASE(15) + 0x00a0,
		.to	= GET_CH_BASE(15) + 0x00a0
	},
	{
		.from	= GET_CH_BASE(15) + 0x00ac,
		.to	= GET_CH_BASE(15) + 0x00ac
	},
	{
		.from	= GET_CH_BASE(15) + 0x00b8,
		.to	= GET_CH_BASE(15) + 0x01fc
	},
	{
		.from	= 0x2200,
		.to	= 0xffdc
	},
	{
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};


static const struct iva_mmr_skip_range_info mot_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c
	},
	{
		.from	= 0x0020,
		.to	= 0x00fc
	},
	{
		.from	= 0x0160,
		.to	= 0xf1fc
	},
	{
		.from	= 0xf218,
		.to	= 0xf230
	},
	{
		.from	= 0xf23c,
		.to	= 0xf2fc
	},
	{
		.from	= 0xf318,
		.to	= 0xf330
	},
	{
		.from	= 0xf33c,
		.to	= 0xffec
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc
	},
	{
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info bax_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0000,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info bld_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0008,
		.to	= 0x000c
	},
	{
		.from	= 0x0020,
		.to	= 0x00fc
	},
	{
		.from	= 0x010c,
		.to	= 0x010c
	},
	{
		.from	= 0x011c,
		.to	= 0x011c
	},
	{
		.from	= 0x019c,
		.to	= 0xeffc
	},
	{
		.from	= 0xf018,
		.to	= 0xf03c
	},
	{
		.from	= 0xf058,
		.to	= 0xf07c
	},
	{
		.from	= 0xf098,
		.to	= 0xf0bc
	},
	{
		.from	= 0xf0d8,
		.to	= 0xf0fc
	},
	{
		.from	= 0xf118,
		.to	= 0xf1fc
	},
	{
		.from	= 0xf218,
		.to	= 0xf23c
	},
	{
		.from	= 0xf258,
		.to	= 0xf27c
	},
	{
		.from	= 0xf298,
		.to	= 0xf2bc
	},
	{
		.from	= 0xf2d8,
		.to	= 0xf2fc
	},
	{
		.from	= 0xf318,
		.to	= 0xf33c
	},
	{
		.from	= 0xf358,
		.to	= 0xf37c
	},
	{
		.from	= 0xf398,
		.to	= 0xf3bc
	},
	{
		.from	= 0xf3d8,
		.to	= 0xf3fc
	},
	{
		.from	= 0xf418,
		.to	= 0xf43c
	},
	{
		.from	= 0xf458,
		.to	= 0xf47c
	},
	{
		.from	= 0xf498,
		.to	= 0xf4fc
	},
	{
		.from	= 0xf518,
		.to	= 0xf524
	},
	{
		.from	= 0xf52c,
		.to	= 0xf53c
	},
	{
		.from	= 0xf558,
		.to	= 0xf564
	},
	{
		.from	= 0xf56c,
		.to	= 0xf57c
	},
	{
		.from	= 0xf598,
		.to	= 0xf5a4
	},
	{
		.from	= 0xf5ac,
		.to	= 0xf5bc
	},
	{
		.from	= 0xf5d8,
		.to	= 0xf5e4
	},
	{
		.from	= 0xf5ec,
		.to	= 0xffec
	},
	{
		.from	= 0xffd4,
		.to	= 0xffdc
	},
	{	/* last 8 bytes */
		.from	= 0xfff8,
		.to	= 0xfffc
	},
#endif
};


static const struct iva_mmr_skip_range_info dif_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0000,
		.to	= 0xfffc
	},
#endif
};

static const struct iva_mmr_skip_range_info vcm_cbuf_skip_range[] = {
#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.from	= 0x0400,
		.to	= 0x0ffc
	},
	{
		.from	= 0x1400,
		.to	= 0x1ffc
	},
	{
		.from	= 0x2400,
		.to	= 0x2ffc
	},
	{
		.from	= 0x3400,
		.to	= 0x3ffc
	},
	{
		.from	= 0x4400,
		.to	= 0x4ffc
	},
	{
		.from	= 0x5400,
		.to	= 0x5ffc
	},
	{
		.from	= 0x6400,
		.to	= 0x6ffc
	},
	{
		.from	= 0x7400,
		.to	= 0x7ffc
	},
#endif
};

static const struct iva_mmr_sect_info iva_sfr_sects[] = {
	/*
	 * causion: do not add mbox if there is no specific reason.
	 */
	{
		.name = "vcf",				//ok
		.base = IVA_VCF_BASE_ADDR,
		.size = VCF_MEM_SIZE,
	}, {
		.name = "vcm",
		.base = IVA_VCM_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = vcm_skip_range,
		.skip_num = (uint32_t) ARRAY_SIZE(vcm_skip_range),
	}, {
		/* sub range in vcm section : sync with vcm_skip_range */
		.name = "vcm_sbuf",
		.base = IVA_VCM_BASE_ADDR + IVA_VCM_SCH_BUF_OFFSET,
		.size = IVA_VCM_SCH_BUF_SIZE,
		.pre_func = iva_ram_dump_prepare_vcm_buf,
	}, {
		/*
		 * sub range in vcm section : sync with vcm_skip_range
		 * always behind vcm_sbuf.
		 */
		.name = "vcm_cbuf",
		.base = IVA_VCM_BASE_ADDR + IVA_VCM_CMD_BUF_OFFSET,
		.size = IVA_VCM_CMD_BUF_SIZE,
		.skip_info = vcm_cbuf_skip_range,
		.skip_num = ARRAY_SIZE(vcm_cbuf_skip_range),
	}, {
		.name = "csc",				//ok
		.base = IVA_CSC_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = csc_skip_range,
		.skip_num = ARRAY_SIZE(csc_skip_range),
	}, {
		.name = "scl0",				//ok
		.base = IVA_SCL0_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = scl_skip_range,
		.skip_num = ARRAY_SIZE(scl_skip_range),
	}, {
		.name = "scl1",				//ok
		.base = IVA_SCL1_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = scl_skip_range,
		.skip_num = ARRAY_SIZE(scl_skip_range),
	}, {
		.name = "orb",				//ok
		.base = IVA_ORB_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
		.skip_info = orb_skip_range,
		.skip_num = (uint32_t) ARRAY_SIZE(orb_skip_range),
	#endif
	}, {
		.name = "mch",				//ok
		.base = IVA_MCH_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = mch_skip_range,
		.skip_num = ARRAY_SIZE(mch_skip_range),
	}, {
		.name = "lmd",
		.base = IVA_LMD_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
		.skip_info = lmd_skip_range,
		.skip_num = (uint32_t) ARRAY_SIZE(lmd_skip_range),
	#endif
	}, {
		.name = "lkt",
		.base = IVA_LKT_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = lkt_skip_range,
		.skip_num = ARRAY_SIZE(lkt_skip_range),
	}, {
		.name = "wig0",
		.base = IVA_WIG0_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = wig_skip_range,
		.skip_num = ARRAY_SIZE(wig_skip_range),
	}, {
		.name = "wig1",
		.base = IVA_WIG1_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = wig_skip_range,
		.skip_num = ARRAY_SIZE(wig_skip_range),
	}, {
		.name = "enf",
		.base = IVA_ENF_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = enf_skip_range,
		.skip_num = ARRAY_SIZE(enf_skip_range),
	}, {
#if defined(CONFIG_SOC_EXYNOS8895)
		.name = "vdma0",
#elif defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
		.name = "vdma",
#endif
		.base = IVA_VDMA0_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = vdma_skip_range,
		.skip_num = ARRAY_SIZE(vdma_skip_range),
	},
#if defined(CONFIG_SOC_EXYNOS8895)
	{
		.name = "vdma1",
		.base = IVA_VDMA1_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}
#elif defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
	{
		.name = "bax",
		.base = IVA_BAX_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = bax_skip_range,
		.skip_num = ARRAY_SIZE(bax_skip_range),
	}, {
		.name = "mot",
		.base = IVA_MOT_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = mot_skip_range,
		.skip_num = ARRAY_SIZE(mot_skip_range)
	}, {
		.name = "bld",
		.base = IVA_BLD_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = bld_skip_range,
		.skip_num = ARRAY_SIZE(bld_skip_range),
#if 0
	}, {
		.name = "dif",
		.base = IVA_DIF_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = dif_skip_range,
		.skip_num = ARRAY_SIZE(dif_skip_range),
#endif
	}, {
		.name = "wig2",
		.base = IVA_WIG2_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
		.skip_info = wig_skip_range,
		.skip_num = ARRAY_SIZE(wig_skip_range),
	},
#if defined(CONFIG_SOC_EXYNOS9810)
	{
		.name = "wig3",
		.base = IVA_WIG3_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}
#endif

#if defined(CONFIG_SOC_EXYNOS9820)
	{
		.name = "mcu_dtcm",
		.base = IVA_VMCU_DTCM_BASE_ADDR,
		.size = IVA_HWA_ADDR_GAP,
	}
#endif
#endif
};

static void iva_ram_dump_copy_section_sfrs(struct iva_dev_data *iva,
		u32 *dst_sect_addr, const struct iva_mmr_sect_info *sect_info)
{
	u32 __iomem	*src_sfr_base;
	uint32_t	i, skip_idx;
	u32		size_u32 = (u32) sizeof(u32);
	u32		align_mask = (u32)(~(size_u32 - 1));
	u32		aligned_from, aligned_to;

	if (!sect_info) {
		dev_warn(iva->dev, "%s() null sect_info.\n", __func__);
		return;
	}

	if (!dst_sect_addr) {
		dev_warn(iva->dev, "%s() %s: null dst_sect_addr.\n", __func__,
				sect_info->name);
		return;
	}

	src_sfr_base = (u32 __iomem *) (iva->iva_va + sect_info->base);

	/* init memory with 0xfc */
	memset(dst_sect_addr, 0xfc, sect_info->size);

	dev_dbg(iva->dev, "%s() start to save %s from %p (size: 0x%x)\n",
			__func__, sect_info->name, src_sfr_base, sect_info->size);

	/* call prepare function to secure dump its hwa's sfrs */
	if (sect_info->pre_func)
		sect_info->pre_func(iva);

	for (i = 0; i < sect_info->skip_num; i++) {
		aligned_from	= sect_info->skip_info[i].from & align_mask;
		aligned_to	= sect_info->skip_info[i].to & align_mask;

		/* check boundary */
		if ((aligned_from > aligned_to) ||
				aligned_from >= sect_info->size ||
				aligned_to >= sect_info->size) {
			dev_warn(iva->dev, "%s() %s has incorrect skip boundary",
					__func__, sect_info->name);
			dev_warn(iva->dev, "(0x%x - 0x%x) size(0x%x)\n",
					aligned_from, aligned_to,
					sect_info->size);
			continue;
		}

		/* mark skip area */
		for (skip_idx = aligned_from / size_u32;
				skip_idx <= aligned_to / size_u32; skip_idx++) {
			dst_sect_addr[skip_idx] = 0x0;
		}
	}

	for (i = 0; i < sect_info->size / size_u32; i++) {
		if (dst_sect_addr[i] == 0x0)	/* skip */
			continue;
		dst_sect_addr[i] = __raw_readl(src_sfr_base + i);
	}
}

static void iva_ram_dump_pretty_print_vcm_dbg(char *dst_buf, uint32_t dst_buf_sz,
						int *copy_size, uint32_t mux_sel,
						uint32_t reg_val)
{
	uint32_t i;

#define GET_BIT(pos) GET_BITS((pos), 1)
#define GET_BITS(pos, count) ((reg_val >> (pos)) & ((1u << (count)) - 1))
#define NUM_CMD_BUF 8
#define NUM_SCH 8
#define CMD_STATE(val) ((val) == 0 ? "IDLE" : (val) == 1 ? "RUN" : (val) == 2 \
	? "WAIT" : "UNKNOWN")
#define CMD_STATE_BIT(val) ((val) == 0 ? "IDLE" : "RUN")
#define PRINT_TO_BUF(fmt, ...) \
	*copy_size += snprintf(dst_buf + *copy_size, \
			dst_buf_sz - *copy_size - 1, fmt, __VA_ARGS__);

	if (mux_sel == 0x00) {
		PRINT_TO_BUF("%-7s | %-7s | %s\n", "Sched", "Active", "State");
		for (i = 0; i < NUM_SCH; ++i)
			PRINT_TO_BUF("%-7u | %-7u | %s\n", \
				i, GET_BIT(8 + i), CMD_STATE_BIT(GET_BIT(NUM_SCH - i - 1)));
	} else if (mux_sel == 0x01) {
		PRINT_TO_BUF("%-7s | %-10s | %-10s | %-10s | %s\n", \
			"Sched", "Cmd valid", "Cmd ready", "Cmd done", "Cmd rerun");
		for (i = 0; i < NUM_SCH; ++i)
			PRINT_TO_BUF("%-7u | %-10u | %-10u | %-10u | %u\n", \
				i, GET_BIT(24 + i), GET_BIT(16 + i),
				GET_BIT(8 + i), GET_BIT(0 + i));
	} else if (mux_sel >= 0x04 && mux_sel <= 0x1D) {
		i = mux_sel / 4 - 1;
		if (i >= 6) // Debug info for scheduler 6 is not available!
			++i;

		if (mux_sel % 2 == 0) {
			PRINT_TO_BUF("%-7s | %-7s | %-15s | %s\n", \
				"Sched", "PC", "Outer loop iter", "Inner loop iter");
			PRINT_TO_BUF("%-7u | %-7u | %-15u | %u\n", \
				i, GET_BITS(24, 7), GET_BITS(12, 12), GET_BITS(0, 12));
		} else {
			PRINT_TO_BUF("%-7s | %s\n", "Sched", "Back-Off counter");
			PRINT_TO_BUF("%-7u | %u\n", i, GET_BITS(0, 16));
		}
	} else if (mux_sel == 0x1E) {
		PRINT_TO_BUF("%-7s | %-7s | %-15s | %s\n", \
			"Command", "State", "AXI Cmd valid", "AXI Cmd ready");
		for (i = 0; i < NUM_CMD_BUF; ++i)
			PRINT_TO_BUF("%-7u | %-7s | %-15u | %u\n", \
				i, CMD_STATE(GET_BITS(i * 2, 2)), \
				GET_BIT(24 + i), GET_BIT(16 + i));
	} else if (mux_sel == 0x1F) {
		PRINT_TO_BUF("%-7s | %-10s | %-10s | %-10s | %-10s | %-15s | %s\n", \
			"AXI", "Cmd valid", "Cmd ready", "Data valid", \
			"Data ready", "Response valid", "Response ready");
		PRINT_TO_BUF("%-7s | %-10u | %-10u | %-10u | %-10u | %-15u | %u\n", \
			"Write", GET_BIT(17), GET_BIT(16), GET_BIT(15), \
			GET_BIT(14), GET_BIT(13), GET_BIT(12));
		PRINT_TO_BUF("%-7s | %-10u | %-10u | %-10u | %-10u | %-15s |\n", \
			"Read", GET_BIT(11), GET_BIT(10), GET_BIT(9), GET_BIT(8), "");
	}
	PRINT_TO_BUF("%c", '\n');
#undef PRINT_TO_BUF
#undef CMD_STATE
#undef NUM_SCH
#undef NUM_CMD_BUF
#undef GET_BITS
#undef GET_BIT
}

void iva_ram_dump_copy_vcm_dbg(struct iva_dev_data *iva,
		void *dst_buf, uint32_t dst_buf_sz)
{
	#define VCM_LOG_LINE_LENGTH	(64)
	#define VCM_DGBSEL_REG		(0xFFF0)
	#define ENABLE_CAPTURE_M	(0x1 << 16)
	#define VCM_DGBVAL_REG		(0xFFF4)
	const uint32_t vcm_dbg_mux_sel[] = {
#if defined(CONFIG_SOC_EXYNOS8895)
		0x03, 0x04, 0x07, 0x08, 0x0b, 0x0c, 0x0f, 0x10
#elif defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
		0x00, 0x01,
		0x04, 0x05,	/* sched 0 */
		0x08, 0x09,	/* sched 1 */
		0x0C, 0x0D,	/* sched 2 */
		0x10, 0x11,	/* sched 3 */
		0x14, 0x15,	/* sched 4 */
		0x18, 0x19,	/* sched 5 */
		0x1C, 0x1D,	/* sched 7 */
		0x1E,
		0x1F,
#endif
	};

	uint32_t	i;
	int		copy_size = 0;
	u32		reg_val;
	char		*dst_str = (char *) dst_buf;

	dev_err(iva->dev, "start to save vcm_dbg.\n");

	for (i = 0; i < (uint32_t) ARRAY_SIZE(vcm_dbg_mux_sel); i++) {
		writel(ENABLE_CAPTURE_M | vcm_dbg_mux_sel[i],
				iva->iva_va + IVA_VCM_BASE_ADDR + VCM_DGBSEL_REG);
		reg_val = readl(iva->iva_va + IVA_VCM_BASE_ADDR + VCM_DGBVAL_REG);
		copy_size += snprintf(dst_str + copy_size,
				dst_buf_sz - copy_size - 1 /* including null*/,
				"DBGSEL[%02x]=0x%08x\n",
				vcm_dbg_mux_sel[i], reg_val);
#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
		iva_ram_dump_pretty_print_vcm_dbg(dst_buf, dst_buf_sz, &copy_size,
				vcm_dbg_mux_sel[i], reg_val);
#endif
	}

	if (copy_size != 0)
		dst_str[copy_size - 1] = 0;
}

int32_t iva_ram_dump_copy_mcu_sram(struct iva_dev_data *iva,
		void *dst_buf, uint32_t dst_buf_sz)
{
	const struct iva_mmr_sect_info *sect_info = &iva_sfr_mcu;

#ifdef ENABLE_SRAM_ACCESS_WORKAROUND
#if defined(CONFIG_SOC_EXYNOS9810)
	iva_pmu_prepare_mcu_sram(iva);
#endif
#endif
#if defined(CONFIG_SOC_EXYNOS9820)
		if (iva->mcu_split_sram) {
			iva_mcu_prepare_mcu_reset(iva, 0x0);  /* split i/d srams */
		} else {
			sect_info = &iva_sfr_mcu_extend;
			iva_mcu_prepare_mcu_reset(iva, iva->mcu_bin->io_va);
		}
#endif
	if (dst_buf_sz != sect_info->size) {
		dev_err(iva->dev, "%s() required size(%x) but (0x%x)\n",
				__func__, sect_info->size, dst_buf_sz);
		return -EINVAL;
	}

	iva_ram_dump_copy_section_sfrs(iva, (u32 *) dst_buf, sect_info);
	return 0;
}

int32_t iva_ram_dump_copy_iva_sfrs(struct iva_proc *proc, int shared_fd)
{
	/* TO DO: request with a specific section */
	struct iva_dev_data	*iva = proc->iva_data;
	struct iva_mem_map	*iva_map_node;
	char			*dst_buf;
	uint32_t		iva_sfr_size = (uint32_t) resource_size(iva->iva_res);
	int			ret;
	uint32_t		i;
	const struct iva_mmr_sect_info *sect_info;

	if (!iva_ctrl_is_iva_on(iva)) {
		dev_err(iva->dev, "%s() iva mcu is not booted, state(0x%lx)\n",
				__func__, proc->iva_data->state);
		return -EPERM;
	}

	iva_map_node = iva_mem_find_proc_map_with_fd(proc, (int) shared_fd);
	if (!iva_map_node) {
		dev_warn(iva->dev, "%s() fail to get iva map node with fd(%d)\n",
				__func__, shared_fd);
		return -EINVAL;
	}

	if (iva_map_node->act_size < iva_sfr_size) {
		dev_warn(iva->dev, "%s() 0x%x required but 0x%x\n",
				__func__, iva_sfr_size, iva_map_node->act_size);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_EXYNOS8895)
	ret = dma_buf_begin_cpu_access(iva_map_node->dmabuf, 0, iva_sfr_size, 0);
#elif defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
	ret = dma_buf_begin_cpu_access(iva_map_node->dmabuf, 0);
#endif
	if (ret) {
		dev_warn(iva->dev, "%s() fail to begin cpu access with fd(%d)\n",
				__func__, shared_fd);
		return -ENOMEM;
	}

	dst_buf = (char *) dma_buf_kmap(iva_map_node->dmabuf, 0);
	if (!dst_buf) {
		dev_warn(iva->dev, "%s() fail to kmap ion with fd(%d)\n",
				__func__, shared_fd);
		ret = -ENOMEM;
		goto end_cpu_acc;
	}

	/* pmu */
	sect_info = &iva_sfr_pmu;
	iva_ram_dump_copy_section_sfrs(iva,
			(u32 *)(dst_buf + sect_info->base), sect_info);

	/* prepare all pmu-related preliminaries */
	iva_pmu_prepare_sfr_dump(iva);

	/* for vcm debugging */
	iva_ram_dump_copy_vcm_dbg(iva, dst_buf + VCM_DBG_SEL_SAVE_OFFSET,
			VCM_DBG_SEL_SAVE_MAX_SIZE);

	/* mcu */
#if defined(CONFIG_SOC_EXYNOS9820)
	if (iva->mcu_split_sram) {
#endif
		iva_ram_dump_copy_mcu_sram(iva,
			(void *)(dst_buf + IVA_VMCU_MEM_BASE_ADDR), VMCU_MEM_SIZE);
#if defined(CONFIG_SOC_EXYNOS9820)
	} else {
		iva_ram_dump_copy_mcu_sram(iva,
			(void *)(dst_buf + IVA_VMCU_MEM_BASE_ADDR), EXTEND_VMCU_MEM_SIZE);
	}
#endif

	/* general sfr section */
	for (i = 0; i < (uint32_t) ARRAY_SIZE(iva_sfr_sects); i++) {
		sect_info = iva_sfr_sects + i;
		iva_ram_dump_copy_section_sfrs(iva,
			(u32 *)(dst_buf + sect_info->base), sect_info);
	}

	dma_buf_kunmap(iva_map_node->dmabuf, 0, dst_buf);

end_cpu_acc:
#if defined(CONFIG_SOC_EXYNOS8895)
	dma_buf_end_cpu_access(iva_map_node->dmabuf, 0, IVA_CFG_SIZE, 0);
#elif defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9820)
	dma_buf_end_cpu_access(iva_map_node->dmabuf, 0);
#endif
	return ret;
}

static bool iva_ram_dump_handler(struct iva_mbox_cb_block *ctrl_blk, uint32_t msg)
{
	struct iva_dev_data *iva = (struct iva_dev_data *) ctrl_blk->priv_data;

	dev_info(iva->dev, "%s() msg(0x%x)\n", __func__, msg);

	/* notify the error state to user */
	iva_mcu_handle_error_k(iva, mcu_err_from_irq, 31 /* SYS_ERR */);

	return true;
}

struct iva_mbox_cb_block iva_rd_cb = {
	.callback	= iva_ram_dump_handler,
	.priority	= 0,
};

int32_t iva_ram_dump_init(struct iva_dev_data *iva)
{
	iva_rd_cb.priv_data = iva;
	iva_mbox_callback_register(mbox_msg_fatal, &iva_rd_cb);

	dev_dbg(iva->dev, "%s() success!!!\n", __func__);
	return 0;
}

int32_t iva_ram_dump_deinit(struct iva_dev_data *iva)
{
	iva_mbox_callback_unregister(&iva_rd_cb);

	iva_rd_cb.priv_data = NULL;

	dev_dbg(iva->dev, "%s() success!!!\n", __func__);
	return 0;
}
