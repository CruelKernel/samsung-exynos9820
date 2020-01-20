#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/maxim_dsm.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif /* CONFIG_COMPAT */
#include <sound/sec_adaptation.h>

#define DEBUG_MAXIM_DSM
#ifdef DEBUG_MAXIM_DSM
#define dbg_maxdsm(format, args...)	\
pr_info("[MAXIM_DSM] %s: " format "\n", __func__, ## args)
#else
#define dbg_maxdsm(format, args...)
#endif /* DEBUG_MAXIM_DSM */

#define V30_SIZE	(PARAM_DSM_3_0_MAX >> 1)
#define V35_SIZE	((PARAM_DSM_3_5_MAX - PARAM_DSM_3_0_MAX) >> 1)
#define V40_SIZE	((PARAM_DSM_4_0_MAX - PARAM_DSM_3_5_MAX) >> 1)
#define A_V35_SIZE	(PARAM_A_DSM_3_5_MAX >> 1)
#define A_V40_SIZE	((PARAM_A_DSM_4_0_MAX - PARAM_A_DSM_3_5_MAX) >> 1)

static struct maxim_dsm maxdsm = {
	.regmap = NULL,
	.param_size = PARAM_DSM_3_5_MAX,
	.platform_type = PLATFORM_TYPE_A,
	.rx_port_id = DSM_RX_PORT_ID,
	.tx_port_id = DSM_TX_PORT_ID,
	.rx_mod_id = AFE_MODULE_ID_DSM_RX,
	.tx_mod_id = AFE_MODULE_ID_DSM_TX,
	.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS,
	.version = VERSION_3_5_A,
	.registered = 0,
	.update_cal = 0,
	.ignore_mask =
		MAXDSM_IGNORE_MASK_VOICE_COIL |
		MAXDSM_IGNORE_MASK_AMBIENT_TEMP,
	.sub_reg = 0,
};
module_param_named(ignore_mask, maxdsm.ignore_mask, uint,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

static struct param_set_data maxdsm_spt_saved_params[] = {
	{
		.name = DSM_API_SETGET_ENABLE_SMART_PT,
		.value = 0,
	},
};

static struct param_set_data maxdsm_saved_params[] = {
	{
		.name = PARAM_FEATURE_SET,
		.value = 0x1F,
	},
};

static struct param_info g_pbi[(PARAM_A_DSM_4_0_MAX >> 1)] = {
	{
		.id = PARAM_A_VOICE_COIL_TEMP,
		.addr = 0x2A004C,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_EXCURSION,
		.addr = 0x2A004E,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RDC,
		.addr = 0x2A0050,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_Q_LO,
		.addr = 0x2A0052,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_Q_HI,
		.addr = 0x2A0054,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_FRES_LO,
		.addr = 0x2A0056,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_FRES_HI,
		.addr = 0x2A0058,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_EXCUR_LIMIT,
		.addr = 0x2A005A,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_VOICE_COIL,
		.addr = 0x2A005C,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_THERMAL_LIMIT,
		.addr = 0x2A005E,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RELEASE_TIME,
		.addr = 0x2A0060,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ONOFF,
		.addr = 0x2A0062,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STATIC_GAIN,
		.addr = 0x2A0064,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LFX_GAIN,
		.addr = 0x2A0066,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_PILOT_GAIN,
		.addr = 0x2A0068,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_FEATURE_SET,
		.addr = 0x2A006A,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_SMOOTH_VOLT,
		.addr = 0x2A006C,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_HPF_CUTOFF,
		.addr = 0x2A006E,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LEAD_R,
		.addr = 0x2A0070,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RMS_SMOO_FAC,
		.addr = 0x2A0072,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_CLIP_LIMIT,
		.addr = 0x2A0074,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_THERMAL_COEF,
		.addr = 0x2A0076,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_QSPK,
		.addr = 0x2A0078,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_EXCUR_LOG_THRESH,
		.addr = 0x2A007A,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_TEMP_LOG_THRESH,
		.addr = 0x2A007C,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RES_FREQ,
		.addr = 0x2A007E,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RES_FREQ_GUARD_BAND,
		.addr = 0x2A0080,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_AMBIENT_TEMP,
		.addr = 0x2A0182,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ADMITTANCE_A1,
		.addr = 0x2A0184,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ADMITTANCE_A2,
		.addr = 0x2A0186,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ADMITTANCE_B0,
		.addr = 0x2A0188,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ADMITTANCE_B1,
		.addr = 0x2A018A,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ADMITTANCE_B2,
		.addr = 0x2A018C,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RTH1_HI,
		.addr = 0x2A018E,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RTH1_LO,
		.addr = 0x2A0190,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RTH2_HI,
		.addr = 0x2A0192,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_RTH2_LO,
		.addr = 0x2A0194,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_ATENGAIN_HI,
		.addr = 0x2A0196,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_ATENGAIN_LO,
		.addr = 0x2A0198,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_SPT_RAMP_DOWN_FRAMES,
		.addr = 0x2A019A,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_SPT_THRESHOLD_HI,
		.addr = 0x2A019C,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_SPT_THRESHOLD_LO,
		.addr = 0x2A019E,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_T_HORIZON,
		.addr = 0x2A01A0,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LFX_ADMITTANCE_A1,
		.addr = 0x2A01A2,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LFX_ADMITTANCE_A2,
		.addr = 0x2A01A4,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LFX_ADMITTANCE_B0,
		.addr = 0x2A01A6,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LFX_ADMITTANCE_B1,
		.addr = 0x2A01A8,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_LFX_ADMITTANCE_B2,
		.addr = 0x2A01AA,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_ALGORITHM_X_MAX,
		.addr = 0x2A01AC,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_TCTH1_HI,
		.addr = 0x2A01AE,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_TCTH1_LO,
		.addr = 0x2A01B0,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_TCTH2_HI,
		.addr = 0x2A01B2,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_TCTH2_LO,
		.addr = 0x2A01B4,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_ATTACK_HI,
		.addr = 0x2A01B6,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_ATTACK_LO,
		.addr = 0x2A01B8,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_RELEASE_HI,
		.addr = 0x2A01BA,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_RELEASE_LO,
		.addr = 0x2A01BC,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STL_SPK_FS,
		.addr = 0x2A01BE,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_Q_GUARD_BAND_HI,
		.addr = 0x2A01C0,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_Q_GUARD_BAND_LO,
		.addr = 0x2A01C2,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_A1_HI,
		.addr = 0x2A01C4,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_A1_LO,
		.addr = 0x2A01C6,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_A2_HI,
		.addr = 0x2A01C8,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_A2_LO,
		.addr = 0x2A01CA,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_B0_HI,
		.addr = 0x2A01CC,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_B0_LO,
		.addr = 0x2A01CE,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_B1_HI,
		.addr = 0x2A01D0,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_B1_LO,
		.addr = 0x2A01D2,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_B2_HI,
		.addr = 0x2A01D4,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_COEFFS_B2_LO,
		.addr = 0x2A01D6,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STIMPEDMODEL_FLAG,
		.addr = 0x2A01D8,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_Q_NOTCH_HI,
		.addr = 0x2A01DA,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_Q_NOTCH_LO,
		.addr = 0x2A01DC,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_POWER_MEASUREMENT,
		.addr = 0x2A01DE,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_MAINSPKHFCOMP,
		.addr = 0x2A01E0,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_EARPIECELEVEL,
		.addr = 0x2A01E2,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_XOVER_FREQ,
		.addr = 0x2A01E4,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
	{
		.id = PARAM_A_STEREO_MODE_CONF,
		.addr = 0x2A01E6,
		.size = 2,
		.type = sizeof(uint32_t),
		.val = 0,
	},
};

static DEFINE_MUTEX(dsm_fs_lock);
static DEFINE_MUTEX(dsm_dsp_lock);

#ifdef USE_DSM_LOG
static DEFINE_MUTEX(maxdsm_log_lock);

static uint32_t ex_seq_count_temp;
static uint32_t ex_seq_count_excur;
static uint32_t new_log_avail;

static int maxdsm_log_present;
static int maxdsm_log_max_present;
static struct tm maxdsm_log_timestamp;
static uint8_t maxdsm_byte_log_array[BEFORE_BUFSIZE];
static uint32_t maxdsm_int_log_array[BEFORE_BUFSIZE];
static uint8_t maxdsm_after_prob_byte_log_array[AFTER_BUFSIZE];
static uint32_t maxdsm_after_prob_int_log_array[AFTER_BUFSIZE];
static uint32_t maxdsm_int_log_max_array[LOGMAX_BUFSIZE];  

static struct param_info g_lbi[MAX_LOG_BUFFER_POS] = {
	{
		.id = WRITE_PROTECT,
		.addr = 0x2A0082,
		.size = 2,
	},
	{
		.id = LOG_AVAILABLE,
		.addr = 0x2A0084,
		.size = 2,
		.type = sizeof(uint8_t),
	},
	{
		.id = VERSION_INFO,
		.addr = 0x2A0086,
		.size = 2,
		.type = sizeof(uint8_t),
	},
	{
		.id = LAST_2_SEC_TEMP,
		.addr = 0x2A0088,
		.size = 20,
		.type = sizeof(uint8_t),
	},
	{
		.id = LAST_2_SEC_EXCUR,
		.addr = 0x2A009C,
		.size = 20,
		.type = sizeof(uint8_t),
	},
	{
		.id = RESERVED_1,
		.addr = 0x2A00B0,
		.size = 2,
	},
	{
		.id = SEQUENCE_OF_TEMP,
		.addr = 0x2A00B2,
		.size = 2,
		.type = sizeof(uint32_t),
	},
	{
		.id = SEQUENCE_OF_EXCUR,
		.addr = 0x2A00B4,
		.size = 2,
		.type = sizeof(uint32_t),
	},
	{
		.id = LAST_2_SEC_RDC,
		.addr = 0x2A00B6,
		.size = 20,
		.type = sizeof(uint32_t),
	},
	{
		.id = LAST_2_SEC_FREQ,
		.addr = 0x2A00CA,
		.size = 20,
		.type = sizeof(uint32_t),
	},
	{
		.id = RESERVED_2,
		.addr = 0x2A00DE,
		.size = 2,
	},
	{
		.id = RESERVED_3,
		.addr = 0x2A00E0,
		.size = 2,
	},
	{
		.id = AFTER_2_SEC_TEMP_TEMP,
		.addr = 0x2A00E2,
		.size = 20,
		.type = sizeof(uint8_t) * 2,
	},
	{
		.id = AFTER_2_SEC_EXCUR_TEMP,
		.addr = 0x2A00F6,
		.size = 20,
		.type = sizeof(uint8_t) * 2,
	},
	{
		.id = AFTER_2_SEC_TEMP_EXCUR,
		.addr = 0x2A010A,
		.size = 20,
		.type = sizeof(uint8_t) * 2,
	},
	{
		.id = AFTER_2_SEC_EXCUR_EXCUR,
		.addr = 0x2A011E,
		.size = 20,
		.type = sizeof(uint8_t) * 2,
	},
	{
		.id = AFTER_2_SEC_RDC_TEMP,
		.addr = 0x2A0132,
		.size = 20,
		.type = sizeof(uint32_t) * 2,
	},
	{
		.id = AFTER_2_SEC_FREQ_TEMP,
		.addr = 0x2A0146,
		.size = 20,
		.type = sizeof(uint32_t) * 2,
	},
	{
		.id = AFTER_2_SEC_RDC_EXCUR,
		.addr = 0x2A015A,
		.size = 20,
		.type = sizeof(uint32_t) * 2,
	},
	{
		.id = AFTER_2_SEC_FREQ_EXCUR,
		.addr = 0x2A016E,
		.size = 20,
		.type = sizeof(uint32_t) * 2,
	},
};
#endif /* USE_DSM_LOG */

static inline int32_t maxdsm_dsm_open(void *data)
{
	int32_t ret;

	if (maxdsm.ignore_mask == MAXDSM_IGNORE_MASK_ALL &&
			maxdsm.filter_set == DSM_ID_FILTER_SET_AFE_CNTRLS)
		return -EPERM;

	mutex_lock(&dsm_dsp_lock);
	ret = dsm_read_write(data);
	mutex_unlock(&dsm_dsp_lock);

	return ret;
}

void maxdsm_set_regmap(struct regmap *regmap)
{
	maxdsm.regmap = regmap;
	dbg_maxdsm("Regmap for maxdsm was set by 0x%p",
			maxdsm.regmap);
}
EXPORT_SYMBOL_GPL(maxdsm_set_regmap);

static int maxdsm_check_ignore_mask(uint32_t reg, uint32_t mask)
{
	int ret = 0;

	if ((mask & MAXDSM_IGNORE_MASK_VOICE_COIL) &&
			(reg == g_pbi[PARAM_A_VOICE_COIL >> 1].addr))
		ret = -EPERM;
	else if ((mask & MAXDSM_IGNORE_MASK_AMBIENT_TEMP) &&
			(reg == g_pbi[PARAM_A_AMBIENT_TEMP >> 1].addr))
		ret = -EPERM;
	else if (mask & MAXDSM_IGNORE_MASK_ALL)
		ret = -EPERM;

	return ret;
}

static int maxdsm_regmap_read(unsigned int reg,
		unsigned int *val)
{
	return maxdsm.regmap ?
		regmap_read(maxdsm.regmap, reg, val) : -ENXIO;
}

static int maxdsm_regmap_write(unsigned int reg,
		unsigned int val)
{
	if (maxdsm_check_ignore_mask(reg, maxdsm.ignore_mask)) {
		dbg_maxdsm("Ignored 0x%x register", reg);
		return 0;
	}

	return maxdsm.regmap ?
		regmap_write(maxdsm.regmap, reg, val) : -ENXIO;
}

static void maxdsm_read_all(void)
{
	memset(maxdsm.param, 0x00, maxdsm.param_size * sizeof(uint32_t));
	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		{
		if (maxdsm.param_size <= PARAM_A_DSM_4_0_MAX) {
			int param_idx, pbi_idx = 0;
			uint32_t data;

			while (pbi_idx++ < (maxdsm.param_size >> 1)) {
				param_idx = (pbi_idx - 1) << 1;
				data = 0;
				maxdsm_regmap_read(g_pbi[pbi_idx - 1].addr, &data);
				maxdsm.param[param_idx] = data;
				maxdsm.param[param_idx + 1] =
					(1 << maxdsm.binfo[pbi_idx - 1]);
				dbg_maxdsm("[%d,%d]: 0x%08x / 0x%08x (addr:0x%08x",
						param_idx, param_idx + 1,
						maxdsm.param[param_idx],
						maxdsm.param[param_idx + 1],
						g_pbi[pbi_idx - 1].addr);
			}
		} else {
			dbg_maxdsm("failed regmap read...please check the maxdsm.param_size [%d] !!!",maxdsm.param_size);
		}
		break;
		}
	case PLATFORM_TYPE_B:
		maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
		maxdsm_dsm_open(&maxdsm);
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_get_spk_state())
			maxim_dsm_read(0, PARAM_DSM_5_0_MAX, &maxdsm);
		break;
	}
}

static void maxdsm_write_all(void)
{
	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		{
		if (maxdsm.param_size <= PARAM_A_DSM_4_0_MAX) {
			int param_idx, pbi_idx = 0;

			while (pbi_idx++ < (maxdsm.param_size >> 1)) {
				param_idx = (pbi_idx - 1) << 1;
				maxdsm_regmap_write(
						g_pbi[pbi_idx - 1].addr,
						maxdsm.param[param_idx]);
				dbg_maxdsm("[%d,%d]: 0x%08x / 0x%08x (0x%08x)",
						param_idx, param_idx + 1,
						maxdsm.param[param_idx],
						maxdsm.param[param_idx + 1],
						g_pbi[pbi_idx - 1].addr);
			}
		} else {
			dbg_maxdsm("failed regmap write...please check the maxdsm.param_size [%d] !!!",maxdsm.param_size);
		}
		break;
		}
	case PLATFORM_TYPE_B:
		maxdsm.filter_set = DSM_ID_FILTER_SET_AFE_CNTRLS;
		maxdsm_dsm_open(&maxdsm);
		break;
	case PLATFORM_TYPE_C:
		maxim_dsm_write(&maxdsm.param[0], 0, PARAM_DSM_5_0_MAX);
		break;
	}
}

