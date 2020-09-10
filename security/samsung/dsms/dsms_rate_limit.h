/*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#ifndef _DSMS_RATE_LIMIT_H
#define _DSMS_RATE_LIMIT_H

#ifdef CONFIG_KUNIT
#include <kunit/mock.h>
#endif

extern void dsms_rate_limit_init(void);
extern int dsms_check_message_rate_limit(void);

#ifdef CONFIG_KUNIT
extern int dsms_max_messages_per_round(void);
extern __always_inline u64 round_end_ms(u64 round_start_ms);
extern __always_inline int is_new_round(u64 now_ms, u64 last_round_start_ms);
extern __always_inline u64 dsms_get_time_ms(void);
#endif

#endif /* _DSMS_RATE_LIMIT_H */
