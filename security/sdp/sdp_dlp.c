/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *
 * Sensitive Data Protection
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <sdp/dlp_ioctl.h>
#include <sdp/sdp_dlp.h> 

#define DLP_DUMP_LIST 1 //TODO

static char *DLP_FILE_EXTENSIONS[] = {"xls", "xlsx", "doc", "docx", "ppt", "pptx", "pdf", "jpg", "jpeg", "zip", "mp4", "txt", "asd", "xlam", "htm", "html", "mht", "eml", "msg", "hwp", "gul", "rtf", "mysingle", "png", "gif"};

struct dlp_struct {
	int user_id;
	long time;
	bool lock;
	char* extensions;
	struct list_head list;
	struct mutex list_mutex;
};
struct dlp_struct dlp_info;

#if DLP_DUMP_LIST
static void dlp_dump_list(void) {
	struct list_head *entry;
	struct dlp_struct *tmp;
	
	printk("============ debug ============\n");	

	mutex_lock(&dlp_info.list_mutex);
	
	list_for_each(entry, &dlp_info.list) {
		tmp = list_entry(entry, struct dlp_struct, list);

		printk("DLP : user_id : %d\n", tmp->user_id);
		printk("DLP : lock : %s\n", tmp->lock ? "true" : "false");
		if (tmp->extensions) {
			printk("DLP : extensions : %s\n", tmp->extensions);
		} else {
			printk("DLP : extensions : (empty)\n");
		}
	}
	mutex_unlock(&dlp_info.list_mutex);	
}
#endif

static struct dlp_struct *dlp_find_list(int user_id) {
	struct list_head *entry;
	struct dlp_struct *tmp;

	mutex_lock(&dlp_info.list_mutex);
	printk("DLP: user_id %d\n", user_id);
	list_for_each(entry, &dlp_info.list) {
		tmp = list_entry(entry, struct dlp_struct, list);
		printk("DLP: tmp->user_id %d\n", tmp->user_id);
		if(tmp->user_id == user_id) {
			printk("DLP: found user_id %d\n", user_id); // TODO : deleted
			mutex_unlock(&dlp_info.list_mutex);
			return tmp;
		}
	}
	mutex_unlock(&dlp_info.list_mutex);

	return NULL;
}

bool dlp_is_locked(int user_id) {
	struct dlp_struct *tmp;
	tmp = dlp_find_list(user_id);
	if(tmp){
		return tmp->lock;
	}
	else {
		return false;
	}
}

static struct dlp_struct *dlp_add_info(int user_id){
	struct dlp_struct *new_item;

	new_item = kmalloc(sizeof(struct dlp_struct), GFP_KERNEL);
	if(new_item == NULL) {
		printk("DLP: can't alloc memory for dlp_info\n");
		return NULL;
	}

	new_item->user_id = user_id;
	new_item->lock = false;
	new_item->extensions = NULL;

	mutex_init(&new_item->list_mutex);
	INIT_LIST_HEAD(&new_item->list);

	mutex_lock(&dlp_info.list_mutex);
	list_add_tail(&new_item->list, &dlp_info.list);
	mutex_unlock(&dlp_info.list_mutex);

	return new_item;
}

static int dlp_lock_setting(void __user *argp, bool lock) {
	int ret = 0;
	struct dlp_struct *tmp = NULL;
	struct _dlp_lock_set dlp_lock_set;

	ret = copy_from_user(&dlp_lock_set, (void __user *)argp, sizeof(dlp_lock_set));
	if(ret)	{
		printk("DLP : fail to copy_from_user : dlp_lock_setting\n");
		return -EFAULT;
	}

	tmp = dlp_find_list(dlp_lock_set.user_id);
	if(tmp == NULL){
		tmp = dlp_add_info(dlp_lock_set.user_id);
		if(tmp == NULL){
			return -EFAULT;
		}
	}
	mutex_lock(&dlp_info.list_mutex);
	tmp->lock = lock;
	mutex_unlock(&dlp_info.list_mutex);

	return 0;
};

