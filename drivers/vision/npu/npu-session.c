/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
//#include "lib/vpul-ds.h"
#include "npu-log.h"
#include "npu-common.h"
#include "npu-session.h"
#include "npu-log.h"

#include <asm/cacheflush.h>

#ifdef NPU_LOG_TAG
#undef NPU_LOG_TAG
#endif
#define NPU_LOG_TAG             "npu-sess"

//#define SYSMMU_FAULT_TEST

const struct npu_session_ops npu_session_ops;

struct addr_info *find_addr_info(struct npu_session *session, u32 av_index, mem_opt_e mem_type, u32 cl_index)
{
	struct addr_info *ret = NULL;
	u32 i;
	u32 FM_cnt;
	struct addr_info *addr_info = NULL;

	switch (mem_type) {
	case OFM_TYPE:
		FM_cnt = session->OFM_cnt;
		addr_info = session->OFM_info[cl_index].addr_info;
		break;
	case IMB_TYPE:
		FM_cnt = session->IMB_cnt;
		addr_info = session->IMB_info;
		break;
	default:
		npu_uerr("Invalid mem_type : %d\n", session, mem_type);
		ret = NULL;
		goto err_exit;
	}
	for (i = 0; i < FM_cnt; i++) {
		if ((addr_info + i)->av_index == av_index) {
			ret = addr_info + i;
			break;
		}
	}
err_exit:
	return ret;
}

npu_errno_t chk_nw_result_no_error(struct npu_session *session)
{
	return session->nw_result.result_code;
}

int npu_session_save_result(struct npu_session *session, struct nw_result nw_result)
{
	session->nw_result = nw_result;
	wake_up(&session->wq);
	return 0;
}

void npu_session_queue_done(struct npu_queue *queue, struct vb_container_list *inclist, struct vb_container_list *otclist, unsigned long flag)
{
	npu_queue_done(queue, inclist, otclist, flag);
	return;
}

static save_result_func get_notify_func(const nw_cmd_e nw_cmd)
{
	if (NPU_NW_CMD_UNLOAD == nw_cmd || NPU_NW_CMD_CLEAR_CB == nw_cmd)
		return NULL;
	else
		return npu_session_save_result;
}

static int npu_session_put_nw_req(struct npu_session *session, nw_cmd_e nw_cmd)
{
	int ret = 0;
	struct npu_nw req = {
		.uid = session->uid,
		.npu_req_id = 0,
		.result_code = 0,
		.session = session,
		.cmd = nw_cmd,
		.ncp_addr = session->ncp_info.ncp_addr,
		.magic_tail = NPU_NW_MAGIC_TAIL,
	};

	BUG_ON(!session);

	req.notify_func = get_notify_func(nw_cmd);

	ret = npu_ncp_mgmt_put(&req);
	if (!ret) {
		npu_uerr("npu_ncp_mgmt_put failed", session);
		return ret;
	}
	return ret;
}

int npu_session_put_frame_req(
	struct npu_session *session, struct npu_queue *queue,
	struct vb_container_list *incl, struct vb_container_list *otcl,
	frame_cmd_e frame_cmd, struct av_info *IFM_info, struct av_info *OFM_info)
{
	int ret = 0;
	struct npu_frame frame = {
		.uid = session->uid,
		.frame_id = incl->id,
		.npu_req_id = 0,	/* Determined in manager */
		.result_code = 0,
		.session = session,
		.cmd = frame_cmd,
		.priority = 0,		/* Not used */
		.src_queue = queue,
		.input = incl,
		.output = otcl,
		.magic_tail = NPU_FRAME_MAGIC_TAIL,
		.mbox_process_dat = session->mbox_process_dat,
		.IFM_info = IFM_info,
		.OFM_info = OFM_info,
	};

	ret = npu_buffer_q_put(&frame);
	if (!ret) {
		npu_uerr("fail(%d) in npu_buffer_q_put",
				session, ret);
		ret = -EFAULT;
		goto p_err;
	}
	npu_udbg("success in %s", session, __func__);
p_err:
	return ret;
}

int add_ion_mem(struct npu_session *session, struct npu_memory_buffer *mem_buf, mem_opt_e MEM_OPT)
{
	switch (MEM_OPT) {
	case NCP_TYPE:
		session->ncp_mem_buf = mem_buf;
		break;
	case IOFM_TYPE:
		session->IOFM_mem_buf = mem_buf;
		break;
	case IMB_TYPE:
		session->IMB_mem_buf = mem_buf;
		break;
	default:
		break;
	}
	return 0;
}

void __ion_release(struct npu_memory *memory, struct npu_memory_buffer *ion_mem_buf, u32 idx)
{
	u32 i;

	for (i = 0; i < idx; i++)
		npu_memory_free(memory, ion_mem_buf + i);
}

void __release_graph_ion(struct npu_session *session)
{
	u32 i = 0;
	u32 IMB_cnt = session->IMB_cnt;
	struct npu_memory *memory;
	struct npu_memory_buffer *ion_mem_buf;

	memory = session->memory;
	ion_mem_buf = session->ncp_mem_buf;
	session->ncp_mem_buf = NULL;
	if (ion_mem_buf) {
		npu_memory_unmap(memory, ion_mem_buf);
		kfree(ion_mem_buf);
	}

	ion_mem_buf = session->IOFM_mem_buf;
	session->IOFM_mem_buf = NULL;
	if (ion_mem_buf) {
		for (i = 0; i < VISION_MAX_BUFFER; i++) {
			npu_memory_free(memory, ion_mem_buf + i);
		}
		kfree(ion_mem_buf);
	}

	ion_mem_buf = session->IMB_mem_buf;
	session->IMB_mem_buf = NULL;
	if (ion_mem_buf) {
		for (i = 0; i < IMB_cnt; i++) {
			npu_memory_free(memory, ion_mem_buf + i);
		}
		kfree(ion_mem_buf);
	}
}

int npu_session_NW_CMD_UNLOAD(struct npu_session *session)
{
	/* check npu_device emergency error */
	struct npu_vertex_ctx *vctx;
	struct npu_vertex *vertex;
	struct npu_device *device;
	nw_cmd_e nw_cmd = NPU_NW_CMD_UNLOAD;

	if (!session) {
		npu_err("invalid session\n");
		return -EINVAL;
	}

	vctx = &(session->vctx);
	vertex = vctx->vertex;
	device = container_of(vertex, struct npu_device, vertex);

	if (npu_device_is_emergency_err(device))
		npu_udbg("NPU DEVICE IS EMERGENCY ERROR\n", session);

	/* Post unload command */
	npu_udbg("sending UNLOAD command.\n", session);
	session->nw_result.result_code = NPU_NW_JUST_STARTED;
	npu_session_put_nw_req(session, nw_cmd);

	return 0;
}

int npu_session_close(struct npu_session *session)
{
	int ret = 0;

	BUG_ON(!session);

	session->ss_state |= BIT(NPU_SESSION_STATE_CLOSE);

	ret = npu_session_undo_close(session);
	if (ret)
		goto p_err;

	ret = npu_session_undo_open(session);
	if (ret)
		goto p_err;

	return ret;
p_err:
	npu_err("fail(%d) in npu_session_close\n", ret);
	return ret;
}

