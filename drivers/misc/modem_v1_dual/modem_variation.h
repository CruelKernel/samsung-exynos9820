/*
 * Copyright (C) 2010 Samsung Electronics.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MODEM_VARIATION_H__
#define __MODEM_VARIATION_H__

#include "modem_prj.h"

#define DECLARE_MODEM_INIT(type)					\
	int type ## _init_modemctl_device(				\
				struct modem_ctl *mc,			\
				struct modem_data *pdata)

#define DECLARE_MODEM_INIT_DUMMY(type)					\
	DECLARE_MODEM_INIT(type) { return 0; }

#define DECLARE_LINK_INIT(type)						\
	struct link_device *type ## _create_link_device(		\
				struct platform_device *pdev)

#define DECLARE_LINK_INIT_DUMMY(type)					\
	DECLARE_LINK_INIT(type) { return NULL; }

#define MODEM_INIT_CALL(type)	type ## _init_modemctl_device

#define LINK_INIT_CALL(type)	type ## _create_link_device

/**
 * Add extern declaration of modem & link type
 * (CAUTION!!! Every DUMMY function must be declared in modem_variation.c)
 */

/* modem device support */
#ifdef CONFIG_UMTS_MODEM_XMM6260
DECLARE_MODEM_INIT(xmm6260);
#endif

#ifdef CONFIG_UMTS_MODEM_XMM6262
DECLARE_MODEM_INIT(xmm6262);
#endif

#ifdef CONFIG_LTE_MODEM_XMM7260
DECLARE_MODEM_INIT(xmm7260);
#endif

#ifdef CONFIG_CDMA_MODEM_CBP71
DECLARE_MODEM_INIT(cbp71);
#endif

#ifdef CONFIG_CDMA_MODEM_CBP72
DECLARE_MODEM_INIT(cbp72);
#endif

#ifdef CONFIG_CDMA_MODEM_CBP82
DECLARE_MODEM_INIT(cbp82);
#endif

#ifdef CONFIG_LTE_MODEM_CMC220
DECLARE_MODEM_INIT(cmc220);
#endif

#ifdef CONFIG_LTE_MODEM_CMC221
DECLARE_MODEM_INIT(cmc221);
#endif

#ifdef CONFIG_UMTS_MODEM_SH222AP
DECLARE_MODEM_INIT(sh222ap);
#endif

#ifdef CONFIG_UMTS_MODEM_SS310AP
DECLARE_MODEM_INIT(ss310ap);
#endif

#ifdef CONFIG_UMTS_MODEM_SS222
DECLARE_MODEM_INIT(ss222);
#endif

#ifdef CONFIG_UMTS_MODEM_SS300
DECLARE_MODEM_INIT(ss300);
#endif

#ifdef CONFIG_UMTS_MODEM_S5000AP
DECLARE_MODEM_INIT(s5000ap);
#endif

#ifdef CONFIG_UMTS_MODEM_S5100
DECLARE_MODEM_INIT(s5100);
#endif

#ifdef CONFIG_CDMA_MODEM_MDM6600
DECLARE_MODEM_INIT(mdm6600);
#endif

#ifdef CONFIG_GSM_MODEM_ESC6270
DECLARE_MODEM_INIT(esc6270);
#endif

#ifdef CONFIG_CDMA_MODEM_QSC6085
DECLARE_MODEM_INIT(qsc6085);
#endif

#ifdef CONFIG_TDSCDMA_MODEM_SPRD8803
DECLARE_MODEM_INIT(sprd8803);
#endif

/* link device support */
#ifdef CONFIG_LINK_DEVICE_MIPI
DECLARE_LINK_INIT(mipi);
#endif

#ifdef CONFIG_LINK_DEVICE_USB
DECLARE_LINK_INIT(usb);
#endif

#ifdef CONFIG_LINK_DEVICE_HSIC
DECLARE_LINK_INIT(hsic);
#endif

#ifdef CONFIG_LINK_DEVICE_DPRAM
DECLARE_LINK_INIT(dpram);
#endif

#ifdef CONFIG_LINK_DEVICE_PLD
DECLARE_LINK_INIT(pld);
#endif

#ifdef CONFIG_LINK_DEVICE_C2C
DECLARE_LINK_INIT(c2c);
#endif

#ifdef CONFIG_LINK_DEVICE_LLI
DECLARE_LINK_INIT(lli);
#endif

#ifdef CONFIG_LINK_DEVICE_SHMEM
DECLARE_LINK_INIT(shmem);
#endif

#ifdef CONFIG_LINK_DEVICE_SPI
DECLARE_LINK_INIT(spi);
#endif

#ifdef CONFIG_LINK_DEVICE_PCIE
DECLARE_LINK_INIT(pcie);
#endif

typedef int (*modem_init_call)(struct modem_ctl *, struct modem_data *);
typedef struct link_device *(*link_init_call)(struct platform_device *);

int call_modem_init_func(struct modem_ctl *mc, struct modem_data *pdata);

struct link_device *call_link_init_func(struct platform_device *pdev,
					       enum modem_link link_type);

#endif