static int maxdsm_read_wrapper(unsigned int reg,
		unsigned int *val)
{
	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		if (reg >= START_ADDR_FOR_LSI &&
			reg <= END_ADDR_FOR_LSI)
			maxdsm_regmap_read(reg, val);
		break;
	case PLATFORM_TYPE_B:
		if (reg < PARAM_DSM_4_0_MAX) {
			maxdsm_read_all();
			*val = maxdsm.param[reg];
		}
		break;
	case PLATFORM_TYPE_C:
		if (maxdsm_get_spk_state() && (reg < PARAM_DSM_5_0_MAX)) {
			maxim_dsm_read(reg, 1, &maxdsm);
			*val = maxdsm.param[reg];
		}
		break;
	}

	return *val;
}

static int maxdsm_write_wrapper(unsigned int reg,
		unsigned int val, unsigned int flag)
{
	int ret = -ENODATA;

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_regmap_write(reg, val);
		maxdsm_regmap_read(reg, &ret);
		break;
	case PLATFORM_TYPE_B:
		if (reg > maxdsm.param_size)
			pr_err("%s: Unknown parameter index. %d\n",
					__func__, reg);
		else {
			maxdsm.param[PARAM_WRITE_FLAG] = flag;
			maxdsm.param[reg] = val;
			maxdsm_write_all();
			maxdsm_read_all();
			ret = maxdsm.param[reg];
		}
		break;
	case PLATFORM_TYPE_C:
		if (reg > maxdsm.param_size)
			pr_err("%s: Unknown parameter index. %d\n",
					__func__, reg);
		else {
			maxdsm.param[reg] = val;
			maxim_dsm_write(&maxdsm.param[0], reg, 1);
			maxim_dsm_read(reg, 1, &maxdsm);
			ret = maxdsm.param[reg];
		}
		break;
	}

	return ret;
}

