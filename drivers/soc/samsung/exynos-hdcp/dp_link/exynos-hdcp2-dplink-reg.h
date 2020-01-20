/* driver/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-dplink-reg.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_DPLINK_REG_H__
#define __EXYNOS_HDCP2_DPLINK_REG_H__

#define DPCD_PACKET_SIZE (8)

#define DPCD_ADDR_HDCP22_Rtx		0x69000
#define DPCD_ADDR_HDCP22_TxCaps		0x69008
#define DPCD_ADDR_HDCP22_cert_rx	0x6900B
#define DPCD_ADDR_HDCP22_Rrx		0x69215
#define DPCD_ADDR_HDCP22_RxCaps		0x6921D
#define DPCD_ADDR_HDCP22_Ekpub_km	0x69220
#define DPCD_ADDR_HDCP22_Ekh_km_w	0x692A0
#define DPCD_ADDR_HDCP22_m		0x692B0
#define DPCD_ADDR_HDCP22_Hprime		0x692C0
#define DPCD_ADDR_HDCP22_Ekh_km_r	0x692E0
#define DPCD_ADDR_HDCP22_rn		0x692F0
#define DPCD_ADDR_HDCP22_Lprime		0x692F8
#define DPCD_ADDR_HDCP22_Edkey0_ks	0x69318
#define DPCD_ADDR_HDCP22_Edkey1_ks	0x69320
#define DPCD_ADDR_HDCP22_riv		0x69328
#define DPCD_ADDR_HDCP22_RxInfo		0x69330
#define DPCD_ADDR_HDCP22_seq_num_V	0x69332
#define DPCD_ADDR_HDCP22_Vprime		0x69335
#define DPCD_ADDR_HDCP22_Rec_ID_list	0x69345
#define DPCD_ADDR_HDCP22_V		0x693E0
#define DPCD_ADDR_HDCP22_seq_num_M	0x693F0
#define DPCD_ADDR_HDCP22_k		0x693F3
#define DPCD_ADDR_HDCP22_stream_IDtype	0x693F5
#define DPCD_ADDR_HDCP22_Mprime		0x69473
#define DPCD_ADDR_HDCP22_RxStatus	0x69493
#define DPCD_ADDR_HDCP22_Type		0x69494

#endif
