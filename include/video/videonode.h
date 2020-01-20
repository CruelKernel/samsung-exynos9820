/* include/video/videonode.h
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Video node definitions for EXYNOS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __VIDEO_VIDEONODE_H
#define __VIDEO_VIDEONODE_H __FILE__

#define EXYNOS_VIDEONODE_MFC_DEC		6
#define EXYNOS_VIDEONODE_MFC_ENC		7
#define EXYNOS_VIDEONODE_MFC_DEC_DRM		8
#define EXYNOS_VIDEONODE_MFC_ENC_DRM		9
#define EXYNOS_VIDEONODE_MFC_ENC_OTF		10
#define EXYNOS_VIDEONODE_MFC_ENC_OTF_DRM	11


#define EXYNOS_VIDEONODE_SCALER(x)		(50 + x)

/* 100 ~ 149 is used by FIMC-IS */
#define EXYNOS_VIDEONODE_FIMC_IS		(100)

#endif /* __VIDEO_VIDEONODE_H */
