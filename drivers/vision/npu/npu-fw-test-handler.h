#ifndef _NPU_FW_TEST_HANDLER_H_
#define _NPU_FW_TEST_HANDLER_H_

#include <linux/wait.h>
#include "npu-util-statekeeper.h"

/* Execution timeout : 100 seconds */
#define FW_TC_EXEC_TIMEOUT	(100 * HZ)

/* Special return value for timeout */
#define FW_TC_RESULT_TIMEOUT	(0x00008282)

/* Mask for test failure */
#define FW_TC_FAILED		(0x0000FF00)
#define FW_TC_FAIL_CODE_MASK	(0x000000FF)

struct npu_fw_test_handler {
	struct npu_statekeeper		sk;

	struct device			*dev;
	struct npu_memory_buffer	*dram_fw_log_buf;

	wait_queue_head_t		wq;
	atomic_t			wait_flag;

	struct npu_session		*cur_sess;
	struct nw_result		result;
};

int npu_fw_test_initialize(struct npu_system *system, struct npu_memory_buffer *dram_fw_log_buf);
npu_s_param_ret fw_test_s_param_handler(struct npu_session *sess, struct vs4l_param *param, int *retval);

#endif	/* _NPU_FW_TEST_HANDLER_H_ */
