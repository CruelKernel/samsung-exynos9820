#ifndef ACPM_FRAMEWORK

/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 0), (0x1 << 0), 0x15870000, 0x0208, (0x1 << 0), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_INT_EN", 0x15860000, 0x1044, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 0), (0x1 << 0), 0x15870000, 0x0108, (0x1 << 0), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 8), (0x1 << 8), 0x15870000, 0x0108, (0x1 << 8), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_INT_EN", 0x15860000, 0x1044, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x15860000, 0x1004, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 1), (0x0 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 1), (0x1 << 1), 0x15870000, 0x0208, (0x1 << 1), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU1_INT_EN", 0x15860000, 0x10c4, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 1), (0x1 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 1), (0x1 << 1), 0x15870000, 0x0108, (0x1 << 1), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 9), (0x1 << 9), 0x15870000, 0x0108, (0x1 << 9), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU1_INT_EN", 0x15860000, 0x10c4, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x15860000, 0x1084, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core02_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 2), (0x0 << 2), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 2), (0x1 << 2), 0x15870000, 0x0208, (0x1 << 2), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU2_INT_EN", 0x15860000, 0x1144, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core02_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 2), (0x1 << 2), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 2), (0x1 << 2), 0x15870000, 0x0108, (0x1 << 2), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 10), (0x1 << 10), 0x15870000, 0x0108, (0x1 << 10), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU2_INT_EN", 0x15860000, 0x1144, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core02_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU2_STATUS", 0x15860000, 0x1104, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core03_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 3), (0x1 << 3), 0x15870000, 0x0208, (0x1 << 3), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU3_INT_EN", 0x15860000, 0x11c4, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core03_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 3), (0x1 << 3), 0x15870000, 0x0108, (0x1 << 3), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 11), (0x1 << 11), 0x15870000, 0x0108, (0x1 << 11), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU3_INT_EN", 0x15860000, 0x11c4, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core03_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU3_STATUS", 0x15860000, 0x1184, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core10_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 4), (0x0 << 4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 4), (0x1 << 4), 0x15870000, 0x0208, (0x1 << 4), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_INT_EN", 0x15860000, 0x1344, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 4), (0x1 << 4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 4), (0x1 << 4), 0x15870000, 0x0108, (0x1 << 4), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 12), (0x1 << 12), 0x15870000, 0x0108, (0x1 << 12), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_INT_EN", 0x15860000, 0x1344, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU0_STATUS", 0x15860000, 0x1304, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core11_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 5), (0x0 << 5), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 5), (0x1 << 5), 0x15870000, 0x0208, (0x1 << 5), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_INT_EN", 0x15860000, 0x13c4, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 5), (0x1 << 5), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 5), (0x1 << 5), 0x15870000, 0x0108, (0x1 << 5), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 13), (0x1 << 13), 0x15870000, 0x0108, (0x1 << 13), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_INT_EN", 0x15860000, 0x13c4, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU1_STATUS", 0x15860000, 0x1384, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core20_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 6), (0x0 << 6), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 6), (0x1 << 6), 0x15870000, 0x0208, (0x1 << 6), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER2_CPU0_INT_EN", 0x15860000, 0x1544, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core20_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 6), (0x1 << 6), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 6), (0x1 << 6), 0x15870000, 0x0108, (0x1 << 6), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 14), (0x1 << 14), 0x15870000, 0x0108, (0x1 << 14), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER2_CPU0_INT_EN", 0x15860000, 0x1544, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core20_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU0_STATUS", 0x15860000, 0x1504, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core21_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 7), (0x0 << 7), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP2_INTR_BID_CLEAR", 0x15870000, 0x020c, (0x1 << 7), (0x1 << 7), 0x15870000, 0x0208, (0x1 << 7), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER2_CPU1_INT_EN", 0x15860000, 0x15c4, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core21_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x15870000, 0x0200, (0x1 << 7), (0x1 << 7), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 7), (0x1 << 7), 0x15870000, 0x0108, (0x1 << 7), 0),
	PMUCAL_SEQ_DESC(PMUCAL_CLEAR_PEND, "GRP1_INTR_BID_CLEAR", 0x15870000, 0x010c, (0x1 << 15), (0x1 << 15), 0x15870000, 0x0108, (0x1 << 15), 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER2_CPU1_INT_EN", 0x15860000, 0x15c4, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core21_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU1_STATUS", 0x15860000, 0x1584, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x15860000, 0x1204, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x15860000, 0x1204, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster2_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_STATUS", 0x15860000, 0x1604, (0x1 << 0), 0, 0, 0, 0xffffffff, 0),
};
enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE02,
	CPU_CORE03,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE20,
	CPU_CORE21,
	PMUCAL_NUM_CORES,
};