#ifdef USE_DSM_LOG
void maxdsm_log_update(const void *byte_log_array,
		const void *int_log_array,
		const void *after_prob_byte_log_array,
		const void *after_prob_int_log_array,
		const void *int_log_max_array)
{
	struct timeval tv;
	uint32_t log_max_prev_array[LOGMAX_BUFSIZE];
	uint32_t int_log_overcnt_array[2];
	int i;

	mutex_lock(&maxdsm_log_lock);

	for (i = 0; i < 2; i++) {
		log_max_prev_array[i] = maxdsm_int_log_max_array[i];
		int_log_overcnt_array[i] = maxdsm_int_log_array[i];
	}

	memcpy(maxdsm_byte_log_array,
			byte_log_array, sizeof(maxdsm_byte_log_array));
	memcpy(maxdsm_int_log_array,
			int_log_array, sizeof(maxdsm_int_log_array));
	memcpy(maxdsm_after_prob_byte_log_array,
			after_prob_byte_log_array,
			sizeof(maxdsm_after_prob_byte_log_array));
	memcpy(maxdsm_after_prob_int_log_array,
			after_prob_int_log_array,
			sizeof(maxdsm_after_prob_int_log_array));
	memcpy(maxdsm_int_log_max_array,
			int_log_max_array, sizeof(maxdsm_int_log_max_array));

	for (i = 0; i < 2; i++) {
		/* [0] : excu max, [1] : temp max */
		if (log_max_prev_array[i] > maxdsm_int_log_max_array[i]) {
			maxdsm_int_log_max_array[i] = log_max_prev_array[i];
		}
		/* accumulate overcnt [0] : temp overcnt, [1] : excu overcnt */
		maxdsm_int_log_array[i] += int_log_overcnt_array[i];
	}

	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, 0, &maxdsm_log_timestamp);

	if (maxdsm_byte_log_array[0] & 0x3)
		maxdsm_log_present = 1;

	maxdsm_log_max_present = 1;

	dbg_maxdsm("[MAX|OVCNT] EX [%d|%d] TEMP [%d|%d]",
		maxdsm_int_log_max_array[0], maxdsm_int_log_array[1],
		maxdsm_int_log_max_array[1], maxdsm_int_log_array[0]);

	mutex_unlock(&maxdsm_log_lock);
}
EXPORT_SYMBOL_GPL(maxdsm_log_update);

void maxdsm_read_logbuf_reg(void)
{
	int idx;
	int b_idx, i_idx;
	int apb_idx, api_idx;
	int loop;
	uint32_t data;
	struct timeval tv;

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_B:
	case PLATFORM_TYPE_C:
		return;
	}

	mutex_lock(&maxdsm_log_lock);

	/* If the following variables are not initialized,
	* these can not have zero data on some linux platform.
	*/
	idx = b_idx = i_idx = apb_idx = api_idx = 0;

	while (idx < MAX_LOG_BUFFER_POS) {
		for (loop = 0; loop < g_lbi[idx].size; loop += 2) {
			if (!g_lbi[idx].type)
				continue;
			maxdsm_regmap_read(g_lbi[idx].addr + loop, &data);
			switch (g_lbi[idx].type) {
			case sizeof(uint8_t):
				maxdsm_byte_log_array[b_idx++] =
					data & 0xFF;
				break;
			case sizeof(uint32_t):
				maxdsm_int_log_array[i_idx++] =
					data & 0xFFFFFFFF;
				break;
			case sizeof(uint8_t)*2:
				maxdsm_after_prob_byte_log_array[apb_idx++] =
					data & 0xFF;
				break;
			case sizeof(uint32_t)*2:
				maxdsm_after_prob_int_log_array[api_idx++] =
					data & 0xFFFFFFFF;
				break;
			}
		}
		idx++;
	}

	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, 0, &maxdsm_log_timestamp);
	maxdsm_log_present = 1;

	mutex_unlock(&maxdsm_log_lock);
}

int maxdsm_get_dump_status(void)
{
	int ret = 0;

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		ret = maxdsm_regmap_read(g_lbi[LOG_AVAILABLE].addr,
				&new_log_avail);
		break;
	}

	return !ret ? (new_log_avail & 0x03) : ret;
}
EXPORT_SYMBOL_GPL(maxdsm_get_dump_status);

void maxdsm_update_param(void)
{
	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_regmap_write(g_lbi[WRITE_PROTECT].addr, 1);
		maxdsm_read_logbuf_reg();
		maxdsm_regmap_write(g_lbi[WRITE_PROTECT].addr, 0);
		break;
	case PLATFORM_TYPE_B:
		maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
		maxdsm_dsm_open(&maxdsm);

		maxdsm.filter_set = DSM_ID_FILTER_GET_LOG_AFE_PARAMS;
		maxdsm_dsm_open(&maxdsm);

		if (maxdsm.param[PARAM_EXCUR_LIMIT] != 0
				&& maxdsm.param[PARAM_THERMAL_LIMIT] != 0)
			new_log_avail |= 0x1;
		break;
	case PLATFORM_TYPE_C:
		maxdsm_read_all();
		maxim_dsm_read(PARAM_DSM_5_0_ABOX_GET_LOGGING,
					sizeof(maxdsm_byte_log_array)+
					sizeof(maxdsm_int_log_array)+
					sizeof(maxdsm_after_prob_byte_log_array)+
					sizeof(maxdsm_after_prob_int_log_array)+
					sizeof(maxdsm_int_log_max_array)
					, &maxdsm);
		maxdsm_log_update(&maxdsm.param[1],
							&maxdsm.param[(BEFORE_BUFSIZE/sizeof(uint32_t))+1],
							&maxdsm.param[(BEFORE_BUFSIZE/sizeof(uint32_t))+BEFORE_BUFSIZE+1],
							&maxdsm.param[((BEFORE_BUFSIZE/sizeof(uint32_t))+BEFORE_BUFSIZE)+(AFTER_BUFSIZE/sizeof(uint32_t))+1],
							&maxdsm.param[((BEFORE_BUFSIZE/sizeof(uint32_t))+BEFORE_BUFSIZE)+(AFTER_BUFSIZE/sizeof(uint32_t))+AFTER_BUFSIZE+1]);
		maxdsm_read_all();
		break;
	}
}
EXPORT_SYMBOL_GPL(maxdsm_update_param);

void maxdsm_log_max_prepare(struct maxim_dsm_log_max_values *values)
{

	if (maxdsm_log_max_present) {
		values->excursion_max = maxdsm_int_log_max_array[0]; /* excu log max */
		values->excursion_overcnt = maxdsm_int_log_array[1]; /* excu log overshoot cnt */
		values->coil_temp_max = maxdsm_int_log_max_array[1]; /* coil temp log max */
		values->coil_temp_overcnt = maxdsm_int_log_array[0]; /* coil temp log overshoot cnt */

		snprintf(values->dsm_timestamp, PAGE_SIZE,
				"%4d-%02d-%02d %02d:%02d:%02d (UTC)\n",
				(int)(maxdsm_log_timestamp.tm_year + 1900),
				(int)(maxdsm_log_timestamp.tm_mon + 1),
				(int)(maxdsm_log_timestamp.tm_mday),
				(int)(maxdsm_log_timestamp.tm_hour),
				(int)(maxdsm_log_timestamp.tm_min),
				(int)(maxdsm_log_timestamp.tm_sec));
	} else {
		dbg_maxdsm("No update the logging data. Need to call the DSM LOG mixer peviously\n");

		values->excursion_max = 0; /* excu log max */
		values->excursion_overcnt = 0; /* excu log overshoot cnt */
		values->coil_temp_max = 0; /* coil temp log max */
		values->coil_temp_overcnt = 0; /* coil temp log overshoot cnt */

		snprintf(values->dsm_timestamp, PAGE_SIZE,
				"No update time\n");
	}
}
EXPORT_SYMBOL_GPL(maxdsm_log_max_prepare);

void maxdsm_log_max_refresh(int values)
{
	switch (values) {
	case SPK_EXCURSION_MAX:
		maxdsm_int_log_max_array[0] = 0;
		break;
	case SPK_TEMP_MAX:
		maxdsm_int_log_max_array[1] = 0;
		break;
	case SPK_EXCURSION_OVERCNT:
		maxdsm_int_log_array[1] = 0;
		break;
	case SPK_TEMP_OVERCNT:
		maxdsm_int_log_array[0] = 0;
		break;
	default:
		dbg_maxdsm("Not proper argument for refresh logging max value[%d].",values);
		break;
	}
}
EXPORT_SYMBOL_GPL(maxdsm_log_max_refresh);

