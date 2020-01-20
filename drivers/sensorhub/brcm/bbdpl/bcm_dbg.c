#include <kernel.h>
#include <linux/printk.h>
#include <linux/time.h>

#define MAX_DBG_LOG_LEN 60
#define MAX_DBG_RECORD	(4096*128)

struct dbg_record {
	unsigned long time;
	char log[MAX_DBG_LOG_LEN];
};

static int dbg_idx;
static struct dbg_record rec[MAX_DBG_RECORD];

void record(const char *str)
{
	struct timeval tv;

	do_gettimeofday(&tv);
	rec[dbg_idx].time = tv.tv_sec * 1000000UL + tv.tv_usec;
	strncpy(rec[dbg_idx].log, str, MAX_DBG_LOG_LEN);
	dbg_idx = ++dbg_idx&(MAX_DBG_RECORD - 1);
}

void print_record(void)
{
	int i;

	for (i = 0; i < MAX_DBG_RECORD; i++)
		pr_info("<%lu>\t%s\n", rec[i].time, rec[i].log);
}
