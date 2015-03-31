;; Machine description for RISC-V for GNU compiler.
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
  ;; Floating-point moves.
  UNSPEC_LOAD_LOW
  UNSPEC_LOAD_HIGH
  UNSPEC_STORE_WORD

  ;; GP manipulation.
  UNSPEC_EH_RETURN

  ;; Symbolic accesses.
  UNSPEC_ADDRESS_FIRST
  UNSPEC_LOAD_GOT
  UNSPEC_TLS
  UNSPEC_TLS_LE
  UNSPEC_TLS_IE
  UNSPEC_TLS_GD

  ;; Blockage and synchronisation.
  UNSPEC_BLOCKAGE
  UNSPEC_FENCE
  UNSPEC_FENCE_I
])

(define_constants
  [(RETURN_ADDR_REGNUM		1)
])

(include "predicates.md")
(include "constraints.md")

;; ....................
;;
;;	Attributes
;;
;; ....................

(define_attr "got" "unset,xgot_high,load"
  (const_string "unset"))

;; For jal instructions, this attribute is DIRECT when the target address
;; is symbolic and INDIRECT when it is a register.
(define_attr "jal" "unset,direct,indirect"
  (const_string "unset"))

;; Classification of moves, extensions and truncations.  Most values
;; are as for "type" (see below) but there are also the following
;; move-specific values:
;;
;; andi		a single ANDI instruction
;; shift_shift	a shift left followed by a shift right
;;
;; This attribute is used to determine the instruction's length and
;; scheduling type.  For doubleword moves, the attribute always describes
;; the split instructions; in some cases, it is more appropriate for the
;; scheduling type to be "multi" instead.
(define_attr "move_type"
  "unknown,load,fpload,store,fpstore,mtc,mfc,move,fmove,
   const,logical,arith,andi,shift_shift"
  (const_string "unknown"))

(define_attr "alu_type" "unknown,add,sub,and,or,xor"
  (const_string "unknown"))

;; Main data type used by the insn
(define_attr "mode" "unknown,none,QI,HI,SI,DI,TI,SF,DF,TF,FPSW"
  (const_string "unknown"))

;; True if the main data type is twice the size of a word.
(define_attr "dword_mode" "no,yes"
  (cond [(and (eq_attr "mode" "DI,DF")
	      (eq (symbol_ref "TARGET_64BIT") (const_int 0)))
	 (const_string "yes")

	 (and (eq_attr "mode" "TI,TF")
	      (ne (symbol_ref "TARGET_64BIT") (const_int 0)))
	 (const_string "yes")]
	(const_string "no")))

;; Classification of each insn.
;; branch	conditional branch
;; jump		unconditional jump
;; call		unconditional call
;; load		load instruction(s)
;; fpload	floating point load
;; fpidxload    floating point indexed load
;; store	store instruction(s)
;; fpstore	floating point store
;; fpidxstore	floating point indexed store
;; mtc		transfer to coprocessor
;; mfc		transfer from coprocessor
;; const	load constant
;; arith	integer arithmetic instructions
;; logical      integer logical instructions
;; shift	integer shift instructions
;; slt		set less than instructions
;; imul		integer multiply 
;; idiv		integer divide
;; move		integer register move (addi rd, rs1, 0)
;; fmove	floating point register move
;; fadd		floating point add/subtract
;; fmul		floating point multiply
;; fmadd	floating point multiply-add
;; fdiv		floating point divide
;; fcmp		floating point compare
;; fcvt		floating point convert
;; fsqrt	floating point square root
;; multi	multiword sequence (or user asm statements)
;; nop		no operation
;; ghost	an instruction that produces no real code
(define_attr "type"
  "unknown,branch,jump,call,load,fpload,fpidxload,store,fpstore,fpidxstore,
   mtc,mfc,const,arith,logical,shift,slt,imul,idiv,move,fmove,fadd,fmul,
   fmadd,fdiv,fcmp,fcvt,fsqrt,multi,nop,ghost"
  (cond [(eq_attr "jal" "!unset") (const_string "call")
	 (eq_attr "got" "load") (const_string "load")

	 (eq_attr "alu_type" "add,sub") (const_string "arith")

	 (eq_attr "alu_type" "and,or,xor") (const_string "logical")

	 ;; If a doubleword move uses these expensive instructions,
	 ;; it is usually better to schedule them in the same way
	 ;; as the singleword form, rather than as "multi".
	 (eq_attr "move_type" "load") (const_string "load")
	 (eq_attr "move_type" "fpload") (const_string "fpload")
	 (eq_attr "move_type" "store") (const_string "store")
	 (eq_attr "move_type" "fpstore") (const_string "fpstore")
	 (eq_attr "move_type" "mtc") (const_string "mtc")
	 (eq_attr "move_type" "mfc") (const_string "mfc")

	 ;; These types of move are always single insns.
	 (eq_attr "move_type" "fmove") (const_string "fmove")
	 (eq_attr "move_type" "arith") (const_string "arith")
	 (eq_attr "move_type" "logical") (const_string "logical")
	 (eq_attr "move_type" "andi") (const_string "logical")

	 ;; These types of move are always split.
	 (eq_attr "move_type" "shift_shift")
	   (const_string "multi")

	 ;; These types of move are split for doubleword modes only.
	 (and (eq_attr "move_type" "move,const")
	      (eq_attr "dword_mode" "yes"))
	   (const_string "multi")
	 (eq_attr "move_type" "move") (const_string "move")
	 (eq_attr "move_type" "const") (const_string "const")]
	(const_string "unknown")))

;; Mode for conversion types (fcvt)
;; I2S          integer to float single (SI/DI to SF)
;; I2D          integer to float double (SI/DI to DF)
;; S2I          float to integer (SF to SI/DI)
;; D2I          float to integer (DF to SI/DI)
;; D2S          double to float single
;; S2D          float single to double

(define_attr "cnv_mode" "unknown,I2S,I2D,S2I,D2I,D2S,S2D" 
  (const_string "unknown"))

;; Length of instruction in bytes.
(define_attr "length" ""
   (cond [
	  ;; Direct branch instructions have a range of [-0x1000,0xffc],
	  ;; relative to the address of the delay slot.  If a branch is
	  ;; outside this range, convert a branch like:
	  ;;
	  ;;	bne	r1,r2,target
	  ;;
	  ;; to:
	  ;;
	  ;;	beq	r1,r2,1f
	  ;;  j target
	  ;; 1:
	  ;;
	  (eq_attr "type" "branch")
	  (if_then_else (and (le (minus (match_dup 0) (pc)) (const_int 4088))
				  (le (minus (pc) (match_dup 0)) (const_int 4092)))
	  (const_int 4)
	  (const_int 8))

	  ;; Conservatively assume calls take two instructions, as in:
	  ;;   auipc t0, %pcrel_hi(target)
	  ;;   jalr  ra, t0, %lo(target)
	  ;; The linker will relax these into JAL when appropriate.
	  (eq_attr "type" "call")
	  (const_int 8)

	  ;; "Ghost" instructions occupy no space.
	  (eq_attr "type" "ghost")
	  (const_int 0)

	  (eq_attr "got" "load") (const_int 8)

	  ;; SHIFT_SHIFTs are decomposed into two separate instructions.
	  (eq_attr "move_type" "shift_shift")
		(const_int 8)

	  ;; Check for doubleword moves that are decomposed into two
	  ;; instructions.
	  (and (eq_attr "move_type" "mtc,mfc,move")
	       (eq_attr "dword_mode" "yes"))
	  (const_int 8)

	  ;; Doubleword CONST{,N} moves are split into two word
	  ;; CONST{,N} moves.
	  (and (eq_attr "move_type" "const")
	       (eq_attr "dword_mode" "yes"))
	  (symbol_ref "riscv_split_const_insns (operands[1]) * 4")

	  ;; Otherwise, constants, loads and stores are handled by external
	  ;; routines.
	  (eq_attr "move_type" "load,fpload")
	  (symbol_ref "riscv_load_store_insns (operands[1], insn) * 4")
	  (eq_attr "move_type" "store,fpstore")
	  (symbol_ref "riscv_load_store_insns (operands[0], insn) * 4")
	  ] (const_int 4)))

;; Describe a user's asm statement.
(define_asm_attributes
  [(set_attr "type" "multi")])

;; This mode iterator allows 32-bit and 64-bit GPR patterns to be generated
;; from the same template.
(define_mode_iterator GPR [SI (DI "TARGET_64BIT")])
(define_mode_iterator SUPERQI [HI SI (DI "TARGET_64BIT")])

;; A copy of GPR that can be used when a pattern has two independent
;; modes.
(define_mode_iterator GPR2 [SI (DI "TARGET_64BIT")])

;; This mode iterator allows :P to be used for patterns that operate on
;; pointer-sized quantities.  Exactly one of the two alternatives will match.
(define_mode_iterator P [(SI "Pmode == SImode") (DI "Pmode == DImode")])

;; 32-bit integer moves for which we provide move patterns.
(define_mode_iterator IMOVE32 [SI])

;; 64-bit modes for which we provide move patterns.
(define_mode_iterator MOVE64 [DI DF])

;; 128-bit modes for which we provide move patterns on 64-bit targets.
(define_mode_iterator MOVE128 [TI TF])

;; This mode iterator allows the QI and HI extension patterns to be
;; defined from the same template.
(define_mode_iterator SHORT [QI HI])

;; Likewise the 64-bit truncate-and-shift patterns.
(define_mode_iterator SUBDI [QI HI SI])
(define_mode_iterator HISI [HI SI])
(define_mode_iterator ANYI [QI HI SI (DI "TARGET_64BIT")])

;; This mode iterator allows :ANYF to be used wherever a scalar or vector
;; floating-point mode is allowed.
(define_mode_iterator ANYF [(SF "TARGET_HARD_FLOAT")
			    (DF "TARGET_HARD_FLOAT")])
(define_mode_iterator ANYIF [QI HI SI (DI "TARGET_64BIT")
			     (SF "TARGET_HARD_FLOAT")
			     (DF "TARGET_HARD_FLOAT")])

;; Like ANYF, but only applies to scalar modes.
(define_mode_iterator SCALARF [(SF "TARGET_HARD_FLOAT")
			       (DF "TARGET_HARD_FLOAT")])

;; A floating-point mode for which moves involving FPRs may need to be split.
(define_mode_iterator SPLITF
  [(DF "!TARGET_64BIT")
   (DI "!TARGET_64BIT")
   (TF "TARGET_64BIT")])

