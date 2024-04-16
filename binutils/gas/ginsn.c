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

#include "as.h"
#include "subsegs.h"
#include "ginsn.h"
#include "scfi.h"

#ifdef TARGET_USE_GINSN

static const char *const ginsn_type_names[] =
{
#define _GINSN_TYPE_ITEM(NAME, STR) STR,
  _GINSN_TYPES
#undef _GINSN_TYPE_ITEM
};

static ginsnS *
ginsn_alloc (void)
{
  ginsnS *ginsn = XCNEW (ginsnS);
  return ginsn;
}

static ginsnS *
ginsn_init (enum ginsn_type type, const symbolS *sym, bool real_p)
{
  ginsnS *ginsn = ginsn_alloc ();
  ginsn->type = type;
  ginsn->sym = sym;
  if (real_p)
    ginsn->flags |= GINSN_F_INSN_REAL;
  return ginsn;
}

static void
ginsn_cleanup (ginsnS **ginsnp)
{
  ginsnS *ginsn;

  if (!ginsnp || !*ginsnp)
    return;

  ginsn = *ginsnp;
  if (ginsn->scfi_ops)
    {
      scfi_ops_cleanup (ginsn->scfi_ops);
      ginsn->scfi_ops = NULL;
    }

  free (ginsn);
  *ginsnp = NULL;
}

static void
ginsn_set_src (struct ginsn_src *src, enum ginsn_src_type type, unsigned int reg,
	       offsetT immdisp)
{
  if (!src)
    return;

  src->type = type;
  /* Even when the use-case is SCFI, the value of reg may be > SCFI_MAX_REG_ID.
     E.g., in AMD64, push fs etc.  */
  src->reg = reg;
  src->immdisp = immdisp;
}

static void
ginsn_set_dst (struct ginsn_dst *dst, enum ginsn_dst_type type, unsigned int reg,
	       offsetT disp)
{
  if (!dst)
    return;

  dst->type = type;
  dst->reg = reg;

  if (type == GINSN_DST_INDIRECT)
    dst->disp = disp;
}

static void
ginsn_set_file_line (ginsnS *ginsn, const char *file, unsigned int line)
{
  if (!ginsn)
    return;

  ginsn->file = file;
  ginsn->line = line;
}

struct ginsn_src *
ginsn_get_src1 (ginsnS *ginsn)
{
  return &ginsn->src[0];
}

struct ginsn_src *
ginsn_get_src2 (ginsnS *ginsn)
{
  return &ginsn->src[1];
}

struct ginsn_dst *
ginsn_get_dst (ginsnS *ginsn)
{
  return &ginsn->dst;
}

unsigned int
ginsn_get_src_reg (struct ginsn_src *src)
{
  return src->reg;
}

enum ginsn_src_type
ginsn_get_src_type (struct ginsn_src *src)
{
  return src->type;
}

offsetT
ginsn_get_src_disp (struct ginsn_src *src)
{
  return src->immdisp;
}

offsetT
ginsn_get_src_imm (struct ginsn_src *src)
{
  return src->immdisp;
}

unsigned int
ginsn_get_dst_reg (struct ginsn_dst *dst)
{
  return dst->reg;
}

enum ginsn_dst_type
ginsn_get_dst_type (struct ginsn_dst *dst)
{
  return dst->type;
}

offsetT
ginsn_get_dst_disp (struct ginsn_dst *dst)
{
  return dst->disp;
}

void
label_ginsn_map_insert (const symbolS *label, ginsnS *ginsn)
{
  const char *name = S_GET_NAME (label);
  str_hash_insert (frchain_now->frch_ginsn_data->label_ginsn_map,
		   name, ginsn, 0 /* noreplace.  */);
}

ginsnS *
label_ginsn_map_find (const symbolS *label)
{
  const char *name = S_GET_NAME (label);
  ginsnS *ginsn
    = (ginsnS *) str_hash_find (frchain_now->frch_ginsn_data->label_ginsn_map,
				name);
  return ginsn;
}

ginsnS *
ginsn_new_phantom (const symbolS *sym)
{
  ginsnS *ginsn = ginsn_alloc ();
  ginsn->type = GINSN_TYPE_PHANTOM;
  ginsn->sym = sym;
  /* By default, GINSN_F_INSN_REAL is not set in ginsn->flags.  */
  return ginsn;
}

ginsnS *
ginsn_new_symbol (const symbolS *sym, bool func_begin_p)
{
  ginsnS *ginsn = ginsn_alloc ();
  ginsn->type = GINSN_TYPE_SYMBOL;
  ginsn->sym = sym;
  if (func_begin_p)
    ginsn->flags |= GINSN_F_FUNC_MARKER;
  return ginsn;
}