struct pmucal_cpu pmucal_cpu_list[PMUCAL_NUM_CORES] = {
	[CPU_CORE00] = {
		.id = CPU_CORE00,
		.release = 0,
		.on = core00_on,
		.off = core00_off,
		.status = core00_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core00_on),
		.num_off = ARRAY_SIZE(core00_off),
		.num_status = ARRAY_SIZE(core00_status),
	},
	[CPU_CORE01] = {
		.id = CPU_CORE01,
		.release = 0,
		.on = core01_on,
		.off = core01_off,
		.status = core01_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core01_on),
		.num_off = ARRAY_SIZE(core01_off),
		.num_status = ARRAY_SIZE(core01_status),
	},
	[CPU_CORE02] = {
		.id = CPU_CORE02,
		.release = 0,
		.on = core02_on,
		.off = core02_off,
		.status = core02_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core02_on),
		.num_off = ARRAY_SIZE(core02_off),
		.num_status = ARRAY_SIZE(core02_status),
	},
	[CPU_CORE03] = {
		.id = CPU_CORE03,
		.release = 0,
		.on = core03_on,
		.off = core03_off,
		.status = core03_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core03_on),
		.num_off = ARRAY_SIZE(core03_off),
		.num_status = ARRAY_SIZE(core03_status),
	},
	[CPU_CORE10] = {
		.id = CPU_CORE10,
		.release = 0,
		.on = core10_on,
		.off = core10_off,
		.status = core10_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core10_on),
		.num_off = ARRAY_SIZE(core10_off),
		.num_status = ARRAY_SIZE(core10_status),
	},
	[CPU_CORE11] = {
		.id = CPU_CORE11,
		.release = 0,
		.on = core11_on,
		.off = core11_off,
		.status = core11_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core11_on),
		.num_off = ARRAY_SIZE(core11_off),
		.num_status = ARRAY_SIZE(core11_status),
	},
	[CPU_CORE20] = {
		.id = CPU_CORE20,
		.release = 0,
		.on = core20_on,
		.off = core20_off,
		.status = core20_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core20_on),
		.num_off = ARRAY_SIZE(core20_off),
		.num_status = ARRAY_SIZE(core20_status),
	},
	[CPU_CORE21] = {
		.id = CPU_CORE21,
		.release = 0,
		.on = core21_on,
		.off = core21_off,
		.status = core21_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core21_on),
		.num_off = ARRAY_SIZE(core21_off),
		.num_status = ARRAY_SIZE(core21_status),
	},
};
unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
	CPU_CLUSTER2,
	PMUCAL_NUM_CLUSTERS,
};

struct pmucal_cpu pmucal_cluster_list[PMUCAL_NUM_CLUSTERS] = {
	[CPU_CLUSTER0] = {
		.id = CPU_CLUSTER0,
		.on = 0,
		.off = 0,
		.status = cluster0_status,
		.num_on = 0,
		.num_off = 0,
		.num_status = ARRAY_SIZE(cluster0_status),
	},
	[CPU_CLUSTER1] = {
		.id = CPU_CLUSTER1,
		.on = 0,
		.off = 0,
		.status = cluster1_status,
		.num_on = 0,
		.num_off = 0,
		.num_status = ARRAY_SIZE(cluster1_status),
	},
	[CPU_CLUSTER2] = {
		.id = CPU_CLUSTER2,
		.on = 0,
		.off = 0,
		.status = cluster2_status,
		.num_on = 0,
		.num_off = 0,
		.num_status = ARRAY_SIZE(cluster2_status),
	},
};
unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	NUM_PMUCAL_OPTIONS,
};

struct pmucal_cpu pmucal_pmu_ops_list[] = {};

unsigned int pmucal_option_list_size = 0;

#else

