/* include/soc/samsung/secmem.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * EXYNOS - Secure memory support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_SECMEM_H
#define __ASM_ARCH_SECMEM_H __FILE__

#define MAX_NAME_LEN	20

struct secchunk_info {
	int		index;
	char		name[MAX_NAME_LEN];
	phys_addr_t	base;
	size_t		size;
};

#if defined(CONFIG_ION)
struct secfd_info {
	int	fd;
	unsigned long phys;
};
#endif

struct secmem_crypto_driver_ftn {
	int (*lock) (void);
	int (*release) (void);
};

struct secmem_region {
	char		*virt_addr;
	unsigned long	phys_addr;
	unsigned long	len;
};

#if defined(CONFIG_EXYNOS_CONTENT_PATH_PROTECTION)
void secmem_crypto_register(struct secmem_crypto_driver_ftn *ftn);
void secmem_crypto_deregister(void);
#else
#define secmem_crypto_register(ftn)
#define secmem_crypto_deregister()
#endif

int drm_gsc_enable_locked(bool enable);

#define SECMEM_IOC_CHUNKINFO		_IOWR('S', 1, struct secchunk_info)
#define SECMEM_IOC_SET_DRM_ONOFF	_IOWR('S', 2, int)
#define SECMEM_IOC_GET_DRM_ONOFF	_IOWR('S', 3, int)
#define SECMEM_IOC_GET_CRYPTO_LOCK	_IOR('S', 4, int)
#define SECMEM_IOC_RELEASE_CRYPTO_LOCK	_IOR('S', 5, int)
#if defined(CONFIG_ION)
#define SECMEM_IOC_GET_FD_PHYS_ADDR    _IOWR('S', 8, struct secfd_info)
#endif
#define SECMEM_IOC_GET_CHUNK_NUM	_IOWR('S', 9, int)
#define SECMEM_IOC_SET_TZPC		_IOWR('S', 11, struct protect_info)
#define SECMEM_IOC_SET_PROTECT		_IOWR('S', 12, int)
#define SECMEM_IOC_SET_VIDEO_EXT_PROC	_IOWR('S', 13, int)
#define SECMEM_IOC_GET_DRM_PROT_VER	_IOWR('S', 14, int)

#endif /* __ASM_ARCH_SECMEM_H */
