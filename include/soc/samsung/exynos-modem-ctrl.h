/*
 * Copyright (C) 2015 Samsung Electronics Co.Ltd
 * http://www.samsung.com
 *
 * EXYNOS MODEM CONTROL driver
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
*/

#ifndef __EXYNOS_MODEM_CTRL_H
#define __EXYNOS_MODEM_CTRL_H

#include <linux/skbuff.h>

extern int modem_force_crash_exit_ext(void);
extern int modem_send_panic_noti_ext(void);
extern bool __skb_free_head_cp_zerocopy(struct sk_buff *skb);

#if defined(CONFIG_CP_ZEROCOPY)
/**
 @brief		skb_free_head_cp_zerocopy
 @param skb	the pointer to skbuff
 @return bool	return true if skb->data in zerocopy buffer,
		not to do a rest of skb_free_head process.
*/
static inline bool skb_free_head_cp_zerocopy(struct sk_buff *skb)
{
	return __skb_free_head_cp_zerocopy(skb);
}
#else
static inline bool skb_free_head_cp_zerocopy(struct sk_buff *skb) {
	return false;
}
#endif

#endif
