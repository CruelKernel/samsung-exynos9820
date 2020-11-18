
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/limits.h>
#include <linux/device.h>
#include <asm/cacheflush.h>

#include "npu-vs4l.h"
#include "npu-log.h"
#include "npu-common.h"
#include "npu-session.h"
#include "npu-qos.h"
#include "npu-util-statekeeper.h"
#include "npu-binary.h"
#include "npu-errno.h"

/* State management */
typedef enum {
	FW_TEST_STATE_UNLOADED = 0,
	FW_TEST_STATE_LOADED,
	FW_TEST_STATE_EXECUTING,
	FW_TEST_STATE_INVALID,
} fw_test_state_e;

const char *FW_TEST_STATE_NAME[FW_TEST_STATE_INVALID + 1]
	= {"UNLOADED", "LOADED", "EXECUTING", "INVALID"};

static const u8 fw_test_state_transition[][STATE_KEEPER_MAX_STATE] = {
	/* From    -   To       UNLOADED        LOADED          EXECUTING       INVALID*/
	/* UNLOADED     */ {    0,              1,              0,              0},
	/* LOADED       */ {    1,              1,              1,              0},
	/* EXECUTING    */ {    0,              1,              0,              0},
	/* INVALID      */ {    0,              0,              0,              0},
};

/* Reference to manage object */
static struct npu_fw_test_handler *ft_handle;

static struct device *get_dev_from_session(struct npu_session *sess)
{
	struct npu_vertex_ctx	*ctx;
	struct npu_vertex	*vertex;
	struct npu_device	*device;
	struct device		*dev;

	BUG_ON(!sess);
	ctx = &sess->vctx;
	BUG_ON(!ctx);
	vertex = ctx->vertex;
	BUG_ON(!vertex);
	device = container_of(vertex, struct npu_device, vertex);
	BUG_ON(!device);
	dev = device->dev;
	BUG_ON(!dev);
	return dev;
}