ssize_t maxdsm_log_prepare(char *buf)
{
	uint8_t *byte_log_array = maxdsm_byte_log_array;
	uint32_t *int_log_array = maxdsm_int_log_array;
	uint8_t *afterbyte_log_array = maxdsm_after_prob_byte_log_array;
	uint32_t *after_int_log_array = maxdsm_after_prob_int_log_array;
	uint32_t *int_log_max_array = maxdsm_int_log_max_array;
	int rc = 0;

	uint8_t log_available;
	uint8_t version_id;
	uint8_t *coil_temp_log_array;
	uint8_t *excur_log_array;
	uint8_t *after_coil_temp_log_array;
	uint8_t *after_excur_log_array;
	uint8_t *excur_after_coil_temp_log_array;
	uint8_t *excur_after_excur_log_array;

	uint32_t seq_count_temp;
	uint32_t seq_count_excur;
	uint32_t *rdc_log_array;
	uint32_t *freq_log_array;
	uint32_t *after_rdc_log_array;
	uint32_t *after_freq_log_array;
	uint32_t *excur_after_rdc_log_array;
	uint32_t *excur_after_freq_log_array;

	uint32_t coil_temp_log_max;
	uint32_t excur_log_max;
	uint32_t rdc_log_max;
	uint32_t freq_log_max;

	int param_excur_limit = PARAM_A_EXCUR_LIMIT;
	int param_thermal_limit = PARAM_A_THERMAL_LIMIT;
	int param_voice_coil = PARAM_A_VOICE_COIL;
	int param_release_time = PARAM_A_RELEASE_TIME;
	int param_static_gain = PARAM_A_STATIC_GAIN;
	int param_lfx_gain = PARAM_A_LFX_GAIN;
	int param_pilot_gain = PARAM_A_PILOT_GAIN;

	if (unlikely(!maxdsm_log_present)) {
		rc = -ENODATA;
	}

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		/* Already initialized */
		break;
	case PLATFORM_TYPE_B:
		param_excur_limit = PARAM_EXCUR_LIMIT;
		param_thermal_limit = PARAM_THERMAL_LIMIT;
		param_voice_coil = PARAM_VOICE_COIL;
		param_release_time = PARAM_RELEASE_TIME;
		param_static_gain = PARAM_STATIC_GAIN;
		param_lfx_gain = PARAM_LFX_GAIN;
		param_pilot_gain = PARAM_PILOT_GAIN;
		break;
	case PLATFORM_TYPE_C:
		param_excur_limit = DSM_API_SETGET_XCL_THRESHOLD;
		param_thermal_limit = DSM_API_SETGET_COILTEMP_THRESHOLD;
		param_voice_coil = DSM_API_SETGET_RDC_AT_ROOMTEMP;
		param_release_time = DSM_API_SETGET_LIMITERS_RELTIME;
		param_static_gain = DSM_API_SETGET_MAKEUP_GAIN;
		param_lfx_gain = DSM_API_SETGET_LFX_GAIN;
		param_pilot_gain = DSM_API_SETGET_PITONE_GAIN;
		break;
	}

	if (unlikely(rc)) {
		rc = snprintf(buf, PAGE_SIZE, "no log\n");
		if (maxdsm.param[param_excur_limit] != 0 &&
				maxdsm.param[param_thermal_limit] != 0) {
			rc += snprintf(buf+rc, PAGE_SIZE,
					"[Parameter Set] excursionlimit:0x%x, ",
					maxdsm.param[param_excur_limit]);
			rc += snprintf(buf+rc, PAGE_SIZE,
					"rdcroomtemp:0x%x, coilthermallimit:0x%x, ",
					maxdsm.param[param_voice_coil],
					maxdsm.param[param_thermal_limit]);
			rc += snprintf(buf+rc, PAGE_SIZE,
					"releasetime:0x%x\n",
					maxdsm.param[param_release_time]);
			rc += snprintf(buf+rc, PAGE_SIZE,
					"[Parameter Set] staticgain:0x%x, ",
					maxdsm.param[param_static_gain]);
			rc += snprintf(buf+rc, PAGE_SIZE,
					"lfxgain:0x%x, pilotgain:0x%0x\n",
					maxdsm.param[param_lfx_gain],
					maxdsm.param[param_pilot_gain]);
		}
		goto out;
	}

	log_available     = byte_log_array[0];
	version_id        = byte_log_array[1];
	coil_temp_log_array = &byte_log_array[2];
	excur_log_array    = &byte_log_array[2+LOG_BUFFER_ARRAY_SIZE];

	seq_count_temp       = int_log_array[0];
	seq_count_excur   = int_log_array[1];
	rdc_log_array  = &int_log_array[2];
	freq_log_array = &int_log_array[2+LOG_BUFFER_ARRAY_SIZE];

	after_coil_temp_log_array = &afterbyte_log_array[0];
	after_excur_log_array = &afterbyte_log_array[LOG_BUFFER_ARRAY_SIZE];
	after_rdc_log_array = &after_int_log_array[0];
	after_freq_log_array = &after_int_log_array[LOG_BUFFER_ARRAY_SIZE];

	excur_after_coil_temp_log_array
		= &afterbyte_log_array[LOG_BUFFER_ARRAY_SIZE*2];
	excur_after_excur_log_array
		= &afterbyte_log_array[LOG_BUFFER_ARRAY_SIZE*3];
	excur_after_rdc_log_array
		= &after_int_log_array[LOG_BUFFER_ARRAY_SIZE*2];
	excur_after_freq_log_array
		= &after_int_log_array[LOG_BUFFER_ARRAY_SIZE*3];

	coil_temp_log_max = int_log_max_array[0];
	excur_log_max     = int_log_max_array[1];
	rdc_log_max       = int_log_max_array[2];
	freq_log_max      = int_log_max_array[3];

	if (log_available > 0 &&
			(ex_seq_count_temp != seq_count_temp
			 || ex_seq_count_excur != seq_count_excur)) {
		ex_seq_count_temp = seq_count_temp;
		ex_seq_count_excur = seq_count_excur;
		new_log_avail |= 0x2;
	}

	rc += snprintf(buf+rc, PAGE_SIZE,
			"DSM LogData saved at %4d-%02d-%02d %02d:%02d:%02d (UTC)\n",
			(int)(maxdsm_log_timestamp.tm_year + 1900),
			(int)(maxdsm_log_timestamp.tm_mon + 1),
			(int)(maxdsm_log_timestamp.tm_mday),
			(int)(maxdsm_log_timestamp.tm_hour),
			(int)(maxdsm_log_timestamp.tm_min),
			(int)(maxdsm_log_timestamp.tm_sec));

	if ((log_available & 0x1) == 0x1) {
		rc += snprintf(buf+rc, PAGE_SIZE,
				"*** Excursion Limit was exceeded.\n");
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Seq:%d, log_available=%d, version_id:3.1.%d\n",
				seq_count_excur, log_available, version_id);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Temperature={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				excur_after_coil_temp_log_array[0],
				excur_after_coil_temp_log_array[1],
				excur_after_coil_temp_log_array[2],
				excur_after_coil_temp_log_array[3],
				excur_after_coil_temp_log_array[4],
				excur_after_coil_temp_log_array[5],
				excur_after_coil_temp_log_array[6],
				excur_after_coil_temp_log_array[7],
				excur_after_coil_temp_log_array[8],
				excur_after_coil_temp_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Excursion={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				excur_after_excur_log_array[0],
				excur_after_excur_log_array[1],
				excur_after_excur_log_array[2],
				excur_after_excur_log_array[3],
				excur_after_excur_log_array[4],
				excur_after_excur_log_array[5],
				excur_after_excur_log_array[6],
				excur_after_excur_log_array[7],
				excur_after_excur_log_array[8],
				excur_after_excur_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Rdc={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				excur_after_rdc_log_array[0],
				excur_after_rdc_log_array[1],
				excur_after_rdc_log_array[2],
				excur_after_rdc_log_array[3],
				excur_after_rdc_log_array[4],
				excur_after_rdc_log_array[5],
				excur_after_rdc_log_array[6],
				excur_after_rdc_log_array[7],
				excur_after_rdc_log_array[8],
				excur_after_rdc_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Frequency={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				excur_after_freq_log_array[0],
				excur_after_freq_log_array[1],
				excur_after_freq_log_array[2],
				excur_after_freq_log_array[3],
				excur_after_freq_log_array[4],
				excur_after_freq_log_array[5],
				excur_after_freq_log_array[6],
				excur_after_freq_log_array[7],
				excur_after_freq_log_array[8],
				excur_after_freq_log_array[9]);
	}

	if ((log_available & 0x2) == 0x2) {
		rc += snprintf(buf+rc, PAGE_SIZE,
				"*** Temperature Limit was exceeded.\n");
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Seq:%d, log_available=%d, version_id:3.1.%d\n",
				seq_count_temp, log_available, version_id);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Temperature={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n",
				coil_temp_log_array[0],
				coil_temp_log_array[1],
				coil_temp_log_array[2],
				coil_temp_log_array[3],
				coil_temp_log_array[4],
				coil_temp_log_array[5],
				coil_temp_log_array[6],
				coil_temp_log_array[7],
				coil_temp_log_array[8],
				coil_temp_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"              %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				after_coil_temp_log_array[0],
				after_coil_temp_log_array[1],
				after_coil_temp_log_array[2],
				after_coil_temp_log_array[3],
				after_coil_temp_log_array[4],
				after_coil_temp_log_array[5],
				after_coil_temp_log_array[6],
				after_coil_temp_log_array[7],
				after_coil_temp_log_array[8],
				after_coil_temp_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Excursion={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n",
				excur_log_array[0],
				excur_log_array[1],
				excur_log_array[2],
				excur_log_array[3],
				excur_log_array[4],
				excur_log_array[5],
				excur_log_array[6],
				excur_log_array[7],
				excur_log_array[8],
				excur_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"             %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				after_excur_log_array[0],
				after_excur_log_array[1],
				after_excur_log_array[2],
				after_excur_log_array[3],
				after_excur_log_array[4],
				after_excur_log_array[5],
				after_excur_log_array[6],
				after_excur_log_array[7],
				after_excur_log_array[8],
				after_excur_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Rdc={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n",
				rdc_log_array[0],
				rdc_log_array[1],
				rdc_log_array[2],
				rdc_log_array[3],
				rdc_log_array[4],
				rdc_log_array[5],
				rdc_log_array[6],
				rdc_log_array[7],
				rdc_log_array[8],
				rdc_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"      %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				after_rdc_log_array[0],
				after_rdc_log_array[1],
				after_rdc_log_array[2],
				after_rdc_log_array[3],
				after_rdc_log_array[4],
				after_rdc_log_array[5],
				after_rdc_log_array[6],
				after_rdc_log_array[7],
				after_rdc_log_array[8],
				after_rdc_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Frequency={ %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n",
				freq_log_array[0],
				freq_log_array[1],
				freq_log_array[2],
				freq_log_array[3],
				freq_log_array[4],
				freq_log_array[5],
				freq_log_array[6],
				freq_log_array[7],
				freq_log_array[8],
				freq_log_array[9]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"             %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }\n",
				after_freq_log_array[0],
				after_freq_log_array[1],
				after_freq_log_array[2],
				after_freq_log_array[3],
				after_freq_log_array[4],
				after_freq_log_array[5],
				after_freq_log_array[6],
				after_freq_log_array[7],
				after_freq_log_array[8],
				after_freq_log_array[9]);
	}

	if (maxdsm.param[param_excur_limit] != 0 &&
			maxdsm.param[param_thermal_limit] != 0) {
		rc += snprintf(buf+rc, PAGE_SIZE,
				"[Parameter Set] excursionlimit:0x%x, ",
				maxdsm.param[param_excur_limit]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"rdcroomtemp:0x%x, coilthermallimit:0x%x, ",
				maxdsm.param[param_voice_coil],
				maxdsm.param[param_thermal_limit]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"releasetime:0x%x\n",
				maxdsm.param[param_release_time]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"[Parameter Set] staticgain:0x%x, ",
				maxdsm.param[param_static_gain]);
		rc += snprintf(buf+rc, PAGE_SIZE,
				"lfxgain:0x%x, pilotgain:0x%x\n",
				maxdsm.param[param_lfx_gain],
				maxdsm.param[param_pilot_gain]);
	}

out:

	return (ssize_t)rc;
}
EXPORT_SYMBOL_GPL(maxdsm_log_prepare);
#endif /* USE_DSM_LOG */

#ifdef USE_DSM_UPDATE_CAL
ssize_t maxdsm_cal_prepare(char *buf)
{
	int rc = 0;
	int x;

	if ((maxdsm.update_cal) && (maxdsm.param_size <= PARAM_A_DSM_4_0_MAX))	{
		for (x = 0; x < (maxdsm.param_size >> 1); x++)	{
			rc += snprintf(buf+rc, PAGE_SIZE,
					"[%2d] 0x%08x, ",
					x,
					(int)(g_pbi[x].val));
			if ((x%5) == 4)
				rc += snprintf(buf+rc, PAGE_SIZE,
						"\n");
		}
		rc += snprintf(buf+rc, PAGE_SIZE,
				"Use Updated Parameters.\n");
	}	else
		rc += snprintf(buf+rc, PAGE_SIZE,
			"Use Default Parameters.\n");

	return (ssize_t)rc;
}
EXPORT_SYMBOL_GPL(maxdsm_cal_prepare);
#endif /* USE_DSM_UPDATE_CAL */

static int maxdsm_set_param(struct param_set_data *data, int size)
{
	int loop, ret = 0;

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		for (loop = 0; loop < size; loop++)
			maxdsm_regmap_write(data[loop].addr, data[loop].value);
		break;
	case PLATFORM_TYPE_B:
		maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
		ret = maxdsm_dsm_open(&maxdsm);
		maxdsm.param[PARAM_WRITE_FLAG] = data[0].wflag;
		for (loop = 0; loop < size; loop++)
			maxdsm.param[data[loop].name] = data[loop].value;
		maxdsm.filter_set = DSM_ID_FILTER_SET_AFE_CNTRLS;
		ret = maxdsm_dsm_open(&maxdsm);
		break;
	case PLATFORM_TYPE_C:
		maxdsm_read_all();
		maxdsm.param[DSM_API_SETGET_WRITE_FLAG] = data[0].wflag;
		for (loop = 0; loop < size; loop++)
			maxdsm.param[data[loop].name] = data[loop].value;
		maxdsm_write_all();
		break;
	default:
		return -ENODATA;
	}

	return ret < 0 ? ret : 0;
}

static int maxdsm_find_index_of_saved_params(
		struct param_set_data *params,
		int size,
		uint32_t param_name)
{
	int index = 0;

	while (index < size) {
		if (params[index].name == param_name)
			break;
		index++;
	}

	return index == size ? -ENODATA : index;
}

uint32_t maxdsm_get_platform_type(void)
{
	dbg_maxdsm("platform_type=%d", maxdsm.platform_type);
	return maxdsm.platform_type;
}
EXPORT_SYMBOL_GPL(maxdsm_get_platform_type);

uint32_t maxdsm_get_version(void)
{
	dbg_maxdsm("version=%d", maxdsm.version);
	return maxdsm.version;
}
EXPORT_SYMBOL_GPL(maxdsm_get_version);

uint32_t maxdsm_is_stereo(void)
{
	dbg_maxdsm("sub_reg=%d", maxdsm.sub_reg);
	return maxdsm.sub_reg;
}
EXPORT_SYMBOL_GPL(maxdsm_is_stereo);

int maxdsm_update_feature_en_adc(int apply)
{
	unsigned int val = 0;
	unsigned int reg, reg_r, ret = 0;
	struct param_set_data data = {
		.name = PARAM_FEATURE_SET,
		.addr = 0x2A006A,
		.value = 0x200,
		.wflag = FLAG_WRITE_FEATURE_ONLY,
	};

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		reg = data.addr;
		if (maxdsm_is_stereo() && maxdsm.version == VERSION_4_0_A_S) {
			reg_r = reg + DSM_4_0_LSI_STEREO_OFFSET;
		}
		break;
	case PLATFORM_TYPE_B:
		reg = data.name;
		data.value <<= 2;
		break;
	case PLATFORM_TYPE_C:
		reg = DSM_API_SETGET_PILOT_ENABLE;
		data.value = 1;
		break;
	default:
		return -ENODATA;
	}

	maxdsm_read_wrapper(reg, &val);

	if (apply)
		data.value = val | data.value;
	else
		data.value = val & ~data.value;
	dbg_maxdsm("apply=%d data.value=0x%x val=0x%x reg=%x",
			apply, data.value, val, reg);

	ret = maxdsm_set_param(&data, 1);

	if (reg_r) {
		data.addr = reg_r;
		maxdsm_read_wrapper(reg_r, &val);

		if (apply)
			data.value = val | data.value;
		else
			data.value = val & ~data.value;
		dbg_maxdsm("apply=%d data.value=0x%x val=0x%x reg=%x",
				apply, data.value, val, reg_r);

		ret = maxdsm_set_param(&data, 1);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_update_feature_en_adc);

int maxdsm_set_feature_en(int on)
{
	int index, ret = 0;
	struct param_set_data data = {
		.name = PARAM_FEATURE_SET,
		.value = 0,
		.wflag = FLAG_WRITE_FEATURE_ONLY,
	};

	maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
	ret = maxdsm_dsm_open(&maxdsm);
	if (ret)
		return ret;

	index = maxdsm_find_index_of_saved_params(
				maxdsm_saved_params,
				sizeof(maxdsm_saved_params)
					/ sizeof(struct param_set_data),
				data.name);
	if (index < 0 || (maxdsm.platform_type != PLATFORM_TYPE_B))
		return -ENODATA;

	if (on) {
		if (maxdsm_saved_params[index].value & 0x40) {
			pr_err("%s: feature_en has already 0x40\n",
					__func__);
			return -EALREADY;
		}

		maxdsm_saved_params[index].value
			= maxdsm.param[data.name];
		/* remove SPT bit */
		data.value =
			maxdsm_saved_params[index].value & ~0x80;
		/* enable rdc calibration bit */
		data.value |= 0x40;
		dbg_maxdsm("data.value=0x%08x", data.value);
		dbg_maxdsm("maxdsm_saved_params[%d].value=0x%08x",
				index, maxdsm_saved_params[index].value);
	} else {
		data.value = maxdsm_saved_params[index].value;
		dbg_maxdsm("data.value=0x%08x", data.value);
		dbg_maxdsm("maxdsm_saved_params[%d].value=0x%08x",
				index, maxdsm_saved_params[index].value);
	}

	return maxdsm_set_param(&data, 1);
}
EXPORT_SYMBOL_GPL(maxdsm_set_feature_en);

int maxdsm_set_cal_mode(int on)
{
	int index, ret = 0;

	maxdsm_read_all();

	index = maxdsm_find_index_of_saved_params(
				maxdsm_spt_saved_params,
				sizeof(maxdsm_spt_saved_params)
					/ sizeof(struct param_set_data),
				DSM_API_SETGET_ENABLE_SMART_PT);

	if (index < 0 || (maxdsm.platform_type != PLATFORM_TYPE_C))
		return -ENODATA;

	maxdsm.param[DSM_API_SETGET_WRITE_FLAG] = FLAG_WRITE_FEATURE_ONLY;

	if (on)
	{
		if (maxdsm.param[DSM_API_SETGET_ENABLE_SMART_PT]) //SPT enabled
		{
			maxdsm_spt_saved_params[index].value = maxdsm.param[DSM_API_SETGET_ENABLE_SMART_PT];

		}else {  //SPT disabled
			if (maxdsm.param[DSM_API_SETGET_ENABLE_RDC_CAL]) {
				dbg_maxdsm("already Rdc cal mode!!!return!!");
				return ret;
			}
		}
		maxdsm.param[DSM_API_SETGET_ENABLE_SMART_PT] = 0;
		maxdsm.param[DSM_API_SETGET_ENABLE_RDC_CAL] = 1;

		dbg_maxdsm("maxdsm.param[%d]=0x%08x -> for diabled SPT",
				DSM_API_SETGET_ENABLE_SMART_PT, maxdsm.param[DSM_API_SETGET_ENABLE_SMART_PT]);
		dbg_maxdsm("maxdsm_spt_saved_params[%d].value=0x%08x",
				index, maxdsm_spt_saved_params[index].value);
		dbg_maxdsm("maxdsm.param[%d]=0x%08x -> for enabled Rdc cal mode",
				DSM_API_SETGET_ENABLE_RDC_CAL, maxdsm.param[DSM_API_SETGET_ENABLE_RDC_CAL]);

	} else
	{
		maxdsm.param[DSM_API_SETGET_ENABLE_SMART_PT] = maxdsm_spt_saved_params[index].value;
		maxdsm.param[DSM_API_SETGET_ENABLE_RDC_CAL] = 0;

		dbg_maxdsm("maxdsm.param[%d]=0x%08x -> for recovering SPT",
				DSM_API_SETGET_ENABLE_SMART_PT, maxdsm.param[DSM_API_SETGET_ENABLE_SMART_PT]);
		dbg_maxdsm("maxdsm_spt_saved_params[%d].value=0x%08x",
				index, maxdsm_spt_saved_params[index].value);
		dbg_maxdsm("maxdsm.param[%d]=0x%08x -> for disabled Rdc cal mode",
				DSM_API_SETGET_ENABLE_RDC_CAL, maxdsm.param[DSM_API_SETGET_ENABLE_RDC_CAL]);
	}

	maxdsm_write_all();
	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_set_cal_mode);

int maxdsm_set_rdc_temp(int rdc, int temp)
{
	int ret = 0;

	dbg_maxdsm("rdc=0x%08x temp=0x%08x", rdc, temp);

	switch (maxdsm.platform_type) {
		case PLATFORM_TYPE_B:
			maxdsm.tx_port_id |= 1 << 31;
			maxdsm.param[PARAM_WRITE_FLAG] = FLAG_WRITE_RDC_CAL_ONLY;
			maxdsm.param[PARAM_VOICE_COIL] = rdc;
			maxdsm.param[PARAM_AMBIENT_TEMP] = temp << 19;
			maxdsm.filter_set = DSM_ID_FILTER_SET_AFE_CNTRLS;
			ret = maxdsm_dsm_open(&maxdsm);
			maxdsm.tx_port_id &= ~(1 << 31);
			break;
		case PLATFORM_TYPE_C:
			maxdsm.param[DSM_API_SETGET_WRITE_FLAG] = FLAG_WRITE_RDC_CAL_ONLY;
			maxdsm.param[DSM_API_SETGET_RDC_AT_ROOMTEMP] = rdc<<8;
			maxdsm.param[DSM_API_SETGET_COLDTEMP] = temp << 19;
			maxdsm_write_all();
			break;
		default:
			dbg_maxdsm("Invalid platform type[%d]!!",maxdsm.platform_type);
			break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_set_rdc_temp);

int maxdsm_set_dsm_onoff_status(int on)
{
	int ret = 0;
	struct param_set_data data[] = {
		{
			.name = PARAM_ONOFF,
			.addr = 0x2A0062,
			.value = on,
			.wflag = FLAG_WRITE_ONOFF_ONLY,
		},
	};

	ret = maxdsm_set_param(
			data,
			sizeof(data) / sizeof(struct param_set_data));

	if (maxdsm_is_stereo() && (maxdsm.version == VERSION_4_0_A_S)) {
		data->addr += DSM_4_0_LSI_STEREO_OFFSET;
		ret = maxdsm_set_param(
				data,
				sizeof(data) / sizeof(struct param_set_data));
	}

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_set_dsm_onoff_status);

uint32_t maxdsm_get_dcresistance(void)
{
	uint32_t dcresistance = 0;
	switch (maxdsm.platform_type) {
		case PLATFORM_TYPE_B:
			maxdsm.tx_port_id |= 1 << 31;
			maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
			maxdsm_dsm_open(&maxdsm);
			maxdsm.tx_port_id &= ~(1 << 31);
			dcresistance = maxdsm.param[PARAM_RDC];
			break;
		case PLATFORM_TYPE_C:
			maxdsm_read_all();
			dcresistance = maxdsm.param[DSM_API_GET_ADAPTIVE_DC_RES];
			break;
		default:
			dbg_maxdsm("Invalid platform type[%d]!!",maxdsm.platform_type);
			break;
	}

	return dcresistance;
}
EXPORT_SYMBOL_GPL(maxdsm_get_dcresistance);

uint32_t maxdsm_get_dsm_onoff_status(void)
{
	uint32_t dsm_onoff = 0;

	switch (maxdsm.platform_type) {
		case PLATFORM_TYPE_B:
			dsm_onoff = maxdsm.param[PARAM_ONOFF];
			break;
		case PLATFORM_TYPE_C:
			dsm_onoff = maxdsm.param[DSM_API_SETGET_ENABLE];
			break;
		default:
			dbg_maxdsm("Invalid platform type[%d]!!",maxdsm.platform_type);
			break;
	}
	return dsm_onoff;
}

int maxdsm_update_param_info(struct maxim_dsm *maxdsm)
{
	int i = 0;
	uint32_t binfo_c_v50[PARAM_DSM_5_0_MAX];

	uint32_t binfo_v30[V30_SIZE] = {
		/* dcResistance, coilTemp, qualityFactor */
		27, 19, 29,
		/* resonanceFreq, excursionMeasure, rdcroomtemp */
		9, 0, 27,
		/* releasetime, coilthermallimit, excursionlimit */
		30, 19, 27,
		/* dsmenable */
		0,
		/* staticgain, lfxgain, pilotgain */
		29, 30, 31,
		/* flagToWrite, featureSetEnable */
		0, 0,
		/* smooFacVoltClip, highPassCutOffFactor, leadResistance */
		30, 9, 27,
		/* rmsSmooFac, clipLimit, thermalCoeff */
		31, 27, 20,
		/* qSpk, excurLoggingThresh, coilTempLoggingThresh */
		29, 0, 0,
		/* resFreq, resFreqGuardBand */
		9, 9
	}; /* 26 */

	uint32_t binfo_v35[V35_SIZE] = {
		/* Ambient_Temp, STL_attack_tiem, STL_release_time */
		19, 19, 19,
		/* STL_Admittance_a1, STL_Admittance_a2 */
		30, 30,
		/* STL_Admittance_b1, STL_Admittance_b1, STL_Admittance_b2 */
		30, 30, 30,
		/* Tch1, Rth1, Tch2, Rth2 */
		20, 24, 20, 23,
		/* STL_Attenuation_Gain */
		30,
		/* SPT_rampDownFrames, SPT_Threshold */
		0, 0,
		/* T_horizon */
		0,
		/* LFX_Admittance_a1, LFX_Admittance_a2 */
		28, 28,
		/* LFX_Admittance_b0, LFX_Admittance_b1, LFX_Admittance_b2 */
		28, 28, 28,
	}; /* 21 */

	uint32_t binfo_v40[V40_SIZE] = {
		/* X_MAX, SPK_FS, Q_GUARD_BAND */
		15, 27, 29,
		/* STIMPEDMODEL_CEFS_A1, A2, B0, B1, B2 */
		30, 30, 27, 27, 27,
		/* STIMPEDEMODEL_FLAG, Q_NOTCH, POWER */
		0, 0, 8,
	};

	uint32_t binfo_a_v35[A_V35_SIZE] = {
		/* temp, excur, rdc, qc_low, qc_hi, fc_low, fc_high */
		0, 0, 0, 0, 0, 0, 0,
		/* excur limit, rdc roomt temp, temp limit */
		1, 1, 1,
		/* rel time, dsm on/off */
		2, 2,
		/* makeup gain, lfx gain, pilot gain */
		3, 3, 3,
		/* feature, smoofact, hpfcutoff, lead, rms_smoofact */
		4, 4, 4, 4, 4,
		/* volt, thermal coeff, qspk, excursion log, temp log */
		5, 5, 5, 5, 5,
		/* res freq, res freq guardband */
		6, 6,
		/* ambient temp, stl a1, stl a2, stl b0, stl b1, stl b2 */
		10, 10, 10, 10, 10, 10,
		/* rth1 hi, rth2 hi, rth1 lo, rth2 lo */
		11, 12, 13, 14,
		/* stl atengain hi, stl atengain lo */
		15, 16,
		/* ramp down, spt threshold hi, spt threshold lo */
		17, 18, 19,
		/* t horizon, lfx a1, lfx a2 */
		20, 21, 22,
		/* lfx b0, lfx b1, lfx b2, algorithm x */
		23, 24, 25, 26,
	};

	uint32_t binfo_a_v40[A_V40_SIZE] = {
		/* TCTH1 hi, TCTH1 lo, TCTH2 hi, TCTH2 lo */
		1, 2, 3, 4,
		/* ATTACK hi, ATTACK lo, RELEASE hi, RELEASE lo */
		5, 6, 7, 8,
		/* STL_SPK_FS, Q_GUARD_BAND hi, Q_GUARD_BAND lo */
		9, 10, 11,
		/* STIMPEDMODEL_CEFS_A1 hi, lo */
		12, 13,
		/* STIMPEDMODEL_CEFS_A2 hi, lo */
		14, 15,
		/* STIMPEDMODEL_CEFS_B0 hi, lo */
		16, 17,
		/* STIMPEDMODEL_CEFS_B1 hi, lo */
		18, 19,
		/* STIMPEDMODEL_CEFS_B2 hi, lo */
		20, 21,
		/* STIMPEDMODEL_FLAG, Q_NOTCH hi/lo, POWER */
		22, 23, 24, 8,
	};

	for (i=0;i<PARAM_DSM_5_0_MAX;i++) {
		binfo_c_v50[i] = 0;
	}

	/* Try to get parameter size. */
	switch (maxdsm->version) {
	case VERSION_4_0_A:
	case VERSION_4_0_A_S:
		maxdsm->param_size = PARAM_A_DSM_4_0_MAX;
		break;
	case VERSION_4_0_B:
		maxdsm->param_size = PARAM_DSM_4_0_MAX;
		break;
	case VERSION_3_5_A:
		maxdsm->param_size = PARAM_A_DSM_3_5_MAX;
		break;
	case VERSION_3_5_B:
		maxdsm->param_size = PARAM_DSM_3_5_MAX;
		break;
	case VERSION_3_0:
		maxdsm->param_size = PARAM_DSM_3_0_MAX;
		break;
	case VERSION_5_0_C:
		maxdsm->param_size = PARAM_DSM_5_0_MAX;
		break;

	default:
		pr_err("%s: Unknown version. %d\n", __func__,
				maxdsm->version);
		return -EINVAL;
	}

	kfree(maxdsm->binfo);
	maxdsm->binfo = kzalloc(
			sizeof(uint32_t) * maxdsm->param_size, GFP_KERNEL);
	if (!maxdsm->binfo)
		return -ENOMEM;

	/* Try to copy parameter size. */
	switch (maxdsm->version) {
	case VERSION_4_0_A:
	case VERSION_4_0_A_S:
		memcpy(&maxdsm->binfo[ARRAY_SIZE(binfo_a_v35)],
				binfo_a_v40, sizeof(binfo_a_v40));
	case VERSION_3_5_A:
		memcpy(maxdsm->binfo,
				binfo_a_v35, sizeof(binfo_a_v35));
		break;
	case VERSION_4_0_B:
		memcpy(&maxdsm->binfo[
				ARRAY_SIZE(binfo_v30) + ARRAY_SIZE(binfo_v35)],
				binfo_v40, sizeof(binfo_v40));
	case VERSION_3_5_B:
		memcpy(&maxdsm->binfo[ARRAY_SIZE(binfo_v30)],
				binfo_v35, sizeof(binfo_v35));
	case VERSION_3_0:
		memcpy(maxdsm->binfo,
				binfo_v30, sizeof(binfo_v30));
		break;
	case VERSION_5_0_C:
		memcpy(maxdsm->binfo,
				binfo_c_v50, sizeof(binfo_c_v50));
		break;
	}

	kfree(maxdsm->param);
	maxdsm->param = kzalloc(
			sizeof(uint32_t) * maxdsm->param_size, GFP_KERNEL);
	if (!maxdsm->param) {
		kfree(maxdsm->binfo);
		return -ENOMEM;
	}

	dbg_maxdsm("version=%d, platform_type=%d",
			maxdsm->version, maxdsm->platform_type);

	return 0;
}

int maxdsm_update_info(uint32_t *pinfo)
{
	int ret = 0;
	int32_t *data = pinfo;

	if (pinfo == NULL) {
		pr_debug("%s: pinfo was not set.\n",
				__func__);
		ret = -EINVAL;
		return ret;
	}

	maxdsm.platform_type = data[PARAM_OFFSET_PLATFORM];
	maxdsm.rx_port_id = data[PARAM_OFFSET_PORT_ID];
	maxdsm.tx_port_id = maxdsm.rx_port_id + 1;
	maxdsm.rx_mod_id = data[PARAM_OFFSET_RX_MOD_ID];
	maxdsm.tx_mod_id = data[PARAM_OFFSET_TX_MOD_ID];
	maxdsm.filter_set = data[PARAM_OFFSET_FILTER_SET];
	maxdsm.version = data[PARAM_OFFSET_VERSION];

	ret = maxdsm_update_param_info(&maxdsm);

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_update_info);

void maxdsm_update_sub_reg(uint32_t sub_reg)
{
	maxdsm.sub_reg = sub_reg;
	return;
}
EXPORT_SYMBOL_GPL(maxdsm_update_sub_reg);

int maxdsm_get_rx_port_id(void)
{
	return maxdsm.rx_port_id;
}
EXPORT_SYMBOL_GPL(maxdsm_get_rx_port_id);

int maxdsm_get_rx_mod_id(void)
{
	return maxdsm.rx_mod_id;
}
EXPORT_SYMBOL_GPL(maxdsm_get_rx_mod_id);

int maxdsm_get_tx_port_id(void)
{
	return maxdsm.tx_port_id;
}
EXPORT_SYMBOL_GPL(maxdsm_get_tx_port_id);

int maxdsm_get_tx_mod_id(void)
{
	return maxdsm.tx_mod_id;
}
EXPORT_SYMBOL_GPL(maxdsm_get_tx_mod_id);

int maxdsm_get_spk_state(void)
{
	return maxdsm.spk_state;
}
EXPORT_SYMBOL_GPL(maxdsm_get_spk_state);

void maxdsm_set_spk_state(int state)
{
	maxdsm.spk_state = state;
}
EXPORT_SYMBOL_GPL(maxdsm_set_spk_state);

uint32_t maxdsm_get_power_measurement(void)
{
	uint32_t val = 0;

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_regmap_read(0x2A01DE, &val);
		break;
	case PLATFORM_TYPE_B:
		maxdsm.tx_port_id |= 1 << 31;
		maxdsm_read_all();
		val = maxdsm.param[PARAM_POWER_MEASUREMENT];
		maxdsm.tx_port_id &= ~(1 << 31);
		break;
	case PLATFORM_TYPE_C:
		maxdsm_read_wrapper(DSM_API_SETGET_POWER_MEASUREMENT, &val);
		break;
	}

	return val;
}
EXPORT_SYMBOL_GPL(maxdsm_get_power_measurement);

void maxdsm_set_stereo_mode_configuration(unsigned int mode)
{
	dbg_maxdsm("platform %d, mode %d", maxdsm.platform_type, mode);
	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		maxdsm_regmap_write(0x2A0380, mode);
		break;
	case PLATFORM_TYPE_B:
	case PLATFORM_TYPE_C:
		break;
	}
}
EXPORT_SYMBOL_GPL(maxdsm_set_stereo_mode_configuration);

int maxdsm_set_pilot_signal_state(int on)
{
	int ret = 0;

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		break;
	case PLATFORM_TYPE_B:
		/* update dsm parameters */
		maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
		ret = maxdsm_dsm_open(&maxdsm);
		if (ret)
			goto error;

		/* feature_set parameter is set by pilot signal off */
		maxdsm.param[PARAM_WRITE_FLAG] = FLAG_WRITE_FEATURE_ONLY;
		maxdsm.param[PARAM_FEATURE_SET] =
			on ? maxdsm.param[PARAM_FEATURE_SET] & ~0x200
			: maxdsm.param[PARAM_FEATURE_SET] | 0x200;
		maxdsm.filter_set = DSM_ID_FILTER_SET_AFE_CNTRLS;
		ret = maxdsm_dsm_open(&maxdsm);
		if (ret)
			goto error;

		/* check feature_set parameter */
		maxdsm.filter_set = DSM_ID_FILTER_GET_AFE_PARAMS;
		ret = maxdsm_dsm_open(&maxdsm);
		if (!(maxdsm.param[PARAM_FEATURE_SET] & 0x200)) {
			dbg_maxdsm("Feature set param was not updated. 0x%08x",
					maxdsm.param[PARAM_FEATURE_SET]);
			ret = -EAGAIN;
		}
		break;
	case PLATFORM_TYPE_C:
		maxdsm_read_all();

		/* set pilot signal on/off */
		maxdsm.param[DSM_API_SETGET_WRITE_FLAG] = FLAG_WRITE_FEATURE_ONLY;
  		maxdsm.param[DSM_API_SETGET_PILOT_ENABLE] = on;
		maxdsm_write_all();

		/* check feature_set parameter */
		maxdsm_read_all();
		if (!maxdsm.param[DSM_API_SETGET_PILOT_ENABLE]) {
			dbg_maxdsm("Feature set param was not updated. 0x%08x",
					maxdsm.param[DSM_API_SETGET_PILOT_ENABLE]);
			ret = -EAGAIN;
		}
		break;
	}

error:
	return ret;
}

