/* The common simulator framework for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney and Red Hat.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#ifdef SIM_COMMON_BUILD
#error "This header is unusable in common builds due to reliance on SIM_AC_OPTION_BITSIZE"
#endif

#include <stdint.h>

#include "symcat.h"

/* INTEGER QUANTITIES:

   TYPES:

     intNN_t    Signed type of the given bit size
     uintNN_t   The corresponding unsigned type

     signed128     Non-standard type for 128-bit integers.
     unsigned128   Likewise, but unsigned.

   SIZES

     *NN	Size based on the number of bits
     *_NN       Size according to the number of bytes
     *_word     Size based on the target architecture's word
     		word size (32/64 bits)
     *_cell     Size based on the target architecture's
     		IEEE 1275 cell size (almost always 32 bits)

*/


/* bit based */

#ifdef _MSC_VER
# define UNSIGNED32(X)	(X##ui32)
# define UNSIGNED64(X)	(X##ui64)
# define SIGNED32(X)	(X##i32)
# define SIGNED64(X)	(X##i64)
#else
# define UNSIGNED32(X)	((uint32_t) X##UL)
# define UNSIGNED64(X)	((uint64_t) X##ULL)
# define SIGNED32(X)	((int32_t) X##L)
# define SIGNED64(X)	((int64_t) X##LL)
#endif

typedef struct { uint64_t a[2]; } unsigned128;
typedef struct { int64_t a[2]; } signed128;


/* byte based */

typedef int8_t signed_1;
typedef int16_t signed_2;
typedef int32_t signed_4;
typedef int64_t signed_8;
typedef signed128 signed_16;

typedef uint8_t unsigned_1;
typedef uint16_t unsigned_2;
typedef uint32_t unsigned_4;
typedef uint64_t unsigned_8;
typedef unsigned128 unsigned_16;


/* Macros for printf.  Usage is restricted to this header.  */
#define SIM_PRI_TB(t, b)	XCONCAT3 (PRI,t,b)


/* for general work, the following are defined */
/* unsigned: >= 32 bits */
/* signed:   >= 32 bits */
/* long:     >= 32 bits, sign undefined */
/* int:      small indicator */

/* target architecture based */
#if (WITH_TARGET_WORD_BITSIZE == 64)
typedef uint64_t unsigned_word;
typedef int64_t signed_word;
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
typedef uint32_t unsigned_word;
typedef int32_t signed_word;
#endif
#if (WITH_TARGET_WORD_BITSIZE == 16)
typedef uint16_t unsigned_word;
typedef int16_t signed_word;
#endif

#define PRI_TW(t)	SIM_PRI_TB (t, WITH_TARGET_WORD_BITSIZE)
#define PRIiTW	PRI_TW (i)
#define PRIxTW	PRI_TW (x)


/* Other instructions */
#if (WITH_TARGET_ADDRESS_BITSIZE == 64)
typedef uint64_t unsigned_address;
typedef int64_t signed_address;
#endif
#if (WITH_TARGET_ADDRESS_BITSIZE == 32)
typedef uint32_t unsigned_address;
typedef int32_t signed_address;
#endif
#if (WITH_TARGET_ADDRESS_BITSIZE == 16)
typedef uint16_t unsigned_address;
typedef int16_t signed_address;
#endif
typedef unsigned_address address_word;

#define PRI_TA(t)	SIM_PRI_TB (t, WITH_TARGET_ADDRESS_BITSIZE)
#define PRIiTA	PRI_TA (i)
#define PRIxTA	PRI_TA (x)


/* IEEE 1275 cell size */
#if (WITH_TARGET_CELL_BITSIZE == 64)
typedef uint64_t unsigned_cell;
typedef int64_t signed_cell;
#endif
#if (WITH_TARGET_CELL_BITSIZE == 32)
typedef uint32_t unsigned_cell;
typedef int32_t signed_cell;
#endif
typedef signed_cell cell_word; /* cells are normally signed */

#define PRI_TC(t)	SIM_PRI_TB (t, WITH_TARGET_CELL_BITSIZE)
#define PRIiTC	PRI_TC (i)
#define PRIxTC	PRI_TC (x)


/* Floating point registers */
#if (WITH_TARGET_FLOATING_POINT_BITSIZE == 64)
typedef uint64_t fp_word;
#endif
#if (WITH_TARGET_FLOATING_POINT_BITSIZE == 32)
typedef uint32_t fp_word;
#endif

#define PRI_TF(t)	SIM_PRI_TB (t, WITH_TARGET_FLOATING_POINT_BITSIZE)
#define PRIiTF	PRI_TF (i)
#define PRIxTF	PRI_TF (x)

#endif
