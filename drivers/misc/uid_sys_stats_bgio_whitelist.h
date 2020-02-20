#ifndef _UID_SYS_STATS_BGIO_WHITELIST_H_
#define _UID_SYS_STATS_BGIO_WHITELIST_H_

#define BG_STAT_VER		2

char *bgio_whitelist_by_name[] = {
	"com.android.vending",
	"com.google.android.gms",
	"com.android.providers.media",
	"com.google.android.googlequicksearchbox",
	"com.sec.spp.push",
};

int bgio_whitelist_by_uid[] = {
	0,
	1000,
};

#endif
