/*
 * Exynos CPU Performance
 *   memcpy/memset: cacheable/non-cacheable
 *   Author: Jungwook Kim, jwook1.kim@samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <asm/cacheflush.h>
#include <linux/swiotlb.h>
#include <linux/types.h>

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/init.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <asm/io.h>
#include <asm/neon.h>

#define _PMU_COUNT_64
#include "exynos_perf_pmufunc.h"

#include <soc/samsung/cal-if.h>
#include <soc/samsung/exynos-devfreq.h>

#define EVENT_MAX	7

struct device *memcpy_dev;
static uint cal_id_mif = 0;
static uint devfreq_mif = 0;

enum memtype {
	MT_CACHE = 1,
	MT_NC,
	MT_C2NC,
	MT_NC2C,
	MT_ION,
	MT_ION_INF,
	MT_MAX,
};

enum functype {
	FT_MEMCPY = 1,
	FT_MEMSET,
	FT_CPYTOUSR,
	FT_CPYFRMUSR,
	FT_CACHE_FLUSH,
	FT_MAX,
};

static char *memtype_str[] = {"", "MT_CACHE", "MT_NC", "MT_C2NC", "MT_NC2C", "MT_ION", "MT_ION_INF"};
static char *functype_str[] = {"",
	"memcpy",
	"memset",
	"__copy_to_user",
	"__copy_from_user",
	"cache_flush",
};
static char *level_str[] = { "ALL_CL", "LOUIS", "LLC" };
static uint type = MT_CACHE;
static uint start_size = 64;
static uint end_size = SZ_4M;
static uint core = 4;
static uint run = 0;
static uint func = 1;
static int val = 0;
static uint align = 0;
static uint flush = 0;
static uint dump = 0;
static uint pmu = 0;
static uint chk = 0;
static uint src_mset = 0;
static char nc_memcpy_result_buf[PAGE_SIZE];
static const char *prefix = "exynos_perf_ncmemcpy";
static int nc_memcpy_done = 0;

// cache flush settings
static uint level = 2;
static uint setway = 1;

static int events[7] = {0x50,0x51,0x52,0x53,0x60,0x61,0};

#define GiB	(1024*1024*1024LL)
#define GB	1000000000LL
#define MiB	(1024*1024)
#define NS_PER_SEC	1000000000LL
#define US_PER_SEC	1000000LL

static u64 patterns[] = {
	/* The first entry has to be 0 to leave memtest with zeroed memory */
	0,
	0xffffffffffffffffULL,
	0x5555555555555555ULL,
	0xaaaaaaaaaaaaaaaaULL,
	0x1111111111111111ULL,
	0x2222222222222222ULL,
	0x4444444444444444ULL,
	0x8888888888888888ULL,
	0x3333333333333333ULL,
	0x6666666666666666ULL,
	0x9999999999999999ULL,
	0xccccccccccccccccULL,
	0x7777777777777777ULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xddddddddddddddddULL,
	0xeeeeeeeeeeeeeeeeULL,
	0x7a6c7258554e494cULL, /* yeah ;-) */
};

static u64 nano_time(void)
{
	struct timespec t;
	t.tv_sec = t.tv_nsec = 0;
	getnstimeofday(&t);
	return (t.tv_sec * NS_PER_SEC + t.tv_nsec);
}

static void memset_ptrn(ulong *p, uint count)
{
	uint j, idx;
	for (j = 0; j < count; j += sizeof(p)) {
		idx = j % ARRAY_SIZE(patterns);
		*p++ = patterns[idx];
	}
}

