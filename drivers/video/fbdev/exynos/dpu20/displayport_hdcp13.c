/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * Samsung SoC DisplayPort HDCP1.3 driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include "displayport.h"
#include "decon.h"

HDCP13 HDCP13_DPCD;
struct hdcp13_info hdcp13_info;

void hdcp13_dpcd_buffer(void)
{
	u8 i = 0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BKSV); i++)
		HDCP13_DPCD.HDCP13_BKSV[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_R0); i++)
		HDCP13_DPCD.HDCP13_R0[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_AKSV); i++)
		HDCP13_DPCD.HDCP13_AKSV[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_AN); i++)
		HDCP13_DPCD.HDCP13_AN[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_V_H); i++)
		HDCP13_DPCD.HDCP13_V_H[i] = 0x0;

	HDCP13_DPCD.HDCP13_BCAP[0] = 0x0;
	HDCP13_DPCD.HDCP13_BSTATUS[0] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BINFO); i++)
		HDCP13_DPCD.HDCP13_BINFO[i] = 0x0;

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_KSV_FIFO); i++)
		HDCP13_DPCD.HDCP13_KSV_FIFO[i] = 0x0;

	HDCP13_DPCD.HDCP13_AINFO[0] = 0x0;
}

void hdcp13_dump(char *str, u8 *buf, int size)
{
	int i;
	u8 *buffer = buf;

	displayport_dbg("[HDCP 1.3] %s = 0x", str);

	for (i = 0; i < size; i++)
		displayport_dbg("%02x ", *(buffer+i));

	displayport_dbg("\n");
}

void hdcp13_func_en(u32 en)
{
	displayport_write_mask(SYSTEM_HPD_CONTROL, en, HPD_EVENT_CTRL_EN);
	displayport_write_mask(SYSTEM_HPD_CONTROL, en, HPD_FORCE_EN);
	displayport_write_mask(SYSTEM_HPD_CONTROL, en, HPD_FORCE);
	displayport_write_mask(SYSTEM_COMMON_FUNCTION_ENABLE, en, HDCP13_FUNC_EN);
}

u8 hdcp13_read_bcap(void)
{
	u8 return_val = 0;
	u8 hdcp_capa = 0;

	displayport_reg_dpcd_read(ADDR_HDCP13_BCAP, 1, HDCP13_DPCD.HDCP13_BCAP);

	displayport_info("[HDCP 1.3] HDCP13_BCAP= 0x%x\n", HDCP13_DPCD.HDCP13_BCAP[0]);

	if (!(HDCP13_DPCD.HDCP13_BCAP[0] & BCAPS_RESERVED_BIT_MASK)) {
		hdcp13_info.is_repeater = (HDCP13_DPCD.HDCP13_BCAP[0] & BCAPS_REPEATER) >> 1;

		hdcp_capa = HDCP13_DPCD.HDCP13_BCAP[0] & BCAPS_HDCP_CAPABLE;

		if (hdcp_capa)
			return_val = 0;
		else
			return_val = -EINVAL;
	} else
		return_val = -EINVAL;

	return return_val;
}

void hdcp13_repeater_set(void)
{
	displayport_write_mask(HDCP13_CONTROL_0, hdcp13_info.is_repeater, SW_RX_REPEATER);
}

void hdcp13_write_bksv(void)
{
	int i;
	u32 val = 0;

	for (i = 0; i < 4; i++)
		val |= HDCP13_DPCD.HDCP13_BKSV[i] << (i * 8);

	displayport_write(HDCP13_BKSV_0, val);

	val = 0;
	val |= (u32)HDCP13_DPCD.HDCP13_BKSV[4];
	displayport_write(HDCP13_BKSV_1, val);
}

