/* minilzo.c -- mini subset of the LZO real-time data compression library
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

/*
 * NOTE:
 *   the full LZO package can be found at
 *   http://www.oberhumer.com/opensource/lzo/
 */

#include "../../debug.h"	/* for reiser4 assert macro -edward */

#define __LZO_IN_MINILZO
#define LZO_BUILD

#include "minilzo.h"

#if !defined(MINILZO_VERSION) || (MINILZO_VERSION != 0x1080)
#  error "version mismatch in miniLZO source files"
#endif

#ifndef __LZO_CONF_H
#define __LZO_CONF_H

#  define BOUNDS_CHECKING_OFF_DURING(stmt)      stmt
#  define BOUNDS_CHECKING_OFF_IN_EXPR(expr)     (expr)

#  define HAVE_MEMCMP
#  define HAVE_MEMCPY
#  define HAVE_MEMMOVE
#  define HAVE_MEMSET

#undef NDEBUG
#if !defined(LZO_DEBUG)
#  define NDEBUG
#endif
#if defined(LZO_DEBUG) || !defined(NDEBUG)
#  if !defined(NO_STDIO_H)
#    include <stdio.h>
#  endif
#endif

#if !defined(LZO_COMPILE_TIME_ASSERT)
#  define LZO_COMPILE_TIME_ASSERT(expr) \
	{ typedef int __lzo_compile_time_assert_fail[1 - 2 * !(expr)]; }
#endif

#if !defined(LZO_UNUSED)
#  if 1
#    define LZO_UNUSED(var)     ((void)&var)
#  elif 0
#    define LZO_UNUSED(var)     { typedef int __lzo_unused[sizeof(var) ? 2 : 1]; }
#  else
#    define LZO_UNUSED(parm)    (parm = parm)
#  endif
#endif

#if defined(NO_MEMCMP)
#  undef HAVE_MEMCMP
#endif

#if !defined(HAVE_MEMSET)
#  undef memset
#  define memset    lzo_memset
#endif

#  define LZO_BYTE(x)       ((unsigned char) ((x) & 0xff))

#define LZO_MAX(a,b)        ((a) >= (b) ? (a) : (b))
#define LZO_MIN(a,b)        ((a) <= (b) ? (a) : (b))
#define LZO_MAX3(a,b,c)     ((a) >= (b) ? LZO_MAX(a,c) : LZO_MAX(b,c))
#define LZO_MIN3(a,b,c)     ((a) <= (b) ? LZO_MIN(a,c) : LZO_MIN(b,c))

#define lzo_sizeof(type)    ((lzo_uint) (sizeof(type)))

#define LZO_HIGH(array)     ((lzo_uint) (sizeof(array)/sizeof(*(array))))

#define LZO_SIZE(bits)      (1u << (bits))
#define LZO_MASK(bits)      (LZO_SIZE(bits) - 1)

#define LZO_LSIZE(bits)     (1ul << (bits))
#define LZO_LMASK(bits)     (LZO_LSIZE(bits) - 1)

#define LZO_USIZE(bits)     ((lzo_uint) 1 << (bits))
#define LZO_UMASK(bits)     (LZO_USIZE(bits) - 1)

#define LZO_STYPE_MAX(b)    (((1l  << (8*(b)-2)) - 1l)  + (1l  << (8*(b)-2)))
#define LZO_UTYPE_MAX(b)    (((1ul << (8*(b)-1)) - 1ul) + (1ul << (8*(b)-1)))

#if !defined(SIZEOF_UNSIGNED)
#  if (UINT_MAX == 0xffff)
#    define SIZEOF_UNSIGNED         2
#  elif (UINT_MAX == LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED         4
#  elif (UINT_MAX >= LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED         8
#  else
#    error "SIZEOF_UNSIGNED"
#  endif
#endif

#if !defined(SIZEOF_UNSIGNED_LONG)
#  if (ULONG_MAX == LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG    4
#  elif (ULONG_MAX >= LZO_0xffffffffL)
#    define SIZEOF_UNSIGNED_LONG    8
#  else
#    error "SIZEOF_UNSIGNED_LONG"
#  endif
#endif

#if !defined(SIZEOF_SIZE_T)
#  define SIZEOF_SIZE_T             SIZEOF_UNSIGNED
#endif
#if !defined(SIZE_T_MAX)
#  define SIZE_T_MAX                LZO_UTYPE_MAX(SIZEOF_SIZE_T)
#endif

#if 1 && defined(__LZO_i386) && (UINT_MAX == LZO_0xffffffffL)
#  if !defined(LZO_UNALIGNED_OK_2) && (USHRT_MAX == 0xffff)
#    define LZO_UNALIGNED_OK_2
#  endif
#  if !defined(LZO_UNALIGNED_OK_4) && (LZO_UINT32_MAX == LZO_0xffffffffL)
#    define LZO_UNALIGNED_OK_4
#  endif
#endif

#if defined(LZO_UNALIGNED_OK_2) || defined(LZO_UNALIGNED_OK_4)
#  if !defined(LZO_UNALIGNED_OK)
#    define LZO_UNALIGNED_OK
#  endif
#endif

#if defined(__LZO_NO_UNALIGNED)
#  undef LZO_UNALIGNED_OK
#  undef LZO_UNALIGNED_OK_2
#  undef LZO_UNALIGNED_OK_4
#endif

#if defined(LZO_UNALIGNED_OK_2) && (USHRT_MAX != 0xffff)
#  error "LZO_UNALIGNED_OK_2 must not be defined on this system"
#endif
#if defined(LZO_UNALIGNED_OK_4) && (LZO_UINT32_MAX != LZO_0xffffffffL)
#  error "LZO_UNALIGNED_OK_4 must not be defined on this system"
#endif

#if defined(__LZO_NO_ALIGNED)
#  undef LZO_ALIGNED_OK_4
#endif

#if defined(LZO_ALIGNED_OK_4) && (LZO_UINT32_MAX != LZO_0xffffffffL)
#  error "LZO_ALIGNED_OK_4 must not be defined on this system"
#endif

#define LZO_LITTLE_ENDIAN       1234
#define LZO_BIG_ENDIAN          4321
#define LZO_PDP_ENDIAN          3412

#if !defined(LZO_BYTE_ORDER)
#  if defined(MFX_BYTE_ORDER)
#    define LZO_BYTE_ORDER      MFX_BYTE_ORDER
#  elif defined(__LZO_i386)
#    define LZO_BYTE_ORDER      LZO_LITTLE_ENDIAN
#  elif defined(BYTE_ORDER)
#    define LZO_BYTE_ORDER      BYTE_ORDER
#  elif defined(__BYTE_ORDER)
#    define LZO_BYTE_ORDER      __BYTE_ORDER
#  endif
#endif

#if defined(LZO_BYTE_ORDER)
#  if (LZO_BYTE_ORDER != LZO_LITTLE_ENDIAN) && \
      (LZO_BYTE_ORDER != LZO_BIG_ENDIAN)
#    error "invalid LZO_BYTE_ORDER"
#  endif
#endif

#if defined(LZO_UNALIGNED_OK) && !defined(LZO_BYTE_ORDER)
#  error "LZO_BYTE_ORDER is not defined"
#endif

#define LZO_OPTIMIZE_GNUC_i386_IS_BUGGY

#if defined(NDEBUG) && !defined(LZO_DEBUG) && !defined(__LZO_CHECKER)
#  if defined(__GNUC__) && defined(__i386__)
#    if !defined(LZO_OPTIMIZE_GNUC_i386_IS_BUGGY)
#      define LZO_OPTIMIZE_GNUC_i386
#    endif
#  endif
#endif

extern const lzo_uint32 _lzo_crc32_table[256];

#define _LZO_STRINGIZE(x)           #x
#define _LZO_MEXPAND(x)             _LZO_STRINGIZE(x)

#define _LZO_CONCAT2(a,b)           a ## b
#define _LZO_CONCAT3(a,b,c)         a ## b ## c
#define _LZO_CONCAT4(a,b,c,d)       a ## b ## c ## d
#define _LZO_CONCAT5(a,b,c,d,e)     a ## b ## c ## d ## e

