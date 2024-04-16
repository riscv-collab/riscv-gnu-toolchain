/* Ada Pragma Import support.

   Copyright (C) 2023-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "symtab.h"
#include "value.h"
#include "dwarf2/loc.h"

/* Helper to get the imported symbol's real name.  */
static const char *
get_imported_name (const struct symbol *sym)
{
  return (const char *) SYMBOL_LOCATION_BATON (sym);
}

/* Implement the read_variable method from symbol_computed_ops.  */

static struct value *
ada_imported_read_variable (struct symbol *symbol, frame_info_ptr frame)
{
  const char *name = get_imported_name (symbol);
  bound_minimal_symbol minsym = lookup_minimal_symbol_linkage (name, false);
  if (minsym.minsym == nullptr)
    error (_("could not find imported name %s"), name);
  return value_at (symbol->type (), minsym.value_address ());
}

/* Implement the read_variable method from symbol_computed_ops.  */

static enum symbol_needs_kind
ada_imported_get_symbol_read_needs (struct symbol *symbol)
{
  return SYMBOL_NEEDS_NONE;
}

/* Implement the describe_location method from
   symbol_computed_ops.  */

static void
ada_imported_describe_location (struct symbol *symbol, CORE_ADDR addr,
				struct ui_file *stream)
{
  gdb_printf (stream, "an imported name for '%s'",
	      get_imported_name (symbol));
}

/* Implement the tracepoint_var_ref method from
   symbol_computed_ops.  */

static void
ada_imported_tracepoint_var_ref (struct symbol *symbol, struct agent_expr *ax,
				 struct axs_value *value)
{
  /* Probably could be done, but not needed right now.  */
  error (_("not implemented: trace of imported Ada symbol"));
}

/* Implement the generate_c_location method from
   symbol_computed_ops.  */

static void
ada_imported_generate_c_location (struct symbol *symbol, string_file *stream,
				  struct gdbarch *gdbarch,
				  std::vector<bool> &registers_used,
				  CORE_ADDR pc, const char *result_name)
{
  /* Probably could be done, but not needed right now, and perhaps not
     ever.  */
  error (_("not implemented: compile translation of imported Ada symbol"));
}

const struct symbol_computed_ops ada_imported_funcs =
{
  ada_imported_read_variable,
  nullptr,
  ada_imported_get_symbol_read_needs,
  ada_imported_describe_location,
  0,
  ada_imported_tracepoint_var_ref,
  ada_imported_generate_c_location
};

/* Implement the get_block_value method from symbol_block_ops.  */

static const block *
ada_alias_get_block_value (const struct symbol *sym)
{
  const char *name = get_imported_name (sym);
  block_symbol real_symbol = lookup_global_symbol (name, nullptr,
						   VAR_DOMAIN);
  if (real_symbol.symbol == nullptr)
    error (_("could not find alias '%s' for function '%s'"),
	   name, sym->print_name ());
  if (real_symbol.symbol->aclass () != LOC_BLOCK)
    error (_("alias '%s' for function '%s' is not a function"),
	   name, sym->print_name ());

  return real_symbol.symbol->value_block ();
}

const struct symbol_block_ops ada_function_alias_funcs =
{
  nullptr,
  nullptr,
  ada_alias_get_block_value
};
