/* MI Command Set - symbol commands.
   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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
#include "mi-cmds.h"
#include "symtab.h"
#include "objfiles.h"
#include "ui-out.h"
#include "source.h"
#include "mi-getopt.h"

/* Print the list of all pc addresses and lines of code for the
   provided (full or base) source file name.  The entries are sorted
   in ascending PC order.  */

void
mi_cmd_symbol_list_lines (const char *command, const char *const *argv,
			  int argc)
{
  struct gdbarch *gdbarch;
  const char *filename;
  struct symtab *s;
  int i;
  struct ui_out *uiout = current_uiout;

  if (argc != 1)
    error (_("-symbol-list-lines: Usage: SOURCE_FILENAME"));

  filename = argv[0];
  s = lookup_symtab (filename);

  if (s == NULL)
    error (_("-symbol-list-lines: Unknown source file name."));

  /* Now, dump the associated line table.  The pc addresses are
     already sorted by increasing values in the symbol table, so no
     need to perform any other sorting.  */

  struct objfile *objfile = s->compunit ()->objfile ();
  gdbarch = objfile->arch ();

  ui_out_emit_list list_emitter (uiout, "lines");
  if (s->linetable () != NULL && s->linetable ()->nitems > 0)
    for (i = 0; i < s->linetable ()->nitems; i++)
      {
	ui_out_emit_tuple tuple_emitter (uiout, NULL);
	uiout->field_core_addr ("pc", gdbarch,
				s->linetable ()->item[i].pc (objfile));
	uiout->field_signed ("line", s->linetable ()->item[i].line);
      }
}

/* Used by the -symbol-info-* and -symbol-info-module-* commands to print
   information about the symbol SYM in a block of index BLOCK (either
   GLOBAL_BLOCK or STATIC_BLOCK).  KIND is the kind of symbol we searched
   for in order to find SYM, which impact which fields are displayed in the
   results.  */

static void
output_debug_symbol (ui_out *uiout, enum search_domain kind,
		     struct symbol *sym, int block)
{
  ui_out_emit_tuple tuple_emitter (uiout, NULL);

  if (sym->line () != 0)
    uiout->field_unsigned ("line", sym->line ());
  uiout->field_string ("name", sym->print_name ());

  if (kind == FUNCTIONS_DOMAIN || kind == VARIABLES_DOMAIN)
    {
      string_file tmp_stream;
      type_print (sym->type (), "", &tmp_stream, -1);
      uiout->field_string ("type", tmp_stream.string ());

      std::string str = symbol_to_info_string (sym, block, kind);
      uiout->field_string ("description", str);
    }
}

/* Actually output one nondebug symbol, puts a tuple emitter in place
   and then outputs the fields for this msymbol.  */

static void
output_nondebug_symbol (ui_out *uiout,
			const struct bound_minimal_symbol &msymbol)
{
  struct gdbarch *gdbarch = msymbol.objfile->arch ();
  ui_out_emit_tuple tuple_emitter (uiout, NULL);

  uiout->field_core_addr ("address", gdbarch,
			  msymbol.value_address ());
  uiout->field_string ("name", msymbol.minsym->print_name ());
}

/* This is the guts of the commands '-symbol-info-functions',
   '-symbol-info-variables', and '-symbol-info-types'.  It searches for
   symbols matching KING, NAME_REGEXP, TYPE_REGEXP, and EXCLUDE_MINSYMS,
   and then prints the matching [m]symbols in an MI structured format.  */