#define _LZO_ECONCAT2(a,b)          _LZO_CONCAT2(a,b)
#define _LZO_ECONCAT3(a,b,c)        _LZO_CONCAT3(a,b,c)
#define _LZO_ECONCAT4(a,b,c,d)      _LZO_CONCAT4(a,b,c,d)
#define _LZO_ECONCAT5(a,b,c,d,e)    _LZO_CONCAT5(a,b,c,d,e)

#ifndef __LZO_PTR_H
#define __LZO_PTR_H

#if !defined(lzo_ptrdiff_t)
#  if (UINT_MAX >= LZO_0xffffffffL)
typedef ptrdiff_t lzo_ptrdiff_t;
#  else
typedef long lzo_ptrdiff_t;
#  endif
#endif

#if !defined(__LZO_HAVE_PTR_T)
#  if defined(lzo_ptr_t)
#    define __LZO_HAVE_PTR_T
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED_LONG)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED_LONG)
typedef unsigned long lzo_ptr_t;
typedef long lzo_sptr_t;
#      define __LZO_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED)
typedef unsigned int lzo_ptr_t;
typedef int lzo_sptr_t;
#      define __LZO_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(SIZEOF_CHAR_P) && defined(SIZEOF_UNSIGNED_SHORT)
#    if (SIZEOF_CHAR_P == SIZEOF_UNSIGNED_SHORT)
typedef unsigned short lzo_ptr_t;
typedef short lzo_sptr_t;
#      define __LZO_HAVE_PTR_T
#    endif
#  endif
#endif
#if !defined(__LZO_HAVE_PTR_T)
#  if defined(LZO_HAVE_CONFIG_H) || defined(SIZEOF_CHAR_P)
#    error "no suitable type for lzo_ptr_t"
#  else
typedef unsigned long lzo_ptr_t;
typedef long lzo_sptr_t;
#    define __LZO_HAVE_PTR_T
#  endif
#endif

#define PTR(a)              ((lzo_ptr_t) (a))
#define PTR_LINEAR(a)       PTR(a)
#define PTR_ALIGNED_4(a)    ((PTR_LINEAR(a) & 3) == 0)
#define PTR_ALIGNED_8(a)    ((PTR_LINEAR(a) & 7) == 0)
#define PTR_ALIGNED2_4(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 3) == 0)
#define PTR_ALIGNED2_8(a,b) (((PTR_LINEAR(a) | PTR_LINEAR(b)) & 7) == 0)

#define PTR_LT(a,b)         (PTR(a) < PTR(b))
#define PTR_GE(a,b)         (PTR(a) >= PTR(b))
#define PTR_DIFF(a,b)       ((lzo_ptrdiff_t) (PTR(a) - PTR(b)))
#define pd(a,b)             ((lzo_uint) ((a)-(b)))

typedef union {
	char a_char;
	unsigned char a_uchar;
	short a_short;
	unsigned short a_ushort;
	int a_int;
	unsigned int a_uint;
	long a_long;
	unsigned long a_ulong;
	lzo_int a_lzo_int;
	lzo_uint a_lzo_uint;
	lzo_int32 a_lzo_int32;
	lzo_uint32 a_lzo_uint32;
	ptrdiff_t a_ptrdiff_t;
	lzo_ptrdiff_t a_lzo_ptrdiff_t;
	lzo_ptr_t a_lzo_ptr_t;
	lzo_voidp a_lzo_voidp;
	void *a_void_p;
	lzo_bytep a_lzo_bytep;
	lzo_bytepp a_lzo_bytepp;
	lzo_uintp a_lzo_uintp;
	lzo_uint *a_lzo_uint_p;
	lzo_uint32p a_lzo_uint32p;
	lzo_uint32 *a_lzo_uint32_p;
	unsigned char *a_uchar_p;
	char *a_char_p;
} lzo_full_align_t;

#endif
#define LZO_DETERMINISTIC
#define LZO_DICT_USE_PTR
#  define lzo_dict_t    const lzo_bytep
#  define lzo_dict_p    lzo_dict_t __LZO_MMODEL *
#if !defined(lzo_moff_t)
#define lzo_moff_t      lzo_uint
#endif
#endif
static lzo_ptr_t __lzo_ptr_linear(const lzo_voidp ptr)
{
	return PTR_LINEAR(ptr);
}

static unsigned __lzo_align_gap(const lzo_voidp ptr, lzo_uint size)
{
	lzo_ptr_t p, s, n;

	assert("lzo-01", size > 0);

	p = __lzo_ptr_linear(ptr);
	s = (lzo_ptr_t) (size - 1);
	n = (((p + s) / size) * size) - p;

	assert("lzo-02", (long)n >= 0);
	assert("lzo-03", n <= s);

	return (unsigned)n;
}

#ifndef __LZO_UTIL_H
#define __LZO_UTIL_H

#ifndef __LZO_CONF_H
#endif

#if 1 && defined(HAVE_MEMCPY)
#define MEMCPY8_DS(dest,src,len) \
    memcpy(dest,src,len); \
    dest += len; \
    src += len
#endif

#if !defined(MEMCPY8_DS)

#define MEMCPY8_DS(dest,src,len) \
    { register lzo_uint __l = (len) / 8; \
    do { \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
	*dest++ = *src++; \
    } while (--__l > 0); }

#endif

#define MEMCPY_DS(dest,src,len) \
    do *dest++ = *src++; \
    while (--len > 0)

#define MEMMOVE_DS(dest,src,len) \
    do *dest++ = *src++; \
    while (--len > 0)

#if (LZO_UINT_MAX <= SIZE_T_MAX) && defined(HAVE_MEMSET)

#define BZERO8_PTR(s,l,n)   memset((s),0,(lzo_uint)(l)*(n))

#else

#define BZERO8_PTR(s,l,n) \
    lzo_memset((lzo_voidp)(s),0,(lzo_uint)(l)*(n))

#endif
#endif

/* If you use the LZO library in a product, you *must* keep this
 * copyright string in the executable of your product.
 */

static const lzo_byte __lzo_copyright[] =
#if !defined(__LZO_IN_MINLZO)
    LZO_VERSION_STRING;
#else
    "\n\n\n"
    "LZO real-time data compression library.\n"
    "Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002 Markus Franz Xaver Johannes Oberhumer\n"
    "<markus.oberhumer@jk.uni-linz.ac.at>\n"
    "http://www.oberhumer.com/opensource/lzo/\n"
    "\n"
    "LZO version: v" LZO_VERSION_STRING ", " LZO_VERSION_DATE "\n"
    "LZO build date: " __DATE__ " " __TIME__ "\n\n"
    "LZO special compilation options:\n"
#ifdef __cplusplus
    " __cplusplus\n"
#endif
#if defined(__PIC__)
    " __PIC__\n"
#elif defined(__pic__)
    " __pic__\n"
#endif
#if (UINT_MAX < LZO_0xffffffffL)
    " 16BIT\n"
#endif
#if defined(__LZO_STRICT_16BIT)
    " __LZO_STRICT_16BIT\n"
#endif
#if (UINT_MAX > LZO_0xffffffffL)
    " UINT_MAX=" _LZO_MEXPAND(UINT_MAX) "\n"
#endif
#if (ULONG_MAX > LZO_0xffffffffL)
    " ULONG_MAX=" _LZO_MEXPAND(ULONG_MAX) "\n"
#endif
#if defined(LZO_BYTE_ORDER)
    " LZO_BYTE_ORDER=" _LZO_MEXPAND(LZO_BYTE_ORDER) "\n"
#endif
#if defined(LZO_UNALIGNED_OK_2)
    " LZO_UNALIGNED_OK_2\n"
#endif
#if defined(LZO_UNALIGNED_OK_4)
    " LZO_UNALIGNED_OK_4\n"
#endif
#if defined(LZO_ALIGNED_OK_4)
    " LZO_ALIGNED_OK_4\n"
#endif
#if defined(LZO_DICT_USE_PTR)
    " LZO_DICT_USE_PTR\n"
#endif
#if defined(__LZO_QUERY_COMPRESS)
    " __LZO_QUERY_COMPRESS\n"
#endif
#if defined(__LZO_QUERY_DECOMPRESS)
    " __LZO_QUERY_DECOMPRESS\n"
#endif
#if defined(__LZO_IN_MINILZO)
    " __LZO_IN_MINILZO\n"
#endif
    "\n\n" "$Id: LZO " LZO_VERSION_STRING " built " __DATE__ " " __TIME__
