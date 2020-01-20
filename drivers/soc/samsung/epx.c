#include <asm/map.h>
#include <asm/memory.h>
#include <asm/uaccess.h>

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/of_address.h>
#include <linux/of_reserved_mem.h>
#include <linux/memblock.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <linux/kallsyms.h>

#define S5P_VA_EPX	(VMALLOC_START + 0xF6000000 + 0x03000000)
#define EPX_SIZE	(1024 * 1024)

static struct vm_struct epx_early_vm;
static bool epx_loaded = false;

typedef unsigned long (*pfn_kallsyms_lookup_name)(const char *name);
typedef int (*pfn_binary_entry)(unsigned long address, pfn_kallsyms_lookup_name ksyms);

struct page_change_data {
        pgprot_t set_mask;
        pgprot_t clear_mask;
};

static int __init epx_rmem_remap(void)
{
        unsigned long i;
        pgprot_t prot = PAGE_KERNEL_EXEC;
        int page_size, ret;
        struct page *page;
        struct page **pages;

        page_size = epx_early_vm.size / PAGE_SIZE;
        pages = kzalloc(sizeof(struct page*) * page_size, GFP_KERNEL);
        page = phys_to_page(epx_early_vm.phys_addr);

        for (i = 0; i < page_size; i++)
                pages[i] = page++;

	ret = map_vm_area(&epx_early_vm, prot, pages);
	if (ret) {
		pr_err("EPX: failed to map virtual memory area!\n");
		kfree(pages);
		return -ENOMEM;
	}
	kfree(pages);

	epx_loaded = false;

	return 0;
}
postcore_initcall(epx_rmem_remap);

static int __init epx_mem_reserved_mem_setup(char *cmd)
{
	epx_early_vm.phys_addr = memblock_alloc(EPX_SIZE, SZ_4K);
	epx_early_vm.size = EPX_SIZE + SZ_4K;
	epx_early_vm.addr = (void *)S5P_VA_EPX;

	vm_area_add_early(&epx_early_vm);

	return 0;
}
__setup("epx_activate=", epx_mem_reserved_mem_setup);

static int epx_change_page_range(pte_t *ptep, pgtable_t token, unsigned long addr,
		void *data)
{
	struct page_change_data *cdata = data;
	pte_t pte = *ptep;

	pte = clear_pte_bit(pte, cdata->clear_mask);
	pte = set_pte_bit(pte, cdata->set_mask);

	set_pte(ptep, pte);
	return 0;
}

static int epx_change_memory_common(unsigned long addr, int numpages,
		pgprot_t set_mask, pgprot_t clear_mask)
{
	unsigned long start = addr;
	unsigned long size = PAGE_SIZE*numpages;
	unsigned long end = start + size;
	int ret;
	struct page_change_data data;

	if (!PAGE_ALIGNED(addr)) {
		start &= PAGE_MASK;
		end = start + size;
		WARN_ON_ONCE(1);
	}

	if (!numpages)
		return 0;

	data.set_mask = set_mask;
	data.clear_mask = clear_mask;

	ret = apply_to_page_range(&init_mm, start, size, epx_change_page_range,
			&data);

	flush_tlb_kernel_range(start, end);
	return ret;
}

static int epx_set_memory_rw(unsigned long addr, int numpages)
{
	return epx_change_memory_common(addr, numpages,
			__pgprot(PTE_WRITE),
			__pgprot(PTE_RDONLY));
}

static int epx_set_memory_nx(unsigned long addr, int numpages)
{
	return epx_change_memory_common(addr, numpages,
			__pgprot(PTE_PXN),
			__pgprot(0));
}

static int epx_set_memory_x(unsigned long addr, int numpages)
{
	return epx_change_memory_common(addr, numpages,
			__pgprot(0),
			__pgprot(PTE_PXN));
}

static ssize_t show_epx_activate(struct class *class,
		struct class_attribute *attr, char *buf)
{
	int ret = 0;
	long epx_fd;
	struct file *fp;
	mm_segment_t old_fs;
	loff_t pos = 0;
	unsigned int epx_signature = 0x585045;
	unsigned long long epx_offset;
	char *binary_buffer;
	void *epx_ptr;
	pfn_binary_entry binary_entry;

	if (epx_loaded)
		return 0;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	epx_fd = do_sys_open(AT_FDCWD, "/vendor/firmware/epx.bin", O_RDONLY | O_NOFOLLOW, 0664);
	if (epx_fd < 0) {
		pr_err("EPX : error to open file\n");
		ret = -EINVAL;
		goto finalize;
	}

	epx_set_memory_nx((unsigned long)epx_early_vm.addr, PFN_UP(EPX_SIZE));
	epx_set_memory_rw((unsigned long)epx_early_vm.addr, PFN_UP(EPX_SIZE));

	fp = fget(epx_fd);
	if (!fp) {
		pr_err("[EPX] : error to load file\n");
		ret = -EINVAL;
		goto finalize;
	}

	binary_buffer = kzalloc(EPX_SIZE, GFP_KERNEL);
	if (binary_buffer != NULL) {
		vfs_read(fp, binary_buffer, EPX_SIZE, &pos);

		memcpy((void *)epx_early_vm.addr, binary_buffer, EPX_SIZE);

		flush_icache_range((unsigned long)(epx_early_vm.addr), (unsigned long)(epx_early_vm.addr + EPX_SIZE));
		epx_set_memory_x((unsigned long)epx_early_vm.addr, PFN_UP(EPX_SIZE));

		epx_ptr = (void *)epx_early_vm.addr;
		if (*((unsigned int *)epx_ptr) != epx_signature) {
			pr_err("[EPX] : signature not matched!\n");
			ret = -EINVAL;
			goto free_buffer;
		}

		epx_ptr += 8;
		epx_offset = *((unsigned long long *)epx_ptr);

		binary_entry = (pfn_binary_entry)(epx_early_vm.addr + epx_offset);
		binary_entry((unsigned long)epx_early_vm.addr, kallsyms_lookup_name);

		epx_loaded = true;
free_buffer:
		kfree(binary_buffer);
	}
	fput(fp);

finalize:
	set_fs(old_fs);

	return 0;
}

static struct class_attribute class_attr_epx_activate = __ATTR(epx_activate, S_IRUSR, show_epx_activate, NULL);

static int __init epx_load(void)
{
	static struct class *epx_class;

	epx_class = class_create(THIS_MODULE, "epx");
	if (IS_ERR(epx_class)) {
		pr_err("EPX : couldn't create class (%s : %d)\n", __FILE__, __LINE__);
		return PTR_ERR(epx_class);
	}

	if (class_create_file(epx_class, &class_attr_epx_activate)) {
		pr_err("EPX : couldn't create class (%s : %d)\n", __FILE__, __LINE__);
		return -EINVAL;
	}

	return 0;
}
late_initcall(epx_load);
