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

#include "modem_variation.h"

/* add declaration of modem & link type */
/* modem device support */
DECLARE_MODEM_INIT_DUMMY(dummy)

#ifndef CONFIG_UMTS_MODEM_XMM6260
DECLARE_MODEM_INIT_DUMMY(xmm6260)
#endif

#ifndef CONFIG_UMTS_MODEM_XMM6262
DECLARE_MODEM_INIT_DUMMY(xmm6262)
#endif

#ifndef CONFIG_LTE_MODEM_XMM7260
DECLARE_MODEM_INIT_DUMMY(xmm7260)
#endif

#ifndef CONFIG_CDMA_MODEM_CBP71
DECLARE_MODEM_INIT_DUMMY(cbp71)
#endif

#ifndef CONFIG_CDMA_MODEM_CBP72
DECLARE_MODEM_INIT_DUMMY(cbp72)
#endif

#ifndef CONFIG_CDMA_MODEM_CBP82
DECLARE_MODEM_INIT_DUMMY(cbp82)
#endif

#ifndef CONFIG_LTE_MODEM_CMC220
DECLARE_MODEM_INIT_DUMMY(cmc220)
#endif

#ifndef CONFIG_LTE_MODEM_CMC221
DECLARE_MODEM_INIT_DUMMY(cmc221)
#endif

#ifndef CONFIG_UMTS_MODEM_SS222
DECLARE_MODEM_INIT_DUMMY(ss222)
#endif

#ifndef CONFIG_UMTS_MODEM_SH222AP
DECLARE_MODEM_INIT_DUMMY(sh222ap)
#endif

#ifndef CONFIG_UMTS_MODEM_SS310AP
DECLARE_MODEM_INIT_DUMMY(ss310ap)
#endif

#ifndef CONFIG_UMTS_MODEM_SS300
DECLARE_MODEM_INIT_DUMMY(ss300)
#endif

#ifndef CONFIG_UMTS_MODEM_S5000AP
DECLARE_MODEM_INIT_DUMMY(s5000ap)
#endif

#ifndef CONFIG_UMTS_MODEM_S5100
DECLARE_MODEM_INIT_DUMMY(s5100)
#endif

#ifndef CONFIG_CDMA_MODEM_MDM6600
DECLARE_MODEM_INIT_DUMMY(mdm6600)
#endif

#ifndef CONFIG_GSM_MODEM_ESC6270
DECLARE_MODEM_INIT_DUMMY(esc6270)
#endif

#ifndef CONFIG_CDMA_MODEM_QSC6085
DECLARE_MODEM_INIT_DUMMY(qsc6085)
#endif

#ifndef CONFIG_TDSCDMA_MODEM_SPRD8803
DECLARE_MODEM_INIT_DUMMY(sprd8803)
#endif

/* link device support */
DECLARE_LINK_INIT_DUMMY(undefined)

#ifndef CONFIG_LINK_DEVICE_MIPI
DECLARE_LINK_INIT_DUMMY(mipi)
#endif

#ifndef CONFIG_LINK_DEVICE_USB
DECLARE_LINK_INIT_DUMMY(usb)
#endif

#ifndef CONFIG_LINK_DEVICE_HSIC
DECLARE_LINK_INIT_DUMMY(hsic)
#endif

#ifndef CONFIG_LINK_DEVICE_DPRAM
DECLARE_LINK_INIT_DUMMY(dpram)
#endif

#ifndef CONFIG_LINK_DEVICE_PLD
DECLARE_LINK_INIT_DUMMY(pld)
#endif

#ifndef CONFIG_LINK_DEVICE_C2C
DECLARE_LINK_INIT_DUMMY(c2c)
#endif

#ifndef CONFIG_LINK_DEVICE_LLI
DECLARE_LINK_INIT_DUMMY(lli)
#endif

#ifndef CONFIG_LINK_DEVICE_SHMEM
DECLARE_LINK_INIT_DUMMY(shmem)
#endif

#ifndef CONFIG_LINK_DEVICE_SPI
DECLARE_LINK_INIT_DUMMY(spi)
#endif

#ifndef CONFIG_LINK_DEVICE_PCIE
DECLARE_LINK_INIT_DUMMY(pcie);
#endif

static modem_init_call modem_init_func[MAX_MODEM_TYPE] = {
	[IMC_XMM6260] = MODEM_INIT_CALL(xmm6260),
	[IMC_XMM6262] = MODEM_INIT_CALL(xmm6262),
	[VIA_CBP71] = MODEM_INIT_CALL(cbp71),
	[VIA_CBP72] = MODEM_INIT_CALL(cbp72),
	[VIA_CBP82] = MODEM_INIT_CALL(cbp82),
	[SEC_CMC220] = MODEM_INIT_CALL(cmc220),
	[SEC_CMC221] = MODEM_INIT_CALL(cmc221),
	[SEC_SS222] = MODEM_INIT_CALL(ss222),
	[SEC_SS300] = MODEM_INIT_CALL(ss300),
	[SEC_SH222AP] = MODEM_INIT_CALL(sh222ap),
	[SEC_SS310AP] = MODEM_INIT_CALL(ss310ap),
	[SEC_S5000AP] = MODEM_INIT_CALL(s5000ap),
	[SEC_S5100] = MODEM_INIT_CALL(s5100),
	[QC_MDM6600] = MODEM_INIT_CALL(mdm6600),
	[QC_ESC6270] = MODEM_INIT_CALL(esc6270),
	[QC_QSC6085] = MODEM_INIT_CALL(qsc6085),
	[SPRD_SC8803] = MODEM_INIT_CALL(sprd8803),
	[IMC_XMM7260] = MODEM_INIT_CALL(xmm7260),
	[DUMMY] = MODEM_INIT_CALL(dummy),
};

static link_init_call link_init_func[LINKDEV_MAX] = {
	[LINKDEV_UNDEFINED] = LINK_INIT_CALL(undefined),
	[LINKDEV_MIPI] = LINK_INIT_CALL(mipi),
	[LINKDEV_USB] = LINK_INIT_CALL(usb),
	[LINKDEV_HSIC] = LINK_INIT_CALL(hsic),
	[LINKDEV_DPRAM] = LINK_INIT_CALL(dpram),
	[LINKDEV_PLD] = LINK_INIT_CALL(pld),
	[LINKDEV_C2C] = LINK_INIT_CALL(c2c),
	[LINKDEV_LLI] = LINK_INIT_CALL(lli),
	[LINKDEV_SHMEM] = LINK_INIT_CALL(shmem),
	[LINKDEV_SPI] = LINK_INIT_CALL(spi),
	[LINKDEV_PCIE] = LINK_INIT_CALL(pcie),
};

int call_modem_init_func(struct modem_ctl *mc, struct modem_data *pdata)
{
	if (modem_init_func[pdata->modem_type])
		return modem_init_func[pdata->modem_type](mc, pdata);
	else
		return -ENOTSUPP;
}

struct link_device *call_link_init_func(struct platform_device *pdev,
					enum modem_link link_type)
{
	if (link_init_func[link_type])
		return link_init_func[link_type](pdev);
	else
		return NULL;
}
