/* RISC-V atomic operations.
   Copyright (C) 2016 Free Software Foundation, Inc

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

/* RISC-V doesn't have hardware support for atomic operations less than 32 bits
 * wide.  */

/* This is a simple spin lock, all the emulation routines serialize here.  */
static int lock = 0;

static void get_lock(void)
{
    while (__sync_val_compare_and_swap (&lock, 0, 1) != 0);
}

static void put_lock(void)
{
    lock = 0;
}

/* Emulation routines for GCC's atomic operations. */

#define GENERATE__SYNC_FETCH_AND_X(type, suffix, opname, op) \
type __attribute__((hidden)) \
__sync_fetch_and_ ## opname ## _ ## suffix (type *p, type v) \
{ \
    type tmp; \
    get_lock(); \
    tmp = *p; \
    *p = op; \
    put_lock(); \
    return tmp; \
} \
\
type __attribute__((hidden)) \
__sync_ ## opname ## _and_fetch_ ## suffix (type *p, type v) \
{ \
    get_lock(); \
    *p = op; \
    put_lock(); \
    return *p; \
}

GENERATE__SYNC_FETCH_AND_X(char,  1, add,  *p + v);
GENERATE__SYNC_FETCH_AND_X(short, 2, add,  *p + v);
GENERATE__SYNC_FETCH_AND_X(char,  1, sub,  *p - v);
GENERATE__SYNC_FETCH_AND_X(short, 2, sub,  *p - v);
GENERATE__SYNC_FETCH_AND_X(char,  1, or,   *p | v);
GENERATE__SYNC_FETCH_AND_X(short, 2, or,   *p | v);
GENERATE__SYNC_FETCH_AND_X(char,  1, nand, ~(*p & v));
GENERATE__SYNC_FETCH_AND_X(short, 2, nand, ~(*p & v));

#define GENERATE__SYNC_OTHER(type, suffix)  \
type __attribute__((hidden)) \
__sync_val_compare_and_swap_ ## suffix (type *p, type oldval, type newval) \
{ \
    type readval; \
    get_lock(); \
    readval = *p; \
    if (*p == oldval) \
        *p = newval; \
    put_lock(); \
    return readval; \
} \
\
char __attribute__((hidden)) \
__sync_bool_compare_and_swap_ ## suffix (type *p, type oldval, type newval) \
{ \
    type readval; \
    char out = 0; \
    get_lock(); \
    readval = *p; \
    if (*p == oldval) \
      { \
        out = 1; \
        *p = newval; \
      } \
    put_lock(); \
    return out; \
} \
\
type __attribute__((hidden)) \
__sync_lock_test_and_set_ ## suffix (type *p, type newval) \
{ \
    type readval; \
    get_lock(); \
    readval = *p; \
    *p = newval; \
    put_lock(); \
    return readval; \
} \
\
void __attribute__((hidden)) \
__sync_lock_release_ ## suffix (type *p) \
{ \
    *p = 0; \
}

GENERATE__SYNC_OTHER(char, 1)
GENERATE__SYNC_OTHER(short, 2)
