/* scfidw2gen.c - Support for emission of synthesized Dwarf2 CFI.
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
#include "ginsn.h"
#include "scfi.h"
#include "dw2gencfi.h"
#include "subsegs.h"
#include "scfidw2gen.h"

#if defined (TARGET_USE_SCFI) && defined (TARGET_USE_GINSN)

static bool scfi_ignore_warn_once;

static void
dot_scfi_ignore (int ignored ATTRIBUTE_UNUSED)
{
  gas_assert (flag_synth_cfi);

  if (!scfi_ignore_warn_once)
    {
      as_warn (_("SCFI ignores most user-specified CFI directives"));
      scfi_ignore_warn_once = true;
    }
  ignore_rest_of_line ();
}

static void
scfi_process_cfi_label (void)
{
  char *name;
  ginsnS *ginsn;

  name = read_symbol_name ();
  if (name == NULL)
    return;

  /* Add a new ginsn.  */
  ginsn = ginsn_new_phantom (symbol_temp_new_now ());
  frch_ginsn_data_append (ginsn);

  scfi_op_add_cfi_label (ginsn, name);
  /* TODO.  */
  // free (name);

  demand_empty_rest_of_line ();
}

static void
scfi_process_cfi_signal_frame (void)
{
  ginsnS *ginsn;

  ginsn = ginsn_new_phantom (symbol_temp_new_now ());
  frch_ginsn_data_append (ginsn);

  scfi_op_add_signal_frame (ginsn);
}

static void
dot_scfi (int arg)
{
  switch (arg)
    {
      case CFI_label:
	scfi_process_cfi_label ();
	break;
      case CFI_signal_frame:
	scfi_process_cfi_signal_frame ();
	break;
      default:
	abort ();
    }
}

const pseudo_typeS scfi_pseudo_table[] =
  {
    { "cfi_sections", dot_cfi_sections, 0 }, /* No ignore.  */
    { "cfi_signal_frame", dot_scfi, CFI_signal_frame }, /* No ignore.  */
    { "cfi_label", dot_scfi, CFI_label }, /* No ignore.  */
    { "cfi_startproc", dot_scfi_ignore, 0 },
    { "cfi_endproc", dot_scfi_ignore, 0 },
    { "cfi_fde_data", dot_scfi_ignore, 0 },
    { "cfi_def_cfa", dot_scfi_ignore, 0 },
    { "cfi_def_cfa_register", dot_scfi_ignore, 0 },
    { "cfi_def_cfa_offset", dot_scfi_ignore, 0 },
    { "cfi_adjust_cfa_offset", dot_scfi_ignore, 0 },
    { "cfi_offset", dot_scfi_ignore, 0 },
    { "cfi_rel_offset", dot_scfi_ignore, 0 },
    { "cfi_register", dot_scfi_ignore, 0 },
    { "cfi_return_column", dot_scfi_ignore, 0 },
    { "cfi_restore", dot_scfi_ignore, 0 },
    { "cfi_undefined", dot_scfi_ignore, 0 },
    { "cfi_same_value", dot_scfi_ignore, 0 },
    { "cfi_remember_state", dot_scfi_ignore, 0 },
    { "cfi_restore_state", dot_scfi_ignore, 0 },
    { "cfi_window_save", dot_scfi_ignore, 0 },
    { "cfi_negate_ra_state", dot_scfi_ignore, 0 },
    { "cfi_escape", dot_scfi_ignore, 0 },
    { "cfi_personality", dot_scfi_ignore, 0 },
    { "cfi_personality_id", dot_scfi_ignore, 0 },
    { "cfi_lsda", dot_scfi_ignore, 0 },
    { "cfi_val_encoded_addr", dot_scfi_ignore, 0 },
    { "cfi_inline_lsda", dot_scfi_ignore, 0 },
    { "cfi_val_offset", dot_scfi_ignore, 0 },
    { NULL, NULL, 0 }
  };