#if defined(__GNUC__) && defined(__VERSION__)
    " by gcc " __VERSION__
#elif defined(__BORLANDC__)
    " by Borland C " _LZO_MEXPAND(__BORLANDC__)
#elif defined(_MSC_VER)
    " by Microsoft C " _LZO_MEXPAND(_MSC_VER)
#elif defined(__PUREC__)
    " by Pure C " _LZO_MEXPAND(__PUREC__)
#elif defined(__SC__)
    " by Symantec C " _LZO_MEXPAND(__SC__)
#elif defined(__TURBOC__)
    " by Turbo C " _LZO_MEXPAND(__TURBOC__)
#elif defined(__WATCOMC__)
    " by Watcom C " _LZO_MEXPAND(__WATCOMC__)
#endif
    " $\n"
    "$Copyright: LZO (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002 Markus Franz Xaver Johannes Oberhumer $\n";
#endif

#define LZO_BASE 65521u
#define LZO_NMAX 5552

#define LZO_DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define LZO_DO2(buf,i)  LZO_DO1(buf,i); LZO_DO1(buf,i+1);
#define LZO_DO4(buf,i)  LZO_DO2(buf,i); LZO_DO2(buf,i+2);
#define LZO_DO8(buf,i)  LZO_DO4(buf,i); LZO_DO4(buf,i+4);
#define LZO_DO16(buf,i) LZO_DO8(buf,i); LZO_DO8(buf,i+8);

#  define IS_SIGNED(type)       (((type) (-1)) < ((type) 0))
#  define IS_UNSIGNED(type)     (((type) (-1)) > ((type) 0))

#define IS_POWER_OF_2(x)        (((x) & ((x) - 1)) == 0)

static lzo_bool schedule_insns_bug(void);
static lzo_bool strength_reduce_bug(int *);

#  define __lzo_assert(x)   ((x) ? 1 : 0)

#undef COMPILE_TIME_ASSERT

#  define COMPILE_TIME_ASSERT(expr)     LZO_COMPILE_TIME_ASSERT(expr)

static lzo_bool basic_integral_check(void)
{
	lzo_bool r = 1;

	COMPILE_TIME_ASSERT(CHAR_BIT == 8);
	COMPILE_TIME_ASSERT(sizeof(char) == 1);
	COMPILE_TIME_ASSERT(sizeof(short) >= 2);
	COMPILE_TIME_ASSERT(sizeof(long) >= 4);
	COMPILE_TIME_ASSERT(sizeof(int) >= sizeof(short));
	COMPILE_TIME_ASSERT(sizeof(long) >= sizeof(int));

	COMPILE_TIME_ASSERT(sizeof(lzo_uint) == sizeof(lzo_int));
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == sizeof(lzo_int32));

	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) >= 4);
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) >= sizeof(unsigned));
#if defined(__LZO_STRICT_16BIT)
	COMPILE_TIME_ASSERT(sizeof(lzo_uint) == 2);
#else
	COMPILE_TIME_ASSERT(sizeof(lzo_uint) >= 4);
	COMPILE_TIME_ASSERT(sizeof(lzo_uint) >= sizeof(unsigned));
#endif

#if (USHRT_MAX == 65535u)
	COMPILE_TIME_ASSERT(sizeof(short) == 2);
#elif (USHRT_MAX == LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(short) == 4);
#elif (USHRT_MAX >= LZO_0xffffffffL)
	COMPILE_TIME_ASSERT(sizeof(short) > 4);
#endif
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned char));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned short));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(unsigned long));
	COMPILE_TIME_ASSERT(IS_SIGNED(short));
	COMPILE_TIME_ASSERT(IS_SIGNED(int));
	COMPILE_TIME_ASSERT(IS_SIGNED(long));

	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_uint32));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_uint));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_int32));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_int));

	COMPILE_TIME_ASSERT(INT_MAX == LZO_STYPE_MAX(sizeof(int)));
	COMPILE_TIME_ASSERT(UINT_MAX == LZO_UTYPE_MAX(sizeof(unsigned)));
	COMPILE_TIME_ASSERT(LONG_MAX == LZO_STYPE_MAX(sizeof(long)));
	COMPILE_TIME_ASSERT(ULONG_MAX == LZO_UTYPE_MAX(sizeof(unsigned long)));
	COMPILE_TIME_ASSERT(USHRT_MAX == LZO_UTYPE_MAX(sizeof(unsigned short)));
	COMPILE_TIME_ASSERT(LZO_UINT32_MAX ==
			    LZO_UTYPE_MAX(sizeof(lzo_uint32)));
	COMPILE_TIME_ASSERT(LZO_UINT_MAX == LZO_UTYPE_MAX(sizeof(lzo_uint)));

	r &= __lzo_assert(LZO_BYTE(257) == 1);

	return r;
}

static lzo_bool basic_ptr_check(void)
{
	lzo_bool r = 1;

	COMPILE_TIME_ASSERT(sizeof(char *) >= sizeof(int));
	COMPILE_TIME_ASSERT(sizeof(lzo_byte *) >= sizeof(char *));

	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_byte *));
	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_voidpp));
	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) == sizeof(lzo_bytepp));
	COMPILE_TIME_ASSERT(sizeof(lzo_voidp) >= sizeof(lzo_uint));

	COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) == sizeof(lzo_voidp));
	COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) == sizeof(lzo_sptr_t));
	COMPILE_TIME_ASSERT(sizeof(lzo_ptr_t) >= sizeof(lzo_uint));

	COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= 4);
	COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= sizeof(ptrdiff_t));

	COMPILE_TIME_ASSERT(sizeof(ptrdiff_t) >= sizeof(size_t));
	COMPILE_TIME_ASSERT(sizeof(lzo_ptrdiff_t) >= sizeof(lzo_uint));

#if defined(SIZEOF_CHAR_P)
	COMPILE_TIME_ASSERT(SIZEOF_CHAR_P == sizeof(char *));
#endif
#if defined(SIZEOF_PTRDIFF_T)
	COMPILE_TIME_ASSERT(SIZEOF_PTRDIFF_T == sizeof(ptrdiff_t));
#endif

	COMPILE_TIME_ASSERT(IS_SIGNED(ptrdiff_t));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(size_t));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_ptrdiff_t));
	COMPILE_TIME_ASSERT(IS_SIGNED(lzo_sptr_t));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_ptr_t));
	COMPILE_TIME_ASSERT(IS_UNSIGNED(lzo_moff_t));

	return r;
}