u8 hdcp13_read_bksv(void)
{
	u8 i = 0;
	u8 j = 0;
	int one = 0;
	u8 ret;

	displayport_reg_dpcd_read_burst(ADDR_HDCP13_BKSV, sizeof(HDCP13_DPCD.HDCP13_BKSV), HDCP13_DPCD.HDCP13_BKSV);

	hdcp13_dump("BKSV", &(HDCP13_DPCD.HDCP13_BKSV[0]), 5);

	for (i = 0; i < sizeof(HDCP13_DPCD.HDCP13_BKSV); i++) {
		for (j = 0; j < 8; j++) {
			if (HDCP13_DPCD.HDCP13_BKSV[i] & (0x1 << j))
				one++;
		}
	}

	if (one == 20) {
		hdcp13_write_bksv();

		displayport_dbg("[HDCP 1.3] Valid Bksv\n");
		ret = 0;
	} else {
		displayport_info("[HDCP 1.3] Invalid Bksv\n");
		ret = -EINVAL;
	}

	return ret;
}

void hdcp13_set_an_val(void)
{
	displayport_write_mask(HDCP13_CONTROL_0, 1, SW_STORE_AN);

	displayport_write_mask(HDCP13_CONTROL_0, 0, SW_STORE_AN);
}

void hdcp13_write_an_val(void)
{
	u8 i = 0;
	u32 val = 0;

	hdcp13_set_an_val();

	val = displayport_read(HDCP13_AN_0);
	for (i = 0; i < 4; i++)
		HDCP13_DPCD.HDCP13_AN[i] = (u8)((val >> (i * 8)) & 0xFF);

	val = displayport_read(HDCP13_AN_1);
	for (i = 0; i < 4; i++)
		HDCP13_DPCD.HDCP13_AN[i + 4] = (u8)((val >> (i * 8)) & 0xFF);

	displayport_reg_dpcd_write_burst(ADDR_HDCP13_AN, 8, HDCP13_DPCD.HDCP13_AN);
}

u8 hdcp13_write_aksv(void)
{
	u8 i = 0;
	u32 val = 0;
	u8 ret;

	if (displayport_read_mask(HDCP13_STATUS, AKSV_VALID)) {
		val = displayport_read(HDCP13_AKSV_0);
		for (i = 0; i < 4; i++)
			HDCP13_DPCD.HDCP13_AKSV[i] = (u8)((val >> (i * 8)) & 0xFF);

		val = displayport_read(HDCP13_AKSV_1);
		HDCP13_DPCD.HDCP13_AKSV[i] = (u8)(val & 0xFF);

		hdcp13_info.cp_irq_flag = 0;
		displayport_reg_dpcd_write_burst(ADDR_HDCP13_AKSV, 5, HDCP13_DPCD.HDCP13_AKSV);
		displayport_dbg("[HDCP 1.3] Valid Aksv\n");

		ret = 0;
	} else {
		displayport_info("[HDCP 1.3] Invalid Aksv\n");
		ret = -EINVAL;
	}

	return ret;
}

u8 hdcp13_cmp_ri(void)
{
	u8 cnt = 0;
	u8 ri_retry_cnt = 0;
	u8 ri[2];
	u8 ret = 0;
	u32 val = 0;

	cnt = 0;
	while (hdcp13_info.cp_irq_flag != 1 && cnt < RI_WAIT_COUNT) {
		mdelay(RI_AVAILABLE_WAITING);
		cnt++;
	}

	if (cnt >= RI_WAIT_COUNT) {
		displayport_info("[HDCP 1.3] Don't receive CP_IRQ interrupt\n");
		ret = -EFAULT;
	}

	hdcp13_info.cp_irq_flag = 0;

	cnt = 0;
	while ((HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_R0_AVAILABLE) == 0 && cnt < RI_WAIT_COUNT) {
		/* R0 Sink Available check */
		displayport_reg_dpcd_read(ADDR_HDCP13_BSTATUS, 1, HDCP13_DPCD.HDCP13_BSTATUS);

		mdelay(RI_AVAILABLE_WAITING);
		cnt++;
	}

	if (cnt >= RI_WAIT_COUNT) {
		displayport_info("[HDCP 1.3] R0 not available in RX part\n");
		ret = -EFAULT;
	}

	while (ri_retry_cnt < RI_READ_RETRY_CNT) {
		/* Read R0 from Sink */
		displayport_reg_dpcd_read_burst(ADDR_HDCP13_R0, sizeof(HDCP13_DPCD.HDCP13_R0), HDCP13_DPCD.HDCP13_R0);

		/* Read R0 from Source */
		val = displayport_read(HDCP13_R0_REG);
		ri[0] = (u8)(val & 0xFF);
		ri[1] = (u8)((val >> 8) & 0xFF);

		ri_retry_cnt++;

		if ((ri[0] == HDCP13_DPCD.HDCP13_R0[0]) && (ri[1] == HDCP13_DPCD.HDCP13_R0[1])) {
			displayport_dbg("[HDCP 1.3] Ri_Tx(0x%02x%02x) == Ri_Rx(0x%02x%02x)\n",
					ri[1], ri[0], HDCP13_DPCD.HDCP13_R0[1], HDCP13_DPCD.HDCP13_R0[0]);

			ret = 0;
			break;
		}

		displayport_info("[HDCP 1.3] Ri_Tx(0x%02x%02x) != Ri_Rx(0x%02x%02x)\n",
				ri[1], ri[0], HDCP13_DPCD.HDCP13_R0[1], HDCP13_DPCD.HDCP13_R0[0]);

		mdelay(RI_DELAY);
		ret = -EFAULT;
	}

	return ret;
}

