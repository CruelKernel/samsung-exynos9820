/*
 *sec_debeg_kasan.c
 *
 * Copyright (c) 2018 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sec_debug.h>
#include <linux/sec_debug_ksan.h>
#include <linux/sec_ext.h>
#include <linux/sec_class.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/slab.h>


static LIST_HEAD(kasan_list);

void sec_debug_kasan_handler(struct kasandebug *ka)
{
	struct kasandebug *this;
	char *ap, *str=ka->ap;

	ap = strsep(&str, "+");

	list_for_each_entry(this, &kasan_list, entry) {
		if((this->rType == ALLTYPE) && !strncmp(this->ap, "all", 3))
			panic("kasan for all rType\n");

		if(this->rType == ALLTYPE)
			if(!strcmp(this->ap, ap))
				panic("kasan for all type @%s",ap);

		if(this->rType == ka->rType)
		{
			if(!strncmp(this->ap, "all", 3))
				panic("kasan for type :%d",ka->rType);

			if(!strcmp(this->ap, ap))
				panic("kasan for type %d with %s",ka->rType, ka->ap);
		}
	}
}


static ssize_t sec_kasan_test_info_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	pr_info("-----------------------------------\n");
	pr_info(" Report Type               #rType  \n");
	pr_info("-----------------------------------\n");
	pr_info(" out-of-bounds               1     \n");
	pr_info(" slab-of-bounds              2     \n");
	pr_info(" global-of-bounds            3     \n");
	pr_info(" stack-of-bounds             4     \n");
	pr_info(" use-after-free              5     \n");
	pr_info(" use-after-scope             6     \n");
	pr_info("-----------------------------------\n");
	pr_info("\n");
	pr_info("-----------------------------------\n");
	pr_info("ex)\n");
	pr_info(" echo add@#rType@Symbolname \n");
	pr_info("          #rType '100' is for all type\n");
	pr_info("          symbolname 'all' is for all symbol\n");
	pr_info("-----------------------------------\n");
	pr_info(" In case of bugreport below \n");
	pr_info("  => BUG: KASAN: global-out-of-bounds in find_lowest_rq_fluid+0x1e64/0x2558 \n");
	pr_info(" Set the setting as below \n");
	pr_info("  => echo add@3@find_lowest_rq_fluid > /sys/class/sec/sec_debug_kasan/kasan_info\n");
	pr_info("-----------------------------------\n");


	return size;
}

static ssize_t sec_kasan_test_info_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	char * kasanslub;

	kasanslub = kmalloc(4096,GFP_KERNEL);
	kfree(kasanslub);
	*(kasanslub)='C';

	
	return size;
}

static void sec_kasan_show(void)
{
	struct kasandebug *this;
	int cnt=1;

	if(list_empty(&kasan_list)){
		pr_info("%s : sec upsan list empty \n", __func__);
		return ;
	}
	pr_info("----------------------------------------\n");
	pr_info("  idx  rType       acessposition        \n");
	pr_info("----------------------------------------\n");
	list_for_each_entry(this, &kasan_list, entry) {
		pr_info("[%3d] %4d %10s\n",cnt++, this->rType, this->ap);
	}
	pr_info("----------------------------------------\n");
}
static ssize_t sec_kasan_info_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	sec_kasan_show();
	return size;
}

static int sec_kasan_delete_item(int rType, char *accesspoint)
{
	struct kasandebug *this, *q;

	list_for_each_entry_safe(this,q ,&kasan_list ,entry) {
		if(this->rType == rType)
		{
			if(!strcmp(this->ap, accesspoint)){
				list_del(&this->entry);
				kfree(this);
				return 1;
			}
		}
	}

	return 0;
}

static int sec_kasan_list_is_not_duplicated(struct kasandebug *ub)
{
	struct kasandebug *this, *q;

	list_for_each_entry_safe(this,q, &kasan_list, entry) {
		if(this->rType == ub->rType)
		{
			if(!strncmp(this->ap, ub->ap, FILESIZE)){
				return 0;
			}
		}
	}
	return 1;
}

static void sec_kasan_list_add(struct kasandebug *ka)
{
	struct kasandebug *this, *q;

	list_for_each_entry_safe(this,q, &kasan_list, entry) {
		if(this->rType == ka->rType)
		{
			/*
			 * In case of set the all for the rType, 
			 * remove 'all' set and set the new specific filename for the rType.
			 */
			if(!strncmp(this->ap,"all",3)){
				list_del(&this->entry);
				kfree(this);
			}
		}
	}

	list_add(&ka->entry, &kasan_list);
}

