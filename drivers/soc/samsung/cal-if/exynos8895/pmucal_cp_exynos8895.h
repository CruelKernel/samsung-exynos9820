/* individual sequence descriptor for CP control - init, reset, release, cp_active_clear, cp_reset_req_clear */
struct pmucal_seq cp_init[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__CP_PWRON", 0x16480000, 0x0030, (0x1 << 1), (0x1 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_S__CP_START", 0x16480000, 0x0034, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "RESET_SEQUENCER_STATUS", 0x16480000, 0x0504, (0x7 << 0), (0x5 << 0), 0x16480000, 0x0504, (0x7 << 0), (0x5 << 0)),
};
struct pmucal_seq cp_status[] = {
	PMUCAL_SEQ_DESC(PMUCAL_READ, "RESET_SEQUENCER_STATUS", 0x16480000, 0x0504, (0x7 << 0), 0, 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cp_reset_assert[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLEANY_BUS_CP_CONFIGURATION", 0x16480000, 0x3920, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CLEANY_BUS_CP_STATUS", 0x16480000, 0x3924, (0x3 << 16), (0x3 << 16), 0x16480000, 0x3924, (0x3 << 16), (0x3 << 16)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CENTRAL_SEQ_CP_CONFIGURATION", 0x16480000, 0x0280, (0x1 << 16), (0x0 << 16), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "RESET_AHEAD_CP_SYS_PWR_REG", 0x16480000, 0x1320, (0x3 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLEANY_BUS_CP_SYS_PWR_REG", 0x16480000, 0x1324, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "LOGIC_RESET_CP_SYS_PWR_REG", 0x16480000, 0x1328, (0x3 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "TCXO_GATE_CP_SYS_PWR_REG", 0x16480000, 0x132c, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_DISABLE_ISO_SYS_PWR_REG", 0x16480000, 0x1330, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_RESET_ISO_SYS_PWR_REG", 0x16480000, 0x1334, (0x1 << 0), (0x0 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__CP_PWRON", 0x16480000, 0x0030, (0x1 << 1), (0x0 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CENTRAL_SEQ_CP_STATUS__STATES", 0x16480000, 0x0284, (0xff << 16), (0x80 << 16), 0x16480000, 0x0284, (0xff << 16), (0x80 << 16)),
};
struct pmucal_seq cp_reset_release[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__SWEEPER_BYPASS_DATA_EN", 0x16480000, 0x0030, (0x1 << 16), (0x1 << 16), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__CP_PWRON", 0x16480000, 0x0030, (0x1 << 1), (0x1 << 1), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_S__CP_START", 0x16480000, 0x0034, (0x1 << 3), (0x1 << 3), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CENTRAL_SEQ_CP_STATUS__STATES", 0x16480000, 0x0284, (0xff << 16), (0x0 << 16), 0x16480000, 0x0284, (0xff << 16), (0x0 << 16)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CLEANY_BUS_CP_CONFIGURATION", 0x16480000, 0x3920, (0x1 << 0), (0x1 << 0), 0, 0, 0xffffffff, 0),
	PMUCAL_SEQ_DESC(PMUCAL_WAIT, "CLEANY_BUS_CP_STATUS", 0x16480000, 0x3924, (0x3 << 16), (0x0 << 16), 0x16480000, 0x3924, (0x3 << 16), (0x0 << 16)),
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__SWEEPER_BYPASS_DATA_EN", 0x16480000, 0x0030, (0x1 << 16), (0x0 << 16), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cp_cp_active_clear[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__CP_ACTIVE_CLR", 0x16480000, 0x0030, (0x1 << 6), (0x1 << 6), 0, 0, 0xffffffff, 0),
};
struct pmucal_seq cp_cp_reset_req_clear[] = {
	PMUCAL_SEQ_DESC(PMUCAL_WRITE, "CP_CTRL_NS__CP_RESET_REQ_CLR", 0x16480000, 0x0030, (0x1 << 8), (0x1 << 8), 0, 0, 0xffffffff, 0),
};
struct pmucal_cp pmucal_cp_list = {
		.init = cp_init,
		.status = cp_status,
		.reset_assert = cp_reset_assert,
		.reset_release = cp_reset_release,
		.cp_active_clear = cp_cp_active_clear,
		.cp_reset_req_clear = cp_cp_reset_req_clear,
		.num_init = ARRAY_SIZE(cp_init),
		.num_status = ARRAY_SIZE(cp_status),
		.num_reset_assert = ARRAY_SIZE(cp_reset_assert),
		.num_reset_release = ARRAY_SIZE(cp_reset_release),
		.num_cp_active_clear = ARRAY_SIZE(cp_cp_active_clear),
		.num_cp_reset_req_clear = ARRAY_SIZE(cp_cp_reset_req_clear),
};
unsigned int pmucal_cp_list_size = 1;