ginsnS *
ginsn_new_symbol_func_begin (const symbolS *sym)
{
  return ginsn_new_symbol (sym, true);
}

ginsnS *
ginsn_new_symbol_func_end (const symbolS *sym)
{
  return ginsn_new_symbol (sym, false);
}

ginsnS *
ginsn_new_symbol_user_label (const symbolS *sym)
{
  ginsnS *ginsn = ginsn_new_symbol (sym, false);
  ginsn->flags |= GINSN_F_USER_LABEL;
  return ginsn;
}

ginsnS *
ginsn_new_add (const symbolS *sym, bool real_p,
	       enum ginsn_src_type src1_type, unsigned int src1_reg, offsetT src1_disp,
	       enum ginsn_src_type src2_type, unsigned int src2_reg, offsetT src2_disp,
	       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_ADD, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src1_type, src1_reg, src1_disp);
  ginsn_set_src (&ginsn->src[1], src2_type, src2_reg, src2_disp);
  /* dst info.  */
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, dst_disp);

  return ginsn;
}

ginsnS *
ginsn_new_and (const symbolS *sym, bool real_p,
	       enum ginsn_src_type src1_type, unsigned int src1_reg, offsetT src1_disp,
	       enum ginsn_src_type src2_type, unsigned int src2_reg, offsetT src2_disp,
	       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_AND, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src1_type, src1_reg, src1_disp);
  ginsn_set_src (&ginsn->src[1], src2_type, src2_reg, src2_disp);
  /* dst info.  */
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, dst_disp);

  return ginsn;
}

ginsnS *
ginsn_new_call (const symbolS *sym, bool real_p,
		enum ginsn_src_type src_type, unsigned int src_reg,
		const symbolS *src_text_sym)

{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_CALL, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src_type, src_reg, 0);

  if (src_type == GINSN_SRC_SYMBOL)
    ginsn->src[0].sym = src_text_sym;

  return ginsn;
}

ginsnS *
ginsn_new_jump (const symbolS *sym, bool real_p,
		enum ginsn_src_type src_type, unsigned int src_reg,
		const symbolS *src_ginsn_sym)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_JUMP, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src_type, src_reg, 0);

  if (src_type == GINSN_SRC_SYMBOL)
    ginsn->src[0].sym = src_ginsn_sym;

  return ginsn;
}

ginsnS *
ginsn_new_jump_cond (const symbolS *sym, bool real_p,
		     enum ginsn_src_type src_type, unsigned int src_reg,
		     const symbolS *src_ginsn_sym)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_JUMP_COND, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src_type, src_reg, 0);

  if (src_type == GINSN_SRC_SYMBOL)
    ginsn->src[0].sym = src_ginsn_sym;

  return ginsn;
}

ginsnS *
ginsn_new_mov (const symbolS *sym, bool real_p,
	       enum ginsn_src_type src_type, unsigned int src_reg, offsetT src_disp,
	       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_MOV, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src_type, src_reg, src_disp);
  /* dst info.  */
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, dst_disp);

  return ginsn;
}

ginsnS *
ginsn_new_store (const symbolS *sym, bool real_p,
		 enum ginsn_src_type src_type, unsigned int src_reg,
		 enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_STORE, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src_type, src_reg, 0);
  /* dst info.  */
  gas_assert (dst_type == GINSN_DST_INDIRECT);
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, dst_disp);

  return ginsn;
}

ginsnS *
ginsn_new_load (const symbolS *sym, bool real_p,
		enum ginsn_src_type src_type, unsigned int src_reg, offsetT src_disp,
		enum ginsn_dst_type dst_type, unsigned int dst_reg)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_LOAD, sym, real_p);
  /* src info.  */
  gas_assert (src_type == GINSN_SRC_INDIRECT);
  ginsn_set_src (&ginsn->src[0], src_type, src_reg, src_disp);
  /* dst info.  */
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, 0);

  return ginsn;
}

ginsnS *
ginsn_new_sub (const symbolS *sym, bool real_p,
	       enum ginsn_src_type src1_type, unsigned int src1_reg, offsetT src1_disp,
	       enum ginsn_src_type src2_type, unsigned int src2_reg, offsetT src2_disp,
	       enum ginsn_dst_type dst_type, unsigned int dst_reg, offsetT dst_disp)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_SUB, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src1_type, src1_reg, src1_disp);
  ginsn_set_src (&ginsn->src[1], src2_type, src2_reg, src2_disp);
  /* dst info.  */
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, dst_disp);

  return ginsn;
}

