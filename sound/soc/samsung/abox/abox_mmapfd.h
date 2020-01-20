/* sound/soc/samsung/abox/abox_mmap_fd.h
 *
 * ALSA SoC - Samsung Abox MMAP Exclusive mode driver
 *
 * Copyright (c) 2018 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SND_SOC_ABOX_MMAPFD_H
#define __SND_SOC_ABOX_MMAPFD_H

#include "abox.h"

#define BUFFER_ION_BYTES_MAX		(SZ_512K)

extern int abox_mmap_fd(struct abox_platform_data *data,
		struct snd_pcm_mmap_fd *mmap_fd);

extern int abox_ion_alloc(struct abox_platform_data *data,
		struct abox_ion_buf *buf,
		unsigned long iova,
		size_t size,
		size_t align);

extern int abox_ion_free(struct abox_platform_data *data);
#endif
