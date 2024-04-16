/* This file is part of psim (model of the PowerPC(tm) architecture)

   Copyright (C) 1994-1995, Andrew Cagney <cagney@highland.com.au>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 3 of
   the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 
   --

   PowerPC is a trademark of International Business Machines Corporation. */


/* Basic type sizes for the PowerPC */

#ifndef _WORDS_H_
#define _WORDS_H_

/* TYPES:

     intNN_t    Signed type of the given bit size
     uintNN_t   The corresponding unsigned type

   SIZES

     *NN	Size based on the number of bits
     *_NN       Size according to the number of bytes
     *_word     Size based on the target architecture's word
     		word size (32/64 bits)
     *_cell     Size based on the target architecture's
     		IEEE 1275 cell size (almost always 32 bits)

*/


/* This must come before any other includes.  */
#include "defs.h"

#include <stdint.h>

/* byte based */
typedef int8_t signed_1;
typedef int16_t signed_2;
typedef int32_t signed_4;
typedef int64_t signed_8;

typedef uint8_t unsigned_1;
typedef uint16_t unsigned_2;
typedef uint32_t unsigned_4;
typedef uint64_t unsigned_8;


/* for general work, the following are defined */
/* unsigned: >= 32 bits */
/* signed:   >= 32 bits */
/* long:     >= 32 bits, sign undefined */
/* int:      small indicator */

/* target architecture based */
#if (WITH_TARGET_WORD_BITSIZE == 64)
typedef uint64_t unsigned_word;
typedef int64_t signed_word;
#else
typedef uint32_t unsigned_word;
typedef int32_t signed_word;
#endif


/* Other instructions */
typedef uint32_t instruction_word;

/* IEEE 1275 cell size - only support 32bit mode at present */
typedef uint32_t unsigned_cell;
typedef int32_t signed_cell;

#endif /* _WORDS_H_ */
