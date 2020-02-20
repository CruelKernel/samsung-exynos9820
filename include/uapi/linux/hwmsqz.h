/* include/uapi/linux/hwmmsqz.h
 *
 * Userspace API for Samsung MMSQZ driver
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Author: Jungik Seo <jungik.seo@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __HWMMSQZ_H__
#define __HWMMSQZ_H__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <hwjsqz.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

/*
 * NOTE: THESE ENUMS ARE ALSO USED BY USERSPACE TO SPECIFY SETTINGS AND THEY'RE
 *       SUPPOSED TO WORK WITH FUTURE HARDWARE AS WELL (WHICH MAY OR MAY NOT
 *       SUPPORT ALL THE SETTINGS.
 *
 *       FOR THAT REASON, YOU MUST NOT CHANGE THE ORDER OF VALUES,
 *       AND NEW VALUES MUST BE ADDED AT THE END OF THE ENUMS TO AVOID
 *       API BREAKAGES.
 */


/**
 * @brief the video size
 */
struct hwMSQZ_video_info {
	__u32 width;					/**< width of the image */
	__u32 height;					/**< height of the image */
	__u32 stride;					/**<: stride of the image */
};

/**
 * @brief the gyro data
 */
struct hwMSQZ_velocity {
	__u32 vel_x;   /**< Gyro X data */
	__u32 vel_y;   /**< Gyro Y data */
};

/**
 * @brief the tune data of DC
 */
struct hwMSQZ_tune_dc {
	unsigned short base;    /**< Gyro X data */
	unsigned char reduce;   /**< Gyro X data */
	unsigned char wshift;   /**< Gyro X data */
};

/**
 * @brief the tune data of Alpha
 */
struct hwMSQZ_tune_alpha {
    short jnd;
    short dqp;
};

/**
 * @brief the tune data of DQP
 */
struct hwMSQZ_tune_dqp {
    short dqp_max;
    short dqp_min;
};


/**
 * @brief the task information to be sent to the driver
 *
 * This is the struct holding all the task information that the driver needs
 * to process a task.
 *
 * The client needs to fill it in with the necessary information and send it
 * to the driver through the IOCTL.
 */
struct hwMSQZ_task {
    struct hwMSQZ_video_info video_info;   /**< info related to the video sent to the HW for processing */
    struct hwMSQZ_velocity vel_info;		/**< info related to gyro data sent to the HW for processing */
    struct hwMSQZ_tune_dc dc_tune;			/**< info related to tuning dc value sent to the HW for processing */
    struct hwMSQZ_tune_alpha alpha_tune;	/**< info related to tuning alpha value sent to the HW for processing */
    struct hwMSQZ_tune_dqp dqp_tune;		/**< info related to tuning dqp value sent to the HW for processing */
	struct hwSQZ_buffer buf_in;           	/**< info related to the input buffer sent to the HW for processing */
	struct hwSQZ_buffer buf_out;          	/**< info related to the output buffer sent to the HW for processing */
	int frame_dqp;							/**< frame average dqp */
};


#define HWMSQZ_IOC_PROCESS	_IOWR('M', 1, struct hwMMSQZ_task)


#endif // __HWMSQZ_H__