int npu_session_open(struct npu_session **session, void *cookie, void *memory)
{
	int ret = 0;
	int i = 0;

	*session = kzalloc(sizeof(struct npu_session), GFP_KERNEL);
	if (*session == NULL) {
		npu_err("fail in npu_session_open kzalloc\n");
		ret = -ENOMEM;
		return ret;
	}

	(*session)->ss_state |= BIT(NPU_SESSION_STATE_OPEN);

	ret = npu_sessionmgr_regID(cookie, *session);
	if (ret) {
		npu_err("fail(%d) in npu_sessionmgr_regID\n", ret);
		npu_session_register_undo_cb(*session, npu_session_undo_open);
		goto p_err;
	}

	(*session)->ss_state |= BIT(NPU_SESSION_STATE_REGISTER);

	init_waitqueue_head(&((*session)->wq));

	(*session)->cookie = cookie;
	(*session)->memory = memory;
	(*session)->gops = &npu_session_ops;
	(*session)->control.owner = *session;

	(*session)->IMB_info = NULL;
	for (i = 0; i < VISION_MAX_BUFFER; i++) {
		(*session)->IFM_info[i].addr_info = NULL;
		(*session)->IFM_info[i].state = SS_BUF_STATE_UNPREPARE;
		(*session)->OFM_info[i].addr_info = NULL;
		(*session)->OFM_info[i].state = SS_BUF_STATE_UNPREPARE;
	}
	(*session)->ncp_mem_buf = NULL;
	(*session)->IOFM_mem_buf = NULL;
	(*session)->IMB_mem_buf = NULL;

	(*session)->qbuf_IOFM_idx = -1;
	(*session)->dqbuf_IOFM_idx = -1;

	(*session)->pid	= task_pid_nr(current);

	return ret;
p_err:
	npu_session_execute_undo_cb(*session);
	return ret;
}

int _undo_s_graph_each_state(struct npu_session *session)
{
	u32 i = 0;
	u32 IMB_cnt;
	struct npu_memory *memory;
	struct npu_memory_buffer *ion_mem_buf;
	struct addr_info *addr_info;

	BUG_ON(!session);

	memory = session->memory;
	IMB_cnt = session->IMB_cnt;

	if (session->ss_state <= BIT(NPU_SESSION_STATE_GRAPH_ION_MAP))
		goto graph_ion_unmap;
	if (session->ss_state <= BIT(NPU_SESSION_STATE_WGT_KALLOC))
		goto wgt_kfree;
	if (session->ss_state <= BIT(NPU_SESSION_STATE_IOFM_KALLOC))
		goto iofm_kfree;
	if (session->ss_state <= BIT(NPU_SESSION_STATE_IOFM_ION_ALLOC))
		goto iofm_ion_unmap;
	if (session->ss_state <= BIT(NPU_SESSION_STATE_IMB_ION_ALLOC))
		goto imb_ion_unmap;

imb_ion_unmap:
	addr_info = session->IMB_info;
	session->IMB_info = NULL;
	if (addr_info)
		kfree(addr_info);

	ion_mem_buf = session->IMB_mem_buf;
	session->IMB_mem_buf = NULL;
	if (ion_mem_buf) {
		for (i = 0; i < IMB_cnt; i++) {
			npu_memory_free(memory, ion_mem_buf + i);
		}
		kfree(ion_mem_buf);
	}

iofm_ion_unmap:
	ion_mem_buf = session->IOFM_mem_buf;
	session->IOFM_mem_buf = NULL;
	if (ion_mem_buf) {
		for (i = 0; i < VISION_MAX_BUFFER; i++) {
			npu_memory_free(memory, ion_mem_buf + i);
		}
		kfree(ion_mem_buf);
	}

iofm_kfree:
	for (i = 0; i < VISION_MAX_BUFFER; i++) {
		addr_info = session->IFM_info[i].addr_info;
		session->IFM_info[i].addr_info = NULL;
		if (addr_info)
			kfree(addr_info);

		addr_info = session->OFM_info[i].addr_info;
		session->OFM_info[i].addr_info = NULL;
		if (addr_info)
			kfree(addr_info);
	}

wgt_kfree:
	addr_info = session->WGT_info;
	session->WGT_info = NULL;
	if (addr_info)
		kfree(addr_info);

graph_ion_unmap:
	ion_mem_buf = session->ncp_mem_buf;
	session->ncp_mem_buf = NULL;
	if (ion_mem_buf) {
		npu_memory_unmap(memory, ion_mem_buf);
		kfree(ion_mem_buf);
	}

	return 0;
}

int _undo_s_format_each_state(struct npu_session *session)
{
	int ret = 0;

	BUG_ON(!session);

	if (session->ss_state < BIT(NPU_SESSION_STATE_IMB_ION_ALLOC)) {
		ret = -EINVAL;
		goto p_err;
	}

	if (session->ss_state <= BIT(NPU_SESSION_STATE_FORMAT_OT)) {
		goto init_format;
	}

init_format:

p_err:
	return ret;
}

int _undo_streamon_each_state(struct npu_session *session)
{
	int ret = 0;

	BUG_ON(!session);

	if (session->ss_state < BIT(NPU_SESSION_STATE_FORMAT_OT)) {
		ret = -EINVAL;
		goto p_err;
	}

	if (session->ss_state <= BIT(NPU_SESSION_STATE_START))
		goto release_streamon;

release_streamon:

p_err:
	return ret;
}

int _undo_streamoff_each_state(struct npu_session *session)
{
	int ret = 0;

	BUG_ON(!session);

	if (session->ss_state < BIT(NPU_SESSION_STATE_START)) {
		ret = -EINVAL;
		goto p_err;
	}

	if (session->ss_state <= BIT(NPU_SESSION_STATE_STOP))
		goto release_streamoff;

release_streamoff:

p_err:
	return ret;
}

int _undo_close_each_state(struct npu_session *session)
{
	int ret = 0;

	BUG_ON(!session);

	if (session->ss_state < BIT(NPU_SESSION_STATE_STOP)) {
		ret = -EINVAL;
		goto p_err;
	}

	if (session->ss_state <= BIT(NPU_SESSION_STATE_CLOSE))
		goto session_close;

session_close:

p_err:
	return ret;
}

bool EVER_FIND_FM(u32 *FM_cnt, struct temp_av *FM_av, u32 address_vector_index)
{
	bool ret = false;
	u32 i;

	for (i = 0; i < (*FM_cnt) ; i++) {
		if ((FM_av + i)->index == address_vector_index) {
			ret = true;
			break;
		}
	}
	return ret;
}

void __set_unique_id(struct npu_session *session, struct drv_usr_share *usr_data)
{
	u32 uid = session->uid;
	usr_data->id = uid;
}

int __get_ncp_bin_size(struct npu_binary *binary, char *ncp_path, char *ncp_name, size_t *ncp_size)
{
	int ret = 0;
	struct device dev_obj = {};
	struct device *dev = &dev_obj;

	ret = npu_binary_init(binary, dev, ncp_path, NCP_BIN_PATH, ncp_name);
	if (ret)
		goto p_err;

	ret = npu_binary_g_size(binary, ncp_size);
	if (ret)
		goto p_err;

	return ret;
p_err:
	npu_err("fail in __get_ncp_bin_size %d\n", ret);
	return ret;
}

int __update_ncp_info(struct npu_session *session, struct npu_memory_buffer *ncp_mem_buf)
{
	if (!session) {
		npu_err("invalid session or ncp_mem_buf\n");
		return -EINVAL;
	}

	session->ncp_info.ncp_addr.size = ncp_mem_buf->size;
	session->ncp_info.ncp_addr.vaddr = ncp_mem_buf->vaddr;
	session->ncp_info.ncp_addr.daddr = ncp_mem_buf->daddr;
	return 0;
}