void
scfi_dot_cfi_startproc (const symbolS *start_sym)
{
  if (frchain_now->frch_cfi_data != NULL)
    {
      as_bad (_("SCFI: missing previous SCFI endproc marker"));
      return;
    }

  cfi_new_fde ((symbolS *)start_sym);

  cfi_set_sections ();

  frchain_now->frch_cfi_data->cur_cfa_offset = 0;

  /* By default, SCFI machinery assumes .cfi_startproc is used without
     parameter simple.  */
  tc_cfi_frame_initial_instructions ();

  if ((all_cfi_sections & CFI_EMIT_target) != 0)
    tc_cfi_startproc ();
}

void
scfi_dot_cfi_endproc (const symbolS *end_sym)
{
  struct fde_entry *fde_last;

  if (frchain_now->frch_cfi_data == NULL)
    {
      as_bad (_(".cfi_endproc without corresponding .cfi_startproc"));
      return;
    }

  fde_last = frchain_now->frch_cfi_data->cur_fde_data;
  cfi_set_last_fde (fde_last);

  cfi_end_fde ((symbolS *)end_sym);

  if ((all_cfi_sections & CFI_EMIT_target) != 0)
    tc_cfi_endproc (fde_last);
}

void
scfi_dot_cfi (int arg, unsigned reg1, unsigned reg2, offsetT offset,
	      const char *name, const symbolS *advloc)
{
  if (frchain_now->frch_cfi_data == NULL)
    {
      as_bad (_("CFI instruction used without previous .cfi_startproc"));
      return;
    }

  /* If the last address was not at the current PC, advance to current.  */
  if (frchain_now->frch_cfi_data->last_address != advloc)
    cfi_add_advance_loc ((symbolS *)advloc);

  switch (arg)
    {
    case DW_CFA_offset:
      cfi_add_CFA_offset (reg1, offset);
      break;

    case DW_CFA_val_offset:
      cfi_add_CFA_val_offset (reg1, offset);
      break;

    case CFI_rel_offset:
      cfi_add_CFA_offset (reg1,
			  offset - frchain_now->frch_cfi_data->cur_cfa_offset);
      break;

    case DW_CFA_def_cfa:
      cfi_add_CFA_def_cfa (reg1, offset);
      break;

    case DW_CFA_register:
      cfi_add_CFA_register (reg1, reg2);
      break;

    case DW_CFA_def_cfa_register:
      cfi_add_CFA_def_cfa_register (reg1);
      break;

    case DW_CFA_def_cfa_offset:
      cfi_add_CFA_def_cfa_offset (offset);
      break;

    case CFI_adjust_cfa_offset:
      cfi_add_CFA_def_cfa_offset (frchain_now->frch_cfi_data->cur_cfa_offset
				  + offset);
      break;

    case DW_CFA_restore:
      cfi_add_CFA_restore (reg1);
      break;

    case DW_CFA_remember_state:
      cfi_add_CFA_remember_state ();
      break;

    case DW_CFA_restore_state:
      cfi_add_CFA_restore_state ();
      break;

    case CFI_label:
      cfi_add_label (name);
      break;

    case CFI_signal_frame:
      frchain_now->frch_cfi_data->cur_fde_data->signal_frame = 1;
      break;

/*
    case DW_CFA_undefined:
      for (;;)
	{
	  reg1 = cfi_parse_reg ();
	  cfi_add_CFA_undefined (reg1);
	  SKIP_WHITESPACE ();
	  if (*input_line_pointer != ',')
	    break;
	  ++input_line_pointer;
	}
      break;

    case DW_CFA_same_value:
      reg1 = cfi_parse_reg ();
      cfi_add_CFA_same_value (reg1);
      break;

    case CFI_return_column:
      reg1 = cfi_parse_reg ();
      cfi_set_return_column (reg1);
      break;

    case DW_CFA_GNU_window_save:
      cfi_add_CFA_insn (DW_CFA_GNU_window_save);
      break;

*/
    default:
      abort ();
    }
}

#endif