#ifdef __aarch64__
#define MEMSET_FUNC(FUNC)					\
	total_size = (core < 4)? 128*MiB : GiB;			\
	iter = total_size / xfer_size;				\
	for (i = 0; i < iter; i++) {				\
		start_ns = nano_time();				\
		FUNC;						\
		end_ns = nano_time();				\
		sum_ns += (end_ns - start_ns);			\
	}							\
	bw = total_size / MiB * NS_PER_SEC / sum_ns;		\
	dst_align = (ulong)dst + align;				\
	dst_align &= 0xfffff;					\
	ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5d %8s %7d %7d "		\
			"%10u %10llu %10u %15llu %15lx\n",	\
		functype_str[func], core, memtype_str[type], cpufreq, mif,		 \
		xfer_size, bw, iter, sum_ns, dst_align);

#if defined(CONFIG_SOC_EXYNOS9810) || defined(CONFIG_SOC_EXYNOS9610)
#define MEMCPY_FUNC(FUNC, do_flush, is_ret, do_pmu, do_chk, is_neon)	\
	total_size = (do_flush || core < 4 || type == MT_NC)? 128*MiB : GiB;	\
	iter = total_size / xfer_size;				\
	for (k = 0; k < iter; k++) {				\
		if (src_mset)					\
			memset_ptrn((ulong *)src + align, xfer_size);	\
		if (do_flush)					\
			__dma_flush_area(src + align,		\
				xfer_size);	\
		if (do_pmu) {					\
			__start_timer();			\
			for (i = 0; i < EVENT_MAX; i++) {	\
				pmu_enable_counter(i);		\
				pmnc_config(i, events[i]);	\
			}					\
			pmnc_reset();				\
		}						\
		if (is_neon)					\
			kernel_neon_begin();			\
		start_ns = nano_time();				\
		FUNC;						\
		end_ns = nano_time();				\
		if (is_neon)					\
			kernel_neon_end();			\
		sum_ns += (end_ns - start_ns);			\
		if (do_pmu) {					\
			ccnt += __stop_timer();			\
			for(i = 0; i < EVENT_MAX; i++)		\
				v[i] += pmnc_read2(i);		\
		}						\
		if (do_chk)					\
			chk_sum += (memcmp(dst, src + align, xfer_size) != 0)? 1 : 0;\
	}							\
	if (is_ret && ret > 0) {				\
		pr_info("bench failed: ret > 0\n");		\
		continue;					\
	}							\
	bw = total_size / MiB * NS_PER_SEC / sum_ns;		\
	src_align = (ulong)src + align;				\
	src_align &= 0xfffff;					\
	if (do_pmu) {						\
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5d %8s %7d %7d "	\
			"%10u %10llu %10u %15llu %15lu "	\
			"%15lu %15lu %15lu %15lu %15lu %15lu %15lu "	\
			"%15lx %7d %5lu\n",	\
			functype_str[func], core, memtype_str[type], cpufreq, mif,	\
			xfer_size, bw, iter, sum_ns, ccnt,	\
			v[0], v[1], v[2], v[3], v[4], v[5], v[6],	\
			src_align, do_flush, chk_sum);	\
	} else {						\
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5d %8s %7d %7d "	\
			"%10u %10llu %15u %15llu %15lx %7d %5lu\n",	\
			functype_str[func], core, memtype_str[type], cpufreq, mif,	\
			xfer_size, bw, iter, sum_ns, src_align, do_flush, chk_sum);\
	}