static int maxdsm_validation_check(uint32_t flag)
{
	int ret = 0;

	/* Validation check */
	switch (flag) {
	case FLAG_WRITE_ALL:
	case FLAG_WRITE_ONOFF_ONLY:
	case FLAG_WRITE_RDC_CAL_ONLY:
	case FLAG_WRITE_FEATURE_ONLY:
		break;
	default:
		pr_err("%s: Wrong information was received.\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}

#ifdef USE_DSM_UPDATE_CAL
int maxdsm_update_caldata(int on)
{
	int x;
	uint32_t val;
	int ret = 0;

	if (!maxdsm.update_cal || on == 0)	{
		dbg_maxdsm("Calibration data is not available. Cmd:%d", on);
		return ret;
	}

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		if (maxdsm.param_size <= PARAM_A_DSM_4_0_MAX) {
			for (x = 0; x < (maxdsm.param_size >> 1); x++)	{
				ret = maxdsm_regmap_read(g_pbi[x].addr, &val);
				if (val != g_pbi[x].val) {
					maxdsm_regmap_write(g_pbi[x].addr,
						g_pbi[x].val);
					dbg_maxdsm("[%d]: 0x%08x / 0x%08x",
							x, g_pbi[x].addr, g_pbi[x].val);
				}
			}
			if (maxdsm_is_stereo () && maxdsm.version == VERSION_4_0_A_S) {
				for (x = 0; x < (maxdsm.param_size >> 1); x++)	{
					maxdsm_regmap_write(g_pbi[x].addr+DSM_4_0_LSI_STEREO_OFFSET,
						g_pbi[x].val);
					dbg_maxdsm("[%d]: 0x%08x / 0x%08x",
							x, g_pbi[x].addr+DSM_4_0_LSI_STEREO_OFFSET, g_pbi[x].val);
				}
			}
		} else {
			dbg_maxdsm("failed update caldata...please check the platform A maxdsm.param_size [%d] !!!",maxdsm.param_size);
		}
		break;
	case PLATFORM_TYPE_B:
		if (maxdsm.param_size <= PARAM_DSM_4_0_MAX) {
			for (x = 0; x < maxdsm.param_size; x += 2)	{
				maxdsm.param[x] = g_pbi[x>>1].val;
				maxdsm.param[x+1] = 1 << maxdsm.binfo[x>>1];
				dbg_maxdsm("[%d]: 0x%08x / 0x%08x",
						x, maxdsm.param[x], maxdsm.param[x+1]);
			}
			maxdsm.param[PARAM_WRITE_FLAG] = FLAG_WRITE_ALL;
			maxdsm.filter_set = DSM_ID_FILTER_SET_AFE_CNTRLS;
			maxdsm_dsm_open(&maxdsm);
		} else {
			dbg_maxdsm("failed update caldata...please check the platform B maxdsm.param_size [%d] !!!",maxdsm.param_size);
		}
		break;
	}

	return ret;
}

int maxdsm_cal_avail(void)
{
	return maxdsm.update_cal;
}

static void maxdsm_store_caldata(void)
{
	int x;
	switch (maxdsm.platform_type) {
		case PLATFORM_TYPE_A:
		case PLATFORM_TYPE_B:
			if (maxdsm.param_size <= PARAM_A_DSM_4_0_MAX) {
				for (x = 0; x < (maxdsm.param_size >> 1); x++) {
					g_pbi[x].val = maxdsm.param[x<<1];
					dbg_maxdsm("[%d]: 0x%08x",
							x, g_pbi[x].val);
				}

				maxdsm.update_cal = 1;
			} else {
				dbg_maxdsm("failed store caldata...please check the maxdsm.param_size [%d] !!!",maxdsm.param_size);
			}
			break;
		default:
			x=0;
			break;
	}
}
#endif /* USE_DSM_UPDATE_CAL */

static int maxdsm_open(struct inode *inode, struct file *filep)
{
	return 0;
}

static long maxdsm_ioctl_handler(struct file *file,
		unsigned int cmd, unsigned int arg,
		void __user *argp)
{
	unsigned int reg, val;
	long ret = -EINVAL;

	mutex_lock(&dsm_fs_lock);

	switch (cmd) {
	case MAXDSM_IOCTL_GET_VERSION:
		ret = maxdsm.version;
		if (copy_to_user(argp,
				&maxdsm.version,
				sizeof(maxdsm.version)))
			goto error;
		break;
	case MAXDSM_IOCTL_SET_VERSION:
		if (arg < VERSION_3_0 ||
				arg >= VERSION_MAX)
			goto error;
		maxdsm.version = arg;
		ret = maxdsm_update_param_info(&maxdsm);
		if (!ret)
			goto error;
		ret = maxdsm.version;
		break;
	case MAXDSM_IOCTL_GET_ALL_PARAMS:
		maxdsm_read_all();
		if (copy_to_user(argp, maxdsm.param,
				sizeof(int) * maxdsm.param_size))
			goto error;
		break;
	case MAXDSM_IOCTL_SET_ALL_PARAMS:
		if (copy_from_user(maxdsm.param, argp,
				sizeof(int) * maxdsm.param_size))
			goto error;
		maxdsm_write_all();
		break;
	case MAXDSM_IOCTL_GET_PARAM:
		reg = (unsigned int)(arg & 0xFFFFFFFF);
		val = 0;
		/*
		 * protocol rule.
		 * PLATFORM_TYPE_A:
		 *	reg : register
		 *	val : value
		 * PLATFORM_TYPE_B:
		 * PLATFORM_TYPE_C:
		 *	reg : parameter index
		 *	val : value
		 */
		maxdsm_read_wrapper(reg, &val);
		ret = val;
		if (copy_to_user(argp, &val, sizeof(val)))
			goto error;
		break;
	case MAXDSM_IOCTL_SET_PARAM:
		if (copy_from_user(maxdsm.param, argp,
				sizeof(int) * 3/* reg, val, flag */))
			goto error;
		ret = maxdsm_write_wrapper(maxdsm.param[0],
				maxdsm.param[1], maxdsm.param[2]);
		break;
#ifdef USE_DSM_UPDATE_CAL
	case MAXDSM_IOCTL_GET_CAL_DATA:
		/* todo */
		break;
	case MAXDSM_IOCTL_SET_CAL_DATA:
		if (copy_from_user(maxdsm.param, argp,
				sizeof(int) * maxdsm.param_size))
			goto error;
		maxdsm_store_caldata();
		break;
#endif /* USE_DSM_UPDATE_CAL */
	case MAXDSM_IOCTL_GET_PLATFORM_TYPE:
		ret = maxdsm.platform_type;
		if (copy_to_user(argp,
				&maxdsm.platform_type,
				sizeof(maxdsm.platform_type)))
			goto error;
		break;
	case MAXDSM_IOCTL_SET_PLATFORM_TYPE:
		if (arg < PLATFORM_TYPE_A ||
				arg >= PLATFORM_TYPE_MAX)
			goto error;
		maxdsm.platform_type = arg;
		ret = maxdsm.platform_type;
		break;
	case MAXDSM_IOCTL_GET_RX_PORT_ID:
		ret = maxdsm.rx_port_id;
		if (copy_to_user(argp,
					&maxdsm.rx_port_id,
					sizeof(maxdsm.rx_port_id)))
			goto error;
		break;
	case MAXDSM_IOCTL_SET_RX_PORT_ID:
		if ((arg < AFE_PORT_ID_START ||
				arg > AFE_PORT_ID_END) &&
				(arg & 0x7FFF0000))
			goto error;
		maxdsm.rx_port_id = arg;
		ret = maxdsm.rx_port_id;
		break;
	case MAXDSM_IOCTL_GET_TX_PORT_ID:
		ret = maxdsm.tx_port_id;
		if (copy_to_user(argp,
					&maxdsm.tx_port_id,
					sizeof(maxdsm.tx_port_id)))
			goto error;
		break;
	case MAXDSM_IOCTL_SET_TX_PORT_ID:
		if ((arg < AFE_PORT_ID_START ||
				arg > AFE_PORT_ID_END) &&
				(arg & 0x7FFF0000))
			goto error;
		maxdsm.tx_port_id = arg;
		ret = maxdsm.tx_port_id;
		break;
	default:
		break;
	}

error:
	mutex_unlock(&dsm_fs_lock);

	return ret;
}

static long maxdsm_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	return maxdsm_ioctl_handler(file, cmd, arg,
			(void __user *)arg);
}

#ifdef CONFIG_COMPAT
static long maxdsm_compat_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	return maxdsm_ioctl_handler(file, cmd, arg,
			(void __user *)(unsigned long)arg);
}
#endif /* CONFIG_COMPAT */