static lzo_bool ptr_check(void)
{
	lzo_bool r = 1;
	int i;
	char _wrkmem[10 * sizeof(lzo_byte *) + sizeof(lzo_full_align_t)];
	lzo_bytep wrkmem;
	lzo_bytepp dict;
	unsigned char x[4 * sizeof(lzo_full_align_t)];
	long d;
	lzo_full_align_t a;
	lzo_full_align_t u;

	for (i = 0; i < (int)sizeof(x); i++)
		x[i] = LZO_BYTE(i);

	wrkmem =
	    LZO_PTR_ALIGN_UP((lzo_byte *) _wrkmem, sizeof(lzo_full_align_t));

	u.a_lzo_bytep = wrkmem;
	dict = u.a_lzo_bytepp;

	d = (long)((const lzo_bytep)dict - (const lzo_bytep)_wrkmem);
	r &= __lzo_assert(d >= 0);
	r &= __lzo_assert(d < (long)sizeof(lzo_full_align_t));

	memset(&a, 0, sizeof(a));
	r &= __lzo_assert(a.a_lzo_voidp == NULL);

	memset(&a, 0xff, sizeof(a));
	r &= __lzo_assert(a.a_ushort == USHRT_MAX);
	r &= __lzo_assert(a.a_uint == UINT_MAX);
	r &= __lzo_assert(a.a_ulong == ULONG_MAX);
	r &= __lzo_assert(a.a_lzo_uint == LZO_UINT_MAX);
	r &= __lzo_assert(a.a_lzo_uint32 == LZO_UINT32_MAX);

	if (r == 1) {
		for (i = 0; i < 8; i++)
			r &= __lzo_assert((const lzo_voidp)(&dict[i]) ==
					  (const
					   lzo_voidp)(&wrkmem[i *
							      sizeof(lzo_byte
								     *)]));
	}

	memset(&a, 0, sizeof(a));
	r &= __lzo_assert(a.a_char_p == NULL);
	r &= __lzo_assert(a.a_lzo_bytep == NULL);
	r &= __lzo_assert(NULL == (void *)0);
	if (r == 1) {
		for (i = 0; i < 10; i++)
			dict[i] = wrkmem;
		BZERO8_PTR(dict + 1, sizeof(dict[0]), 8);
		r &= __lzo_assert(dict[0] == wrkmem);
		for (i = 1; i < 9; i++)
			r &= __lzo_assert(dict[i] == NULL);
		r &= __lzo_assert(dict[9] == wrkmem);
	}

	if (r == 1) {
		unsigned k = 1;
		const unsigned n = (unsigned)sizeof(lzo_uint32);
		lzo_byte *p0;
		lzo_byte *p1;

		k += __lzo_align_gap(&x[k], n);
		p0 = (lzo_bytep) & x[k];
#if defined(PTR_LINEAR)
		r &= __lzo_assert((PTR_LINEAR(p0) & (n - 1)) == 0);
#else
		r &= __lzo_assert(n == 4);
		r &= __lzo_assert(PTR_ALIGNED_4(p0));
#endif

		r &= __lzo_assert(k >= 1);
		p1 = (lzo_bytep) & x[1];
		r &= __lzo_assert(PTR_GE(p0, p1));

		r &= __lzo_assert(k < 1 + n);
		p1 = (lzo_bytep) & x[1 + n];
		r &= __lzo_assert(PTR_LT(p0, p1));

		if (r == 1) {
			lzo_uint32 v0, v1;

			u.a_uchar_p = &x[k];
			v0 = *u.a_lzo_uint32_p;
			u.a_uchar_p = &x[k + n];
			v1 = *u.a_lzo_uint32_p;

			r &= __lzo_assert(v0 > 0);
			r &= __lzo_assert(v1 > 0);
		}
	}

	return r;
}

static int _lzo_config_check(void)
{
	lzo_bool r = 1;
	int i;
	union {
		lzo_uint32 a;
		unsigned short b;
		lzo_uint32 aa[4];
		unsigned char x[4 * sizeof(lzo_full_align_t)];
	} u;

	COMPILE_TIME_ASSERT((int)((unsigned char)((signed char)-1)) == 255);
	COMPILE_TIME_ASSERT((((unsigned char)128) << (int)(8 * sizeof(int) - 8))
			    < 0);

	r &= basic_integral_check();
	r &= basic_ptr_check();
	if (r != 1)
		return LZO_E_ERROR;

	u.a = 0;
	u.b = 0;
	for (i = 0; i < (int)sizeof(u.x); i++)
		u.x[i] = LZO_BYTE(i);

#if defined(LZO_BYTE_ORDER)
	if (r == 1) {
#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		lzo_uint32 a = (lzo_uint32) (u.a & LZO_0xffffffffL);
		unsigned short b = (unsigned short)(u.b & 0xffff);
		r &= __lzo_assert(a == 0x03020100L);
		r &= __lzo_assert(b == 0x0100);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
		lzo_uint32 a = u.a >> (8 * sizeof(u.a) - 32);
		unsigned short b = u.b >> (8 * sizeof(u.b) - 16);
		r &= __lzo_assert(a == 0x00010203L);
		r &= __lzo_assert(b == 0x0001);
#  else
#    error "invalid LZO_BYTE_ORDER"
#  endif
	}
#endif

#if defined(LZO_UNALIGNED_OK_2)
	COMPILE_TIME_ASSERT(sizeof(short) == 2);
	if (r == 1) {
		unsigned short b[4];

		for (i = 0; i < 4; i++)
			b[i] = *(const unsigned short *)&u.x[i];

#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		r &= __lzo_assert(b[0] == 0x0100);
		r &= __lzo_assert(b[1] == 0x0201);
		r &= __lzo_assert(b[2] == 0x0302);
		r &= __lzo_assert(b[3] == 0x0403);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
		r &= __lzo_assert(b[0] == 0x0001);
		r &= __lzo_assert(b[1] == 0x0102);
		r &= __lzo_assert(b[2] == 0x0203);
		r &= __lzo_assert(b[3] == 0x0304);
#  endif
	}
#endif

#if defined(LZO_UNALIGNED_OK_4)
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == 4);
	if (r == 1) {
		lzo_uint32 a[4];

		for (i = 0; i < 4; i++)
			a[i] = *(const lzo_uint32 *)&u.x[i];

#  if (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
		r &= __lzo_assert(a[0] == 0x03020100L);
		r &= __lzo_assert(a[1] == 0x04030201L);
		r &= __lzo_assert(a[2] == 0x05040302L);
		r &= __lzo_assert(a[3] == 0x06050403L);
#  elif (LZO_BYTE_ORDER == LZO_BIG_ENDIAN)
		r &= __lzo_assert(a[0] == 0x00010203L);
		r &= __lzo_assert(a[1] == 0x01020304L);
		r &= __lzo_assert(a[2] == 0x02030405L);
		r &= __lzo_assert(a[3] == 0x03040506L);
#  endif
	}
#endif

#if defined(LZO_ALIGNED_OK_4)
	COMPILE_TIME_ASSERT(sizeof(lzo_uint32) == 4);
#endif

	COMPILE_TIME_ASSERT(lzo_sizeof_dict_t == sizeof(lzo_dict_t));

	if (r == 1) {
		r &= __lzo_assert(!schedule_insns_bug());
	}

	if (r == 1) {
		static int x[3];
		static unsigned xn = 3;
		register unsigned j;

		for (j = 0; j < xn; j++)
			x[j] = (int)j - 3;
		r &= __lzo_assert(!strength_reduce_bug(x));
	}

	if (r == 1) {
		r &= ptr_check();
	}

	return r == 1 ? LZO_E_OK : LZO_E_ERROR;
}

static lzo_bool schedule_insns_bug(void)
{
#if defined(__LZO_CHECKER)
	return 0;
#else
	const int clone[] = { 1, 2, 0 };
	const int *q;
	q = clone;
	return (*q) ? 0 : 1;
#endif
}

static lzo_bool strength_reduce_bug(int *x)
{
	return x[0] != -3 || x[1] != -2 || x[2] != -1;
}

#undef COMPILE_TIME_ASSERT

int __lzo_init2(unsigned v, int s1, int s2, int s3, int s4, int s5,
		int s6, int s7, int s8, int s9)
{
	int r;

	if (v == 0)
		return LZO_E_ERROR;

	r = (s1 == -1 || s1 == (int)sizeof(short)) &&
	    (s2 == -1 || s2 == (int)sizeof(int)) &&
	    (s3 == -1 || s3 == (int)sizeof(long)) &&
	    (s4 == -1 || s4 == (int)sizeof(lzo_uint32)) &&
	    (s5 == -1 || s5 == (int)sizeof(lzo_uint)) &&
	    (s6 == -1 || s6 == (int)lzo_sizeof_dict_t) &&
	    (s7 == -1 || s7 == (int)sizeof(char *)) &&
	    (s8 == -1 || s8 == (int)sizeof(lzo_voidp)) &&
	    (s9 == -1 || s9 == (int)sizeof(lzo_compress_t));
	if (!r)
		return LZO_E_ERROR;

	r = _lzo_config_check();
	if (r != LZO_E_OK)
		return r;

	return r;
}

#define do_compress         _lzo1x_1_do_compress

#define LZO_NEED_DICT_H
#define D_BITS          14
#define D_INDEX1(d,p)       d = DM((0x21*DX3(p,5,5,6)) >> 5)
#define D_INDEX2(d,p)       d = (d & (D_MASK & 0x7ff)) ^ (D_HIGH | 0x1f)

#ifndef __LZO_CONFIG1X_H
#define __LZO_CONFIG1X_H

#if !defined(LZO1X) && !defined(LZO1Y) && !defined(LZO1Z)
#  define LZO1X
#endif

#define LZO_EOF_CODE
#undef LZO_DETERMINISTIC

#define M1_MAX_OFFSET   0x0400
#ifndef M2_MAX_OFFSET
#define M2_MAX_OFFSET   0x0800
#endif
#define M3_MAX_OFFSET   0x4000
#define M4_MAX_OFFSET   0xbfff