void hdcp13_encryption_con(u8 enable)
{
	struct decon_device *decon = get_decon_drvdata(2);

	/* wait 2 frames for hdcp encryption enable/disable */
	decon_wait_for_vsync(decon, VSYNC_TIMEOUT_MSEC);
	decon_wait_for_vsync(decon, VSYNC_TIMEOUT_MSEC);

	if (enable == 1) {
		displayport_write_mask(HDCP13_CONTROL_0, ~0, SW_AUTH_OK | HDCP13_ENC_EN);
		/*displayport_reg_video_mute(0);*/
		displayport_info("[HDCP 1.3] HDCP13 Encryption Enable\n");
	} else {
		/*displayport_reg_video_mute(1);*/
		displayport_write_mask(HDCP13_CONTROL_0, 0, SW_AUTH_OK | HDCP13_ENC_EN);
		displayport_info("[HDCP 1.3] HDCP13 Encryption Disable\n");
	}
}

void hdcp13_link_integrity_check(void)
{
	int i;
	if (hdcp13_info.link_check == LINK_CHECK_NEED) {
		displayport_info("[HDCP 1.3] HDCP13_Link_integrity_check\n");

		for (i = 0; i < 10; i++) {
			displayport_reg_dpcd_read(ADDR_HDCP13_BSTATUS, 1,
					HDCP13_DPCD.HDCP13_BSTATUS);
			if ((HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_REAUTH_REQ) ||
				(HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_LINK_INTEGRITY_FAIL)) {

				displayport_info("[HDCP 1.3] HDCP13_DPCD.HDCP13_BSTATUS = %02x : retry(%d)\n",
						HDCP13_DPCD.HDCP13_BSTATUS[0], i);
				hdcp13_info.link_check = LINK_CHECK_FAIL;
				hdcp13_dpcd_buffer();
				hdcp13_info.auth_state = HDCP13_STATE_FAIL;
				displayport_reg_video_mute(1);
				hdcp13_info.cp_irq_flag = 0;

				if (hdcp13_read_bcap() != 0) {
					displayport_info("[HDCP 1.3] NOT HDCP CAPABLE\n");
					hdcp13_encryption_con(0);
				} else {
					displayport_info("[HDCP 1.3] ReAuth\n");
					hdcp13_run();
				}
				break;
			}
			msleep(20);
		}
	}
}

void hdcp13_irq_mask(void)
{
	displayport_reg_set_interrupt_mask(HDCP_LINK_CHECK_INT_MASK, 1);
	displayport_reg_set_interrupt_mask(HDCP_LINK_FAIL_INT_MASK, 1);
}

void hdcp13_make_sha1_input_buf(u8 *sha1_input_buf, u8 *binfo, u8 device_cnt)
{
	int i = 0;
	u32 val = 0;

	for (i = 0; i < BINFO_SIZE; i++)
		sha1_input_buf[KSV_SIZE * device_cnt + i] = binfo[i];

	val = displayport_read(HDCP13_AM0_0);
	for (i = 0; i < 4; i++)
		sha1_input_buf[KSV_SIZE * device_cnt + BINFO_SIZE + i] =
			(u8)((val >> (i * 8)) & 0xFF);

	val = displayport_read(HDCP13_AM0_1);
	for (i = 0; i < 4; i++)
		sha1_input_buf[KSV_SIZE * device_cnt + BINFO_SIZE + i + 4] =
			(u8)((val >> (i * 8)) & 0xFF);
}

