/*
 * Exynos CPU Performance Profiling
 * Author: Jungwook Kim, jwook1.kim@samsung.com
 * Created: 2014
 * Updated: 2016, 2019
 */

#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include "exynos_perf_cpu.h"
#include <linux/uaccess.h>

/* This is used only for cpu performance profiling */
