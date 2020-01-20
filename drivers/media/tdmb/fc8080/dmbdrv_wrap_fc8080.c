/*
 *	Copyright(c) 2008 SEC Corp. All Rights Reserved
 *
 *	File name : DMBDrv_wrap_FC8080.c
 *
 *	Description : fc8080 tuner control driver
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	History :
 *	----------------------------------------------------------------------
 *	2009/01/19	changsul.park		initial
 *	2009/09/23	jason			porting QSC6270
 */

#include "dmbdrv_wrap_fc8080.h"
#include "fci_types.h"
#include "bbm.h"
#include "fci_oal.h"
#include "fc8080_demux.h"
#include "fic.h"
#include "fci_tun.h"
#include "fc8080_regs.h"
#include "tdmb.h"

struct sub_channel_info_type dmb_subchannel_info;
struct sub_channel_info_type dab_subchannel_info;
struct sub_channel_info_type dat_subchannel_info;

static u32 saved_ber = 3000;
static u32 dmb_initialize;

unsigned char current_service_type = 0x18;
unsigned char current_subchannel_id;

#ifdef CONFIG_TDMB_XTAL_FREQ
u32 main_xtal_freq = 24576;
#endif

int tdmb_interrupt_fic_callback(u32 userdata, u8 *data, int length)
{
	fic_decoder_put((struct fic *)data, length);

	return 0;
}

#ifdef FEATURE_FC8080_DEBUG
#define FC8080_DMB 0x01
#define FC8080_DATA 0x08
#define FC8080_DAB 0x04

u16 dmb_mode = FC8080_DMB;
#endif

int tdmb_interrupt_msc_callback(
u32 userdata, u8 subchannel_id, u8 *data, int length)
{
	tdmb_store_data(&data[0], length);

	return 0;
}

static int viterbi_rt_ber_read(unsigned int *ber)
{
	u32 dmp_ber_rxd_bits;
	u32 dmp_ber_err_bits;

	bbm_com_write(NULL, 0xe01, 0x0f);

	bbm_com_long_read(NULL, 0xe40, &dmp_ber_rxd_bits);
	bbm_com_long_read(NULL, 0xe44, &dmp_ber_err_bits);

	if (dmp_ber_rxd_bits)
		*ber = (dmp_ber_err_bits * 10000) /	 dmp_ber_rxd_bits;
	else
		*ber = 3000;

	print_log(NULL, "BER : %d\n", *ber);
	return BBM_OK;
}


static int get_signal_level(u32 ber, u8 *level)
{
	if (ber >= 900)
		*level = 0;
	else if ((ber >= 800) && (ber < 900))
		*level = 1;
	else if ((ber >= 700) && (ber < 800))
		*level = 2;
	else if ((ber >= 600) && (ber < 700))
		*level = 3;
	else if ((ber >= 500) && (ber < 600))
		*level = 4;
	else if ((ber >= 400) && (ber < 500))
		*level = 5;
	else if (ber < 400)
		*level = 6;

	return BBM_OK;
}

void dmb_drv_channel_deselect_all(void)
{
	bbm_com_video_deselect(NULL, 0, 0, 0);
	bbm_com_audio_deselect(NULL, 0, 1);
	bbm_com_data_deselect(NULL, 0, 2);

	ms_wait(100);

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	fc8080_demux_deselect_video(current_subchannel_id, 0);
	fc8080_demux_deselect_channel(current_subchannel_id, 0);
#endif
}

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
void dmb_drv_isr(u8 *data, u32 length)
{
	fc8080_demux(data, length);
}
#else
void dmb_drv_isr(void)
{
	bbm_com_isr(NULL);
}
#endif

