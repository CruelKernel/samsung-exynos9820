/*
 *
 * drivers/media/tdmb/tdmb_port_fc8080.c
 *
 * tdmb driver
 *
 * Copyright (C) (2011, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fcntl.h>

#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/uaccess.h>

#include <linux/time.h>
#include <linux/timer.h>

#include <linux/vmalloc.h>

#include "tdmb.h"
#include "dmbdrv_wrap_fc8080.h"

static bool fc8080_on_air;
static bool fc8080_pwr_on;

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
#define FIC_PACKET_COUNT 3
#define MSC_PACKET_COUNT 16
#endif
/* #define TDMB_DEBUG_SCAN */
#ifdef TDMB_DEBUG_SCAN
static void __print_ensemble_info(struct ensemble_info_type *e_info)
{
	int i = 0;

	DPRINTK("ensem_freq(%d)\n", e_info->ensem_freq);
	DPRINTK("ensem_label(%s)\n", e_info->ensem_label);
	for (i = 0; i < e_info->tot_sub_ch; i++) {
		DPRINTK("sub_ch_id(0x%x)\n", e_info->sub_ch[i].sub_ch_id);
		DPRINTK("start_addr(0x%x)\n", e_info->sub_ch[i].start_addr);
		DPRINTK("tmid(0x%x)\n", e_info->sub_ch[i].tmid);
		DPRINTK("svc_type(0x%x)\n", e_info->sub_ch[i].svc_type);
		DPRINTK("svc_label(%s)\n", e_info->sub_ch[i].svc_label);
		DPRINTK("scids(0x%x)\n", e_info->sub_ch[i].scids);
		DPRINTK("ecc(0x%x)\n", e_info->sub_ch[i].ecc);
	}
}
#endif

static bool __get_ensemble_info(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	struct sub_channel_info_type *fci_sub_info;

	DPRINTK("%s : freq(%ld)\n", __func__, freq);

	e_info->tot_sub_ch
		= dmb_drv_get_dmb_sub_ch_cnt() + dmb_drv_get_dab_sub_ch_cnt();
	DPRINTK("total subchannel number : %d\n", e_info->tot_sub_ch);

	if (e_info->tot_sub_ch > 0) {
		int i;
		int j;
		int sub_i = 0;
		const char *ensembleName = NULL;

		ensembleName = (char *)dmb_drv_get_ensemble_label();

		if (ensembleName)
			strncpy((char *)e_info->ensem_label,
					(char *)ensembleName,
					ENSEMBLE_LABEL_MAX);

		e_info->ensem_freq = freq;

		for (i = 0; i < 2; i++) {
			int cnt;

			cnt = (i == 0)
				? dmb_drv_get_dmb_sub_ch_cnt()
				: dmb_drv_get_dab_sub_ch_cnt();

			for (j = 0; j < cnt; j++, sub_i++) {
				fci_sub_info = (i == 0)
					? dmb_drv_get_fic_dmb(j)
					: dmb_drv_get_fic_dab(j);

				e_info->ensem_id
					= fci_sub_info->uiEnsembleID;
				e_info->sub_ch[sub_i].sub_ch_id
					= fci_sub_info->ucSubchID;
				e_info->sub_ch[sub_i].start_addr
					= fci_sub_info->uiStartAddress;
				e_info->sub_ch[sub_i].tmid
					= fci_sub_info->ucTMId;
				e_info->sub_ch[sub_i].svc_type
					= fci_sub_info->ucServiceType;
				e_info->sub_ch[sub_i].svc_id
					= fci_sub_info->ulServiceID;

				e_info->sub_ch[sub_i].ca_flags
					= fci_sub_info->ucCAFlag;

				e_info->sub_ch[sub_i].scids
					= fci_sub_info->scids;
				e_info->sub_ch[sub_i].ecc
					= fci_sub_info->ecc;

				if (fci_sub_info->ucCAFlag)
					DPRINTK("%s: sub_channel_id(%d) ca_flag detected\n",
					__func__, sub_i);
				if (i == 0)
					memcpy(
						e_info->sub_ch[sub_i].svc_label,
						dmb_drv_get_sub_ch_dmb_label(j),
						SVC_LABEL_MAX);
				else
					memcpy(e_info->sub_ch[sub_i].svc_label,
						dmb_drv_get_sub_ch_dab_label(j),
						SVC_LABEL_MAX);

			}
		}
#ifdef TDMB_DEBUG_SCAN
		__print_ensemble_info(e_info);
#endif
		return true;
	}
	return false;
}

