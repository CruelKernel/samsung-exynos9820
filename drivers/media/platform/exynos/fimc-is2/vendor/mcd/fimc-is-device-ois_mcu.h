/*
 * Samsung Exynos5 SoC series FIMC-IS OIS driver
 *
 * exynos5 fimc-is core functions
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef FIMC_IS_MCU_H
#define FIMC_IS_MCU_H

typedef struct {
  u32 ver;
  u32 id;
} sysboot_info_type;

typedef struct {
  u32 page;
  u32 count;
} sysboot_erase_param_type;

typedef enum {
	GPIO_PIN_RESET = 0,
	GPIO_PIN_SET
} GPIO_PinState;

/* Utility MACROs */

#ifndef NTOHL
#define NTOHL(x)                        ((((x) & 0xFF000000U) >> 24) | \
                                         (((x) & 0x00FF0000U) >>  8) | \
                                         (((x) & 0x0000FF00U) <<  8) | \
                                         (((x) & 0x000000FFU) << 24))
#endif
#ifndef HTONL
#define HTONL(x)                        NTOHL(x)
#endif

#ifndef NTOHS
#define NTOHS(x)                        (((x >> 8) & 0x00FF) | ((x << 8) & 0xFF00))
#endif
#ifndef HTONS
#define HTONS(x)                        NTOHS(x)
#endif

#define	FIMC_MCU_FW_NAME		"fimc_is_fw_mcu.bin"
#define	FW_TRANS_SIZE			256
#define	OIS_BIN_LEN				45056
#define	FW_RELEASE_YEAR		0
#define	FW_RELEASE_MONTH		1
#define	FW_RELEASE_COUNT		2

#define	FW_DRIVER_IC			0
#define	FW_GYRO_SENSOR			1
#define	FW_MODULE_TYPE		2
#define	FW_PROJECT				3
#define	FW_CORE_VERSION		4

#define	OIS_MEM_STATUS_RETRY	6
#define	POSITION_NUM			512
#define	AF_BOUNDARY			(1 << 6)

#define	BOOT_I2C_SYNC_RETRY_COUNT       (3)
#define	BOOT_I2C_SYNC_RETRY_INTVL       (50)
#define	BOOT_NRST_PULSE_INTVL           (2) /* msec */

#define	BOOT_I2C_CMD_GET                (0x00)
#define	BOOT_I2C_CMD_GET_VER            (0x01)
#define	BOOT_I2C_CMD_GET_ID             (0x02)
#define	BOOT_I2C_CMD_READ               (0x11)
#define	BOOT_I2C_CMD_WRITE              (0x31)
#define	BOOT_I2C_CMD_ERASE              (0x44)
#define	BOOT_I2C_CMD_GO                 (0x21)
#define	BOOT_I2C_CMD_WRITE_UNPROTECT    (0x73)
#define	BOOT_I2C_CMD_READ_UNPROTECT     (0x92)

#define	BOOT_I2C_RESP_ACK               (0x79)
#define	BOOT_I2C_RESP_NACK              (0x1F)
#define	BOOT_I2C_RESP_BUSY              (0x76)

#define	BOOT_I2C_RESP_GET_VER_LEN       (0x01) /* bootloader version(1) */
#define	BOOT_I2C_RESP_GET_ID_LEN        (0x03) /* number of bytes - 1(1) + product ID(2) */

/* Flash memory characteristics from target datasheet (msec unit) */
#define	FLASH_PROG_TIME 			37 /* per page or sector */
#define	FLASH_FULL_ERASE_TIME  		40 * 32 /* 2K erase time(40ms) * 32 pages */
#define	FLASH_PAGE_ERASE_TIME  		36 /* per page or sector */

#define	BOOT_I2C_CMD_TMOUT              (30)
#define	BOOT_I2C_WRITE_TMOUT            (FLASH_PROG_TIME)
#define	BOOT_I2C_FULL_ERASE_TMOUT       (FLASH_FULL_ERASE_TIME)
#define	BOOT_I2C_PAGE_ERASE_TMOUT(n)    (FLASH_PAGE_ERASE_TIME * n)
#define	BOOT_I2C_WAIT_RESP_TMOUT        (30)
#define	BOOT_I2C_WAIT_RESP_POLL_TMOUT   (500)
#define	BOOT_I2C_WAIT_RESP_POLL_INTVL   (3)
#define	BOOT_I2C_WAIT_RESP_POLL_RETRY   (BOOT_I2C_WAIT_RESP_POLL_TMOUT / BOOT_I2C_WAIT_RESP_POLL_INTVL)
#define	BOOT_I2C_XMIT_TMOUT(count)      (5 + (1 * count))
#define	BOOT_I2C_RECV_TMOUT(count)      BOOT_I2C_XMIT_TMOUT(count)

#define	BOOT_I2C_MAX_WRITE_LEN          (256)  /* Protocol limitation */
#define	BOOT_I2C_MAX_ERASE_PARAM_LEN    (4096) /* In case of erase parameter with 2048 pages */
#define	BOOT_I2C_MAX_PAYLOAD_LEN        (BOOT_I2C_MAX_ERASE_PARAM_LEN) /* Larger one between write and erase., */

#define	OIS_GYRO_SCALE_FACTOR_LSM6DSO	114

/* Memory map specific */
typedef struct {
	u32 size;
	u32 count;
} sysboot_page_type;

typedef struct {
	u32 flashbase;  /* flash memory starting address */
	u32 sysboot;    /* system memory starting address */
	u32 optionbyte; /* option byte starting address */
	sysboot_page_type *pages;
} sysboot_map_type;

enum{
	eBIG_ENDIAN = 0, // big endian
	eLIT_ENDIAN = 1  // little endian
};

enum fimc_is_ois_power_mode {
	OIS_POWER_MODE_SINGLE = 0,
	OIS_POWER_MODE_DUAL,
};

struct mcu_default_data {
	u32 ois_gyro_list[5];
	u32 ois_gyro_list_len;
	u32 aperture_delay_list[2];
	u32 aperture_delay_list_len;
};

#define	SWAP32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#define	RND_DIV(num, den) ((num > 0) ? (num+(den>>1))/den : (num-(den>>1))/den)
#define	SCALE				10000
#define	Coef_angle_max		3500		// unit : 1/SCALE, OIS Maximum compensation angle, 0.35*SCALE
#define	SH_THRES			798000		// unit : 1/SCALE, 39.9*SCALE
#define	Gyrocode			1000			// Gyro input code for 1 angle degree

void fimc_is_mcu_fw_update(struct fimc_is_core *core);
struct i2c_client *fimc_is_mcu_i2c_get_client(struct fimc_is_core *core);
bool fimc_is_mcu_halltest_aperture(struct v4l2_subdev *subdev, u16 *hall_value);
void fimc_is_mcu_set_aperture_onboot(struct fimc_is_core *core);

#endif /* FIMC_IS_MCU_H_ */
