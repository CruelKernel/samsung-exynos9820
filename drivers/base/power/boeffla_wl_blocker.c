/*
 * Author: andip71, 01.09.2017
 *
 * Version 1.1.0
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * Change log:
 *
 * 1.1.0 (01.09.2017)
 *   - By default, the following wakelocks are blocked in an own list
 *     qcom_rx_wakelock, wlan, wlan_wow_wl, wlan_extscan_wl, NETLINK
 *
 * 1.0.1 (29.08.2017)
 *   - Add killing wakelock when currently active
 *
 * 1.0.0 (28.08.2017)
 *   - Initial version
 *
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/printk.h>
#include "boeffla_wl_blocker.h"


/*****************************************/
// Variables
/*****************************************/

char list_wl[LENGTH_LIST_WL] = {0};
char list_wl_default[LENGTH_LIST_WL_DEFAULT] = {0};

extern char list_wl_search[LENGTH_LIST_WL_SEARCH];
extern bool wl_blocker_active;
extern bool wl_blocker_debug;


/*****************************************/
// internal functions
/*****************************************/

static void build_search_string(char *list1, char *list2)
{
	// store wakelock list and search string (with semicolons added at start and end)
	sprintf(list_wl_search, ";%s;%s;", list1, list2);

	// set flag if wakelock blocker should be active (for performance reasons)
	if (strlen(list_wl_search) > 5)
		wl_blocker_active = true;
	else
		wl_blocker_active = false;
}


/*****************************************/
// sysfs interface functions
/*****************************************/

// show list of user configured wakelocks
static ssize_t wakelock_blocker_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	// return list of wakelocks to be blocked
	return scnprintf(buf, PAGE_SIZE, "%s\n", list_wl);
}


// store list of user configured wakelocks
static ssize_t wakelock_blocker_store(struct device * dev, struct device_attribute *attr,
			     const char * buf, size_t n)
{
	// check if string is too long to be stored
	if (n > LENGTH_LIST_WL)
		return -EINVAL;

	// store user configured wakelock list and rebuild search string
	sscanf(buf, "%s", list_wl);
	build_search_string(list_wl_default, list_wl);

	return n;
}


// show list of default, predefined wakelocks
static ssize_t wakelock_blocker_default_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	// return list of wakelocks to be blocked
	return scnprintf(buf, PAGE_SIZE, "%s\n", list_wl_default);
}


// store list of default, predefined wakelocks
static ssize_t wakelock_blocker_default_store(struct device * dev, struct device_attribute *attr,
			     const char * buf, size_t n)
{
	// check if string is too long to be stored
	if (n > LENGTH_LIST_WL_DEFAULT)
		return -EINVAL;

	// store default, predefined wakelock list and rebuild search string
	sscanf(buf, "%s", list_wl_default);
	build_search_string(list_wl_default, list_wl);

	return n;
}


// show debug information of driver internals
static ssize_t debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// return current debug status
	return scnprintf(buf, PAGE_SIZE,
			 "Debug status: %d\n\nUser list: %s\nDefault list: %s\n"
			 "Search list: %s\nActive: %d\n",
			 wl_blocker_debug, list_wl, list_wl_default,
			 list_wl_search, wl_blocker_active);
}


static int parse_strtoul(const char *buf, unsigned long max, unsigned long *value)
{
	char *endp;

	*value = simple_strtoul(skip_spaces(buf), &endp, 0);
	endp = skip_spaces(endp);
	if (*endp || *value > max)
		return -EINVAL;

	return 0;
}


// store debug mode on/off (1/0)
static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	ssize_t ret = -EINVAL;
	unsigned long val;

	// check data and store if valid
	ret = parse_strtoul(buf, 1, &val);

	if (ret)
		return ret;

	if (val)
		wl_blocker_debug = true;
	else
		wl_blocker_debug = false;

	return count;
}


static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// return version information
	return scnprintf(buf, PAGE_SIZE, "%s\n", BOEFFLA_WL_BLOCKER_VERSION);
}



/*****************************************/
// Initialize sysfs objects
/*****************************************/

// define objects
static DEVICE_ATTR_RW(wakelock_blocker);
static DEVICE_ATTR_RW(wakelock_blocker_default);
static DEVICE_ATTR_RW(debug);
static DEVICE_ATTR_RO(version);

// define attributes
static struct attribute *boeffla_wl_blocker_attributes[] = {
	&dev_attr_wakelock_blocker.attr,
	&dev_attr_wakelock_blocker_default.attr,
	&dev_attr_debug.attr,
	&dev_attr_version.attr,
	NULL
};

// define attribute group
static struct attribute_group boeffla_wl_blocker_control_group = {
	.attrs = boeffla_wl_blocker_attributes,
};

// define control device
static struct miscdevice boeffla_wl_blocker_control_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "boeffla_wakelock_blocker",
};


/*****************************************/
// Driver init and exit functions
/*****************************************/

static int boeffla_wl_blocker_init(void)
{
	// register boeffla wakelock blocker control device
	misc_register(&boeffla_wl_blocker_control_device);
	if (sysfs_create_group(&boeffla_wl_blocker_control_device.this_device->kobj,
				&boeffla_wl_blocker_control_group) < 0) {
		printk("Boeffla WL blocker: failed to create sys fs object.\n");
		return 0;
	}

	// initialize default list
	sprintf(list_wl_default, "%s", LIST_WL_DEFAULT);
	build_search_string(list_wl_default, list_wl);

	// Print debug info
	printk("Boeffla WL blocker: driver version %s started\n", BOEFFLA_WL_BLOCKER_VERSION);

	return 0;
}


static void boeffla_wl_blocker_exit(void)
{
	// remove boeffla wakelock blocker control device
	sysfs_remove_group(&boeffla_wl_blocker_control_device.this_device->kobj,
                           &boeffla_wl_blocker_control_group);

	// Print debug info
	printk("Boeffla WL blocker: driver stopped\n");
}


/* define driver entry points */
module_init(boeffla_wl_blocker_init);
module_exit(boeffla_wl_blocker_exit);
