/* Definition of RISC-V target for GNU compiler.
   Copyright (C) 2011-2014 Free Software Foundation, Inc.
   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target for GNU compiler.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_RISCV_PROTOS_H
#define GCC_RISCV_PROTOS_H

enum riscv_symbol_type {
  SYMBOL_ABSOLUTE,
  SYMBOL_GOT_DISP,
  SYMBOL_TLS,
  SYMBOL_TLS_LE,
  SYMBOL_TLS_IE,
  SYMBOL_TLS_GD
};
#define NUM_SYMBOL_TYPES (SYMBOL_TLS_GD + 1)

enum riscv_code_model {
  CM_MEDLOW,
  CM_MEDANY,
  CM_PIC
};
extern enum riscv_code_model riscv_cmodel;

extern bool riscv_symbolic_constant_p (rtx, enum riscv_symbol_type *);
extern int riscv_regno_mode_ok_for_base_p (int, enum machine_mode, bool);
extern int riscv_address_insns (rtx, enum machine_mode, bool);
extern int riscv_const_insns (rtx);
extern int riscv_split_const_insns (rtx);
extern int riscv_load_store_insns (rtx, rtx);
extern rtx riscv_emit_move (rtx, rtx);
extern bool riscv_split_symbol (rtx, rtx, enum machine_mode, rtx *);
extern rtx riscv_unspec_address (rtx, enum riscv_symbol_type);
extern void riscv_move_integer (rtx, rtx, HOST_WIDE_INT);
extern bool riscv_legitimize_move (enum machine_mode, rtx, rtx);
extern bool riscv_legitimize_vector_move (enum machine_mode, rtx, rtx);

extern rtx riscv_subword (rtx, bool);
extern bool riscv_split_64bit_move_p (rtx, rtx);
extern void riscv_split_doubleword_move (rtx, rtx);
extern const char *riscv_output_move (rtx, rtx);
extern const char *riscv_riscv_output_vector_move (enum machine_mode, rtx, rtx);
#ifdef RTX_CODE
extern void riscv_expand_scc (rtx *);
extern void riscv_expand_conditional_branch (rtx *);
#endif
extern rtx riscv_expand_call (bool, rtx, rtx, rtx);
extern void riscv_expand_fcc_reload (rtx, rtx, rtx);
extern void riscv_set_return_address (rtx, rtx);
extern bool riscv_expand_block_move (rtx, rtx, rtx);
extern void riscv_expand_synci_loop (rtx, rtx);

extern bool riscv_expand_ext_as_unaligned_load (rtx, rtx, HOST_WIDE_INT,
					       HOST_WIDE_INT);
extern bool riscv_expand_ins_as_unaligned_store (rtx, rtx, HOST_WIDE_INT,
						HOST_WIDE_INT);
extern void riscv_order_regs_for_local_alloc (void);

extern rtx riscv_return_addr (int, rtx);
extern HOST_WIDE_INT riscv_initial_elimination_offset (int, int);
extern void riscv_expand_prologue (void);
extern void riscv_expand_epilogue (bool);
extern bool riscv_can_use_return_insn (void);
extern rtx riscv_function_value (const_tree, const_tree, enum machine_mode);

extern enum reg_class riscv_secondary_reload_class (enum reg_class,
						   enum machine_mode,
						   rtx, bool);
extern int riscv_class_max_nregs (enum reg_class, enum machine_mode);

extern unsigned int riscv_hard_regno_nregs (int, enum machine_mode);

extern void irix_asm_output_align (FILE *, unsigned);
extern const char *current_section_name (void);
extern unsigned int current_section_flags (void);

extern void riscv_expand_vector_init (rtx, rtx);

#endif /* ! GCC_RISCV_PROTOS_H */
