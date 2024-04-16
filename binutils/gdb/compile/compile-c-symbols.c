/* Convert symbols from GDB to GCC

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "compile-internal.h"
#include "compile-c.h"
#include "symtab.h"
#include "parser-defs.h"
#include "block.h"
#include "objfiles.h"
#include "compile.h"
#include "value.h"
#include "exceptions.h"
#include "gdbtypes.h"
#include "dwarf2/loc.h"
#include "inferior.h"


/* Compute the name of the pointer representing a local symbol's
   address.  */

gdb::unique_xmalloc_ptr<char>
c_symbol_substitution_name (struct symbol *sym)
{
  return gdb::unique_xmalloc_ptr<char>
    (concat ("__", sym->natural_name (), "_ptr", (char *) NULL));
}

/* Convert a given symbol, SYM, to the compiler's representation.
   CONTEXT is the compiler instance.  IS_GLOBAL is true if the
   symbol came from the global scope.  IS_LOCAL is true if the symbol
   came from a local scope.  (Note that the two are not strictly
   inverses because the symbol might have come from the static
   scope.)  */

static void
convert_one_symbol (compile_c_instance *context,
		    struct block_symbol sym,
		    int is_global,
		    int is_local)
{
  gcc_type sym_type;
  const char *filename = sym.symbol->symtab ()->filename;
  unsigned int line = sym.symbol->line ();

  context->error_symbol_once (sym.symbol);

  if (sym.symbol->aclass () == LOC_LABEL)
    sym_type = 0;
  else
    sym_type = context->convert_type (sym.symbol->type ());

  if (sym.symbol->domain () == STRUCT_DOMAIN)
    {
      /* Binding a tag, so we don't need to build a decl.  */
      context->plugin ().tagbind (sym.symbol->natural_name (),
				  sym_type, filename, line);
    }
  else
    {
      gcc_decl decl;
      enum gcc_c_symbol_kind kind;
      CORE_ADDR addr = 0;
      gdb::unique_xmalloc_ptr<char> symbol_name;

      switch (sym.symbol->aclass ())
	{
	case LOC_TYPEDEF:
	  kind = GCC_C_SYMBOL_TYPEDEF;
	  break;

	case LOC_LABEL:
	  kind = GCC_C_SYMBOL_LABEL;
	  addr = sym.symbol->value_address ();
	  break;

	case LOC_BLOCK:
	  kind = GCC_C_SYMBOL_FUNCTION;
	  addr = sym.symbol->value_block ()->entry_pc ();
	  if (is_global && sym.symbol->type ()->is_gnu_ifunc ())
	    addr = gnu_ifunc_resolve_addr (current_inferior ()->arch (), addr);
	  break;

	case LOC_CONST:
	  if (sym.symbol->type ()->code () == TYPE_CODE_ENUM)
	    {
	      /* Already handled by convert_enum.  */
	      return;
	    }
	  context->plugin ().build_constant
	    (sym_type, sym.symbol->natural_name (),
	     sym.symbol->value_longest (),
	     filename, line);
	  return;

	case LOC_CONST_BYTES:
	  error (_("Unsupported LOC_CONST_BYTES for symbol \"%s\"."),
		 sym.symbol->print_name ());

	case LOC_UNDEF:
	  internal_error (_("LOC_UNDEF found for \"%s\"."),
			  sym.symbol->print_name ());

	case LOC_COMMON_BLOCK:
	  error (_("Fortran common block is unsupported for compilation "
		   "evaluaton of symbol \"%s\"."),
		 sym.symbol->print_name ());

	case LOC_OPTIMIZED_OUT:
	  error (_("Symbol \"%s\" cannot be used for compilation evaluation "
		   "as it is optimized out."),
		 sym.symbol->print_name ());

	case LOC_COMPUTED:
	  if (is_local)
	    goto substitution;
	  /* Probably TLS here.  */
	  warning (_("Symbol \"%s\" is thread-local and currently can only "
		     "be referenced from the current thread in "
		     "compiled code."),
		   sym.symbol->print_name ());
	  [[fallthrough]];
	case LOC_UNRESOLVED:
	  /* 'symbol_name' cannot be used here as that one is used only for
	     local variables from compile_dwarf_expr_to_c.
	     Global variables can be accessed by GCC only by their address, not
	     by their name.  */
	  {
	    struct value *val;
	    frame_info_ptr frame = NULL;

	    if (symbol_read_needs_frame (sym.symbol))
	      {
		frame = get_selected_frame (NULL);
		if (frame == NULL)
		  error (_("Symbol \"%s\" cannot be used because "
			   "there is no selected frame"),
			 sym.symbol->print_name ());
	      }

	    val = read_var_value (sym.symbol, sym.block, frame);
	    if (val->lval () != lval_memory)
	      error (_("Symbol \"%s\" cannot be used for compilation "
		       "evaluation as its address has not been found."),
		     sym.symbol->print_name ());

	    kind = GCC_C_SYMBOL_VARIABLE;
	    addr = val->address ();
	  }
	  break;


	case LOC_REGISTER:
	case LOC_ARG:
	case LOC_REF_ARG:
	case LOC_REGPARM_ADDR:
	case LOC_LOCAL:
	substitution:
	  kind = GCC_C_SYMBOL_VARIABLE;
	  symbol_name = c_symbol_substitution_name (sym.symbol);
	  break;

	case LOC_STATIC:
	  kind = GCC_C_SYMBOL_VARIABLE;
	  addr = sym.symbol->value_address ();
	  break;

	case LOC_FINAL_VALUE:
	default:
	  gdb_assert_not_reached ("Unreachable case in convert_one_symbol.");

	}

      /* Don't emit local variable decls for a raw expression.  */
      if (context->scope () != COMPILE_I_RAW_SCOPE
	  || symbol_name == NULL)
	{
	  decl = context->plugin ().build_decl
	    (sym.symbol->natural_name (),
	     kind,
	     sym_type,
	     symbol_name.get (), addr,
	     filename, line);

	  context->plugin ().bind (decl, is_global);
	}
    }
}