/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x0, 0, 0, 0, 0),
};
struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x15860000, 0x1004, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core01_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x1, 0, 0, 0, 0),
};
struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x1, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x1, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x15860000, 0x1084, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core02_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x2, 0, 0, 0, 0),
};
struct pmucal_seq core02_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x2, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core02_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x2, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core02_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU2_STATUS", 0x15860000, 0x1104, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core03_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_reset_release", 0, 0, 0x20, 0x3, 0, 0, 0, 0),
};
struct pmucal_seq core03_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_up", 0, 0, 0x21, 0x3, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core03_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_cpu_down", 0, 0, 0x22, 0x3, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core03_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU3_STATUS", 0x15860000, 0x1184, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core10_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x0, 0, 0, 0, 0),
};
struct pmucal_seq core10_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU0_STATUS", 0x15860000, 0x1304, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core11_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_reset_release", 0, 0, 0x30, 0x1, 0, 0, 0, 0),
};
struct pmucal_seq core11_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_up", 0, 0, 0x31, 0x1, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster1_cpu_down", 0, 0, 0x32, 0x1, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1d030000, 0x0840, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU1_STATUS", 0x15860000, 0x1384, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core20_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_reset_release", 0, 0, 0x40, 0x0, 0, 0, 0, 0),
};
struct pmucal_seq core20_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_up", 0, 0, 0x41, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core20_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_down", 0, 0, 0x42, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core20_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU0_STATUS", 0x15860000, 0x1504, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core21_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_reset_release", 0, 0, 0x40, 0x1, 0, 0, 0, 0),
};
struct pmucal_seq core21_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_up", 0, 0, 0x41, 0x1, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core21_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_cpu_down", 0, 0, 0x42, 0x1, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core21_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_CPU1_STATUS", 0x15860000, 0x1584, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster0_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_up", 0, 0, 0x28, 0x0, 0, 0, 0, 0),
};
struct pmucal_seq cluster0_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL0_HCHGEN_CLKMUX", 0x1d020000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster0_noncpu_down", 0, 0, 0x29, 0x0, 0, 0, 0, 0),
};
struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x15860000, 0x1204, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster1_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x15860000, 0x1204, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster1_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x15860000, 0x1204, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x15860000, 0x1204, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster2_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_noncpu_up", 0, 0, 0x48, 0x0, 0, 0, 0, 0),
};
struct pmucal_seq cluster2_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CRP_PM_CC1_0", 0x1A000000, 0x01A4, (0x3 << 0), (0x3 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CRP_PM_CC1_3", 0x1A000000, 0x021C, (0x3 << 0), (0x3 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CRP_PM_CC1_0", 0x1A000000, 0x01A4, (0x1 << 16), (0x0 << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CRP_PM_CC1_3", 0x1A000000, 0x021C, (0x1 << 16), (0x0 << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_EXT_FUNC, "cluster2_noncpu_down", 0, 0, 0x49, 0x0, 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "CPUCL2_OPTION", 0x15860000, 0x1c8c, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL2_HCHGEN_CLKMUX", 0x1d120000, 0x0850, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq cluster2_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_STATUS", 0x15860000, 0x1604, (0x1 << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq ops_l3flush_on[] = {
};
struct pmucal_seq ops_l3flush_off[] = {
};
struct pmucal_seq ops_l3flush_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER2_NONCPU_INT_IN", 0x15860000, 0x1640, (0x1 << 2), 0, 0, 0, 0, 0),
};
enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE02,
	CPU_CORE03,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE20,
	CPU_CORE21,
	PMUCAL_NUM_CORES,
};

struct pmucal_cpu pmucal_cpu_list[PMUCAL_NUM_CORES] = {
	[CPU_CORE00] = {
		.id = CPU_CORE00,
		.release = core00_release,
		.on = core00_on,
		.off = core00_off,
		.status = core00_status,
		.num_release = ARRAY_SIZE(core00_release),
		.num_on = ARRAY_SIZE(core00_on),
		.num_off = ARRAY_SIZE(core00_off),
		.num_status = ARRAY_SIZE(core00_status),
	},
	[CPU_CORE01] = {
		.id = CPU_CORE01,
		.release = core01_release,
		.on = core01_on,
		.off = core01_off,
		.status = core01_status,
		.num_release = ARRAY_SIZE(core01_release),
		.num_on = ARRAY_SIZE(core01_on),
		.num_off = ARRAY_SIZE(core01_off),
		.num_status = ARRAY_SIZE(core01_status),
	},
	[CPU_CORE02] = {
		.id = CPU_CORE02,
		.release = core02_release,
		.on = core02_on,
		.off = core02_off,
		.status = core02_status,
		.num_release = ARRAY_SIZE(core02_release),
		.num_on = ARRAY_SIZE(core02_on),
		.num_off = ARRAY_SIZE(core02_off),
		.num_status = ARRAY_SIZE(core02_status),
	},
	[CPU_CORE03] = {
		.id = CPU_CORE03,
		.release = core03_release,
		.on = core03_on,
		.off = core03_off,
		.status = core03_status,
		.num_release = ARRAY_SIZE(core03_release),
		.num_on = ARRAY_SIZE(core03_on),
		.num_off = ARRAY_SIZE(core03_off),
		.num_status = ARRAY_SIZE(core03_status),
	},
	[CPU_CORE10] = {
		.id = CPU_CORE10,
		.release = core10_release,
		.on = core10_on,
		.off = core10_off,
		.status = core10_status,
		.num_release = ARRAY_SIZE(core10_release),
		.num_on = ARRAY_SIZE(core10_on),
		.num_off = ARRAY_SIZE(core10_off),
		.num_status = ARRAY_SIZE(core10_status),
	},
	[CPU_CORE11] = {
		.id = CPU_CORE11,
		.release = core11_release,
		.on = core11_on,
		.off = core11_off,
		.status = core11_status,
		.num_release = ARRAY_SIZE(core11_release),
		.num_on = ARRAY_SIZE(core11_on),
		.num_off = ARRAY_SIZE(core11_off),
		.num_status = ARRAY_SIZE(core11_status),
	},
	[CPU_CORE20] = {
		.id = CPU_CORE20,
		.release = core20_release,
		.on = core20_on,
		.off = core20_off,
		.status = core20_status,
		.num_release = ARRAY_SIZE(core20_release),
		.num_on = ARRAY_SIZE(core20_on),
		.num_off = ARRAY_SIZE(core20_off),
		.num_status = ARRAY_SIZE(core20_status),
	},
	[CPU_CORE21] = {
		.id = CPU_CORE21,
		.release = core21_release,
		.on = core21_on,
		.off = core21_off,
		.status = core21_status,
		.num_release = ARRAY_SIZE(core21_release),
		.num_on = ARRAY_SIZE(core21_on),
		.num_off = ARRAY_SIZE(core21_off),
		.num_status = ARRAY_SIZE(core21_status),
	},
};
unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
	CPU_CLUSTER2,
	PMUCAL_NUM_CLUSTERS,
};

