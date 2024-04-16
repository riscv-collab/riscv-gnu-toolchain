/* ginsn.h - GAS instruction representation.
   Copyright (C) 2023 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef GINSN_H
#define GINSN_H

#include "as.h"

/* Maximum number of source operands of a ginsn.  */
#define GINSN_NUM_SRC_OPNDS   2

/* A ginsn in printed in the following format:
      "ginsn: OPCD SRC1, SRC2, DST"
      "<-5->  <--------125------->"
   where each of SRC1, SRC2, and DST are in the form:
      "%rNN,"  (up to 5 chars)
      "imm,"   (up to int32_t+1 chars)
      "[%rNN+-imm]," (up to int32_t+9 chars)
      Hence a max of 19 chars.  */

#define GINSN_LISTING_OPND_LEN	40
#define GINSN_LISTING_LEN 156

enum ginsn_gen_mode
{
  GINSN_GEN_NONE,
  /* Generate ginsns for program validation passes.  */
  GINSN_GEN_FVAL,
  /* Generate ginsns for synthesizing DWARF CFI.  */
  GINSN_GEN_SCFI,
};

/* ginsn types.

   GINSN_TYPE_PHANTOM are phantom ginsns.  They are used where there is no real
   machine instruction counterpart, but a ginsn is needed only to carry
   information to GAS.  For example, to carry an SCFI Op.

   Note that, ginsns do not have a push / pop instructions.
   Instead, following are used:
      type=GINSN_TYPE_LOAD, src=GINSN_SRC_INDIRECT, REG_SP: Load from stack.
      type=GINSN_TYPE_STORE, dst=GINSN_DST_INDIRECT, REG_SP: Store to stack.
*/

#define _GINSN_TYPES \
  _GINSN_TYPE_ITEM (GINSN_TYPE_SYMBOL, "SYM") \
  _GINSN_TYPE_ITEM (GINSN_TYPE_PHANTOM, "PHANTOM")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_ADD, "ADD")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_AND, "AND")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_CALL, "CALL") \
  _GINSN_TYPE_ITEM (GINSN_TYPE_JUMP, "JMP") \
  _GINSN_TYPE_ITEM (GINSN_TYPE_JUMP_COND, "JCC")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_MOV, "MOV")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_LOAD, "LOAD")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_STORE, "STORE")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_RETURN, "RET") \
  _GINSN_TYPE_ITEM (GINSN_TYPE_SUB, "SUB")  \
  _GINSN_TYPE_ITEM (GINSN_TYPE_OTHER, "OTH")

enum ginsn_type
{
#define _GINSN_TYPE_ITEM(NAME, STR) NAME,
  _GINSN_TYPES
#undef _GINSN_TYPE_ITEM
};

enum ginsn_src_type
{
  GINSN_SRC_UNKNOWN,
  GINSN_SRC_REG,
  GINSN_SRC_IMM,
  GINSN_SRC_INDIRECT,
  GINSN_SRC_SYMBOL,
};

/* GAS instruction source operand representation.  */

struct ginsn_src
{
  enum ginsn_src_type type;
  /* DWARF register number.  */
  unsigned int reg;
  /* Immediate or disp for indirect memory access.  */
  offsetT immdisp;
  /* Src symbol.  May be needed for some control flow instructions.  */
  const symbolS *sym;
};

enum ginsn_dst_type
{
  GINSN_DST_UNKNOWN,
  GINSN_DST_REG,
  GINSN_DST_INDIRECT,
};

/* GAS instruction destination operand representation.  */

struct ginsn_dst
{
  enum ginsn_dst_type type;
  /* DWARF register number.  */
  unsigned int reg;
  /* Disp for indirect memory access.  */
  offsetT disp;
};

/* Various flags for additional information per GAS instruction.  */

/* Function begin or end symbol.  */
#define GINSN_F_FUNC_MARKER	    0x1
/* Identify real or implicit GAS insn.
   Some targets employ CISC-like instructions.  Multiple ginsn's may be used
   for a single machine instruction in some ISAs.  For some optimizations,
   there is need to identify whether a ginsn, e.g., GINSN_TYPE_ADD or
   GINSN_TYPE_SUB is a result of an user-specified instruction or not.  */
#define GINSN_F_INSN_REAL	    0x2
/* Identify if the GAS insn of type GINSN_TYPE_SYMBOL is due to a user-defined
   label.  Each user-defined labels in a function will cause addition of a new
   ginsn.  This simplifies control flow graph creation.
   See htab_t label_ginsn_map usage.  */
#define GINSN_F_USER_LABEL	    0x4
/* Max bit position for flags (uint32_t).  */
#define GINSN_F_MAX		    0x20

#define GINSN_F_FUNC_BEGIN_P(ginsn)	    \
  ((ginsn != NULL)			    \
   && (ginsn->type == GINSN_TYPE_SYMBOL)    \
   && (ginsn->flags & GINSN_F_FUNC_MARKER))

/* PS: For ginsn associated with a user-defined symbol location,
   GINSN_F_FUNC_MARKER is unset, but GINSN_F_USER_LABEL is set.  */