int __ncp_ion_map(struct npu_session *session, struct drv_usr_share *usr_data)
{
	int ret = 0;

	struct npu_memory_buffer *ncp_mem_buf = NULL;
	mem_opt_e opt = NCP_TYPE;

	ncp_mem_buf = kzalloc(sizeof(struct npu_memory_buffer), GFP_KERNEL);
	if (ncp_mem_buf == NULL) {
		npu_err("fail in npu_ion_map kzalloc\n");
		ret = -ENOMEM;
		goto p_err;
	}
	ncp_mem_buf->fd = usr_data->ncp_fd;
	ncp_mem_buf->size = usr_data->ncp_size;

	ret = npu_memory_map(session->memory, ncp_mem_buf);
	if (ret) {
		npu_err("npu_memory_map is fail(%d).\n", ret);
		if (ncp_mem_buf)
			kfree(ncp_mem_buf);
		goto p_err;
	}
	npu_info("ncp_ion_map(0x%pad), vaddr(0x%pK)\n", &ncp_mem_buf->daddr, ncp_mem_buf->vaddr);
	ret = __update_ncp_info(session, ncp_mem_buf);
	if (ret) {
		npu_err("__ncp_ion_map is fail(%d).\n", ret);
		if (ncp_mem_buf)
			kfree(ncp_mem_buf);
		goto p_err;
	}
	ret = add_ion_mem(session, ncp_mem_buf, opt);
	if (ret) {
		npu_err("__ncp_ion_map is fail(%d).\n", ret);
		if (ncp_mem_buf)
			kfree(ncp_mem_buf);
		goto p_err;
	}
p_err:
	return ret;
}

int __get_session_info(struct npu_session *session, struct vs4l_graph *info)
{
	int ret = 0;
	struct drv_usr_share *usr_data = NULL;
	usr_data = kzalloc(sizeof(struct drv_usr_share), GFP_KERNEL);
	if (!usr_data) {
		npu_err("Not enough Memory\n");
		ret = -ENOMEM;
		return ret;
	}
	copy_from_user((void *)usr_data, (void *)info->addr, sizeof(struct drv_usr_share));
	__set_unique_id(session, usr_data);
	npu_utrace("usr_data(0x%pK), ncp_size(%u)\n", session, usr_data, usr_data->ncp_size);
	copy_to_user((void *)info->addr, (void *)usr_data, sizeof(struct drv_usr_share));
	ret = __ncp_ion_map(session, usr_data);
	if (ret) {
		npu_uerr("__ncp_ion_map is fail(%d)\n", session, ret);
		goto p_err;
	}
	session->ss_state |= BIT(NPU_SESSION_STATE_GRAPH_ION_MAP);
p_err:
	if (usr_data)
		kfree(usr_data);
	return ret;
}

int __pilot_parsing_ncp(struct npu_session *session, u32 *IFM_cnt, u32 *OFM_cnt, u32 *IMB_cnt, u32 *WGT_cnt)
{
	int ret = 0;
	u32 i = 0;
	char *ncp_vaddr;
	u32 memory_vector_cnt;
	u32 memory_vector_offset;
	struct memory_vector *mv;
	struct ncp_header *ncp;

	ncp_vaddr = (char *)session->ncp_mem_buf->vaddr;
	ncp = (struct ncp_header *)ncp_vaddr;
	memory_vector_offset = ncp->memory_vector_offset;
	if (memory_vector_offset > session->ncp_mem_buf->size) {
		npu_err("memory vector offset(0x%x) > max size(0x%x) ;out of bounds\n",
				(u32)memory_vector_offset, (u32)session->ncp_mem_buf->size);
		return -EFAULT;
	}

	memory_vector_cnt = ncp->memory_vector_cnt;
	if (((memory_vector_cnt * sizeof(struct memory_vector)) + memory_vector_offset) >  session->ncp_mem_buf->size) {
		npu_err("memory vector count(0x%x) seems abnormal ;out of bounds\n", memory_vector_cnt);
		return -EFAULT;
	}
	session->memory_vector_offset = memory_vector_offset;
	session->memory_vector_cnt = memory_vector_cnt;
	mv = (struct memory_vector *)(ncp_vaddr + memory_vector_offset);

	for (i = 0; i < memory_vector_cnt; i++) {
		u32 memory_type = (mv + i)->type;
		switch (memory_type) {
		case MEMORY_TYPE_IN_FMAP:
			{
				(*IFM_cnt)++;
				break;
			}
		case MEMORY_TYPE_OT_FMAP:
			{
				(*OFM_cnt)++;
				break;
			}
		case MEMORY_TYPE_IM_FMAP:
			{
				(*IMB_cnt)++;
				break;
			}
		case MEMORY_TYPE_CUCODE:
		case MEMORY_TYPE_WEIGHT:
		case MEMORY_TYPE_WMASK:
			{
				(*WGT_cnt)++;
				break;
			}
		}
	}
	return ret;
}

int __second_parsing_ncp(
	struct npu_session *session,
	struct temp_av **temp_IFM_av, struct temp_av **temp_OFM_av,
	struct temp_av **temp_IMB_av, struct addr_info **WGT_av)
{
	int ret = 0;
	u32 i = 0;
	u32 weight_size;

	u32 IFM_cnt = 0;
	u32 OFM_cnt = 0;
	u32 IMB_cnt = 0;
	u32 WGT_cnt = 0;

	u32 address_vector_offset;
	u32 address_vector_cnt;
	u32 memory_vector_offset;
	u32 memory_vector_cnt;

	struct ncp_header *ncp;
	struct address_vector *av;
	struct memory_vector *mv;

	char *ncp_vaddr;
	dma_addr_t ncp_daddr;

	ncp_vaddr = (char *)session->ncp_mem_buf->vaddr;
	ncp_daddr = session->ncp_mem_buf->daddr;
	ncp = (struct ncp_header *)ncp_vaddr;

	address_vector_offset = ncp->address_vector_offset;
	if (address_vector_offset > session->ncp_mem_buf->size) {
		npu_err("address vector offset(0x%x) > max size(0x%x) ;out of bounds\n",
				address_vector_offset, (u32)session->ncp_mem_buf->size);
		return -EFAULT;
	}

	address_vector_cnt = ncp->address_vector_cnt;
	if (((address_vector_cnt * sizeof(struct address_vector)) + address_vector_offset) >
									session->ncp_mem_buf->size) {
		npu_err("address vector count(0x%x) seems abnormal ;out of bounds\n", address_vector_cnt);
		return -EFAULT;
	}

	session->address_vector_offset = address_vector_offset;
	session->address_vector_cnt = address_vector_cnt;

	session->ncp_info.address_vector_cnt = address_vector_cnt;

	memory_vector_offset = session->memory_vector_offset;
	memory_vector_cnt = session->memory_vector_cnt;

	if (address_vector_cnt > memory_vector_cnt) {
		npu_err("address_vector_cnt(%d) should not exceed memory_vector_cnt(%d)\n",
						address_vector_cnt, memory_vector_cnt);
		return -EFAULT;
	}

	mv = (struct memory_vector *)(ncp_vaddr + memory_vector_offset);
	av = (struct address_vector *)(ncp_vaddr + address_vector_offset);

