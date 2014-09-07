;;........................
;; DI -> SI optimizations
;;........................

;; Simplify (int)(a + 1), etc.
(define_peephole2
  [(set (match_operand:DI 0 "register_operand")
	(match_operator:DI 4 "modular_operator"
	  [(match_operand:DI 1 "register_operand")
	   (match_operand:DI 2 "arith_operand")]))
   (set (match_operand:SI 3 "register_operand")
        (truncate:SI (match_dup 0)))]
  "TARGET_64BIT && (REGNO (operands[0]) == REGNO (operands[3]) || peep2_reg_dead_p (2, operands[0]))"
  [(set (match_dup 3)
          (truncate:SI
	     (match_op_dup:DI 4 
	       [(match_operand:DI 1 "register_operand")
		(match_operand:DI 2 "arith_operand")])))])

;; Simplify (int)a + 1, etc.
(define_peephole2
  [(set (match_operand:SI 0 "register_operand")
        (truncate:SI (match_operand:DI 1 "register_operand")))
   (set (match_operand:SI 3 "register_operand")
	(match_operator:SI 4 "modular_operator"
	  [(match_dup 0)
	   (match_operand:SI 2 "arith_operand")]))]
  "TARGET_64BIT && (REGNO (operands[0]) == REGNO (operands[3]) || peep2_reg_dead_p (2, operands[0]))"
  [(set (match_dup 3)
	(match_op_dup:SI 4 [(match_dup 1) (match_dup 2)]))])

;; Simplify -(int)a, etc.
(define_peephole2
  [(set (match_operand:SI 0 "register_operand")
        (truncate:SI (match_operand:DI 2 "register_operand")))
   (set (match_operand:SI 3 "register_operand")
	(match_operator:SI 4 "modular_operator"
	  [(match_operand:SI 1 "reg_or_0_operand")
	   (match_dup 0)]))]
  "TARGET_64BIT && (REGNO (operands[0]) == REGNO (operands[3]) || peep2_reg_dead_p (2, operands[0]))"
  [(set (match_dup 3)
	(match_op_dup:SI 4 [(match_dup 1) (match_dup 2)]))])

;; Simplify PIC loads to static variables.
(define_peephole2
  [(set (match_operand:P 0 "register_operand")
        (match_operand:P 1 "absolute_symbolic_operand"))
   (set (match_operand:ANYI 2 "register_operand")
	(mem:ANYI (match_dup 0)))]
  "(flag_pic && SYMBOL_REF_LOCAL_P (operands[1])) && (REGNO (operands[0]) == REGNO (operands[2]) || peep2_reg_dead_p (2, operands[0]))"
  [(set (match_dup 2) (mem:ANYI (match_dup 1)))])
(define_peephole2
  [(set (match_operand:P 0 "register_operand")
        (match_operand:P 1 "absolute_symbolic_operand"))
   (set (match_operand:ANYF 2 "register_operand")
	(mem:ANYF (match_dup 0)))]
  "(flag_pic && SYMBOL_REF_LOCAL_P (operands[1])) && peep2_reg_dead_p (2, operands[0])"
  [(set (match_dup 2) (mem:ANYF (match_dup 1)))
   (clobber (match_dup 0))])
(define_peephole2
  [(set (match_operand:DI 0 "register_operand")
        (match_operand:DI 1 "absolute_symbolic_operand"))
   (set (mem:ANYIF (match_dup 0))
	(match_operand:ANYIF 2 "reg_or_0_operand"))]
  "TARGET_64BIT && (flag_pic && SYMBOL_REF_LOCAL_P (operands[1])) && peep2_reg_dead_p (2, operands[0])"
  [(set (mem:ANYIF (match_dup 1)) (match_dup 2))
   (clobber (match_dup 0))])
(define_peephole2
  [(set (match_operand:SI 0 "register_operand")
        (match_operand:SI 1 "absolute_symbolic_operand"))
   (set (mem:ANYIF (match_dup 0))
	(match_operand:ANYIF 2 "register_operand"))]
  "!TARGET_64BIT && (flag_pic && SYMBOL_REF_LOCAL_P (operands[1])) && peep2_reg_dead_p (2, operands[0])"
  [(set (mem:ANYIF (match_dup 1)) (match_dup 2))
   (clobber (match_dup 0))])
(define_insn "*local_pic_load<mode>"
  [(set (match_operand:ANYI 0 "register_operand" "=r")
        (mem:ANYI (match_operand 1 "absolute_symbolic_operand" "")))]
  "flag_pic && SYMBOL_REF_LOCAL_P (operands[1])"
  "<load>\t%0,%1"
  [(set (attr "length") (const_int 8))])
(define_insn "*local_pic_load<mode>"
  [(set (match_operand:ANYF 0 "register_operand" "=f")
        (mem:ANYF (match_operand 1 "absolute_symbolic_operand" "")))
   (clobber (match_scratch:DI 2 "=&r"))]
  "flag_pic && SYMBOL_REF_LOCAL_P (operands[1])"
  "<load>\t%0,%1,%2"
  [(set (attr "length") (const_int 8))])
(define_insn "*local_pic_loadu<mode>"
  [(set (match_operand:SUPERQI 0 "register_operand" "=r")
        (zero_extend:SUPERQI (mem:SUBDI (match_operand 1 "absolute_symbolic_operand" ""))))]
  "flag_pic && SYMBOL_REF_LOCAL_P (operands[1])"
  "<load>u\t%0,%1"
  [(set (attr "length") (const_int 8))])
(define_insn "*local_pic_storedi<mode>"
  [(set (mem:ANYI (match_operand 0 "absolute_symbolic_operand" ""))
	(match_operand:ANYI 1 "reg_or_0_operand" "rJ"))
   (clobber (match_scratch:DI 2 "=&r"))]
  "TARGET_64BIT && (flag_pic && SYMBOL_REF_LOCAL_P (operands[0]))"
  "<store>\t%z1,%0,%2"
  [(set (attr "length") (const_int 8))])
(define_insn "*local_pic_storesi<mode>"
  [(set (mem:ANYI (match_operand 0 "absolute_symbolic_operand" ""))
	(match_operand:ANYI 1 "reg_or_0_operand" "rJ"))
   (clobber (match_scratch:SI 2 "=&r"))]
  "!TARGET_64BIT && (flag_pic && SYMBOL_REF_LOCAL_P (operands[0]))"
  "<store>\t%z1,%0,%2"
  [(set (attr "length") (const_int 8))])
(define_insn "*local_pic_storedi<mode>"
  [(set (mem:ANYF (match_operand 0 "absolute_symbolic_operand" ""))
	(match_operand:ANYF 1 "register_operand" "f"))
   (clobber (match_scratch:DI 2 "=&r"))]
  "TARGET_64BIT && (flag_pic && SYMBOL_REF_LOCAL_P (operands[0]))"
  "<store>\t%1,%0,%2"
  [(set (attr "length") (const_int 8))])
(define_insn "*local_pic_storesi<mode>"
  [(set (mem:ANYF (match_operand 0 "absolute_symbolic_operand" ""))
	(match_operand:ANYF 1 "register_operand" "f"))
   (clobber (match_scratch:SI 2 "=&r"))]
  "!TARGET_64BIT && (flag_pic && SYMBOL_REF_LOCAL_P (operands[0]))"
  "<store>\t%1,%0,%2"
  [(set (attr "length") (const_int 8))])