struct pmucal_cpu pmucal_cluster_list[PMUCAL_NUM_CLUSTERS] = {
	[CPU_CLUSTER0] = {
		.id = CPU_CLUSTER0,
		.on = cluster0_on,
		.off = cluster0_off,
		.status = cluster0_status,
		.num_on = ARRAY_SIZE(cluster0_on),
		.num_off = ARRAY_SIZE(cluster0_off),
		.num_status = ARRAY_SIZE(cluster0_status),
	},
	[CPU_CLUSTER1] = {
		.id = CPU_CLUSTER1,
		.on = cluster1_on,
		.off = cluster1_off,
		.status = cluster1_status,
		.num_on = ARRAY_SIZE(cluster1_on),
		.num_off = ARRAY_SIZE(cluster1_off),
		.num_status = ARRAY_SIZE(cluster1_status),
	},
	[CPU_CLUSTER2] = {
		.id = CPU_CLUSTER2,
		.on = cluster2_on,
		.off = cluster2_off,
		.status = cluster2_status,
		.num_on = ARRAY_SIZE(cluster2_on),
		.num_off = ARRAY_SIZE(cluster2_off),
		.num_status = ARRAY_SIZE(cluster2_status),
	},
};
unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	OPS_L3FLUSH,
	NUM_PMUCAL_OPTIONS,
};

struct pmucal_cpu pmucal_pmu_ops_list[NUM_PMUCAL_OPTIONS] = {
	[OPS_L3FLUSH] = {
		.id = OPS_L3FLUSH,
		.on = ops_l3flush_on,
		.off = ops_l3flush_off,
		.status = ops_l3flush_status,
		.num_on = ARRAY_SIZE(ops_l3flush_on),
		.num_off = ARRAY_SIZE(ops_l3flush_off),
		.num_status = ARRAY_SIZE(ops_l3flush_status),
	},
};
unsigned int pmucal_option_list_size = ARRAY_SIZE(pmucal_pmu_ops_list);

struct cpu_info cpuinfo[] = {
	[0] = {
		.min = 0,
		.max = 3,
		.total = 4,
	},
	[1] = {
		.min = 4,
		.max = 5,
		.total = 2,
	},
	[2] = {
		.min = 6,
		.max = 7,
		.total = 2,
	},
};
#endif
