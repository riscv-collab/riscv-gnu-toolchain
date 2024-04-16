/* Print in infix form a struct expression.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "language.h"
#include "parser-defs.h"
#include "user-regs.h"
#include "target.h"
#include "block.h"
#include "objfiles.h"
#include "valprint.h"
#include "cli/cli-style.h"
#include "c-lang.h"
#include "expop.h"
#include "ada-exp.h"

#include <ctype.h>

/* Meant to be used in debug sessions, so don't export it in a header file.  */
extern void ATTRIBUTE_USED debug_exp (struct expression *exp);

/* Print EXP.  */

void
ATTRIBUTE_USED
debug_exp (struct expression *exp)
{
  exp->dump (gdb_stdlog);
  gdb_flush (gdb_stdlog);
}

namespace expr
{

bool
check_objfile (const struct block *block, struct objfile *objfile)
{
  return check_objfile (block->objfile (), objfile);
}

void
dump_for_expression (struct ui_file *stream, int depth, enum exp_opcode op)
{
  gdb_printf (stream, _("%*sOperation: "), depth, "");

  switch (op)
    {
    default:
      gdb_printf (stream, "<unknown %d>", op);
      break;

#define OP(name)	\
    case name:		\
      gdb_puts (#name, stream); \
      break;
#include "std-operator.def"
#undef OP
    }

  gdb_puts ("\n", stream);
}

void
dump_for_expression (struct ui_file *stream, int depth, const std::string &str)
{
  gdb_printf (stream, _("%*sString: %s\n"), depth, "", str.c_str ());
}

void
dump_for_expression (struct ui_file *stream, int depth, struct type *type)
{
  gdb_printf (stream, _("%*sType: "), depth, "");
  type_print (type, nullptr, stream, 0);
  gdb_printf (stream, "\n");
}

void
dump_for_expression (struct ui_file *stream, int depth, CORE_ADDR addr)
{
  gdb_printf (stream, _("%*sConstant: %s\n"), depth, "",
	      core_addr_to_string (addr));
}

void
dump_for_expression (struct ui_file *stream, int depth, const gdb_mpz &val)
{
  gdb_printf (stream, _("%*sConstant: %s\n"), depth, "", val.str ().c_str ());
}

void
dump_for_expression (struct ui_file *stream, int depth, internalvar *ivar)
{
  gdb_printf (stream, _("%*sInternalvar: $%s\n"), depth, "",
	      internalvar_name (ivar));
}

void
dump_for_expression (struct ui_file *stream, int depth, symbol *sym)
{
  gdb_printf (stream, _("%*sSymbol: %s\n"), depth, "",
	      sym->print_name ());
  dump_for_expression (stream, depth + 1, sym->type ());
}

void
dump_for_expression (struct ui_file *stream, int depth,
		     bound_minimal_symbol msym)
{
  gdb_printf (stream, _("%*sMinsym %s in objfile %s\n"), depth, "",
	      msym.minsym->print_name (), objfile_name (msym.objfile));
}

void
dump_for_expression (struct ui_file *stream, int depth, const block *bl)
{
  gdb_printf (stream, _("%*sBlock: %p\n"), depth, "", bl);
}

void
dump_for_expression (struct ui_file *stream, int depth,
		     const block_symbol &sym)
{
  gdb_printf (stream, _("%*sBlock symbol:\n"), depth, "");
  dump_for_expression (stream, depth + 1, sym.symbol);
  dump_for_expression (stream, depth + 1, sym.block);
}

void
dump_for_expression (struct ui_file *stream, int depth,
		     type_instance_flags flags)
{
  gdb_printf (stream, _("%*sType flags: "), depth, "");
  if (flags & TYPE_INSTANCE_FLAG_CONST)
    gdb_puts ("const ", stream);
  if (flags & TYPE_INSTANCE_FLAG_VOLATILE)
    gdb_puts ("volatile", stream);
  gdb_printf (stream, "\n");
}

void
dump_for_expression (struct ui_file *stream, int depth,
		     enum c_string_type_values flags)
{
  gdb_printf (stream, _("%*sC string flags: "), depth, "");
  switch (flags & ~C_CHAR)
    {
    case C_WIDE_STRING:
      gdb_puts (_("wide "), stream);
      break;
    case C_STRING_16:
      gdb_puts (_("u16 "), stream);
      break;
    case C_STRING_32:
      gdb_puts (_("u32 "), stream);
      break;
    default:
      gdb_puts (_("ordinary "), stream);
      break;
    }

  if ((flags & C_CHAR) != 0)
    gdb_puts (_("char"), stream);
  else
    gdb_puts (_("string"), stream);
  gdb_puts ("\n", stream);
}

void
dump_for_expression (struct ui_file *stream, int depth,
		     enum range_flag flags)
{
  gdb_printf (stream, _("%*sRange:"), depth, "");
  if ((flags & RANGE_LOW_BOUND_DEFAULT) != 0)
    gdb_puts (_("low-default "), stream);
  if ((flags & RANGE_HIGH_BOUND_DEFAULT) != 0)
    gdb_puts (_("high-default "), stream);
  if ((flags & RANGE_HIGH_BOUND_EXCLUSIVE) != 0)
    gdb_puts (_("high-exclusive "), stream);
  if ((flags & RANGE_HAS_STRIDE) != 0)
    gdb_puts (_("has-stride"), stream);
  gdb_printf (stream, "\n");
}

void
dump_for_expression (struct ui_file *stream, int depth,
		     const std::unique_ptr<ada_component> &comp)
{
  comp->dump (stream, depth);
}

void
float_const_operation::dump (struct ui_file *stream, int depth) const
{
  gdb_printf (stream, _("%*sFloat: "), depth, "");
  print_floating (m_data.data (), m_type, stream);
  gdb_printf (stream, "\n");
}

} /* namespace expr */
