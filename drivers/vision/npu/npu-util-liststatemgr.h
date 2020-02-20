/*
 * Samsung Exynos SoC series NPU driver
 *
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _NPU_UTIL_LISTSTATEMGR_H
#define _NPU_UTIL_LISTSTATEMGR_H

#include <linux/list.h>
#include <linux/kthread.h>

#define LSM_NAME_LEN			64
#define LSM_MAX_ENTRY_NUM		(2<<10)
#define AST_PREFIX_LEN			16
#define AST_PREFIX			"-AST worker"

/* Used for defining reference variable of LSM */
#define LSM_DECLARE_REF(LSM_NAME)	\
struct LSM_NAME##_entry;		\
struct LSM_NAME##_type;			\

/* Used to create a reference to LSM
   struct my_struct {
       LSM_TYPE(my_lsm)*	lsm_ref;
   }
   ...
*/
#define LSM_TYPE(LSM_NAME)	struct LSM_NAME##_type

#include "npu-log.h"
#include "npu-util-autosleepthr.h"

/* Enum definition for State managed in the LSM.
   LSM_LIST_TYPE_INVALID is not valid state - it is here
   to parameter check and boundary value protection */
typedef enum {FREE, REQUESTED, PROCESSING, COMPLETED, STUCKED, LSM_LIST_TYPE_INVALID} lsm_list_type_e;

/* Generalized type definition for comparator function.
   Actual comparator will have template parameter but
   all such a function pointer will be complient with this version */
typedef int (*lsm_comparator_t)(const void *lhs, const void *rhs);

/* State name table, for cosmetic purpose */
extern const char *LSM_STATE_NAMES[];

/* Base definition of entries in the LSM.
   LSM_DECLARE(..) will declare a new LSM_NAME##_entry object
   with same layout of this structure but only difference would
   be the 'data_place' memeber will be replaced with actual
   payload structure. It means that the struct LSM_NAME##_entry
   is always bigger than struct lsm_base_entry but all the memeber
   above data_place will be compliant. */
struct lsm_base_entry {
	struct list_head list;
	lsm_list_type_e state;
	u64 data_place;
};

/* Hook function called when an entry is put into LSM
   second parameter will be exported as a D_TYPE* on instantiation */
typedef void (*lsm_hook_t)(lsm_list_type_e state, void *new_entry);

/* A data structire for LSM management */
struct lsm_internal_data {
	struct auto_sleep_thread ast_worker;	/* Auto sleep thread */
	struct auto_sleep_thread_param ast_param;
	char *name;
	size_t entry_num;
	int is_ast_enabled;		/* True if the LSM incorporates AST */

	lsm_comparator_t compare;	/* Entry compare function (optional) */
	lsm_hook_t	put_hook;	/* A hook function called when an entry is put */
	struct list_head list_for_state[LSM_LIST_TYPE_INVALID];
};

/* Convinient macro function, to iterate elements in specific state */
#define LSM_FOR_EACH_ENTRY_IN(LSM_NAME, STATE, cursor, ...)					\
{\
	struct lsm_base_entry *__iter_pos, *__iter_temp;\
	list_for_each_entry_safe(__iter_pos, __iter_temp, &(LSM_NAME.lsm_data.list_for_state[STATE]), list) {\
		cursor = (typeof(LSM_NAME.type) *)(&__iter_pos->data_place);\
		__VA_ARGS__\
	} \
} \
/*
#define LSM_FOR_EACH_ENTRY_IN(LSM_NAME, pos, n, STATE) \
	list_for_each_entry_safe(pos, n, &(LSM_NAME.lsm_data.list_for_state[STATE]), list)
*/
/* Special BUG_ON macro to ease tracking */
#define LSM_BUG_ON(cond)	do {if (unlikely(cond)) {npu_err("BUG_ON Assert " #cond); BUG_ON(1); } } while(0)

/* Base helper functions in npu-util-liststatemgr.c */
void __lsm_reset_list(struct lsm_internal_data *lsm);
void __lsm_destroy(struct lsm_internal_data *lsm);
struct lsm_base_entry *__lsm_get_entry(struct lsm_internal_data *lsm, lsm_list_type_e type);
int __lsm_put_entry(struct lsm_internal_data *lsm, lsm_list_type_e type, struct lsm_base_entry *new_entry);
int __lsm_move_entry(struct lsm_internal_data *lsm, lsm_list_type_e to, struct lsm_base_entry *entry);
void __lsm_send_signal(struct lsm_internal_data *lsm);
void __lsm_set_hook(struct lsm_internal_data *lsm, lsm_hook_t put_hook);