;; This attribute gives the length suffix for a sign- or zero-extension
;; instruction.
(define_mode_attr size [(QI "b") (HI "h")])

;; Mode attributes for loads.
(define_mode_attr load [(QI "lb") (HI "lh") (SI "lw") (DI "ld") (SF "flw") (DF "fld")])

;; Instruction names for stores.
(define_mode_attr store [(QI "sb") (HI "sh") (SI "sw") (DI "sd") (SF "fsw") (DF "fsd")])

;; This attribute gives the best constraint to use for registers of
;; a given mode.
(define_mode_attr reg [(SI "d") (DI "d") (CC "d")])

;; This attribute gives the format suffix for floating-point operations.
(define_mode_attr fmt [(SF "s") (DF "d")])

;; This attribute gives the format suffix for atomic memory operations.
(define_mode_attr amo [(SI "w") (DI "d")])

;; This attribute gives the upper-case mode name for one unit of a
;; floating-point mode.
(define_mode_attr UNITMODE [(SF "SF") (DF "DF")])

;; This attribute gives the integer mode that has half the size of
;; the controlling mode.
(define_mode_attr HALFMODE [(DF "SI") (DI "SI") (TF "DI")])

;; This code iterator allows signed and unsigned widening multiplications
;; to use the same template.
(define_code_iterator any_extend [sign_extend zero_extend])

;; This code iterator allows the two right shift instructions to be
;; generated from the same template.
(define_code_iterator any_shiftrt [ashiftrt lshiftrt])

;; This code iterator allows the three shift instructions to be generated
;; from the same template.
(define_code_iterator any_shift [ashift ashiftrt lshiftrt])

;; This code iterator allows unsigned and signed division to be generated
;; from the same template.
(define_code_iterator any_div [div udiv])

;; This code iterator allows unsigned and signed modulus to be generated
;; from the same template.
(define_code_iterator any_mod [mod umod])

;; These code iterators allow the signed and unsigned scc operations to use
;; the same template.
(define_code_iterator any_gt [gt gtu])
(define_code_iterator any_ge [ge geu])
(define_code_iterator any_lt [lt ltu])
(define_code_iterator any_le [le leu])

;; <u> expands to an empty string when doing a signed operation and
;; "u" when doing an unsigned operation.
(define_code_attr u [(sign_extend "") (zero_extend "u")
		     (div "") (udiv "u")
		     (mod "") (umod "u")
		     (gt "") (gtu "u")
		     (ge "") (geu "u")
		     (lt "") (ltu "u")
		     (le "") (leu "u")])

;; <su> is like <u>, but the signed form expands to "s" rather than "".
(define_code_attr su [(sign_extend "s") (zero_extend "u")])

;; <optab> expands to the name of the optab for a particular code.
(define_code_attr optab [(ashift "ashl")
			 (ashiftrt "ashr")
			 (lshiftrt "lshr")
			 (ior "ior")
			 (xor "xor")
			 (and "and")
			 (plus "add")
			 (minus "sub")])

;; <insn> expands to the name of the insn that implements a particular code.
(define_code_attr insn [(ashift "sll")
			(ashiftrt "sra")
			(lshiftrt "srl")
			(ior "or")
			(xor "xor")
			(and "and")
			(plus "add")
			(minus "sub")])

;; Pipeline descriptions.
;;
;; generic.md provides a fallback for processors without a specific
;; pipeline description.  It is derived from the old define_function_unit
;; version and uses the "alu" and "imuldiv" units declared below.
;;
;; Some of the processor-specific files are also derived from old
;; define_function_unit descriptions and simply override the parts of
;; generic.md that don't apply.  The other processor-specific files
;; are self-contained.
(define_automaton "alu,imuldiv")

(define_cpu_unit "alu" "alu")
(define_cpu_unit "imuldiv" "imuldiv")

;; Ghost instructions produce no real code and introduce no hazards.
;; They exist purely to express an effect on dataflow.
(define_insn_reservation "ghost" 0
  (eq_attr "type" "ghost")
  "nothing")

(include "generic.md")

;;
;;  ....................
;;
;;	ADDITION
;;
;;  ....................
;;

(define_insn "add<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(plus:ANYF (match_operand:ANYF 1 "register_operand" "f")
		   (match_operand:ANYF 2 "register_operand" "f")))]
  ""
  "fadd.<fmt>\t%0,%1,%2"
  [(set_attr "type" "fadd")
   (set_attr "mode" "<UNITMODE>")])

(define_expand "add<mode>3"
  [(set (match_operand:GPR 0 "register_operand")
	(plus:GPR (match_operand:GPR 1 "register_operand")
		  (match_operand:GPR 2 "arith_operand")))]
  "")

(define_insn "*addsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	(plus:SI (match_operand:GPR 1 "register_operand" "r,r")
		  (match_operand:GPR2 2 "arith_operand" "r,Q")))]
  ""
  { return TARGET_64BIT ? "addw\t%0,%1,%2" : "add\t%0,%1,%2"; }
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*adddi3"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(plus:DI (match_operand:DI 1 "register_operand" "r,r")
		  (match_operand:DI 2 "arith_operand" "r,Q")))]
  "TARGET_64BIT"
  "add\t%0,%1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "DI")])

(define_insn "*addsi3_extended"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
	(sign_extend:DI
	     (plus:SI (match_operand:SI 1 "register_operand" "r,r")
		      (match_operand:SI 2 "arith_operand" "r,Q"))))]
  "TARGET_64BIT"
  "addw\t%0,%1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*adddisi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	     (plus:SI (truncate:SI (match_operand:DI 1 "register_operand" "r,r"))
		      (truncate:SI (match_operand:DI 2 "arith_operand" "r,Q"))))]
  "TARGET_64BIT"
  "addw\t%0,%1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*adddisisi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
	     (plus:SI (truncate:SI (match_operand:DI 1 "register_operand" "r,r"))
		      (match_operand:SI 2 "arith_operand" "r,Q")))]
  "TARGET_64BIT"
  "addw\t%0,%1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*adddi3_truncsi"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
          (truncate:SI
	     (plus:DI (match_operand:DI 1 "register_operand" "r,r")
		      (match_operand:DI 2 "arith_operand" "r,Q"))))]
  "TARGET_64BIT"
  "addw\t%0,%1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

;;
;;  ....................
;;
;;	SUBTRACTION
;;
;;  ....................
;;

(define_insn "sub<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(minus:ANYF (match_operand:ANYF 1 "register_operand" "f")
		    (match_operand:ANYF 2 "register_operand" "f")))]
  ""
  "fsub.<fmt>\t%0,%1,%2"
  [(set_attr "type" "fadd")
   (set_attr "mode" "<UNITMODE>")])

(define_expand "sub<mode>3"
  [(set (match_operand:GPR 0 "register_operand")
	(minus:GPR (match_operand:GPR 1 "reg_or_0_operand")
		   (match_operand:GPR 2 "register_operand")))]
  "")

(define_insn "*subdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(minus:DI (match_operand:DI 1 "reg_or_0_operand" "rJ")
		   (match_operand:DI 2 "register_operand" "r")))]
  "TARGET_64BIT"
  "sub\t%0,%z1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "DI")])

(define_insn "*subsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(minus:SI (match_operand:GPR 1 "reg_or_0_operand" "rJ")
		   (match_operand:GPR2 2 "register_operand" "r")))]
  ""
  { return TARGET_64BIT ? "subw\t%0,%z1,%2" : "sub\t%0,%z1,%2"; }
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*subsi3_extended"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(sign_extend:DI
	    (minus:SI (match_operand:SI 1 "reg_or_0_operand" "rJ")
		      (match_operand:SI 2 "register_operand" "r"))))]
  "TARGET_64BIT"
  "subw\t%0,%z1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "DI")])

(define_insn "*subdisi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	     (minus:SI (truncate:SI (match_operand:DI 1 "reg_or_0_operand" "rJ"))
		      (truncate:SI (match_operand:DI 2 "register_operand" "r"))))]
  "TARGET_64BIT"
  "subw\t%0,%z1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*subdisisi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	     (minus:SI (truncate:SI (match_operand:DI 1 "reg_or_0_operand" "rJ"))
		      (match_operand:SI 2 "register_operand" "r")))]
  "TARGET_64BIT"
  "subw\t%0,%z1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*subsidisi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	     (minus:SI (match_operand:SI 1 "reg_or_0_operand" "rJ")
		      (truncate:SI (match_operand:DI 2 "register_operand" "r"))))]
  "TARGET_64BIT"
  "subw\t%0,%z1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

(define_insn "*subdi3_truncsi"
  [(set (match_operand:SI 0 "register_operand" "=r,r")
          (truncate:SI
	     (minus:DI (match_operand:DI 1 "reg_or_0_operand" "rJ,r")
		      (match_operand:DI 2 "arith_operand" "r,Q"))))]
  "TARGET_64BIT"
  "subw\t%0,%z1,%2"
  [(set_attr "type" "arith")
   (set_attr "mode" "SI")])

;;
;;  ....................
;;
;;	MULTIPLICATION
;;
;;  ....................
;;

(define_insn "mul<mode>3"
  [(set (match_operand:SCALARF 0 "register_operand" "=f")
	(mult:SCALARF (match_operand:SCALARF 1 "register_operand" "f")
		      (match_operand:SCALARF 2 "register_operand" "f")))]
  ""
  "fmul.<fmt>\t%0,%1,%2"
  [(set_attr "type" "fmul")
   (set_attr "mode" "<UNITMODE>")])

(define_expand "mul<mode>3"
  [(set (match_operand:GPR 0 "register_operand")
	(mult:GPR (match_operand:GPR 1 "reg_or_0_operand")
		   (match_operand:GPR 2 "register_operand")))]
  "TARGET_MULDIV")

(define_insn "*mulsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(mult:SI (match_operand:GPR 1 "register_operand" "r")
		  (match_operand:GPR2 2 "register_operand" "r")))]
  "TARGET_MULDIV"
  { return TARGET_64BIT ? "mulw\t%0,%1,%2" : "mul\t%0,%1,%2"; }
  [(set_attr "type" "imul")
   (set_attr "mode" "SI")])

(define_insn "*muldisi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	     (mult:SI (truncate:SI (match_operand:DI 1 "register_operand" "r"))
		      (truncate:SI (match_operand:DI 2 "register_operand" "r"))))]
  "TARGET_MULDIV && TARGET_64BIT"
  "mulw\t%0,%1,%2"
  [(set_attr "type" "imul")
   (set_attr "mode" "SI")])