/* Convert a full symbol to its gcc form.  CONTEXT is the compiler to
   use, IDENTIFIER is the name of the symbol, SYM is the symbol
   itself, and DOMAIN is the domain which was searched.  */

static void
convert_symbol_sym (compile_c_instance *context, const char *identifier,
		    struct block_symbol sym, domain_enum domain)
{
  int is_local_symbol;

  /* If we found a symbol and it is not in the  static or global
     scope, then we should first convert any static or global scope
     symbol of the same name.  This lets this unusual case work:

     int x; // Global.
     int func(void)
     {
     int x;
     // At this spot, evaluate "extern int x; x"
     }
  */

  const struct block *static_block = nullptr;
  if (sym.block != nullptr)
    static_block = sym.block->static_block ();
  /* STATIC_BLOCK is NULL if FOUND_BLOCK is the global block.  */
  is_local_symbol = (sym.block != static_block && static_block != NULL);
  if (is_local_symbol)
    {
      struct block_symbol global_sym;

      global_sym = lookup_symbol (identifier, NULL, domain, NULL);
      /* If the outer symbol is in the static block, we ignore it, as
	 it cannot be referenced.  */
      if (global_sym.symbol != NULL
	  && global_sym.block != global_sym.block->static_block ())
	{
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"gcc_convert_symbol \"%s\": global symbol\n",
			identifier);
	  convert_one_symbol (context, global_sym, 1, 0);
	}
    }

  if (compile_debug)
    gdb_printf (gdb_stdlog,
		"gcc_convert_symbol \"%s\": local symbol\n",
		identifier);
  convert_one_symbol (context, sym, 0, is_local_symbol);
}

/* Convert a minimal symbol to its gcc form.  CONTEXT is the compiler
   to use and BMSYM is the minimal symbol to convert.  */

static void
convert_symbol_bmsym (compile_c_instance *context,
		      struct bound_minimal_symbol bmsym)
{
  struct minimal_symbol *msym = bmsym.minsym;
  struct objfile *objfile = bmsym.objfile;
  struct type *type;
  enum gcc_c_symbol_kind kind;
  gcc_type sym_type;
  gcc_decl decl;
  CORE_ADDR addr;

  addr = msym->value_address (objfile);

  /* Conversion copied from write_exp_msymbol.  */
  switch (msym->type ())
    {
    case mst_text:
    case mst_file_text:
    case mst_solib_trampoline:
      type = builtin_type (objfile)->nodebug_text_symbol;
      kind = GCC_C_SYMBOL_FUNCTION;
      break;

    case mst_text_gnu_ifunc:
      type = builtin_type (objfile)->nodebug_text_gnu_ifunc_symbol;
      kind = GCC_C_SYMBOL_FUNCTION;
      addr = gnu_ifunc_resolve_addr (current_inferior ()->arch (), addr);
      break;

    case mst_data:
    case mst_file_data:
    case mst_bss:
    case mst_file_bss:
      type = builtin_type (objfile)->nodebug_data_symbol;
      kind = GCC_C_SYMBOL_VARIABLE;
      break;

    case mst_slot_got_plt:
      type = builtin_type (objfile)->nodebug_got_plt_symbol;
      kind = GCC_C_SYMBOL_FUNCTION;
      break;

    default:
      type = builtin_type (objfile)->nodebug_unknown_symbol;
      kind = GCC_C_SYMBOL_VARIABLE;
      break;
    }