unsigned char dmb_drv_init(unsigned long param
#ifdef CONFIG_TDMB_XTAL_FREQ
	, u32 xtal_freq
#endif
)
{
#ifdef FEATURE_INTERFACE_TEST_MODE
	int i;
	u8 data;
	u16 wdata;
	u32 ldata;
	u8 temp = 0x1e;
#endif

#ifdef CONFIG_TDMB_SPI
	if (bbm_com_hostif_select(NULL, BBM_SPI, param))
		return TDMB_FAIL;
#elif defined(CONFIG_TDMB_EBI)
	if (bbm_com_hostif_select(NULL, BBM_PPI, param))
		return TDMB_FAIL;
#elif defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	if (bbm_com_hostif_select(NULL, BBM_I2C, param))
		return TDMB_FAIL;
#endif

	/* check for factory  chip interface test */
	if (bbm_com_probe(NULL) != BBM_OK) {
		DPRINTK("%s : BBM_PROBE fail\n", __func__);
		return TDMB_FAIL;
	}

#ifdef CONFIG_TDMB_XTAL_FREQ
	main_xtal_freq = xtal_freq;
#endif

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	fc8080_demux_fic_callback_register(
		0, tdmb_interrupt_fic_callback);
	fc8080_demux_msc_callback_register(
		0, tdmb_interrupt_msc_callback);
#else
	bbm_com_fic_callback_register(0, tdmb_interrupt_fic_callback);
	bbm_com_msc_callback_register(0, tdmb_interrupt_msc_callback);
#endif

	bbm_com_init(NULL);
	bbm_com_tuner_select(NULL, FC8080_TUNER, BAND3_TYPE);

#ifdef FEATURE_INTERFACE_TEST_MODE
	for (i = 0; i < 1000; i++) {
		bbm_com_write(NULL, 0xa4, i & 0xff);
		bbm_com_read(NULL, 0xa4, &data);
		if ((i & 0xff) != data)
			DPRINTK("FC8080 byte test (0x%x,0x%x)\r\n"
			, i & 0xff, data);
	}
	for (i = 0; i < 1000; i++) {
		bbm_com_word_write(NULL, 0xa4, i & 0xffff);
		bbm_com_word_read(NULL, 0xa4, &wdata);
		if ((i & 0xffff) != wdata)
			DPRINTK("FC8080 word test (0x%x,0x%x)\r\n"
			, i & 0xffff, wdata);
	}
	for (i = 0; i < 1000; i++) {
		bbm_com_long_write(NULL, 0xa4, i & 0xffffffff);
		bbm_com_long_read(NULL, 0xa4, &ldata);
		if ((i & 0xffffffff) != ldata)
			DPRINTK("FC8080 long test (0x%x,0x%x)\r\n"
			, i & 0xffffffff, ldata);
	}
	for (i = 0; i < 1000; i++) {
		temp = i&0xff;
		bbm_com_tuner_write(NULL, 0x13, 0x01, &temp, 0x01);
		bbm_com_tuner_read(NULL, 0x13, 0x01, &data, 0x01);
		if ((i & 0xff) != data)
			DPRINTK("FC8080 tuner test (0x%x,0x%x)\r\n"
			, i & 0xff, data);
	}
#endif

	saved_ber = 3000;
	dmb_initialize = 1;

	return TDMB_SUCCESS;
}

unsigned char dmb_drv_deinit(void)
{
	dmb_initialize = 0;

	dmb_drv_channel_deselect_all();

	bbm_com_deinit(NULL);

	bbm_com_fic_callback_deregister(NULL);
	bbm_com_msc_callback_deregister(NULL);

	bbm_com_hostif_deselect(NULL);

	return TDMB_SUCCESS;
}

#ifdef FIC_USE_I2C
void dmb_drv_get_fic(void)
{
	u8 i;
	u8 lmode;
	u16 mfIntStatus = 0;
	u8 buf[FIC_BUF_LENGTH / 2];

	bbm_com_read(NULL, BBM_TSO_SELREG, &lmode);
	bbm_com_write(NULL, BBM_TSO_SELREG, lmode &= ~0x40);
	bbm_com_word_write(NULL, BBM_BUF_ENABLE, 0x0100);
	for (i = 0; i < 24; i++) {
		bbm_com_word_read(NULL, BBM_BUF_STATUS, &mfIntStatus);
		if (mfIntStatus & 0x0100) {
			bbm_com_word_write(NULL, BBM_BUF_STATUS, mfIntStatus);

			bbm_com_data(NULL, BBM_FIC_I2C_RD
				, &buf[0], FIC_BUF_LENGTH / 2);

			fic_decoder_put((struct fic *)&buf[0]
				, FIC_BUF_LENGTH / 2);
			print_log(NULL, "fic_decoder_put 0x%x\n", buf[0]);

		}
		ms_wait(50);
	}

	bbm_com_word_write(NULL, BBM_BUF_ENABLE, 0x0000);
	bbm_com_write(NULL, BBM_TSO_SELREG, lmode);
}
#endif

