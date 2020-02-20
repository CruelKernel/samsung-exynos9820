#ifndef _OLOG_KERNEL_H_
#define _OLOG_KERNEL_H_

#include <linux/unistd.h>
#include "olog.pb.h"

#define OLOG_CPU_FREQ_FILTER   1500000

#define ologk(...) _perflog(PERFLOG_LOG, PERFLOG_UNKNOWN, __VA_ARGS__)
#define perflog(...) _perflog(PERFLOG_LOG, __VA_ARGS__)
extern void _perflog(int type, int logid, const char *fmt, ...);
extern void perflog_evt(int logid, int arg1);

#endif
