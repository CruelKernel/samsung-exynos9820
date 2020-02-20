/**
 * sdp_cache.c
 *
 * Store inode number of all the sensitive files that have been opened since booting
 */
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/pagemap.h>

#include "../fscrypt_private.h"

extern int dek_is_locked(int engine_id);

static struct kmem_cache *cachep;

LIST_HEAD(list_head);
static spinlock_t list_lock;

struct _entry {
	u32 engine_id;
	struct super_block *sb;
	unsigned long ino; // inode number
	struct list_head list;
};

static void *dump_entry_list_locked(const char *msg)
{
	struct list_head *e;
	int i = 0;

	DEK_LOGD("%s(%s) : ", __func__, msg);
	spin_lock(&list_lock);
	list_for_each(e, &list_head) {
		struct _entry *entry = list_entry(e, struct _entry, list);

		if (entry)
			DEK_LOGD("[%d: %p %lu] ", i, entry->sb, entry->ino);
		i++;
	}
	spin_unlock(&list_lock);
	DEK_LOGD("\n");
	DEK_LOGE("%s(%s) : entry-num(%d)", __func__, msg, i);

	return NULL;
}

static struct _entry *find_entry_locked(struct super_block *sb, unsigned long ino)
{
	struct list_head *e;

	spin_lock(&list_lock);
	list_for_each(e, &list_head) {
		struct _entry *entry = list_entry(e, struct _entry, list);

		if (entry->sb == sb)
			if (entry->ino == ino) {
				spin_unlock(&list_lock);
				return entry;
			}
	}
	spin_unlock(&list_lock);

	return NULL;
}