(define_insn "*muldi3_truncsi"
  [(set (match_operand:SI 0 "register_operand" "=r")
          (truncate:SI
	     (mult:DI (match_operand:DI 1 "register_operand" "r")
		      (match_operand:DI 2 "register_operand" "r"))))]
  "TARGET_MULDIV && TARGET_64BIT"
  "mulw\t%0,%1,%2"
  [(set_attr "type" "imul")
   (set_attr "mode" "SI")])

(define_insn "*muldi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(mult:DI (match_operand:DI 1 "register_operand" "r")
		  (match_operand:DI 2 "register_operand" "r")))]
  "TARGET_MULDIV && TARGET_64BIT"
  "mul\t%0,%1,%2"
  [(set_attr "type" "imul")
   (set_attr "mode" "DI")])

;;
;;  ........................
;;
;;	MULTIPLICATION HIGH-PART
;;
;;  ........................
;;


;; Using a clobber here is ghetto, but I'm not smart enough to do better. '
(define_insn_and_split "<u>mulditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
	(mult:TI (any_extend:TI
		   (match_operand:DI 1 "register_operand" "r"))
		 (any_extend:TI
		   (match_operand:DI 2 "register_operand" "r"))))
  (clobber (match_scratch:DI 3 "=r"))]
  "TARGET_MULDIV && TARGET_64BIT"
  "#"
  "reload_completed"
  [
   (set (match_dup 3) (mult:DI (match_dup 1) (match_dup 2)))
   (set (match_dup 4) (truncate:DI
			(lshiftrt:TI
			  (mult:TI (any_extend:TI (match_dup 1))
				   (any_extend:TI (match_dup 2)))
			  (const_int 64))))
   (set (match_dup 5) (match_dup 3))
  ]
{
  operands[4] = riscv_subword (operands[0], true);
  operands[5] = riscv_subword (operands[0], false);
}
  )

(define_insn "<u>muldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(truncate:DI
	  (lshiftrt:TI
	    (mult:TI (any_extend:TI
		       (match_operand:DI 1 "register_operand" "r"))
		     (any_extend:TI
		       (match_operand:DI 2 "register_operand" "r")))
	    (const_int 64))))]
  "TARGET_MULDIV && TARGET_64BIT"
  "mulh<u>\t%0,%1,%2"
  [(set_attr "type" "imul")
   (set_attr "mode" "DI")])


(define_insn_and_split "usmulditi3"
  [(set (match_operand:TI 0 "register_operand" "=r")
	(mult:TI (zero_extend:TI
		   (match_operand:DI 1 "register_operand" "r"))
		 (sign_extend:TI
		   (match_operand:DI 2 "register_operand" "r"))))
  (clobber (match_scratch:DI 3 "=r"))]
  "TARGET_MULDIV && TARGET_64BIT"
  "#"
  "reload_completed"
  [
   (set (match_dup 3) (mult:DI (match_dup 1) (match_dup 2)))
   (set (match_dup 4) (truncate:DI
			(lshiftrt:TI
			  (mult:TI (zero_extend:TI (match_dup 1))
				   (sign_extend:TI (match_dup 2)))
			  (const_int 64))))
   (set (match_dup 5) (match_dup 3))
  ]
{
  operands[4] = riscv_subword (operands[0], true);
  operands[5] = riscv_subword (operands[0], false);
}
  )

(define_insn "usmuldi3_highpart"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(truncate:DI
	  (lshiftrt:TI
	    (mult:TI (zero_extend:TI
		       (match_operand:DI 1 "register_operand" "r"))
		     (sign_extend:TI
		       (match_operand:DI 2 "register_operand" "r")))
	    (const_int 64))))]
  "TARGET_MULDIV && TARGET_64BIT"
  "mulhsu\t%0,%2,%1"
  [(set_attr "type" "imul")
   (set_attr "mode" "DI")])

(define_expand "<u>mulsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(mult:DI (any_extend:DI
		   (match_operand:SI 1 "register_operand" "r"))
		 (any_extend:DI
		   (match_operand:SI 2 "register_operand" "r"))))
  (clobber (match_scratch:SI 3 "=r"))]
  "TARGET_MULDIV && !TARGET_64BIT"
{
  rtx temp = gen_reg_rtx (SImode);
  emit_insn (gen_mulsi3 (temp, operands[1], operands[2]));
  emit_insn (gen_<u>mulsi3_highpart (riscv_subword (operands[0], true),
				     operands[1], operands[2]));
  emit_insn (gen_movsi (riscv_subword (operands[0], false), temp));
  DONE;
}
  )

(define_insn "<u>mulsi3_highpart"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(truncate:SI
	  (lshiftrt:DI
	    (mult:DI (any_extend:DI
		       (match_operand:SI 1 "register_operand" "r"))
		     (any_extend:DI
		       (match_operand:SI 2 "register_operand" "r")))
	    (const_int 32))))]
  "TARGET_MULDIV && !TARGET_64BIT"
  "mulh<u>\t%0,%1,%2"
  [(set_attr "type" "imul")
   (set_attr "mode" "SI")])


(define_expand "usmulsidi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(mult:DI (zero_extend:DI
		   (match_operand:SI 1 "register_operand" "r"))
		 (sign_extend:DI
		   (match_operand:SI 2 "register_operand" "r"))))
  (clobber (match_scratch:SI 3 "=r"))]
  "TARGET_MULDIV && !TARGET_64BIT"
{
  rtx temp = gen_reg_rtx (SImode);
  emit_insn (gen_mulsi3 (temp, operands[1], operands[2]));
  emit_insn (gen_usmulsi3_highpart (riscv_subword (operands[0], true),
				     operands[1], operands[2]));
  emit_insn (gen_movsi (riscv_subword (operands[0], false), temp));
  DONE;
}
  )

(define_insn "usmulsi3_highpart"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(truncate:SI
	  (lshiftrt:DI
	    (mult:DI (zero_extend:DI
		       (match_operand:SI 1 "register_operand" "r"))
		     (sign_extend:DI
		       (match_operand:SI 2 "register_operand" "r")))
	    (const_int 32))))]
  "TARGET_MULDIV && !TARGET_64BIT"
  "mulhsu\t%0,%2,%1"
  [(set_attr "type" "imul")
   (set_attr "mode" "SI")])

;;
;;  ....................
;;
;;	DIVISION and REMAINDER
;;
;;  ....................
;;

(define_insn "<u>divsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(any_div:SI (match_operand:SI 1 "register_operand" "r")
		  (match_operand:SI 2 "register_operand" "r")))]
  "TARGET_MULDIV"
  { return TARGET_64BIT ? "div<u>w\t%0,%1,%2" : "div<u>\t%0,%1,%2"; }
  [(set_attr "type" "idiv")
   (set_attr "mode" "SI")])

(define_insn "<u>divdi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(any_div:DI (match_operand:DI 1 "register_operand" "r")
		  (match_operand:DI 2 "register_operand" "r")))]
  "TARGET_MULDIV && TARGET_64BIT"
  "div<u>\t%0,%1,%2"
  [(set_attr "type" "idiv")
   (set_attr "mode" "DI")])

(define_insn "<u>modsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(any_mod:SI (match_operand:SI 1 "register_operand" "r")
		  (match_operand:SI 2 "register_operand" "r")))]
  "TARGET_MULDIV"
  { return TARGET_64BIT ? "rem<u>w\t%0,%1,%2" : "rem<u>\t%0,%1,%2"; }
  [(set_attr "type" "idiv")
   (set_attr "mode" "SI")])

(define_insn "<u>moddi3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(any_mod:DI (match_operand:DI 1 "register_operand" "r")
		  (match_operand:DI 2 "register_operand" "r")))]
  "TARGET_MULDIV && TARGET_64BIT"
  "rem<u>\t%0,%1,%2"
  [(set_attr "type" "idiv")
   (set_attr "mode" "DI")])

(define_insn "div<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(div:ANYF (match_operand:ANYF 1 "register_operand" "f")
		  (match_operand:ANYF 2 "register_operand" "f")))]
  "TARGET_HARD_FLOAT && TARGET_FDIV"
  "fdiv.<fmt>\t%0,%1,%2"
  [(set_attr "type" "fdiv")
   (set_attr "mode" "<UNITMODE>")])

;;
;;  ....................
;;
;;	SQUARE ROOT
;;
;;  ....................

(define_insn "sqrt<mode>2"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(sqrt:ANYF (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT && TARGET_FDIV"
{
    return "fsqrt.<fmt>\t%0,%1";
}
  [(set_attr "type" "fsqrt")
   (set_attr "mode" "<UNITMODE>")])

;; Floating point multiply accumulate instructions.

(define_insn "fma<mode>4"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
    (fma:ANYF
      (match_operand:ANYF 1 "register_operand" "f")
      (match_operand:ANYF 2 "register_operand" "f")
      (match_operand:ANYF 3 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fmadd.<fmt>\t%0,%1,%2,%3"
  [(set_attr "type" "fmadd")
   (set_attr "mode" "<UNITMODE>")])

(define_insn "fms<mode>4"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
    (fma:ANYF
      (match_operand:ANYF 1 "register_operand" "f")
      (match_operand:ANYF 2 "register_operand" "f")
      (neg:ANYF (match_operand:ANYF 3 "register_operand" "f"))))]
  "TARGET_HARD_FLOAT"
  "fmsub.<fmt>\t%0,%1,%2,%3"
  [(set_attr "type" "fmadd")
   (set_attr "mode" "<UNITMODE>")])

(define_insn "nfma<mode>4"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
    (neg:ANYF
      (fma:ANYF
        (match_operand:ANYF 1 "register_operand" "f")
        (match_operand:ANYF 2 "register_operand" "f")
        (match_operand:ANYF 3 "register_operand" "f"))))]
  "TARGET_HARD_FLOAT"
  "fnmadd.<fmt>\t%0,%1,%2,%3"
  [(set_attr "type" "fmadd")
   (set_attr "mode" "<UNITMODE>")])

(define_insn "nfms<mode>4"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
    (neg:ANYF
      (fma:ANYF
        (match_operand:ANYF 1 "register_operand" "f")
        (match_operand:ANYF 2 "register_operand" "f")
        (neg:ANYF (match_operand:ANYF 3 "register_operand" "f")))))]
  "TARGET_HARD_FLOAT"
  "fnmsub.<fmt>\t%0,%1,%2,%3"
  [(set_attr "type" "fmadd")
   (set_attr "mode" "<UNITMODE>")])