/* PS: Note this API does not identify the displacement values of
   src1/src2/dst.  At this time, it is unnecessary for correctness to support
   the additional argument.  */

ginsnS *
ginsn_new_other (const symbolS *sym, bool real_p,
		 enum ginsn_src_type src1_type, unsigned int src1_val,
		 enum ginsn_src_type src2_type, unsigned int src2_val,
		 enum ginsn_dst_type dst_type, unsigned int dst_reg)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_OTHER, sym, real_p);
  /* src info.  */
  ginsn_set_src (&ginsn->src[0], src1_type, src1_val, src1_val);
  /* GINSN_SRC_INDIRECT src2_type is not expected.  */
  gas_assert (src2_type != GINSN_SRC_INDIRECT);
  ginsn_set_src (&ginsn->src[1], src2_type, src2_val, src2_val);
  /* dst info.  */
  ginsn_set_dst (&ginsn->dst, dst_type, dst_reg, 0);

  return ginsn;
}

ginsnS *
ginsn_new_return (const symbolS *sym, bool real_p)
{
  ginsnS *ginsn = ginsn_init (GINSN_TYPE_RETURN, sym, real_p);
  return ginsn;
}

void
ginsn_set_where (ginsnS *ginsn)
{
  const char *file;
  unsigned int line;
  file = as_where (&line);
  ginsn_set_file_line (ginsn, file, line);
}

int
ginsn_link_next (ginsnS *ginsn, ginsnS *next)
{
  int ret = 0;

  /* Avoid data corruption by limiting the scope of the API.  */
  if (!ginsn || ginsn->next)
    return 1;

  ginsn->next = next;

  return ret;
}

bool
ginsn_track_reg_p (unsigned int dw2reg, enum ginsn_gen_mode gmode)
{
  bool track_p = false;

  if (gmode == GINSN_GEN_SCFI && dw2reg <= SCFI_MAX_REG_ID)
    {
      /* FIXME - rename this to tc_ ? */
      track_p |= SCFI_CALLEE_SAVED_REG_P (dw2reg);
      track_p |= (dw2reg == REG_FP);
      track_p |= (dw2reg == REG_SP);
    }

  return track_p;
}

static bool
ginsn_indirect_jump_p (ginsnS *ginsn)
{
  bool ret_p = false;
  if (!ginsn)
    return ret_p;

  ret_p = (ginsn->type == GINSN_TYPE_JUMP
	   && ginsn->src[0].type == GINSN_SRC_REG);
  return ret_p;
}

static bool
ginsn_direct_local_jump_p (ginsnS *ginsn)
{
  bool ret_p = false;
  if (!ginsn)
    return ret_p;

  ret_p |= (ginsn->type == GINSN_TYPE_JUMP
	    && ginsn->src[0].type == GINSN_SRC_SYMBOL);
  return ret_p;
}

static char *
ginsn_src_print (struct ginsn_src *src)
{
  size_t len = 40;
  char *src_str = XNEWVEC (char, len);

  memset (src_str, 0, len);

  switch (src->type)
    {
    case GINSN_SRC_REG:
      snprintf (src_str, len, "%%r%d, ", ginsn_get_src_reg (src));
      break;
    case GINSN_SRC_IMM:
      snprintf (src_str, len, "%lld, ",
		(long long int) ginsn_get_src_imm (src));
      break;
    case GINSN_SRC_INDIRECT:
      snprintf (src_str, len, "[%%r%d+%lld], ", ginsn_get_src_reg (src),
		(long long int) ginsn_get_src_disp (src));
      break;
    default:
      break;
    }

  return src_str;
}

static char*
ginsn_dst_print (struct ginsn_dst *dst)
{
  size_t len = GINSN_LISTING_OPND_LEN;
  char *dst_str = XNEWVEC (char, len);

  memset (dst_str, 0, len);

  if (dst->type == GINSN_DST_REG)
    {
      char *buf = XNEWVEC (char, 32);
      sprintf (buf, "%%r%d", ginsn_get_dst_reg (dst));
      strcat (dst_str, buf);
    }
  else if (dst->type == GINSN_DST_INDIRECT)
    {
      char *buf = XNEWVEC (char, 32);
      sprintf (buf, "[%%r%d+%lld]", ginsn_get_dst_reg (dst),
		 (long long int) ginsn_get_dst_disp (dst));
      strcat (dst_str, buf);
    }

  gas_assert (strlen (dst_str) < GINSN_LISTING_OPND_LEN);

  return dst_str;
}

