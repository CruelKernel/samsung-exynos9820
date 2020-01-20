#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/uh.h>
#include <linux/bootmem.h>
#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif

static ssize_t uh_call_write(struct file *file, const char __user *buf,
		size_t count, loff_t *ppos)
{
	char buffer[128] = {0,};
	unsigned long missing;
	int result;
	u64 app_id = 0, command = 0, arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;

	if (sizeof(buffer) < count + 1)
		return -EFAULT;

	missing = copy_from_user(buffer, buf, count);
	if (missing)
		return -EFAULT;

	buffer[count] = '\0';

	result = sscanf(buffer, "%llu %llu %llu %llu %llu %llu\n",
			&app_id, &command, &arg0, &arg1, &arg2, &arg3);

	pr_alert("%s app_id:%llu, command:%llu, %llu, %llu, %llu, %llu\n", __func__,
			app_id, command, arg0, arg1, arg2, arg3);
	pr_alert("%s app_id:%llu, command:%llu, %llu, %llu, %llu, %llu\n", __func__,
			app_id, command, arg0, arg1, arg2, arg3);

	switch (app_id) {
		case APP_INIT:
			uh_call(UH_APP_INIT, command, arg0, arg1, arg2, arg3);
			break;
		case APP_SAMPLE:
			uh_call(UH_APP_SAMPLE, command, arg0, arg1, arg2, arg3);
			break;
		default:
			break;
	}

	return count;
}

static const struct file_operations uh_call_fops = {
	.write = uh_call_write,
};

static int __init uh_debugfs_init(void)
{
	int ret = 0;
	struct dentry *dir;
	struct dentry *file;

	dir = debugfs_create_dir("uh", NULL);
	if (IS_ERR_OR_NULL(dir)) {
		pr_err("failed to create uh dir in debugfs\n");
		ret = PTR_ERR(dir);
		goto init_exit;
	}

	file = debugfs_create_file("uh_call", 0660, dir, NULL,
				   &uh_call_fops);
	if (IS_ERR_OR_NULL(file)) {
		debugfs_remove(dir);
		pr_err("failed to create uh_call file for uh\n");
		ret = PTR_ERR(file);
		goto init_exit;
	}

init_exit:
	return ret;
}
late_initcall(uh_debugfs_init)