static void
mi_symbol_info (enum search_domain kind, const char *name_regexp,
		const char *type_regexp, bool exclude_minsyms,
		size_t max_results)
{
  global_symbol_searcher sym_search (kind, name_regexp);
  sym_search.set_symbol_type_regexp (type_regexp);
  sym_search.set_exclude_minsyms (exclude_minsyms);
  sym_search.set_max_search_results (max_results);
  std::vector<symbol_search> symbols = sym_search.search ();
  ui_out *uiout = current_uiout;
  int i = 0;

  ui_out_emit_tuple outer_symbols_emitter (uiout, "symbols");

  /* Debug symbols are placed first. */
  if (i < symbols.size () && symbols[i].msymbol.minsym == nullptr)
    {
      ui_out_emit_list debug_symbols_list_emitter (uiout, "debug");

      /* As long as we have debug symbols...  */
      while (i < symbols.size () && symbols[i].msymbol.minsym == nullptr)
	{
	  symtab *symtab = symbols[i].symbol->symtab ();
	  ui_out_emit_tuple symtab_tuple_emitter (uiout, nullptr);

	  uiout->field_string ("filename",
			       symtab_to_filename_for_display (symtab));
	  uiout->field_string ("fullname", symtab_to_fullname (symtab));

	  ui_out_emit_list symbols_list_emitter (uiout, "symbols");

	  /* As long as we have debug symbols from this symtab...  */
	  for (; (i < symbols.size ()
		  && symbols[i].msymbol.minsym == nullptr
		  && symbols[i].symbol->symtab () == symtab);
	       ++i)
	    {
	      symbol_search &s = symbols[i];

	      output_debug_symbol (uiout, kind, s.symbol, s.block);
	    }
	}
    }

  /* Non-debug symbols are placed after.  */
  if (i < symbols.size ())
    {
      ui_out_emit_list nondebug_symbols_list_emitter (uiout, "nondebug");

      /* As long as we have nondebug symbols...  */
      for (; i < symbols.size (); i++)
	{
	  gdb_assert (symbols[i].msymbol.minsym != nullptr);
	  output_nondebug_symbol (uiout, symbols[i].msymbol);
	}
    }
}

/* Helper to parse the option text from an -max-results argument and return
   the parsed value.  If the text can't be parsed then an error is thrown.  */

static size_t
parse_max_results_option (const char *arg)
{
  char *ptr;
  long long val = strtoll (arg, &ptr, 10);
  if (arg == ptr || *ptr != '\0' || val > SIZE_MAX || val < 0)
    error (_("invalid value for --max-results argument"));
  size_t max_results = (size_t) val;

  return max_results;
}

/* Helper for mi_cmd_symbol_info_{functions,variables} - depending on KIND.
   Processes command line options from ARGV and ARGC.  */

static void
mi_info_functions_or_variables (enum search_domain kind,
				const char *const *argv, int argc)
{
  size_t max_results = SIZE_MAX;
  const char *regexp = nullptr;
  const char *t_regexp = nullptr;
  bool exclude_minsyms = true;

  enum opt
    {
     INCLUDE_NONDEBUG_OPT, TYPE_REGEXP_OPT, NAME_REGEXP_OPT, MAX_RESULTS_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"-include-nondebug" , INCLUDE_NONDEBUG_OPT, 0},
    {"-type", TYPE_REGEXP_OPT, 1},
    {"-name", NAME_REGEXP_OPT, 1},
    {"-max-results", MAX_RESULTS_OPT, 1},
    { 0, 0, 0 }
  };

  int oind = 0;
  const char *oarg = nullptr;

  while (1)
    {
      const char *cmd_string
	= ((kind == FUNCTIONS_DOMAIN)
	   ? "-symbol-info-functions" : "-symbol-info-variables");
      int opt = mi_getopt (cmd_string, argc, argv, opts, &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case INCLUDE_NONDEBUG_OPT:
	  exclude_minsyms = false;
	  break;
	case TYPE_REGEXP_OPT:
	  t_regexp = oarg;
	  break;
	case NAME_REGEXP_OPT:
	  regexp = oarg;
	  break;
	case MAX_RESULTS_OPT:
	  max_results = parse_max_results_option (oarg);
	  break;
	}
    }

  mi_symbol_info (kind, regexp, t_regexp, exclude_minsyms, max_results);
}