static const char*
ginsn_type_func_marker_print (ginsnS *ginsn)
{
  int id = 0;
  static const char * const ginsn_sym_strs[] =
    { "", "FUNC_BEGIN", "FUNC_END" };

  if (GINSN_F_FUNC_BEGIN_P (ginsn))
    id = 1;
  else if (GINSN_F_FUNC_END_P (ginsn))
    id = 2;

  return ginsn_sym_strs[id];
}

static char*
ginsn_print (ginsnS *ginsn)
{
  struct ginsn_src *src;
  struct ginsn_dst *dst;
  int str_size = 0;
  size_t len = GINSN_LISTING_LEN;
  char *ginsn_str = XNEWVEC (char, len);

  memset (ginsn_str, 0, len);

  str_size = snprintf (ginsn_str, GINSN_LISTING_LEN, "ginsn: %s",
		       ginsn_type_names[ginsn->type]);
  gas_assert (str_size >= 0 && str_size < GINSN_LISTING_LEN);

  /* For some ginsn types, no further information is printed for now.  */
  if (ginsn->type == GINSN_TYPE_CALL
      || ginsn->type == GINSN_TYPE_RETURN)
    goto end;
  else if (ginsn->type == GINSN_TYPE_SYMBOL)
    {
      if (GINSN_F_USER_LABEL_P (ginsn))
	str_size += snprintf (ginsn_str + str_size,
			      GINSN_LISTING_LEN - str_size,
			      " %s", S_GET_NAME (ginsn->sym));
      else
	str_size += snprintf (ginsn_str + str_size,
			      GINSN_LISTING_LEN - str_size,
			      " %s", ginsn_type_func_marker_print (ginsn));
      goto end;
    }

  /* src 1.  */
  src = ginsn_get_src1 (ginsn);
  str_size += snprintf (ginsn_str + str_size, GINSN_LISTING_LEN - str_size,
			" %s", ginsn_src_print (src));
  gas_assert (str_size >= 0 && str_size < GINSN_LISTING_LEN);

  /* src 2.  */
  src = ginsn_get_src2 (ginsn);
  str_size += snprintf (ginsn_str + str_size, GINSN_LISTING_LEN - str_size,
			"%s", ginsn_src_print (src));
  gas_assert (str_size >= 0 && str_size < GINSN_LISTING_LEN);

  /* dst.  */
  dst = ginsn_get_dst (ginsn);
  str_size += snprintf (ginsn_str + str_size, GINSN_LISTING_LEN - str_size,
			"%s", ginsn_dst_print (dst));

end:
  gas_assert (str_size >= 0 && str_size < GINSN_LISTING_LEN);
  return ginsn_str;
}

static void
gbb_cleanup (gbbS **bbp)
{
  gbbS *bb = NULL;

  if (!bbp && !*bbp)
    return;

  bb = *bbp;

  if (bb->entry_state)
    {
      free (bb->entry_state);
      bb->entry_state = NULL;
    }
  if (bb->exit_state)
    {
      free (bb->exit_state);
      bb->exit_state = NULL;
    }
  free (bb);
  *bbp = NULL;
}

static void
bb_add_edge (gbbS* from_bb, gbbS *to_bb)
{
  gedgeS *tmpedge = NULL;
  gedgeS *gedge;
  bool exists = false;

  if (!from_bb || !to_bb)
    return;

  /* Create a new edge object.  */
  gedge = XCNEW (gedgeS);
  gedge->dst_bb = to_bb;
  gedge->next = NULL;
  gedge->visited = false;

  /* Add it in.  */
  if (from_bb->out_gedges == NULL)
    {
      from_bb->out_gedges = gedge;
      from_bb->num_out_gedges++;
    }
  else
    {
      /* Get the tail of the list.  */
      tmpedge = from_bb->out_gedges;
      while (tmpedge)
	{
	  /* Do not add duplicate edges.  Duplicated edges will cause unwanted
	     failures in the forward and backward passes for SCFI.  */
	  if (tmpedge->dst_bb == to_bb)
	    {
	      exists = true;
	      break;
	    }
	  if (tmpedge->next)
	    tmpedge = tmpedge->next;
	  else
	    break;
	}

      if (!exists)
	{
	  tmpedge->next = gedge;
	  from_bb->num_out_gedges++;
	}
      else
	free (gedge);
    }
}

static void
cfg_add_bb (gcfgS *gcfg, gbbS *gbb)
{
  gbbS *last_bb = NULL;

  if (!gcfg->root_bb)
    gcfg->root_bb = gbb;
  else
    {
      last_bb = gcfg->root_bb;
      while (last_bb->next)
	last_bb = last_bb->next;

      last_bb->next = gbb;
    }
  gcfg->num_gbbs++;

  gbb->id = gcfg->num_gbbs;
}