#define MX_MAX_OFFSET   (M1_MAX_OFFSET + M2_MAX_OFFSET)

#define M1_MIN_LEN      2
#define M1_MAX_LEN      2
#define M2_MIN_LEN      3
#ifndef M2_MAX_LEN
#define M2_MAX_LEN      8
#endif
#define M3_MIN_LEN      3
#define M3_MAX_LEN      33
#define M4_MIN_LEN      3
#define M4_MAX_LEN      9

#define M1_MARKER       0
#define M2_MARKER       64
#define M3_MARKER       32
#define M4_MARKER       16

#ifndef MIN_LOOKAHEAD
#define MIN_LOOKAHEAD       (M2_MAX_LEN + 1)
#endif

#if defined(LZO_NEED_DICT_H)

#ifndef LZO_HASH
#define LZO_HASH            LZO_HASH_LZO_INCREMENTAL_B
#endif
#define DL_MIN_LEN          M2_MIN_LEN

#ifndef __LZO_DICT_H
#define __LZO_DICT_H

#if !defined(D_BITS) && defined(DBITS)
#  define D_BITS        DBITS
#endif
#if !defined(D_BITS)
#  error "D_BITS is not defined"
#endif
#if (D_BITS < 16)
#  define D_SIZE        LZO_SIZE(D_BITS)
#  define D_MASK        LZO_MASK(D_BITS)
#else
#  define D_SIZE        LZO_USIZE(D_BITS)
#  define D_MASK        LZO_UMASK(D_BITS)
#endif
#define D_HIGH          ((D_MASK >> 1) + 1)

#if !defined(DD_BITS)
#  define DD_BITS       0
#endif
#define DD_SIZE         LZO_SIZE(DD_BITS)
#define DD_MASK         LZO_MASK(DD_BITS)

#if !defined(DL_BITS)
#  define DL_BITS       (D_BITS - DD_BITS)
#endif
#if (DL_BITS < 16)
#  define DL_SIZE       LZO_SIZE(DL_BITS)
#  define DL_MASK       LZO_MASK(DL_BITS)
#else
#  define DL_SIZE       LZO_USIZE(DL_BITS)
#  define DL_MASK       LZO_UMASK(DL_BITS)
#endif

#if (D_BITS != DL_BITS + DD_BITS)
#  error "D_BITS does not match"
#endif
#if (D_BITS < 8 || D_BITS > 18)
#  error "invalid D_BITS"
#endif
#if (DL_BITS < 8 || DL_BITS > 20)
#  error "invalid DL_BITS"
#endif
#if (DD_BITS < 0 || DD_BITS > 6)
#  error "invalid DD_BITS"
#endif

#if !defined(DL_MIN_LEN)
#  define DL_MIN_LEN    3
#endif
#if !defined(DL_SHIFT)
#  define DL_SHIFT      ((DL_BITS + (DL_MIN_LEN - 1)) / DL_MIN_LEN)
#endif

#define LZO_HASH_GZIP                   1
#define LZO_HASH_GZIP_INCREMENTAL       2
#define LZO_HASH_LZO_INCREMENTAL_A      3
#define LZO_HASH_LZO_INCREMENTAL_B      4

#if !defined(LZO_HASH)
#  error "choose a hashing strategy"
#endif

#if (DL_MIN_LEN == 3)
#  define _DV2_A(p,shift1,shift2) \
	(((( (lzo_uint32)((p)[0]) << shift1) ^ (p)[1]) << shift2) ^ (p)[2])
#  define _DV2_B(p,shift1,shift2) \
	(((( (lzo_uint32)((p)[2]) << shift1) ^ (p)[1]) << shift2) ^ (p)[0])
#  define _DV3_B(p,shift1,shift2,shift3) \
	((_DV2_B((p)+1,shift1,shift2) << (shift3)) ^ (p)[0])
#elif (DL_MIN_LEN == 2)
#  define _DV2_A(p,shift1,shift2) \
	(( (lzo_uint32)(p[0]) << shift1) ^ p[1])
#  define _DV2_B(p,shift1,shift2) \
	(( (lzo_uint32)(p[1]) << shift1) ^ p[2])
#else
#  error "invalid DL_MIN_LEN"
#endif
#define _DV_A(p,shift)      _DV2_A(p,shift,shift)
#define _DV_B(p,shift)      _DV2_B(p,shift,shift)
#define DA2(p,s1,s2) \
	(((((lzo_uint32)((p)[2]) << (s2)) + (p)[1]) << (s1)) + (p)[0])
#define DS2(p,s1,s2) \
	(((((lzo_uint32)((p)[2]) << (s2)) - (p)[1]) << (s1)) - (p)[0])
#define DX2(p,s1,s2) \
	(((((lzo_uint32)((p)[2]) << (s2)) ^ (p)[1]) << (s1)) ^ (p)[0])
#define DA3(p,s1,s2,s3) ((DA2((p)+1,s2,s3) << (s1)) + (p)[0])
#define DS3(p,s1,s2,s3) ((DS2((p)+1,s2,s3) << (s1)) - (p)[0])
#define DX3(p,s1,s2,s3) ((DX2((p)+1,s2,s3) << (s1)) ^ (p)[0])
#define DMS(v,s)        ((lzo_uint) (((v) & (D_MASK >> (s))) << (s)))
#define DM(v)           DMS(v,0)

#if (LZO_HASH == LZO_HASH_GZIP)
#  define _DINDEX(dv,p)     (_DV_A((p),DL_SHIFT))

#elif (LZO_HASH == LZO_HASH_GZIP_INCREMENTAL)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_A((p),DL_SHIFT)
#  define DVAL_NEXT(dv,p)   dv = (((dv) << DL_SHIFT) ^ p[2])
#  define _DINDEX(dv,p)     (dv)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#elif (LZO_HASH == LZO_HASH_LZO_INCREMENTAL_A)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_A((p),5)
#  define DVAL_NEXT(dv,p) \
		dv ^= (lzo_uint32)(p[-1]) << (2*5); dv = (((dv) << 5) ^ p[2])
#  define _DINDEX(dv,p)     ((0x9f5f * (dv)) >> 5)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#elif (LZO_HASH == LZO_HASH_LZO_INCREMENTAL_B)
#  define __LZO_HASH_INCREMENTAL
#  define DVAL_FIRST(dv,p)  dv = _DV_B((p),5)
#  define DVAL_NEXT(dv,p) \
		dv ^= p[-1]; dv = (((dv) >> 5) ^ ((lzo_uint32)(p[2]) << (2*5)))
#  define _DINDEX(dv,p)     ((0x9f5f * (dv)) >> 5)
#  define DVAL_LOOKAHEAD    DL_MIN_LEN

#else
#  error "choose a hashing strategy"
#endif

#ifndef DINDEX
#define DINDEX(dv,p)        ((lzo_uint)((_DINDEX(dv,p)) & DL_MASK) << DD_BITS)
#endif
#if !defined(DINDEX1) && defined(D_INDEX1)
#define DINDEX1             D_INDEX1
#endif
#if !defined(DINDEX2) && defined(D_INDEX2)
#define DINDEX2             D_INDEX2
#endif

#if !defined(__LZO_HASH_INCREMENTAL)
#  define DVAL_FIRST(dv,p)  ((void) 0)
#  define DVAL_NEXT(dv,p)   ((void) 0)
#  define DVAL_LOOKAHEAD    0
#endif

#if !defined(DVAL_ASSERT)
#if defined(__LZO_HASH_INCREMENTAL) && !defined(NDEBUG)
static void DVAL_ASSERT(lzo_uint32 dv, const lzo_byte * p)
{
	lzo_uint32 df;
	DVAL_FIRST(df, (p));
	assert(DINDEX(dv, p) == DINDEX(df, p));
}
#else
#  define DVAL_ASSERT(dv,p) ((void) 0)
#endif
#endif

#  define DENTRY(p,in)                          (p)
#  define GINDEX(m_pos,m_off,dict,dindex,in)    m_pos = dict[dindex]

#if (DD_BITS == 0)

#  define UPDATE_D(dict,drun,dv,p,in)       dict[ DINDEX(dv,p) ] = DENTRY(p,in)
#  define UPDATE_I(dict,drun,index,p,in)    dict[index] = DENTRY(p,in)
#  define UPDATE_P(ptr,drun,p,in)           (ptr)[0] = DENTRY(p,in)