static ssize_t maxdsm_read(struct file *filep, char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret;

	mutex_lock(&dsm_fs_lock);

	maxdsm_read_all();

	/* copy params to user */
	ret = copy_to_user(buf, maxdsm.param,
			sizeof(uint32_t) * maxdsm.param_size < count ?
			sizeof(uint32_t) * maxdsm.param_size : count);
	if (ret)
		pr_err("%s: copy_to_user failed - %d\n", __func__, ret);

	mutex_unlock(&dsm_fs_lock);

	return ret;
}

static ssize_t maxdsm_write(struct file *filep, const char __user *buf,
		size_t count, loff_t *ppos)
{
	int ret = 0;

	mutex_lock(&dsm_fs_lock);

	if (copy_from_user(maxdsm.param, buf,
			sizeof(uint32_t) * maxdsm.param_size)) {
		pr_err("%s: Failed to copy user data.\n", __func__);
		goto error;
	}

	switch (maxdsm.platform_type) {
	case PLATFORM_TYPE_A:
		dbg_maxdsm("reg=0x%x val=0x%x flag=%x regmap=%p",
				maxdsm.param[0],
				maxdsm.param[1],
				maxdsm.param[2],
				maxdsm.regmap);
		ret = maxdsm_validation_check(maxdsm.param[2]);
		if (!ret)
			maxdsm_regmap_write(maxdsm.param[0], maxdsm.param[1]);
		break;
	case PLATFORM_TYPE_B:
		ret = maxdsm_validation_check(maxdsm.param[PARAM_WRITE_FLAG]);
		if (!ret) {
			/* set params from the algorithm to application */
			maxdsm.filter_set = DSM_ID_FILTER_SET_AFE_CNTRLS;
			maxdsm_dsm_open(&maxdsm);
		}
		break;
	case PLATFORM_TYPE_C:
		maxim_dsm_write(&maxdsm.param[0], 0, PARAM_DSM_5_0_MAX);
		break;
	default:
		dbg_maxdsm("Unknown platform type %d",
				maxdsm.platform_type);
		ret = -ENODATA;
		break;
	}

error:
	mutex_unlock(&dsm_fs_lock);

	return ret;
}

