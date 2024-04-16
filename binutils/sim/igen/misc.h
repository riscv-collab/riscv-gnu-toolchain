/* The IGEN simulator generator for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney.

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

#ifndef IGEN_MISC_H
#define IGEN_MISC_H

/* Frustrating header junk */

enum
{
  default_insn_bit_size = 32,
  max_insn_bit_size = 64,
};


#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ansidecl.h"

#include "filter_host.h"

typedef struct _line_ref line_ref;
struct _line_ref
{
  const char *file_name;
  int line_nr;
};

/* Error appends a new line, warning and notify do not */
typedef void error_func (const line_ref *line, const char *msg, ...)
  ATTRIBUTE_PRINTF_2;

extern error_func error ATTRIBUTE_PRINTF_2 ATTRIBUTE_NORETURN;
extern error_func warning ATTRIBUTE_PRINTF_2;
extern error_func notify ATTRIBUTE_PRINTF_2;


#define ERROR(EXPRESSION, args...) \
do { \
  line_ref line; \
  line.file_name = filter_filename (__FILE__); \
  line.line_nr = __LINE__; \
  error (&line, EXPRESSION "\n", ## args); \
} while (0)

#define ASSERT(EXPRESSION) \
do { \
  if (!(EXPRESSION)) { \
    line_ref line; \
    line.file_name = filter_filename (__FILE__); \
    line.line_nr = __LINE__; \
    error (&line, "assertion failed - %s\n", #EXPRESSION); \
  } \
} while (0)

#define ZALLOC(TYPE) ((TYPE*) zalloc (sizeof(TYPE)))
#define NZALLOC(TYPE,N) ((TYPE*) zalloc (sizeof(TYPE) * (N)))

extern void *zalloc (long size);

extern unsigned target_a2i (int ms_bit_nr, const char *a);

extern unsigned i2target (int ms_bit_nr, unsigned bit);

extern unsigned long long a2i (const char *a);


/* Try looking for name in the map table (returning the corresponding
   integer value).

   If the the sentinal (NAME == NULL) its value if >= zero is returned
   as the default. */

typedef struct _name_map
{
  const char *name;
  int i;
}
name_map;

extern int name2i (const char *name, const name_map * map);

extern const char *i2name (const int i, const name_map * map);

#endif /* IGEN_MISC_H */
