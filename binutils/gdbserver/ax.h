/* Data structures and functions associated with agent expressions in GDB.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_AX_H
#define GDBSERVER_AX_H

#include "regcache.h"

#ifdef IN_PROCESS_AGENT
#include "gdbsupport/agent.h"
#define debug_threads debug_agent
#endif

struct traceframe;

/* Enumeration of the different kinds of things that can happen during
   agent expression evaluation.  */

enum eval_result_type
  {
#define AX_RESULT_TYPE(ENUM,STR) ENUM,
#include "ax-result-types.def"
#undef AX_RESULT_TYPE
  };

struct agent_expr
{
  int length;

  unsigned char *bytes;
};

#ifndef IN_PROCESS_AGENT

/* The packet form of an agent expression consists of an 'X', number
   of bytes in expression, a comma, and then the bytes.  */
struct agent_expr *gdb_parse_agent_expr (const char **actparm);

/* Release an agent expression.  */
void gdb_free_agent_expr (struct agent_expr *aexpr);

/* Convert the bytes of an agent expression back into hex digits, so
   they can be printed or uploaded.  This allocates the buffer,
   callers should free when they are done with it.  */
char *gdb_unparse_agent_expr (struct agent_expr *aexpr);
void emit_prologue (void);
void emit_epilogue (void);
enum eval_result_type compile_bytecodes (struct agent_expr *aexpr);
#endif

/* The context when evaluating agent expression.  */

struct eval_agent_expr_context
{
  /* The registers when evaluating agent expression.  */
  struct regcache *regcache;
  /* The traceframe, if any, when evaluating agent expression.  */
  struct traceframe *tframe;
  /* The tracepoint, if any, when evaluating agent expression.  */
  struct tracepoint *tpoint;
};

enum eval_result_type
  gdb_eval_agent_expr (struct eval_agent_expr_context *ctx,
		       struct agent_expr *aexpr,
		       ULONGEST *rslt);

/* Bytecode compilation function vector.  */

struct emit_ops
{
  void (*emit_prologue) (void);
  void (*emit_epilogue) (void);
  void (*emit_add) (void);
  void (*emit_sub) (void);
  void (*emit_mul) (void);
  void (*emit_lsh) (void);
  void (*emit_rsh_signed) (void);
  void (*emit_rsh_unsigned) (void);
  void (*emit_ext) (int arg);
  void (*emit_log_not) (void);
  void (*emit_bit_and) (void);
  void (*emit_bit_or) (void);
  void (*emit_bit_xor) (void);
  void (*emit_bit_not) (void);
  void (*emit_equal) (void);
  void (*emit_less_signed) (void);
  void (*emit_less_unsigned) (void);
  void (*emit_ref) (int size);
  void (*emit_if_goto) (int *offset_p, int *size_p);
  void (*emit_goto) (int *offset_p, int *size_p);
  void (*write_goto_address) (CORE_ADDR from, CORE_ADDR to, int size);
  void (*emit_const) (LONGEST num);
  void (*emit_call) (CORE_ADDR fn);
  void (*emit_reg) (int reg);
  void (*emit_pop) (void);
  void (*emit_stack_flush) (void);
  void (*emit_zero_ext) (int arg);
  void (*emit_swap) (void);
  void (*emit_stack_adjust) (int n);

  /* Emit code for a generic function that takes one fixed integer
     argument and returns a 64-bit int (for instance, tsv getter).  */
  void (*emit_int_call_1) (CORE_ADDR fn, int arg1);

  /* Emit code for a generic function that takes one fixed integer
     argument and a 64-bit int from the top of the stack, and returns
     nothing (for instance, tsv setter).  */
  void (*emit_void_call_2) (CORE_ADDR fn, int arg1);

  /* Emit code specialized for common combinations of compare followed
     by a goto.  */
  void (*emit_eq_goto) (int *offset_p, int *size_p);
  void (*emit_ne_goto) (int *offset_p, int *size_p);
  void (*emit_lt_goto) (int *offset_p, int *size_p);
  void (*emit_le_goto) (int *offset_p, int *size_p);
  void (*emit_gt_goto) (int *offset_p, int *size_p);
  void (*emit_ge_goto) (int *offset_p, int *size_p);
};

extern CORE_ADDR current_insn_ptr;
extern int emit_error;

#endif /* GDBSERVER_AX_H */