;; modulo signed zeros, -(a*b+c) == -c-a*b
(define_insn "*nfma<mode>4_fastmath"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
    (minus:ANYF
      (match_operand:ANYF 3 "register_operand" "f")
      (mult:ANYF
        (neg:ANYF (match_operand:ANYF 1 "register_operand" "f"))
        (match_operand:ANYF 2 "register_operand" "f"))))]
  "TARGET_HARD_FLOAT && !HONOR_SIGNED_ZEROS (<MODE>mode)"
  "fnmadd.<fmt>\t%0,%1,%2,%3"
  [(set_attr "type" "fmadd")
   (set_attr "mode" "<UNITMODE>")])

;; modulo signed zeros, -(a*b-c) == c-a*b
(define_insn "*nfms<mode>4_fastmath"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
    (minus:ANYF
      (match_operand:ANYF 3 "register_operand" "f")
      (mult:ANYF
        (match_operand:ANYF 1 "register_operand" "f")
        (match_operand:ANYF 2 "register_operand" "f"))))]
  "TARGET_HARD_FLOAT && !HONOR_SIGNED_ZEROS (<MODE>mode)"
  "fnmsub.<fmt>\t%0,%1,%2,%3"
  [(set_attr "type" "fmadd")
   (set_attr "mode" "<UNITMODE>")])

;;
;;  ....................
;;
;;	ABSOLUTE VALUE
;;
;;  ....................