#else
#define MEMCPY_FUNC(FUNC, do_flush, is_ret, do_pmu, do_chk, is_neon)	\
	total_size = (do_flush || core < 4 || type == MT_NC)? 128*MiB : GiB;	\
	iter = total_size / xfer_size;				\
	for (k = 0; k < iter; k++) {				\
		if (src_mset)					\
			memset_ptrn((ulong *)src + align, xfer_size);	\
		if (do_flush)					\
			__dma_flush_area(src + align, xfer_size);\
		if (do_pmu) {					\
			__start_timer();			\
			for (i = 0; i < EVENT_MAX; i++) {	\
				pmu_enable_counter(i);		\
				pmnc_config(i, events[i]);	\
			}					\
			pmnc_reset();				\
		}						\
		if (is_neon)					\
			kernel_neon_begin();			\
		start_ns = nano_time();				\
		FUNC;						\
		end_ns = nano_time();				\
		if (is_neon)					\
			kernel_neon_end();			\
		sum_ns += (end_ns - start_ns);			\
		if (do_pmu) {					\
			ccnt += __stop_timer();			\
			for(i = 0; i < EVENT_MAX; i++)		\
				v[i] += pmnc_read2(i);		\
		}						\
		if (do_chk)					\
			chk_sum += (memcmp(dst, src + align, xfer_size) != 0)? 1 : 0;\
	}							\
	if (is_ret && ret > 0) {				\
		pr_info("[%s] bench failed: ret > 0\n", prefix);		\
		continue;					\
	}							\
	bw = total_size / MiB * NS_PER_SEC / sum_ns;		\
	src_align = (ulong)src + align;				\
	src_align &= 0xfffff;					\
	if (do_pmu) {						\
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5d %8s %7d %7d "	\
			"%10u %10llu %10u %15llu %15lu "	\
			"%15lu %15lu %15lu %15lu %15lu %15lu %15lu "	\
			"%15lx %7d %5lu\n",	\
			functype_str[func], core, memtype_str[type], cpufreq, mif,	\
			xfer_size, bw, iter, sum_ns, ccnt,	\
			v[0], v[1], v[2], v[3], v[4], v[5], v[6],	\
			src_align, do_flush, chk_sum);	\
	} else {						\
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5d %8s %7d %7d "	\
			"%10u %10llu %15u %15llu %15lx %7d %5lu\n",	\
			functype_str[func], core, memtype_str[type], cpufreq, mif,	\
			xfer_size, bw, iter, sum_ns, src_align, do_flush, chk_sum);\
	}
#endif

#define CACHE_FLUSH_TEST(FUNC, do_flush, do_chk)		\
	iter = 100;				\
	for (i = 0; i < iter; i++) {				\
		memset_ptrn((ulong *)src + align, xfer_size);	\
		memset(dst, 0x0, xfer_size);			\
		memcpy(dst, src + align, xfer_size);		\
		if (do_flush) {					\
			start_ns = nano_time();			\
			FUNC;					\
			end_ns = nano_time();			\
			sum_ns += (end_ns - start_ns);		\
		}						\
	/*	if (do_chk)					\
			chk_sum += (memcmp(dst_remap, dst, xfer_size) != 0)? 1 : 0; */\
	}							\
	avg_us = sum_ns / 1000 / iter;				\
	ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5d %8s %7d %7d "	\
		"%10u %10llu %7u %10llu %5d %5lu %8d %10s\n",			\
		functype_str[func], core, memtype_str[type], cpufreq, mif, \
		xfer_size, avg_us, iter, sum_ns, do_flush, chk_sum, setway, level_str[level]);

#endif