__attribute__((unused)) static int load_fw_utc_vector(struct npu_session *sess, struct vs4l_param *param)
{
	struct device			*dev;
	char				*vector_path;
	size_t				vector_len = 0;
	int				ret;
	struct npu_binary		vector_binary;
	struct npu_memory_buffer	*dram_buf;
	u8				*p8;

	BUG_ON(!ft_handle);
	BUG_ON(!sess);
	dev = get_dev_from_session(sess);

	dram_buf = ft_handle->dram_fw_log_buf;

	npu_trace("%s() started.\n", __func__);

	if (!SKEEPER_IS_TRANSITABLE(&ft_handle->sk, FW_TEST_STATE_LOADED))
		return -EINVAL;

	vector_path = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!vector_path) {
		ret = -ENOMEM;
		goto err_exit;
	}

	/* Retreve vector filename from parameter */
	if (param->size <= 0 || param->size >= PATH_MAX) {
		npu_err("Invalid size specified : %u\n", param->size);
		ret = -EINVAL;
		goto err_exit;
	}
	if (!param->addr) {
		npu_err("Invalid addr specified : %lu\n", param->addr);
		ret = -EINVAL;
		goto err_exit;
	}
	copy_from_user(vector_path, (__user void *)param->addr, param->size);
	vector_path[param->size] = '\0';
	npu_dbg("Loading FW test vector from : %s\n", vector_path);

	/* Check file's integrity */
	ret = npu_binary_init(&vector_binary, dev, vector_path, "\0", "\0");
	if (ret) {
		npu_err("npu_binary_init() failed : %d\n", ret);
		ret = -EINVAL;
		goto err_exit;
	}
	ret = npu_binary_g_size(&vector_binary, &vector_len);
	if (ret) {
		npu_err("npu_binary_g_size() failed : %d\n", ret);
		ret = -EINVAL;
		goto err_exit;
	}
	if (vector_len <= 0) {
		npu_err("Vector file[%s] is not found or invalid.\n", vector_path);
		ret = -EINVAL;
		goto err_exit;
	}
	if (vector_len >= dram_buf->size) {
		npu_err("Vector file[%s] is too large(%zuB). Maximum DRAM buf is %zuB\n",
			vector_path, vector_len, dram_buf->size);
		ret = -EINVAL;
		goto err_exit;
	}
	/* Disable store log, because it shares same DRAM buffer */
	npu_store_log_deinit();
	npu_info("Store log is disabled, until next resume.\n");

	/* Load file contents to memory */
	npu_info("Loading FW test vector[%s] to vaddr[%pK], len = %zuB\n",
		vector_path, dram_buf->vaddr, vector_len);
	if (!dram_buf->vaddr) {
		npu_err("DRAM buffer is not initialized.\n");
		ret = -EFAULT;
		goto err_exit;
	}
	ret = npu_binary_read(&vector_binary, dram_buf->vaddr, vector_len);
	if (ret) {
		npu_err("npu_binary_read([%s], %pK, %zu) failed : %d\n",
			vector_path, dram_buf->vaddr, vector_len, ret);
		ret = -EFAULT;
		goto err_exit;
	}

	/* Printout signature */
	if (vector_len > 4) {
		p8 = (u8 *)dram_buf->vaddr;
		npu_info("FW test vector HEAD=[%02x %02x %02x %02x], TAIL=[%02x %02x %02x %02x]\n",
			p8[0], p8[1], p8[2], p8[3],
			p8[vector_len - 1], p8[vector_len - 2],
			p8[vector_len - 3], p8[vector_len - 4]);
	} else {
		npu_warn("FW vector is too short. Signature is not available.\n");
	}

	/* Cache invalidation */
	__dma_map_area(dram_buf->vaddr, vector_len, DMA_TO_DEVICE);

	/* Transition state */
	npu_statekeeper_transition(&ft_handle->sk, FW_TEST_STATE_LOADED);

	ret = 0;

err_exit:
	if (vector_path)
		kfree(vector_path);

	npu_trace("%s() completed. ret = %d\n", __func__, ret);
	return ret;
}

static int fw_utc_save_result_cb(struct npu_session *sess, struct nw_result result)
{
	ft_handle->result = result;
	atomic_set(&ft_handle->wait_flag, 1);
	npu_info("FW test result received : [%u]\n", result.result_code);

	if (sess != ft_handle->cur_sess) {
		npu_warn("Session mismatch [%pK] / [%pK]. Something wrong.\n",
			sess, ft_handle->cur_sess);
	} else {
		/* Wake-up user thread */
		wake_up_all(&ft_handle->wq);
	}

	return 0;
}

