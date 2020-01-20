#ifndef _UAPI__SOUND_VTS_H
#define _UAPI__SOUND_VTS_H

#if defined(__KERNEL__) || defined(__linux__)
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#endif

#ifndef __KERNEL__
#include <stdlib.h>
#endif

#define VTSDRV_MISC_IOCTL_WRITE_SVOICE	_IOW('V', 0x00, int)
#define VTSDRV_MISC_IOCTL_WRITE_GOOGLE	_IOW('V', 0x01, int)
#define VTSDRV_MISC_MODEL_BIN_MAXSZ  0xB500

#endif /* _UAPI__SOUND_VTS_H */

