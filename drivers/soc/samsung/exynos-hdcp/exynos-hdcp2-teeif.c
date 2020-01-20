/*
 * drivers/soc/samsung/exynos-hdcp/exynos-hdcp2-teeif.c
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/smc.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

#include "exynos-hdcp2-teeif.h"
#include "exynos-hdcp2.h"
#include "exynos-hdcp2-log.h"

extern void __dma_inv_area(const void *start, size_t size);

static struct hci_ctx hctx = {
	.msg = NULL,
	.state = HCI_DISCONNECTED
};

int hdcp_tee_open(void)
{
	int ret = 0;
	u64 phys_addr = 0;

	if (hctx.state == HCI_CONNECTED) {
		hdcp_info("HCI is already connected\n");
		return 0;
	}

	/* Allocate WSM for HDCP commnad interface */
	hctx.msg = (struct hci_message *)kzalloc(sizeof(struct hci_message), GFP_KERNEL);
	if (!hctx.msg) {
		hdcp_err("alloc wsm for HDCP SWd is failed\n");
		return -ENOMEM;
	}

	/* send WSM address to SWd */
	phys_addr = virt_to_phys((void *)hctx.msg);
	ret = exynos_smc(SMC_HDCP_INIT, phys_addr, sizeof(struct hci_message), 0);
	if (ret) {
		hdcp_err("Fail to set up connection with SWd. ret(%d)\n", ret);
		kfree(hctx.msg);
		hctx.msg = NULL;
		return -ECONNREFUSED;
	}

	hctx.state = HCI_CONNECTED;

	return 0;
}

int hdcp_tee_close()
{
	int ret;

	if (hctx.state == HCI_DISCONNECTED) {
		hdcp_info("HCI is already disconnected\n");
		return 0;
	}

	if (hctx.msg) {
		kfree(hctx.msg);
		hctx.msg = NULL;
	}

	/* send terminate command to SWd */
	ret = exynos_smc(SMC_HDCP_TERMINATE, 0, 0, 0);
	if (ret) {
		hdcp_err("HDCP: Fail to set up connection with SWd. ret(%d)\n", ret);
		return -EFAULT;
	}

	hctx.state = HCI_DISCONNECTED;

	return 0;
}

int hdcp_tee_comm(struct hci_message *hci)
{
	int ret;

	if (!hci)
		return -EINVAL;

	/**
	 * kernel & TEE does not share cache.
	 * So, cache should be flushed before sending data to TEE
	 * and invalidate after come back to kernel
	 */
	__flush_dcache_area((void *)hci, sizeof(struct hci_message));
	ret = exynos_smc(SMC_HDCP_PROT_MSG, 0, 0, 0);
	__inval_dcache_area((void *)hci, sizeof(struct hci_message));

	if (ret) {
		hdcp_info("SWd returned(%x)\n", ret);
		return ret;
	}

	return 0;
}

int teei_gen_rtx(uint32_t lk_type,
		uint8_t *rtx, size_t rtx_len,
		uint8_t *caps, uint32_t caps_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GEN_RTX;
	hci->genrtx.lk_type = lk_type;
	hci->genrtx.len = rtx_len;

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* check returned message from SWD */
	if (rtx && rtx_len)
		memcpy(rtx, hci->genrtx.rtx, rtx_len);
	if (caps && caps_len)
		memcpy(caps, hci->genrtx.tx_caps, caps_len);

	return ret;
}

int teei_get_txinfo(uint8_t *version, size_t version_len,
		uint8_t *caps_mask, uint32_t caps_mask_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GET_TXINFO;

	/* send command to swd */
	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* check returned message from SWD */
	memcpy(version, hci->gettxinfo.version, version_len);
	memcpy(caps_mask, hci->gettxinfo.caps_mask, caps_mask_len);

	return ret;
}

int teei_set_rxinfo(uint8_t *version, size_t version_len,
		uint8_t *caps_mask, uint32_t caps_mask_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_SET_RXINFO;
	memcpy(hci->setrxinfo.version, version, version_len);
	memcpy(hci->setrxinfo.caps_mask, caps_mask, caps_mask_len);

	/* send command to swd */
	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	return ret;
}