static int execute_fw_utc_vector(struct npu_session *sess, struct vs4l_param *param)
{
	int				ret;
	struct npu_nw			nw;
	int				test_result = 0;
	struct npu_system		*system;

	npu_trace("%s() started.\n", __func__);
	BUG_ON(!ft_handle);

	if (!SKEEPER_IS_TRANSITABLE(&ft_handle->sk, FW_TEST_STATE_EXECUTING))
		return -EINVAL;

	npu_statekeeper_transition(&ft_handle->sk, FW_TEST_STATE_EXECUTING);

	/* Setting QoS to make the NPU at operation clock */
	system = container_of(ft_handle, struct npu_system, npu_fw_test_handler);
	BUG_ON(!system);
	ret = npu_qos_start(system);
	if (ret) {
		npu_uerr("npu_qos_start failed : (%d)\n", sess, ret);
		test_result = NPU_ERR_DRIVER(NPU_ERR_INVALID_STATE);
		goto err_exit;
	}

	/* Prepare npu_nw message */
	/* TODO: It is good to have common initialzier for npu_nw */
	memset(&nw, 0, sizeof(nw));
	nw.uid = sess->uid;
	nw.session = sess;
	nw.cmd = NPU_NW_CMD_FW_TC_EXECUTE;
	nw.param0 = param->offset;
	nw.magic_tail = NPU_NW_MAGIC_TAIL;

	npu_uinfo("Executing FW test #%u\n", sess, nw.param0);

	/* Setting callback */
	nw.notify_func = fw_utc_save_result_cb;
	ft_handle->cur_sess = sess;	/* For sanity check */

	/* Post message and wait until the callback is invoked */
	atomic_set(&ft_handle->wait_flag, 0);
	ret = npu_ncp_mgmt_put(&nw);
	if (!ret) {
		npu_uerr("npu_ncp_mgmt_put failed : (%d)\n", sess, ret);
		test_result = NPU_ERR_DRIVER(NPU_ERR_INVALID_STATE);
		goto qos_off;
	}

	/* Collect result */
	ret = wait_event_timeout(
		ft_handle->wq,
		atomic_read(&ft_handle->wait_flag) > 0,
		FW_TC_EXEC_TIMEOUT);

	/* Construct return value */
	if (ret == 0) {
		/* Timeout and result is not available */
		npu_uerr("Test timeout\n", sess);
		test_result = NPU_ERR_DRIVER(FW_TC_RESULT_TIMEOUT);
	} else {
		npu_uinfo("Test finished with code [%u]\n", sess, ft_handle->result.result_code);
		/* Set result value */
		test_result = ft_handle->result.result_code;
	}

qos_off:
	ret = npu_qos_stop(system);
	if (ret) {
		npu_uerr("npu_qos_stop failed : (%d)\n", sess, ret);
		test_result = NPU_ERR_DRIVER(NPU_ERR_INVALID_STATE);
		goto err_exit;
	}

err_exit:
	ft_handle->cur_sess = NULL;
	npu_statekeeper_transition(&ft_handle->sk, FW_TEST_STATE_LOADED);
	npu_trace("%s() completed with result [%d].\n", __func__, test_result);
	return test_result;
}

int npu_fw_test_initialize(struct npu_system *system, struct npu_memory_buffer *dram_fw_log_buf)
{
	BUG_ON(!system);
	BUG_ON(!dram_fw_log_buf);
	BUG_ON(!dram_fw_log_buf->vaddr);

	ft_handle = &(system->npu_fw_test_handler);
	memset(ft_handle, 0, sizeof(*ft_handle));

	/* Initialize state keeper */
	npu_statekeeper_initialize(
		&ft_handle->sk, FW_TEST_STATE_INVALID,
		FW_TEST_STATE_NAME, fw_test_state_transition);

	/* Keep reference to device and DRAM buffer */
	ft_handle->dev = &(system->pdev->dev);
	ft_handle->dram_fw_log_buf = dram_fw_log_buf;

	/* Initialize wait q */
	init_waitqueue_head(&ft_handle->wq);

	npu_info("fw_test : initialized.\n");
	return 0;
}

npu_s_param_ret fw_test_s_param_handler(struct npu_session *sess, struct vs4l_param *param, int *retval)
{
	int		ret;

	BUG_ON(!sess);
	BUG_ON(!param);

	switch (param->target) {
	case NPU_S_PARAM_FW_UTC_LOAD:
#ifdef CONFIG_EXYNOS_NPU_DRAM_FW_LOG_BUF
		ret = load_fw_utc_vector(sess, param);
		return S_PARAM_HANDLED;
#else
		npu_err("Firmware UTC support requires CONFIG_EXYNOS_NPU_DRAM_FW_LOG_BUF option.\n");
		return S_PARAM_ERROR;
#endif

	case NPU_S_PARAM_FW_UTC_EXECUTE:
		ret = execute_fw_utc_vector(sess, param);
		return S_PARAM_HANDLED;

	default:
		/* Unknown set param command -> Pass to next handler */
		return S_PARAM_NOMB;
	}
}