static gbbS *
add_bb_at_ginsn (const symbolS *func, gcfgS *gcfg, ginsnS *ginsn, gbbS *prev_bb,
		 int *errp);

static gbbS *
find_bb (gcfgS *gcfg, ginsnS *ginsn)
{
  gbbS *found_bb = NULL;
  gbbS *gbb = NULL;

  if (!ginsn)
    return found_bb;

  if (ginsn->visited)
    {
      cfg_for_each_bb (gcfg, gbb)
	{
	  if (gbb->first_ginsn == ginsn)
	    {
	      found_bb = gbb;
	      break;
	    }
	}
      /* Must be found if ginsn is visited.  */
      gas_assert (found_bb);
    }

  return found_bb;
}

static gbbS *
find_or_make_bb (const symbolS *func, gcfgS *gcfg, ginsnS *ginsn, gbbS *prev_bb,
		 int *errp)
{
  gbbS *found_bb = NULL;

  found_bb = find_bb (gcfg, ginsn);
  if (found_bb)
    return found_bb;

  return add_bb_at_ginsn (func, gcfg, ginsn, prev_bb, errp);
}

/* Add the basic block starting at GINSN to the given GCFG.
   Also adds an edge from the PREV_BB to the newly added basic block.

   This is a recursive function which returns the root of the added
   basic blocks.  */

static gbbS *
add_bb_at_ginsn (const symbolS *func, gcfgS *gcfg, ginsnS *ginsn, gbbS *prev_bb,
		 int *errp)
{
  gbbS *current_bb = NULL;
  ginsnS *target_ginsn = NULL;
  const symbolS *taken_label;

  while (ginsn)
    {
      /* Skip these as they may be right after a GINSN_TYPE_RETURN.
	 For GINSN_TYPE_RETURN, we have already considered that as
	 end of bb, and a logical exit from function.  */
      if (GINSN_F_FUNC_END_P (ginsn))
	{
	  ginsn = ginsn->next;
	  continue;
	}

      if (ginsn->visited)
	{
	  /* If the ginsn has been visited earlier, the bb must exist by now
	     in the cfg.  */
	  prev_bb = current_bb;
	  current_bb = find_bb (gcfg, ginsn);
	  gas_assert (current_bb);
	  /* Add edge from the prev_bb.  */
	  if (prev_bb)
	    bb_add_edge (prev_bb, current_bb);
	  break;
	}
      else if (current_bb && GINSN_F_USER_LABEL_P (ginsn))
	{
	  /* Create new bb starting at this label ginsn.  */
	  prev_bb = current_bb;
	  find_or_make_bb (func, gcfg, ginsn, prev_bb, errp);
	  break;
	}

      if (current_bb == NULL)
	{
	  /* Create a new bb.  */
	  current_bb = XCNEW (gbbS);
	  cfg_add_bb (gcfg, current_bb);
	  /* Add edge for the Not Taken, or Fall-through path.  */
	  if (prev_bb)
	    bb_add_edge (prev_bb, current_bb);
	}

      if (current_bb->first_ginsn == NULL)
	current_bb->first_ginsn = ginsn;

      ginsn->visited = true;
      current_bb->num_ginsns++;
      current_bb->last_ginsn = ginsn;

      /* Note that BB is _not_ split on ginsn of type GINSN_TYPE_CALL.  */
      if (ginsn->type == GINSN_TYPE_JUMP
	  || ginsn->type == GINSN_TYPE_JUMP_COND
	  || ginsn->type == GINSN_TYPE_RETURN)
	{
	  /* Indirect Jumps or direct jumps to symbols non-local to the
	     function must not be seen here.  The caller must have already
	     checked for that.  */
	  gas_assert (!ginsn_indirect_jump_p (ginsn));
	  if (ginsn->type == GINSN_TYPE_JUMP)
	    gas_assert (ginsn_direct_local_jump_p (ginsn));

	  /* Direct Jumps.  May include conditional or unconditional change of
	     flow.  What is important for CFG creation is that the target be
	     local to function.  */
	  if (ginsn->type == GINSN_TYPE_JUMP_COND
	      || ginsn_direct_local_jump_p (ginsn))
	    {
	      gas_assert (ginsn->src[0].type == GINSN_SRC_SYMBOL);
	      taken_label = ginsn->src[0].sym;
	      gas_assert (taken_label);

	      /* Preserve the prev_bb to be the dominator bb as we are
		 going to follow the taken path of the conditional branch
		 soon.  */
	      prev_bb = current_bb;

	      /* Follow the target on the taken path.  */
	      target_ginsn = label_ginsn_map_find (taken_label);
	      /* Add the bb for the target of the taken branch.  */
	      if (target_ginsn)
		find_or_make_bb (func, gcfg, target_ginsn, prev_bb, errp);
	      else
		{
		  *errp = GCFG_JLABEL_NOT_PRESENT;
		  as_warn_where (ginsn->file, ginsn->line,
				 _("missing label '%s' in func '%s' may result in imprecise cfg"),
				 S_GET_NAME (taken_label), S_GET_NAME (func));
		}
	      /* Add the bb for the fall through path.  */
	      find_or_make_bb (func, gcfg, ginsn->next, prev_bb, errp);
	    }
	 else if (ginsn->type == GINSN_TYPE_RETURN)
	   {
	     /* We'll come back to the ginsns following GINSN_TYPE_RETURN
		from another path if they are indeed reachable code.  */
	     break;
	   }

	 /* Current BB has been processed.  */
	 current_bb = NULL;
	}
      ginsn = ginsn->next;
    }

  return current_bb;
}

