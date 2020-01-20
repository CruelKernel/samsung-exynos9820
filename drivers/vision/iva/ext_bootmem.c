/*
 * ext_bootmem.c
 */
#include <linux/bootmem.h>
#include <linux/ext_bootmem.h>
#include <linux/sizes.h>
#include <linux/mmzone.h>
#include <linux/memblock.h>
#include <linux/kernel.h>

#include <asm/memory.h>

#define _DEBUG

void __init reserve_ext_bootmem(void)
{
	if (memblock_reserve(PRE_RESERVED_PA_START, PRE_RESERVED_PA_SIZE)) {
		pr_err("memblock reserve is failed!(pa: 0x%08X, size: 0x%08X)\n",
				PRE_RESERVED_PA_START, PRE_RESERVED_PA_SIZE);
	} else {
#ifdef _DEBUG
		pr_info("Pre-reserved Memory : SUCCESS\n");
		pr_info("- Physical memory = 0x%08X ~ 0x%08X\n",
			PRE_RESERVED_PA_START,
			PRE_RESERVED_PA_START + PRE_RESERVED_PA_SIZE);
		pr_info("- Pre-reserved size = %d KB\n", PRE_RESERVED_PA_SIZE >> 10);
#endif
	}
}
