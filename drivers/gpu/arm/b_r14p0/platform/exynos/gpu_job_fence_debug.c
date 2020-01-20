/* drivers/gpu/arm/.../platform/exynos/gpu_job_fence_debug.c
 *
 * Copyright 2018 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series JOB & FENCE debug driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_job_fence_debug.c
 * JOB FENCE DEBUG
 */

#include <mali_kbase.h>

#ifdef CONFIG_MALI_SEC_JOB_STATUS_CHECK
#if defined(CONFIG_MALI_FENCE_DEBUG)
#error YOU MUST turn off MALI_FENCE_DEBUG.
#endif

#include "backend/gpu/mali_kbase_jm_rb.h"

#if defined(CONFIG_SYNC)
int gpu_job_fence_status_dump(struct sync_fence *timeout_fence);
static char *gpu_fence_status_to_string(int status)
{
	if (status == 0)
		return "signaled";
	else if (status > 0)
		return "active";
	else
		return "error";
}

void gpu_fence_debug_check_dependency_atom(struct kbase_jd_atom *katom)
{
	struct kbase_context *kctx = katom->kctx;
	struct device *dev = kctx->kbdev->dev;
	int i;

	for (i = 0; i < 2; i++) {
		struct kbase_jd_atom *dep;

		list_for_each_entry(dep, &katom->dep_head[i], dep_item[i]) {
			if (dep->status == KBASE_JD_ATOM_STATE_UNUSED ||
					dep->status == KBASE_JD_ATOM_STATE_COMPLETED)
				continue;

			if ((dep->core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_TRIGGER ||
					(dep->core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_WAIT) {
				struct sync_fence *fence = dep->fence;
				int status = atomic_read(&fence->status);

				/* Found blocked fence. */
				dev_warn(dev,
						"\t\t\t\t--- Atom %d fence [%p] %s: %s, fence type = 0x%x\n",
						kbase_jd_atom_id(kctx, dep),
						fence, fence->name,
						gpu_fence_status_to_string(status),
						dep->core_req);
			} else {
				dev_warn(dev,
						"\t\t\t\t--- Atom %d\n", kbase_jd_atom_id(kctx, dep));
			}

			/* gpu_fence_debug_check_dependency_atom(dep); */
		}
	}
}

int gpu_job_fence_status_dump(struct sync_fence *timeout_fence)
{
	struct device *dev;
	struct list_head *entry;
	const struct list_head *kbdev_list;
	struct kbase_device *kbdev = NULL;
	struct kbasep_kctx_list_element *element;
	struct kbase_context *kctx;
	struct sync_fence *fence;
	unsigned long lflags;
	int i;
	int cnt[5] = {0,};

	/* dev_warn(dev,"GPU JOB STATUS DUMP\n"); */

	kbdev_list = kbase_dev_list_get();

	if (kbdev_list == NULL) {
		kbase_dev_list_put(kbdev_list);
		return -ENODEV;
	}

	list_for_each(entry, kbdev_list) {
		kbdev = list_entry(entry, struct kbase_device, entry);

		if (kbdev == NULL) {
			kbase_dev_list_put(kbdev_list);
			return -ENODEV;
		}

		dev = kbdev->dev;
		dev_warn(dev, "[%p] kbdev dev name : %s\n", kbdev, kbdev->devname);
		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry(element, &kbdev->kctx_list, link) {
			kctx = element->kctx;
			mutex_lock(&kctx->jctx.lock);
			dev_warn(dev, "\t[%p] kctx(%d_%d_%d)_jobs_nr(%d)\n", kctx, kctx->pid, kctx->tgid, kctx->id, kctx->jctx.job_nr);
			if (kctx->jctx.job_nr > 0) {
				for (i = BASE_JD_ATOM_COUNT-1; i > 0; i--) {
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_UNUSED) {
						cnt[0]++;
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED) {
						cnt[1]++;
						dev_warn(dev, "\t\t- [%p] Atom %d STATE_QUEUED\n", &kctx->jctx.atoms[i], i);
						/* dev_warn(dev, "		-- Atom %d slot_nr 0x%x coreref_state 0x%x core_req 0x%x event_code 0x%x gpu_rb_state 0x%x\n",
						   i, kctx->jctx.atoms[i].slot_nr, kctx->jctx.atoms[i].coreref_state, kctx->jctx.atoms[i].core_req, kctx->jctx.atoms[i].event_code, kctx->jctx.atoms[i].gpu_rb_state); */
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_IN_JS) {
						cnt[2]++;
						dev_warn(dev, "\t\t- [%p] Atom %d STATE_IN_JS\n", &kctx->jctx.atoms[i], i);
						dev_warn(dev, "\t\t\t-- Atom %d	slot_nr 0x%x coreref_state 0x%x core_req 0x%x event_code 0x%x gpu_rb_state 0x%x\n",
								i, kctx->jctx.atoms[i].slot_nr, kctx->jctx.atoms[i].coreref_state, kctx->jctx.atoms[i].core_req, kctx->jctx.atoms[i].event_code, kctx->jctx.atoms[i].gpu_rb_state);
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_HW_COMPLETED) {
						cnt[3]++;
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_COMPLETED) {
						cnt[4]++;
					}

					spin_lock_irqsave(&kctx->waiting_soft_jobs_lock, lflags);
					/* Print fence infomation */
					fence = kctx->jctx.atoms[i].fence;
					if (fence != NULL) {
						dev_warn(dev, "\t\t\t-- Atom %d Fence Info [%p] %s: %s, fence type = 0x%x, %s\n",
								i, fence, fence->name, gpu_fence_status_to_string(atomic_read(&fence->status)), kctx->jctx.atoms[i].core_req, (fence == timeout_fence) ? "***" : "  ");
					}
					/* Print dependency atom infomation */
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED || kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_IN_JS) {
						dev_warn(dev, "\t\t\t-- Dependency Atom List\n");
						gpu_fence_debug_check_dependency_atom(&kctx->jctx.atoms[i]);
					}
					spin_unlock_irqrestore(&kctx->waiting_soft_jobs_lock, lflags);
				}
				dev_warn(dev, "\t\t: ATOM STATE INFO : UNUSED(%d)_QUEUED(%d)_IN_JS(%d)_HW_COMPLETED(%d)_COMPLETED(%d)\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4]);
				cnt[0] = cnt[1] = cnt[2] = cnt[3] = cnt[4] = 0;
			}
			mutex_unlock(&kctx->jctx.lock);
		}
		mutex_unlock(&kbdev->kctx_list_lock);
		/* katom list in backed slot rb */
		kbase_gpu_dump_slots(kbdev);
	}

	if (timeout_fence != NULL)
		dev_warn(dev, "Timeout Fence *** [%p] %s: %s\n", timeout_fence, timeout_fence->name, gpu_fence_status_to_string(atomic_read(&timeout_fence->status)));

	kbase_dev_list_put(kbdev_list);

	return 0;
} /* #if defined(CONFIG_SYNC) */

#elif defined(CONFIG_SYNC_FILE)

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 10, 0))
#error YOU MUST turn off MALI_SEC_JOB_STATUS_CHECK
#endif

