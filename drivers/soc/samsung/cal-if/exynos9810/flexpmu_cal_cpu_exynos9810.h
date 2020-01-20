#ifndef ACPM_FRAMEWORK

/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 4), (0x0 << 4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 4), (0x1 << 4), 0x14070000, 0x0208, (0x1 << 4), (0x1 << 4) | (0x1 << 4)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 4), (0x0 << 4), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 4), (0x1 << 4), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 4), (0x1 << 4), 0x14070000, 0x0108, (0x1 << 4), (0x1 << 4) | (0x1 << 4)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 4), (0x1 << 4), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x14060000, 0x2024, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 5), (0x0 << 5), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 5), (0x1 << 5), 0x14070000, 0x0208, (0x1 << 5), (0x1 << 5) | (0x1 << 5)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 5), (0x0 << 5), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 5), (0x1 << 5), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 5), (0x1 << 5), 0x14070000, 0x0108, (0x1 << 5), (0x1 << 5) | (0x1 << 5)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 5), (0x1 << 5), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x14060000, 0x202c, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core02_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 6), (0x0 << 6), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 6), (0x1 << 6), 0x14070000, 0x0208, (0x1 << 6), (0x1 << 6) | (0x1 << 6)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 6), (0x0 << 6), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core02_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 6), (0x1 << 6), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 6), (0x1 << 6), 0x14070000, 0x0108, (0x1 << 6), (0x1 << 6) | (0x1 << 6)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 6), (0x1 << 6), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core02_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU2_STATUS", 0x14060000, 0x2034, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core03_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 7), (0x0 << 7), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 7), (0x1 << 7), 0x14070000, 0x0208, (0x1 << 7), (0x1 << 7) | (0x1 << 7)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 7), (0x0 << 7), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core03_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 7), (0x1 << 7), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 7), (0x1 << 7), 0x14070000, 0x0108, (0x1 << 7), (0x1 << 7) | (0x1 << 7)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 7), (0x1 << 7), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core03_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU3_STATUS", 0x14060000, 0x203c, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core10_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 0), (0x1 << 0), 0x14070000, 0x0208, (0x1 << 0), (0x1 << 0) | (0x1 << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 0), (0x1 << 0), 0x14070000, 0x0108, (0x1 << 0), (0x1 << 0) | (0x1 << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU0_STATUS", 0x14060000, 0x2004, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core11_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 1), (0x0 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 1), (0x1 << 1), 0x14070000, 0x0208, (0x1 << 1), (0x1 << 1) | (0x1 << 1)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 1), (0x0 << 1), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 1), (0x1 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 1), (0x1 << 1), 0x14070000, 0x0108, (0x1 << 1), (0x1 << 1) | (0x1 << 1)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 1), (0x1 << 1), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU1_STATUS", 0x14060000, 0x200c, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core12_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 2), (0x0 << 2), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 2), (0x1 << 2), 0x14070000, 0x0208, (0x1 << 2), (0x1 << 2) | (0x1 << 2)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 2), (0x0 << 2), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core12_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 2), (0x1 << 2), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 2), (0x1 << 2), 0x14070000, 0x0108, (0x1 << 2), (0x1 << 2) | (0x1 << 2)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 2), (0x1 << 2), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core12_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU2_STATUS", 0x14060000, 0x2014, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core13_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP2_INTR_BID_CLEAR", 0x14070000, 0x020c, (0x1 << 3), (0x1 << 3), 0x14070000, 0x0208, (0x1 << 3), (0x1 << 3) | (0x1 << 3)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 3), (0x0 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core13_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP2_INTR_BID_ENABLE", 0x14070000, 0x0200, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_COND_WRITE, "GRP1_INTR_BID_CLEAR", 0x14070000, 0x010c, (0x1 << 3), (0x1 << 3), 0x14070000, 0x0108, (0x1 << 3), (0x1 << 3) | (0x1 << 3)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "PMU_EVENT_INTERRUPT_ENABLE_GRP2", 0x14060000, 0x7f08, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq core13_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU3_STATUS", 0x14060000, 0x201c, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster0_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MAILBOX_APM2AP", 0x14100000, 0x0010, (0x1 << 24), (0x0 << 24), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster0_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MAILBOX_APM2AP", 0x14100000, 0x0010, (0x1 << 24), (0x1 << 24), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x14060000, 0x204c, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster1_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MAILBOX_APM2AP", 0x14100000, 0x0010, (0x1 << 24), (0x0 << 24), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster1_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MAILBOX_APM2AP", 0x14100000, 0x0010, (0x1 << 24), (0x1 << 24), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_NONCPU_STATUS", 0x14060000, 0x2044, (0xf << 0), 0, 0, 0, 0xffffffff, 0),
};
enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE02,
	CPU_CORE03,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE12,
	CPU_CORE13,
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
	[CPU_CORE12] = {
		.id = CPU_CORE12,
		.release = 0,
		.on = core12_on,
		.off = core12_off,
		.status = core12_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core12_on),
		.num_off = ARRAY_SIZE(core12_off),
		.num_status = ARRAY_SIZE(core12_status),
	},
	[CPU_CORE13] = {
		.id = CPU_CORE13,
		.release = 0,
		.on = core13_on,
		.off = core13_off,
		.status = core13_status,
		.num_release = 0,
		.num_on = ARRAY_SIZE(core13_on),
		.num_off = ARRAY_SIZE(core13_off),
		.num_status = ARRAY_SIZE(core13_status),
	},
};
unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
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
};
unsigned int pmucal_cluster_list_size = ARRAY_SIZE(pmucal_cluster_list);