static void fc8080_power_off(void)
{
	DPRINTK("%s %d\n", __func__, fc8080_pwr_on);

	if (fc8080_pwr_on) {
		fc8080_on_air = false;
		fc8080_pwr_on = false;

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
		tdmb_tsi_stop();
#else
		tdmb_control_irq(false);
#endif
		dmb_drv_deinit();
		tdmb_control_gpio(false);
	}
}

static bool fc8080_power_on(int param)
{
	DPRINTK("%s\n", __func__);

	if (fc8080_pwr_on)
		return true;
	tdmb_control_gpio(true);
	if (dmb_drv_init(tdmb_get_if_handle()
#ifdef CONFIG_TDMB_XTAL_FREQ
		, param
#endif
	) == TDMB_FAIL) {
		tdmb_control_gpio(false);
		return false;
	}
#if !defined(CONFIG_TDMB_TSIF_SLSI) && !defined(CONFIG_TDMB_TSIF_QC)
	tdmb_control_irq(true);
#endif
	fc8080_pwr_on = true;
	return true;
}
static void fc8080_get_dm(struct tdmb_dm *info)
{
	if (fc8080_pwr_on == true && fc8080_on_air == true) {
		info->rssi = dmb_drv_get_rssi();
		info->antenna = dmb_drv_get_ant();
		info->ber = dmb_drv_get_ber();
		info->per = 0;
	} else {
		info->rssi = 100;
		info->ber = 2000;
		info->per = 0;
		info->antenna = 0;
	}
}

static bool fc8080_set_ch(unsigned long freq,
							unsigned char sub_ch_id,
							bool factory_test)
{
	unsigned long freq_temp = freq / 1000;
	unsigned char sub_ch_id_temp = sub_ch_id % 1000;
	unsigned char svc_type_temp = 0x0;

	if (sub_ch_id_temp >= 64) {
		sub_ch_id_temp -= 64;
		svc_type_temp  = 0x18;
	}

	DPRINTK("%s freq:%ld, sub_ch_id:%d, svc_type:%d\n", __func__,
			freq_temp, sub_ch_id_temp, svc_type_temp);

	fc8080_on_air = false;
#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	tdmb_tsi_stop();
	if (tdmb_tsi_start(dmb_drv_isr, MSC_PACKET_COUNT) != 0)
		return false;
#endif

	if (factory_test) {
		if (dmb_drv_set_ch_factory(freq_temp, sub_ch_id_temp, svc_type_temp) == 1) {
			DPRINTK("dmb_drv_set_ch_factory Success\n");
			fc8080_on_air = true;
			return true;
		}
		DPRINTK("dmb_drv_set_ch_factory Fail\n");
		return false;

	}
	if (dmb_drv_set_ch(freq_temp, sub_ch_id_temp, svc_type_temp) == 1) {
		DPRINTK("dmb_drv_set_ch Success\n");
		fc8080_on_air = true;
		return true;
	}
	DPRINTK("dmb_drv_set_ch Fail\n");
	return false;
}

static bool fc8080_scan_ch(struct ensemble_info_type *e_info
							, unsigned long freq)
{
	bool ret = false;

	if (fc8080_pwr_on == false || e_info == NULL)
		return ret;

#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	tdmb_tsi_stop();
	if (tdmb_tsi_start(dmb_drv_isr, FIC_PACKET_COUNT) != 0)
		return false;
#endif
	if (dmb_drv_scan_ch((freq / 1000)) == TDMB_SUCCESS)
		ret = __get_ensemble_info(e_info, freq);
	else
		ret = false;
	return ret;
}

static int fc8080_int_size(void)
{
#if defined(CONFIG_TDMB_TSIF_SLSI) || defined(CONFIG_TDMB_TSIF_QC)
	return 188*16;
#else
	return 188*40;
#endif
}

static struct tdmb_drv_func fci_fc8080_drv_func = {
	.power_on = fc8080_power_on,
	.power_off = fc8080_power_off,
	.scan_ch = fc8080_scan_ch,
	.get_dm = fc8080_get_dm,
	.set_ch = fc8080_set_ch,
#if !defined(CONFIG_TDMB_TSIF_SLSI) && !defined(CONFIG_TDMB_TSIF_QC)
	.pull_data = dmb_drv_isr,
#endif
	.get_int_size = fc8080_int_size,
};

struct tdmb_drv_func *fc8080_drv_func(void)
{
	DPRINTK("tdmb_get_drv_func : fc8080\n");
	return &fci_fc8080_drv_func;
}