unsigned char dmb_drv_scan_ch(unsigned long frequency)
{
	struct esbinfo_t *esb;

	if (!dmb_initialize)
		return TDMB_FAIL;

	if (bbm_com_tuner_set_freq(NULL, frequency))
		return TDMB_FAIL;

	fic_decoder_subchannel_info_clean();

	if (bbm_com_scan_status(NULL)) {
		bbm_com_word_write(NULL, BBM_BUF_ENABLE, 0x0000);
		return TDMB_FAIL;
	}

#ifdef FIC_USE_I2C
	dmb_drv_get_fic();
#else
	bbm_com_word_write(NULL, BBM_BUF_ENABLE, 0x0100);

	/* wait 1.2 sec for gathering fic information */
	ms_wait(1200);

	bbm_com_word_write(NULL, BBM_BUF_ENABLE, 0x0000);
#endif

	esb = fic_decoder_get_ensemble_info(0);
	if (esb->flag != 99) {
		print_log(NULL, "ESB ERROR\n");
		fic_decoder_subchannel_info_clean();
		return TDMB_FAIL;
	}

	if (strnlen(esb->label, sizeof(esb->label)) <= 0) {
		fic_decoder_subchannel_info_clean();
		print_log(NULL, "label ERROR\n");
		return TDMB_FAIL;
	}

	return TDMB_SUCCESS;
}

int dmb_drv_get_dmb_sub_ch_cnt(void)
{
	int i, n;

	if (!dmb_initialize)
		return 0;

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		struct service_info_t *svc_info;

		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if ((svc_info->tmid == 0x01)
				&& (svc_info->dscty == 0x18))
				n++;
		}
	}

	return n;
}

int dmb_drv_get_dab_sub_ch_cnt(void)
{
	int i, n;

	if (!dmb_initialize)
		return 0;

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		struct service_info_t *svc_info;

		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if ((svc_info->tmid == 0x00)
				&& (svc_info->ascty == 0x00))
				n++;
		}
	}

	return n;
}

int dmb_drv_get_dat_sub_ch_cnt(void)
{
	int i, n;

	if (!dmb_initialize)
		return 0;

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		struct service_info_t *svc_info;

		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if (svc_info->tmid == 0x03)
				n++;
		}
	}

	return n;
}


char *dmb_drv_get_ensemble_label()
{
	struct esbinfo_t *esb;

	if (!dmb_initialize)
		return NULL;

	esb = fic_decoder_get_ensemble_info(0);

	if (esb->flag == 99)
		return (char *)esb->label;

	return NULL;
}

char *dmb_drv_get_sub_ch_dmb_label(int subchannel_count)
{
	int i, n;
	char *label = NULL;

	if (!dmb_initialize)
		return NULL;

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		struct service_info_t *svc_info;

		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if ((svc_info->tmid == 0x01)
				&& (svc_info->dscty == 0x18)) {
				if (n == subchannel_count) {
					label = (char *) svc_info->label;
					break;
				}
				n++;
			}
		}
	}

	return label;
}

char *dmb_drv_get_sub_ch_dab_label(int subchannel_count)
{
	int i, n;
	char *label = NULL;

	if (!dmb_initialize)
		return NULL;

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		struct service_info_t *svc_info;

		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if ((svc_info->tmid == 0x00)
				&& (svc_info->ascty == 0x00)) {
				if (n == subchannel_count) {
					label = (char *) svc_info->label;
					break;
				}
				n++;
			}
		}
	}

	return label;
}

char *dmb_drv_get_sub_ch_dat_label(int subchannel_count)
{
	int i, n;
	char *label = NULL;

	if (!dmb_initialize)
		return NULL;

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		struct service_info_t *svc_info;

		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if (svc_info->tmid == 0x03) {
				if (n == subchannel_count) {
					label = (char *) svc_info->label;
					break;
				}
				n++;
			}
		}
	}

	return label;
}

struct sub_channel_info_type *dmb_drv_get_fic_dmb(int subchannel_count)
{
	int i, n, j;
	struct esbinfo_t *esb;
	struct service_info_t *svc_info;
	u8 num_of_user_appl;

	if (!dmb_initialize)
		return NULL;

