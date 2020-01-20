/*
 * Samsung Exynos SoC series SCore driver
 *
 * Module for controlling shared command queue (SCQ)
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SCORE_SCQ_H__
#define __SCORE_SCQ_H__

#include <linux/kthread.h>
#include "score_frame.h"

/* max scq size is 512 word */
#define MAX_SCQ_SIZE	(512)

/* result size is 4 word */
#define RESULT_SCQ_SIZE	(4)

struct score_system;

/*
 * enum score_scq_state - State of scq object
 * @SCORE_SCQ_STATE_OPEN:
 *      open is state that scq object is ready to boot
 * @SCORE_SCQ_STATE_START:
 *      start is state that scq object is ready to be used
 * @SCORE_SCQ_STATE_CLOSE:
 *      close is state that scq object is stopped
 */
enum score_scq_state {
	SCORE_SCQ_STATE_OPEN,
	SCORE_SCQ_STATE_START,
	SCORE_SCQ_STATE_CLOSE,
};

/**
 * struct score_scq
 * @in_size: size of read_command
 * @in_param: contents of read_command
 * @master_id: sender id of read_command
 * @out_size: size of write_command
 * @out_param: contents of write_command
 * @slave_id: receiver id of write_command
 * @priority: priority of command
 * @count: count of frame which included in scq
 * @high_count: count of frame of which priority is high
 * @normal_count: count of frame of which priority is normal
 * @high_max_count: max count of frame of which priority is high
 * @normal_max_count: max count of frame of which priority is normal
 * @sfr: sfr
 * @state: state of scq object
 * @write_worker: worker sending command to SCore device
 * @write_task: task sending command that is assigned to each frame
 */
struct score_scq {
	unsigned int		in_size;
	unsigned int		in_param[RESULT_SCQ_SIZE];
	unsigned int		master_id;
	unsigned int		out_size;
	unsigned int		*out_param;
	unsigned int		slave_id;
	unsigned int		priority;

	unsigned int		count;
	unsigned int		high_count;
	unsigned int		normal_count;
	unsigned int		high_max_count;
	unsigned int		normal_max_count;
	void __iomem		*sfr;
	unsigned int		state;
	struct kthread_worker	write_worker;
	struct task_struct	*write_task;
};

int score_scq_send_packet(struct score_scq *scq, struct score_frame *frame);
int score_scq_read(struct score_scq *scq, struct score_frame *frame);
int score_scq_write(struct score_scq *scq, struct score_frame *frame);

void score_scq_init(struct score_scq *scq);
int score_scq_open(struct score_scq *scq);
int score_scq_close(struct score_scq *scq);
int score_scq_probe(struct score_system *system);
int score_scq_remove(struct score_scq *scq);

#endif
