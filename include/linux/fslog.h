/*
 * fslog internals
 */

#ifndef _LINUX_FSLOG_H
#define  _LINUX_FSLOG_H

int fslog_stlog(const char *fmt, ...);
int fslog_selog(const char *fmt, ...);
int fslog_kmsg_selog(const char *filter, int lines);

#ifdef CONFIG_PROC_STLOG
#define ST_LOG(fmt, ...) fslog_stlog(fmt, ##__VA_ARGS__)
#define SE_LOG(fmt, ...) fslog_selog(fmt, ##__VA_ARGS__)
#else
#define ST_LOG(fmt, ...)
#define SE_LOG(fmt, ...)
#endif /* CONFIG_PROC_STLOG */


#endif /* _FSLOG_H */