#else

#  define UPDATE_D(dict,drun,dv,p,in)   \
	dict[ DINDEX(dv,p) + drun++ ] = DENTRY(p,in); drun &= DD_MASK
#  define UPDATE_I(dict,drun,index,p,in)    \
	dict[ (index) + drun++ ] = DENTRY(p,in); drun &= DD_MASK
#  define UPDATE_P(ptr,drun,p,in)   \
	(ptr) [ drun++ ] = DENTRY(p,in); drun &= DD_MASK

#endif

#define LZO_CHECK_MPOS_DET(m_pos,m_off,in,ip,max_offset) \
	(m_pos == NULL || (m_off = (lzo_moff_t) (ip - m_pos)) > max_offset)

#define LZO_CHECK_MPOS_NON_DET(m_pos,m_off,in,ip,max_offset) \
    (BOUNDS_CHECKING_OFF_IN_EXPR( \
	(PTR_LT(m_pos,in) || \
	 (m_off = (lzo_moff_t) PTR_DIFF(ip,m_pos)) <= 0 || \
	  m_off > max_offset) ))

#if defined(LZO_DETERMINISTIC)
#  define LZO_CHECK_MPOS    LZO_CHECK_MPOS_DET
#else
#  define LZO_CHECK_MPOS    LZO_CHECK_MPOS_NON_DET
#endif
#endif
#endif
#endif
#define DO_COMPRESS     lzo1x_1_compress
static
lzo_uint do_compress(const lzo_byte * in, lzo_uint in_len,
		     lzo_byte * out, lzo_uintp out_len, lzo_voidp wrkmem)
{
	register const lzo_byte *ip;
	lzo_byte *op;
	const lzo_byte *const in_end = in + in_len;
	const lzo_byte *const ip_end = in + in_len - M2_MAX_LEN - 5;
	const lzo_byte *ii;
	lzo_dict_p const dict = (lzo_dict_p) wrkmem;

	op = out;
	ip = in;
	ii = ip;

	ip += 4;
	for (;;) {
		register const lzo_byte *m_pos;

		lzo_moff_t m_off;
		lzo_uint m_len;
		lzo_uint dindex;

		DINDEX1(dindex, ip);
		GINDEX(m_pos, m_off, dict, dindex, in);
		if (LZO_CHECK_MPOS_NON_DET(m_pos, m_off, in, ip, M4_MAX_OFFSET))
			goto literal;
#if 1
		if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
			goto try_match;
		DINDEX2(dindex, ip);
#endif
		GINDEX(m_pos, m_off, dict, dindex, in);
		if (LZO_CHECK_MPOS_NON_DET(m_pos, m_off, in, ip, M4_MAX_OFFSET))
			goto literal;
		if (m_off <= M2_MAX_OFFSET || m_pos[3] == ip[3])
			goto try_match;
		goto literal;

	      try_match:
#if 1 && defined(LZO_UNALIGNED_OK_2)
		if (*(const lzo_ushortp)m_pos != *(const lzo_ushortp)ip) {
#else
		if (m_pos[0] != ip[0] || m_pos[1] != ip[1]) {
#endif
			;
		} else {
			if (m_pos[2] == ip[2]) {
				goto match;
			} else {
				;
			}
		}

	      literal:
		UPDATE_I(dict, 0, dindex, ip, in);
		++ip;
		if (ip >= ip_end)
			break;
		continue;

	      match:
		UPDATE_I(dict, 0, dindex, ip, in);
		if (pd(ip, ii) > 0) {
			register lzo_uint t = pd(ip, ii);

			if (t <= 3) {
				assert("lzo-04", op - 2 > out);
				op[-2] |= LZO_BYTE(t);
			} else if (t <= 18)
				*op++ = LZO_BYTE(t - 3);
			else {
				register lzo_uint tt = t - 18;

				*op++ = 0;
				while (tt > 255) {
					tt -= 255;
					*op++ = 0;
				}
				assert("lzo-05", tt > 0);
				*op++ = LZO_BYTE(tt);
			}
			do
				*op++ = *ii++;
			while (--t > 0);
		}

		assert("lzo-06", ii == ip);
		ip += 3;
		if (m_pos[3] != *ip++ || m_pos[4] != *ip++ || m_pos[5] != *ip++
		    || m_pos[6] != *ip++ || m_pos[7] != *ip++
		    || m_pos[8] != *ip++
#ifdef LZO1Y
		    || m_pos[9] != *ip++ || m_pos[10] != *ip++
		    || m_pos[11] != *ip++ || m_pos[12] != *ip++
		    || m_pos[13] != *ip++ || m_pos[14] != *ip++
#endif
		    ) {
			--ip;
			m_len = ip - ii;
			assert("lzo-07", m_len >= 3);
			assert("lzo-08", m_len <= M2_MAX_LEN);

			if (m_off <= M2_MAX_OFFSET) {
				m_off -= 1;
#if defined(LZO1X)
				*op++ =
				    LZO_BYTE(((m_len -
					       1) << 5) | ((m_off & 7) << 2));
				*op++ = LZO_BYTE(m_off >> 3);
#elif defined(LZO1Y)
				*op++ =
				    LZO_BYTE(((m_len +
					       1) << 4) | ((m_off & 3) << 2));
				*op++ = LZO_BYTE(m_off >> 2);
#endif
			} else if (m_off <= M3_MAX_OFFSET) {
				m_off -= 1;
				*op++ = LZO_BYTE(M3_MARKER | (m_len - 2));
				goto m3_m4_offset;
			} else
#if defined(LZO1X)
			{
				m_off -= 0x4000;
				assert("lzo-09", m_off > 0);
				assert("lzo-10", m_off <= 0x7fff);
				*op++ = LZO_BYTE(M4_MARKER |
						 ((m_off & 0x4000) >> 11) |
						 (m_len - 2));
				goto m3_m4_offset;
			}
#elif defined(LZO1Y)
				goto m4_match;
#endif
		} else {
			{
				const lzo_byte *end = in_end;
				const lzo_byte *m = m_pos + M2_MAX_LEN + 1;
				while (ip < end && *m == *ip)
					m++, ip++;
				m_len = (ip - ii);
			}
			assert("lzo-11", m_len > M2_MAX_LEN);

			if (m_off <= M3_MAX_OFFSET) {
				m_off -= 1;
				if (m_len <= 33)
					*op++ =
					    LZO_BYTE(M3_MARKER | (m_len - 2));
				else {
					m_len -= 33;
					*op++ = M3_MARKER | 0;
					goto m3_m4_len;
				}
			} else {
#if defined(LZO1Y)
			      m4_match:
#endif
				m_off -= 0x4000;
				assert("lzo-12", m_off > 0);
				assert("lzo-13", m_off <= 0x7fff);
				if (m_len <= M4_MAX_LEN)
					*op++ = LZO_BYTE(M4_MARKER |
							 ((m_off & 0x4000) >>
							  11) | (m_len - 2));
				else {
					m_len -= M4_MAX_LEN;
					*op++ =
					    LZO_BYTE(M4_MARKER |
						     ((m_off & 0x4000) >> 11));
				      m3_m4_len:
					while (m_len > 255) {
						m_len -= 255;
						*op++ = 0;
					}
					assert("lzo-14", m_len > 0);
					*op++ = LZO_BYTE(m_len);
				}
			}

		      m3_m4_offset:
			*op++ = LZO_BYTE((m_off & 63) << 2);
			*op++ = LZO_BYTE(m_off >> 6);
		}

		ii = ip;
		if (ip >= ip_end)
			break;
	}

	*out_len = op - out;
	return pd(in_end, ii);
}

int DO_COMPRESS(const lzo_byte * in, lzo_uint in_len,
		lzo_byte * out, lzo_uintp out_len, lzo_voidp wrkmem)
{
	lzo_byte *op = out;
	lzo_uint t;

#if defined(__LZO_QUERY_COMPRESS)
	if (__LZO_IS_COMPRESS_QUERY(in, in_len, out, out_len, wrkmem))
		return __LZO_QUERY_COMPRESS(in, in_len, out, out_len, wrkmem,
					    D_SIZE, lzo_sizeof(lzo_dict_t));
#endif

	if (in_len <= M2_MAX_LEN + 5)
		t = in_len;
	else {
		t = do_compress(in, in_len, op, out_len, wrkmem);
		op += *out_len;
	}

	if (t > 0) {
		const lzo_byte *ii = in + in_len - t;

		if (op == out && t <= 238)
			*op++ = LZO_BYTE(17 + t);
		else if (t <= 3)
			op[-2] |= LZO_BYTE(t);
		else if (t <= 18)
			*op++ = LZO_BYTE(t - 3);
		else {
			lzo_uint tt = t - 18;

			*op++ = 0;
			while (tt > 255) {
				tt -= 255;
				*op++ = 0;
			}
			assert("lzo-15", tt > 0);
			*op++ = LZO_BYTE(tt);
		}
		do
			*op++ = *ii++;
		while (--t > 0);
	}

	*op++ = M4_MARKER | 1;
	*op++ = 0;
	*op++ = 0;

	*out_len = op - out;
	return LZO_E_OK;
}

#undef do_compress
#undef DO_COMPRESS
#undef LZO_HASH

#undef LZO_TEST_DECOMPRESS_OVERRUN
#undef LZO_TEST_DECOMPRESS_OVERRUN_INPUT
#undef LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT
#undef LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND
#undef DO_DECOMPRESS
#define DO_DECOMPRESS       lzo1x_decompress

#if defined(LZO_TEST_DECOMPRESS_OVERRUN)
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND
#  endif
#endif

#undef TEST_IP
#undef TEST_OP
#undef TEST_LOOKBEHIND
#undef NEED_IP
#undef NEED_OP
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
	    if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP
#    define NEED_OP(x) \
	    if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#  define TEST_LOOKBEHIND(m_pos,out)    if (m_pos < out) goto lookbehind_overrun
#else
#  define TEST_LOOKBEHIND(m_pos,op)     ((void) 0)
#endif

#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
#  define TEST_IP               (ip < ip_end)
#endif

#if defined(TEST_IP)
#  define HAVE_TEST_IP
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP
#else
#  define TEST_OP               1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP
#else
#  define NEED_IP(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP
#else
#  define NEED_OP(x)            ((void) 0)
#endif

#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP
#endif

#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#if defined(LZO_UNALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4(dst,src)
#elif defined(LZO_ALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4((lzo_ptr_t)(dst),(lzo_ptr_t)(src))
#endif

#if defined(DO_DECOMPRESS)
int DO_DECOMPRESS(const lzo_byte * in, lzo_uint in_len,
		  lzo_byte * out, lzo_uintp out_len, lzo_voidp wrkmem)
#endif
{
	register lzo_byte *op;
	register const lzo_byte *ip;
	register lzo_uint t;
#if defined(COPY_DICT)
	lzo_uint m_off;
	const lzo_byte *dict_end;
#else
	register const lzo_byte *m_pos;
#endif

	const lzo_byte *const ip_end = in + in_len;
#if defined(HAVE_ANY_OP)
	lzo_byte *const op_end = out + *out_len;
#endif
#if defined(LZO1Z)
	lzo_uint last_m_off = 0;
#endif

	LZO_UNUSED(wrkmem);

#if defined(__LZO_QUERY_DECOMPRESS)
	if (__LZO_IS_DECOMPRESS_QUERY(in, in_len, out, out_len, wrkmem))
		return __LZO_QUERY_DECOMPRESS(in, in_len, out, out_len, wrkmem,
					      0, 0);
#endif

#if defined(COPY_DICT)
	if (dict) {
		if (dict_len > M4_MAX_OFFSET) {
			dict += dict_len - M4_MAX_OFFSET;
			dict_len = M4_MAX_OFFSET;
		}
		dict_end = dict + dict_len;
	} else {
		dict_len = 0;
		dict_end = NULL;
	}
#endif

	*out_len = 0;

	op = out;
	ip = in;

	if (*ip > 17) {
		t = *ip++ - 17;
		if (t < 4)
			goto match_next;
		assert("lzo-16", t > 0);
		NEED_OP(t);
		NEED_IP(t + 1);
		do
			*op++ = *ip++;
		while (--t > 0);
		goto first_literal_run;
	}

	while (TEST_IP && TEST_OP) {
		t = *ip++;
		if (t >= 16)
			goto match;
		if (t == 0) {
			NEED_IP(1);
			while (*ip == 0) {
				t += 255;
				ip++;
				NEED_IP(1);
			}
			t += 15 + *ip++;
		}
		assert("lzo-17", t > 0);
		NEED_OP(t + 3);
		NEED_IP(t + 4);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
		if (PTR_ALIGNED2_4(op, ip)) {
#endif
			COPY4(op, ip);
			op += 4;
			ip += 4;
			if (--t > 0) {
				if (t >= 4) {
					do {
						COPY4(op, ip);
						op += 4;
						ip += 4;
						t -= 4;
					} while (t >= 4);
					if (t > 0)
						do
							*op++ = *ip++;
						while (--t > 0);
				} else
					do
						*op++ = *ip++;
					while (--t > 0);
			}
#if !defined(LZO_UNALIGNED_OK_4)
		} else
#endif
#endif
#if !defined(LZO_UNALIGNED_OK_4)
		{
			*op++ = *ip++;
			*op++ = *ip++;
			*op++ = *ip++;
			do
				*op++ = *ip++;
			while (--t > 0);
		}
#endif

	      first_literal_run:

		t = *ip++;
		if (t >= 16)
			goto match;
#if defined(COPY_DICT)
#if defined(LZO1Z)
		m_off = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
		last_m_off = m_off;
#else
		m_off = (1 + M2_MAX_OFFSET) + (t >> 2) + (*ip++ << 2);
#endif
		NEED_OP(3);
		t = 3;
		COPY_DICT(t, m_off)
#else
#if defined(LZO1Z)
		t = (1 + M2_MAX_OFFSET) + (t << 6) + (*ip++ >> 2);
		m_pos = op - t;
		last_m_off = t;
#else
		m_pos = op - (1 + M2_MAX_OFFSET);
		m_pos -= t >> 2;
		m_pos -= *ip++ << 2;
#endif
		TEST_LOOKBEHIND(m_pos, out);
		NEED_OP(3);
		*op++ = *m_pos++;
		*op++ = *m_pos++;
		*op++ = *m_pos;
#endif
		goto match_done;

		while (TEST_IP && TEST_OP) {
		      match:
			if (t >= 64) {
#if defined(COPY_DICT)
#if defined(LZO1X)
				m_off = 1 + ((t >> 2) & 7) + (*ip++ << 3);
				t = (t >> 5) - 1;
#elif defined(LZO1Y)
				m_off = 1 + ((t >> 2) & 3) + (*ip++ << 2);
				t = (t >> 4) - 3;
#elif defined(LZO1Z)
				m_off = t & 0x1f;
				if (m_off >= 0x1c)
					m_off = last_m_off;
				else {
					m_off = 1 + (m_off << 6) + (*ip++ >> 2);
					last_m_off = m_off;
				}
				t = (t >> 5) - 1;
#endif
#else
#if defined(LZO1X)
				m_pos = op - 1;
				m_pos -= (t >> 2) & 7;
				m_pos -= *ip++ << 3;
				t = (t >> 5) - 1;
#elif defined(LZO1Y)
				m_pos = op - 1;
				m_pos -= (t >> 2) & 3;
				m_pos -= *ip++ << 2;
				t = (t >> 4) - 3;
#elif defined(LZO1Z)
				{
					lzo_uint off = t & 0x1f;
					m_pos = op;
					if (off >= 0x1c) {
						assert(last_m_off > 0);
						m_pos -= last_m_off;
					} else {
						off =
						    1 + (off << 6) +
						    (*ip++ >> 2);
						m_pos -= off;
						last_m_off = off;
					}
				}
				t = (t >> 5) - 1;
#endif
				TEST_LOOKBEHIND(m_pos, out);
				assert("lzo-18", t > 0);
				NEED_OP(t + 3 - 1);
				goto copy_match;
#endif
			} else if (t >= 32) {
				t &= 31;
				if (t == 0) {
					NEED_IP(1);
					while (*ip == 0) {
						t += 255;
						ip++;
						NEED_IP(1);
					}
					t += 31 + *ip++;
				}
#if defined(COPY_DICT)
#if defined(LZO1Z)
				m_off = 1 + (ip[0] << 6) + (ip[1] >> 2);
				last_m_off = m_off;
#else
				m_off = 1 + (ip[0] >> 2) + (ip[1] << 6);
#endif
#else
#if defined(LZO1Z)
				{
					lzo_uint off =
					    1 + (ip[0] << 6) + (ip[1] >> 2);
					m_pos = op - off;
					last_m_off = off;
				}
#elif defined(LZO_UNALIGNED_OK_2) && (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
				m_pos = op - 1;
				m_pos -= (*(const lzo_ushortp)ip) >> 2;
#else
				m_pos = op - 1;
				m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
#endif
				ip += 2;
			} else if (t >= 16) {
#if defined(COPY_DICT)
				m_off = (t & 8) << 11;
#else
				m_pos = op;
				m_pos -= (t & 8) << 11;
#endif
				t &= 7;
				if (t == 0) {
					NEED_IP(1);
					while (*ip == 0) {
						t += 255;
						ip++;
						NEED_IP(1);
					}
					t += 7 + *ip++;
				}
#if defined(COPY_DICT)
#if defined(LZO1Z)
				m_off += (ip[0] << 6) + (ip[1] >> 2);
#else
				m_off += (ip[0] >> 2) + (ip[1] << 6);
#endif
				ip += 2;
				if (m_off == 0)
					goto eof_found;
				m_off += 0x4000;
#if defined(LZO1Z)
				last_m_off = m_off;
#endif
#else
#if defined(LZO1Z)
				m_pos -= (ip[0] << 6) + (ip[1] >> 2);
#elif defined(LZO_UNALIGNED_OK_2) && (LZO_BYTE_ORDER == LZO_LITTLE_ENDIAN)
				m_pos -= (*(const lzo_ushortp)ip) >> 2;
#else
				m_pos -= (ip[0] >> 2) + (ip[1] << 6);
#endif
				ip += 2;
				if (m_pos == op)
					goto eof_found;
				m_pos -= 0x4000;
#if defined(LZO1Z)
				last_m_off = op - m_pos;
#endif
#endif
			} else {
#if defined(COPY_DICT)
#if defined(LZO1Z)
				m_off = 1 + (t << 6) + (*ip++ >> 2);
				last_m_off = m_off;
#else
				m_off = 1 + (t >> 2) + (*ip++ << 2);
#endif
				NEED_OP(2);
				t = 2;
				COPY_DICT(t, m_off)
#else
#if defined(LZO1Z)
				t = 1 + (t << 6) + (*ip++ >> 2);
				m_pos = op - t;
				last_m_off = t;
#else
				m_pos = op - 1;
				m_pos -= t >> 2;
				m_pos -= *ip++ << 2;
#endif
				TEST_LOOKBEHIND(m_pos, out);
				NEED_OP(2);
				*op++ = *m_pos++;
				*op++ = *m_pos;
#endif
				goto match_done;
			}

#if defined(COPY_DICT)

			NEED_OP(t + 3 - 1);
			t += 3 - 1;
			COPY_DICT(t, m_off)
#else

			TEST_LOOKBEHIND(m_pos, out);
			assert("lzo-19", t > 0);
			NEED_OP(t + 3 - 1);
#if defined(LZO_UNALIGNED_OK_4) || defined(LZO_ALIGNED_OK_4)
#if !defined(LZO_UNALIGNED_OK_4)
			if (t >= 2 * 4 - (3 - 1) && PTR_ALIGNED2_4(op, m_pos)) {
				assert((op - m_pos) >= 4);
#else
			if (t >= 2 * 4 - (3 - 1) && (op - m_pos) >= 4) {
#endif
				COPY4(op, m_pos);
				op += 4;
				m_pos += 4;
				t -= 4 - (3 - 1);
				do {
					COPY4(op, m_pos);
					op += 4;
					m_pos += 4;
					t -= 4;
				} while (t >= 4);
				if (t > 0)
					do
						*op++ = *m_pos++;
					while (--t > 0);
			} else
#endif
			{
			      copy_match:
				*op++ = *m_pos++;
				*op++ = *m_pos++;
				do
					*op++ = *m_pos++;
				while (--t > 0);
			}

#endif

		      match_done:
#if defined(LZO1Z)
			t = ip[-1] & 3;
#else
			t = ip[-2] & 3;
#endif
			if (t == 0)
				break;

		      match_next:
			assert("lzo-20", t > 0);
			NEED_OP(t);
			NEED_IP(t + 1);
			do
				*op++ = *ip++;
			while (--t > 0);
			t = *ip++;
		}
	}

#if defined(HAVE_TEST_IP) || defined(HAVE_TEST_OP)
	*out_len = op - out;
	return LZO_E_EOF_NOT_FOUND;
#endif

      eof_found:
	assert("lzo-21", t == 1);
	*out_len = op - out;
	return (ip == ip_end ? LZO_E_OK :
		(ip < ip_end ? LZO_E_INPUT_NOT_CONSUMED : LZO_E_INPUT_OVERRUN));

#if defined(HAVE_NEED_IP)
      input_overrun:
	*out_len = op - out;
	return LZO_E_INPUT_OVERRUN;
#endif

#if defined(HAVE_NEED_OP)
      output_overrun:
	*out_len = op - out;
	return LZO_E_OUTPUT_OVERRUN;
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
      lookbehind_overrun:
	*out_len = op - out;
	return LZO_E_LOOKBEHIND_OVERRUN;
#endif
}

#define LZO_TEST_DECOMPRESS_OVERRUN
#undef DO_DECOMPRESS
#define DO_DECOMPRESS       lzo1x_decompress_safe

#if defined(LZO_TEST_DECOMPRESS_OVERRUN)
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_INPUT       2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#    define LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT      2
#  endif
#  if !defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#    define LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND
#  endif
#endif

#undef TEST_IP
#undef TEST_OP
#undef TEST_LOOKBEHIND
#undef NEED_IP
#undef NEED_OP
#undef HAVE_TEST_IP
#undef HAVE_TEST_OP
#undef HAVE_NEED_IP
#undef HAVE_NEED_OP
#undef HAVE_ANY_IP
#undef HAVE_ANY_OP

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_INPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 1)
#    define TEST_IP             (ip < ip_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_INPUT >= 2)
#    define NEED_IP(x) \
	    if ((lzo_uint)(ip_end - ip) < (lzo_uint)(x))  goto input_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT)
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 1)
#    define TEST_OP             (op <= op_end)
#  endif
#  if (LZO_TEST_DECOMPRESS_OVERRUN_OUTPUT >= 2)
#    undef TEST_OP
#    define NEED_OP(x) \
	    if ((lzo_uint)(op_end - op) < (lzo_uint)(x))  goto output_overrun