enum pmucal_opsnum {
	NUM_PMUCAL_OPTIONS,
};

unsigned int pmucal_option_list_size;

#else

/* individual sequence descriptor for each core - on, off, status */
struct pmucal_seq core00_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU0_CONFIGURATION", 0x14060000, 0x2020, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core00_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_CONFIGURATION", 0x14060000, 0x2020, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_CONFIGURATION", 0x14060000, 0x2020, (0xf << 16), (0x0 << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CLUSTER0_CPU0_STATUS", 0x14060000, 0x2024, (0xf << 0), (0xF << 0), 0x14060000, 0x2024, (0xf << 0), (0xF << 0)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 4), (0x0 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core00_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU0_CONFIGURATION", 0x14060000, 0x2020, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 4), (0x1 << 4), 0, 0, 0, 0),
};
struct pmucal_seq core00_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU0_STATUS", 0x14060000, 0x2024, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core01_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU1_CONFIGURATION", 0x14060000, 0x2028, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 5), (0x0 << 5), 0, 0, 0, 0),
};
struct pmucal_seq core01_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU1_CONFIGURATION", 0x14060000, 0x2028, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 5), (0x0 << 5), 0, 0, 0, 0),
};
struct pmucal_seq core01_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU1_CONFIGURATION", 0x14060000, 0x2028, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 5), (0x1 << 5), 0, 0, 0, 0),
};
struct pmucal_seq core01_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU1_STATUS", 0x14060000, 0x202c, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core02_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU2_CONFIGURATION", 0x14060000, 0x2030, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 6), (0x0 << 6), 0, 0, 0, 0),
};
struct pmucal_seq core02_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU2_CONFIGURATION", 0x14060000, 0x2030, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 6), (0x0 << 6), 0, 0, 0, 0),
};
struct pmucal_seq core02_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU2_CONFIGURATION", 0x14060000, 0x2030, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 6), (0x1 << 6), 0, 0, 0, 0),
};
struct pmucal_seq core02_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU2_STATUS", 0x14060000, 0x2034, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core03_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU3_CONFIGURATION", 0x14060000, 0x2038, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 7), (0x0 << 7), 0, 0, 0, 0),
};
struct pmucal_seq core03_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU3_CONFIGURATION", 0x14060000, 0x2038, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 7), (0x0 << 7), 0, 0, 0, 0),
};
struct pmucal_seq core03_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_CPU3_CONFIGURATION", 0x14060000, 0x2038, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 7), (0x1 << 7), 0, 0, 0, 0),
};
struct pmucal_seq core03_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_CPU3_STATUS", 0x14060000, 0x203c, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core10_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU0_CONFIGURATION", 0x14060000, 0x2000, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU0_SYS_PWR_REG", 0x14060000, 0x1014, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU0_SYS_PWR_REG", 0x14060000, 0x100c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
};
struct pmucal_seq core10_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU0_CONFIGURATION", 0x14060000, 0x2000, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU0_SYS_PWR_REG", 0x14060000, 0x1014, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU0_SYS_PWR_REG", 0x14060000, 0x100c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
};
struct pmucal_seq core10_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU0_CONFIGURATION", 0x14060000, 0x2000, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
};
struct pmucal_seq core10_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU0_STATUS", 0x14060000, 0x2004, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core11_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU1_CONFIGURATION", 0x14060000, 0x2008, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU1_SYS_PWR_REG", 0x14060000, 0x1034, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU1_SYS_PWR_REG", 0x14060000, 0x102c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 1), (0x0 << 1), 0, 0, 0, 0),
};
struct pmucal_seq core11_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU1_CONFIGURATION", 0x14060000, 0x2008, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU1_SYS_PWR_REG", 0x14060000, 0x1034, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU1_SYS_PWR_REG", 0x14060000, 0x102c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 1), (0x0 << 1), 0, 0, 0, 0),
};
struct pmucal_seq core11_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU1_CONFIGURATION", 0x14060000, 0x2008, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 1), (0x1 << 1), 0, 0, 0, 0),
};
struct pmucal_seq core11_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU1_STATUS", 0x14060000, 0x200c, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core12_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU2_CONFIGURATION", 0x14060000, 0x2010, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU2_SYS_PWR_REG", 0x14060000, 0x1054, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU2_SYS_PWR_REG", 0x14060000, 0x104c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 2), (0x0 << 2), 0, 0, 0, 0),
};
struct pmucal_seq core12_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU2_CONFIGURATION", 0x14060000, 0x2010, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU2_SYS_PWR_REG", 0x14060000, 0x1054, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU2_SYS_PWR_REG", 0x14060000, 0x104c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 2), (0x0 << 2), 0, 0, 0, 0),
};
struct pmucal_seq core12_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU2_CONFIGURATION", 0x14060000, 0x2010, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 2), (0x1 << 2), 0, 0, 0, 0),
};
struct pmucal_seq core12_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU2_STATUS", 0x14060000, 0x2014, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq core13_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU3_CONFIGURATION", 0x14060000, 0x2018, (0xf << 16), (0xF << 16), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU3_SYS_PWR_REG", 0x14060000, 0x1074, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU3_SYS_PWR_REG", 0x14060000, 0x106c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 3), (0x0 << 3), 0, 0, 0, 0),
};
struct pmucal_seq core13_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU3_CONFIGURATION", 0x14060000, 0x2018, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_CPU3_SYS_PWR_REG", 0x14060000, 0x1074, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_CPU3_SYS_PWR_REG", 0x14060000, 0x106c, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 3), (0x0 << 3), 0, 0, 0, 0),
};
struct pmucal_seq core13_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_CPU3_CONFIGURATION", 0x14060000, 0x2018, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "GRP1_INTR_BID_ENABLE", 0x14070000, 0x0100, (0x1 << 3), (0x1 << 3), 0, 0, 0, 0),
};
struct pmucal_seq core13_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_CPU3_STATUS", 0x14060000, 0x201c, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster0_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_NONCPU_CONFIGURATION", 0x14060000, 0x2048, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "LPI_CSSYS_CONFIGURATION", 0x14060000, 0x2b60, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
};
struct pmucal_seq cluster0_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MAILBOX_APM2AP", 0x14100000, 0x0010, (0x1 << 24), (0x0 << 24), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU0_ETC_OPTION", 0x14060000, 0x3c48, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU1_ETC_OPTION", 0x14060000, 0x3c58, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU2_ETC_OPTION", 0x14060000, 0x3c68, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER0_CPU3_ETC_OPTION", 0x14060000, 0x3c78, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "LPI_CSSYS_CONFIGURATION", 0x14060000, 0x2b60, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER0_NONCPU_CONFIGURATION", 0x14060000, 0x2048, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
};
struct pmucal_seq cluster0_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER0_NONCPU_STATUS", 0x14060000, 0x204c, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq cluster1_on[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1D100000, 0x0850, (0x7f << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_NONCPU_CONFIGURATION", 0x14060000, 0x2040, (0xf << 0), (0xF << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "BISR_RETENTION_CLUSTER1_NONCPU_SYS_PWR_REG", 0x14060000, 0x10b4, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "OTP_CLUSTER1_NONCPU_SYS_PWR_REG", 0x14060000, 0x1098, (0x1 << 0), (0x1 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_ETC_RESET", 0x14060000, 0x3c0c, (0x3 << 12), (0x3 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_ETC_RESET", 0x14060000, 0x3c0c, (0x3 << 8), (0x3 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_ETC_RESET", 0x14060000, 0x3c1c, (0x3 << 12), (0x3 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_ETC_RESET", 0x14060000, 0x3c1c, (0x3 << 8), (0x3 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU2_ETC_RESET", 0x14060000, 0x3c2c, (0x3 << 12), (0x3 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU2_ETC_RESET", 0x14060000, 0x3c2c, (0x3 << 8), (0x3 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU3_ETC_RESET", 0x14060000, 0x3c3c, (0x3 << 12), (0x3 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU3_ETC_RESET", 0x14060000, 0x3c3c, (0x3 << 8), (0x3 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_NONCPU_ETC_RESET", 0x14060000, 0x3c8c, (0x1 << 31), (0x1 << 31), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_NONCPU_ETC_RESET", 0x14060000, 0x3c8c, (0x1 << 25), (0x1 << 25), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_CHECK_SKIP, "RESET_SEQUENCER_CONFIGURATION", 0x14060000, 0x0500, (0x1 << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_NONCPU_ETC_RESET", 0x14060000, 0x3c8c, (0x1 << 18), (0x1 << 18), 0, 0, 0, 0),
};
struct pmucal_seq cluster1_off[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "MAILBOX_APM2AP", 0x14100000, 0x0010, (0x1 << 24), (0x0 << 24), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_ETC_RESET", 0x14060000, 0x3c0c, (0x3 << 12), (0x0 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_ETC_RESET", 0x14060000, 0x3c0c, (0x3 << 8), (0x0 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_ETC_RESET", 0x14060000, 0x3c1c, (0x3 << 12), (0x0 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_ETC_RESET", 0x14060000, 0x3c1c, (0x3 << 8), (0x0 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU2_ETC_RESET", 0x14060000, 0x3c2c, (0x3 << 12), (0x0 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU2_ETC_RESET", 0x14060000, 0x3c2c, (0x3 << 8), (0x0 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU3_ETC_RESET", 0x14060000, 0x3c3c, (0x3 << 12), (0x0 << 12), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU3_ETC_RESET", 0x14060000, 0x3c3c, (0x3 << 8), (0x0 << 8), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_NONCPU_ETC_RESET", 0x14060000, 0x3c8c, (0x1 << 31), (0x0 << 31), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_NONCPU_ETC_RESET", 0x14060000, 0x3c8c, (0x1 << 25), (0x0 << 25), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_NONCPU_ETC_RESET", 0x14060000, 0x3c8c, (0x1 << 18), (0x0 << 18), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU0_ETC_OPTION", 0x14060000, 0x3c08, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU1_ETC_OPTION", 0x14060000, 0x3c18, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU2_ETC_OPTION", 0x14060000, 0x3c28, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLUSTER1_CPU3_ETC_OPTION", 0x14060000, 0x3c38, (0x1 << 17), (0x0 << 17), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CRP_PM_CC1_0", 0x1A000000, 0x01A4, (0x3 << 0), (0x3 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CRP_PM_CC1_0", 0x1A000000, 0x01A4, (0x1 << 16), (0x0 << 16), 0x1A000000, 0x01A4, (0x1 << 16), (0x0 << 16)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE_WAIT, "CLUSTER1_NONCPU_CONFIGURATION", 0x14060000, 0x2040, (0xf << 0), (0x0 << 0), 0, 0, 0, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CPUCL1_HCHGEN_CLKMUX", 0x1D100000, 0x0850, (0x7f << 0), (0x1 << 0), 0, 0, 0, 0),
};
struct pmucal_seq cluster1_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "CLUSTER1_NONCPU_STATUS", 0x14060000, 0x2044, (0xf << 0), 0, 0, 0, 0, 0),
};
struct pmucal_seq ops_l3flush_on[] = {
};
struct pmucal_seq ops_l3flush_off[] = {
};
struct pmucal_seq ops_l3flush_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "L3FLUSH_CLUSTER1_NONCPU_STATUS", 0x14060000, 0x2614, (0x1 << 0), 0, 0, 0, 0, 0),
};
enum pmucal_cpu_corenum {
	CPU_CORE00,
	CPU_CORE01,
	CPU_CORE02,
	CPU_CORE03,
	CPU_CORE10,
	CPU_CORE11,
	CPU_CORE12,
	CPU_CORE13,
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
	[CPU_CORE12] = {
		.id = CPU_CORE12,
		.release = core12_release,
		.on = core12_on,
		.off = core12_off,
		.status = core12_status,
		.num_release = ARRAY_SIZE(core12_release),
		.num_on = ARRAY_SIZE(core12_on),
		.num_off = ARRAY_SIZE(core12_off),
		.num_status = ARRAY_SIZE(core12_status),
	},
	[CPU_CORE13] = {
		.id = CPU_CORE13,
		.release = core13_release,
		.on = core13_on,
		.off = core13_off,
		.status = core13_status,
		.num_release = ARRAY_SIZE(core13_release),
		.num_on = ARRAY_SIZE(core13_on),
		.num_off = ARRAY_SIZE(core13_off),
		.num_status = ARRAY_SIZE(core13_status),
	},
};
unsigned int pmucal_cpu_list_size = ARRAY_SIZE(pmucal_cpu_list);

enum pmucal_cpu_clusternum {
	CPU_CLUSTER0,
	CPU_CLUSTER1,
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
		.max = 7,
		.total = 4,
	},
};
#endif