void func_perf(void *src, void *dst)
{
	u64 start_ns, end_ns, bw, sum_ns, avg_us;
	ulong chk_sum;
	uint total_size;
	uint iter;
	int i, k;
	uint xfer_size = start_size;
	size_t ret;
	ulong src_align;
	ulong dst_align;
	ccnt_t v[7] = {0,};
	ccnt_t ccnt;

	static char buf[PAGE_SIZE];
	uint cpufreq;
	uint mif = 0;
	bool is_need_warm_up = true;
	size_t ret1 = 0;

	nc_memcpy_done = 0;

	// HEADER
	if (pmu) {
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5s %8s %7s %7s %10s %10s %15s %10s %15s "
			"%8s(0x%x) %8s(0x%x) %8s(0x%x) %8s(0x%x) %8s(0x%x) %8s(0x%x) %8s(0x%x) %15s %7s %5s\n",
			"name", "core", "type", "cpufreq", "mif", "size", "MiB/s", "iter", "total_ns", "total_ccnt",
			"e0", events[0],
			"e1", events[1],
			"e2", events[2],
			"e3", events[3],
			"e4", events[4],
			"e5", events[5],
			"e6", events[6],
			"src+align", "flush", "chk");
	} else if (func == FT_MEMSET) {
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5s %8s %7s %7s %10s %10s %15s %10s %15s\n",
			"name", "core", "type", "cpufreq", "mif", "size", "MiB/s", "iter", "total_ns", "dst+align");
	} else if (func == FT_CACHE_FLUSH) {
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5s %8s %7s %7s %10s %10s %7s %10s %5s %5s %8s %10s\n",
			"name", "core", "type", "cpufreq", "mif", "size", "avg_us", "iter", "total_ns", "flush", "chk", "setway", "level");
	} else {
		ret1 += snprintf(buf + ret1, PAGE_SIZE - ret1, "%-20s %5s %8s %7s %7s %10s %10s %15s %15s %15s %7s %5s\n",
			"name", "core", "type", "cpufreq", "mif", "size", "MiB/s", "iter", "total_ns", "src+align", "flush", "chk");
	}

	// BODY - START
	bw = 0;
	while (xfer_size <= end_size) {
		ret = 0;
		sum_ns = 0;
		ccnt = 0;
		chk_sum = 0;
		v[0] = v[1] = v[2] = v[3] = v[4] = v[5] = v[6] = 0;
		cpufreq = cpufreq_quick_get(core) / 1000;
#if defined(CONFIG_SOC_EXYNOS9830) || defined(CONFIG_SOC_EXYNOS991)
		mif = (uint)exynos_devfreq_get_domain_freq(devfreq_mif) / 1000;
#else
		mif = (uint)cal_dfs_cached_get_rate(cal_id_mif) / 1000;
#endif
		switch (func) {
			case FT_MEMCPY:
				MEMCPY_FUNC(memcpy(dst, src + align, xfer_size), flush, 0, pmu, chk, 0);
				break;
			case FT_MEMSET:
				MEMSET_FUNC(memset(dst, val, xfer_size));
				break;
			case FT_CPYTOUSR:
				MEMCPY_FUNC(ret += __copy_to_user(dst, src + align, xfer_size), flush, 1, pmu, chk, 0);
				break;
			case FT_CPYFRMUSR:
				MEMCPY_FUNC(ret += __copy_from_user(dst, src + align, xfer_size), flush, 1, pmu, chk, 0);
				break;
			case FT_CACHE_FLUSH:
				if (setway == 0) {
					CACHE_FLUSH_TEST(__dma_flush_area(src + align, xfer_size), flush, chk);
				} else {
					switch (level) {
						case 1: CACHE_FLUSH_TEST(flush_cache_louis(), flush, chk); break;
						case 2: CACHE_FLUSH_TEST(flush_cache_all(), flush, chk); break;
					}
				}
				break;
			default:
				pr_info("[%s] func type = %d invalid!\n", prefix, func);
				break;
		}

		if (is_need_warm_up) {	// run first iteration 2 times for warm up
			is_need_warm_up = false;
		} else {
			xfer_size *= 2;
		}
	}
	// BODY - END

	memcpy(nc_memcpy_result_buf, buf, PAGE_SIZE);

	nc_memcpy_done = 1;
}