	for (i = 0; i < memory_vector_cnt; i++) {
		u32 memory_type = (mv + i)->type;
		u32 address_vector_index;
		u32 weight_offset;

		switch (memory_type) {
		case MEMORY_TYPE_IN_FMAP:
			{
				if (IFM_cnt >= session->IFM_cnt) {
					npu_err("IFM_cnt(%d) should not exceed size of allocated array(%d)\n",
							IFM_cnt, session->IFM_cnt);
					return -EFAULT;
				}
				address_vector_index = (mv + i)->address_vector_index;
				if (!EVER_FIND_FM(&IFM_cnt, *temp_IFM_av, address_vector_index)) {
					(*temp_IFM_av + IFM_cnt)->index = address_vector_index;
					if (unlikely(((address_vector_index * sizeof(struct address_vector)) + address_vector_offset) >
								session->ncp_mem_buf->size) && (address_vector_index >= address_vector_cnt)) {
						npu_err("address_vector_index(%d) should not exceed max addr vec count(%d)\n",
								address_vector_index, address_vector_cnt);
						return -EFAULT;
					}
					(*temp_IFM_av + IFM_cnt)->size = (av + address_vector_index)->size;
					(*temp_IFM_av + IFM_cnt)->pixel_format = (mv + i)->pixel_format;
					(*temp_IFM_av + IFM_cnt)->width = (mv + i)->width;
					(*temp_IFM_av + IFM_cnt)->height = (mv + i)->height;
					(*temp_IFM_av + IFM_cnt)->channels = (mv + i)->channels;
					(mv + i)->stride = 0;
					(*temp_IFM_av + IFM_cnt)->stride = (mv + i)->stride;
					npu_uinfo("(IFM_av + %u)->index = %u\n", session,
						IFM_cnt, (*temp_IFM_av + IFM_cnt)->index);
					npu_utrace("[session] IFM, index(%u)\n"
						"[session] IFM, size(%zu)\n"
						"[session] IFM, pixel_format(%u)\n"
						"[session] IFM, width(%u)\n"
						"[session] IFM, height(%u)\n"
						"[session] IFM, channels(%u)\n"
						"[session] IFM, stride(%u)\n",
						session,
						(*temp_IFM_av + IFM_cnt)->index,
						(*temp_IFM_av + IFM_cnt)->size,
						(*temp_IFM_av + IFM_cnt)->pixel_format,
						(*temp_IFM_av + IFM_cnt)->width,
						(*temp_IFM_av + IFM_cnt)->height,
						(*temp_IFM_av + IFM_cnt)->channels,
						(*temp_IFM_av + IFM_cnt)->stride);

					IFM_cnt++;
				}
				break;
			}
		case MEMORY_TYPE_OT_FMAP:
			{
				if (OFM_cnt >= session->OFM_cnt) {
					npu_err("OFM_cnt(%d) should not exceed size of allocated array(%d)\n",
							OFM_cnt, session->OFM_cnt);
					return -EFAULT;
				}
				address_vector_index = (mv + i)->address_vector_index;
				if (!EVER_FIND_FM(&OFM_cnt, *temp_OFM_av, address_vector_index)) {
					(*temp_OFM_av + OFM_cnt)->index = address_vector_index;
					if (unlikely(((address_vector_index * sizeof(struct address_vector)) + address_vector_offset) >
								session->ncp_mem_buf->size) && (address_vector_index >= address_vector_cnt)) {
						npu_err("address_vector_index(%d) should not exceed max addr vec count(%d)\n",
								address_vector_index, address_vector_cnt);
						return -EFAULT;
					}
					(*temp_OFM_av + OFM_cnt)->size = (av + address_vector_index)->size;
					(*temp_OFM_av + OFM_cnt)->pixel_format = (mv + i)->pixel_format;
					(*temp_OFM_av + OFM_cnt)->width = (mv + i)->width;
					(*temp_OFM_av + OFM_cnt)->height = (mv + i)->height;
					(*temp_OFM_av + OFM_cnt)->channels = (mv + i)->channels;
					(mv + i)->stride = 0;
					(*temp_OFM_av + OFM_cnt)->stride = (mv + i)->stride;
					npu_uinfo("(OFM_av + %u)->index = %u\n", session,
						OFM_cnt, (*temp_OFM_av + OFM_cnt)->index);
					npu_utrace("OFM, index(%d)\n"
						"[session] OFM, size(%zu)\n"
						"[session] OFM, pixel_format(%u)\n"
						"[session] OFM, width(%u)\n"
						"[session] OFM, height(%u)\n"
						"[session] OFM, channels(%u)\n"
						"[session] OFM, stride(%u)\n",
						session,
						(*temp_OFM_av + OFM_cnt)->index,
						(*temp_OFM_av + OFM_cnt)->size,
						(*temp_OFM_av + OFM_cnt)->pixel_format,
						(*temp_OFM_av + OFM_cnt)->width,
						(*temp_OFM_av + OFM_cnt)->height,
						(*temp_OFM_av + OFM_cnt)->channels,
						(*temp_OFM_av + OFM_cnt)->stride);
					OFM_cnt++;
				}
				break;
			}
		case MEMORY_TYPE_IM_FMAP:
			{
				if (IMB_cnt >= session->IMB_cnt) {
					npu_err("IMB_cnt(%d) should not exceed size of allocated array(%d)\n",
							IMB_cnt, session->IMB_cnt);
					return -EFAULT;
				}
				address_vector_index = (mv + i)->address_vector_index;
				if (!EVER_FIND_FM(&IMB_cnt, *temp_IMB_av, address_vector_index)) {
					(*temp_IMB_av + IMB_cnt)->index = address_vector_index;
					if (unlikely(((address_vector_index * sizeof(struct address_vector)) + address_vector_offset) >
								session->ncp_mem_buf->size) && (address_vector_index >= address_vector_cnt)) {
						npu_err("address_vector_index(%d) should not exceed max addr vec count(%d)\n",
								address_vector_index, address_vector_cnt);
						return -EFAULT;
					}
					(*temp_IMB_av + IMB_cnt)->size = (av + address_vector_index)->size;
					(*temp_IMB_av + IMB_cnt)->pixel_format = (mv + i)->pixel_format;
					(*temp_IMB_av + IMB_cnt)->width = (mv + i)->width;
					(*temp_IMB_av + IMB_cnt)->height = (mv + i)->height;
					(*temp_IMB_av + IMB_cnt)->channels = (mv + i)->channels;
					(mv + i)->stride = 0;
					(*temp_IMB_av + IMB_cnt)->stride = (mv + i)->stride;
					//npu_info("(*temp_IMB_av + %ld)->index = 0x%x\n",
					//	IMB_cnt, (*temp_IMB_av + IMB_cnt)->index);
					//npu_info("(*temp_IMB_av + %ld)->size = 0x%x\n",
					//	IMB_cnt, (*temp_IMB_av + IMB_cnt)->size);
					IMB_cnt++;
				}
				break;
			}
		case MEMORY_TYPE_CUCODE:
		case MEMORY_TYPE_WEIGHT:
		case MEMORY_TYPE_WMASK:
			{
				if (WGT_cnt >= session->WGT_cnt) {
					npu_err("WGT_cnt(%d) should not exceed size of allocated array(%d)\n",
							WGT_cnt, session->WGT_cnt);
					return -EFAULT;
				}
				// update address vector, m_addr with ncp_alloc_daddr + offset
				address_vector_index = (mv + i)->address_vector_index;
				if (unlikely(((address_vector_index * sizeof(struct address_vector)) + address_vector_offset) >
							session->ncp_mem_buf->size) && (address_vector_index >= address_vector_cnt)) {
					npu_err("address_vector_index(%d) should not exceed max addr vec count(%d)\n",
							address_vector_index, address_vector_cnt);
					return -EFAULT;
				}
				weight_offset = (av + address_vector_index)->m_addr;
				if (weight_offset > (u32)session->ncp_mem_buf->size) {
					ret = -EINVAL;
					npu_uerr("weight_offset is invalid, offset(0x%x), ncp_daddr(0x%x)\n",
						session, (u32)weight_offset, (u32)session->ncp_mem_buf->size);
					goto p_err;
				}
				(av + address_vector_index)->m_addr = weight_offset + ncp_daddr;

				(*WGT_av + WGT_cnt)->av_index = address_vector_index;
				weight_size = (av + address_vector_index)->size;
				if ((weight_offset + weight_size) > (u32)session->ncp_mem_buf->size) {
					npu_err("weight_offset(0x%x) + weight size (0x%x) seems to go beyond ncp size(0x%x)\n",
							weight_offset, weight_size, (u32)session->ncp_mem_buf->size);
					return -EFAULT;
				}
				(*WGT_av + WGT_cnt)->size = weight_size;
				(*WGT_av + WGT_cnt)->size = (av + address_vector_index)->size;
				(*WGT_av + WGT_cnt)->daddr = weight_offset + ncp_daddr;
				(*WGT_av + WGT_cnt)->vaddr = weight_offset + ncp_vaddr;
				(*WGT_av + WGT_cnt)->memory_type = memory_type;
				npu_utrace("(*WGT_av + %u)->av_index = %u\n"
					"(*WGT_av + %u)->size = %zu\n"
					"(*WGT_av + %u)->daddr = 0x%pad\n"
					"(*WGT_av + %u)->vaddr = 0x%pK\n",
					session,
					WGT_cnt, (*WGT_av + WGT_cnt)->av_index,
					WGT_cnt, (*WGT_av + WGT_cnt)->size,
					WGT_cnt, &((*WGT_av + WGT_cnt)->daddr),
					WGT_cnt, (*WGT_av + WGT_cnt)->vaddr);
				WGT_cnt++;
				break;
			}
		default:
			break;
		}
	}
	session->IOFM_cnt = IFM_cnt + OFM_cnt;
	return ret;
p_err:
	return ret;
}