int teei_verify_cert(uint8_t *cert, size_t cert_len,
		uint8_t *rrx, size_t rrx_len,
		uint8_t *rx_caps, size_t rx_caps_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_VERIFY_CERT;
	hci->vfcert.len = cert_len;
	memcpy(hci->vfcert.cert, cert, cert_len);
	if (rrx && rrx_len)
		memcpy(hci->vfcert.rrx, rrx, rrx_len);
	if (rx_caps && rx_caps_len)
		memcpy(hci->vfcert.rx_caps, rx_caps, rx_caps_len);

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* return verification result */
	return ret;
}

int teei_generate_master_key(uint32_t lk_type, uint8_t *emkey, size_t emkey_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GEN_MKEY;
	hci->genmkey.lk_type = lk_type;
	hci->genmkey.emkey_len = emkey_len;

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* copy encrypted mkey & wrapped mkey to hdcp ctx */
	memcpy(emkey, hci->genmkey.emkey, hci->genmkey.emkey_len);

	/* check returned message from SWD */

	return 0;
}

int teei_set_rrx(uint8_t *rrx, size_t rrx_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_SET_RRX;
	memcpy(hci->setrrx.rrx, rrx, rrx_len);

	/* send command to swd */
	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	return ret;
}

int teei_compare_ake_hmac(uint8_t *rx_hmac, size_t rx_hmac_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_COMPARE_AKE_HMAC;
	hci->comphmac.rx_hmac_len = rx_hmac_len;
	memcpy(hci->comphmac.rx_hmac, rx_hmac, rx_hmac_len);

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	return ret;
}

int teei_set_pairing_info(uint8_t *ekh_mkey, size_t ekh_mkey_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_SET_PAIRING_INFO;
	memcpy(hci->setpairing.ekh_mkey, ekh_mkey, ekh_mkey_len);

	/* send command to swd */
	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	return ret;
}

int teei_get_pairing_info(uint8_t *ekh_mkey, size_t ekh_mkey_len,
			uint8_t *m, size_t m_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GET_PAIRING_INFO;

	/* send command to swd */
	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	memcpy(ekh_mkey, hci->getpairing.ekh_mkey, ekh_mkey_len);
	memcpy(m, hci->getpairing.m, m_len);

	return ret;
}

int teei_gen_rn(uint8_t *out, size_t len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GEN_RN;
	hci->genrn.len = len;

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* check returned message from SWD */

	memcpy(out, hci->genrn.rn, len);

	return ret;
}

int teei_gen_lc_hmac(uint32_t lk_type, uint8_t *lsb16_hmac)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GEN_LC_HMAC;
	hci->genlchmac.lk_type = lk_type;
	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* Send LSB 16bytes to Rx */
	if (lsb16_hmac)
		memcpy(lsb16_hmac, hci->genlchmac.lsb16_hmac, (HDCP_HMAC_SHA256_LEN / 2));

	/* todo: check returned message from SWD */

	return 0;
}

int teei_compare_lc_hmac(uint8_t *rx_hmac, size_t rx_hmac_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_COMPARE_LC_HMAC;
	hci->complchmac.rx_hmac_len = rx_hmac_len;
	memcpy(hci->complchmac.rx_hmac, rx_hmac, rx_hmac_len);

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	return ret;
}

int teei_generate_riv(uint8_t *out, size_t len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GEN_RIV;
	hci->genriv.len = len;

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	memcpy(out, hci->genriv.riv, len);

	/* todo:  check returned message from SWD */

	return ret;
}

int teei_generate_skey(uint32_t lk_type,
		uint8_t *eskey, size_t eskey_len,
		int share_skey)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_GEN_SKEY;
	hci->genskey.lk_type = lk_type;
	hci->genskey.eskey_len = eskey_len;
	hci->genskey.share_skey = share_skey;

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	/* copy encrypted mkey & wrapped mkey to hdcp ctx */
	memcpy(eskey, hci->genskey.eskey, hci->genskey.eskey_len);

	/* todo: check returned message from SWD */

	return 0;
}

int teei_encrypt_packet(uint8_t *input, size_t input_len,
			uint8_t *output, size_t output_len,
			uint8_t *str_ctr, size_t str_ctr_len,
			uint8_t *input_ctr, size_t input_ctr_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */
	hci->cmd_id = HDCP_TEEI_ENC_PACKET;
	hci->encpacket.input_len = input_len;
	hci->encpacket.output_len = output_len;
	hci->encpacket.input_addr = (uint64_t)input;
	hci->encpacket.output_addr = (uint64_t)output;
	memcpy(hci->encpacket.str_ctr, str_ctr, str_ctr_len);
	memcpy(hci->encpacket.input_ctr, input_ctr, input_ctr_len);

	hdcp_debug("teeif command(%x)\n", hci->cmd_id);
	hdcp_debug("input(%llx), output(%llx)\n", hci->encpacket.input_addr, hci->encpacket.output_addr);

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	return 0;
}