static int perf_main(void)
{
	struct device *dev = memcpy_dev;
	void *src_cpu, *dst_cpu;
	dma_addr_t src_dma, dst_dma;
	gfp_t gfp;
	uint buf_size;
	u64 start_ns, end_ns;
	start_ns = nano_time();

	/* dma address */
	if (!dma_set_mask(dev, DMA_BIT_MASK(64))) {
		pr_info("[%s] dma_mask: 64-bits ok\n", prefix);
	} else if (!dma_set_mask(dev, DMA_BIT_MASK(32))) {
		pr_info("[%s] dma_mask: 32-bits ok\n", prefix);
	}

	buf_size = (align == 0)? end_size : end_size + 64;
	gfp = GFP_KERNEL;

	if (type == MT_CACHE) {
		gfp = GFP_KERNEL;
		src_cpu = kmalloc(buf_size, gfp);
		if (src_cpu == NULL) {
			pr_info("[%s] kmalloc failed: src\n", prefix);
			return 0;
		}
		dst_cpu = kmalloc(buf_size, gfp);
		if (dst_cpu == NULL) {
			pr_info("[%s] kmalloc failed: dst\n", prefix);
			kfree(src_cpu);
			return 0;
		}
		func_perf(src_cpu, dst_cpu);
		kfree(dst_cpu);
		kfree(src_cpu);

	} else if (type == MT_NC) {
		src_cpu = dma_alloc_coherent(dev, buf_size, &src_dma, gfp);
		if (src_cpu == NULL) {
			pr_info("[%s] dma_alloc_coherent failed: src: buf_size=%d\n", prefix, buf_size);
			return 0;
		}
		dst_cpu = dma_alloc_coherent(dev, buf_size, &dst_dma, gfp);
		if (dst_cpu == NULL) {
			pr_info("[%s] dma_alloc_coherent failed: dst: buf_size=%d\n", prefix, buf_size);
			dma_free_coherent(dev, buf_size, src_cpu, src_dma);
			return 0;
		}
		func_perf(src_cpu, dst_cpu);
		dma_free_coherent(dev, buf_size, dst_cpu, dst_dma);
		dma_free_coherent(dev, buf_size, src_cpu, src_dma);
	} else if (type == MT_C2NC) {
		/* cacheable --> non-cacheable */
		pr_info("[%s] cacheable --> non-cacheable\n", prefix);
		src_cpu = kmalloc(buf_size, gfp);
		if (src_cpu == NULL) {
			pr_info("[%s] kmalloc failed: src\n", prefix);
			return 0;
		}
		dst_cpu = dma_alloc_coherent(dev, buf_size, &dst_dma, gfp);
		if (dst_cpu == NULL) {
			pr_info("[%s] dma_alloc_coherent failed: dst\n", prefix);
			kfree(src_cpu);
			return 0;
		}
		func_perf(src_cpu, dst_cpu);
		dma_free_coherent(dev, buf_size, dst_cpu, dst_dma);
		kfree(src_cpu);
	} else if (type == MT_NC2C) {
		/* non-cacheable --> cacheable */
		pr_info("[%s] non-cacheable --> cacheable\n", prefix);
		src_cpu = dma_alloc_coherent(dev, buf_size, &src_dma, gfp);
		if (src_cpu == NULL) {
			pr_info("[%s] dma_alloc_coherent failed: src\n", prefix);
			return 0;
		}
		dst_cpu = kmalloc(buf_size, gfp);
		if (dst_cpu == NULL) {
			pr_info("[%s] kmalloc failed: dst\n", prefix);
			dma_free_coherent(dev, buf_size, src_cpu, src_dma);
			return 0;
		}
		func_perf(src_cpu, dst_cpu);
		dma_free_coherent(dev, buf_size, src_cpu, src_dma);
		kfree(dst_cpu);
#if 0
	} else if (type == MT_ION) {
//		alloc_ctx = vb2_ion_create_context(dev, SZ_4K, VB2ION_CTX_IOMMU | VB2ION_CTX_VMCONTIG);
		alloc_ctx = vb2_ion_create_context(dev, SZ_4K, VB2ION_CTX_VMCONTIG);
		if (IS_ERR(alloc_ctx)) {
			pr_info("-----> ion context failed = %ld\n", PTR_ERR(alloc_ctx));
			return 0;
		}
		cookie = vb2_ion_private_alloc(alloc_ctx, buf_size * 2);
		if (IS_ERR(cookie)) {
			pr_info("-----> ion alloc failed = %ld\n", PTR_ERR(cookie));
			return 0;
		}
		src_cpu = vb2_ion_private_vaddr(cookie);
		if (src_cpu == NULL) {
			pr_info("-----> ion vaddr failed\n");
			return 0;
		}
		dst_cpu = src_cpu + buf_size;
		vb2_ion_sync_for_device(cookie, 0, buf_size * 2, DMA_BIDIRECTIONAL);

		func_perf(src_cpu, dst_cpu);
		func_perf(dst_cpu, src_cpu);

		vb2_ion_private_free(cookie);
		vb2_ion_destroy_context(alloc_ctx);
	} else if (type == MT_ION_INF) {
		alloc_ctx = vb2_ion_create_context(dev, SZ_4K, VB2ION_CTX_VMCONTIG);
		if (IS_ERR(alloc_ctx)) {
			pr_info("-----> ion context failed = %ld\n", PTR_ERR(alloc_ctx));
			return 0;
		}
		cookie = vb2_ion_private_alloc(alloc_ctx, buf_size * 2);
		if (IS_ERR(cookie)) {
			pr_info("-----> ion alloc failed = %ld\n", PTR_ERR(cookie));
			return 0;
		}
		src_cpu = vb2_ion_private_vaddr(cookie);
		if (src_cpu == NULL) {
			pr_info("-----> ion vaddr failed\n");
			return 0;
		}
		dst_cpu = src_cpu + buf_size;
		vb2_ion_sync_for_device(cookie, 0, buf_size * 2, DMA_BIDIRECTIONAL);

		while (1) {
			func_perf(src_cpu, dst_cpu);
			func_perf(dst_cpu, src_cpu);
		}

		vb2_ion_private_free(cookie);
		vb2_ion_destroy_context(alloc_ctx);
#endif
	} else {
		pr_info("[%s] type = %d invalid!\n", prefix, type);
	}

	end_ns = nano_time();
	pr_info("[%s] Total elapsed time = %llu seconds\n", prefix, (end_ns - start_ns) / NS_PER_SEC);

	return 0;
}