int __make_IFM_info(struct npu_session *session, struct temp_av **temp_IFM_av)
{
	int ret = 0;
	u32 i = 0;
	u32 j = 0;
	u32 IFM_cnt = session->IFM_cnt;
	struct addr_info *IFM_addr;

	for (i = 0; i < VISION_MAX_BUFFER; i++) {
		IFM_addr = kcalloc(IFM_cnt, sizeof(struct addr_info), GFP_KERNEL);
		if (!IFM_addr) {
				npu_err("failed in __make_IFM_info(ENOMEM)\n");
				ret = -ENOMEM;
				goto p_err;
			}
		session->IFM_info[i].addr_info = IFM_addr;
		for (j = 0; j < IFM_cnt; j++) {
			(IFM_addr + j)->av_index = (*temp_IFM_av + j)->index;
			(IFM_addr + j)->size = (*temp_IFM_av + j)->size;
			(IFM_addr + j)->pixel_format = (*temp_IFM_av + j)->pixel_format;
			(IFM_addr + j)->width = (*temp_IFM_av + j)->width;
			(IFM_addr + j)->height = (*temp_IFM_av + j)->height;
			(IFM_addr + j)->channels = (*temp_IFM_av + j)->channels;
			(IFM_addr + j)->stride = (*temp_IFM_av + j)->stride;
		}
	}

p_err:
	return ret;
}


int __make_OFM_info(struct npu_session *session, struct temp_av **temp_OFM_av)
{
	int ret = 0;
	u32 i = 0;
	u32 j = 0;
	u32 OFM_cnt = session->OFM_cnt;
	struct addr_info *OFM_addr;

	for (i = 0; i < VISION_MAX_BUFFER; i++) {
		OFM_addr = kcalloc(OFM_cnt, sizeof(struct addr_info), GFP_KERNEL);
		if (!OFM_addr) {
				npu_err("failed in __make_OFM_info(ENOMEM)\n");
				ret = -ENOMEM;
				goto p_err;
			}
		session->OFM_info[i].addr_info = OFM_addr;
		for (j = 0; j < OFM_cnt; j++) {
			(OFM_addr + j)->av_index = (*temp_OFM_av + j)->index;
			(OFM_addr + j)->size = (*temp_OFM_av + j)->size;
			(OFM_addr + j)->pixel_format = (*temp_OFM_av + j)->pixel_format;
			(OFM_addr + j)->width = (*temp_OFM_av + j)->width;
			(OFM_addr + j)->height = (*temp_OFM_av + j)->height;
			(OFM_addr + j)->channels = (*temp_OFM_av + j)->channels;
			(OFM_addr + j)->stride = (*temp_OFM_av + j)->stride;
		}
	}

p_err:
	return ret;
}

int __ion_alloc_IOFM(struct npu_session *session, struct temp_av **temp_IFM_av, struct temp_av **temp_OFM_av)
{
	int ret = 0;
	u32 i = 0;
	u32 j = 0;
	struct npu_memory_buffer *IOFM_mem_buf;
	mem_opt_e opt = IOFM_TYPE;

	u32 IFM_cnt = session->IFM_cnt;
	u32 IOFM_cnt = session->IOFM_cnt;

	IOFM_mem_buf = kzalloc(sizeof(struct npu_memory_buffer) * VISION_MAX_BUFFER, GFP_KERNEL);
	if (!IOFM_mem_buf) {
		npu_err("failed in __ion_alloc_IOFM(ENOMEM)\n");
		ret = -ENOMEM;
		return ret;
	}
	npu_udbg("ion alloc IOFM(0x%pK)\n", session, IOFM_mem_buf);

	for (i = 0; i < VISION_MAX_BUFFER; i++) {
		(IOFM_mem_buf + i)->size = IOFM_cnt * sizeof(struct address_vector);
		ret = npu_memory_alloc(session->memory, IOFM_mem_buf + i);
		if (ret) {
			npu_uerr("npu_memory_alloc is fail(%d).\n", session, ret);
			goto p_err;
		}
		npu_udbg("(IOFM_mem_buf + %d)->vaddr(0x%pK), daddr(0x%pad), size(%zu)\n",
			session, i, (IOFM_mem_buf + i)->vaddr, &(IOFM_mem_buf + i)->daddr, (IOFM_mem_buf + i)->size);
	}

	for (i = 0; i < IOFM_cnt; i++) {
		for (j = 0; j < VISION_MAX_BUFFER; j++) {
			if (i < IFM_cnt) {
				(((struct address_vector *)((IOFM_mem_buf + j)->vaddr)) + i)->index = (*temp_IFM_av + i)->index;
				(((struct address_vector *)((IOFM_mem_buf + j)->vaddr)) + i)->size = (*temp_IFM_av + i)->size;
			} else {
				(((struct address_vector *)((IOFM_mem_buf + j)->vaddr)) + i)->index = (*temp_OFM_av + (i - IFM_cnt))->index;
				(((struct address_vector *)((IOFM_mem_buf + j)->vaddr)) + i)->size = (*temp_OFM_av + (i - IFM_cnt))->size;
			}
		}
	}
	ret = add_ion_mem(session, IOFM_mem_buf, opt);
	return ret;
p_err:
	__ion_release(session->memory, IOFM_mem_buf, i);
	if (IOFM_mem_buf)
		kfree(IOFM_mem_buf);
	return ret;
}

int __make_IMB_info(struct npu_session *session, struct npu_memory_buffer *IMB_mem_buf, struct temp_av **temp_IMB_av)
{
	int ret = 0;
	u32 i = 0;
	u32 IMB_cnt = session->IMB_cnt;
	struct addr_info *IMB_info;

	IMB_info = kcalloc(IMB_cnt, sizeof(struct addr_info), GFP_KERNEL);
	if (!IMB_info) {
		npu_err("failed in __make_IMB_info(ENOMEM)\n");
		ret = -ENOMEM;
		return ret;
	}

	session->IMB_info = IMB_info;

	for (i = 0; i < IMB_cnt; i++) {
		(IMB_info + i)->av_index = (*temp_IMB_av + i)->index;
		(IMB_info + i)->vaddr = (IMB_mem_buf + i)->vaddr;
		(IMB_info + i)->daddr = (IMB_mem_buf + i)->daddr;
		(IMB_info + i)->size = (IMB_mem_buf + i)->size;
		(IMB_info + i)->pixel_format = (*temp_IMB_av + i)->pixel_format;
		(IMB_info + i)->width = (*temp_IMB_av + i)->width;
		(IMB_info + i)->height = (*temp_IMB_av + i)->height;
		(IMB_info + i)->channels = (*temp_IMB_av + i)->channels;
		(IMB_info + i)->stride = (*temp_IMB_av + i)->stride;
	}
	return ret;
}

