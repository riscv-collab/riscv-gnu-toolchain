/* Low-level functions for atomic operations. RISC-V version.
   Copyright (C) 2011-2016 Free Software Foundation, Inc.

   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.

   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _RISCV_BITS_ATOMIC_H
#define _RISCV_BITS_ATOMIC_H 1

#include <inttypes.h>

typedef int32_t atomic32_t;
typedef uint32_t uatomic32_t;
typedef int_fast32_t atomic_fast32_t;
typedef uint_fast32_t uatomic_fast32_t;

typedef int64_t atomic64_t;
typedef uint64_t uatomic64_t;
typedef int_fast64_t atomic_fast64_t;
typedef uint_fast64_t uatomic_fast64_t;

typedef intptr_t atomicptr_t;
typedef uintptr_t uatomicptr_t;
typedef intmax_t atomic_max_t;
typedef uintmax_t uatomic_max_t;

#ifdef __riscv_atomic

#ifdef __riscv64
# define __HAVE_64B_ATOMICS 1
#else
# define __HAVE_64B_ATOMICS 0
#endif

#define USE_ATOMIC_COMPILER_BUILTINS 1

#define asm_amo(which, ordering, mem, value) ({ 		\
  typeof(*mem) __tmp; 						\
  if (sizeof(__tmp) == 4)					\
    asm volatile (which ".w" ordering "\t%0, %z2, %1"		\
		  : "=r"(__tmp), "+A"(*(mem))			\
		  : "rJ"(value));				\
  else if (sizeof(__tmp) == 8)					\
    asm volatile (which ".d" ordering "\t%0, %z2, %1"		\
		  : "=r"(__tmp), "+A"(*(mem))			\
		  : "rJ"(value));				\
  else								\
    abort();							\
  __tmp; })

/* Atomic compare and exchange. */

#define atomic_cas(ordering, mem, newval, oldval) ({ 		\
  typeof(*mem) __tmp; 						\
  int __tmp2; 							\
  if (sizeof(__tmp) == 4)					\
    asm volatile ("1: lr.w" ordering "\t%0, %2\n"		\
		  "bne\t%0, %z4, 1f\n"				\
		  "sc.w" ordering "\t%1, %z3, %2\n"		\
		  "bnez\t%1, 1b\n"				\
		  "1:"						\
		  : "=&r"(__tmp), "=&r"(__tmp2), "+A"(*(mem))	\
		  : "rJ"(newval), "rJ"(oldval));		\
  else if (sizeof(__tmp) == 8)					\
    asm volatile ("1: lr.d" ordering "\t%0, %2\n"		\
		  "bne\t%0, %z4, 1f\n"				\
		  "sc.d" ordering "\t%1, %z3, %2\n"		\
		  "bnez\t%1, 1b\n"				\
		  "1:"						\
		  : "=&r"(__tmp), "=&r"(__tmp2), "+A"(*(mem))	\
		  : "rJ"(newval), "rJ"(oldval));		\
  else								\
    abort();							\
  __tmp; })

#define atomic_compare_and_exchange_val_acq(mem, newval, oldval) \
  atomic_cas(".aq", mem, newval, oldval)

#define atomic_compare_and_exchange_val_rel(mem, newval, oldval) \
  atomic_cas(".rl", mem, newval, oldval)

/* Atomic exchange (without compare).  */

#define atomic_exchange_acq(mem, value) asm_amo("amoswap", ".aq", mem, value)
#define atomic_exchange_rel(mem, value) asm_amo("amoswap", ".rl", mem, value)


/* Atomically add value and return the previous (unincremented) value.  */

#define atomic_exchange_and_add(mem, value) asm_amo("amoadd", "", mem, value)

#define atomic_max(mem, value) asm_amo("amomaxu", "", mem, value)
#define atomic_min(mem, value) asm_amo("amominu", "", mem, value)

#define atomic_bit_test_set(mem, bit)                   \
  ({ typeof(*mem) __mask = (typeof(*mem))1 << (bit);    \
     asm_amo("amoor", "", mem, __mask) & __mask; })

#define atomic_full_barrier() __sync_synchronize()

#define catomic_exchange_and_add(mem, value)		\
  atomic_exchange_and_add(mem, value)
#define catomic_max(mem, value) atomic_max(mem, value)

#else /* __riscv_atomic */

#define __HAVE_64B_ATOMICS 0
#define USE_ATOMIC_COMPILER_BUILTINS 0

#endif /* !__riscv_atomic */

#endif /* bits/atomic.h */
