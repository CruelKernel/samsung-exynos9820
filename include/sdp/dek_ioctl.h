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
 
#ifndef DEK_IOCTL_H_
#define DEK_IOCTL_H_

#include <sdp/dek_common.h>
#define __DEKIOC		        0x77

typedef struct _dek_arg_generate_dek {
	int engine_id;
	dek_t dek;
}dek_arg_generate_dek;

typedef struct _dek_arg_encrypt_dek {
	int engine_id;
	dek_t plain_dek;
	dek_t enc_dek;
}dek_arg_encrypt_dek;

typedef struct _dek_arg_decrypt_dek {
	int engine_id;
	dek_t plain_dek;
	dek_t enc_dek;
}dek_arg_decrypt_dek;

typedef struct _dek_arg_is_kek_avail {
    int engine_id;
    int kek_type;
    int ret;
}dek_arg_is_kek_avail;

/*
 * DEK_ON_BOOT indicates that there's persona in the system.
 *
 * The driver will load public key and encrypted private key.
 */
typedef struct _dek_arg_on_boot {
	int engine_id;
    int user_id;
	kek_t SDPK_Rpub;
    kek_t SDPK_Dpub;
    kek_t SDPK_EDpub;
}dek_arg_on_boot;

typedef struct _dek_arg_on_device_locked {
	int engine_id;
    int user_id;
}dek_arg_on_device_locked;

typedef struct _dek_arg_on_device_unlocked {
	int engine_id;
	kek_t SDPK_Rpri;
    kek_t SDPK_Dpri;
    kek_t SDPK_EDpri;
	kek_t SDPK_sym;
}dek_arg_on_device_unlocked;

typedef struct _dek_arg_on_user_added {
    int engine_id;
    int user_id;
	kek_t SDPK_Rpub;
    kek_t SDPK_Dpub;
    kek_t SDPK_EDpub;
}dek_arg_on_user_added;

typedef struct _dek_arg_on_user_removed {
	int engine_id;
	int user_id;
}dek_arg_on_user_removed, dek_arg_disk_cache_cleanup;

// SDP driver events
#define DEK_ON_BOOT              _IOW(__DEKIOC, 0, unsigned int)
#define DEK_ON_DEVICE_LOCKED     _IOW(__DEKIOC, 4, unsigned int)
#define DEK_ON_DEVICE_UNLOCKED   _IOW(__DEKIOC, 5, unsigned int)
#define DEK_ON_USER_ADDED        _IOW(__DEKIOC, 6, unsigned int)
#define DEK_ON_USER_REMOVED      _IOW(__DEKIOC, 7, unsigned int)
#define DEK_ON_CHANGE_PASSWORD   _IOW(__DEKIOC, 8, unsigned int)  // @Deprecated

// SDP driver DEK requests
#define DEK_GENERATE_DEK         _IOW(__DEKIOC, 1, unsigned int)
#define DEK_ENCRYPT_DEK          _IOW(__DEKIOC, 2, unsigned int)
#define DEK_DECRYPT_DEK          _IOR(__DEKIOC, 3, unsigned int)
#define DEK_GET_KEK         	 _IOW(__DEKIOC, 9, unsigned int)
#define DEK_DISK_CACHE_CLEANUP   _IOW(__DEKIOC, 10, unsigned int)
#define DEK_IS_KEK_AVAIL        _IOW(__DEKIOC, 11, unsigned int)

#endif /* DEK_IOCTL_H_ */