(define_insn "abs<mode>2"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(abs:ANYF (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fabs.<fmt>\t%0,%1"
  [(set_attr "type" "fmove")
   (set_attr "mode" "<UNITMODE>")])


;;
;;  ....................
;;
;;	MIN/MAX
;;
;;  ....................

(define_insn "smin<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
		   (smin:ANYF (match_operand:ANYF 1 "register_operand" "f")
			    (match_operand:ANYF 2 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fmin.<fmt>\t%0,%1,%2"
  [(set_attr "type" "fmove")
   (set_attr "mode" "<UNITMODE>")])

(define_insn "smax<mode>3"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
		   (smax:ANYF (match_operand:ANYF 1 "register_operand" "f")
			    (match_operand:ANYF 2 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fmax.<fmt>\t%0,%1,%2"
  [(set_attr "type" "fmove")
   (set_attr "mode" "<UNITMODE>")])


;;
;;  ....................
;;
;;	NEGATION and ONE'S COMPLEMENT '
;;
;;  ....................

(define_insn "neg<mode>2"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
	(neg:ANYF (match_operand:ANYF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fneg.<fmt>\t%0,%1"
  [(set_attr "type" "fmove")
   (set_attr "mode" "<UNITMODE>")])

(define_insn "one_cmpl<mode>2"
  [(set (match_operand:GPR 0 "register_operand" "=r")
	(not:GPR (match_operand:GPR 1 "register_operand" "r")))]
  ""
  "not\t%0,%1"
  [(set_attr "type" "logical")
   (set_attr "mode" "<MODE>")])

;;
;;  ....................
;;
;;	LOGICAL
;;
;;  ....................
;;

(define_insn "and<mode>3"
  [(set (match_operand:GPR 0 "register_operand" "=r,r")
	(and:GPR (match_operand:GPR 1 "register_operand" "%r,r")
		 (match_operand:GPR 2 "arith_operand" "r,Q")))]
  ""
  "and\t%0,%1,%2"
  [(set_attr "type" "logical")
   (set_attr "mode" "<MODE>")])

(define_insn "ior<mode>3"
  [(set (match_operand:GPR 0 "register_operand" "=r,r")
	(ior:GPR (match_operand:GPR 1 "register_operand" "%r,r")
		 (match_operand:GPR 2 "arith_operand" "r,Q")))]
  ""
  "or\t%0,%1,%2"
  [(set_attr "type" "logical")
   (set_attr "mode" "<MODE>")])

(define_insn "xor<mode>3"
  [(set (match_operand:GPR 0 "register_operand" "=r,r")
	(xor:GPR (match_operand:GPR 1 "register_operand" "%r,r")
		 (match_operand:GPR 2 "arith_operand" "r,Q")))]
  ""
  "xor\t%0,%1,%2"
  [(set_attr "type" "logical")
   (set_attr "mode" "<MODE>")])

;;
;;  ....................
;;
;;	TRUNCATION
;;
;;  ....................

(define_insn "truncdfsf2"
  [(set (match_operand:SF 0 "register_operand" "=f")
	(float_truncate:SF (match_operand:DF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvt.s.d\t%0,%1"
  [(set_attr "type"	"fcvt")
   (set_attr "cnv_mode"	"D2S")   
   (set_attr "mode"	"SF")])

;; Integer truncation patterns.  Truncating to HImode/QImode is a no-op.
;; Truncating from DImode to SImode is not, because we always keep SImode
;; values sign-extended in a register so we can safely use DImode branches
;; and comparisons on SImode values.

(define_insn "truncdisi2"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=r,m")
        (truncate:SI (match_operand:DI 1 "register_operand" "r,r")))]
  "TARGET_64BIT"
  "@
    sext.w\t%0,%1
    sw\t%1,%0"
  [(set_attr "move_type" "arith,store")
   (set_attr "mode" "SI")])

;; Combiner patterns to optimize shift/truncate combinations.

(define_insn "*ashr_trunc<mode>"
  [(set (match_operand:SUBDI 0 "register_operand" "=r")
        (truncate:SUBDI
	  (ashiftrt:DI (match_operand:DI 1 "register_operand" "r")
		       (match_operand:DI 2 "const_arith_operand" ""))))]
  "TARGET_64BIT && IN_RANGE (INTVAL (operands[2]), 32, 63)"
  "sra\t%0,%1,%2"
  [(set_attr "type" "shift")
   (set_attr "mode" "<MODE>")])

(define_insn "*lshr32_trunc<mode>"
  [(set (match_operand:SUBDI 0 "register_operand" "=r")
        (truncate:SUBDI
	  (lshiftrt:DI (match_operand:DI 1 "register_operand" "r")
		       (const_int 32))))]
  "TARGET_64BIT"
  "sra\t%0,%1,32"
  [(set_attr "type" "shift")
   (set_attr "mode" "<MODE>")])

;;
;;  ....................
;;
;;	ZERO EXTENSION
;;
;;  ....................

;; Extension insns.

(define_insn_and_split "zero_extendsidi2"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (zero_extend:DI (match_operand:SI 1 "nonimmediate_operand" "r,W")))]
  "TARGET_64BIT"
  "@
   #
   lwu\t%0,%1"
  "&& reload_completed && REG_P (operands[1])"
  [(set (match_dup 0)
        (ashift:DI (match_dup 1) (const_int 32)))
   (set (match_dup 0)
        (lshiftrt:DI (match_dup 0) (const_int 32)))]
  { operands[1] = gen_lowpart (DImode, operands[1]); }
  [(set_attr "move_type" "shift_shift,load")
   (set_attr "mode" "DI")])

;; Combine is not allowed to convert this insn into a zero_extendsidi2
;; because of TRULY_NOOP_TRUNCATION.

(define_insn_and_split "*clear_upper32"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (and:DI (match_operand:DI 1 "nonimmediate_operand" "r,W")
		(const_int 4294967295)))]
  "TARGET_64BIT"
{
  if (which_alternative == 0)
    return "#";

  operands[1] = gen_lowpart (SImode, operands[1]);
  return "lwu\t%0,%1";
}
  "&& reload_completed && REG_P (operands[1])"
  [(set (match_dup 0)
        (ashift:DI (match_dup 1) (const_int 32)))
   (set (match_dup 0)
        (lshiftrt:DI (match_dup 0) (const_int 32)))]
  ""
  [(set_attr "move_type" "shift_shift,load")
   (set_attr "mode" "DI")])

(define_insn_and_split "zero_extendhi<GPR:mode>2"
  [(set (match_operand:GPR 0 "register_operand" "=r,r")
        (zero_extend:GPR (match_operand:HI 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   #
   lhu\t%0,%1"
  "&& reload_completed && REG_P (operands[1])"
  [(set (match_dup 0)
        (ashift:GPR (match_dup 1) (match_dup 2)))
   (set (match_dup 0)
        (lshiftrt:GPR (match_dup 0) (match_dup 2)))]
  {
    operands[1] = gen_lowpart (<GPR:MODE>mode, operands[1]);
    operands[2] = GEN_INT(GET_MODE_BITSIZE(<GPR:MODE>mode) - 16);
  }
  [(set_attr "move_type" "shift_shift,load")
   (set_attr "mode" "<GPR:MODE>")])

(define_insn "zero_extendqi<SUPERQI:mode>2"
  [(set (match_operand:SUPERQI 0 "register_operand" "=r,r")
        (zero_extend:SUPERQI
	     (match_operand:QI 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   and\t%0,%1,0xff
   lbu\t%0,%1"
  [(set_attr "move_type" "andi,load")
   (set_attr "mode" "<SUPERQI:MODE>")])

;;
;;  ....................
;;
;;	SIGN EXTENSION
;;
;;  ....................

;; Extension insns.
;; Those for integer source operand are ordered widest source type first.

;; When TARGET_64BIT, all SImode integer registers should already be in
;; sign-extended form (see TRULY_NOOP_TRUNCATION and truncdisi2).  We can
;; therefore get rid of register->register instructions if we constrain
;; the source to be in the same register as the destination.
;;
;; The register alternative has type "arith" so that the pre-reload
;; scheduler will treat it as a move.  This reflects what happens if
;; the register alternative needs a reload.
(define_insn_and_split "extendsidi2"
  [(set (match_operand:DI 0 "register_operand" "=r,r")
        (sign_extend:DI (match_operand:SI 1 "nonimmediate_operand" "r,m")))]
  "TARGET_64BIT"
  "@
   #
   lw\t%0,%1"
  "&& reload_completed && register_operand (operands[1], VOIDmode)"
  [(set (match_dup 0) (match_dup 1))]
{
  if (REGNO (operands[0]) == REGNO (operands[1]))
    {
      emit_note (NOTE_INSN_DELETED);
      DONE;
    }
  operands[1] = gen_rtx_REG (DImode, REGNO (operands[1]));
}
  [(set_attr "move_type" "move,load")
   (set_attr "mode" "DI")])

(define_insn_and_split "extend<SHORT:mode><SUPERQI:mode>2"
  [(set (match_operand:SUPERQI 0 "register_operand" "=r,r")
        (sign_extend:SUPERQI
	     (match_operand:SHORT 1 "nonimmediate_operand" "r,m")))]
  ""
  "@
   #
   l<SHORT:size>\t%0,%1"
  "&& reload_completed && REG_P (operands[1])"
  [(set (match_dup 0) (ashift:SI (match_dup 1) (match_dup 2)))
   (set (match_dup 0) (ashiftrt:SI (match_dup 0) (match_dup 2)))]
{
  operands[0] = gen_lowpart (SImode, operands[0]);
  operands[1] = gen_lowpart (SImode, operands[1]);
  operands[2] = GEN_INT (GET_MODE_BITSIZE (SImode)
			 - GET_MODE_BITSIZE (<SHORT:MODE>mode));
}
  [(set_attr "move_type" "shift_shift,load")
   (set_attr "mode" "SI")])

(define_insn "extendsfdf2"
  [(set (match_operand:DF 0 "register_operand" "=f")
	(float_extend:DF (match_operand:SF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvt.d.s\t%0,%1"
  [(set_attr "type"	"fcvt")
   (set_attr "cnv_mode"	"S2D")   
   (set_attr "mode"	"DF")])

;;
;;  ....................
;;
;;	CONVERSIONS
;;
;;  ....................

(define_insn "fix_truncdfsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(fix:SI (match_operand:DF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvt.w.d %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"D2I")])


(define_insn "fix_truncsfsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(fix:SI (match_operand:SF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvt.w.s %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"S2I")])


(define_insn "fix_truncdfdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(fix:DI (match_operand:DF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.l.d %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"D2I")])


(define_insn "fix_truncsfdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(fix:DI (match_operand:SF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.l.s %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"S2I")])


(define_insn "floatsidf2"
  [(set (match_operand:DF 0 "register_operand" "=f")
	(float:DF (match_operand:SI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT"
  "fcvt.d.w\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"I2D")])


(define_insn "floatdidf2"
  [(set (match_operand:DF 0 "register_operand" "=f")
	(float:DF (match_operand:DI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.d.l\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"I2D")])


(define_insn "floatsisf2"
  [(set (match_operand:SF 0 "register_operand" "=f")
	(float:SF (match_operand:SI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT"
  "fcvt.s.w\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"I2S")])


(define_insn "floatdisf2"
  [(set (match_operand:SF 0 "register_operand" "=f")
	(float:SF (match_operand:DI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.s.l\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"I2S")])


(define_insn "floatunssidf2"
  [(set (match_operand:DF 0 "register_operand" "=f")
	(unsigned_float:DF (match_operand:SI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT"
  "fcvt.d.wu\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"I2D")])


(define_insn "floatunsdidf2"
  [(set (match_operand:DF 0 "register_operand" "=f")
	(unsigned_float:DF (match_operand:DI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.d.lu\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"I2D")])


(define_insn "floatunssisf2"
  [(set (match_operand:SF 0 "register_operand" "=f")
	(unsigned_float:SF (match_operand:SI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT"
  "fcvt.s.wu\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"I2S")])


(define_insn "floatunsdisf2"
  [(set (match_operand:SF 0 "register_operand" "=f")
	(unsigned_float:SF (match_operand:DI 1 "reg_or_0_operand" "rJ")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.s.lu\t%0,%z1"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"I2S")])


(define_insn "fixuns_truncdfsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(unsigned_fix:SI (match_operand:DF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvt.wu.d %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"D2I")])


(define_insn "fixuns_truncsfsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(unsigned_fix:SI (match_operand:SF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT"
  "fcvt.wu.s %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"S2I")])


(define_insn "fixuns_truncdfdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unsigned_fix:DI (match_operand:DF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.lu.d %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"DF")
   (set_attr "cnv_mode"	"D2I")])


(define_insn "fixuns_truncsfdi2"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(unsigned_fix:DI (match_operand:SF 1 "register_operand" "f")))]
  "TARGET_HARD_FLOAT && TARGET_64BIT"
  "fcvt.lu.s %0,%1,rtz"
  [(set_attr "type"	"fcvt")
   (set_attr "mode"	"SF")
   (set_attr "cnv_mode"	"S2I")])

;;
;;  ....................
;;
;;	DATA MOVEMENT
;;
;;  ....................

;; Lower-level instructions for loading an address from the GOT.
;; We could use MEMs, but an unspec gives more optimization
;; opportunities.

(define_insn "got_load<mode>"
   [(set (match_operand:P 0 "register_operand" "=r")
       (unspec:P [(match_operand:P 1 "symbolic_operand" "")]
		 UNSPEC_LOAD_GOT))]
  "flag_pic"
  "la\t%0,%1"
   [(set_attr "got" "load")
    (set_attr "mode" "<MODE>")])

(define_insn "tls_add_tp_le<mode>"
  [(set (match_operand:P 0 "register_operand" "=r")
	(unspec:P [(match_operand:P 1 "register_operand" "r")
		   (match_operand:P 2 "register_operand" "r")
		   (match_operand:P 3 "symbolic_operand" "")]
		  UNSPEC_TLS_LE))]
  "!flag_pic || flag_pie"
  "add\t%0,%1,%2,%%tprel_add(%3)"
  [(set_attr "type" "arith")
   (set_attr "mode" "<MODE>")])

(define_insn "got_load_tls_gd<mode>"
  [(set (match_operand:P 0 "register_operand" "=r")
       (unspec:P [(match_operand:P 1 "symbolic_operand" "")]
                 UNSPEC_TLS_GD))]
  "flag_pic"
  "la.tls.gd\t%0,%1"
  [(set_attr "got" "load")
   (set_attr "mode" "<MODE>")])

(define_insn "got_load_tls_ie<mode>"
  [(set (match_operand:P 0 "register_operand" "=r")
       (unspec:P [(match_operand:P 1 "symbolic_operand" "")]
                 UNSPEC_TLS_IE))]
  "flag_pic"
  "la.tls.ie\t%0,%1"
  [(set_attr "got" "load")
   (set_attr "mode" "<MODE>")])

;; Instructions for adding the low 16 bits of an address to a register.
;; Operand 2 is the address: riscv_print_operand works out which relocation
;; should be applied.

(define_insn "*low<mode>"
  [(set (match_operand:P 0 "register_operand" "=r")
	(lo_sum:P (match_operand:P 1 "register_operand" "r")
		  (match_operand:P 2 "immediate_operand" "")))]
  ""
  "add\t%0,%1,%R2"
  [(set_attr "alu_type" "add")
   (set_attr "mode" "<MODE>")])

;; Allow combine to split complex const_int load sequences, using operand 2
;; to store the intermediate results.  See move_operand for details.
(define_split
  [(set (match_operand:GPR 0 "register_operand")
	(match_operand:GPR 1 "splittable_const_int_operand"))
   (clobber (match_operand:GPR 2 "register_operand"))]
  ""
  [(const_int 0)]
{
  riscv_move_integer (operands[2], operands[0], INTVAL (operands[1]));
  DONE;
})

;; Likewise, for symbolic operands.
(define_split
  [(set (match_operand:P 0 "register_operand")
	(match_operand:P 1))
   (clobber (match_operand:P 2 "register_operand"))]
  "riscv_split_symbol (operands[2], operands[1], MAX_MACHINE_MODE, NULL)"
  [(set (match_dup 0) (match_dup 3))]
{
  riscv_split_symbol (operands[2], operands[1],
		     MAX_MACHINE_MODE, &operands[3]);
})

;; 64-bit integer moves

;; Unlike most other insns, the move insns can't be split with '
;; different predicates, because register spilling and other parts of
;; the compiler, have memoized the insn number already.

(define_expand "movdi"
  [(set (match_operand:DI 0 "")
	(match_operand:DI 1 ""))]
  ""
{
  if (riscv_legitimize_move (DImode, operands[0], operands[1]))
    DONE;
})

(define_insn "*movdi_32bit"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=r,r,r,m,*f,*f,*r,*m")
	(match_operand:DI 1 "move_operand" "r,i,m,r,*J*r,*m,*f,*f"))]
  "!TARGET_64BIT
   && (register_operand (operands[0], DImode)
       || reg_or_0_operand (operands[1], DImode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,const,load,store,mtc,fpload,mfc,fpstore")
   (set_attr "mode" "DI")])

(define_insn "*movdi_64bit"
  [(set (match_operand:DI 0 "nonimmediate_operand" "=r,r,r,m,*f,*f,*r,*m")
	(match_operand:DI 1 "move_operand" "r,T,m,rJ,*r*J,*m,*f,*f"))]
  "TARGET_64BIT
   && (register_operand (operands[0], DImode)
       || reg_or_0_operand (operands[1], DImode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,const,load,store,mtc,fpload,mfc,fpstore")
   (set_attr "mode" "DI")])

;; 32-bit Integer moves

;; Unlike most other insns, the move insns can't be split with
;; different predicates, because register spilling and other parts of
;; the compiler, have memoized the insn number already.

(define_expand "mov<mode>"
  [(set (match_operand:IMOVE32 0 "")
	(match_operand:IMOVE32 1 ""))]
  ""
{
  if (riscv_legitimize_move (<MODE>mode, operands[0], operands[1]))
    DONE;
})

;; The difference between these two is whether or not ints are allowed
;; in FP registers (off by default, use -mdebugh to enable).

(define_insn "*mov<mode>_internal"
  [(set (match_operand:IMOVE32 0 "nonimmediate_operand" "=r,r,r,m,*f,*f,*r,*m")
	(match_operand:IMOVE32 1 "move_operand" "r,T,m,rJ,*r*J,*m,*f,*f"))]
  "(register_operand (operands[0], <MODE>mode)
    || reg_or_0_operand (operands[1], <MODE>mode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,const,load,store,mtc,fpload,mfc,fpstore")
   (set_attr "mode" "SI")])

;; 16-bit Integer moves

;; Unlike most other insns, the move insns can't be split with
;; different predicates, because register spilling and other parts of
;; the compiler, have memoized the insn number already.
;; Unsigned loads are used because LOAD_EXTEND_OP returns ZERO_EXTEND.

(define_expand "movhi"
  [(set (match_operand:HI 0 "")
	(match_operand:HI 1 ""))]
  ""
{
  if (riscv_legitimize_move (HImode, operands[0], operands[1]))
    DONE;
})

(define_insn "*movhi_internal"
  [(set (match_operand:HI 0 "nonimmediate_operand" "=r,r,r,m,*f,*r")
	(match_operand:HI 1 "move_operand"         "r,T,m,rJ,*r*J,*f"))]
  "(register_operand (operands[0], HImode)
    || reg_or_0_operand (operands[1], HImode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,const,load,store,mtc,mfc")
   (set_attr "mode" "HI")])

;; HImode constant generation; see riscv_move_integer for details.
;; si+si->hi without truncation is legal because of TRULY_NOOP_TRUNCATION.

(define_insn "add<mode>hi3"
  [(set (match_operand:HI 0 "register_operand" "=r,r")
	(plus:HI (match_operand:HISI 1 "register_operand" "r,r")
		  (match_operand:HISI 2 "arith_operand" "r,Q")))]
  ""
  { return TARGET_64BIT ? "addw\t%0,%1,%2" : "add\t%0,%1,%2"; }
  [(set_attr "type" "arith")
   (set_attr "mode" "HI")])

(define_insn "xor<mode>hi3"
  [(set (match_operand:HI 0 "register_operand" "=r,r")
	(xor:HI (match_operand:HISI 1 "register_operand" "r,r")
		  (match_operand:HISI 2 "arith_operand" "r,Q")))]
  ""
  "xor\t%0,%1,%2"
  [(set_attr "type" "logical")
   (set_attr "mode" "HI")])

;; 8-bit Integer moves

(define_expand "movqi"
  [(set (match_operand:QI 0 "")
	(match_operand:QI 1 ""))]
  ""
{
  if (riscv_legitimize_move (QImode, operands[0], operands[1]))
    DONE;
})

(define_insn "*movqi_internal"
  [(set (match_operand:QI 0 "nonimmediate_operand" "=r,r,r,m,*f,*r")
	(match_operand:QI 1 "move_operand"         "r,I,m,rJ,*r*J,*f"))]
  "(register_operand (operands[0], QImode)
    || reg_or_0_operand (operands[1], QImode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,const,load,store,mtc,mfc")
   (set_attr "mode" "QI")])

;; 32-bit floating point moves

(define_expand "movsf"
  [(set (match_operand:SF 0 "")
	(match_operand:SF 1 ""))]
  ""
{
  if (riscv_legitimize_move (SFmode, operands[0], operands[1]))
    DONE;
})

(define_insn "*movsf_hardfloat"
  [(set (match_operand:SF 0 "nonimmediate_operand" "=f,f,f,m,m,*f,*r,*r,*r,*m")
	(match_operand:SF 1 "move_operand" "f,G,m,f,G,*r,*f,*G*r,*m,*r"))]
  "TARGET_HARD_FLOAT
   && (register_operand (operands[0], SFmode)
       || reg_or_0_operand (operands[1], SFmode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "fmove,mtc,fpload,fpstore,store,mtc,mfc,move,load,store")
   (set_attr "mode" "SF")])

(define_insn "*movsf_softfloat"
  [(set (match_operand:SF 0 "nonimmediate_operand" "=r,r,m")
	(match_operand:SF 1 "move_operand" "Gr,m,r"))]
  "TARGET_SOFT_FLOAT
   && (register_operand (operands[0], SFmode)
       || reg_or_0_operand (operands[1], SFmode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,load,store")
   (set_attr "mode" "SF")])

;; 64-bit floating point moves

(define_expand "movdf"
  [(set (match_operand:DF 0 "")
	(match_operand:DF 1 ""))]
  ""
{
  if (riscv_legitimize_move (DFmode, operands[0], operands[1]))
    DONE;
})

;; In RV32, we lack mtf.d/mff.d.  Go through memory instead.
;; (except for moving a constant 0 to an FPR.  for that we use fcvt.d.w.)
(define_insn "*movdf_hardfloat_rv32"
  [(set (match_operand:DF 0 "nonimmediate_operand" "=f,f,f,m,m,*r,*r,*m")
	(match_operand:DF 1 "move_operand" "f,G,m,f,G,*r*G,*m,*r"))]
  "!TARGET_64BIT && TARGET_HARD_FLOAT
   && (register_operand (operands[0], DFmode)
       || reg_or_0_operand (operands[1], DFmode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "fmove,mtc,fpload,fpstore,store,move,load,store")
   (set_attr "mode" "DF")])

(define_insn "*movdf_hardfloat_rv64"
  [(set (match_operand:DF 0 "nonimmediate_operand" "=f,f,f,m,m,*f,*r,*r,*r,*m")
	(match_operand:DF 1 "move_operand" "f,G,m,f,G,*r,*f,*r*G,*m,*r"))]
  "TARGET_64BIT && TARGET_HARD_FLOAT
   && (register_operand (operands[0], DFmode)
       || reg_or_0_operand (operands[1], DFmode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "fmove,mtc,fpload,fpstore,store,mtc,mfc,move,load,store")
   (set_attr "mode" "DF")])

(define_insn "*movdf_softfloat"
  [(set (match_operand:DF 0 "nonimmediate_operand" "=r,r,m")
	(match_operand:DF 1 "move_operand" "rG,m,rG"))]
  "TARGET_SOFT_FLOAT
   && (register_operand (operands[0], DFmode)
       || reg_or_0_operand (operands[1], DFmode))"
  { return riscv_output_move (operands[0], operands[1]); }
  [(set_attr "move_type" "move,load,store")
   (set_attr "mode" "DF")])

;; 128-bit integer moves

(define_expand "movti"
  [(set (match_operand:TI 0)
	(match_operand:TI 1))]
  "TARGET_64BIT"
{
  if (riscv_legitimize_move (TImode, operands[0], operands[1]))
    DONE;
})

(define_insn "*movti"
  [(set (match_operand:TI 0 "nonimmediate_operand" "=r,r,r,m")
	(match_operand:TI 1 "move_operand" "r,i,m,rJ"))]
  "TARGET_64BIT
   && (register_operand (operands[0], TImode)
       || reg_or_0_operand (operands[1], TImode))"
  "#"
  [(set_attr "move_type" "move,const,load,store")
   (set_attr "mode" "TI")])

(define_split
  [(set (match_operand:MOVE64 0 "nonimmediate_operand")
	(match_operand:MOVE64 1 "move_operand"))]
  "reload_completed && !TARGET_64BIT
   && riscv_split_64bit_move_p (operands[0], operands[1])"
  [(const_int 0)]
{
  riscv_split_doubleword_move (operands[0], operands[1]);
  DONE;
})

(define_split
  [(set (match_operand:MOVE128 0 "nonimmediate_operand")
	(match_operand:MOVE128 1 "move_operand"))]
  "TARGET_64BIT && reload_completed"
  [(const_int 0)]
{
  riscv_split_doubleword_move (operands[0], operands[1]);
  DONE;
})

;; 64-bit paired-single floating point moves

;; Load the low word of operand 0 with operand 1.
(define_insn "load_low<mode>"
  [(set (match_operand:SPLITF 0 "register_operand" "=f,f")
	(unspec:SPLITF [(match_operand:<HALFMODE> 1 "general_operand" "rJ,m")]
		       UNSPEC_LOAD_LOW))]
  "TARGET_HARD_FLOAT"
{
  operands[0] = riscv_subword (operands[0], 0);
  return riscv_output_move (operands[0], operands[1]);
}
  [(set_attr "move_type" "mtc,fpload")
   (set_attr "mode" "<HALFMODE>")])

;; Load the high word of operand 0 from operand 1, preserving the value
;; in the low word.
(define_insn "load_high<mode>"
  [(set (match_operand:SPLITF 0 "register_operand" "=f,f")
	(unspec:SPLITF [(match_operand:<HALFMODE> 1 "general_operand" "rJ,m")
			(match_operand:SPLITF 2 "register_operand" "0,0")]
		       UNSPEC_LOAD_HIGH))]
  "TARGET_HARD_FLOAT"
{
  operands[0] = riscv_subword (operands[0], 1);
  return riscv_output_move (operands[0], operands[1]);
}
  [(set_attr "move_type" "mtc,fpload")
   (set_attr "mode" "<HALFMODE>")])

;; Store one word of operand 1 in operand 0.  Operand 2 is 1 to store the
;; high word and 0 to store the low word.
(define_insn "store_word<mode>"
  [(set (match_operand:<HALFMODE> 0 "nonimmediate_operand" "=r,m")
	(unspec:<HALFMODE> [(match_operand:SPLITF 1 "register_operand" "f,f")
			    (match_operand 2 "const_int_operand")]
			   UNSPEC_STORE_WORD))]
  "TARGET_HARD_FLOAT"
{
  operands[1] = riscv_subword (operands[1], INTVAL (operands[2]));
  return riscv_output_move (operands[0], operands[1]);
}
  [(set_attr "move_type" "mfc,fpstore")
   (set_attr "mode" "<HALFMODE>")])

;; Expand in-line code to clear the instruction cache between operand[0] and
;; operand[1].
(define_expand "clear_cache"
  [(match_operand 0 "pmode_register_operand")
   (match_operand 1 "pmode_register_operand")]
  ""
  "
{
  emit_insn(gen_fence_i());
  DONE;
}")

(define_insn "fence"
  [(unspec_volatile [(const_int 0)] UNSPEC_FENCE)]
  ""
  "%|fence%-")

(define_insn "fence_i"
  [(unspec_volatile [(const_int 0)] UNSPEC_FENCE_I)]
  ""
  "fence.i")

;; Block moves, see riscv.c for more details.
;; Argument 0 is the destination
;; Argument 1 is the source
;; Argument 2 is the length
;; Argument 3 is the alignment

(define_expand "movmemsi"
  [(parallel [(set (match_operand:BLK 0 "general_operand")
		   (match_operand:BLK 1 "general_operand"))
	      (use (match_operand:SI 2 ""))
	      (use (match_operand:SI 3 "const_int_operand"))])]
  "!TARGET_MEMCPY"
{
  if (riscv_expand_block_move (operands[0], operands[1], operands[2]))
    DONE;
  else
    FAIL;
})

;;
;;  ....................
;;
;;	SHIFTS
;;
;;  ....................

(define_insn "<optab>si3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(any_shift:SI (match_operand:SI 1 "register_operand" "r")
		       (match_operand:SI 2 "arith_operand" "rI")))]
  ""
{
  if (GET_CODE (operands[2]) == CONST_INT)
    operands[2] = GEN_INT (INTVAL (operands[2])
			   & (GET_MODE_BITSIZE (SImode) - 1));

  return TARGET_64BIT ? "<insn>w\t%0,%1,%2" : "<insn>\t%0,%1,%2";
}
  [(set_attr "type" "shift")
   (set_attr "mode" "SI")])

(define_insn "*<optab>disi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	     (any_shift:SI (truncate:SI (match_operand:DI 1 "register_operand" "r"))
		      (truncate:SI (match_operand:DI 2 "arith_operand" "rI"))))]
  "TARGET_64BIT"
  "<insn>w\t%0,%1,%2"
  [(set_attr "type" "shift")
   (set_attr "mode" "SI")])

(define_insn "*ashldi3_truncsi"
  [(set (match_operand:SI 0 "register_operand" "=r")
          (truncate:SI
	     (ashift:DI (match_operand:DI 1 "register_operand" "r")
		      (match_operand:DI 2 "const_arith_operand" "I"))))]
  "TARGET_64BIT && INTVAL (operands[2]) < 32"
  "sllw\t%0,%1,%2"
  [(set_attr "type" "shift")
   (set_attr "mode" "SI")])

(define_insn "*ashldisi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	  (ashift:SI (match_operand:GPR 1 "register_operand" "r")
		      (match_operand:GPR2 2 "arith_operand" "rI")))]
  "TARGET_64BIT && (GET_CODE (operands[2]) == CONST_INT ? INTVAL (operands[2]) < 32 : 1)"
  "sllw\t%0,%1,%2"
  [(set_attr "type" "shift")
   (set_attr "mode" "SI")])

(define_insn "<optab>di3"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(any_shift:DI (match_operand:DI 1 "register_operand" "r")
		       (match_operand:DI 2 "arith_operand" "rI")))]
  "TARGET_64BIT"
{
  if (GET_CODE (operands[2]) == CONST_INT)
    operands[2] = GEN_INT (INTVAL (operands[2])
			   & (GET_MODE_BITSIZE (DImode) - 1));

  return "<insn>\t%0,%1,%2";
}
  [(set_attr "type" "shift")
   (set_attr "mode" "DI")])

(define_insn "<optab>si3_extend"
  [(set (match_operand:DI 0 "register_operand" "=r")
	(sign_extend:DI
	   (any_shift:SI (match_operand:SI 1 "register_operand" "r")
			 (match_operand:SI 2 "arith_operand" "rI"))))]
  "TARGET_64BIT"
{
  if (GET_CODE (operands[2]) == CONST_INT)
    operands[2] = GEN_INT (INTVAL (operands[2]) & 0x1f);

  return "<insn>w\t%0,%1,%2";
}
  [(set_attr "type" "shift")
   (set_attr "mode" "SI")])

;;
;;  ....................
;;
;;	CONDITIONAL BRANCHES
;;
;;  ....................

;; Conditional branches

(define_insn "*branch_order<mode>"
  [(set (pc)
	(if_then_else
	 (match_operator 1 "order_operator"
			 [(match_operand:GPR 2 "register_operand" "r")
			  (match_operand:GPR 3 "reg_or_0_operand" "rJ")])
	 (label_ref (match_operand 0 "" ""))
	 (pc)))]
  ""
{
  if (GET_CODE (operands[3]) == CONST_INT)
    return "b%C1z\t%2,%0";
  return "b%C1\t%2,%3,%0";
}
  [(set_attr "type" "branch")
   (set_attr "mode" "none")])

;; Used to implement built-in functions.
(define_expand "condjump"
  [(set (pc)
	(if_then_else (match_operand 0)
		      (label_ref (match_operand 1))
		      (pc)))])

(define_expand "cbranch<mode>4"
  [(set (pc)
	(if_then_else (match_operator 0 "comparison_operator"
		       [(match_operand:GPR 1 "register_operand")
		        (match_operand:GPR 2 "nonmemory_operand")])
		      (label_ref (match_operand 3 ""))
		      (pc)))]
  ""
{
  riscv_expand_conditional_branch (operands);
  DONE;
})

(define_expand "cbranch<mode>4"
  [(set (pc)
	(if_then_else (match_operator 0 "comparison_operator"
		       [(match_operand:SCALARF 1 "register_operand")
		        (match_operand:SCALARF 2 "register_operand")])
		      (label_ref (match_operand 3 ""))
		      (pc)))]
  ""
{
  riscv_expand_conditional_branch (operands);
  DONE;
})

(define_insn_and_split "*branch_on_bit<GPR:mode>"
  [(set (pc)
	(if_then_else
	 (match_operator 0 "equality_operator"
	  [(zero_extract:GPR (match_operand:GPR 2 "register_operand" "r")
		 (const_int 1)
		 (match_operand 3 "const_int_operand"))
		 (const_int 0)])
	 (label_ref (match_operand 1))
	 (pc)))
   (clobber (match_scratch:GPR 4 "=&r"))]
  ""
  "#"
  "reload_completed"
  [(set (match_dup 4)
        (ashift:GPR (match_dup 2) (match_dup 3)))
   (set (pc)
	(if_then_else
	 (match_op_dup 0 [(match_dup 4) (const_int 0)])
	 (label_ref (match_operand 1))
	 (pc)))]
{
  int shift = GET_MODE_BITSIZE (<MODE>mode) - 1 - INTVAL (operands[3]);
  operands[3] = GEN_INT (shift);

  if (GET_CODE (operands[0]) == EQ)
    operands[0] = gen_rtx_GE (<MODE>mode, operands[4], const0_rtx);
  else
    operands[0] = gen_rtx_LT (<MODE>mode, operands[4], const0_rtx);
})

(define_insn_and_split "*branch_on_bit_range<GPR:mode>"
  [(set (pc)
	(if_then_else
	 (match_operator 0 "equality_operator"
	  [(zero_extract:GPR (match_operand:GPR 2 "register_operand" "r")
		 (match_operand 3 "const_int_operand")
		 (const_int 0))
		 (const_int 0)])
	 (label_ref (match_operand 1))
	 (pc)))
   (clobber (match_scratch:GPR 4 "=&r"))]
  ""
  "#"
  "reload_completed"
  [(set (match_dup 4)
        (ashift:GPR (match_dup 2) (match_dup 3)))
   (set (pc)
	(if_then_else
	 (match_op_dup 0 [(match_dup 4) (const_int 0)])
	 (label_ref (match_operand 1))
	 (pc)))]
{
  operands[3] = GEN_INT (GET_MODE_BITSIZE (<MODE>mode) - INTVAL (operands[3]));
})

;;
;;  ....................
;;
;;	SETTING A REGISTER FROM A COMPARISON
;;
;;  ....................

;; Destination is always set in SI mode.

(define_expand "cstore<mode>4"
  [(set (match_operand:SI 0 "register_operand")
	(match_operator:SI 1 "order_operator"
	 [(match_operand:GPR 2 "register_operand")
	  (match_operand:GPR 3 "nonmemory_operand")]))]
  ""
{
  riscv_expand_scc (operands);
  DONE;
})

(define_insn "cstore<mode>4"
   [(set (match_operand:SI 0 "register_operand" "=r")
        (match_operator:SI 1 "fp_order_operator"
	      [(match_operand:SCALARF 2 "register_operand" "f")
	       (match_operand:SCALARF 3 "register_operand" "f")]))]
  "TARGET_HARD_FLOAT"
  "f%C1.<fmt>\t%0,%2,%3"
  [(set_attr "type" "fcmp")
   (set_attr "mode" "<UNITMODE>")])

(define_insn "*seq_zero_<GPR:mode><GPR2:mode>"
  [(set (match_operand:GPR2 0 "register_operand" "=r")
	(eq:GPR2 (match_operand:GPR 1 "register_operand" "r")
		 (const_int 0)))]
  ""
  "seqz\t%0,%1"
  [(set_attr "type" "slt")
   (set_attr "mode" "<GPR:MODE>")])

(define_insn "*sne_zero_<GPR:mode><GPR2:mode>"
  [(set (match_operand:GPR2 0 "register_operand" "=r")
	(ne:GPR2 (match_operand:GPR 1 "register_operand" "r")
		 (const_int 0)))]
  ""
  "snez\t%0,%1"
  [(set_attr "type" "slt")
   (set_attr "mode" "<GPR:MODE>")])

(define_insn "*sgt<u>_<GPR:mode><GPR2:mode>"
  [(set (match_operand:GPR2 0 "register_operand" "=r")
	(any_gt:GPR2 (match_operand:GPR 1 "register_operand" "r")
		     (match_operand:GPR 2 "reg_or_0_operand" "rJ")))]
  ""
  "slt<u>\t%0,%z2,%1"
  [(set_attr "type" "slt")
   (set_attr "mode" "<GPR:MODE>")])

(define_insn "*sge<u>_<GPR:mode><GPR2:mode>"
  [(set (match_operand:GPR2 0 "register_operand" "=r")
	(any_ge:GPR2 (match_operand:GPR 1 "register_operand" "r")
		     (const_int 1)))]
  ""
  "slt<u>\t%0,zero,%1"
  [(set_attr "type" "slt")
   (set_attr "mode" "<GPR:MODE>")])

(define_insn "*slt<u>_<GPR:mode><GPR2:mode>"
  [(set (match_operand:GPR2 0 "register_operand" "=r")
	(any_lt:GPR2 (match_operand:GPR 1 "register_operand" "r")
		     (match_operand:GPR 2 "arith_operand" "rI")))]
  ""
  "slt<u>\t%0,%1,%2"
  [(set_attr "type" "slt")
   (set_attr "mode" "<GPR:MODE>")])

(define_insn "*sle<u>_<GPR:mode><GPR2:mode>"
  [(set (match_operand:GPR2 0 "register_operand" "=r")
	(any_le:GPR2 (match_operand:GPR 1 "register_operand" "r")
		     (match_operand:GPR 2 "sle_operand" "")))]
  ""
{
  operands[2] = GEN_INT (INTVAL (operands[2]) + 1);
  return "slt<u>\t%0,%1,%2";
}
  [(set_attr "type" "slt")
   (set_attr "mode" "<GPR:MODE>")])

;;
;;  ....................
;;
;;	UNCONDITIONAL BRANCHES
;;
;;  ....................

;; Unconditional branches.

(define_insn "jump"
  [(set (pc)
	(label_ref (match_operand 0 "" "")))]
  ""
  "j\t%l0"
  [(set_attr "type"	"jump")
   (set_attr "mode"	"none")])

(define_expand "indirect_jump"
  [(set (pc) (match_operand 0 "register_operand"))]
  ""
{
  operands[0] = force_reg (Pmode, operands[0]);
  if (Pmode == SImode)
    emit_jump_insn (gen_indirect_jumpsi (operands[0]));
  else
    emit_jump_insn (gen_indirect_jumpdi (operands[0]));
  DONE;
})

(define_insn "indirect_jump<mode>"
  [(set (pc) (match_operand:P 0 "register_operand" "r"))]
  ""
  "jr\t%0"
  [(set_attr "type" "jump")
   (set_attr "mode" "none")])

(define_expand "tablejump"
  [(set (pc) (match_operand 0 "register_operand" ""))
	      (use (label_ref (match_operand 1 "" "")))]
  ""
{
  if (CASE_VECTOR_PC_RELATIVE)
      operands[0] = expand_simple_binop (Pmode, PLUS, operands[0],
					 gen_rtx_LABEL_REF (Pmode, operands[1]),
					 NULL_RTX, 0, OPTAB_DIRECT);

  if (CASE_VECTOR_PC_RELATIVE && Pmode == DImode)
    emit_jump_insn (gen_tablejumpdi (operands[0], operands[1]));
  else
    emit_jump_insn (gen_tablejumpsi (operands[0], operands[1]));
  DONE;
})

(define_insn "tablejump<mode>"
  [(set (pc) (match_operand:GPR 0 "register_operand" "r"))
   (use (label_ref (match_operand 1 "" "")))]
  ""
  "jr\t%0"
  [(set_attr "type" "jump")
   (set_attr "mode" "none")])

;;
;;  ....................
;;
;;	Function prologue/epilogue
;;
;;  ....................
;;

(define_expand "prologue"
  [(const_int 1)]
  ""
{
  riscv_expand_prologue ();
  DONE;
})

;; Block any insns from being moved before this point, since the
;; profiling call to mcount can use various registers that aren't
;; saved or used to pass arguments.

(define_insn "blockage"
  [(unspec_volatile [(const_int 0)] UNSPEC_BLOCKAGE)]
  ""
  ""
  [(set_attr "type" "ghost")
   (set_attr "mode" "none")])

(define_expand "epilogue"
  [(const_int 2)]
  ""
{
  riscv_expand_epilogue (false);
  DONE;
})

(define_expand "sibcall_epilogue"
  [(const_int 2)]
  ""
{
  riscv_expand_epilogue (true);
  DONE;
})

;; Trivial return.  Make it look like a normal return insn as that
;; allows jump optimizations to work better.

(define_expand "return"
  [(simple_return)]
  "riscv_can_use_return_insn ()"
  "")

(define_insn "simple_return"
  [(simple_return)]
  ""
  "ret"
  [(set_attr "type"	"jump")
   (set_attr "mode"	"none")])

;; Normal return.

(define_insn "simple_return_internal"
  [(simple_return)
   (use (match_operand 0 "pmode_register_operand" ""))]
  ""
  "jr\t%0"
  [(set_attr "type"	"jump")
   (set_attr "mode"	"none")])

;; This is used in compiling the unwind routines.
(define_expand "eh_return"
  [(use (match_operand 0 "general_operand"))]
  ""
{
  if (GET_MODE (operands[0]) != word_mode)
    operands[0] = convert_to_mode (word_mode, operands[0], 0);
  if (TARGET_64BIT)
    emit_insn (gen_eh_set_lr_di (operands[0]));
  else
    emit_insn (gen_eh_set_lr_si (operands[0]));
  DONE;
})

;; Clobber the return address on the stack.  We can't expand this
;; until we know where it will be put in the stack frame.

(define_insn "eh_set_lr_si"
  [(unspec [(match_operand:SI 0 "register_operand" "r")] UNSPEC_EH_RETURN)
   (clobber (match_scratch:SI 1 "=&r"))]
  "! TARGET_64BIT"
  "#")

(define_insn "eh_set_lr_di"
  [(unspec [(match_operand:DI 0 "register_operand" "r")] UNSPEC_EH_RETURN)
   (clobber (match_scratch:DI 1 "=&r"))]
  "TARGET_64BIT"
  "#")

(define_split
  [(unspec [(match_operand 0 "register_operand")] UNSPEC_EH_RETURN)
   (clobber (match_scratch 1))]
  "reload_completed"
  [(const_int 0)]
{
  riscv_set_return_address (operands[0], operands[1]);
  DONE;
})

;;
;;  ....................
;;
;;	FUNCTION CALLS
;;
;;  ....................

;; Sibling calls.  All these patterns use jump instructions.

;; call_insn_operand will only accept constant
;; addresses if a direct jump is acceptable.  Since the 'S' constraint
;; is defined in terms of call_insn_operand, the same is true of the
;; constraints.

;; When we use an indirect jump, we need a register that will be
;; preserved by the epilogue (constraint j).

(define_expand "sibcall"
  [(parallel [(call (match_operand 0 "")
		    (match_operand 1 ""))
	      (use (match_operand 2 ""))	;; next_arg_reg
	      (use (match_operand 3 ""))])]	;; struct_value_size_rtx
  ""
{
  riscv_expand_call (true, NULL_RTX, XEXP (operands[0], 0), operands[1]);
  DONE;
})

(define_insn "sibcall_internal"
  [(call (mem:SI (match_operand 0 "call_insn_operand" "j,S"))
	 (match_operand 1 "" ""))]
  "SIBLING_CALL_P (insn)"
  { return REG_P (operands[0]) ? "jr\t%0"
	   : absolute_symbolic_operand (operands[0], VOIDmode) ? "tail\t%0"
	   : "tail\t%0@"; }
  [(set_attr "type" "call")])

(define_expand "sibcall_value"
  [(parallel [(set (match_operand 0 "")
		   (call (match_operand 1 "")
			 (match_operand 2 "")))
	      (use (match_operand 3 ""))])]		;; next_arg_reg
  ""
{
  riscv_expand_call (true, operands[0], XEXP (operands[1], 0), operands[2]);
  DONE;
})

(define_insn "sibcall_value_internal"
  [(set (match_operand 0 "register_operand" "")
        (call (mem:SI (match_operand 1 "call_insn_operand" "j,S"))
              (match_operand 2 "" "")))]
  "SIBLING_CALL_P (insn)"
  { return REG_P (operands[1]) ? "jr\t%1"
	   : absolute_symbolic_operand (operands[1], VOIDmode) ? "tail\t%1"
	   : "tail\t%1@"; }
  [(set_attr "type" "call")])

(define_insn "sibcall_value_multiple_internal"
  [(set (match_operand 0 "register_operand" "")
        (call (mem:SI (match_operand 1 "call_insn_operand" "j,S"))
              (match_operand 2 "" "")))
   (set (match_operand 3 "register_operand" "")
	(call (mem:SI (match_dup 1))
	      (match_dup 2)))
   (clobber (match_scratch:SI 4 "=j,j"))]
  "SIBLING_CALL_P (insn)"
  { return REG_P (operands[1]) ? "jr\t%1"
	   : absolute_symbolic_operand (operands[1], VOIDmode) ? "tail\t%1"
	   : "tail\t%1@"; }
  [(set_attr "type" "call")])

(define_expand "call"
  [(parallel [(call (match_operand 0 "")
		    (match_operand 1 ""))
	      (use (match_operand 2 ""))	;; next_arg_reg
	      (use (match_operand 3 ""))])]	;; struct_value_size_rtx
  ""
{
  riscv_expand_call (false, NULL_RTX, XEXP (operands[0], 0), operands[1]);
  DONE;
})

(define_insn "call_internal"
  [(call (mem:SI (match_operand 0 "call_insn_operand" "r,S"))
	 (match_operand 1 "" ""))
   (clobber (reg:SI RETURN_ADDR_REGNUM))]
  ""
  { return REG_P (operands[0]) ? "jalr\t%0"
	   : absolute_symbolic_operand (operands[0], VOIDmode) ? "call\t%0"
	   : "call\t%0@"; }
  [(set_attr "jal" "indirect,direct")])

(define_expand "call_value"
  [(parallel [(set (match_operand 0 "")
		   (call (match_operand 1 "")
			 (match_operand 2 "")))
	      (use (match_operand 3 ""))])]		;; next_arg_reg
  ""
{
  riscv_expand_call (false, operands[0], XEXP (operands[1], 0), operands[2]);
  DONE;
})

;; See comment for call_internal.
(define_insn "call_value_internal"
  [(set (match_operand 0 "register_operand" "")
        (call (mem:SI (match_operand 1 "call_insn_operand" "r,S"))
              (match_operand 2 "" "")))
   (clobber (reg:SI RETURN_ADDR_REGNUM))]
  ""
  { return REG_P (operands[1]) ? "jalr\t%1"
	   : absolute_symbolic_operand (operands[1], VOIDmode) ? "call\t%1"
	   : "call\t%1@"; }
  [(set_attr "jal" "indirect,direct")])

;; See comment for call_internal.
(define_insn "call_value_multiple_internal"
  [(set (match_operand 0 "register_operand" "")
        (call (mem:SI (match_operand 1 "call_insn_operand" "r,S"))
              (match_operand 2 "" "")))
   (set (match_operand 3 "register_operand" "")
	(call (mem:SI (match_dup 1))
	      (match_dup 2)))
   (clobber (reg:SI RETURN_ADDR_REGNUM))]
  ""
  { return REG_P (operands[1]) ? "jalr\t%1"
	   : absolute_symbolic_operand (operands[1], VOIDmode) ? "call\t%1"
	   : "call\t%1@"; }
  [(set_attr "jal" "indirect,direct")])

;; Call subroutine returning any type.

(define_expand "untyped_call"
  [(parallel [(call (match_operand 0 "")
		    (const_int 0))
	      (match_operand 1 "")
	      (match_operand 2 "")])]
  ""
{
  int i;

  emit_call_insn (GEN_CALL (operands[0], const0_rtx, NULL, const0_rtx));

  for (i = 0; i < XVECLEN (operands[2], 0); i++)
    {
      rtx set = XVECEXP (operands[2], 0, i);
      riscv_emit_move (SET_DEST (set), SET_SRC (set));
    }

  emit_insn (gen_blockage ());
  DONE;
})

(define_insn "nop"
  [(const_int 0)]
  ""
  "nop"
  [(set_attr "type"	"nop")
   (set_attr "mode"	"none")])

(define_insn "trap"
  [(trap_if (const_int 1) (const_int 0))]
  ""
  "sbreak")

(include "sync.md")
(include "peephole.md")