#include "mali_kbase_sync.h"

int gpu_job_fence_status_dump(struct sync_file *timeout_sync_file);
void gpu_fence_debug_check_dependency_atom(struct kbase_jd_atom *katom)
{
	struct kbase_context *kctx = katom->kctx;
	struct device *dev = kctx->kbdev->dev;
	int i;

	for (i = 0; i < 2; i++) {
		struct kbase_jd_atom *dep;

		list_for_each_entry(dep, &katom->dep_head[i], dep_item[i]) {
			if (dep->status == KBASE_JD_ATOM_STATE_UNUSED ||
					dep->status == KBASE_JD_ATOM_STATE_COMPLETED)
				continue;

			/* Found defendency fence & job */
			if ((dep->core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_TRIGGER) {
				struct kbase_sync_fence_info info;
				struct fence *fence;	/* trigger fence */
				if (!kbase_sync_fence_out_info_get(dep, &info)) {
					fence = info.fence;
					dev_warn(dev,
							"\t\t\t\t--- Atom %d fence_out [%p] %s: fence type = 0x%x, fence ctx = %llu, fence seqno = %u\n",
							kbase_jd_atom_id(kctx, dep),
							info.fence, info.name,
							dep->core_req,
							fence->context,
							fence->seqno);
				}
			} else if ((dep->core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_WAIT) {
				struct kbase_sync_fence_info info;
				struct fence *fence;	/* wait fence */
				if (!kbase_sync_fence_in_info_get(dep, &info)) {
					fence = info.fence;
					dev_warn(dev,
							"\t\t\t\t--- Atom %d fence_in [%p] %s: fence type = 0x%x, fence ctx = %llu, fence seqno = %u\n",
							kbase_jd_atom_id(kctx, dep),
							info.fence, info.name,
							dep->core_req,
							fence->context,
							fence->seqno);
				}
			} else {
				dev_warn(dev,
						"\t\t\t\t--- Atom %d\n", kbase_jd_atom_id(kctx, dep));
			}

			/* gpu_fence_debug_check_dependency_atom(dep); */
		}
	}
}

int gpu_job_fence_status_dump(struct sync_file *timeout_sync_file)
{
	struct device *dev;
	struct list_head *entry;
	const struct list_head *kbdev_list;
	struct kbase_device *kbdev = NULL;
	struct kbasep_kctx_list_element *element;
	struct kbase_context *kctx;
	struct kbase_sync_fence_info info_in;
	struct kbase_sync_fence_info info_out;
	struct fence *fence_in;
	struct fence *fence_out;
	unsigned long lflags;
	int i;
	int cnt[5] = {0,};
	bool check_fence;

	/* dev_warn(dev,"GPU JOB STATUS DUMP\n"); */

	kbdev_list = kbase_dev_list_get();

	if (kbdev_list == NULL) {
		kbase_dev_list_put(kbdev_list);
		return -ENODEV;
	}

	list_for_each(entry, kbdev_list) {
		kbdev = list_entry(entry, struct kbase_device, entry);

		if (kbdev == NULL) {
			kbase_dev_list_put(kbdev_list);
			return -ENODEV;
		}

		dev = kbdev->dev;
		dev_warn(dev, "[%p] kbdev dev name : %s\n", kbdev, kbdev->devname);
		mutex_lock(&kbdev->kctx_list_lock);
		list_for_each_entry(element, &kbdev->kctx_list, link) {
			kctx = element->kctx;
			mutex_lock(&kctx->jctx.lock);
			dev_warn(dev, "\t[%p] kctx(%d_%d_%d)_jobs_nr(%d)\n", kctx, kctx->pid, kctx->tgid, kctx->id, kctx->jctx.job_nr);
			if (kctx->jctx.job_nr > 0) {
				for (i = BASE_JD_ATOM_COUNT-1; i > 0; i--) {
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_UNUSED) {
						cnt[0]++;
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED) {
						cnt[1]++;
						dev_warn(dev, "\t\t- [%p] Atom %d STATE_QUEUED\n", &kctx->jctx.atoms[i], i);
						/* dev_warn(dev, "		-- Atom %d slot_nr 0x%x coreref_state 0x%x core_req 0x%x event_code 0x%x gpu_rb_state 0x%x\n",
						   i, kctx->jctx.atoms[i].slot_nr, kctx->jctx.atoms[i].coreref_state, kctx->jctx.atoms[i].core_req, kctx->jctx.atoms[i].event_code, kctx->jctx.atoms[i].gpu_rb_state); */
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_IN_JS) {
						cnt[2]++;
						dev_warn(dev, "\t\t- [%p] Atom %d STATE_IN_JS\n", &kctx->jctx.atoms[i], i);
						dev_warn(dev, "\t\t\t-- Atom %d	slot_nr 0x%x coreref_state 0x%x core_req 0x%x event_code 0x%x gpu_rb_state 0x%x\n",
								i, kctx->jctx.atoms[i].slot_nr, kctx->jctx.atoms[i].coreref_state, kctx->jctx.atoms[i].core_req, kctx->jctx.atoms[i].event_code, kctx->jctx.atoms[i].gpu_rb_state);
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_HW_COMPLETED) {
						cnt[3]++;
					} else if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_COMPLETED) {
						cnt[4]++;
					}

					spin_lock_irqsave(&kctx->waiting_soft_jobs_lock, lflags);
					/* Print fence infomation */
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED) {
						if ((kctx->jctx.atoms[i].core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_TRIGGER) {
							if (!kbase_sync_fence_out_info_get(&kctx->jctx.atoms[i], &info_out)) {
								fence_out = info_out.fence;
								if (timeout_sync_file != NULL && timeout_sync_file->fence != NULL) {
									if (fence_out == timeout_sync_file->fence)
										check_fence = true;
									else
										check_fence = false;
								} else
									check_fence = false;

								dev_warn(dev, "\t\t\t-- Atom %d Fence_out Info [%p] %s: fence type = 0x%x, fence ctx = %llu, fence seqno = %u, %s\n",
										i, info_out.fence, info_out.name, kctx->jctx.atoms[i].core_req, fence_out->context, fence_out->seqno, (check_fence == true) ? "***" : "  ");
							}
						}
						if ((kctx->jctx.atoms[i].core_req & BASE_JD_REQ_SOFT_JOB_TYPE) == BASE_JD_REQ_SOFT_FENCE_WAIT) {
							if (!kbase_sync_fence_in_info_get(&kctx->jctx.atoms[i], &info_in)) {
								fence_in = info_in.fence;
								if (timeout_sync_file != NULL && timeout_sync_file->fence != NULL) {
									if (fence_in == timeout_sync_file->fence)
										check_fence = true;
									else
										check_fence = false;
								} else
									check_fence = false;

								dev_warn(dev, "\t\t\t-- Atom %d Fence_in Info [%p] %s: fence type = 0x%x, fence ctx = %llu, fence seqno = %u, %s\n",
										i, info_in.fence, info_in.name, kctx->jctx.atoms[i].core_req, fence_in->context, fence_in->seqno, (check_fence == true) ? "***" : "  ");
							}
						}
					}
					/* Print dependency atom infomation */
					if (kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_QUEUED || kctx->jctx.atoms[i].status == KBASE_JD_ATOM_STATE_IN_JS) {
						dev_warn(dev, "\t\t\t-- Dependency Atom List\n");
						gpu_fence_debug_check_dependency_atom(&kctx->jctx.atoms[i]);
					}
					spin_unlock_irqrestore(&kctx->waiting_soft_jobs_lock, lflags);

				}
				dev_warn(dev, "\t\t: ATOM STATE INFO : UNUSED(%d)_QUEUED(%d)_IN_JS(%d)_HW_COMPLETED(%d)_COMPLETED(%d)\n", cnt[0], cnt[1], cnt[2], cnt[3], cnt[4]);
				cnt[0] = cnt[1] = cnt[2] = cnt[3] = cnt[4] = 0;
			}
			mutex_unlock(&kctx->jctx.lock);
		}
		mutex_unlock(&kbdev->kctx_list_lock);
		/* katom list in backed slot rb */
		kbase_gpu_dump_slots(kbdev);
	}

	if (timeout_sync_file != NULL) {
		dev_warn(dev, "Timeout Sync_file [%p] Sync_file name %s\n", timeout_sync_file, timeout_sync_file->name);
		dev_warn(dev, "Timeout Fence *** [%p] \n", timeout_sync_file->fence);
	}

	kbase_dev_list_put(kbdev_list);

	return 0;
}
#endif  /* #if defined(CONFIG_SYNC_FILE) */
#endif	/* #if CONFIG_MALI_SEC_JOB_STATUS_CHECK */