	memset((void *)&dmb_subchannel_info, 0, sizeof(dmb_subchannel_info));

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if ((svc_info->tmid == 0x01)
				&& (svc_info->dscty == 0x18)) {
				if (n == subchannel_count) {
					dmb_subchannel_info.ucSubchID
						= svc_info->sub_channel_id;
					dmb_subchannel_info.uiStartAddress
						= 0;
					dmb_subchannel_info.ucTMId
						= svc_info->tmid;
					dmb_subchannel_info.ucServiceType
						= svc_info->dscty;
					dmb_subchannel_info.ulServiceID
						= svc_info->sid;
					dmb_subchannel_info.scids
						= svc_info->scids;
					dmb_subchannel_info.ucCAFlag
						= svc_info->ca_flag;
					num_of_user_appl =
						svc_info->num_of_user_appl;
					dmb_subchannel_info.num_of_user_appl
						= num_of_user_appl;
					for (j = 0; j < num_of_user_appl; j++) {
						dmb_subchannel_info.user_appl_type[j]
						= svc_info->user_appl_type[j];
						dmb_subchannel_info.user_appl_length[j]
						= svc_info->user_appl_length[j];
						memcpy(
						&dmb_subchannel_info.user_appl_data[j][0]
						, &svc_info->user_appl_data[j][0]
						, dmb_subchannel_info.user_appl_length[j]);
					}

					esb = fic_decoder_get_ensemble_info(0);
					if (esb->flag == 99)
						dmb_subchannel_info.uiEnsembleID
						= esb->eid;
					else
						dmb_subchannel_info.uiEnsembleID
						= 0;
					dmb_subchannel_info.ecc	= esb->ecc;

					break;
				}
				n++;
			}
		}
	}

	return &dmb_subchannel_info;
}

struct sub_channel_info_type *dmb_drv_get_fic_dab(int subchannel_count)
{
	int i, n;
	struct esbinfo_t *esb;
	struct service_info_t *svc_info;

	if (!dmb_initialize)
		return NULL;

	memset((void *)&dab_subchannel_info, 0, sizeof(dab_subchannel_info));

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if ((svc_info->tmid == 0x00)
				&& (svc_info->ascty == 0x00)) {
				if (n == subchannel_count) {
					dab_subchannel_info.ucSubchID =
						svc_info->sub_channel_id;
					dab_subchannel_info.uiStartAddress = 0;
					dab_subchannel_info.ucTMId
						= svc_info->tmid;
					dab_subchannel_info.ucServiceType =
						svc_info->ascty;
					dab_subchannel_info.ulServiceID =
						svc_info->sid;
					dab_subchannel_info.scids =
						svc_info->scids;
					dab_subchannel_info.ucCAFlag =
						svc_info->ca_flag;

					esb = fic_decoder_get_ensemble_info(0);
					if (esb->flag == 99)
						dab_subchannel_info.uiEnsembleID
						= esb->eid;
					else
						dab_subchannel_info.uiEnsembleID
						= 0;
					dab_subchannel_info.ecc	= esb->ecc;

					break;
				}
				n++;
			}
		}
	}

	return &dab_subchannel_info;
}

struct sub_channel_info_type *dmb_drv_get_fic_dat(int subchannel_count)
{
	int i, n, j;
	struct esbinfo_t *esb;
	struct service_info_t *svc_info;
	u8 num_of_user_appl;
	struct scInfo_t  *pScInfo;

	if (!dmb_initialize)
		return NULL;

	memset((void *)&dat_subchannel_info, 0, sizeof(dat_subchannel_info));

	n = 0;
	for (i = 0; i < MAX_SVC_NUM; i++) {
		svc_info = fic_decoder_get_service_info_list(i);

		if ((svc_info->flag & 0x07) == 0x07) {
			if (svc_info->tmid == 0x03) {
				if (n == subchannel_count) {
					dat_subchannel_info.ucSubchID =
						svc_info->sub_channel_id;
					dat_subchannel_info.uiStartAddress = 0;
					dat_subchannel_info.ucTMId
						= svc_info->tmid;
					pScInfo = get_sc_info(svc_info->scid);
					dat_subchannel_info.ucServiceType =
						pScInfo->dscty;
					dat_subchannel_info.ulServiceID =
						svc_info->sid;
					dat_subchannel_info.scids =
						svc_info->scids;
					dat_subchannel_info.ucCAFlag
						= svc_info->ca_flag;

					num_of_user_appl =
						svc_info->num_of_user_appl;
					dat_subchannel_info.num_of_user_appl
						= num_of_user_appl;
					for (j = 0; j < num_of_user_appl; j++) {
						dat_subchannel_info.user_appl_type[j]
						= svc_info->user_appl_type[j];
						dat_subchannel_info.user_appl_length[j]
						= svc_info->user_appl_length[j];
						memcpy(
						&dat_subchannel_info.user_appl_data[j][0]
						, &svc_info->user_appl_data[j][0]
						, dat_subchannel_info.user_appl_length[j]);
					}

					esb = fic_decoder_get_ensemble_info(0);
					if (esb->flag == 99)
						dat_subchannel_info.uiEnsembleID
						= esb->eid;
					else
						dat_subchannel_info.uiEnsembleID
						= 0;
					dat_subchannel_info.ecc	= esb->ecc;

					break;
				}
				n++;
			}
		}
	}

	return &dat_subchannel_info;
}

