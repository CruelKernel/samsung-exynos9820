#ifndef DM_VERITY_DEBUG_H
#define DM_VERITY_DEBUG_H

#include "dm-verity.h"
/*
 * If not use debug mode 
 * Please Command the Define(SEC_HEX_DEBUG) below
 */
#define SEC_HEX_DEBUG

#ifdef SEC_HEX_DEBUG
extern int verity_handle_err_hex_debug(struct dm_verity *v, enum verity_block_type type,
					     unsigned long long block, struct dm_verity_io *io, struct bvec_iter *iter);
#endif

#endif