static int
gbbs_compare (const void *v1, const void *v2)
{
  const gbbS *bb1 = *(const gbbS **) v1;
  const gbbS *bb2 = *(const gbbS **) v2;

  if (bb1->first_ginsn->id < bb2->first_ginsn->id)
    return -1;
  else if (bb1->first_ginsn->id > bb2->first_ginsn->id)
    return 1;
  else if (bb1->first_ginsn->id == bb2->first_ginsn->id)
    return 0;

  return 0;
}

/* Synthesize DWARF CFI and emit it.  */

static int
ginsn_pass_execute_scfi (const symbolS *func, gcfgS *gcfg, gbbS *root_bb)
{
  int err = scfi_synthesize_dw2cfi (func, gcfg, root_bb);
  if (!err)
    scfi_emit_dw2cfi (func);

  return err;
}

/* Traverse the list of ginsns for the function and warn if some
   ginsns are not visited.

   FIXME - this code assumes the caller has already performed a pass over
   ginsns such that the reachable ginsns are already marked.  Revisit this - we
   should ideally make this pass self-sufficient.  */

static int
ginsn_pass_warn_unreachable_code (const symbolS *func,
				  gcfgS *gcfg ATTRIBUTE_UNUSED,
				  ginsnS *root_ginsn)
{
  ginsnS *ginsn;
  bool unreach_p = false;

  if (!gcfg || !func || !root_ginsn)
    return 0;

  ginsn = root_ginsn;

  while (ginsn)
    {
      /* Some ginsns of type GINSN_TYPE_SYMBOL remain unvisited.  Some
	 may even be excluded from the CFG as they are not reachable, given
	 their function, e.g., user labels after return machine insn.  */
      if (!ginsn->visited
	  && !GINSN_F_FUNC_END_P (ginsn)
	  && !GINSN_F_USER_LABEL_P (ginsn))
	{
	  unreach_p = true;
	  break;
	}
      ginsn = ginsn->next;
    }

  if (unreach_p)
    as_warn_where (ginsn->file, ginsn->line,
		   _("GINSN: found unreachable code in func '%s'"),
		   S_GET_NAME (func));

  return unreach_p;
}

void
gcfg_get_bbs_in_prog_order (gcfgS *gcfg, gbbS **prog_order_bbs)
{
  uint64_t i = 0;
  gbbS *gbb;

  if (!prog_order_bbs)
    return;

  cfg_for_each_bb (gcfg, gbb)
    {
      gas_assert (i < gcfg->num_gbbs);
      prog_order_bbs[i++] = gbb;
    }

  qsort (prog_order_bbs, gcfg->num_gbbs, sizeof (gbbS *), gbbs_compare);
}

/* Build the control flow graph for the ginsns of the function.

   It is important that the target adds an appropriate ginsn:
     - GINSN_TYPE_JUMP,
     - GINSN_TYPE_JUMP_COND,
     - GINSN_TYPE_CALL,
     - GINSN_TYPE_RET
  at the associated points in the function.  The correctness of the CFG
  depends on the accuracy of these 'change of flow instructions'.  */

gcfgS *
gcfg_build (const symbolS *func, int *errp)
{
  gcfgS *gcfg;
  ginsnS *first_ginsn;

  gcfg = XCNEW (gcfgS);
  first_ginsn = frchain_now->frch_ginsn_data->gins_rootP;
  add_bb_at_ginsn (func, gcfg, first_ginsn, NULL /* prev_bb.  */, errp);

  return gcfg;
}