static int add_entry_locked(u32 engine_id, struct super_block *sb, unsigned long ino)
{
	struct _entry *entry = NULL;

	DEK_LOGD("%s(sb:%p, ino:%lu) entered\n", __func__, sb, ino);
	if (find_entry_locked(sb, ino))
		return -EEXIST;
	entry = kmem_cache_alloc(cachep, GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	memset(entry, 0, sizeof(struct _entry));
	INIT_LIST_HEAD(&entry->list);
	entry->engine_id = engine_id;
	entry->sb = sb;
	entry->ino = ino;
	spin_lock(&list_lock);
	list_add_tail(&entry->list, &list_head);
	spin_unlock(&list_lock);

	dump_entry_list_locked("entry-added");

	return 0;
}


static void _init(void *foo)
{
}

int fscrypt_sdp_cache_init(void)
{
	spin_lock_init(&list_lock);
	INIT_LIST_HEAD(&list_head);

	cachep = kmem_cache_create("sdp_sensitive_ino_entry_cache",
			 sizeof(struct _entry),
			 0,
			 (SLAB_RECLAIM_ACCOUNT|SLAB_PANIC|
			 SLAB_MEM_SPREAD),
			 _init);

	if (cachep == NULL) {
		DEK_LOGE("%s failed\n", __func__);
		return -1;
	}

	return 0;
}

void fscrypt_sdp_cache_add_inode_num(struct inode *inode)
{
	struct fscrypt_info *ci = inode->i_crypt_info;//This pointer must be loaded by get_encryption_info completely
	if (ci && ci->ci_sdp_info) {
		int res = add_entry_locked(ci->ci_sdp_info->engine_id, inode->i_sb, inode->i_ino);
		if (!res) {
			ci->ci_sdp_info->sdp_flags |= SDP_IS_INO_CACHED;
			DEK_LOGD("%s(ino:%lu) sb:%p res:%d\n", __func__, inode->i_ino, inode->i_sb, res);
		}
	}
}

void fscrypt_sdp_cache_remove_inode_num(struct inode *inode)
{
	if (inode) {
		struct list_head *e, *temp;
		struct fscrypt_info *ci = inode->i_crypt_info;

		spin_lock(&list_lock);
//		list_for_each(e, &list_head) {
		list_for_each_safe(e, temp, &list_head) {
			struct _entry *entry = list_entry(e, struct _entry, list);

			if (entry->sb == inode->i_sb) {
				if (entry->ino == inode->i_ino) {
					list_del(&entry->list);
					if (ci && ci->ci_sdp_info) {
						ci->ci_sdp_info->sdp_flags &= ~(SDP_IS_INO_CACHED);
					}
					spin_unlock(&list_lock);
					kmem_cache_free(cachep, entry);
					DEK_LOGD("%s(ino:%lu) sb:%p\n", __func__, inode->i_ino, inode->i_sb);
					return;
				}
			}
		}
		spin_unlock(&list_lock);
	}
}

struct inode_drop_task_param {
	int engine_id;
};

static unsigned long invalidate_mapping_pages_retry(struct address_space *mapping,
		pgoff_t start, pgoff_t end, int retries)
{
	DEK_LOGD("freeing [%s] sensitive inode[mapped pagenum = %lu]\n",
			mapping->host->i_sb->s_type->name,
			mapping->nrpages);
retry:
	invalidate_mapping_pages(mapping, start, end);
	DEK_LOGD("invalidate_mapping_pages [%lu] remained\n",
			mapping->nrpages);

	if (mapping->nrpages != 0) {
		if (retries > 0) {
			DEK_LOGD("[%lu] mapped pages remained in sensitive inode, retry..\n",
					mapping->nrpages);
			retries--;
			msleep(100);
			goto retry;
		}
	}

	return mapping->nrpages;
}

static void wait_file_io_retry(struct inode *inode)
{
	bool ret;

retry:
	ret = fscrypt_sdp_is_cache_releasable(inode);

	if (!ret) {
		msleep(100);
		goto retry;
	}
}

static int inode_drop_task(void *arg)
{
	struct inode_drop_task_param *param = arg;
	int engine_id = param->engine_id;

	struct _entry *entry, *entry_safe;
	struct inode *inode;
	struct fscrypt_info *ci;
	LIST_HEAD(drop_list);

	DEK_LOGD("%s(engine_id:%d) entered\n", __func__, engine_id);
	dump_entry_list_locked("inode_drop");

	spin_lock(&list_lock);
	list_for_each_entry_safe(entry, entry_safe, &list_head, list) {
		if (entry && entry->engine_id == engine_id) {
			list_del_init(&entry->list);
			list_add_tail(&entry->list, &drop_list);
		}
	}
	spin_unlock(&list_lock);

	if (!list_empty(&drop_list)) {
		//Traverse local list and operate safely
		list_for_each_entry_safe(entry, entry_safe, &drop_list, list) {
			inode = ilookup(entry->sb, entry->ino);
			if (!inode) {
				DEK_LOGD("%s inode(%lu) not found\n", __func__, entry->ino);
				goto err;
			}

			DEK_LOGD("%s found ino:%lu sb:%p\n", __func__, entry->ino, entry->sb);
			ci = inode->i_crypt_info;
			/*
			 * Instead of occuring BUG, skip the clearing only
			 * TODO: Must research later whether we can skip the logic in this case
			BUG_ON(!ci);
			BUG_ON(!ci->ci_sdp_info);
			*/
			if (!ci || !ci->ci_sdp_info) {
				DEK_LOGD("%s May be already cleared\n", __func__);
				goto free_icnt;
			}

			DEK_LOGD("%s found ino:%lu engine_id:%d sdp_flags:%x\n",
					__func__, entry->ino, ci->ci_sdp_info->engine_id, ci->ci_sdp_info->sdp_flags);

			if ((ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE) == 0) {
				DEK_LOGE("%s not sensitive file\n", __func__);
				goto free_icnt;
			}

			wait_file_io_retry(inode);
			if (filemap_write_and_wait(inode->i_mapping))
				DEK_LOGD("May failed to writeback");

			DEK_LOGD("%s invalidating...\n", __func__);
			if (invalidate_mapping_pages_retry(inode->i_mapping, 0, -1, 3) > 0) {
				DEK_LOGE("Failed to invalidate entire pages..");
			}
			fscrypt_sdp_unset_clearing_ongoing(inode);
free_icnt:
			iput(inode);
err:
			list_del(&entry->list);
			kmem_cache_free(cachep, entry);
		}
	}

	DEK_LOGD("%s complete...\n", __func__);
	dump_entry_list_locked("inode_drop(complete)");

	kzfree(param);
	return 0;
}

void fscrypt_sdp_cache_drop_inode_mappings(int engine_id)
{
	struct task_struct *task;
	struct inode_drop_task_param *param =
			kzalloc(sizeof(*param), GFP_KERNEL);

	param->engine_id = engine_id;
	task = kthread_run(inode_drop_task, param, "fscrypt_sdp_drop_cached");
	if (IS_ERR(task)) {
		DEK_LOGE("unable to create kernel thread fscrypt_sdp_drop_cached res:%ld\n",
				PTR_ERR(task));
	}
}

int fscrypt_sdp_file_not_readable(struct file *file)
{
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	struct fscrypt_info *ci = inode->i_crypt_info;
	int retval = 0;

	if (ci && ci->ci_sdp_info) {
		if (!S_ISDIR(inode->i_mode) && (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
			int is_locked = dek_is_locked(ci->ci_sdp_info->engine_id);
			if (is_locked) {
				wait_file_io_retry(inode);
				if (filemap_write_and_wait(inode->i_mapping))
					DEK_LOGD("May failed to writeback");

				invalidate_mapping_pages_retry(inode->i_mapping, 0, -1, 3);
				fscrypt_sdp_cache_remove_inode_num(inode);
				fscrypt_sdp_unset_clearing_ongoing(inode);
			} else {
				int is_ino_cached = 0;

				spin_lock(&ci->ci_sdp_info->sdp_flag_lock);
				if (ci->ci_sdp_info->sdp_flags & SDP_IS_CLEARING_ONGOING) {
					is_locked = 1;
				} else {
					ci->ci_sdp_info->sdp_flags |= SDP_IS_FILE_IO_ONGOING;
				}
				if (ci->ci_sdp_info->sdp_flags & SDP_IS_INO_CACHED)
					is_ino_cached = 1;
				spin_unlock(&ci->ci_sdp_info->sdp_flag_lock);

				if (!is_ino_cached)
					fscrypt_sdp_cache_add_inode_num(inode);
			}
			retval = is_locked;
		}
	}

	return retval;
}

int fscrypt_sdp_file_not_writable(struct file *file)
{
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	struct fscrypt_info *ci = inode->i_crypt_info;
	int retval = 0;

	if (!S_ISDIR(inode->i_mode) && ci && ci->ci_sdp_info &&
			(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		spin_lock(&ci->ci_sdp_info->sdp_flag_lock);
		if (ci->ci_sdp_info->sdp_flags & SDP_IS_CLEARING_ONGOING) {
			retval = -EINVAL;
		} else {
			ci->ci_sdp_info->sdp_flags |= SDP_IS_FILE_IO_ONGOING;
		}
		spin_unlock(&ci->ci_sdp_info->sdp_flag_lock);
	}

	return retval;
}

void fscrypt_sdp_unset_file_io_ongoing(struct file *file)
{
	struct address_space *mapping = file->f_mapping;
	struct inode *inode = mapping->host;
	struct fscrypt_info *ci = inode->i_crypt_info;

	if (ci && ci->ci_sdp_info && (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		ci->ci_sdp_info->sdp_flags &= ~(SDP_IS_FILE_IO_ONGOING);
	}
}

void fscrypt_sdp_unset_clearing_ongoing(struct inode *inode)
{
	struct fscrypt_info *ci = inode->i_crypt_info;

	if (ci && ci->ci_sdp_info && (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		ci->ci_sdp_info->sdp_flags &= ~(SDP_IS_CLEARING_ONGOING);
	}
}

bool fscrypt_sdp_is_cache_releasable(struct inode *inode)
{
	bool retval = false;
	struct fscrypt_info *ci = inode->i_crypt_info;

	if (ci && ci->ci_sdp_info && (ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		spin_lock(&ci->ci_sdp_info->sdp_flag_lock);
		if (ci->ci_sdp_info->sdp_flags & SDP_IS_FILE_IO_ONGOING ||
				ci->ci_sdp_info->sdp_flags & SDP_IS_CLEARING_ONGOING) {
			retval = false;
		} else {
			ci->ci_sdp_info->sdp_flags |= SDP_IS_CLEARING_ONGOING;
			retval = true;
		}
		spin_unlock(&ci->ci_sdp_info->sdp_flag_lock);
	}

	return retval;
}

bool fscrypt_sdp_is_locked_sensitive_inode(struct inode *inode)
{
	struct fscrypt_info *ci = inode->i_crypt_info;

	if (ci && ci->ci_sdp_info &&
			(ci->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE)) {
		DEK_LOGD("Time to drop the inode..\n");
		return true;
	} else
		return false;
}

int __fscrypt_sdp_d_delete(const struct dentry *dentry, int dek_is_locked) {
	struct inode *inode = d_inode(dentry);

	if (inode && dek_is_locked) {
		struct fscrypt_info *crypt_info = inode->i_crypt_info;

		if (crypt_info && crypt_info->ci_sdp_info &&
				crypt_info->ci_sdp_info->sdp_flags & SDP_DEK_IS_SENSITIVE) {
			DEK_LOGD("Time to delete dcache....\n");
			return 1;
		}
	}

	return 0;
}
EXPORT_SYMBOL(__fscrypt_sdp_d_delete);

int fscrypt_sdp_d_delete_wrapper(const struct dentry *dentry) {
struct inode *inode = d_inode(dentry);
	struct fscrypt_info *ci = inode ? inode->i_crypt_info : NULL;
#if 1
	int is_locked = 1; /* TODO: */
#else
	int is_locked = (ci && ci->ci_sdp_info) ? dek_is_locked(ci->ci_sdp_info->engine_id) : 0;
#endif

	if (ci && __fscrypt_sdp_d_delete(dentry, is_locked))
		return 1;

	return 0;
}

void fscrypt_sdp_drop_inode(struct inode *inode) {
	inode->i_state |= I_WILL_FREE;
	spin_unlock(&inode->i_lock);
	write_inode_now(inode, 1);
	spin_lock(&inode->i_lock);
	WARN_ON(inode->i_state & I_NEW);
	inode->i_state &= ~I_WILL_FREE;
}