int teei_set_rcvlist_info(uint8_t *rx_info,
		uint8_t *seq_num_v,
		uint8_t *v_prime,
		uint8_t *rcvid_list,
		uint8_t *v,
		uint8_t *valid)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	hci->cmd_id = HDCP_TEEI_SET_RCV_ID_LIST;

	memcpy(hci->setrcvlist.rcvid_lst, rcvid_list, HDCP_RP_RCVID_LIST_LEN);
	memcpy(hci->setrcvlist.v_prime, v_prime, HDCP_RP_HMAC_V_LEN / 2);
	/* Only used DP */
	if (rx_info != NULL && seq_num_v != NULL) {
		memcpy(hci->setrcvlist.rx_info, rx_info, HDCP_RP_RX_INFO_LEN);
		memcpy(hci->setrcvlist.seq_num_v, seq_num_v, HDCP_RP_SEQ_NUM_V_LEN);
	}


	ret = hdcp_tee_comm(hci);
	if (ret != 0) {
		*valid = 1;
		return ret;
	}
	memcpy(v, hci->setrcvlist.v, HDCP_RP_HMAC_V_LEN / 2);
	*valid = 0;

	return 0;
}

int teei_gen_stream_manage(uint16_t stream_num,
		uint8_t *streamid,
		uint8_t *seq_num_m,
		uint8_t *k,
		uint8_t *streamid_type)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;
	/* todo: input check */

	/* Update TCI buffer */

	hci->cmd_id = HDCP_TEEI_GEN_STREAM_MANAGE;
	hci->genstrminfo.stream_num = stream_num;
	memcpy(hci->genstrminfo.streamid, streamid, sizeof(uint8_t) * stream_num);

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	memcpy(seq_num_m, hci->genstrminfo.seq_num_m, HDCP_RP_SEQ_NUM_M_LEN);
	memcpy(k, hci->genstrminfo.k, HDCP_RP_K_LEN);
	memcpy(streamid_type, hci->genstrminfo.streamid_type, HDCP_RP_STREAMID_TYPE_LEN);

	/* check returned message from SWD */

	/* return verification result */
	return ret;
}

int teei_verify_m_prime(uint8_t *m_prime, uint8_t *input, size_t input_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	hci->cmd_id = HDCP_TEEI_VERIFY_M_PRIME;
	memcpy(hci->verifymprime.m_prime, m_prime, HDCP_RP_HMAC_M_LEN);
	if (input && input_len < sizeof(hci->verifymprime.strmsg)) {
		memcpy(hci->verifymprime.strmsg, input, input_len);
		hci->verifymprime.str_len = input_len;
	}

	ret = hdcp_tee_comm(hci);

	return ret;
}

int teei_wrapped_key(uint8_t *key, uint32_t wrapped, uint32_t key_len)
{
	int ret = 0;
	struct hci_message *hci = hctx.msg;

	hci->cmd_id = HDCP_TEEI_WRAP_KEY;

	if (key_len > (sizeof(hci->wrap_key.enc_key) - HDCP_WRAP_AUTH_TAG)) {
		hdcp_err("(un)wraaping key size is wrong. 0x%x\n", key_len);
		return HDCP_ERROR_WRONG_SIZE;
	}

	if (key_len < HDCP_WRAP_AUTH_TAG || key_len > HDCP_WRAP_MAX_SIZE - HDCP_WRAP_AUTH_TAG)
		return HDCP_ERROR_WRONG_SIZE;

	if (wrapped == UNWRAP)
		memcpy(hci->wrap_key.enc_key, key, key_len + HDCP_WRAP_AUTH_TAG);
	else
		memcpy(hci->wrap_key.key, key, key_len);

	hci->wrap_key.wrapped = wrapped;
	hci->wrap_key.key_len = key_len;

	if ((ret = hdcp_tee_comm(hci)) < 0)
		return ret;

	if (hci->wrap_key.wrapped == WRAP) {
		memcpy(key, hci->wrap_key.enc_key, key_len + HDCP_WRAP_AUTH_TAG);
	}

	return ret;
}
