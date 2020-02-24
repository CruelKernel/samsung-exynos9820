/*
 *  gaf_v6.c
 *
 */
#include "proca_gaf.h"

#include <linux/module.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/proca.h>
#include <asm/pgtable.h>
#include <linux/kernel_stat.h>
#include "../fs/mount.h"

static struct GAForensicINFO {
	unsigned short ver;
	unsigned int size;
	unsigned short task_struct_struct_state;
	unsigned short task_struct_struct_comm;
	unsigned short task_struct_struct_tasks;
	unsigned short task_struct_struct_pid;
	unsigned short task_struct_struct_mm;
	unsigned short mm_struct_struct_pgd;
	unsigned short mm_struct_struct_mmap;
	unsigned short mm_struct_struct_mm_rb;
	unsigned short vm_area_struct_struct_vm_start;
	unsigned short vm_area_struct_struct_vm_end;
	unsigned short vm_area_struct_struct_vm_next;
	unsigned short vm_area_struct_struct_vm_flags;
	unsigned short vm_area_struct_struct_vm_file;
	unsigned short vm_area_struct_struct_vm_rb;
	unsigned short file_struct_f_path;
	unsigned short path_struct_mnt;
	unsigned short path_struct_dentry;
	unsigned short dentry_struct_d_parent;
	unsigned short dentry_struct_d_name;
	unsigned short qstr_struct_name;
	unsigned short qstr_struct_len;
	unsigned short struct_mount_mnt_mountpoint;
	unsigned short struct_mount_mnt;
	unsigned short struct_mount_mnt_parent;
	unsigned short list_head_struct_next;
	unsigned short list_head_struct_prev;
	unsigned short is_kdp_ns_on;
	unsigned short task_struct_integrity;
	unsigned short proca_task_descr_task;
	unsigned short proca_task_descr_proca_identity;
	unsigned short proca_task_descr_pid_map_node;
	unsigned short proca_task_descr_app_name_map_node;
	unsigned short proca_identity_struct_certificate;
	unsigned short proca_identity_struct_certificate_size;
	unsigned short proca_identity_struct_parsed_cert;
	unsigned short proca_identity_struct_file;
	unsigned short file_struct_f_signature;
	unsigned short proca_table_hash_tables_shift;
	unsigned short proca_table_pid_map;
	unsigned short proca_table_app_name_map;
	unsigned short proca_certificate_struct_app_name;
	unsigned short proca_certificate_struct_app_name_size;
	unsigned short hlist_node_struct_next;
	unsigned short struct_vfsmount_bp_mount;
	char reserved[1022];
	unsigned short  GAFINFOCheckSum;
} GAFINFO = {
	.ver = 0x0600, /* by hryhorii tur 2019 10 21 */
	.size = sizeof(GAFINFO),
	.task_struct_struct_state = offsetof(struct task_struct, state),
	.task_struct_struct_comm = offsetof(struct task_struct, comm),
	.task_struct_struct_tasks = offsetof(struct task_struct, tasks),
	.task_struct_struct_pid = offsetof(struct task_struct, pid),
	.task_struct_struct_mm = offsetof(struct task_struct, mm),
	.mm_struct_struct_pgd = offsetof(struct mm_struct, pgd),
	.mm_struct_struct_mmap = offsetof(struct mm_struct, mmap),
	.mm_struct_struct_mm_rb = offsetof(struct mm_struct, mm_rb),
	.vm_area_struct_struct_vm_start =
		offsetof(struct vm_area_struct, vm_start),
	.vm_area_struct_struct_vm_end = offsetof(struct vm_area_struct, vm_end),
	.vm_area_struct_struct_vm_next =
		offsetof(struct vm_area_struct, vm_next),
	.vm_area_struct_struct_vm_flags =
		offsetof(struct vm_area_struct, vm_flags),
	.vm_area_struct_struct_vm_file =
		offsetof(struct vm_area_struct, vm_file),
	.vm_area_struct_struct_vm_rb
		= offsetof(struct vm_area_struct, vm_rb),
	.hlist_node_struct_next = offsetof(struct hlist_node, next),
	.file_struct_f_path = offsetof(struct file, f_path),
	.path_struct_mnt = offsetof(struct path, mnt),
	.path_struct_dentry = offsetof(struct path, dentry),
	.dentry_struct_d_parent = offsetof(struct dentry, d_parent),
	.dentry_struct_d_name = offsetof(struct dentry, d_name),
	.qstr_struct_name = offsetof(struct qstr, name),
	.qstr_struct_len = offsetof(struct qstr, len),
	.struct_mount_mnt_mountpoint = offsetof(struct mount, mnt_mountpoint),
	.struct_mount_mnt = offsetof(struct mount, mnt),
	.struct_mount_mnt_parent = offsetof(struct mount, mnt_parent),
	.list_head_struct_next = offsetof(struct list_head, next),
	.list_head_struct_prev = offsetof(struct list_head, prev),
#if defined(CONFIG_KDP_NS) || defined(CONFIG_RKP_NS_PROT)
	.is_kdp_ns_on = true,
	.struct_vfsmount_bp_mount = offsetof(struct vfsmount, bp_mount),
#else
	.is_kdp_ns_on = false,
#endif
#ifdef CONFIG_FIVE
	.task_struct_integrity = offsetof(struct task_struct, integrity),
#else
	.task_struct_integrity = 0xECEF,
#endif
#if defined(CONFIG_FIVE_PA_FEATURE) || defined(CONFIG_PROCA)
	.file_struct_f_signature = offsetof(struct file, f_signature),
#endif
#ifdef CONFIG_PROCA
	.proca_task_descr_task =
		offsetof(struct proca_task_descr, task),
	.proca_task_descr_proca_identity =
		offsetof(struct proca_task_descr, proca_identity),
	.proca_task_descr_pid_map_node =
		offsetof(struct proca_task_descr, pid_map_node),
	.proca_task_descr_app_name_map_node =
		offsetof(struct proca_task_descr, app_name_map_node),
	.proca_identity_struct_certificate =
		offsetof(struct proca_identity, certificate),
	.proca_identity_struct_certificate_size =
		offsetof(struct proca_identity, certificate_size),
	.proca_identity_struct_parsed_cert =
		offsetof(struct proca_identity, parsed_cert),
	.proca_table_hash_tables_shift =
		offsetof(struct proca_table, hash_tables_shift),
	.proca_table_pid_map =
		offsetof(struct proca_table, pid_map),
	.proca_table_app_name_map =
		offsetof(struct proca_table, app_name_map),
	.proca_identity_struct_file =
		offsetof(struct proca_identity, file),
	.proca_certificate_struct_app_name =
		offsetof(struct proca_certificate, app_name),
	.proca_certificate_struct_app_name_size =
		offsetof(struct proca_certificate, app_name_size),
#endif
	.GAFINFOCheckSum = 0
};

const void *proca_gaf_get_addr(void)
{
	return &GAFINFO;
}

static int __init proca_init_gaf(void)
{
	const unsigned short size =
			offsetof(struct GAForensicINFO, GAFINFOCheckSum);
	unsigned char *memory = (unsigned char *)&GAFINFO;
	unsigned short i = 0;
	unsigned short checksum = 0;

	for (i = 0; i < size; i++) {
		if (checksum & 0x8000)
			checksum = ((checksum << 1) | 1) ^ memory[i];
		else
			checksum = (checksum << 1) ^ memory[i];
	}
	GAFINFO.GAFINFOCheckSum = checksum;

	return 0;
}
core_initcall(proca_init_gaf)