#define GINSN_F_FUNC_END_P(ginsn)	    \
  ((ginsn != NULL)			    \
   && (ginsn->type == GINSN_TYPE_SYMBOL)    \
   && !(ginsn->flags & GINSN_F_FUNC_MARKER) \
   && !(ginsn->flags & GINSN_F_USER_LABEL))

#define GINSN_F_INSN_REAL_P(ginsn)	    \
  ((ginsn != NULL)			    \
   && (ginsn->flags & GINSN_F_INSN_REAL))

#define GINSN_F_USER_LABEL_P(ginsn)	    \
  ((ginsn != NULL)			    \
   && (ginsn->type == GINSN_TYPE_SYMBOL)    \
   && !(ginsn->flags & GINSN_F_FUNC_MARKER) \
   && (ginsn->flags & GINSN_F_USER_LABEL))

typedef struct ginsn ginsnS;
typedef struct scfi_op scfi_opS;
typedef struct scfi_state scfi_stateS;

/* GAS generic instruction.

   Generic instructions are used by GAS to abstract out the binary machine
   instructions.  In other words, ginsn is a target/ABI independent internal
   representation for GAS.  Note that, depending on the target, there may be
   more than one ginsn per binary machine instruction.

   ginsns can be used by GAS to perform validations, or even generate
   additional information like, sythesizing DWARF CFI for hand-written asm.  */

struct ginsn
{
  enum ginsn_type type;
  /* GAS instructions are simple instructions with GINSN_NUM_SRC_OPNDS number
     of source operands and one destination operand at this time.  */
  struct ginsn_src src[GINSN_NUM_SRC_OPNDS];
  struct ginsn_dst dst;
  /* Additional information per instruction.  */
  uint32_t flags;
  /* Symbol.  For ginsn of type other than GINSN_TYPE_SYMBOL, this identifies
     the end of the corresponding machine instruction in the .text segment.
     These symbols are created anew by the targets and are not used elsewhere
     in GAS.  The only exception is some ginsns of type GINSN_TYPE_SYMBOL, when
     generated for the user-defined labels.  See ginsn_frob_label.  */
  const symbolS *sym;
  /* Identifier (linearly increasing natural number) for each ginsn.  Used as
     a proxy for program order of ginsns.  */
  uint64_t id;
  /* Location information for user-interfacing messaging.  Only ginsns with
     GINSN_F_FUNC_BEGIN_P and GINSN_F_FUNC_END_P may present themselves with no
     file or line information.  */
  const char *file;
  unsigned int line;

  /* Information needed for synthesizing CFI.  */
  scfi_opS **scfi_ops;
  uint32_t num_scfi_ops;

  /* Flag to keep track of visited instructions for CFG creation.  */
  bool visited;

  ginsnS *next; /* A linked list.  */
};

struct ginsn_src *ginsn_get_src1 (ginsnS *ginsn);
struct ginsn_src *ginsn_get_src2 (ginsnS *ginsn);
struct ginsn_dst *ginsn_get_dst (ginsnS *ginsn);

unsigned int ginsn_get_src_reg (struct ginsn_src *src);
enum ginsn_src_type ginsn_get_src_type (struct ginsn_src *src);
offsetT ginsn_get_src_disp (struct ginsn_src *src);
offsetT ginsn_get_src_imm (struct ginsn_src *src);

unsigned int ginsn_get_dst_reg (struct ginsn_dst *dst);
enum ginsn_dst_type ginsn_get_dst_type (struct ginsn_dst *dst);
offsetT ginsn_get_dst_disp (struct ginsn_dst *dst);

/* Data object for book-keeping information related to GAS generic
   instructions.  */
struct frch_ginsn_data
{
  /* Mode for GINSN creation.  */
  enum ginsn_gen_mode mode;
  /* Head of the list of ginsns.  */
  ginsnS *gins_rootP;
  /* Tail of the list of ginsns.  */
  ginsnS *gins_lastP;
  /* Function symbol.  */
  const symbolS *func;
  /* Start address of the function.  */
  symbolS *start_addr;
  /* User-defined label to ginsn mapping.  */
  htab_t label_ginsn_map;
  /* Is the list of ginsn apt for creating CFG.  */
  bool gcfg_apt_p;
};

int ginsn_data_begin (const symbolS *func);
int ginsn_data_end (const symbolS *label);
const symbolS *ginsn_data_func_symbol (void);
void ginsn_frob_label (const symbolS *sym);

void frch_ginsn_data_init (const symbolS *func, symbolS *start_addr,
			   enum ginsn_gen_mode gmode);
void frch_ginsn_data_cleanup (void);
int frch_ginsn_data_append (ginsnS *ginsn);
enum ginsn_gen_mode frch_ginsn_gen_mode (void);

void label_ginsn_map_insert (const symbolS *label, ginsnS *ginsn);
ginsnS *label_ginsn_map_find (const symbolS *label);

ginsnS *ginsn_new_symbol_func_begin (const symbolS *sym);
ginsnS *ginsn_new_symbol_func_end (const symbolS *sym);
ginsnS *ginsn_new_symbol_user_label (const symbolS *sym);