#  endif
#endif

#if defined(LZO_TEST_DECOMPRESS_OVERRUN_LOOKBEHIND)
#  define TEST_LOOKBEHIND(m_pos,out)    if (m_pos < out) goto lookbehind_overrun
#else
#  define TEST_LOOKBEHIND(m_pos,op)     ((void) 0)
#endif

#if !defined(LZO_EOF_CODE) && !defined(TEST_IP)
#  define TEST_IP               (ip < ip_end)
#endif

#if defined(TEST_IP)
#  define HAVE_TEST_IP
#else
#  define TEST_IP               1
#endif
#if defined(TEST_OP)
#  define HAVE_TEST_OP
#else
#  define TEST_OP               1
#endif

#if defined(NEED_IP)
#  define HAVE_NEED_IP
#else
#  define NEED_IP(x)            ((void) 0)
#endif
#if defined(NEED_OP)
#  define HAVE_NEED_OP
#else
#  define NEED_OP(x)            ((void) 0)
#endif

#if defined(HAVE_TEST_IP) || defined(HAVE_NEED_IP)
#  define HAVE_ANY_IP
#endif
#if defined(HAVE_TEST_OP) || defined(HAVE_NEED_OP)
#  define HAVE_ANY_OP
#endif

#undef __COPY4
#define __COPY4(dst,src)    * (lzo_uint32p)(dst) = * (const lzo_uint32p)(src)

#undef COPY4
#if defined(LZO_UNALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4(dst,src)
#elif defined(LZO_ALIGNED_OK_4)
#  define COPY4(dst,src)    __COPY4((lzo_ptr_t)(dst),(lzo_ptr_t)(src))
#endif

/***** End of minilzo.c *****/
