#include <mali_midg_regmap.h>

#undef JOB_IRQ_CLEAR
#undef JOB_IRQ_MASK
#undef JOB_IRQ_RAWSTAT
#undef MMU_IRQ_CLEAR
#undef MMU_IRQ_MASK

/* re-definition */
#define JOB_IRQ_RAWSTAT         (JOB_CONTROL_BASE)
#define JOB_IRQ_CLEAR           (JOB_CONTROL_BASE + 0x004)
#define JOB_IRQ_MASK            (JOB_CONTROL_BASE + 0x008)

#define MMU_IRQ_CLEAR           (MEMORY_MANAGEMENT_BASE + 0x004)
#define MMU_IRQ_MASK            (MEMORY_MANAGEMENT_BASE + 0x008)

#define AS0_MEMATTR_HI          MMU_AS_REG(0, AS_MEMATTR_HI)
#define AS0_MEMATTR_LO          MMU_AS_REG(0, AS_MEMATTR_LO)
#define AS0_TRANSTAB_LO         MMU_AS_REG(0, AS_TRANSTAB_LO)
#define AS0_TRANSTAB_HI         MMU_AS_REG(0, AS_TRANSTAB_HI)
#define AS0_COMMAND             MMU_AS_REG(0, AS_COMMAND)

#define AS1_MEMATTR_HI          MMU_AS_REG(1, AS_MEMATTR_HI)
#define AS1_MEMATTR_LO          MMU_AS_REG(1, AS_MEMATTR_LO)
#define AS1_TRANSTAB_LO         MMU_AS_REG(1, AS_TRANSTAB_LO)
#define AS1_TRANSTAB_HI         MMU_AS_REG(1, AS_TRANSTAB_HI)
#define AS1_COMMAND             MMU_AS_REG(1, AS_COMMAND)

#define AS2_MEMATTR_HI          MMU_AS_REG(2, AS_MEMATTR_HI)
#define AS2_MEMATTR_LO          MMU_AS_REG(2, AS_MEMATTR_LO)
#define AS2_TRANSTAB_LO         MMU_AS_REG(2, AS_TRANSTAB_LO)
#define AS2_TRANSTAB_HI         MMU_AS_REG(2, AS_TRANSTAB_HI)
#define AS2_COMMAND             MMU_AS_REG(2, AS_COMMAND)

#define AS3_MEMATTR_HI          MMU_AS_REG(3, AS_MEMATTR_HI)
#define AS3_MEMATTR_LO          MMU_AS_REG(3, AS_MEMATTR_LO)
#define AS3_TRANSTAB_LO         MMU_AS_REG(3, AS_TRANSTAB_LO)
#define AS3_TRANSTAB_HI         MMU_AS_REG(3, AS_TRANSTAB_HI)
#define AS3_COMMAND             MMU_AS_REG(3, AS_COMMAND)

#define JS0_HEAD_NEXT_LO        JOB_SLOT_REG(0, JS_HEAD_NEXT_LO)
#define JS0_HEAD_NEXT_HI        JOB_SLOT_REG(0, JS_HEAD_NEXT_HI)
#define JS0_AFFINITY_NEXT_LO    JOB_SLOT_REG(0, JS_AFFINITY_NEXT_LO)
#define JS0_AFFINITY_NEXT_HI    JOB_SLOT_REG(0, JS_AFFINITY_NEXT_HI)
#define JS0_CONFIG_NEXT         JOB_SLOT_REG(0, JS_CONFIG_NEXT)
#define JS0_COMMAND_NEXT        JOB_SLOT_REG(0, JS_COMMAND_NEXT)

#define JS1_HEAD_NEXT_LO        JOB_SLOT_REG(1, JS_HEAD_NEXT_LO)
#define JS1_HEAD_NEXT_HI        JOB_SLOT_REG(1, JS_HEAD_NEXT_HI)
#define JS1_AFFINITY_NEXT_LO    JOB_SLOT_REG(1, JS_AFFINITY_NEXT_LO)
#define JS1_AFFINITY_NEXT_HI    JOB_SLOT_REG(1, JS_AFFINITY_NEXT_HI)
#define JS1_CONFIG_NEXT         JOB_SLOT_REG(1, JS_CONFIG_NEXT)
#define JS1_COMMAND_NEXT        JOB_SLOT_REG(1, JS_COMMAND_NEXT)

#define JS2_HEAD_NEXT_LO        JOB_SLOT_REG(2, JS_HEAD_NEXT_LO)
#define JS2_HEAD_NEXT_HI        JOB_SLOT_REG(2, JS_HEAD_NEXT_HI)
#define JS2_AFFINITY_NEXT_LO    JOB_SLOT_REG(2, JS_AFFINITY_NEXT_LO)
#define JS2_AFFINITY_NEXT_HI    JOB_SLOT_REG(2, JS_AFFINITY_NEXT_HI)
#define JS2_CONFIG_NEXT         JOB_SLOT_REG(2, JS_CONFIG_NEXT)
#define JS2_COMMAND_NEXT        JOB_SLOT_REG(2, JS_COMMAND_NEXT)

#define MALI_WRITE_REG(reg, val) kbase_os_reg_write(kbdev, (reg), (val))

#define MALI_GPU_CONTROL_WAIT(reg, val, timeout) do { \
		if (mali_wait_reg(kbdev, GPU_CONTROL_REG((reg)), (val), (timeout)) != true) { \
			return false; \
		} \
	} while (0)

#define MALI_JOB_IRQ_WAIT(val, timeout) do { \
		if (mali_wait_reg(kbdev, JOB_IRQ_RAWSTAT, (val), (timeout)) != true) { \
			return false; \
		} \
	} while (0)

#define MALI_WRITE_MEM(dst, val) do { \
		u32 *stream_reg32;\
		dma_addr_t phys_addr32;\
		stream_reg32 = (u32 *)phys_to_virt((dst));\
		phys_addr32 = dma_map_single(kbdev->dev, stream_reg32, 4, DMA_BIDIRECTIONAL); \
		*stream_reg = (u32)(val);\
		dma_unmap_single(kbdev->dev, phys_addr32, 4, DMA_BIDIRECTIONAL);\
	} while (0)

inline bool mali_wait_reg(struct kbase_device *kbdev, u32 reg, u32 val, u16 timeout)
{
	u32 temp;
	int count = 0;

	timeout /= 10;
	do {
		temp = kbase_os_reg_read(kbdev, reg);
		if ((temp & val) == val)
			break;
		if (count > timeout) {
			dev_dbg(kbdev->dev, "mali wait reg timeout\n");
			break;
		}
		udelay(10);
		count++;
	} while (1);

	return true;
}

typedef struct _checksum_block{
	unsigned long start;
	unsigned long end;
	unsigned long golden_sum;
} checksum_block;