/*
 * remove all rType with ap and set rType with all
 */
static void sec_kasan_set_all_for_rType(struct kasandebug* ka)
{
	struct kasandebug *this,*q;

	list_for_each_entry_safe(this,q, &kasan_list, entry) {
		if(this->rType == ka->rType){
			list_del(&this->entry);
			kfree(this);
		}
	}
	list_add(&ka->entry, &kasan_list);
}

static void sec_kasan_delete_list_all(void)
{
	struct kasandebug *this,*q;

	list_for_each_entry_safe(this,q, &kasan_list, entry) {
		list_del(&this->entry);
		kfree(this);
	}
}

static void sec_kasan_set_all(struct kasandebug* ka)
{
	sec_kasan_delete_list_all();
	list_add(&ka->entry, &kasan_list);
}

static ssize_t sec_kasan_info_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	struct kasandebug *new_kasan;
	char *str = (char *)buf;
	char *action, *rTypestr, *ap, *aptmp;
	int rType;

	action = strsep(&str,"@");
	rTypestr = strsep(&str,"@");
	rType = simple_strtol(rTypestr,NULL,10);
	aptmp = strsep(&str,"@");
	ap = strsep(&aptmp,"\n");

	if(!strncmp(action,"add",3)){
		new_kasan = kmalloc(sizeof(struct kasandebug), GFP_KERNEL);
		strncpy(new_kasan->ap, ap, FILESIZE);
		new_kasan->rType = rType;

		if(!strncmp(new_kasan->ap, "all",3) && (rType == ALLTYPE)){
			sec_kasan_set_all(new_kasan);
		} else if(!strncmp(new_kasan->ap, "all",3)){
			sec_kasan_set_all_for_rType(new_kasan);
		} else {
			if(sec_kasan_list_is_not_duplicated(new_kasan)){
				sec_kasan_list_add(new_kasan);
			} else {
				pr_info("duplicated!\n");
				kfree(new_kasan);
			}
		}
	} else if (!strncmp(action,"del",3)) { 
		if( (rType == ALLTYPE ) && (!strncmp(ap,"all",3)) ){ 
			sec_kasan_delete_list_all();
		} else { 
			sec_kasan_delete_item(rType, ap);
		}
	} else {
		pr_info("%s : check your cmd again\n", __func__);
		pr_info("%s : usage : echo add@#rType@accesspoint > kasan_info\n", __func__);
	}

	sec_kasan_show();

	return size;
}

static struct kobj_attribute sec_kasan_info_attr =
	__ATTR(kasan_info, 0644, sec_kasan_info_show, sec_kasan_info_store);


static struct kobj_attribute sec_kasan_test_info_attr =
	__ATTR(kasan_test_info, 0644, sec_kasan_test_info_show, sec_kasan_test_info_store);

static struct attribute *sec_kasan_attributes[] = {
	&sec_kasan_info_attr.attr,
	&sec_kasan_test_info_attr.attr,
	NULL,
};

static struct attribute_group sec_kasan_attr_group = {
	.attrs = sec_kasan_attributes,
};

static int __init sec_debug_kasan_init(void)
{
	int ret = 0;
	struct device *dev;

	dev = sec_device_create(NULL, "sec_debug_kasan");

	if(!dev)
		pr_err("%s : sec device create failed!\n", __func__);

	ret = sysfs_create_group(&dev->kobj, &sec_kasan_attr_group);
	if (ret)
		pr_err("%s : could not create sysfs node\n", __func__);

	return 0;
}

device_initcall(sec_debug_kasan_init);
