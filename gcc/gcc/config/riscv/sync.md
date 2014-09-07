;; Machine description for RISC-V atomic operations.
;; Copyright (C) 2011-2014 Free Software Foundation, Inc.
;; Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
;; Based on MIPS target for GNU compiler.

;; This file is part of GCC.

;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.

;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

(define_c_enum "unspec" [
  UNSPEC_COMPARE_AND_SWAP
  UNSPEC_COMPARE_AND_SWAP_12
  UNSPEC_SYNC_OLD_OP
  UNSPEC_SYNC_NEW_OP
  UNSPEC_SYNC_NEW_OP_12
  UNSPEC_SYNC_OLD_OP_12
  UNSPEC_SYNC_EXCHANGE
  UNSPEC_SYNC_EXCHANGE_12
  UNSPEC_MEMORY_BARRIER
])

(define_code_iterator any_atomic [plus ior xor and])

;; Atomic memory operations.

(define_expand "memory_barrier"
  [(set (match_dup 0)
	(unspec:BLK [(match_dup 0)] UNSPEC_MEMORY_BARRIER))]
  ""
{
  operands[0] = gen_rtx_MEM (BLKmode, gen_rtx_SCRATCH (Pmode));
  MEM_VOLATILE_P (operands[0]) = 1;
})

(define_insn "*memory_barrier"
  [(set (match_operand:BLK 0 "" "")
	(unspec:BLK [(match_dup 0)] UNSPEC_MEMORY_BARRIER))]
  ""
  "fence")

(define_insn "sync_<optab><mode>"
  [(set (match_operand:GPR 0 "memory_operand" "+YR")
	(unspec_volatile:GPR
          [(any_atomic:GPR (match_dup 0)
		     (match_operand:GPR 1 "reg_or_0_operand" "rJ"))]
	 UNSPEC_SYNC_OLD_OP))]
  "TARGET_ATOMIC"
  "amo<insn>.<amo> zero,%z1,%0")

(define_insn "sync_old_<optab><mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&r")
	(match_operand:GPR 1 "memory_operand" "+YR"))
   (set (match_dup 1)
	(unspec_volatile:GPR
          [(any_atomic:GPR (match_dup 1)
		     (match_operand:GPR 2 "reg_or_0_operand" "rJ"))]
	 UNSPEC_SYNC_OLD_OP))]
  "TARGET_ATOMIC"
  "amo<insn>.<amo> %0,%z2,%1")

(define_insn "sync_lock_test_and_set<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&r")
	(match_operand:GPR 1 "memory_operand" "+YR"))
   (set (match_dup 1)
	(unspec_volatile:GPR [(match_operand:GPR 2 "reg_or_0_operand" "rJ")]
	 UNSPEC_SYNC_EXCHANGE))]
  "TARGET_ATOMIC"
  "amoswap.<amo> %0,%z2,%1")

(define_insn "sync_compare_and_swap<mode>"
  [(set (match_operand:GPR 0 "register_operand" "=&r")
	(match_operand:GPR 1 "memory_operand" "+YR"))
   (set (match_dup 1)
	(unspec_volatile:GPR [(match_operand:GPR 2 "reg_or_0_operand" "rJ")
			      (match_operand:GPR 3 "reg_or_0_operand" "rJ")]
	 UNSPEC_COMPARE_AND_SWAP))
   (clobber (match_scratch:GPR 4 "=&r"))]
  "TARGET_ATOMIC"
  "1: lr.<amo> %0,%1; bne %0,%z2,1f; sc.<amo> %4,%z3,%1; bnez %4,1b; 1:"
  [(set (attr "length") (const_int 16))])
