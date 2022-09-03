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



#define pr_fmt(fmt) "SDP_MM: %s: " fmt, __func__
#include <asm/current.h>
#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mm_types.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <sdp/sdp_mm.h>
#define SDP_MM_DEV	"sdp_mm"

#define SENSITIVE 1
#define PER_USER_RANGE 100000

extern int dek_is_sdp_uid(uid_t uid);
static struct class *driver_class;
static dev_t sdp_mm_device_no;
struct sdp_mm_control {
    struct list_head  sdp_proc_list_head;
    spinlock_t        sdp_proc_list_lock;
    struct cdev cdev;
    unsigned int sensitive_proc_list_len;
    unsigned int proc_id[MAX_SENSITIVE_PROC];
};

static struct sdp_mm_control sdp_mm;

static int32_t sdp_mm_query_proc_loaded(void __user *argp)
{
    int32_t ret  = 0;
    struct task_struct *p = NULL;
    unsigned long flags = 0;
    struct sdp_mm_sensitive_proc_list_resp sdp_resp = {0};
    /* foreach process check if 'sensitive' flag is set in task/mm_struct */
    /* Copy the relevant information needed for loading the image */
    spin_lock_irqsave(&sdp_mm.sdp_proc_list_lock, flags);
    for_each_process(p) {
        if (p->sensitive == SENSITIVE) {
            sdp_resp.sensitive_proc_list[sdp_resp.sensitive_proc_list_len] = p->pid;
            sdp_resp.sensitive_proc_list_len++;
        }
        if (sdp_resp.sensitive_proc_list_len >= MAX_SENSITIVE_PROC) {
            break;
        }
    }
    spin_unlock_irqrestore(&sdp_mm.sdp_proc_list_lock, flags);

    if (copy_to_user(argp, &sdp_resp, sizeof(sdp_resp))) {
        pr_err("SDP_MM: copy_to_user_failed\n");
        ret = -EFAULT;
    }
    return ret;
}

int32_t sdp_mm_set_process_sensitive(unsigned int proc_id)
{
    int32_t ret = 0;
    struct task_struct *task = NULL;
    uid_t uid;

    rcu_read_lock();
    /* current.task.sensitive = 1 */
    task = pid_task(find_vpid(proc_id), PIDTYPE_PID);
    if (task) {
        get_task_struct(task);
        uid = from_kuid(&init_user_ns, task_uid(task));
        if (((uid/PER_USER_RANGE) <= 199)  && ((uid/PER_USER_RANGE) >= 100)) {
            if (dek_is_sdp_uid(uid)) {
                task->sensitive = SENSITIVE;
                printk("SDP_MM: set the process as sensitive\n");
            } else {
                printk("SDP_MM: not a SDP process, failed to mark sensitive\n");
                ret = -EINVAL;
                goto task_err;
            }
        }
    } else {
        printk("SDP_MM; task not present\n");
        ret = -EINVAL;
        goto task_err;
    }

task_err:
    if (task)
        put_task_struct(task);
    rcu_read_unlock();
    return ret;
}

EXPORT_SYMBOL(sdp_mm_set_process_sensitive);

static int32_t sdp_mm_set_proc_sensitive(void __user *argp)
{
    unsigned int proc_id = 0;
    if (copy_from_user(&proc_id, (void __user *)argp,
                sizeof(unsigned int))) {
        pr_err("SDP_MM: copy_from_user failed\n");
        return -EFAULT;
    }

    printk("SDP_MM: sensitive process id is %d\n", proc_id);
    if (sdp_mm.sensitive_proc_list_len < MAX_SENSITIVE_PROC) {
        sdp_mm.proc_id[sdp_mm.sensitive_proc_list_len] = proc_id;
        sdp_mm.sensitive_proc_list_len++;
    }

    return sdp_mm_set_process_sensitive(proc_id);
}

static long sdp_mm_ioctl(struct file *file, unsigned cmd,
        unsigned long arg)
{
    int ret = 0;
    void __user *argp = (void __user *) arg;

    printk("SDP_MM: Entering ioctl\n");

    switch (cmd) {
    case SDP_MM_IOCTL_PROC_SENSITIVE_QUERY_REQ: {
        printk("SDP_MM: SENSITIVE_PROC_QUERY\n");
        ret = sdp_mm_query_proc_loaded(argp);
        break;
    }
    case SDP_MM_IOCTL_SET_SENSITIVE_PROC_REQ: {
        printk("SDP_MM: SET_PROC_SENSITIVE\n");
        ret = sdp_mm_set_proc_sensitive(argp);
        break;
    }
    default:
        pr_err("Invalid IOCTL: %d\n", cmd);
        return -EINVAL;
    }

    return ret;
}

