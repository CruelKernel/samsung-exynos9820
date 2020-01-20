/* drivers/gpu/arm/.../platform/gpu_balance.c
 *
 * Copyright 2011 by S.LSI. Samsung Electronics Inc.
 * San#24, Nongseo-Dong, Giheung-Gu, Yongin, Korea
 *
 * Samsung SoC Mali-T Series gpu h/w init code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software FoundatIon.
 */

/**
 * @file gpu_balance.c
 */

#include <mali_kbase.h>
#include <linux/delay.h>
#include <asm/dma-mapping.h>
#include <mach/map.h>

#include "mali_kbase_platform.h"

#include "gpu_balance.h"
#include "streamout.h"
#include "streamout_SRAM.h"

extern struct kbase_device *pkbdev;

#ifdef CONFIG_OF_RESERVED_MEM
#include <linux/of_reserved_mem.h>

struct _mali_balance {
	const char *name;
	unsigned long base;
	unsigned long size;
	unsigned char *stream_reg;
	dma_addr_t  phys_addr;
} mali_balance;

static int __init iram_vector_reserved_mem_setup(struct reserved_mem *rmem)
{
	KBASE_DEBUG_ASSERT(NULL != rmem);

	mali_balance.name = rmem->name;
	mali_balance.base = rmem->base;
	mali_balance.size = rmem->size;

	pr_info("%s: Reserved memory: %s:%#lx (Size: %#lx)\n",
			__func__, mali_balance.name, mali_balance.base, mali_balance.size);

	return 0;
}

RESERVEDMEM_OF_DECLARE(iram_vector, "mali,iram-vector", iram_vector_reserved_mem_setup);
#endif

static checksum_block checksum[] = {
	/* {  start address,          End address,       Golden checksum } */
	{0x5509A000+0x9da00000, 0x5509A0C0+0x9da00000, 0x0000000fb98c1f80}, /* varying_1_1_0_highpa.hex     */
	{0x5509A0C0+0x9da00000, 0x5509A100+0x9da00000, 0x0000000307f80000}, /* varying_1_1_1_highpa.hex     */
	{0x5509A100+0x9da00000, 0x5509A1C0+0x9da00000, 0x00000005f803a800}, /* varying_1_1_2_highpa.hex     */
	{0x5509A1C0+0x9da00000, 0x5509A200+0x9da00000, 0x000000040e800000}, /* varying_1_1_3_highpa.hex     */
	{0x5509A200+0x9da00000, 0x5509A2C0+0x9da00000, 0x0000000e2f5d87f0}, /* varying_1_1_4_highpa.hex     */
	{0x5509A2C0+0x9da00000, 0x5509A300+0x9da00000, 0x000000060f660000}, /* varying_1_1_5_highpa.hex     */
	{0x5509A300+0x9da00000, 0x5509A3C0+0x9da00000, 0x0000000a806ce0a8}, /* varying_1_1_6_highpa.hex     */
	{0x5509A3C0+0x9da00000, 0x5509A400+0x9da00000, 0x0000000411b70000}, /* varying_1_1_7_highpa.hex     */
	{0x5509A400+0x9da00000, 0x5509A4C0+0x9da00000, 0x0000000c82b7ef20}, /* varying_1_1_8_highpa.hex     */
	{0x5509A4C0+0x9da00000, 0x5509A500+0x9da00000, 0x000000050f080000}, /* varying_1_1_9_highpa.hex     */
	{0x5509A500+0x9da00000, 0x5509A5C0+0x9da00000, 0x0000000e7376e360}, /* varying_1_1_10_highpa.hex    */
	{0x5509A5C0+0x9da00000, 0x5509A600+0x9da00000, 0x000000070b3c0000}, /* varying_1_1_11_highpa.hex    */
	{0x5509A600+0x9da00000, 0x5509A6C0+0x9da00000, 0x0000000c756de980}, /* varying_1_1_12_highpa.hex    */
	{0x5509A6C0+0x9da00000, 0x5509A700+0x9da00000, 0x000000060ef80000}, /* varying_1_1_13_highpa.hex    */
	{0x5509A700+0x9da00000, 0x5509A7C0+0x9da00000, 0x0000000e34768300}, /* varying_1_1_14_highpa.hex    */
	{0x5509A7C0+0x9da00000, 0x5509A800+0x9da00000, 0x000000050a040000}, /* varying_1_1_15_highpa.hex    */
	{0x5509A800+0x9da00000, 0x5509A8C0+0x9da00000, 0x0000000e514abb00}, /* varying_1_1_16_highpa.hex    */
	{0x5509A8C0+0x9da00000, 0x5509A900+0x9da00000, 0x0000000509f00000}, /* varying_1_1_17_highpa.hex    */
	{0x5509A900+0x9da00000, 0x5509AA20+0x9da00000, 0x000000130f0d9800}, /* varying_1_1_18_highpa.hex    */
	{0x5509AA40+0x9da00000, 0x5509AAA0+0x9da00000, 0x000000050d140000}, /* varying_1_1_19_highpa.hex    */
	{0x5509F000+0x9da00000, 0x5509F600+0x9da00000, 0x00000003b18774cd}, /* tiler_cmdlist_1_1_highpa.hex */
	{0x5509E000+0x9da00000, 0x5509EE00+0x9da00000, 0x0000000713fd368c}, /* tiler_cmdlist_1_2_highpa.hex */
	{0x55075000+0x9da00000, 0x55085400+0x9da00000, 0x000003634d0d1c02}, /* output_colour_buff_1_3.hex   */
	{0x5503A000+0x9da00000, 0x5503A200+0x9da00000, 0x00000046bc13f4bb}, /* output_crc_buff_2_4.hex      */
	{0x5503B000+0x9da00000, 0x5504B000+0x9da00000, 0x00003fa41612ad18}, /* output_colour_buff_2_4.hex   */
	{0x550C0000+0x9da00000, 0x550C3000+0x9da00000, 0x000000f810fbb6d5}, /* output_crc_buff_1_2.hex		*/
};