/* thread main function */
static bool thread_running;
static int thread_func(void *data)
{
	thread_running = 1;
	perf_main();
	thread_running = 0;
	return 0;
}

static struct task_struct *task;
static int test_thread_init(void)
{
	task = kthread_create(thread_func, NULL, "thread%u", 0);
	kthread_bind(task, core);
	wake_up_process(task);
	return 0;
}

static void test_thread_exit(void)
{
	printk(KERN_ERR "[%s] Exit module: thread_running: %d\n", prefix, thread_running);
	if (thread_running)
		kthread_stop(task);
}


/*-------------------------------------------------------------------------------
 * sysfs starts here
 *-------------------------------------------------------------------------------*/
// macro for normal nodes
#define DEF_NODE(name) \
	static int name##_seq_show(struct seq_file *file, void *iter) {	\
		seq_printf(file, "%d\n", name);	\
		return 0;	\
	}	\
	static ssize_t name##_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off) {	\
		char buf[20];	\
		count = (count > 20)? 20 : count;	\
		if (copy_from_user(buf, buffer, count) != 0)	\
			return -EFAULT;	\
		sscanf(buf, "%d", &name);	\
		return count;	\
	}	\
	static int name##_debugfs_open(struct inode *inode, struct file *file) { \
		return single_open(file, name##_seq_show, inode->i_private);	\
	}	\
	static const struct file_operations name##_debugfs_fops = {	\
		.owner		= THIS_MODULE,	\
		.open		= name##_debugfs_open,	\
		.read		= seq_read,	\
		.write		= name##_seq_write,	\
		.llseek		= seq_lseek,	\
		.release	= single_release,	\
	};