int __ion_alloc_IMB(struct npu_session *session, struct temp_av **temp_IMB_av)
{
	int ret = 0;
	u32 i = 0;
	u32 address_vector_offset;
	struct ncp_header *ncp;
	struct address_vector *av;
	char *ncp_vaddr;
	u32 IMB_cnt = session->IMB_cnt;
	struct npu_memory_buffer *IMB_mem_buf;

	mem_opt_e opt = IMB_TYPE;

	ncp_vaddr = (char *)session->ncp_mem_buf->vaddr;
	ncp = (struct ncp_header *)ncp_vaddr;
	address_vector_offset = session->address_vector_offset;

	av = (struct address_vector *)(ncp_vaddr + address_vector_offset);

	IMB_mem_buf = kcalloc(IMB_cnt, sizeof(struct npu_memory_buffer), GFP_KERNEL);
	if (!IMB_mem_buf) {
		npu_err("failed in __ion_alloc_IMB(ENOMEM)\n");
		ret = -ENOMEM;
		return ret;
	}

	for (i = 0; i < IMB_cnt; i++) {
		(IMB_mem_buf + i)->size = (*temp_IMB_av + i)->size;
		ret = npu_memory_alloc(session->memory, IMB_mem_buf + i);
		if (ret) {
			npu_uerr("npu_memory_alloc is fail(%d).\n", session, ret);
			goto p_err;
		}
		(av + (*temp_IMB_av + i)->index)->m_addr = (IMB_mem_buf + i)->daddr;
		npu_udbg("(IMB_mem_buf + %d)->vaddr(0x%pK), daddr(0x%pad), size(%zu)\n",
			session, i, (IMB_mem_buf + i)->vaddr, &(IMB_mem_buf + i)->daddr, (IMB_mem_buf + i)->size);
	}
	ret = add_ion_mem(session, IMB_mem_buf, opt);
	ret = __make_IMB_info(session, IMB_mem_buf, temp_IMB_av);
	return ret;
p_err:
	__ion_release(session->memory, IMB_mem_buf, i);
	if (IMB_mem_buf)
		kfree(IMB_mem_buf);
	return ret;
}

void npu_session_ion_sync_for_device(struct npu_memory_buffer *pbuf, off_t offset, size_t size, enum dma_data_direction dir)
{
	if (pbuf->vaddr) {
		BUG_ON((offset < 0) || (offset > pbuf->size));
		BUG_ON((offset + size) < size);
		BUG_ON((size > pbuf->size) || ((offset + size) > pbuf->size));
		__dma_map_area(pbuf->vaddr + offset, size, dir);
	}
}

int __config_session_info(struct npu_session *session)
{
	int ret = 0;
	u32 i = 0;
	u32 direction;

	struct temp_av *temp_IFM_av;
	struct temp_av *temp_OFM_av;
	struct temp_av *temp_IMB_av;
	struct addr_info *WGT_av;
	struct npu_memory_buffer *IMB_mem_buf;

	ret = __pilot_parsing_ncp(session, &session->IFM_cnt, &session->OFM_cnt, &session->IMB_cnt, &session->WGT_cnt);

	temp_IFM_av = kcalloc(session->IFM_cnt, sizeof(struct temp_av), GFP_KERNEL);
	temp_OFM_av = kcalloc(session->OFM_cnt, sizeof(struct temp_av), GFP_KERNEL);
	temp_IMB_av = kcalloc(session->IMB_cnt, sizeof(struct temp_av), GFP_KERNEL);
	WGT_av = kcalloc(session->WGT_cnt, sizeof(struct addr_info), GFP_KERNEL);

	session->WGT_info = WGT_av;

	session->ss_state |= BIT(NPU_SESSION_STATE_WGT_KALLOC);

	if (!temp_IFM_av || !temp_OFM_av || !temp_IMB_av || !WGT_av) {
		npu_err("failed in __config_session_info(ENOMEM)\n");
		ret = -ENOMEM;
		goto p_err;
	}

	ret = __second_parsing_ncp(session, &temp_IFM_av, &temp_OFM_av, &temp_IMB_av, &WGT_av);
	if (ret) {
		npu_uerr("fail(%d) in second_parsing_ncp\n", session, ret);
		goto p_err;
	}

	ret += __make_IFM_info(session, &temp_IFM_av);
	ret += __make_OFM_info(session, &temp_OFM_av);
	session->ss_state |= BIT(NPU_SESSION_STATE_IOFM_KALLOC);
	if (ret) {
		npu_uerr("fail(%d) in __make_IOFM_info\n", session, ret);
		goto p_err;
	}

	ret = __ion_alloc_IOFM(session, &temp_IFM_av, &temp_OFM_av);
	if (ret) {
		npu_uerr("fail(%d) in __ion_alloc_IOFM\n", session, ret);
		goto p_err;
	}
	session->ss_state |= BIT(NPU_SESSION_STATE_IOFM_ION_ALLOC);

	ret = __ion_alloc_IMB(session, &temp_IMB_av);
	if (ret) {
		npu_uerr("fail(%d) in __ion_alloc_IMB\n", session, ret);
		goto p_err;
	}
	session->ss_state |= BIT(NPU_SESSION_STATE_IMB_ION_ALLOC);

	direction = DMA_TO_DEVICE;
	npu_session_ion_sync_for_device(session->ncp_mem_buf, 0, session->ncp_mem_buf->size, direction);

	IMB_mem_buf = session->IMB_mem_buf;
	for (i = 0; i < session->IMB_cnt; i++)
		npu_session_ion_sync_for_device(IMB_mem_buf + i, 0, (IMB_mem_buf + i)->size, direction);

p_err:
	if (temp_IFM_av)
		kfree(temp_IFM_av);
	if (temp_OFM_av)
		kfree(temp_OFM_av);
	if (temp_IMB_av)
		kfree(temp_IMB_av);

	return ret;
}

int npu_session_s_graph(struct npu_session *session, struct vs4l_graph *info)
{
	int ret = 0;
	BUG_ON(!session);
	BUG_ON(!info);
	ret = __get_session_info(session, info);
	if (unlikely(ret)) {
		npu_uerr("invalid in __get_session_info\n", session);
		goto p_err;
	}
	ret = __config_session_info(session);
	if (unlikely(ret)) {
		npu_uerr("invalid in __config_session_info\n", session);
		goto p_err;
	}
	return ret;
p_err:
	npu_uerr("Clean-up buffers for graph\n", session);
	__release_graph_ion(session);
	return ret;
}

int npu_session_start(struct npu_queue *queue)
{
	int ret = 0;
	struct npu_vertex_ctx *vctx;
	struct npu_session *session;

	BUG_ON(!queue);

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	if ((!vctx) || (!session)) {
		ret = -EINVAL;
		return ret;
	}

	session->ss_state |= BIT(NPU_SESSION_STATE_START);
	return ret;
}

int npu_session_NW_CMD_STREAMON(struct npu_session *session)
{
	nw_cmd_e nw_cmd = NPU_NW_CMD_STREAMON;

	BUG_ON(!session);

	profile_point1(PROBE_ID_DD_NW_RECEIVED, session->uid, 0, nw_cmd);
	session->nw_result.result_code = NPU_NW_JUST_STARTED;
	npu_session_put_nw_req(session, nw_cmd);

	wait_event(session->wq, session->nw_result.result_code != NPU_NW_JUST_STARTED);
	profile_point1(PROBE_ID_DD_NW_NOTIFIED, session->uid, 0, nw_cmd);

	return 0;
}

int npu_session_streamoff(struct npu_queue *queue)
{
	BUG_ON(!queue);
	return 0;
}

int npu_session_NW_CMD_STREAMOFF(struct npu_session *session)
{
	struct npu_vertex_ctx *vctx;
	struct npu_vertex *vertex;
	struct npu_device *device;
	nw_cmd_e nw_cmd = NPU_NW_CMD_STREAMOFF;

	BUG_ON(!session);

	/* check npu_device emergency error */
	vctx = &session->vctx;
	vertex = vctx->vertex;
	device = container_of(vertex, struct npu_device, vertex);

	npu_udbg("sending STREAM OFF command.\n", session);
	profile_point1(PROBE_ID_DD_NW_RECEIVED, session->uid, 0, nw_cmd);
	session->nw_result.result_code = NPU_NW_JUST_STARTED;
	npu_session_put_nw_req(session, nw_cmd);
	wait_event(session->wq, session->nw_result.result_code != NPU_NW_JUST_STARTED);
	profile_point1(PROBE_ID_DD_NW_NOTIFIED, session->uid, 0, nw_cmd);

	if (npu_device_is_emergency_err(device)) {
		npu_udbg("NPU DEVICE IS EMERGENCY ERROR !\n", session);
		npu_udbg("sending CLEAR_CB command.\n", session);
		npu_session_put_nw_req(session, NPU_NW_CMD_CLEAR_CB);
		/* Clear CB has no notify function */
	}

	return 0;
}

