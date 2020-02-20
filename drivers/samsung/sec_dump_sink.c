#include <linux/proc_fs.h>
#include <linux/sec_debug.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/memblock.h>
#include <linux/of.h>
#include <linux/of_reserved_mem.h>

/* Override the default prefix for the compatibility with other models */
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "sec_debug."

#define ENABLE_SDCARD_RAMDUMP		(0x73646364)
#define MAGIC_SDR_FOR_MINFORM		(0x3)
#define ENABLE_STORAGE_RAMDUMP		(0x42544456)
#define MAGIC_STR_FOR_MINFORM		(0xC)
#define OFFSET_SDR_FOR_MINFORM		(0x0)
#define MASK_SDR_FOR_MINFORM		(0xF)

#define SHA256_DIGEST_SIZE      32
#define SHA256_BLOCK_SIZE       64
#define SHA256_DIGEST_LENGTH  SHA256_DIGEST_SIZE

static unsigned int dump_sink;
static unsigned int upload_count;
static int initialized;

static int sec_sdcard_ramdump(const char *val, const struct kernel_param *kp)
{
	kstrtouint(val, 16, &dump_sink);
	pr_crit("%s: %s %x\n", __func__, val, dump_sink);

	if (!initialized)
		return 0;

	if (dump_sink == ENABLE_SDCARD_RAMDUMP) {
		sec_set_reboot_magic(MAGIC_SDR_FOR_MINFORM, OFFSET_SDR_FOR_MINFORM, MASK_SDR_FOR_MINFORM);
	} else if (dump_sink == ENABLE_STORAGE_RAMDUMP) {
		sec_set_reboot_magic(MAGIC_STR_FOR_MINFORM, OFFSET_SDR_FOR_MINFORM, MASK_SDR_FOR_MINFORM);
	}
	return 0;
}

static const struct kernel_param_ops sec_dump_sink_ops = {
	.set	= sec_sdcard_ramdump,
	.get	= param_get_uint,
};

module_param_cb(dump_sink, &sec_dump_sink_ops, &dump_sink, 0600);

static phys_addr_t sec_rdx_bootdev_paddr;
static unsigned int sec_rdx_bootdev_size;
static DEFINE_MUTEX(rdx_bootdev_mutex);

static void sec_free_rdx_bootdev(phys_addr_t paddr, u64 size)
{
/* caution : this fuction should be called in rdx_bootdev_mutex protected region. */
	unsigned long pfn_start, pfn_end, pfn_idx;
	int ret;

	pr_info("start (0x%p, 0x%llx)\n", paddr, size);

	if (!sec_rdx_bootdev_paddr) {
		pr_err("reserved addr is null\n");
		goto out;
	}

	if (!sec_rdx_bootdev_size) {
		pr_err("reserved size is zero\n");
		goto out;
	}

	if (paddr < sec_rdx_bootdev_paddr) {
		pr_err("paddr is not valid\n");
		goto out;
	}

	if (paddr + size > sec_rdx_bootdev_paddr + sec_rdx_bootdev_size) {
		pr_err("range is not valid\n");
		goto out;
	}

	memset(phys_to_virt(paddr), 0, size);

	ret = memblock_free(paddr, size);
	if (ret) {
		pr_err("memblock_free failed (ret : %d)\n", ret);
		goto out;
	}

	free_memsize_reserved(paddr, size);

	pfn_start = paddr >> PAGE_SHIFT;
	pfn_end = (paddr + size) >> PAGE_SHIFT;
	for (pfn_idx = pfn_start; pfn_idx < pfn_end; pfn_idx++)
		free_reserved_page(pfn_to_page(pfn_idx));

	if (sec_rdx_bootdev_paddr == paddr) {
		sec_rdx_bootdev_paddr = 0;
	}

	sec_rdx_bootdev_size -= size;

out:
	pr_info("end\n");
}

static ssize_t sec_rdx_bootdev_proc_write(struct file *file,
	const char __user *buf, size_t count, loff_t *ppos)
{
	int err = 0;
	struct fiemap *pfiemap;
	phys_addr_t paddr;
	u64 size;

	mutex_lock(&rdx_bootdev_mutex);

	paddr = sec_rdx_bootdev_paddr;
	size = sec_rdx_bootdev_size;

	if (!sec_rdx_bootdev_paddr) {
		pr_err("sec_rdx_bootdev_paddr is null\n");
		err = -EFAULT;
		goto out;
	}

	if (!buf) {
		err = -ENODEV;
	} else {
		if (count > sec_rdx_bootdev_size) {
			pr_err("size is wrong %llu > %llu\n", count, sec_rdx_bootdev_size);
			err = -EINVAL;
			goto out;
		}

		if (copy_from_user(phys_to_virt(sec_rdx_bootdev_paddr),
				buf, count)) {
			pr_err("copy_from_user failed\n");
			err = -EFAULT;
			goto out;
		}

		pfiemap = phys_to_virt(sec_rdx_bootdev_paddr) + SHA256_DIGEST_LENGTH;
		paddr = virt_to_phys(&pfiemap->fm_extents[pfiemap->fm_mapped_extents]);
		if (paddr <
			sec_rdx_bootdev_paddr + sec_rdx_bootdev_size) {
			paddr = ALIGN(paddr, PAGE_SIZE);
			size = paddr - sec_rdx_bootdev_paddr;
			size = sec_rdx_bootdev_size - size;
		}
	}

out:
	sec_free_rdx_bootdev(paddr, size);
	mutex_unlock(&rdx_bootdev_mutex);
	return err < 0 ? err : count;
}

