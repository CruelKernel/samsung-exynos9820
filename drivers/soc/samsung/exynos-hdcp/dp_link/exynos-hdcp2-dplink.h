/* driver/soc/samsung/exynos-hdcp/dplink/exynos-hdcp2-dplink.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef __EXYNOS_HDCP2_DPLINK_H__
#define __EXYNOS_HDCP2_DPLINK_H__

#if defined(CONFIG_HDCP2_EMULATION_MODE)
int dplink_emul_handler(int cmd);
#endif

enum auth_state {
	HDCP_AUTH_PROCESS_IDLE	= 0x1,
	HDCP_AUTH_PROCESS_STOP	= 0x2,
	HDCP_AUTH_PROCESS_DONE	= 0x3
};

enum auth_signal {
	HDCP_DRM_OFF	= 0x100,
	HDCP_DRM_ON	= 0x200,
	HDCP_RP_READY	= 0x300,
};

enum drm_state {
	DRM_OFF = 0x0,
	DRM_ON = 0x1,
	DRM_SAME_STREAM_TYPE = 0x2	/* If the previous contents and stream_type id are the same flag */
};

enum dp_state {
	DP_DISCONNECT,
	DP_CONNECT,
	DP_HDCP_READY,
};

/* Do hdcp2.2 authentication with DP Receiver
 * and enable encryption if authentication is succeed.
 * @return
 *  - 0: Success
 *  - ENOMEM: hdcp context open fail
 *  - EACCES: authentication fail
 */
int hdcp_dplink_get_rxinfo(uint8_t *status);
int hdcp_dplink_authenticate(void);
int do_dplink_auth(struct hdcp_link_info *lk_handle);
void hdcp_clear_session(uint32_t id);
#endif
