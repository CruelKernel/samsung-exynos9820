/*
 * PMU Counter API
 * Author: Jungwook Kim, jwook1.kim@samsung.com
 *
 */

typedef unsigned long ccnt_t;

#ifdef _PMU_COUNT_64
static inline void pmu_enable_counter(u64 val)
{
	val = 1L << val;
	asm volatile("msr PMCNTENSET_EL0, %0" : : "r" (val));
}

static inline void pmu_disable_counter(u64 val)
{
	val = 1L << val;
	asm volatile("msr PMCNTENCLR_EL0, %0" : : "r" (val));
}

static inline void pmnc_config(u64 counter, u64 event)
{
	counter = counter & 0x1F;
	asm volatile("msr PMSELR_EL0, %0" : : "r" (counter));
	asm volatile("msr PMXEVTYPER_EL0, %0" : : "r" (event));
}

static inline void pmnc_reset(void)
{
	u64 val;
	asm volatile("mrs %0, PMCR_EL0" : "=r"(val));
	val = val | 0x2;
	asm volatile("msr PMCR_EL0, %0" : : "r"(val));
}

static inline ccnt_t pmnc_read2(u64 counter)
{
	ccnt_t ret;
	counter = counter & 0x1F;
	asm volatile("msr PMSELR_EL0, %0" : : "r" (counter));
	asm volatile("mrs %0, PMXEVCNTR_EL0" : "=r"(ret));
	return ret;
}

static inline void enable_pmu(void)
{
	u64 val;
	asm volatile("mrs %0, PMCR_EL0" : "=r"(val));
	val = val | 0x1;	// set E bit
	asm volatile("msr PMCR_EL0, %0" : : "r"(val));
}

static inline void disable_pmu(void)
{
	u64 val;
	asm volatile("mrs %0, PMCR_EL0" : "=r"(val));
	val = val & ~0x1;	// clear E bit
	asm volatile("msr PMCR_EL0, %0" : : "r"(val));
}

static inline u32 read_pmnc(void)
{
	u64 val;
	asm volatile("mrs %0, PMCR_EL0" : "=r"(val));
	return val;
}

static inline void disable_ccnt(void)
{
	u64 val = 0x80000000;
	asm volatile("msr PMCNTENCLR_EL0, %0" : : "r" (val));
}

static inline void enable_ccnt(void)
{
	u64 val = 0x80000000;
	asm volatile("msr PMCNTENSET_EL0, %0" : : "r" (val));
}

static inline void reset_ccnt(void)
{
	u64 val;
	asm volatile("mrs %0, PMCR_EL0" : "=r"(val));
	val = val | 0x4;	// set C bit
	asm volatile("msr PMCR_EL0, %0" : : "r"(val));
}

static inline ccnt_t read_ccnt(void)
{
	ccnt_t val;
	asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(val));
	return val;
}
#else
static inline void pmu_enable_counter(u32 val)
{
	val = 1 << val;
	asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r" (val));
}

static inline void pmu_disable_counter(u32 val)
{
	val = 1 << val;
	asm volatile("mcr p15, 0, %0, c9, c12, 2" : : "r" (val));
}

static inline void pmnc_config(u32 counter, u32 event)
{
	counter = counter & 0x1F;
	asm volatile("mcr p15, 0, %0, c9, c12, 5" : : "r"(counter));
	asm volatile("mcr p15, 0, %0, c9, c13, 1" : : "r"(event));
}

static inline void pmnc_reset(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	val = val | 0x2;
	asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
}

static inline ccnt_t pmnc_read2(u32 counter)
{
	ccnt_t ret;
	counter = counter & 0x1F;
	asm volatile("mcr p15, 0, %0, c9, c12, 5" : : "r"(counter));
	asm volatile("mrc p15, 0, %0, c9, c13, 2" : "=r"(ret));
	return ret;
}

static inline void enable_pmu(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	val = val | 0x1;	// set E bit
	asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
}

static inline void disable_pmu(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	val = val & ~0x1;	// clear E bit
	asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
}

static inline u32 read_pmnc(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	return val;
}

static inline void disable_ccnt(void)
{
	u32 val = 0x80000000;
	asm volatile("mcr p15, 0, %0, c9, c12, 2" : : "r"(val));
}

static inline void enable_ccnt(void)
{
	u32 val = 0x80000000;
	asm volatile("mcr p15, 0, %0, c9, c12, 1" : : "r"(val));
}

static inline void reset_ccnt(void)
{
	u32 val;
	asm volatile("mrc p15, 0, %0, c9, c12, 0" : "=r"(val));
	val = val | 0x4;	// set C bit
	asm volatile("mcr p15, 0, %0, c9, c12, 0" : : "r"(val));
}

static inline ccnt_t read_ccnt(void)
{
	ccnt_t val;
	asm volatile("mrc p15, 0, %0, c9, c13, 0" : "=r"(val));
	return val;
}
#endif

#define __start_timer(x) (enable_pmu(), disable_ccnt(), reset_ccnt(), enable_ccnt())
#define __stop_timer(x) read_ccnt()
