/*
 *sec_debeg_ubsan.c
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


static LIST_HEAD(ubsan_list);

void sec_debug_ubsan_handler(struct ubsandebug *ub)
{
	struct ubsandebug *this;

	list_for_each_entry(this, &ubsan_list, entry) {
		if((this->rType == ALLTYPE) && !strncmp(this->filename, "all", 3))
			panic("ubsan for all rType\n");

		if(this->rType == ALLTYPE){
			if(!strcmp(this->filename, ub->filename))
				panic("ubsan for all type @%s",ub->filename);
		}

		if(this->rType == ub->rType)
		{
			if(!strncmp(this->filename, "all", 3))
				panic("ubsan for type :%d",ub->rType);
			
			if(!strcmp(this->filename, ub->filename))
				panic("ubsan for type %d with %s",ub->rType, ub->filename);
		}
	}
}

static ssize_t sec_ubsan_test_info_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	pr_info("------------------------------------------------------------------------\n");
	pr_info(" Report  Type                                                   #rType  \n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info(" handle_overflow  [ .. integer overflow]                           1\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_negate_overflow \n"); 
	pr_info("[negation of .. cannot be represented in type]                     2\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_divrem_overflow [division by zero]                  3\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("handle_null_ptr_deref [.. null pointer of type]                    4\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("handle_missaligned_access [.. misaligned address .. for type]      5\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("handle_object_size_mismatch \n");
	pr_info("[.. address .. with insufficient space]                            6\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_nonnull_return \n");
	pr_info("[null pointer returned from function .. to never return null]      7\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_vla_bound_not_positive\n");
	pr_info("[variable length array bound value .. <= 0]                        8\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_out_of_bounds\n");
	pr_info("[index .. is out of range for type ..]                             9\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_shift_out_of_bounds\n");
	pr_info("[shift exponent ..] ([left shift of~]                             10\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_load_invalid_value\n");
	pr_info("[load of value .. is not a valid value for type]                  11\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("__ubsan_handle_builtin_unreachable [panic]               not assigned\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info("ex)\n");
	pr_info(" echo add@#rType@symbolname \n");
	pr_info("          #rType '100' is for all type\n");
	pr_info("          symbolname 'all' is for all symbol\n");
	pr_info("------------------------------------------------------------------------\n");
	pr_info(" In case of bugreport below \n");
	pr_info("  => UBSAN: Undefined behaviour in kernel/sched/tune.c:673:47 \n");
	pr_info("     division by zero\n");
	pr_info(" Set the setting as below \n");
	pr_info("  => echo add@3@kernel/sched/tune.c > /sys/class/sec/sec_debug_ubsan/ubsan_info\n");
	pr_info("------------------------------------------------------------------------\n");


	return size;
}

static ssize_t sec_ubsan_test_info_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	int itest[10];

	itest[11] = 99;
	
	return size;
}

static void sec_ubsan_show(void)
{
	struct ubsandebug *this;
	int cnt=1;

	if(list_empty(&ubsan_list)){
		pr_info("%s : sec upsan list empty \n", __func__);
		return ;
	}
	pr_info("----------------------------------------\n");
	pr_info("  idx  rType       filename             \n");
	pr_info("----------------------------------------\n");
	list_for_each_entry(this, &ubsan_list, entry) {
		pr_info("[%3d] %4d %8s\n",cnt++, this->rType, this->filename);
	}
	pr_info("----------------------------------------\n");
}
static ssize_t sec_ubsan_info_show(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	sec_ubsan_show();
	return size;
}

static int sec_ubsan_delete_item(int rType, char *filename)
{
	struct ubsandebug *this, *q;

	list_for_each_entry_safe(this,q ,&ubsan_list ,entry) {
		if(this->rType == rType)
		{
			if(!strcmp(this->filename, filename)){
				list_del(&this->entry);
				kfree(this);
				return 1;
			}
		}
	}

	return 0;
}

static int sec_ubsan_list_is_not_duplicated(struct ubsandebug *ub)
{
	struct ubsandebug *this, *q;

	list_for_each_entry_safe(this,q, &ubsan_list, entry) {
		if(this->rType == ub->rType)
		{
			if(!strncmp(this->filename, ub->filename, FILESIZE)){
				return 0;
			}
		}
	}
	return 1;
}

static void sec_ubsan_list_add(struct ubsandebug *ub)
{
	struct ubsandebug *this, *q;

	list_for_each_entry_safe(this,q, &ubsan_list, entry) {
		if(this->rType == ub->rType)
		{
			/*
			 * In case of set the all for the rType, 
			 * remove 'all' set and set the new specific filename for the rType.
			 */
			if(!strncmp(this->filename,"all",3)){
				list_del(&this->entry);
				kfree(this);
			}
		}
	}

	list_add(&ub->entry, &ubsan_list);
}
	