static const struct file_operations sec_rdx_bootdev_fops = {
	.owner = THIS_MODULE,
	.write = sec_rdx_bootdev_proc_write,
};

static int __init sec_set_upload_count(char *arg)
{
	get_option(&arg, &upload_count);
	return 0;
}
early_param("sec_debug.upload_count", sec_set_upload_count);


static int sec_upload_count_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d", upload_count);
	return 0;
}

static int sec_upload_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, sec_upload_count_show, NULL);
}

static const struct file_operations sec_upload_count_proc_fops = {
	.open = sec_upload_count_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init sec_map_rdx_bootdev_region(void)
{
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
	return 0;
#else
	struct device_node *parent, *node;
	int ret = 0;
	u32 temp[3];

	parent = of_find_node_by_path("/reserved-memory");
	if (!parent) {
		pr_err("%s, failed to find reserved-memory node\n", __func__);
		return -EINVAL;
	}

	node = of_find_node_by_name(parent, "sec_rdx_bootdev");
	if (!node) {
		pr_err("%s, failed to find sec_rdx_bootdev\n", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(node, "reg", &temp[0], 3);
	if (ret) {
		pr_err("%s, failed to get address from node %d\n", __func__, ret);
		return -1;
	}

	mutex_lock(&rdx_bootdev_mutex);
	sec_rdx_bootdev_paddr = temp[0];
	sec_rdx_bootdev_paddr = (sec_rdx_bootdev_paddr << 32);
	sec_rdx_bootdev_paddr += temp[1];
	sec_rdx_bootdev_size = temp[2];

	pr_info("%s, sec_rdx_bootdev : 0x%p, 0x%llx\n", __func__,
		sec_rdx_bootdev_paddr, sec_rdx_bootdev_size);

	if (!sec_rdx_bootdev_paddr || !sec_rdx_bootdev_size) {
		pr_err("%s, failed to get address from node\n", __func__);
		mutex_unlock(&rdx_bootdev_mutex);
		return -1;
	}

	if (!sec_debug_enter_upload()) {
		pr_info("%s, sec_debug is not enabled\n", __func__);
		sec_free_rdx_bootdev(sec_rdx_bootdev_paddr, sec_rdx_bootdev_size);
		mutex_unlock(&rdx_bootdev_mutex);
		return 0;
	}
	memset(phys_to_virt(sec_rdx_bootdev_paddr), 0, SHA256_DIGEST_LENGTH);

	mutex_unlock(&rdx_bootdev_mutex);
	return 0;
#endif
}
arch_initcall_sync(sec_map_rdx_bootdev_region);

static int __init sec_dump_sink_init(void)
{
	struct proc_dir_entry *entry;

	entry = proc_create("rdx_bootdev", 0222, NULL, &sec_rdx_bootdev_fops);

	if (!entry) {
		pr_err("%s: fail to create proc entry (rdx_bootdev)\n", __func__);
		return -ENOMEM;
	}
	entry = proc_create("upload_count", 0444, NULL, &sec_upload_count_proc_fops);

	if (!entry) {
		pr_err("%s: fail to create proc entry (upload_count)\n", __func__);
		return -ENOMEM;
	}
	pr_info("%s: success to create proc entry\n", __func__);
	initialized = 1;
	if (dump_sink == ENABLE_SDCARD_RAMDUMP) {
		sec_set_reboot_magic(MAGIC_SDR_FOR_MINFORM, OFFSET_SDR_FOR_MINFORM, MASK_SDR_FOR_MINFORM);
	} else if (dump_sink == ENABLE_STORAGE_RAMDUMP) {
		sec_set_reboot_magic(MAGIC_STR_FOR_MINFORM, OFFSET_SDR_FOR_MINFORM, MASK_SDR_FOR_MINFORM);
	}
	pr_info("%s: dump_sink set to 0x%x\n", __func__, dump_sink);

	return 0;
}
device_initcall(sec_dump_sink_init);