static const struct file_operations dsm_ctrl_fops = {
	.owner			= THIS_MODULE,
	.open			= maxdsm_open,
	.release		= NULL,
	.unlocked_ioctl = maxdsm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= maxdsm_compat_ioctl,
#endif /* CONFIG_COMPAT */
	.read			= maxdsm_read,
	.write			= maxdsm_write,
	.mmap			= NULL,
	.poll			= NULL,
	.fasync			= NULL,
	.llseek			= NULL,
};

static struct miscdevice dsm_ctrl_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "dsm_ctrl_dev",
	.fops =     &dsm_ctrl_fops
};

void maxdsm_deinit(void)
{
	kfree(maxdsm.binfo);
	kfree(maxdsm.param);

	misc_deregister(&dsm_ctrl_miscdev);
}
EXPORT_SYMBOL_GPL(maxdsm_deinit);

int maxdsm_init(void)
{
	int ret;

	if (maxdsm.registered) {
		dbg_maxdsm("%s: Already registered.\n", __func__);
		return -EALREADY;
	}

	ret = misc_register(&dsm_ctrl_miscdev);
	if (!ret) {
		maxdsm.registered = 1;
		ret = maxdsm_update_param_info(&maxdsm);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(maxdsm_init);

MODULE_DESCRIPTION("Module for test Maxim DSM");
MODULE_LICENSE("GPL");