ginsnS *ginsn_new_phantom (const symbolS *sym);
ginsnS *ginsn_new_symbol (const symbolS *sym, bool real_p);
ginsnS *ginsn_new_add (const symbolS *sym, bool real_p,
		       enum ginsn_src_type src1_type, unsigned int src1_reg, offsetT src1_disp,
		       enum ginsn_src_type src2_type, unsigned int src2_reg, offsetT src2_disp,
		       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp);
ginsnS *ginsn_new_and (const symbolS *sym, bool real_p,
		       enum ginsn_src_type src1_type, unsigned int src1_reg, offsetT src1_disp,
		       enum ginsn_src_type src2_type, unsigned int src2_reg, offsetT src2_disp,
		       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp);
ginsnS *ginsn_new_call (const symbolS *sym, bool real_p,
			enum ginsn_src_type src_type, unsigned int src_reg,
			const symbolS *src_text_sym);
ginsnS *ginsn_new_jump (const symbolS *sym, bool real_p,
			enum ginsn_src_type src_type, unsigned int src_reg,
			const symbolS *src_ginsn_sym);
ginsnS *ginsn_new_jump_cond (const symbolS *sym, bool real_p,
			     enum ginsn_src_type src_type, unsigned int src_reg,
			     const symbolS *src_ginsn_sym);
ginsnS *ginsn_new_mov (const symbolS *sym, bool real_p,
		       enum ginsn_src_type src_type, unsigned int src_reg, offsetT src_disp,
		       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp);
ginsnS *ginsn_new_store (const symbolS *sym, bool real_p,
			 enum ginsn_src_type src_type, unsigned int src_reg,
			 enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp);
ginsnS *ginsn_new_load (const symbolS *sym, bool real_p,
			enum ginsn_src_type src_type, unsigned int src_reg, offsetT src_disp,
			enum ginsn_dst_type dst_type, unsigned int dst_reg);
ginsnS *ginsn_new_sub (const symbolS *sym, bool real_p,
		       enum ginsn_src_type src1_type, unsigned int src1_reg, offsetT src1_disp,
		       enum ginsn_src_type src2_type, unsigned int src2_reg, offsetT src2_disp,
		       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp);
ginsnS *ginsn_new_other (const symbolS *sym, bool real_p,
			 enum ginsn_src_type src1_type, unsigned int src1_val,
			 enum ginsn_src_type src2_type, unsigned int src2_val,
			 enum ginsn_dst_type dst_type, unsigned int dst_reg);
ginsnS *ginsn_new_return (const symbolS *sym, bool real_p);

void ginsn_set_where (ginsnS *ginsn);

bool ginsn_track_reg_p (unsigned int dw2reg, enum ginsn_gen_mode);

int ginsn_link_next (ginsnS *ginsn, ginsnS *next);

enum gcfg_err_code
{
  GCFG_OK = 0,
  GCFG_JLABEL_NOT_PRESENT = 1, /* Warning-level code.  */
};

typedef struct gbb gbbS;
typedef struct gedge gedgeS;

/* GBB - Basic block of generic GAS instructions.  */

struct gbb
{
  ginsnS *first_ginsn;
  ginsnS *last_ginsn;
  uint64_t num_ginsns;

  /* Identifier (linearly increasing natural number) for each gbb.  Added for
     debugging purpose only.  */
  uint64_t id;

  bool visited;

  uint32_t num_out_gedges;
  gedgeS *out_gedges;

  /* Members for SCFI purposes.  */
  /* SCFI state at the entry of basic block.  */
  scfi_stateS *entry_state;
  /* SCFI state at the exit of basic block.  */
  scfi_stateS *exit_state;

  /* A linked list.  In order of addition.  */
  gbbS *next;
};

struct gedge
{
  gbbS *dst_bb;
  /* A linked list.  In order of addition.  */
  gedgeS *next;
  bool visited;
};

/* Control flow graph of generic GAS instructions.  */

struct gcfg
{
  uint64_t num_gbbs;
  gbbS *root_bb;
};

typedef struct gcfg gcfgS;

#define bb_for_each_insn(bb, ginsn)  \
  for (ginsn = bb->first_ginsn; ginsn; \
       ginsn = (ginsn != bb->last_ginsn) ? ginsn->next : NULL)

#define bb_for_each_edge(bb, edge) \
  for (edge = (edge == NULL) ? bb->out_gedges : edge; edge; edge = edge->next)

#define cfg_for_each_bb(cfg, bb) \
  for (bb = cfg->root_bb; bb; bb = bb->next)

#define bb_get_first_ginsn(bb)	  \
  (bb->first_ginsn)

#define bb_get_last_ginsn(bb)	  \
  (bb->last_ginsn)

gcfgS *gcfg_build (const symbolS *func, int *errp);
void gcfg_cleanup (gcfgS **gcfg);
void gcfg_print (const gcfgS *gcfg, FILE *outfile);
gbbS *gcfg_get_rootbb (gcfgS *gcfg);
void gcfg_get_bbs_in_prog_order (gcfgS *gcfg, gbbS **prog_order_bbs);

#endif /* GINSN_H.  */