static int dlp_extension_setting(void __user *argp) {
	int ret = 0;
	struct dlp_struct *tmp = NULL;
	struct _dlp_extension_set dlp_ext_set;

	ret = copy_from_user(&dlp_ext_set, (void __user *)argp, sizeof(dlp_ext_set));
	if(ret)	{
		printk("DLP : fail to copy_from_user : dlp_extension_setting\n");
		return -EFAULT;
	}

	printk("DLP: dlp_extension_setting called with user_id %d\n", dlp_ext_set.user_id);

	tmp = dlp_find_list(dlp_ext_set.user_id);
	if(tmp == NULL){
		tmp = dlp_add_info(dlp_ext_set.user_id);
		if(tmp == NULL){
			return -EFAULT;
		}
	}

	/* Delete old extensions and create new */
	mutex_lock(&dlp_info.list_mutex);
	if(tmp->extensions) {
		kfree(tmp->extensions);
		tmp->extensions = NULL;
	}

	if(strlen(dlp_ext_set.extensions)) {
		tmp->extensions = kmalloc(strlen(dlp_ext_set.extensions)+1, GFP_KERNEL);
		if (!tmp->extensions) {
			mutex_unlock(&dlp_info.list_mutex);
			return -EFAULT;
		}
	} else {
		mutex_unlock(&dlp_info.list_mutex);
		return 0;
	}

	strcpy(tmp->extensions, dlp_ext_set.extensions);
	mutex_unlock(&dlp_info.list_mutex);

	return 0;
}


static long dlp_ioctl(struct file *file, unsigned cmd,
		unsigned long arg) {
	int ret = 0;
	void __user *argp = (void __user *) arg;

	switch (cmd) {
	case DLP_LOCK_ENABLE: {
		printk("DLP: DLP_LOCK_ENABLE\n");
		ret = dlp_lock_setting(argp, true);
		break;
	}
	case DLP_LOCK_DISABLE: {
		printk("DLP: DLP_LOCK_DISABLE\n");
		ret = dlp_lock_setting(argp, false);
		break;
	}
	case DLP_EXTENSION_SET: {
		printk("DLP: DLP_EXTENSION_SET\n");
		ret = dlp_extension_setting(argp);
		break;
	}
	default:
		pr_err("DLP : Invalid IOCTL: %d\n", cmd);
		return -EINVAL;
	}

#if DLP_DUMP_LIST	
	dlp_dump_list(); 
#endif

	return ret;	
}

int dlp_isInterestedFile(int user_id, const char *filename){
	int i, j;
	char *ext, *ret;
	char lower[256];
	struct dlp_struct *item = NULL;
	ext = strrchr(filename, '.');

	printk("DLP: dlp_isInterestedFile : %s\n", filename);

	if(ext == NULL){
		printk("DLP: Ext not found\n");
		return -1;
	}else{
		ext++;
		strncpy(lower, ext, sizeof(lower)-1);
		lower[sizeof(lower)-1] = '\0';
		for (j=0; j < strlen(lower); j++) {
			lower[j] = tolower(lower[j]);
		}
		printk("DLP: extension : %s\n", lower);
	}

	item = dlp_find_list(user_id);
	if(item == NULL || item->extensions == NULL){
		//extensions for DLP is not set ...
		// check if the extension is matching with default extensions
		printk("DLP: extension set for DLP is not set\n");
		for(i=(sizeof(DLP_FILE_EXTENSIONS)/sizeof(char*) -1); i>=0; i--){
			if(strcmp(DLP_FILE_EXTENSIONS[i], lower)==0){
				return 0;
			}
		}	
		printk("DLP: extension not matching with default return -1\n");
		return -1;
	} 
	
	printk("DLP: extension = %s\n", item->extensions);

	strcat(lower, ","); // add comma, so that we compare each ext fully (doc vs. docx)
	mutex_lock(&dlp_info.list_mutex);
	ret = strstr(item->extensions, lower);
	mutex_unlock(&dlp_info.list_mutex);
	if (ret) {
		return 0;
	} else {
		return -1;
	}
}

const struct file_operations dlp_fops_evt = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = dlp_ioctl,
	.compat_ioctl = dlp_ioctl,
};

static struct miscdevice dlp_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sdp_dlp",
	.fops = &dlp_fops_evt,
};

static int __init dlp_ioctl_init(void) {
	int ret;

	ret = misc_register(&dlp_misc_dev);
	if (unlikely(ret)) {
		printk("DLP: failed to register sdp_dlp device!\n");
		return ret;
	}

	INIT_LIST_HEAD(&dlp_info.list);
	mutex_init(&dlp_info.list_mutex);

	printk("DLP: dlp_ioctl initialized\n");

	return 0;
}

static void __exit dlp_ioctl_exit(void) {
	misc_deregister(&dlp_misc_dev);
	printk("DLP: dlp_ioctl mics_deregister\n");
}

module_init(dlp_ioctl_init);
module_exit(dlp_ioctl_exit);