static int sdp_mm_open(struct inode *inode, struct file *file)
{
    int ret = 0;
    printk("SDP_MM: entering open\n");
    return ret;
}

static const struct file_operations sdp_mm_fops = {
        .owner = THIS_MODULE,
        .unlocked_ioctl = sdp_mm_ioctl,
        .open = sdp_mm_open,
};

static int sdp_mm_probe(struct platform_device *pdev)
{
    int rc;
    struct device *class_dev;
    printk("SDP_MM: entering probe\n");
    rc = alloc_chrdev_region(&sdp_mm_device_no, 0, 1, SDP_MM_DEV);

    if (rc < 0) {
        pr_err("alloc_chrdev_region failed %d\n", rc);
        return rc;
    }

    driver_class = class_create(THIS_MODULE, SDP_MM_DEV);

    if (IS_ERR(driver_class)) {
        rc = -ENOMEM;
        pr_err("SDP_MM: class_create failed %d\n", rc);
        goto exit_unreg_chrdev_region;
    }

    class_dev = device_create(driver_class, NULL, sdp_mm_device_no, NULL,
            SDP_MM_DEV);

    if (!class_dev) {
        pr_err("SDP_MM: class_device_create failed %d\n", rc);
        rc = -ENOMEM;
        goto exit_destroy_class;
    }

    cdev_init(&sdp_mm.cdev, &sdp_mm_fops);
    sdp_mm.cdev.owner = THIS_MODULE;
    rc = cdev_add(&sdp_mm.cdev, MKDEV(MAJOR(sdp_mm_device_no), 0), 1);

    if (rc < 0) {
        pr_err("SDP_MM: cdev_add failed %d\n", rc);
        goto exit_destroy_device;
    }

    INIT_LIST_HEAD(&sdp_mm.sdp_proc_list_head);
    spin_lock_init(&sdp_mm.sdp_proc_list_lock);

    return 0;

exit_destroy_device:
    device_destroy(driver_class, sdp_mm_device_no);

exit_destroy_class:
    class_destroy(driver_class);

exit_unreg_chrdev_region:
    unregister_chrdev_region(sdp_mm_device_no, 1);
    return rc;
}

static int sdp_mm_remove(struct platform_device *pdev)
{
    int ret = 0;

    cdev_del(&sdp_mm.cdev);
    device_destroy(driver_class, sdp_mm_device_no);
    class_destroy(driver_class);
    unregister_chrdev_region(sdp_mm_device_no, 1);

    return ret;
}

static struct platform_driver sdp_mm_plat_driver = {
    .probe = sdp_mm_probe,
    .remove = sdp_mm_remove,
    .driver = {
        .name = "sdp_mm",
        .owner = THIS_MODULE,
    },
};

static int __init sdp_mm_init(void)
{
    int rc;
    struct device *class_dev;

    printk("SDP_MM: entering************************************************\n");
    rc = alloc_chrdev_region(&sdp_mm_device_no, 0, 1, SDP_MM_DEV);

    if (rc < 0) {
        pr_err("SDP_MM: Alloc_chrdev_region failed %d\n", rc);
        return rc;
    }

    driver_class = class_create(THIS_MODULE, SDP_MM_DEV);

    if (IS_ERR(driver_class)) {
        rc = -ENOMEM;
        pr_err("SDP_MM: class_create failed %d\n", rc);
        goto exit_unreg_chrdev_region;
    }

    class_dev = device_create(driver_class, NULL, sdp_mm_device_no, NULL,
            SDP_MM_DEV);
    if (!class_dev) {
        pr_err("SDP_MM: class_device_create failed %d\n", rc);
       rc = -ENOMEM;
        goto exit_destroy_class;
    }

    cdev_init(&sdp_mm.cdev, &sdp_mm_fops);
    sdp_mm.cdev.owner = THIS_MODULE;

    rc = cdev_add(&sdp_mm.cdev, MKDEV(MAJOR(sdp_mm_device_no), 0), 1);
    if (rc < 0) {
        pr_err("SDP_MM: cdev_add failed %d\n", rc);
        goto exit_destroy_device;
    }

    INIT_LIST_HEAD(&sdp_mm.sdp_proc_list_head);
    spin_lock_init(&sdp_mm.sdp_proc_list_lock);
    return 0;

exit_destroy_device:
    device_destroy(driver_class, sdp_mm_device_no);

exit_destroy_class:
    class_destroy(driver_class);

exit_unreg_chrdev_region:
    unregister_chrdev_region(sdp_mm_device_no, 1);
    return rc;
}

static void __exit sdp_mm_exit(void)
{
    platform_driver_unregister(&sdp_mm_plat_driver);
}

module_init(sdp_mm_init);
module_exit(sdp_mm_exit);
MODULE_AUTHOR("HP");
MODULE_LICENSE("GPL");
