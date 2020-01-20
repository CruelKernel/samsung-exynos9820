/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _SDP_KEK_PACK_H_
#define _SDP_KEK_PACK_H_

#include <sdp/dek_common.h>

void init_kek_pack(void);

int add_kek_pack(int engine_id, int userid);
void del_kek_pack(int engine_id);

int add_kek(int engine_id, kek_t *kek);
int del_kek(int engine_id, int kek_type);
kek_t *get_kek(int engine_id, int kek_type, int *rc);

void put_kek(kek_t *kek);

int is_kek_pack(int engine_id);
int is_kek(int engine_id, int kek_type);

#endif /* _SDP_KEK_PACK_H_ */