int npu_session_stop(struct npu_queue *queue)
{
	struct npu_session *session;
	struct npu_vertex_ctx *vctx;

	BUG_ON(!queue);

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	session->ss_state |= BIT(NPU_SESSION_STATE_STOP);

	return 0;
}

int npu_session_format(struct npu_queue *queue, struct vs4l_format_list *flist)
{
	int ret = 0;
	u32 i = 0;
	struct npu_session *session;
	struct npu_vertex_ctx *vctx;

	struct ncp_header *ncp;
	struct address_vector *av;
	struct memory_vector *mv;

	char *ncp_vaddr;
	struct vs4l_format *formats;
	struct addr_info *FM_av;
	u32 address_vector_offset;
	u32 address_vector_cnt;

	u32 memory_vector_offset;
	u32 memory_vector_cnt;

	u32 cal_size;
	u32 bpp;
	u32 channels;
	u32 width;
	u32 height;

	u32 FM_cnt;

	BUG_ON(!queue);
	BUG_ON(!flist);

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	BUG_ON(!vctx);
	BUG_ON(!session);

	if (session->ncp_mem_buf == NULL) {
		ret = -EFAULT;
		goto p_err;
	}

	ncp_vaddr = (char *)session->ncp_mem_buf->vaddr;
	ncp = (struct ncp_header *)ncp_vaddr;

	address_vector_offset = session->address_vector_offset;
	address_vector_cnt = session->address_vector_cnt;

	memory_vector_offset = session->memory_vector_offset;
	memory_vector_cnt = session->memory_vector_cnt;

	mv = (struct memory_vector *)(ncp_vaddr + memory_vector_offset);
	av = (struct address_vector *)(ncp_vaddr + address_vector_offset);

	formats = flist->formats;

	if (flist->direction == VS4L_DIRECTION_IN) {
		FM_av = session->IFM_info[0].addr_info;
		FM_cnt = session->IFM_cnt;
	} else {
		FM_av = session->OFM_info[0].addr_info;
		FM_cnt = session->OFM_cnt;
	}

	if (flist->count != FM_cnt) {
		ret = -EINVAL;
		goto p_err;
	}

	for (i = 0; i < FM_cnt; i++) {
		(mv + (FM_av + i)->av_index)->stride = (formats + i)->stride;
		(mv + (FM_av + i)->av_index)->cstride = (formats + i)->cstride;

		bpp = (formats + i)->pixel_format;
		channels = (formats + i)->channels;
		width = (formats + i)->width;
		height = (formats + i)->height;
		cal_size = (bpp / 8) * channels * width * height;

		npu_udbg("dir(%d), av_index(%d)\n", session, flist->direction, (FM_av + i)->av_index);
		npu_udbg("dir(%d), av_size(%zu), cal_size(%u)\n", session, flist->direction, (FM_av + i)->size, cal_size);
#ifndef SYSMMU_FAULT_TEST
		if ((FM_av + i)->size > cal_size) {
			npu_uinfo("in_size(%zu), cal_size(%u) invalid\n", session, (FM_av + i)->size, cal_size);
			ret = NPU_ERR_DRIVER(NPU_ERR_SIZE_NOT_MATCH);
			goto p_err;
		}
#endif
	}

	if (flist->direction == VS4L_DIRECTION_IN) {
		session->ss_state |= BIT(NPU_SESSION_STATE_FORMAT_IN);
	} else {
		session->ss_state |= BIT(NPU_SESSION_STATE_FORMAT_OT);
	}
p_err:
	return ret;
}

int npu_session_NW_CMD_LOAD(struct npu_session *session)
{
	int ret = 0;
	nw_cmd_e nw_cmd = NPU_NW_CMD_LOAD;

	if (!session) {
		npu_err("invalid session\n");
		ret = -EINVAL;
		return ret;
	}

	profile_point1(PROBE_ID_DD_NW_RECEIVED, session->uid, 0, nw_cmd);
	session->nw_result.result_code = NPU_NW_JUST_STARTED;
	npu_session_put_nw_req(session, nw_cmd);
	wait_event(session->wq, session->nw_result.result_code != NPU_NW_JUST_STARTED);
	profile_point1(PROBE_ID_DD_NW_NOTIFIED, session->uid, 0, nw_cmd);
	return ret;
}

int __update_qbuf_IOFM_daddr(struct npu_session *session, struct addr_info *IFM_addr, struct addr_info *OFM_addr)
{
	int ret = 0;
	u32 i = 0;
	struct address_vector *IOFM_av;
	struct npu_memory_buffer *IOFM_mem_buf;
	u32 IFM_cnt = session->IFM_cnt;
	u32 IOFM_cnt = session->IOFM_cnt;

	int cur_IOFM_idx = session->qbuf_IOFM_idx;

	if (cur_IOFM_idx == -1 || cur_IOFM_idx >= VISION_MAX_BUFFER) {
		ret = -EINVAL;
		goto p_err;
	}

	IOFM_mem_buf = session->IOFM_mem_buf;
	IOFM_av = (struct address_vector *) ((IOFM_mem_buf + cur_IOFM_idx)->vaddr);

	for (i = 0; i < IOFM_cnt; i++) {
		if (i < IFM_cnt) {
			(IOFM_av + i)->m_addr = (IFM_addr + i)->daddr;
			npu_udbg("update_IOFM_ion, m_addr(%d/0x%x)\n", session, i, (IOFM_av + i)->m_addr);
		} else {
			(IOFM_av + i)->m_addr = (OFM_addr + (i - IFM_cnt))->daddr;
		}
	}
p_err:
	return ret;
}

static int npu_session_queue(struct npu_queue *queue, struct vb_container_list *incl, struct vb_container_list *otcl)
{
	int ret = 0;
	struct npu_session *session;
	struct npu_vertex_ctx *vctx;

	struct mbox_process_dat *mbox_process_dat;

	struct av_info *IFM_info;
	struct av_info *OFM_info;

	struct addr_info *IFM_addr;
	struct addr_info *OFM_addr;

	int IOFM_idx = incl->index;

	frame_cmd_e frame_cmd = NPU_FRAME_CMD_Q;

	BUG_ON(!queue);
	BUG_ON(!incl);
	/*BUG_ON(!incl->index >= NPU_MAX_FRAME);*/
	BUG_ON(!otcl);
	/*BUG_ON(!otcl->index >= NPU_MAX_FRAME);*/

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	session->qbuf_IOFM_idx = IOFM_idx;

	IFM_info = &session->IFM_info[incl->index];
	IFM_addr = IFM_info->addr_info;
	IFM_info->IOFM_buf_idx = IOFM_idx;

	OFM_info = &session->OFM_info[otcl->index];
	OFM_addr = OFM_info->addr_info;
	OFM_info->IOFM_buf_idx = IOFM_idx;

	/* Save profile data */
	profile_point1(PROBE_ID_DD_FRAME_EQ, session->uid, incl->id, incl->direction);
	profile_point1(PROBE_ID_DD_FRAME_EQ, session->uid, otcl->id, otcl->direction);

	ret = __update_qbuf_IOFM_daddr(session, IFM_addr, OFM_addr);
	if (ret) {
		goto p_err;
	}

	mbox_process_dat = &session->mbox_process_dat;
	mbox_process_dat->address_vector_cnt = session->IOFM_cnt;
	mbox_process_dat->address_vector_start_daddr = ((session->IOFM_mem_buf) + IOFM_idx)->daddr;

	ret = npu_session_put_frame_req(session, queue, incl, otcl, frame_cmd, IFM_info, OFM_info);

	IFM_info->state = SS_BUF_STATE_QUEUE;
	OFM_info->state = SS_BUF_STATE_QUEUE;

	return 0;
p_err:
	return ret;
}