/*
 * remove all rType with filename and set rType with all
 */
static void sec_ubsan_set_all_for_rType(struct ubsandebug* ub)
{
	struct ubsandebug *this,*q;

	list_for_each_entry_safe(this,q, &ubsan_list, entry) {
		if(this->rType == ub->rType){
			list_del(&this->entry);
			kfree(this);
		}
	}
	list_add(&ub->entry, &ubsan_list);
}

static void sec_ubsan_delete_list_all(void)
{
	struct ubsandebug *this,*q;

	list_for_each_entry_safe(this,q, &ubsan_list, entry) {
		list_del(&this->entry);
		kfree(this);
	}
}

static void sec_ubsan_set_all(struct ubsandebug* ub)
{
	sec_ubsan_delete_list_all();
	list_add(&ub->entry, &ubsan_list);
}

static ssize_t sec_ubsan_info_store(struct kobject *kobj,
					   struct kobj_attribute *attr,
					   const char *buf, size_t size)
{
	struct ubsandebug *new_ubsan;
	char *str = (char *)buf;
	char *action = NULL, *rTypestr = NULL, *filename = NULL;
	int rType;

	action = strsep(&str,"@");
	rTypestr = strsep(&str,"@");
	rType = simple_strtol(rTypestr,NULL,10);
	filename = strsep(&str,"@");

	if(!strncmp(action,"add",3)){
		new_ubsan = kmalloc(sizeof(struct ubsandebug), GFP_KERNEL);
		strncpy(new_ubsan->filename, filename, FILESIZE);
		new_ubsan->rType = rType;

		if(!strncmp(new_ubsan->filename, "all",3) && (rType == ALLTYPE)){
			sec_ubsan_set_all(new_ubsan);
		} else if(!strncmp(new_ubsan->filename, "all",3)){
			sec_ubsan_set_all_for_rType(new_ubsan);
		} else {
			if(sec_ubsan_list_is_not_duplicated(new_ubsan)){
				sec_ubsan_list_add(new_ubsan);
			} else {
				pr_info("duplicated!\n");
				kfree(new_ubsan);
			}
		}
	} else if (!strncmp(action,"del",3)) { 
		if( (rType == ALLTYPE ) && (!strncmp(filename,"all",3)) ){ 
			sec_ubsan_delete_list_all();
		} else { 
			sec_ubsan_delete_item(rType, filename);
		}
	} else {
		pr_info("%s : check your cmd again\n", __func__);
		pr_info("%s : usage : echo add@#rType@filename > ubsan_info\n", __func__);
	}

	sec_ubsan_show();

	return size;
}


static struct kobj_attribute sec_ubsan_info_attr =
	__ATTR(ubsan_info, 0644, sec_ubsan_info_show, sec_ubsan_info_store);

static struct kobj_attribute sec_ubsan_test_info_attr =
	__ATTR(ubsan_test_info, 0644, sec_ubsan_test_info_show, sec_ubsan_test_info_store);


static struct attribute *sec_ubsan_attributes[] = {
	&sec_ubsan_info_attr.attr,
	&sec_ubsan_test_info_attr.attr,
	NULL,
};

static struct attribute_group sec_ubsan_attr_group = {
	.attrs = sec_ubsan_attributes,
};

static int __init sec_debug_ubsan_init(void)
{
	int ret = 0;
	struct device *dev;

	dev = sec_device_create(NULL, "sec_debug_ubsan");
	if(!dev)
		pr_err("%s : sec device create failed!\n", __func__);

	ret = sysfs_create_group(&dev->kobj, &sec_ubsan_attr_group);
	if (ret)
		pr_err("%s : could not create sysfs node\n", __func__);

	return 0;
}

device_initcall(sec_debug_ubsan_init);
