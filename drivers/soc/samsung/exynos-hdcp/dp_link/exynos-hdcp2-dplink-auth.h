/* drivers/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-dplink-auth.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_DPLINK_AUTH_H__
#define __EXYNOS_HDCP2_DPLINK_AUTH_H__

#include "exynos-hdcp2-dplink-protocol-msg.h"

int dplink_determine_rx_hdcp_cap(struct hdcp_link_data *lk);
int dplink_exchange_master_key(struct hdcp_link_data *lk);
int dplink_locality_check(struct hdcp_link_data *lk);
int dplink_exchange_session_key(struct hdcp_link_data *lk);
int dplink_evaluate_repeater(struct hdcp_link_data *lk);
int dplink_wait_for_receiver_id_list(struct hdcp_link_data *lk);
int dplink_verify_receiver_id_list(struct hdcp_link_data *lk);
int dplink_send_receiver_id_list_ack(struct hdcp_link_data *lk);
int dplink_stream_manage(struct hdcp_link_data *lk);
int dplink_get_rxstatus(uint8_t *status);
int dplink_set_paring_available(void);
int dplink_set_hprime_available(void);
int dplink_set_rp_ready(void);
int dplink_set_reauth_req(void);
int dplink_set_integrity_fail(void);
void dplink_clear_irqflag_all(void);

#endif
