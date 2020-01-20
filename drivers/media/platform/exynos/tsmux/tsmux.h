/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * Header file for Exynos TSMUX driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef TSMUX_H
#define TSMUX_H

#define TSMUX_VIDEO_PID			0x1011
#define TSMUX_AUDIO_PID			0x1100

#define TSMUX_AVC_STREAM_TYPE		0x1b
#define TSMUX_AVC_STREAM_ID		0xe0

#define TSMUX_AAC_STREAM_TYPE		0x0f
#define TSMUX_AAC_STREAM_ID		0xc0

#define TSMUX_MAX_CMD_QUEUE_NUM		4
#define TSMUX_MAX_M2M_CMD_QUEUE_NUM	3
#define TSMUX_MAX_CONTEXTS_NUM		1
#define TSMUX_OUT_BUF_CNT			4

#define TSMUX_PSI_SIZE			64
#define TSMUX_PES_HDR_SIZE		16
#define TSMUX_TS_HDR_SIZE		4
#define TSMUX_RTP_HDR_SIZE		8

struct tsmux_pkt_ctrl {
	int32_t psi_en;
	int32_t rtp_size;
	int32_t rtp_seq_override;
	int32_t pes_stuffing_num;
	int32_t mode;
	int32_t id;
};

struct tsmux_pes_hdr {
	int32_t code;
	int32_t stream_id;
	int32_t pkt_len;
	int32_t marker;
	int32_t scramble;
	int32_t priority;
	int32_t alignment;
	int32_t copyright;
	int32_t original;
	int32_t flags;
	int32_t hdr_len;
	int32_t pts39_16;
	int32_t pts15_0;
};

struct tsmux_ts_hdr {
	int32_t sync;
	int32_t error;
	int32_t priority;
	int32_t pid;
	int32_t scramble;
	int32_t adapt_ctrl;
	int32_t continuity_counter;
};

struct tsmux_rtp_hdr {
	int32_t ver;
	int32_t pad;
	int32_t ext;
	int32_t csrc_cnt;
	int32_t marker;
	int32_t pl_type;
	int32_t seq;
	int32_t ssrc;
};

struct tsmux_swp_ctrl {
	int32_t swap_ctrl_out4;
	int32_t swap_ctrl_out1;
	int32_t swap_ctrl_in4;
	int32_t swap_ctrl_in1;
};

struct tsmux_hex_ctrl {
	int32_t m2m_enable;
	int32_t m2m_key[4];
	int32_t m2m_cnt[4];
	int32_t m2m_stream_cnt;
	int32_t otf_enable;
	int32_t otf_key[4];
	int32_t otf_cnt[4];
	int32_t otf_stream_cnt;
	int32_t dbg_ctrl_bypass;
};

struct tsmux_psi_info {
	int32_t psi_data[TSMUX_PSI_SIZE];
	int32_t pcr_len;
	int32_t pmt_len;
	int32_t pat_len;
};

struct tsmux_buffer {
	int32_t ion_buf_fd;
	int32_t buffer_size;
	int32_t offset;
	int32_t actual_size;
	int32_t es_size;
	int32_t state;
	int32_t job_done;
	int32_t partial_done;
	int64_t time_stamp;

	int64_t g2d_start_stamp;
	int64_t g2d_end_stamp;
	int64_t mfc_start_stamp;
	int64_t mfc_end_stamp;
	int64_t tsmux_start_stamp;
	int64_t tsmux_end_stamp;
	int64_t kernel_end_stamp;
};

struct tsmux_job {
	struct tsmux_pkt_ctrl pkt_ctrl;
	struct tsmux_pes_hdr pes_hdr;
	struct tsmux_ts_hdr ts_hdr;
	struct tsmux_rtp_hdr rtp_hdr;
	struct tsmux_swp_ctrl swp_ctrl;
	struct tsmux_hex_ctrl hex_ctrl;
	struct tsmux_buffer in_buf;
	struct tsmux_buffer out_buf;
};

struct tsmux_otf_config {
	struct tsmux_pkt_ctrl pkt_ctrl;
	struct tsmux_pes_hdr pes_hdr;
	struct tsmux_ts_hdr ts_hdr;
	struct tsmux_rtp_hdr rtp_hdr;
	struct tsmux_swp_ctrl swp_ctrl;
	struct tsmux_hex_ctrl hex_ctrl;
};

struct tsmux_m2m_cmd_queue {
	struct tsmux_job m2m_job[TSMUX_MAX_M2M_CMD_QUEUE_NUM];
};

struct tsmux_otf_cmd_queue {
	// user read parameter
	int32_t cur_buf_num;
	struct tsmux_buffer out_buf[TSMUX_OUT_BUF_CNT];

	// user write parameter
	struct tsmux_otf_config config;
};

struct tsmux_rtp_ts_info {
	int32_t rtp_seq_number;
	int32_t rtp_seq_override;

	int32_t ts_pat_cc;
	int32_t ts_pmt_cc;

	int32_t ts_video_cc;
	int32_t ts_audio_cc;
};

#define TSMUX_IOCTL_SET_INFO		\
	_IOW('T', 0x1, struct tsmux_psi_info)

#define TSMUX_IOCTL_M2M_MAP_BUF		\
	_IOWR('A', 0x10, struct tsmux_m2m_cmd_queue)
#define TSMUX_IOCTL_M2M_UNMAP_BUF	\
	_IO('A', 0x11)
#define TSMUX_IOCTL_M2M_RUN		\
	_IOWR('A', 0x12, struct tsmux_m2m_cmd_queue)

#define TSMUX_IOCTL_OTF_MAP_BUF			\
	_IOR('A', 0x13, struct tsmux_otf_cmd_queue)
#define TSMUX_IOCTL_OTF_UNMAP_BUF		\
	_IO('A', 0x14)
#define TSMUX_IOCTL_OTF_DQ_BUF			\
	_IOR('A', 0x15, struct tsmux_otf_cmd_queue)
#define TSMUX_IOCTL_OTF_Q_BUF			\
	_IOW('A', 0x16, int32_t)
#define TSMUX_IOCTL_OTF_SET_CONFIG		\
	_IOW('A', 0x17, struct tsmux_otf_config)

#define TSMUX_IOCTL_SET_RTP_TS_INFO		\
	_IOW('A', 0x20, struct tsmux_rtp_ts_info)
#define TSMUX_IOCTL_GET_RTP_TS_INFO		\
	_IOR('A', 0x21, struct tsmux_rtp_ts_info)

#endif /* TSMUX_H */