#ifdef FEATURE_FC8080_DEBUG
void dmb_drv_check_overrun(u8 reset)
{
	u16 overrun;
	u16 temp = 0;

	bbm_com_word_read(NULL, BBM_BUF_OVERRUN, &overrun);

	if (overrun & dmb_mode) {
		/* overrun clear */
		bbm_com_word_write(NULL, BBM_BUF_OVERRUN, overrun);

		if (reset) {
			/* buffer restore */
			bbm_com_word_read(NULL, BBM_BUF_ENABLE, &temp);
			temp &= ~dmb_mode;
			bbm_com_word_write(NULL, BBM_BUF_ENABLE, temp);
			temp |= dmb_mode;
			bbm_com_word_write(NULL, BBM_BUF_ENABLE, temp);
		}

		DPRINTK("fc8080 Overrun occurred\n");
	}

}
#endif

unsigned char dmb_drv_set_ch(
unsigned long frequency
, unsigned char subchannel
, unsigned char sevice_type)
{
	if (!dmb_initialize)
		return TDMB_FAIL;

	current_service_type = sevice_type;
	current_subchannel_id = subchannel;

	dmb_drv_channel_deselect_all();

	if (bbm_com_tuner_set_freq(NULL, frequency) != BBM_OK)
		return TDMB_FAIL;

	if (sevice_type == 0x18)
		bbm_com_video_select(NULL, subchannel, 0, 0);
	else if (sevice_type == 0x00)
		bbm_com_audio_select(NULL, subchannel, 1);
	else
		bbm_com_data_select(NULL, subchannel, 2);

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	if (sevice_type == 0x18)
		fc8080_demux_select_video(subchannel, 0);
	else if (sevice_type == 0x00)
		fc8080_demux_select_channel(subchannel, 1);
	else
		fc8080_demux_select_channel(subchannel, 2);
#endif

#ifdef FEATURE_FC8080_DEBUG
	if (sevice_type == 0x18)
		dmb_mode = FC8080_DMB;
	else if (sevice_type == 0x00)
		dmb_mode = FC8080_DAB;
	else
		dmb_mode = FC8080_DATA;
#endif

	return TDMB_SUCCESS;
}

unsigned char dmb_drv_set_ch_factory(
unsigned long frequency
, unsigned char subchannel
, unsigned char sevice_type)
{
	if (!dmb_initialize)
		return TDMB_FAIL;

	current_service_type = sevice_type;
	current_subchannel_id = subchannel;

	dmb_drv_channel_deselect_all();

	if (bbm_com_tuner_set_freq(NULL, frequency) != BBM_OK)
		return TDMB_FAIL;
	if (bbm_com_scan_status(NULL)) {
		DPRINTK("%s scan fail\n", __func__);
		return TDMB_FAIL;
	}
	if (sevice_type == 0x18)
		bbm_com_video_select(NULL, subchannel, 0, 0);
	else if (sevice_type == 0x00)
		bbm_com_audio_select(NULL, subchannel, 1);
	else
		bbm_com_data_select(NULL, subchannel, 2);

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	if (sevice_type == 0x18)
		fc8080_demux_select_video(subchannel, 0);
	else if (sevice_type == 0x00)
		fc8080_demux_select_channel(subchannel, 1);
	else
		fc8080_demux_select_channel(subchannel, 2);
#endif

#ifdef FEATURE_FC8080_DEBUG
	if (sevice_type == 0x18)
		dmb_mode = FC8080_DMB;
	else if (sevice_type == 0x00)
		dmb_mode = FC8080_DAB;
	else
		dmb_mode = FC8080_DATA;
#endif

	return TDMB_SUCCESS;
}

unsigned short dmb_drv_get_ber(void)
{
	return saved_ber;
}

unsigned char dmb_drv_get_ant(void)
{
	u8 level = 0;
	unsigned int ber;

	if (!dmb_initialize) {
		saved_ber = 3000;
		return 0;
	}

	if (viterbi_rt_ber_read(&ber)) {
		saved_ber = 3000;
		return 0;
	}

	if (ber <= 20)
		ber = 0;

	saved_ber = ber;
	if (get_signal_level(ber, &level))
		return 0;

#ifdef FEATURE_FC8080_DEBUG
	dmb_drv_check_overrun(1);
#endif

	return level;
}

signed short dmb_drv_get_rssi(void)
{
	s32 rssi;

	if (!dmb_initialize) {
		rssi = -110;
		return rssi;
	}

	bbm_com_tuner_get_rssi(NULL, &rssi);

	return (signed short)rssi;
}
