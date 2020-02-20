#ifndef _OLOG_KERNEL_H_
#define _OLOG_KERNEL_H_

#include <linux/unistd.h>
#include "olog.pb.h"

#define OLOG_CPU_FREQ_FILTER   1500000


#define ologk(...) perflog(PERFLOG_UNKNOWN, __VA_ARGS__)
extern void perflog(int type, const char *fmt, ...);

#endif