void
gcfg_cleanup (gcfgS **gcfgp)
{
  gcfgS *cfg;
  gbbS *bb, *next_bb;
  gedgeS *edge, *next_edge;

  if (!gcfgp || !*gcfgp)
    return;

  cfg = *gcfgp;
  bb = gcfg_get_rootbb (cfg);

  while (bb)
    {
      next_bb = bb->next;

      /* Cleanup all the edges.  */
      edge = bb->out_gedges;
      while (edge)
	{
	  next_edge = edge->next;
	  free (edge);
	  edge = next_edge;
	}

      gbb_cleanup (&bb);
      bb = next_bb;
    }

  free (cfg);
  *gcfgp = NULL;
}

gbbS *
gcfg_get_rootbb (gcfgS *gcfg)
{
  gbbS *rootbb = NULL;

  if (!gcfg || !gcfg->num_gbbs)
    return NULL;

  rootbb = gcfg->root_bb;

  return rootbb;
}

void
gcfg_print (const gcfgS *gcfg, FILE *outfile)
{
  gbbS *gbb = NULL;
  gedgeS *gedge = NULL;
  uint64_t total_ginsns = 0;

  cfg_for_each_bb(gcfg, gbb)
    {
      fprintf (outfile, "BB [%" PRIu64 "] with num insns: %" PRIu64,
	       gbb->id, gbb->num_ginsns);
      fprintf (outfile, " [insns: %u to %u]\n",
	       gbb->first_ginsn->line, gbb->last_ginsn->line);
      total_ginsns += gbb->num_ginsns;
      bb_for_each_edge(gbb, gedge)
	fprintf (outfile, "  outgoing edge to %" PRIu64 "\n",
		 gedge->dst_bb->id);
    }
  fprintf (outfile, "\nTotal ginsns in all GBBs = %" PRIu64 "\n",
	   total_ginsns);
}

void
frch_ginsn_data_init (const symbolS *func, symbolS *start_addr,
		      enum ginsn_gen_mode gmode)
{
  /* FIXME - error out if prev object is not free'd ?  */
  frchain_now->frch_ginsn_data = XCNEW (struct frch_ginsn_data);

  frchain_now->frch_ginsn_data->mode = gmode;
  /* Annotate with the current function symbol.  */
  frchain_now->frch_ginsn_data->func = func;
  /* Create a new start address symbol now.  */
  frchain_now->frch_ginsn_data->start_addr = start_addr;
  /* Assume the set of ginsn are apt for CFG creation, by default.  */
  frchain_now->frch_ginsn_data->gcfg_apt_p = true;

  frchain_now->frch_ginsn_data->label_ginsn_map = str_htab_create ();
}

void
frch_ginsn_data_cleanup (void)
{
  ginsnS *ginsn = NULL;
  ginsnS *next_ginsn = NULL;

  ginsn = frchain_now->frch_ginsn_data->gins_rootP;
  while (ginsn)
    {
      next_ginsn = ginsn->next;
      ginsn_cleanup (&ginsn);
      ginsn = next_ginsn;
    }

  if (frchain_now->frch_ginsn_data->label_ginsn_map)
    htab_delete (frchain_now->frch_ginsn_data->label_ginsn_map);

  free (frchain_now->frch_ginsn_data);
  frchain_now->frch_ginsn_data = NULL;
}

/* Append GINSN to the list of ginsns for the current function being
   assembled.  */

int
frch_ginsn_data_append (ginsnS *ginsn)
{
  ginsnS *last = NULL;
  ginsnS *temp = NULL;
  uint64_t id = 0;

  if (!ginsn)
    return 1;

  if (frchain_now->frch_ginsn_data->gins_lastP)
    id = frchain_now->frch_ginsn_data->gins_lastP->id;

  /* Do the necessary preprocessing on the set of input GINSNs:
       - Update each ginsn with its ID.
     While you iterate, also keep gcfg_apt_p updated by checking whether any
     ginsn is inappropriate for GCFG creation.  */
  temp = ginsn;
  while (temp)
    {
      temp->id = ++id;

      if (ginsn_indirect_jump_p (temp)
	  || (ginsn->type == GINSN_TYPE_JUMP
	      && !ginsn_direct_local_jump_p (temp)))
	frchain_now->frch_ginsn_data->gcfg_apt_p = false;

      if (listing & LISTING_GINSN_SCFI)
	listing_newline (ginsn_print (temp));

      /* The input GINSN may be a linked list of multiple ginsns chained
	 together.  Find the last ginsn in the input chain of ginsns.  */
      last = temp;

      temp = temp->next;
    }

  /* Link in the ginsn to the tail.  */
  if (!frchain_now->frch_ginsn_data->gins_rootP)
    frchain_now->frch_ginsn_data->gins_rootP = ginsn;
  else
    ginsn_link_next (frchain_now->frch_ginsn_data->gins_lastP, ginsn);

  frchain_now->frch_ginsn_data->gins_lastP = last;

  return 0;
}