/* Big definition of LSM instance */
#define LSM_DECLARE(LSM_NAME, D_TYPE, SIZE, PRINT_NAME) \
														\
/* Element in LSM, which has same layout with lsm_base_entry but contains D_TYPE */				\
struct LSM_NAME##_entry {											\
	struct list_head list;											\
	lsm_list_type_e state;											\
	D_TYPE data;												\
};														\
/* List state manager instance */										\
struct LSM_NAME##_type {											\
	/* Information required to maintain LSM */								\
	struct lsm_internal_data lsm_data;									\
														\
	struct LSM_NAME##_entry entry_array[SIZE];								\
	D_TYPE type;												\
														\
	/* function table */											\
	int (*lsm_init)(int (*do_task)(struct auto_sleep_thread_param *data),					\
			 int (*check_work)(struct auto_sleep_thread_param *data),				\
			 int (*compare)(const D_TYPE *lhs, const D_TYPE *rhs));				\
	void (*lsm_destroy)(void);										\
	D_TYPE* (*lsm_get_entry)(lsm_list_type_e type);								\
	int (*lsm_put_entry)(lsm_list_type_e type, D_TYPE *entry);						\
	int (*lsm_move_entry)(lsm_list_type_e to, D_TYPE *entry);						\
	void (*lsm_send_signal)(void);										\
	void (*lsm_set_hook)(void (*put_hook)(lsm_list_type_e state, D_TYPE *new_entry));				\
};														\
														\
/* Forward declaration */											\
static void LSM_NAME##__lsm_destroy(void);									\
static D_TYPE *LSM_NAME##__lsm_get_entry(lsm_list_type_e type);							\
static int LSM_NAME##__lsm_put_entry(lsm_list_type_e type, D_TYPE *entry);					\
static int LSM_NAME##__lsm_move_entry(lsm_list_type_e to, D_TYPE *entry);					\
static void LSM_NAME##__lsm_send_signal(void);									\
static void LSM_NAME##__lsm_set_hook(void (*put_hook)(lsm_list_type_e state, D_TYPE *new_entry));			\
static int LSM_NAME##__lsm_init(										\
	int (*do_task)(struct auto_sleep_thread_param *data),							\
	int (*check_work)(struct auto_sleep_thread_param *data),						\
	int (*compare)(const D_TYPE *lhs, const D_TYPE *rhs)) ;						\
														\
/* Initialize basic parameters from parameter in declaration macro */						\
struct LSM_NAME##_type LSM_NAME = {										\
	.lsm_data.name = PRINT_NAME,										\
	.lsm_data.entry_num = SIZE,										\
	.lsm_destroy = LSM_NAME##__lsm_destroy,									\
	.lsm_init = LSM_NAME##__lsm_init,									\
	.lsm_get_entry = LSM_NAME##__lsm_get_entry,								\
	.lsm_put_entry = LSM_NAME##__lsm_put_entry,								\
	.lsm_move_entry = LSM_NAME##__lsm_move_entry,								\
	.lsm_send_signal = LSM_NAME##__lsm_send_signal,								\
	.lsm_set_hook = LSM_NAME##__lsm_set_hook,								\
};														\
														\
														\
static void LSM_NAME##__lsm_destroy(void) \
{	\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
														\
	__lsm_destroy(&this->lsm_data);										\
	return;													\
}														\
														\