static int test_checksum(unsigned int *start, unsigned int *end, unsigned long golden_sum)
{
	unsigned int result;
	unsigned long checksum = 0;
	int i = 0;
	struct kbase_device *kbdev = pkbdev;

	do {
		result = start[i];
		checksum += result;
	} while (end != &start[++i]);

	if (checksum != golden_sum) {
		dev_dbg(kbdev->dev, "start 0x%p, end 0x%p %ld != %ld\n", &start[i], end, checksum, golden_sum);
		return 1;
	}

	return 0;
}

static int test_checksum_all(void)
{
	int is_ok = 0;
	unsigned long i;
	struct kbase_device *kbdev = pkbdev;

	for (i = 0 ; i < ARRAY_SIZE(checksum); i++)
		is_ok |= (test_checksum((unsigned int *)phys_to_virt(checksum[i].start), (unsigned int *)phys_to_virt(checksum[i].end), (unsigned long)checksum[i].golden_sum) << i);

	if (is_ok != 0)
		dev_dbg(kbdev->dev, "checksum fail : 0x%x\n", is_ok);

	return is_ok;
}

void preload_balance_init(struct kbase_device *kbdev)
{
	int j = 0;

	struct page *p = phys_to_page(mali_balance.base);
	int npages = PAGE_ALIGN(mali_balance.size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;

	for (j = 0; j < npages; j++) {
		*(tmp++) = p++;
	}

	mali_balance.stream_reg = (unsigned char *)vmap(pages, npages, VM_MAP, pgprot_writecombine(PAGE_KERNEL));
	__flush_dcache_area(mali_balance.stream_reg, mali_balance.size);

	memset(mali_balance.stream_reg, 0x0, mali_balance.size);
	memcpy(mali_balance.stream_reg, streamout, streamout_size);
	memcpy(mali_balance.stream_reg + 0xB0000, streamout_SRAM, streamout_SRAM_size);

	vfree(pages);
}

void preload_balance_setup(struct kbase_device *kbdev)
{
	memcpy(mali_balance.stream_reg, streamout, streamout_size);
	memcpy(mali_balance.stream_reg + 0xB0000, streamout_SRAM, streamout_SRAM_size);
}

bool balance_init(struct kbase_device *kbdev)
{
	bool ret = true;

	GPU_LOG(DVFS_INFO, DUMMY, 0u, 0u, "power initialized\n");

	/* Mali Soft reset */
	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00030781);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((GPU_COMMAND), 0x00000001);

	/* CHECK RESET_COMPLETE @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [8]:RESET_COMPLETED  */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x100, 100);

	/* Cache clean & invalidate */
	MALI_WRITE_REG((GPU_COMMAND), 0x00000008);

	/* CHECK CLEAN_CACHES_COMPLETED @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [17]:CLEAN_CACHES_COMPLETED */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x20000, 100);

	MALI_WRITE_REG((MMU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((SHADER_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((SHADER_PWRON_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((TILER_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((TILER_PWRON_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((L2_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((L2_PWRON_HI), 0xFFFFFFFF);

	/* power check  */
	MALI_GPU_CONTROL_WAIT(SHADER_READY_LO, 0xff, 50);
	MALI_GPU_CONTROL_WAIT(TILER_READY_LO, 0x1, 50);
	MALI_GPU_CONTROL_WAIT(L2_READY_LO, 0x1, 50);

	MALI_WRITE_REG((AS3_MEMATTR_HI), 0x88888888);
	MALI_WRITE_REG((AS3_MEMATTR_LO), 0x88888888);
	MALI_WRITE_REG((AS3_TRANSTAB_LO), 0xF2A26007);
	MALI_WRITE_REG((AS3_TRANSTAB_HI), 0x00000000);
	MALI_WRITE_REG((AS3_COMMAND), 0x00000001);

	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00000081);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0xFFFFFFFF);
	MALI_WRITE_REG((JOB_IRQ_MASK), 0x00000000);

	MALI_WRITE_REG((JS1_HEAD_NEXT_LO), 0xE402F740);
	MALI_WRITE_REG((JS1_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_CONFIG_NEXT), 0x00083703);
	MALI_WRITE_REG((JS1_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000002, 150);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000002);

	/* Mali Soft reset */
	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00030781);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((GPU_COMMAND), 0x00000001);

	/* CHECK RESET_COMPLETE @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [8]:RESET_COMPLETED  */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x100, 100);

	/* Cache clean & invalidate */
	MALI_WRITE_REG((GPU_COMMAND), 0x00000008);

	/* CHECK CLEAN_CACHES_COMPLETED @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [17]:CLEAN_CACHES_COMPLETED */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x20000, 100);

	MALI_WRITE_REG((MMU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((SHADER_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((SHADER_PWRON_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((TILER_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((TILER_PWRON_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((L2_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((L2_PWRON_HI), 0xFFFFFFFF);

	/* power check  */
	MALI_GPU_CONTROL_WAIT(SHADER_READY_LO, 0xff, 50);
	MALI_GPU_CONTROL_WAIT(TILER_READY_LO, 0x1, 50);
	MALI_GPU_CONTROL_WAIT(L2_READY_LO, 0x1, 50);

	MALI_WRITE_REG((AS0_MEMATTR_HI), 0x88888888);
	MALI_WRITE_REG((AS0_MEMATTR_LO), 0x88888888);
	MALI_WRITE_REG((AS0_TRANSTAB_LO), 0xF2A00007);
	MALI_WRITE_REG((AS0_TRANSTAB_HI), 0x00000000);
	MALI_WRITE_REG((AS0_COMMAND), 0x00000001);

	MALI_WRITE_REG((AS1_MEMATTR_HI), 0x88888888);
	MALI_WRITE_REG((AS1_MEMATTR_LO), 0x88888888);
	MALI_WRITE_REG((AS1_TRANSTAB_LO), 0xF2A0F007);
	MALI_WRITE_REG((AS1_TRANSTAB_HI), 0x00000000);
	MALI_WRITE_REG((AS1_COMMAND), 0x00000001);

	MALI_WRITE_REG((AS2_MEMATTR_HI), 0x88888888);
	MALI_WRITE_REG((AS2_MEMATTR_LO), 0x88888888);
	MALI_WRITE_REG((AS2_TRANSTAB_LO), 0xF2A1E007);
	MALI_WRITE_REG((AS2_TRANSTAB_HI), 0x00000000);
	MALI_WRITE_REG((AS2_COMMAND), 0x00000001);

	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00000081);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0xFFFFFFFF);
	MALI_WRITE_REG((JOB_IRQ_MASK), 0x00000000);

	MALI_WRITE_REG((JS1_HEAD_NEXT_LO), 0xE402F740);
	MALI_WRITE_REG((JS1_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_CONFIG_NEXT), 0x00083702);
	MALI_WRITE_REG((JS1_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000002, 150);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000002);

	MALI_WRITE_REG((JS2_HEAD_NEXT_LO), 0x01000000);
	MALI_WRITE_REG((JS2_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS2_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS2_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS2_CONFIG_NEXT), 0x00080700);
	MALI_WRITE_REG((JS2_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000004, 52);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000004);

	MALI_WRITE_REG((JS1_HEAD_NEXT_LO), 0xE4036D00);
	MALI_WRITE_REG((JS1_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_CONFIG_NEXT), 0x00083701);
	MALI_WRITE_REG((JS1_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000002, 450);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000002);

	MALI_WRITE_REG((JS0_HEAD_NEXT_LO), 0xD8C0D3C0);
	MALI_WRITE_REG((JS0_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS0_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS0_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS0_CONFIG_NEXT), 0x00083F00);
	MALI_WRITE_REG((JS0_COMMAND_NEXT), 0x00000001);

	MALI_WRITE_REG((JS1_HEAD_NEXT_LO), 0xD8C0D340);
	MALI_WRITE_REG((JS1_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_CONFIG_NEXT), 0x00083F01);
	MALI_WRITE_REG((JS1_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000002, 220);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000002);

	MALI_JOB_IRQ_WAIT(0x00000001, 15);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000001);

	MALI_WRITE_REG((JS0_HEAD_NEXT_LO), 0xE4036D80);
	MALI_WRITE_REG((JS0_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS0_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS0_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS0_CONFIG_NEXT), 0x00083700);
	MALI_WRITE_REG((JS0_COMMAND_NEXT), 0x00000001);

	MALI_WRITE_REG((JS2_HEAD_NEXT_LO), 0x01000080);
	MALI_WRITE_REG((JS2_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS2_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS2_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS2_CONFIG_NEXT), 0x00080000);
	MALI_WRITE_REG((JS2_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000005, 380);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000005);

	MALI_WRITE_REG((JS1_HEAD_NEXT_LO), 0x010000C0);
	MALI_WRITE_REG((JS1_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS1_CONFIG_NEXT), 0x00083000);
	MALI_WRITE_REG((JS1_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000002, 50);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000002);

	/* Mali reset */
	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00030781);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((GPU_COMMAND), 0x00000001);

	/* CHECK RESET_COMPLETE @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [8]:RESET_COMPLETED  */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x100, 100);

	/* for UHD */
	/* Cache clean & invalidate */
	MALI_WRITE_REG((GPU_COMMAND), 0x00000008);

	/* CHECK CLEAN_CACHES_COMPLETED @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [17]:CLEAN_CACHES_COMPLETED */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x20000, 100);

	MALI_WRITE_REG((MMU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((TILER_CONFIG), 0x00080000);
	MALI_WRITE_REG((SHADER_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((SHADER_PWRON_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((TILER_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((TILER_PWRON_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((L2_PWRON_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((L2_PWRON_HI), 0xFFFFFFFF);

	/* power check  */
	MALI_GPU_CONTROL_WAIT(SHADER_READY_LO, 0xff, 50);
	MALI_GPU_CONTROL_WAIT(TILER_READY_LO, 0x1, 50);
	MALI_GPU_CONTROL_WAIT(L2_READY_LO, 0x1, 50);

	MALI_WRITE_REG((AS0_MEMATTR_HI), 0x88888888);
	MALI_WRITE_REG((AS0_MEMATTR_LO), 0x88888888);
	MALI_WRITE_REG((AS0_TRANSTAB_LO), 0xF2AB0007);
	MALI_WRITE_REG((AS0_TRANSTAB_HI), 0x00000000);
	MALI_WRITE_REG((AS0_COMMAND), 0x00000001);
	MALI_WRITE_REG((AS1_MEMATTR_HI), 0x88888888);
	MALI_WRITE_REG((AS1_MEMATTR_LO), 0x88888888);
	MALI_WRITE_REG((AS1_TRANSTAB_LO), 0xF2AB0007);
	MALI_WRITE_REG((AS1_TRANSTAB_HI), 0x00000000);
	MALI_WRITE_REG((AS1_COMMAND), 0x00000001);
	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00000081);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0xFFFFFFFF);
	MALI_WRITE_REG((JOB_IRQ_MASK), 0x00000000);

	MALI_WRITE_REG((JS0_HEAD_NEXT_LO), 0xE74E6780);
	MALI_WRITE_REG((JS0_HEAD_NEXT_HI), 0x00000000);
	MALI_WRITE_REG((JS0_AFFINITY_NEXT_LO), 0xFFFFFFFF);
	MALI_WRITE_REG((JS0_AFFINITY_NEXT_HI), 0xFFFFFFFF);
	MALI_WRITE_REG((JS0_CONFIG_NEXT), 0x00083700);
	MALI_WRITE_REG((JS0_COMMAND_NEXT), 0x00000001);

	MALI_JOB_IRQ_WAIT(0x00000001, 320);

	MALI_WRITE_REG((JOB_IRQ_CLEAR), 0x00000001);

	if (test_checksum_all() != 0) {
		KBASE_TRACE_ADD_EXYNOS(kbdev, LSI_CHECKSUM, NULL, NULL, 0u, false);
		ret = false;
	}

	/* Mali reset */
	MALI_WRITE_REG((GPU_IRQ_CLEAR), 0x00030781);
	MALI_WRITE_REG((GPU_IRQ_MASK), 0x00000000);
	MALI_WRITE_REG((GPU_COMMAND), 0x00000001);

	/* CHECK RESET_COMPLETE @ 0x14AC0020 (GPU_IRQ_RAWSTAT) -> [8]:RESET_COMPLETED  */
	MALI_GPU_CONTROL_WAIT(GPU_IRQ_RAWSTAT, 0x100, 100);

	return ret;
}

int exynos_gpu_init_hw(void *dev)
{
#ifdef USE_ACLKG3D_FEEDBACK_CLK
	unsigned int temp;
#endif
	int i, count = 0;
	static int first_call = 1;
	struct kbase_device *kbdev = (struct kbase_device *)dev;
	struct exynos_context *platform = (struct exynos_context *)kbdev->platform_context;

#ifdef USE_ACLKG3D_FEEDBACK_CLK
	/* G3D_CFGREG0 register bit16 set to 1.
	 * It is change the blkg3d clock path from aclk-g3d to aclk-g3d feed-back path */
	temp = __raw_readl(EXYNOS7420_VA_SYSREG + 0x1234);
	temp |= (0x1 << 16);
	__raw_writel(temp, EXYNOS7420_VA_SYSREG + 0x1234);
#endif
	/* To provide the stable SRAM power */
	udelay(100);

	/* MALI_SEC */
	if (first_call == 1) {
		first_call = 0;
		for (i = 0; i < 5; i++)
			balance_init(kbdev);
	} else {
		do {
			if (balance_init(kbdev) == true)
				break;
			if (count == 5)
				break;
			count++;
		} while (1);
	}

	if (count > 0 && count < BMAX_RETRY_CNT)
		platform->balance_retry_count[count - 1] += 1;

	return count;
}
