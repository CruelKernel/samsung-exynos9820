/*
* Samsung debugging features for Samsung's SoC's.
*
* Copyright (c) 2014-2017 Samsung Electronics Co., Ltd.
*      http://www.samsung.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*/

#ifndef SEC_DEBUG_EXTRA_INFO_KEYS_H
#define SEC_DEBUG_EXTRA_INFO_KEYS_H

/* keys are grouped by size */
char key32[][MAX_ITEM_KEY_LEN] = {
	"ID", "KTIME", "BIN", "FTYPE", "RR",
	"DPM", "SMP", "MER", "PCB", "SMD",
	"CHI", "LPI", "CDI", "LEV", "DCN",
	"WAK", "ASB", "PSITE", "DDRID", "RST",
	"INFO2", "INFO3", "RBASE", "MAGIC", "PWR",
	"PWROFF", "PINT1", "PINT2", "PINT5", "PINT6",
	"PSTS1", "PSTS2", "RSTCNT", "FRQL1",
	"FRQL2", "FRQB0", "FRQB1", "FRQB2", "FRQI0",
	"FRQI1", "FRQI2", "FRQM0", "FRQM1", "FRQM2",
};

char key64[][MAX_ITEM_KEY_LEN] = {
	"ETC", "BAT", "FAULT", "PINFO", "HINT",
};

char key256[][MAX_ITEM_KEY_LEN] = {
	"KLG", "BUS", "PANIC", "PC", "LR",
	"BUG", "ESR", "SMU", "FRQL0", "ODR",
	"AUD",
};

char key1024[][MAX_ITEM_KEY_LEN] = {
	"CPU0", "CPU1", "CPU2", "CPU3", "CPU4",
	"CPU5", "CPU6", "CPU7", "MFC", "STACK",
};


/* keys are grouped by sysfs node */
char akeys[][MAX_ITEM_KEY_LEN] = {
	"ID", "KTIME", "BIN", "FTYPE", "FAULT",
	"BUG", "PC", "LR", "STACK", "RR",
	"RSTCNT", "PINFO", "SMU", "BUS", "DPM",
	"ETC", "ESR", "MER", "PCB", "SMD",
	"CHI", "LPI", "CDI", "KLG", "HINT",
	"LEV", "DCN", "WAK", "BAT", "SMP",
	"PANIC",
};

char bkeys[][MAX_ITEM_KEY_LEN] = {
	"ID", "RR", "ASB", "PSITE", "DDRID",
	"RST", "INFO2", "INFO3", "RBASE", "MAGIC",
	"PWR", "PWROFF", "PINT1", "PINT2", "PINT5",
	"PINT6", "PSTS1", "PSTS2", "FRQL0",
};

char ckeys[][MAX_ITEM_KEY_LEN] = {
	"ID", "RR", "CPU0", "CPU1", "CPU2",
	"CPU3", "CPU4", "CPU5", "CPU6", "CPU7",
};

char mkeys[][MAX_ITEM_KEY_LEN] = {
	"ID", "RR", "MFC", "ODR", "AUD",
};

#endif	/* SEC_DEBUG_EXTRA_INFO_KEYS_H */