enum ginsn_gen_mode
frch_ginsn_gen_mode (void)
{
  enum ginsn_gen_mode gmode = GINSN_GEN_NONE;

  if (frchain_now->frch_ginsn_data)
    gmode = frchain_now->frch_ginsn_data->mode;

  return gmode;
}

int
ginsn_data_begin (const symbolS *func)
{
  ginsnS *ginsn;

  /* The previous block of asm must have been processed by now.  */
  if (frchain_now->frch_ginsn_data)
    as_bad (_("GINSN process for prev func not done"));

  /* FIXME - hard code the mode to GINSN_GEN_SCFI.
     This can be changed later when other passes on ginsns are formalised.  */
  frch_ginsn_data_init (func, symbol_temp_new_now (), GINSN_GEN_SCFI);

  /* Create and insert ginsn with function begin marker.  */
  ginsn = ginsn_new_symbol_func_begin (func);
  frch_ginsn_data_append (ginsn);

  return 0;
}

int
ginsn_data_end (const symbolS *label)
{
  ginsnS *ginsn;
  gbbS *root_bb;
  gcfgS *gcfg = NULL;
  const symbolS *func;
  int err = 0;

  if (!frchain_now->frch_ginsn_data)
    return err;

  /* Insert Function end marker.  */
  ginsn = ginsn_new_symbol_func_end (label);
  frch_ginsn_data_append (ginsn);

  func = frchain_now->frch_ginsn_data->func;

  /* Build the cfg of ginsn(s) of the function.  */
  if (!frchain_now->frch_ginsn_data->gcfg_apt_p)
    {
      as_bad (_("untraceable control flow for func '%s'"),
	      S_GET_NAME (func));
      goto end;
    }

  gcfg = gcfg_build (func, &err);

  root_bb = gcfg_get_rootbb (gcfg);
  if (!root_bb)
    {
      as_bad (_("Bad cfg of ginsn of func '%s'"), S_GET_NAME (func));
      goto end;
    }

  /* Execute the desired passes on ginsns.  */
  err = ginsn_pass_execute_scfi (func, gcfg, root_bb);
  if (err)
    goto end;

  /* Other passes, e.g., warn for unreachable code can be enabled too.  */
  ginsn = frchain_now->frch_ginsn_data->gins_rootP;
  err = ginsn_pass_warn_unreachable_code (func, gcfg, ginsn);

end:
  if (gcfg)
    gcfg_cleanup (&gcfg);
  frch_ginsn_data_cleanup ();

  return err;
}

/* Add GINSN_TYPE_SYMBOL type ginsn for user-defined labels.  These may be
   branch targets, and hence are necessary for control flow graph.  */

void
ginsn_frob_label (const symbolS *label)
{
  ginsnS *label_ginsn;
  const char *file;
  unsigned int line;

  if (frchain_now->frch_ginsn_data)
    {
      /* PS: Note how we keep the actual LABEL symbol as ginsn->sym.
	 Take care to avoid inadvertent updates or cleanups of symbols.  */
      label_ginsn = ginsn_new_symbol_user_label (label);
      /* Keep the location updated.  */
      file = as_where (&line);
      ginsn_set_file_line (label_ginsn, file, line);

      frch_ginsn_data_append (label_ginsn);

      label_ginsn_map_insert (label, label_ginsn);
    }
}

const symbolS *
ginsn_data_func_symbol (void)
{
  const symbolS *func = NULL;

  if (frchain_now->frch_ginsn_data)
    func = frchain_now->frch_ginsn_data->func;

  return func;
}

#else

int
ginsn_data_begin (const symbolS *func ATTRIBUTE_UNUSED)
{
  as_bad (_("ginsn unsupported for target"));
  return 1;
}

int
ginsn_data_end (const symbolS *label ATTRIBUTE_UNUSED)
{
  as_bad (_("ginsn unsupported for target"));
  return 1;
}

void
ginsn_frob_label (const symbolS *sym ATTRIBUTE_UNUSED)
{
  return;
}

const symbolS *
ginsn_data_func_symbol (void)
{
  return NULL;
}

#endif  /* TARGET_USE_GINSN.  */
