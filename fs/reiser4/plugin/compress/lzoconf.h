/* lzoconf.h -- configuration for the LZO real-time data compression library
   adopted for reiser4 compression transform plugin.

   This file is part of the LZO real-time data compression library
   and not included in any proprietary licenses of reiser4.

   Copyright (C) 2002 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2001 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 2000 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1999 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1998 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1997 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996 Markus Franz Xaver Johannes Oberhumer
   All Rights Reserved.

   The LZO library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   The LZO library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with the LZO library; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer
   <markus@oberhumer.com>
   http://www.oberhumer.com/opensource/lzo/
 */

#include <linux/kernel.h>	/* for UINT_MAX, ULONG_MAX - edward */

#ifndef __LZOCONF_H
#define __LZOCONF_H

#define LZO_VERSION             0x1080
#define LZO_VERSION_STRING      "1.08"
#define LZO_VERSION_DATE        "Jul 12 2002"

/* internal Autoconf configuration file - only used when building LZO */

/***********************************************************************
// LZO requires a conforming <limits.h>
************************************************************************/

#define CHAR_BIT  8
#define USHRT_MAX 0xffff

/* workaround a cpp bug under hpux 10.20 */
#define LZO_0xffffffffL         4294967295ul

/***********************************************************************
// architecture defines
************************************************************************/

#if !defined(__LZO_i386)
#  if defined(__i386__) || defined(__386__) || defined(_M_IX86)
#    define __LZO_i386
#  endif
#endif

/* memory checkers */
#if !defined(__LZO_CHECKER)
#  if defined(__BOUNDS_CHECKING_ON)
#    define __LZO_CHECKER
#  elif defined(__CHECKER__)
#    define __LZO_CHECKER
#  elif defined(__INSURE__)
#    define __LZO_CHECKER
#  elif defined(__PURIFY__)
#    define __LZO_CHECKER
#  endif
#endif

/***********************************************************************
// integral and pointer types
************************************************************************/

/* Integral types with 32 bits or more */
#if !defined(LZO_UINT32_MAX)
#  if (UINT_MAX >= LZO_0xffffffffL)
	typedef unsigned int lzo_uint32;
	typedef int lzo_int32;
#    define LZO_UINT32_MAX      UINT_MAX
#    define LZO_INT32_MAX       INT_MAX
#    define LZO_INT32_MIN       INT_MIN
#  elif (ULONG_MAX >= LZO_0xffffffffL)
	typedef unsigned long lzo_uint32;
	typedef long lzo_int32;
#    define LZO_UINT32_MAX      ULONG_MAX
#    define LZO_INT32_MAX       LONG_MAX
#    define LZO_INT32_MIN       LONG_MIN
#  else
#    error "lzo_uint32"
#  endif
#endif

/* lzo_uint is used like size_t */
#if !defined(LZO_UINT_MAX)
#  if (UINT_MAX >= LZO_0xffffffffL)
	typedef unsigned int lzo_uint;
	typedef int lzo_int;
#    define LZO_UINT_MAX        UINT_MAX
#    define LZO_INT_MAX         INT_MAX
#    define LZO_INT_MIN         INT_MIN
#  elif (ULONG_MAX >= LZO_0xffffffffL)
	typedef unsigned long lzo_uint;
	typedef long lzo_int;
#    define LZO_UINT_MAX        ULONG_MAX
#    define LZO_INT_MAX         LONG_MAX
#    define LZO_INT_MIN         LONG_MIN
#  else
#    error "lzo_uint"
#  endif
#endif

	typedef int lzo_bool;

/***********************************************************************
// memory models
************************************************************************/

/* Memory model that allows to access memory at offsets of lzo_uint. */
#if !defined(__LZO_MMODEL)
#  if (LZO_UINT_MAX <= UINT_MAX)
#    define __LZO_MMODEL
#  else
#    error "__LZO_MMODEL"
#  endif
#endif

/* no typedef here because of const-pointer issues */
#define lzo_byte                unsigned char __LZO_MMODEL
#define lzo_bytep               unsigned char __LZO_MMODEL *
#define lzo_charp               char __LZO_MMODEL *
#define lzo_voidp               void __LZO_MMODEL *
#define lzo_shortp              short __LZO_MMODEL *
#define lzo_ushortp             unsigned short __LZO_MMODEL *
#define lzo_uint32p             lzo_uint32 __LZO_MMODEL *
#define lzo_int32p              lzo_int32 __LZO_MMODEL *
#define lzo_uintp               lzo_uint __LZO_MMODEL *
#define lzo_intp                lzo_int __LZO_MMODEL *
#define lzo_voidpp              lzo_voidp __LZO_MMODEL *
#define lzo_bytepp              lzo_bytep __LZO_MMODEL *

#ifndef lzo_sizeof_dict_t
#  define lzo_sizeof_dict_t     sizeof(lzo_bytep)
#endif

typedef int (*lzo_compress_t) (const lzo_byte * src, lzo_uint src_len,
			       lzo_byte * dst, lzo_uintp dst_len,
			       lzo_voidp wrkmem);


/***********************************************************************
// error codes and prototypes
************************************************************************/

/* Error codes for the compression/decompression functions. Negative
 * values are errors, positive values will be used for special but
 * normal events.
 */
#define LZO_E_OK                    0
#define LZO_E_ERROR                 (-1)
#define LZO_E_OUT_OF_MEMORY         (-2)	/* not used right now */
#define LZO_E_NOT_COMPRESSIBLE      (-3)	/* not used right now */
#define LZO_E_INPUT_OVERRUN         (-4)
#define LZO_E_OUTPUT_OVERRUN        (-5)
#define LZO_E_LOOKBEHIND_OVERRUN    (-6)
#define LZO_E_EOF_NOT_FOUND         (-7)
#define LZO_E_INPUT_NOT_CONSUMED    (-8)

/* lzo_init() should be the first function you call.
 * Check the return code !
 *
 * lzo_init() is a macro to allow checking that the library and the
 * compiler's view of various types are consistent.
 */
#define lzo_init() __lzo_init2(LZO_VERSION,(int)sizeof(short),(int)sizeof(int),\
    (int)sizeof(long),(int)sizeof(lzo_uint32),(int)sizeof(lzo_uint),\
    (int)lzo_sizeof_dict_t,(int)sizeof(char *),(int)sizeof(lzo_voidp),\
    (int)sizeof(lzo_compress_t))
	 extern int __lzo_init2(unsigned, int, int, int, int, int, int,
				int, int, int);

/* checksum functions */
extern lzo_uint32 lzo_crc32(lzo_uint32 _c, const lzo_byte * _buf,
			    lzo_uint _len);
/* misc. */
	typedef union {
		lzo_bytep p;
		lzo_uint u;
	} __lzo_pu_u;
	typedef union {
		lzo_bytep p;
		lzo_uint32 u32;
	} __lzo_pu32_u;
	typedef union {
		void *vp;
		lzo_bytep bp;
		lzo_uint32 u32;
		long l;
	} lzo_align_t;

#define LZO_PTR_ALIGN_UP(_ptr,_size) \
    ((_ptr) + (lzo_uint) __lzo_align_gap((const lzo_voidp)(_ptr),(lzo_uint)(_size)))

/* deprecated - only for backward compatibility */
#define LZO_ALIGN(_ptr,_size) LZO_PTR_ALIGN_UP(_ptr,_size)

#endif				/* already included */
