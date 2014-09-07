;; Generic DFA-based pipeline description for RISC-V targets.
;; Copyright (C) 2011-2014 Free Software Foundation, Inc.
;; Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
;; Based on MIPS target for GNU compiler.

;; This file is part of GCC.

;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 3, or (at your
;; option) any later version.

;; GCC is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
;; License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.


;; This file is derived from the old define_function_unit description.
;; Each reservation can be overridden on a processor-by-processor basis.

(define_insn_reservation "generic_alu" 1
  (eq_attr "type" "unknown,const,arith,shift,slt,multi,nop,logical,move")
  "alu")

(define_insn_reservation "generic_load" 3
  (eq_attr "type" "load,fpload,fpidxload")
  "alu")

(define_insn_reservation "generic_store" 1
  (eq_attr "type" "store,fpstore,fpidxstore")
  "alu")

(define_insn_reservation "generic_xfer" 2
  (eq_attr "type" "mfc,mtc")
  "alu")

(define_insn_reservation "generic_branch" 1
  (eq_attr "type" "branch,jump,call")
  "alu")

(define_insn_reservation "generic_imul" 17
  (eq_attr "type" "imul")
  "imuldiv*17")

(define_insn_reservation "generic_idiv" 38
  (eq_attr "type" "idiv")
  "imuldiv*38")

(define_insn_reservation "generic_fcvt" 1
  (eq_attr "type" "fcvt")
  "alu")

(define_insn_reservation "generic_fmove" 2
  (eq_attr "type" "fmove")
  "alu")

(define_insn_reservation "generic_fcmp" 3
  (eq_attr "type" "fcmp")
  "alu")

(define_insn_reservation "generic_fadd" 4
  (eq_attr "type" "fadd")
  "alu")

(define_insn_reservation "generic_fmul_single" 7
  (and (eq_attr "type" "fmul,fmadd")
       (eq_attr "mode" "SF"))
  "alu")

(define_insn_reservation "generic_fmul_double" 8
  (and (eq_attr "type" "fmul,fmadd")
       (eq_attr "mode" "DF"))
  "alu")

(define_insn_reservation "generic_fdiv_single" 23
  (and (eq_attr "type" "fdiv")
       (eq_attr "mode" "SF"))
  "alu")

(define_insn_reservation "generic_fdiv_double" 36
  (and (eq_attr "type" "fdiv")
       (eq_attr "mode" "DF"))
  "alu")

(define_insn_reservation "generic_fsqrt_single" 54
  (and (eq_attr "type" "fsqrt")
       (eq_attr "mode" "SF"))
  "alu")

(define_insn_reservation "generic_fsqrt_double" 112
  (and (eq_attr "type" "fsqrt")
       (eq_attr "mode" "DF"))
  "alu")