/* Type for an iterator over a vector of module_symbol_search results.  */
typedef std::vector<module_symbol_search>::const_iterator
	module_symbol_search_iterator;

/* Helper for mi_info_module_functions_or_variables.  Display the results
   from ITER up to END or until we find a symbol that is in a different
   module, or in a different symtab than the first symbol we print.  Update
   and return the new value for ITER.  */
static module_symbol_search_iterator
output_module_symbols_in_single_module_and_file
	(struct ui_out *uiout, module_symbol_search_iterator iter,
	 const module_symbol_search_iterator end, enum search_domain kind)
{
  /* The symbol for the module in which the first result resides.  */
  const symbol *first_module_symbol = iter->first.symbol;

  /* The symbol for the first result, and the symtab in which it resides.  */
  const symbol *first_result_symbol = iter->second.symbol;
  symtab *first_symbtab = first_result_symbol->symtab ();

  /* Formatted output.  */
  ui_out_emit_tuple current_file (uiout, nullptr);
  uiout->field_string ("filename",
		       symtab_to_filename_for_display (first_symbtab));
  uiout->field_string ("fullname", symtab_to_fullname (first_symbtab));
  ui_out_emit_list item_list (uiout, "symbols");

  /* Repeatedly output result symbols until either we run out of symbols,
     we change module, or we change symtab.  */
  for (; (iter != end
	  && first_module_symbol == iter->first.symbol
	  && first_symbtab == iter->second.symbol->symtab ());
       ++iter)
    output_debug_symbol (uiout, kind, iter->second.symbol,
			 iter->second.block);

  return iter;
}

/* Helper for mi_info_module_functions_or_variables.  Display the results
   from ITER up to END or until we find a symbol that is in a different
   module than the first symbol we print.  Update and return the new value
   for ITER.  */
static module_symbol_search_iterator
output_module_symbols_in_single_module
	(struct ui_out *uiout, module_symbol_search_iterator iter,
	 const module_symbol_search_iterator end, enum search_domain kind)
{
  gdb_assert (iter->first.symbol != nullptr);
  gdb_assert (iter->second.symbol != nullptr);

  /* The symbol for the module in which the first result resides.  */
  const symbol *first_module_symbol = iter->first.symbol;

  /* Create output formatting.  */
  ui_out_emit_tuple module_tuple (uiout, nullptr);
  uiout->field_string ("module", first_module_symbol->print_name ());
  ui_out_emit_list files_list (uiout, "files");

  /* The results are sorted so that symbols within the same file are next
     to each other in the list.  Calling the output function once will
     print all results within a single file.  We keep calling the output
     function until we change module.  */
  while (iter != end && first_module_symbol == iter->first.symbol)
    iter = output_module_symbols_in_single_module_and_file (uiout, iter,
							    end, kind);
  return iter;
}

/* Core of -symbol-info-module-functions and -symbol-info-module-variables.
   KIND indicates what we are searching for, and ARGV and ARGC are the
   command line options passed to the MI command.  */