void hdcp13_v_value_order_swap(u8 *v_value)
{
	int i;
	u8 temp;

	for (i = 0; i < SHA1_SIZE; i += 4) {
		temp = v_value[i];
		v_value[i] = v_value[i + 3];
		v_value[i + 3] = temp;
		temp = v_value[i + 1];
		v_value[i + 1] = v_value[i + 2];
		v_value[i + 2] = temp;
	}
}

int hdcp13_compare_v(u8 *tx_v_value)
{
	int i = 0;
	int ret = 0;
	u8 v_read_retry_cnt = 0;

	while(v_read_retry_cnt < V_READ_RETRY_CNT) {
		ret = 0;

		displayport_reg_dpcd_read_burst(ADDR_HDCP13_V_H0, SHA1_SIZE, HDCP13_DPCD.HDCP13_V_H);

		v_read_retry_cnt++;

		for (i = 0; i < SHA1_SIZE; i++) {
			if (tx_v_value[i] != HDCP13_DPCD.HDCP13_V_H[i])
				ret = -EFAULT;
		}

		if (ret == 0)
			break;
	}

	return ret;
}

static int hdcp13_proceed_repeater(void)
{
	int retry_cnt = HDCP_RETRY_COUNT;
	int cnt = 0;
	int i;
	u32 b_info = 0;
	u8 device_cnt = 0;
	u8 offset = 0;
	int ksv_read_size = 0;
	u8 sha1_input_buf[KSV_SIZE * MAX_KSV_LIST_COUNT + BINFO_SIZE + M0_SIZE];
	u8 v_value[SHA1_SIZE];

	displayport_info("[HDCP 1.3] HDCP repeater Start!!!\n");

	while (hdcp13_info.cp_irq_flag != 1 && cnt < RI_WAIT_COUNT) {
		mdelay(RI_AVAILABLE_WAITING);
		cnt++;
	}

	if (cnt >= RI_WAIT_COUNT)
		displayport_dbg("[HDCP 1.3] Don't receive CP_IRQ interrupt\n");

	hdcp13_info.cp_irq_flag = 0;

	cnt = 0;
	while ((HDCP13_DPCD.HDCP13_BSTATUS[0] & BSTATUS_READY) == 0) {
		displayport_reg_dpcd_read(ADDR_HDCP13_BSTATUS, 1,
				HDCP13_DPCD.HDCP13_BSTATUS);

		mdelay(RI_AVAILABLE_WAITING);
		cnt++;

		if (cnt > REPEATER_READY_WAIT_COUNT || !displayport_get_hpd_state()) {
			displayport_info("[HDCP 1.3] Not repeater ready in RX part\n");
			hdcp13_info.auth_state = HDCP13_STATE_FAIL;
			goto repeater_err;
		}
	}

	displayport_dbg("[HDCP 1.3] HDCP RX repeater ready!!!\n");

	while ((hdcp13_info.auth_state != HDCP13_STATE_SECOND_AUTH_DONE) &&
			(retry_cnt != 0)) {
		retry_cnt--;

		displayport_reg_dpcd_read(ADDR_HDCP13_BINFO, 2, HDCP13_DPCD.HDCP13_BINFO);

		for (i = 0; i < 2; i++)
			b_info |= (u32)HDCP13_DPCD.HDCP13_BINFO[i] << (i * 8);

		displayport_dbg("[HDCP 1.3] b_info = 0x%x\n", b_info);

		if ((b_info & BINFO_MAX_DEVS_EXCEEDED)
				|| (b_info & BINFO_MAX_CASCADE_EXCEEDED)) {
			hdcp13_info.auth_state = HDCP13_STATE_FAIL;
			displayport_info("[HDCP 1.3] MAXDEVS or CASCADE EXCEEDED!\n");
			goto repeater_err;
		}

		device_cnt = b_info & BINFO_DEVICE_COUNT;

		if (device_cnt != 0) {
			displayport_info("[HDCP 1.3] device count = %d\n", device_cnt);

			offset = 0;

			while (device_cnt > offset) {
				ksv_read_size = (device_cnt - offset) * KSV_SIZE;

				if (ksv_read_size >= KSV_FIFO_SIZE)
					ksv_read_size = KSV_FIFO_SIZE;

				displayport_reg_dpcd_read(ADDR_HDCP13_KSV_FIFO,
						ksv_read_size, HDCP13_DPCD.HDCP13_KSV_FIFO);

				for (i = 0; i < ksv_read_size; i++)
					sha1_input_buf[i + offset * KSV_SIZE] =
						HDCP13_DPCD.HDCP13_KSV_FIFO[i];

				offset += KSV_FIFO_SIZE / KSV_SIZE;
			}
		}

		/* need calculation of V = SHA-1(KSV list || Binfo || M0) */
		hdcp13_make_sha1_input_buf(sha1_input_buf, HDCP13_DPCD.HDCP13_BINFO, device_cnt);
#if defined(CONFIG_EXYNOS_HDCP2)
		hdcp_calc_sha1(v_value, sha1_input_buf, BINFO_SIZE + M0_SIZE + KSV_SIZE * device_cnt);
#else
		displayport_info("Not compiled EXYNOS_HDCP2 driver\n");
#endif
		hdcp13_v_value_order_swap(v_value);

		if (hdcp13_compare_v(v_value) == 0) {
			hdcp13_info.auth_state = HDCP13_STATE_SECOND_AUTH_DONE;
			displayport_reg_video_mute(0);
			displayport_info("[HDCP 1.3] 2nd Auth done!!!\n");
			return 0;
		}

		hdcp13_info.auth_state = HDCP13_STATE_AUTH_PROCESS;
		displayport_info("[HDCP 1.3] 2nd Auth fail!!!\n");
	}

repeater_err:
	return -EINVAL;
}