//------------------------- normal nodes
DEF_NODE(level)
DEF_NODE(setway)
DEF_NODE(src_mset)
DEF_NODE(chk)
DEF_NODE(pmu)
DEF_NODE(dump)
DEF_NODE(flush)
DEF_NODE(align)
DEF_NODE(val)
DEF_NODE(start_size)
DEF_NODE(end_size)
DEF_NODE(func)
DEF_NODE(type)
DEF_NODE(core)

//------------------------- special nodes
// event
static int event_seq_show(struct seq_file *file, void *unused)
{
	u32 val = read_pmnc();
	seq_printf(file, "0x%x, 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
			val,
			events[0],
			events[1],
			events[2],
			events[3],
			events[4],
			events[5],
			events[6]);
	return 0;
}
static ssize_t event_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	char buf[128];
	count = (count > 128)? 128 : count;
	if (copy_from_user(buf, buffer, count) != 0)
		return -EFAULT;
	sscanf(buf, "%x %x %x %x %x %x %x",
			&events[0],
			&events[1],
			&events[2],
			&events[3],
			&events[4],
			&events[5],
			&events[6]);
	return count;
}
static int event_debugfs_open(struct inode *inode, struct file *file) {
	return single_open(file, event_seq_show, inode->i_private);
}
static const struct file_operations event_debugfs_fops = {
		.owner		= THIS_MODULE,
		.open		= event_debugfs_open,
		.read		= seq_read,
		.write		= event_seq_write,
		.llseek		= seq_lseek,
		.release	= single_release,
};

// run
static int run_seq_show(struct seq_file *file, void *unused)
{
	if (nc_memcpy_done == 0) {
		seq_printf(file, "NO RESULT\n");
	} else {
		seq_printf(file, "%s", nc_memcpy_result_buf);				// PRINT RESULT
	}

	return 0;
}
static ssize_t run_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off)
{
	char buf[10];
	count = (count > 10)? 10 : count;
	if (copy_from_user(buf, buffer, count) != 0)
		return -EFAULT;
	sscanf(buf, "%d", &run);
	if (run == 1) {
		test_thread_init();
	} else {
		test_thread_exit();
	}
	return count;
}
static int run_debugfs_open(struct inode *inode, struct file *file) {
	return single_open(file, run_seq_show, inode->i_private);
}
static const struct file_operations run_debugfs_fops = {
		.owner		= THIS_MODULE,
		.open		= run_debugfs_open,
		.read		= seq_read,
		.write		= run_seq_write,
		.llseek		= seq_lseek,
		.release	= single_release,
};