static int npu_session_deque(struct npu_queue *queue, struct vb_container_list *clist)
{
	int ret = 0;

	struct npu_session *session;
	struct npu_vertex_ctx *vctx;

	struct av_info *FM_info;

	BUG_ON(!queue);
	BUG_ON(!clist);

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	if (clist->index >= VISION_MAX_BUFFER) {
		ret = -EINVAL;
		goto p_err;
	}

	if (clist->direction == VS4L_DIRECTION_IN) {
		FM_info = &session->IFM_info[clist->index];
	} else {
		FM_info = &session->OFM_info[clist->index];
	}

	FM_info->state = SS_BUF_STATE_DEQUEUE;
	session->dqbuf_IOFM_idx = clist->index;
	/* Save profile data */
	profile_point1(PROBE_ID_DD_FRAME_DQ, session->uid, clist->id, clist->direction);

	npu_udbg("success in %s()\n", session, __func__);
p_err:
	return ret;
}

const struct npu_queue_ops npu_queue_ops = {
	.start		= npu_session_start,
	.stop		= npu_session_stop,
	.format	= npu_session_format,
	.queue		= npu_session_queue,
	.deque		= npu_session_deque,
	.streamoff	= npu_session_streamoff,
};

static int npu_session_control(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_request(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_process(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_cancel(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_done(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_get_resource(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_put_resource(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

static int npu_session_update_param(struct npu_session *session, struct npu_frame *frame)
{
	int ret = 0;

	return ret;
}

const struct npu_session_ops npu_session_ops = {
	.control	= npu_session_control,
	.request	= npu_session_request,
	.process	= npu_session_process,
	.cancel	= npu_session_cancel,
	.done		= npu_session_done,
	.get_resource	= npu_session_get_resource,
	.put_resource	= npu_session_put_resource,
	.update_param	= npu_session_update_param
};

static int npu_session_prepare(struct vb_queue *q, struct vb_container_list *clist)
{
	int ret = 0;
	struct npu_queue *queue = q->private_data;
	struct npu_vertex_ctx *vctx;
	struct npu_session *session;

	struct vb_buffer *buffer;
	struct av_info *FM_info;
	struct addr_info *FM_addr;
	struct vb_container *container;

	u32 FM_cnt, count;
	u32 i, j;
	u32 buf_cnt;

	BUG_ON(!queue);
	BUG_ON(!clist);

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	if (clist->index >= VISION_MAX_BUFFER) {
		ret = -EINVAL;
		goto p_err;
	}

	if (clist->direction == VS4L_DIRECTION_IN) {
		FM_info = &session->IFM_info[clist->index];
		FM_cnt = session->IFM_cnt;
	} else {
		FM_info = &session->OFM_info[clist->index];
		FM_cnt = session->OFM_cnt;
	}

	count = clist->count;

	if (FM_cnt != count) {
		npu_uerr("dir(%d), FM_cnt(%u) != count(%u)\n", session, clist->direction, FM_cnt, count);
		ret = -EINVAL;
		goto p_err;
	}

	FM_addr = FM_info->addr_info;
	for (i = 0; i < count ; i++) {
		container = &clist->containers[i];
		buf_cnt = container->count;
		for (j = 0; j < buf_cnt; j++) {
			buffer = &container->buffers[j];
			(FM_addr + i)->daddr = buffer->daddr;
			(FM_addr + i)->vaddr = buffer->vaddr;
		}
	}
	FM_info->address_vector_cnt = i;
	FM_info->state = SS_BUF_STATE_PREPARE;
p_err:
	return ret;
}

static int npu_session_unprepare(struct vb_queue *q, struct vb_container_list *clist)
{
	int ret = 0;
	struct npu_queue *queue = q->private_data;
	struct npu_vertex_ctx *vctx;
	struct npu_session *session;

	struct av_info *FM_info;

	BUG_ON(!queue);
	BUG_ON(!clist);

	vctx = container_of(queue, struct npu_vertex_ctx, queue);
	session = container_of(vctx, struct npu_session, vctx);

	if (clist->index >= VISION_MAX_BUFFER) {
		ret = -EINVAL;
		goto p_err;
	}

	if (clist->direction == VS4L_DIRECTION_IN) {
		FM_info = &session->IFM_info[clist->index];
	} else {
		FM_info = &session->OFM_info[clist->index];
	}

	FM_info->state = SS_BUF_STATE_UNPREPARE;
p_err:
	return ret;
}

/* S_PARAM handler list definition - Chain of responsibility pattern */
typedef npu_s_param_ret (*fn_s_param_handler)(struct npu_session *, struct vs4l_param *, int *);
fn_s_param_handler s_param_handlers[] = {
	fw_test_s_param_handler,
	npu_qos_param_handler,
	NULL	/* NULL termination is required to denote EOL */
};

int npu_session_param(struct npu_session *session, struct vs4l_param_list *plist)
{
	npu_s_param_ret		ret;
	int			retval = 0;
	struct vs4l_param	*param;
	u32			i;
	fn_s_param_handler	*p_fn;

	/* Search over each set param request over handlers */
	for (i = 0; i < plist->count; i++) {
		param = &plist->params[i];
		npu_udbg("Try set param [%u/%u] - target: [0x%x]\n",
			session, i+1, plist->count, param->target);

		for (p_fn = s_param_handlers; *p_fn != NULL; p_fn++) {
			ret = (*p_fn)(session, param, &retval);
			if (ret != S_PARAM_NOMB) {
				/* Handled */
				npu_udbg("Handled by handler at [%pK] ret = %d\n",
					session, *p_fn, ret);
				break;
			}
		}
		if (*p_fn == NULL) {
			npu_uwarn("No handler defined for target [%u]. Skipping.\n",
				session, param->target);
			/* Continue process others but retuen value is -EINVAL */
			retval = -EINVAL;
		}
	}

	return retval;
}

const struct vb_ops vb_ops = {
	.buf_prepare = npu_session_prepare,
	.buf_unprepare = npu_session_unprepare
};

int npu_session_register_undo_cb(struct npu_session *session, session_cb cb)
{
	BUG_ON(!session);

	session->undo_cb = cb;

	return 0;
}

int npu_session_execute_undo_cb(struct npu_session *session)
{
	BUG_ON(!session);

	if (session->undo_cb)
		session->undo_cb(session);

	return 0;
}

int npu_session_undo_open(struct npu_session *session)
{
	int ret = 0;

	BUG_ON(!session);

	if (session->ss_state < BIT(NPU_SESSION_STATE_OPEN)) {
		ret = -EINVAL;
		goto p_err;
	}

	if (session->ss_state <= BIT(NPU_SESSION_STATE_OPEN))
		goto session_free;

	npu_sessionmgr_unregID(session->cookie, session);

session_free:
	kfree(session);
	session = NULL;
p_err:
	return ret;
}

int npu_session_undo_s_graph(struct npu_session *session)
{
	int ret = 0;

	if (!_undo_s_graph_each_state(session)) {
		npu_session_register_undo_cb(session, NULL);
		goto next_cb;
	}
	ret = -EINVAL;
	return ret;
next_cb:
	npu_session_execute_undo_cb(session);
	return ret;
}

int npu_session_undo_close(struct npu_session *session)
{
	int ret = 0;

	if (!_undo_close_each_state(session)) {
		npu_session_register_undo_cb(session, npu_session_undo_s_graph);
		goto next_cb;
	}
	ret = -EINVAL;
	return ret;
next_cb:
	npu_session_execute_undo_cb(session);
	return ret;
}