static D_TYPE *LSM_NAME##__lsm_get_entry(lsm_list_type_e type) \
{	\
	/* The list for specified type */									\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
	struct LSM_NAME##_entry *ret_entry;									\
														\
	/* __lsm_get_entry returns 'struct lsm_base_entry' but	*/						\
	/* it is a element of LSM_NAME##_type.entry_array,	*/						\
	/* so it can be casted to 'struct LSM_NAME##_entry'	*/						\
	ret_entry = (struct LSM_NAME##_entry *)__lsm_get_entry(&this->lsm_data, type);				\
	if (ret_entry != NULL)	\
		return &(ret_entry->data);							\
	else\
		return NULL;											\
}														\
														\
static int LSM_NAME##__lsm_put_entry(lsm_list_type_e type, D_TYPE *entry) \
{	\
	/* The list for specified type */									\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
	struct LSM_NAME##_entry *container;									\
														\
	LSM_BUG_ON(!entry);											\
	/* Entry should be one of the entry_array */								\
	LSM_BUG_ON((void *)entry < (void *)this->entry_array);							\
	LSM_BUG_ON((void *)entry >= (void *)(this->entry_array + SIZE));					\
														\
	container = container_of(entry, struct LSM_NAME##_entry, data);						\
	return __lsm_put_entry(&this->lsm_data, type,								\
			       (struct lsm_base_entry *)container);						\
}														\
														\
static int LSM_NAME##__lsm_move_entry(lsm_list_type_e to, D_TYPE *entry) \
{	\
	/* The list for specified type */									\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
	struct LSM_NAME##_entry *container;									\
														\
	LSM_BUG_ON(!entry);											\
	container = container_of(entry, struct LSM_NAME##_entry, data);						\
														\
	return __lsm_move_entry(&this->lsm_data, to,								\
				(struct lsm_base_entry *)container);						\
}														\
														\
static void LSM_NAME##__lsm_send_signal(void) \
{	\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
	__lsm_send_signal(&this->lsm_data);									\
}														\
														\
static void LSM_NAME##__lsm_set_hook(void (*put_hook)(lsm_list_type_e state, D_TYPE *new_entry)) \
{	\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
	__lsm_set_hook(&this->lsm_data, (lsm_hook_t)put_hook);							\
	return;													\
}														\
														\
static int LSM_NAME##__lsm_init(										\
	int (*do_task)(struct auto_sleep_thread_param *data),							\
	int (*check_work)(struct auto_sleep_thread_param *data),						\
	int (*compare)(const D_TYPE *lhs, const D_TYPE *rhs)) {						\
														\
	int ret = 0;												\
	size_t iz;												\
	typeof(LSM_NAME) *this = &LSM_NAME;									\
	char ast_name[LSM_NAME_LEN + AST_PREFIX_LEN];								\
														\
	npu_info("start in Init LSM(%s)\n", this->lsm_data.name);						\
														\
	/* Setting comparator */										\
	/* compare -> Castring is required to generalize its parameger */					\
	/* D_TYPE* -> void* */											\
	this->lsm_data.compare = (lsm_comparator_t)compare;							\
														\
	/* Initialize entry lists */										\
	__lsm_reset_list(&this->lsm_data);									\
														\
	/* Put all the entry to first list */									\
	for (iz = 0; iz < this->lsm_data.entry_num; iz++) {							\
		this->entry_array[iz].state = 0;								\
		list_add_tail(&(this->entry_array[iz].list), &(this->lsm_data.list_for_state[0]));		\
	}													\
														\
	/* Determine AST shall be included or not */								\
	if (do_task) {												\
		npu_info("LSM(%s) - with AST mode.\n", this->lsm_data.name);					\
		this->lsm_data.is_ast_enabled = 1;									\
														\
		/* Sanity check */										\
		LSM_BUG_ON(!do_task);									\
		LSM_BUG_ON(!check_work);									\
														\
		/* Create Auto sleep thread worker name */							\
		snprintf(ast_name, sizeof(ast_name), "%s" AST_PREFIX, this->lsm_data.name);			\
														\
		/* Initialize Auto sleep thread */								\
		ret = auto_sleep_thread_create(									\
			&(this->lsm_data.ast_worker),ast_name, do_task, check_work, NULL);			\
		if (ret) {											\
			npu_err("fail(%d) in auto_sleep_thread_create LSM(%s)\n",			\
				ret, this->lsm_data.name);							\
			ret = -EFAULT;										\
			goto err_exit;										\
		}												\
														\
		/* Execute Auto sleep thread */									\
		auto_sleep_thread_start(&(this->lsm_data.ast_worker), this->lsm_data.ast_param);		\
		if (ret) {											\
			npu_err("fail(%d) in auto_sleep_thread_start LSM(%s)]\n",			\
				ret, this->lsm_data.name);							\
			ret = -EFAULT;										\
			goto err_exit;										\
		}												\
	} else {												\
		npu_info("LSM(%s) - without AST mode.\n", this->lsm_data.name);				\
		this->lsm_data.is_ast_enabled = 0;									\
	}													\
														\
	/* Success */												\
	npu_info("complete in init LSM(%s)\n", this->lsm_data.name);						\
	goto ok_exit;												\
														\
err_exit:													\
	npu_err("init fail(%d) on LSM(%s)\n", ret, this->lsm_data.name);				\
ok_exit:													\
	return ret;												\
}														\
														\


#endif	/* _NPU_UTIL_LISTSTATEMGR_H */
