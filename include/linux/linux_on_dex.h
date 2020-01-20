#ifndef _LINUX_ON_DEX_H
#define _LINUX_ON_DEX_H

#include <uapi/linux/capability.h>
#include <linux/cred.h>
#include <linux/android_aid.h> // load AID_INET

#define LOD_UID_PREFIX   0x61A8 //1638400000
#define CAP_LOD_SET      ((kernel_cap_t){{ CAP_TO_MASK(CAP_CHOWN) \
				    | CAP_TO_MASK(CAP_DAC_OVERRIDE) \
				    | CAP_TO_MASK(CAP_FOWNER) \
				    | CAP_TO_MASK(CAP_KILL) \
				    | CAP_TO_MASK(CAP_SETGID) \
				    | CAP_TO_MASK(CAP_SETUID), \
				    0 } })

#define LOD_INVALID_ID	((u32)-1)

#define current_is_LOD()	(((current_cred()->uid.val) >> 16) == LOD_UID_PREFIX)
#define cred_is_LOD(cred)	((((cred)->uid.val) >> 16) == LOD_UID_PREFIX)
#define inode_is_LOD(inode)	((((inode)->i_uid.val) >> 16) == LOD_UID_PREFIX)

//print out argument name
#define tostr__(x)      # x
#define tostr_(x)       tostr__(x)

#define uid_is_LOD(arg)    __uid_is_LOD((arg), tostr_(arg))
#define gid_is_LOD(arg)    __gid_is_LOD((arg), tostr_(arg))

static inline bool __uid_is_LOD(u32 id, char *id_name){

	if ((id >> 16) == LOD_UID_PREFIX)
		return true;

	// Invalid UID is safe
	if (id == LOD_INVALID_ID)
		return true;

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	printk(KERN_ERR "LOD: %s: PROC %s PID %d CURR_UID %d %s %d\n", __FUNCTION__,
		current->comm, current->pid, current_cred()->uid.val, id_name, id);
#endif
	return false;
}

static inline bool __gid_is_LOD(u32 id, char *id_name){

	if ((id >> 16) == LOD_UID_PREFIX)
		return true;

	// Invalid UID is safe
	if (id == LOD_INVALID_ID)
		return true;

	// INET gid is allowed for LOD
	if (id == AID_INET.val)
		return true;

#ifndef CONFIG_SAMSUNG_PRODUCT_SHIP
	printk(KERN_ERR "LOD: %s: PROC %s PID %d CURR_UID %d %s %d\n", __FUNCTION__,
		current->comm, current->pid, current_cred()->uid.val, id_name, id);
#endif
	return false;
}

#endif /* !_LINUX_ON_DEX_H */