static void
mi_info_module_functions_or_variables (enum search_domain kind,
					const char *const *argv, int argc)
{
  const char *module_regexp = nullptr;
  const char *regexp = nullptr;
  const char *type_regexp = nullptr;

  /* Process the command line options.  */

  enum opt
    {
     MODULE_REGEXP_OPT, TYPE_REGEXP_OPT, NAME_REGEXP_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"-module", MODULE_REGEXP_OPT, 1},
    {"-type", TYPE_REGEXP_OPT, 1},
    {"-name", NAME_REGEXP_OPT, 1},
    { 0, 0, 0 }
  };

  int oind = 0;
  const char *oarg = nullptr;

  while (1)
    {
      const char *cmd_string
	= ((kind == FUNCTIONS_DOMAIN)
	   ? "-symbol-info-module-functions"
	   : "-symbol-info-module-variables");
      int opt = mi_getopt (cmd_string, argc, argv, opts, &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case MODULE_REGEXP_OPT:
	  module_regexp = oarg;
	  break;
	case TYPE_REGEXP_OPT:
	  type_regexp = oarg;
	  break;
	case NAME_REGEXP_OPT:
	  regexp = oarg;
	  break;
	}
    }

  std::vector<module_symbol_search> module_symbols
    = search_module_symbols (module_regexp, regexp, type_regexp, kind);

  struct ui_out *uiout = current_uiout;
  ui_out_emit_list all_matching_symbols (uiout, "symbols");

  /* The results in the module_symbols list are ordered so symbols in the
     same module are next to each other.  Repeatedly call the output
     function to print sequences of symbols that are in the same module
     until we have no symbols left to print.  */
  module_symbol_search_iterator iter = module_symbols.begin ();
  const module_symbol_search_iterator end = module_symbols.end ();
  while (iter != end)
    iter = output_module_symbols_in_single_module (uiout, iter, end, kind);
}

/* Implement -symbol-info-functions command.  */

void
mi_cmd_symbol_info_functions (const char *command, const char *const *argv,
			      int argc)
{
  mi_info_functions_or_variables (FUNCTIONS_DOMAIN, argv, argc);
}

/* Implement -symbol-info-module-functions command.  */

void
mi_cmd_symbol_info_module_functions (const char *command,
				     const char *const *argv, int argc)
{
  mi_info_module_functions_or_variables (FUNCTIONS_DOMAIN, argv, argc);
}

/* Implement -symbol-info-module-variables command.  */

void
mi_cmd_symbol_info_module_variables (const char *command,
				     const char *const *argv, int argc)
{
  mi_info_module_functions_or_variables (VARIABLES_DOMAIN, argv, argc);
}

/* Implement -symbol-inf-modules command.  */

void
mi_cmd_symbol_info_modules (const char *command, const char *const *argv,
			    int argc)
{
  size_t max_results = SIZE_MAX;
  const char *regexp = nullptr;

  enum opt
    {
     NAME_REGEXP_OPT, MAX_RESULTS_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"-name", NAME_REGEXP_OPT, 1},
    {"-max-results", MAX_RESULTS_OPT, 1},
    { 0, 0, 0 }
  };

  int oind = 0;
  const char *oarg = nullptr;

  while (1)
    {
      int opt = mi_getopt ("-symbol-info-modules", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case NAME_REGEXP_OPT:
	  regexp = oarg;
	  break;
	case MAX_RESULTS_OPT:
	  max_results = parse_max_results_option (oarg);
	  break;
	}
    }

  mi_symbol_info (MODULES_DOMAIN, regexp, nullptr, true, max_results);
}

/* Implement -symbol-info-types command.  */

void
mi_cmd_symbol_info_types (const char *command, const char *const *argv,
			  int argc)
{
  size_t max_results = SIZE_MAX;
  const char *regexp = nullptr;

  enum opt
    {
     NAME_REGEXP_OPT, MAX_RESULTS_OPT
    };
  static const struct mi_opt opts[] =
  {
    {"-name", NAME_REGEXP_OPT, 1},
    {"-max-results", MAX_RESULTS_OPT, 1},
    { 0, 0, 0 }
  };

  int oind = 0;
  const char *oarg = nullptr;

  while (true)
    {
      int opt = mi_getopt ("-symbol-info-types", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case NAME_REGEXP_OPT:
	  regexp = oarg;
	  break;
	case MAX_RESULTS_OPT:
	  max_results = parse_max_results_option (oarg);
	  break;
	}
    }

  mi_symbol_info (TYPES_DOMAIN, regexp, nullptr, true, max_results);
}

/* Implement -symbol-info-variables command.  */

void
mi_cmd_symbol_info_variables (const char *command, const char *const *argv,
			      int argc)
{
  mi_info_functions_or_variables (VARIABLES_DOMAIN, argv, argc);
}
