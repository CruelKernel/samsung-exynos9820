#include <linux/ologk.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
/*
static int __init kperfmon_init(void)
{
	return 0;
}

static void __exit kperfmon_exit(void)
{

}
*/
void ologk(const char *fmt, ...) 
{

}
EXPORT_SYMBOL(ologk);
/*
module_init(kperfmon_init);
module_exit(kperfmon_exit);
*/