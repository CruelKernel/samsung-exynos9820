/*
 *  sec_audio_debug.c
 *
 *  Copyright (c) 2018 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _SEC_AUDIO_DEBUG_H
#define _SEC_AUDIO_DEBUG_H

struct sec_audio_log_data {
	ssize_t buff_idx;
	int full;
	char *audio_log_buffer;
	ssize_t read_idx;
	size_t sz_log_buff;
	int virtual;
	char *name;
};

#ifdef CONFIG_SND_SOC_SAMSUNG_AUDIO
int is_abox_rdma_enabled(int id);
int is_abox_wdma_enabled(int id);
void abox_debug_string_update(void);

int register_debug_mixer(struct snd_soc_card *card);
int alloc_sec_audio_log(struct sec_audio_log_data *p_dbg_log_data, size_t buffer_len);
void free_sec_audio_log(struct sec_audio_log_data *p_dbg_log_data);

void sec_audio_log(int level, struct device *dev, const char *fmt, ...);
void sec_audio_bootlog(int level, struct device *dev, const char *fmt, ...);
void sec_audio_pmlog(int level, struct device *dev, const char *fmt, ...);

#ifdef CHANGE_DEV_PRINT
#ifdef dev_err
#undef dev_err
#endif
#define dev_err(dev, fmt, arg...) \
do { \
	dev_printk(KERN_ERR, dev, fmt, ##arg); \
	sec_audio_log(3, dev, fmt, ##arg); \
} while (0)

#ifdef dev_warn
#undef dev_warn
#endif
#define dev_warn(dev, fmt, arg...) \
do { \
	dev_printk(KERN_WARNING, dev, fmt, ##arg); \
	sec_audio_log(4, dev, fmt, ##arg); \
} while (0)

#ifdef dev_info
#undef dev_info
#endif
#define dev_info(dev, fmt, arg...) \
do { \
	dev_printk(KERN_INFO, dev, fmt, ##arg); \
	sec_audio_log(6, dev, fmt, ##arg); \
} while (0)

#ifdef DEBUG
#ifdef dev_dbg
#undef dev_dbg
#endif
#define dev_dbg(dev, fmt, arg...) \
do { \
	dev_printk(KERN_DEBUG, dev, fmt, ##arg); \
	sec_audio_log(7, dev, fmt, ##arg); \
} while (0)
#endif /* DEBUG */

#endif /* CHANGE_DEV_PRINT */

#else /* CONFIG_SND_SOC_SAMSUNG_AUDIO */
inline int is_abox_rdma_enabled(int id)
{
	return 0;
}
inline int is_abox_wdma_enabled(int id)
{
	return 0;
}
inline void abox_gpr_string_update(void)
{}

int register_debug_mixer(struct snd_soc_card *card)
{
	return -EACCES;
}

inline int alloc_sec_audio_log(struct sec_audio_log_data *p_dbg_log_data, size_t buffer_len)
{
	return -EACCES;
}

inline void free_sec_audio_log(struct sec_audio_log_data *p_dbg_log_data)
{
}

inline void sec_audio_log(int level, struct device *dev, const char *fmt, ...)
{
}

inline void sec_audio_bootlog(int level, struct device *dev, const char *fmt, ...)
{
}

inline void sec_audio_pmlog(int level, struct device *dev, const char *fmt, ...)
{
}

#endif /* CONFIG_SND_SOC_SAMSUNG_AUDIO */

#endif