// info
#define PRINT_NODE(name) \
	seq_printf(file, #name"=%d\n", name);
/*
DEF_NODE(level)
DEF_NODE(setway)
DEF_NODE(src_mset)
DEF_NODE(chk)
DEF_NODE(pmu)
DEF_NODE(dump)
DEF_NODE(flush)
DEF_NODE(align)
DEF_NODE(val)
DEF_NODE(start_size)
DEF_NODE(end_size)
DEF_NODE(func)
DEF_NODE(type)
DEF_NODE(core)
*/
static int info_seq_show(struct seq_file *file, void *unused)
{
	int i;
	seq_printf(file, "type_list:\n");
	for (i = 1; i < MT_MAX; i++)
		seq_printf(file, "%d. %s\n", i, memtype_str[i]);

	seq_printf(file, "func_list:\n");
	for (i = 1; i < FT_MAX; i++)
		seq_printf(file, "%d. %s\n", i, functype_str[i]);
	PRINT_NODE(core)
	PRINT_NODE(func)
	PRINT_NODE(type)
	PRINT_NODE(start_size)
	PRINT_NODE(end_size)
	PRINT_NODE(src_mset)
	PRINT_NODE(align)
	PRINT_NODE(pmu)
	PRINT_NODE(flush)
	PRINT_NODE(chk)
	return 0;
}
static ssize_t info_seq_write(struct file *file, const char __user *buffer, size_t count, loff_t *off) {
	char buf[20];
	count = (count > 20)? 20 : count;
	if (copy_from_user(buf, buffer, count) != 0)
		return -EFAULT;
	return count;
}
static int info_debugfs_open(struct inode *inode, struct file *file) {
	return single_open(file, info_seq_show, inode->i_private);
}
static const struct file_operations info_debugfs_fops = {
		.owner		= THIS_MODULE,
		.open		= info_debugfs_open,
		.read		= seq_read,
		.write		= info_seq_write,
		.llseek		= seq_lseek,
		.release	= single_release,
};

// end of sysfs nodes
/*------------------------------------------------------------------*/

// get device
static int memcpy_probe(struct platform_device *pdev)
{
	memcpy_dev = &pdev->dev;
	pr_info("memcpy_dev = %p\n", memcpy_dev);
	return 0;
}

static int memcpy_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id memcpy_dt_match[] = {
	{ .compatible = "samsung,exynos_perf_ncmemcpy", },
	{ },
};

static struct platform_driver memcpy_driver = {
	.probe = memcpy_probe,
	.remove = memcpy_remove,
	.driver = {
		.name = "exynos_perf_ncmemcpy",
		.owner = THIS_MODULE,
		.of_match_table = memcpy_dt_match,
	}
};


//-------------------
// main
//-------------------
static int __init perf_init(void)
{
	struct device_node *dn = NULL;
	struct dentry *root, *d;
	int ret;

	dn = of_find_node_by_name(dn, "exynos_perf_ncmemcpy");
	of_property_read_u32(dn, "cal-id-mif", &cal_id_mif);
	of_property_read_u32(dn, "devfreq-mif", &devfreq_mif);

	root = debugfs_create_dir("exynos_perf_ncmemcpy", NULL);
	if (!root) {
		pr_err("%s: create debugfs\n", __FILE__);
		return -ENOMEM;
	}
	d = debugfs_create_file("level", S_IRUSR, root,
					(unsigned int *)0,
					&level_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("setway", S_IRUSR, root,
					(unsigned int *)0,
					&setway_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("src_mset", S_IRUSR, root,
					(unsigned int *)0,
					&src_mset_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("chk", S_IRUSR, root,
					(unsigned int *)0,
					&chk_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("pmu", S_IRUSR, root,
					(unsigned int *)0,
					&pmu_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("dump", S_IRUSR, root,
					(unsigned int *)0,
					&dump_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("flush", S_IRUSR, root,
					(unsigned int *)0,
					&flush_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("align", S_IRUSR, root,
					(unsigned int *)0,
					&align_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("val", S_IRUSR, root,
					(unsigned int *)0,
					&val_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("start_size", S_IRUSR, root,
					(unsigned int *)0,
					&start_size_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("end_size", S_IRUSR, root,
					(unsigned int *)0,
					&end_size_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("func", S_IRUSR, root,
					(unsigned int *)0,
					&func_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("type", S_IRUSR, root,
					(unsigned int *)0,
					&type_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("core", S_IRUSR, root,
					(unsigned int *)0,
					&core_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("event", S_IRUSR, root,
					(unsigned int *)0,
					&event_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("run", S_IRUSR, root,
					(unsigned int *)0,
					&run_debugfs_fops);
	if (!d)
		return -ENOMEM;
	d = debugfs_create_file("info", S_IRUSR, root,
					(unsigned int *)0,
					&info_debugfs_fops);
	if (!d)
		return -ENOMEM;

	ret = platform_driver_register(&memcpy_driver);
	if (!ret)
		pr_info("%s: init\n", memcpy_driver.driver.name);

	return 0;
}


module_init(perf_init);
//module_exit(perf_exit);

MODULE_AUTHOR("Jungwook Kim");
MODULE_DESCRIPTION("kernel memcpy test module");
MODULE_LICENSE("GPL");