void hdcp13_run(void)
{
	int retry_cnt = HDCP_RETRY_COUNT;

	while ((hdcp13_info.auth_state != HDCP13_STATE_AUTHENTICATED)
			&& (hdcp13_info.auth_state != HDCP13_STATE_SECOND_AUTH_DONE)
			&& (retry_cnt != 0)) {
		retry_cnt--;

		hdcp13_info.auth_state = HDCP13_STATE_AUTH_PROCESS;

		hdcp13_encryption_con(0);
		hdcp13_func_en(1);

		hdcp13_repeater_set();

		displayport_dbg("[HDCP 1.3] SW Auth.\n");

		if (hdcp13_read_bksv() != 0) {
			displayport_info("[HDCP 1.3] ReAuthentication Start!!!\n");
			continue;
		}

		hdcp13_write_an_val();

		if (hdcp13_write_aksv() != 0) {
			displayport_info("[HDCP 1.3] ReAuthentication Start!!!\n");
			continue;
		}

		/* BKSV Rewrite */
		hdcp13_write_bksv();

		if (hdcp13_cmp_ri() != 0)
			continue;

		if (!hdcp13_info.is_repeater) {
			hdcp13_info.auth_state = HDCP13_STATE_AUTHENTICATED;
			displayport_reg_video_mute(0);
		}

		hdcp13_encryption_con(1);
		displayport_dbg("[HDCP 1.3] HDCP 1st Authentication done!!!\n");

		if (hdcp13_info.is_repeater) {
			if (hdcp13_proceed_repeater())
				goto HDCP13_END;
			else
				continue;
		}
	}

HDCP13_END:
	if ((hdcp13_info.auth_state != HDCP13_STATE_AUTHENTICATED) &&
			(hdcp13_info.auth_state != HDCP13_STATE_SECOND_AUTH_DONE)) {
		hdcp13_info.auth_state = HDCP13_STATE_FAIL;
		displayport_reg_video_mute(1);
		hdcp13_encryption_con(0);
		displayport_dbg("[HDCP 1.3] HDCP Authentication fail!!!\n");
	}
}