  sym_type = context->convert_type (type);
  decl = context->plugin ().build_decl (msym->natural_name (),
					kind, sym_type, NULL, addr,
					NULL, 0);
  context->plugin ().bind (decl, 1 /* is_global */);
}

/* See compile-internal.h.  */

void
gcc_convert_symbol (void *datum,
		    struct gcc_c_context *gcc_context,
		    enum gcc_c_oracle_request request,
		    const char *identifier)
{
  compile_c_instance *context
    = static_cast<compile_c_instance *> (datum);
  domain_enum domain;
  int found = 0;

  switch (request)
    {
    case GCC_C_ORACLE_SYMBOL:
      domain = VAR_DOMAIN;
      break;
    case GCC_C_ORACLE_TAG:
      domain = STRUCT_DOMAIN;
      break;
    case GCC_C_ORACLE_LABEL:
      domain = LABEL_DOMAIN;
      break;
    default:
      gdb_assert_not_reached ("Unrecognized oracle request.");
    }

  /* We can't allow exceptions to escape out of this callback.  Safest
     is to simply emit a gcc error.  */
  try
    {
      struct block_symbol sym;

      sym = lookup_symbol (identifier, context->block (), domain, NULL);
      if (sym.symbol != NULL)
	{
	  convert_symbol_sym (context, identifier, sym, domain);
	  found = 1;
	}
      else if (domain == VAR_DOMAIN)
	{
	  struct bound_minimal_symbol bmsym;

	  bmsym = lookup_minimal_symbol (identifier, NULL, NULL);
	  if (bmsym.minsym != NULL)
	    {
	      convert_symbol_bmsym (context, bmsym);
	      found = 1;
	    }
	}
    }

  catch (const gdb_exception &e)
    {
      context->plugin ().error (e.what ());
    }

  if (compile_debug && !found)
    gdb_printf (gdb_stdlog,
		"gcc_convert_symbol \"%s\": lookup_symbol failed\n",
		identifier);
  return;
}

/* See compile-internal.h.  */

gcc_address
gcc_symbol_address (void *datum, struct gcc_c_context *gcc_context,
		    const char *identifier)
{
  compile_c_instance *context
    = static_cast<compile_c_instance *> (datum);
  gcc_address result = 0;
  int found = 0;

  /* We can't allow exceptions to escape out of this callback.  Safest
     is to simply emit a gcc error.  */
  try
    {
      struct symbol *sym;

      /* We only need global functions here.  */
      sym = lookup_symbol (identifier, NULL, VAR_DOMAIN, NULL).symbol;
      if (sym != NULL && sym->aclass () == LOC_BLOCK)
	{
	  if (compile_debug)
	    gdb_printf (gdb_stdlog,
			"gcc_symbol_address \"%s\": full symbol\n",
			identifier);
	  result = sym->value_block ()->entry_pc ();
	  if (sym->type ()->is_gnu_ifunc ())
	    result = gnu_ifunc_resolve_addr (current_inferior ()->arch (),
					     result);
	  found = 1;
	}
      else
	{
	  struct bound_minimal_symbol msym;

	  msym = lookup_bound_minimal_symbol (identifier);
	  if (msym.minsym != NULL)
	    {
	      if (compile_debug)
		gdb_printf (gdb_stdlog,
			    "gcc_symbol_address \"%s\": minimal "
			    "symbol\n",
			    identifier);
	      result = msym.value_address ();
	      if (msym.minsym->type () == mst_text_gnu_ifunc)
		result = gnu_ifunc_resolve_addr (current_inferior ()->arch (),
						 result);
	      found = 1;
	    }
	}
    }

  catch (const gdb_exception_error &e)
    {
      context->plugin ().error (e.what ());
    }

  if (compile_debug && !found)
    gdb_printf (gdb_stdlog,
		"gcc_symbol_address \"%s\": failed\n",
		identifier);
  return result;
}



/* A hash function for symbol names.  */

static hashval_t
hash_symname (const void *a)
{
  const struct symbol *sym = (const struct symbol *) a;

  return htab_hash_string (sym->natural_name ());
}

/* A comparison function for hash tables that just looks at symbol
   names.  */

static int
eq_symname (const void *a, const void *b)
{
  const struct symbol *syma = (const struct symbol *) a;
  const struct symbol *symb = (const struct symbol *) b;

  return strcmp (syma->natural_name (), symb->natural_name ()) == 0;
}

/* If a symbol with the same name as SYM is already in HASHTAB, return
   1.  Otherwise, add SYM to HASHTAB and return 0.  */

static int
symbol_seen (htab_t hashtab, struct symbol *sym)
{
  void **slot;

  slot = htab_find_slot (hashtab, sym, INSERT);
  if (*slot != NULL)
    return 1;

  *slot = sym;
  return 0;
}

