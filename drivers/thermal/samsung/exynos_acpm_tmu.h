/*
 * exynos_acpm_tmu.h - ACPM TMU plugin interface
 *
 * Copyright (C) 2017 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __EXYNOS_ACPM_TMU_H__
#define __EXYNOS_ACPM_TMU_H__

/* Return values */
#define RET_SUCCESS		0
#define RET_FAIL		-1
#define RET_OK			RET_SUCCESS
#define RET_NOK			RET_FAIL

/* Return values - error types (minus) */
#define ERR_REQ_TYPE		2
#define ERR_TZ_ID		3
#define ERR_TEMP		4
#define ERR_APM_IRQ		5
#define ERR_APM_DIVIDER		6

/* Return values - capabilities */
#define CAP_APM_IRQ		0x1
#define CAP_APM_DIVIDER		0x2

/* IPC Request Types */
#define TMU_IPC_INIT		0x01
#define TMU_IPC_READ_TEMP	0x02
#define	TMU_IPC_AP_SUSPEND	0x04
#define	TMU_IPC_CP_CALL		0x08
#define	TMU_IPC_AP_RESUME	0x10

/*
 * 16-byte TMU IPC message format (REQ)
 *  (MSB)    3          2          1          0
 * ---------------------------------------------
 * |        fw_use       |         ctx         |
 * ---------------------------------------------
 * |          | tzid     |          | type     |
 * ---------------------------------------------
 * |          |          |          |          |
 * ---------------------------------------------
 * |          |          |          |          |
 * ---------------------------------------------
 */
struct tmu_ipc_request {
	u16 ctx;	/* LSB */
	u16 fw_use;	/* MSB */
	u8 type;
	u8 rsvd;
	u8 tzid;
	u8 rsvd2;
	u32 reserved;
	u32 reserved2;
};

/*
 * 16-byte TMU IPC message format (RESP)
 *  (MSB)    3          2          1          0
 * ---------------------------------------------
 * |        fw_use       |         ctx         |
 * ---------------------------------------------
 * | temp     |  tz_id   | ret      | type     |
 * ---------------------------------------------
 * |          |          |          | stat     |
 * ---------------------------------------------
 * |          |          |          |          |
 * ---------------------------------------------
 */
struct tmu_ipc_response {
	u16 ctx;	/* LSB */
	u16 fw_use;	/* MSB */
	u8 type;
	s8 ret;
	u8 tzid;
	u8 temp;
	u8 stat;
	u8 rsvd;
	u8 rsvd2;
	u8 rsvd3;
	u32 reserved;
};

union tmu_ipc_message {
	u32 data[4];
	struct tmu_ipc_request req;
	struct tmu_ipc_response resp;
};

struct acpm_tmu_cap {
	bool acpm_irq;
	bool acpm_divider;
};

int exynos_acpm_tmu_set_init(struct acpm_tmu_cap *cap);
int exynos_acpm_tmu_set_read_temp(int tz, int *temp, int *stat);
int exynos_acpm_tmu_set_suspend(int flag);
int exynos_acpm_tmu_set_cp_call(void);
int exynos_acpm_tmu_set_resume(void);
int exynos_acpm_tmu_ipc_dump(int no, unsigned int dump[]);
bool exynos_acpm_tmu_is_test_mode(void);
void exynos_acpm_tmu_set_test_mode(bool mode);
void exynos_acpm_tmu_log(bool mode);

#endif /* __EXYNOS_ACPM_TMU_H__ */