/* Generate C code to compute the length of a VLA.  */

static void
generate_vla_size (compile_instance *compiler,
		   string_file *stream,
		   struct gdbarch *gdbarch,
		   std::vector<bool> &registers_used,
		   CORE_ADDR pc,
		   struct type *type,
		   struct symbol *sym)
{
  type = check_typedef (type);

  if (TYPE_IS_REFERENCE (type))
    type = check_typedef (type->target_type ());

  switch (type->code ())
    {
    case TYPE_CODE_RANGE:
      {
	if (type->bounds ()->high.kind () == PROP_LOCEXPR
	    || type->bounds ()->high.kind () == PROP_LOCLIST)
	  {
	    const struct dynamic_prop *prop = &type->bounds ()->high;
	    std::string name = c_get_range_decl_name (prop);

	    dwarf2_compile_property_to_c (stream, name.c_str (),
					  gdbarch, registers_used,
					  prop, pc, sym);
	  }
      }
      break;

    case TYPE_CODE_ARRAY:
      generate_vla_size (compiler, stream, gdbarch, registers_used, pc,
			 type->index_type (), sym);
      generate_vla_size (compiler, stream, gdbarch, registers_used, pc,
			 type->target_type (), sym);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      {
	int i;

	for (i = 0; i < type->num_fields (); ++i)
	  if (!type->field (i).is_static ())
	    generate_vla_size (compiler, stream, gdbarch, registers_used, pc,
			       type->field (i).type (), sym);
      }
      break;
    }
}

/* Generate C code to compute the address of SYM.  */

static void
generate_c_for_for_one_variable (compile_instance *compiler,
				 string_file *stream,
				 struct gdbarch *gdbarch,
				 std::vector<bool> &registers_used,
				 CORE_ADDR pc,
				 struct symbol *sym)
{

  try
    {
      if (is_dynamic_type (sym->type ()))
	{
	  /* We need to emit to a temporary buffer in case an error
	     occurs in the middle.  */
	  string_file local_file;

	  generate_vla_size (compiler, &local_file, gdbarch, registers_used, pc,
			     sym->type (), sym);

	  stream->write (local_file.c_str (), local_file.size ());
	}

      if (SYMBOL_COMPUTED_OPS (sym) != NULL)
	{
	  gdb::unique_xmalloc_ptr<char> generated_name
	    = c_symbol_substitution_name (sym);
	  /* We need to emit to a temporary buffer in case an error
	     occurs in the middle.  */
	  string_file local_file;

	  SYMBOL_COMPUTED_OPS (sym)->generate_c_location (sym, &local_file,
							  gdbarch,
							  registers_used,
							  pc,
							  generated_name.get ());
	  stream->write (local_file.c_str (), local_file.size ());
	}
      else
	{
	  switch (sym->aclass ())
	    {
	    case LOC_REGISTER:
	    case LOC_ARG:
	    case LOC_REF_ARG:
	    case LOC_REGPARM_ADDR:
	    case LOC_LOCAL:
	      error (_("Local symbol unhandled when generating C code."));

	    case LOC_COMPUTED:
	      gdb_assert_not_reached ("LOC_COMPUTED variable "
				      "missing a method.");

	    default:
	      /* Nothing to do for all other cases, as they don't represent
		 local variables.  */
	      break;
	    }
	}
    }

  catch (const gdb_exception_error &e)
    {
      compiler->insert_symbol_error (sym, e.what ());
    }
}

/* See compile-c.h.  */

std::vector<bool>
generate_c_for_variable_locations (compile_instance *compiler,
				   string_file *stream,
				   struct gdbarch *gdbarch,
				   const struct block *block,
				   CORE_ADDR pc)
{
  if (block == nullptr)
    return {};

  const struct block *static_block = block->static_block ();

  /* If we're already in the static or global block, there is nothing
     to write.  */
  if (static_block == NULL || block == static_block)
    return {};

  std::vector<bool> registers_used (gdbarch_num_regs (gdbarch));

  /* Ensure that a given name is only entered once.  This reflects the
     reality of shadowing.  */
  htab_up symhash (htab_create_alloc (1, hash_symname, eq_symname, NULL,
				      xcalloc, xfree));

  while (1)
    {
      /* Iterate over symbols in this block, generating code to
	 compute the location of each local variable.  */
      for (struct symbol *sym : block_iterator_range (block))
	{
	  if (!symbol_seen (symhash.get (), sym))
	    generate_c_for_for_one_variable (compiler, stream, gdbarch,
					     registers_used, pc, sym);
	}

      /* If we just finished the outermost block of a function, we're
	 done.  */
      if (block->function () != NULL)
	break;
      block = block->superblock ();
    }

  return registers_used;
}
