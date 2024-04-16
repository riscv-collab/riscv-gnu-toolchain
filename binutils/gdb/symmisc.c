/* Do various things to symbol tables (other than lookup), for GDB.

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
#include "bfd.h"
#include "filenames.h"
#include "symfile.h"
#include "objfiles.h"
#include "breakpoint.h"
#include "command.h"
#include "gdbsupport/gdb_obstack.h"
#include "language.h"
#include "bcache.h"
#include "block.h"
#include "gdbsupport/gdb_regex.h"
#include <sys/stat.h>
#include "dictionary.h"
#include "typeprint.h"
#include "gdbcmd.h"
#include "source.h"
#include "readline/tilde.h"
#include <cli/cli-style.h>
#include "gdbsupport/buildargv.h"

/* Prototypes for local functions */

static int block_depth (const struct block *);

static void print_symbol (struct gdbarch *gdbarch, struct symbol *symbol,
			  int depth, ui_file *outfile);


void
print_objfile_statistics (void)
{
  int i, linetables, blockvectors;

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	QUIT;
	gdb_printf (_("Statistics for '%s':\n"), objfile_name (objfile));
	if (OBJSTAT (objfile, n_stabs) > 0)
	  gdb_printf (_("  Number of \"stab\" symbols read: %d\n"),
		      OBJSTAT (objfile, n_stabs));
	if (objfile->per_bfd->n_minsyms > 0)
	  gdb_printf (_("  Number of \"minimal\" symbols read: %d\n"),
		      objfile->per_bfd->n_minsyms);
	if (OBJSTAT (objfile, n_syms) > 0)
	  gdb_printf (_("  Number of \"full\" symbols read: %d\n"),
		      OBJSTAT (objfile, n_syms));
	if (OBJSTAT (objfile, n_types) > 0)
	  gdb_printf (_("  Number of \"types\" defined: %d\n"),
		      OBJSTAT (objfile, n_types));

	i = linetables = 0;
	for (compunit_symtab *cu : objfile->compunits ())
	  {
	    for (symtab *s : cu->filetabs ())
	      {
		i++;
		if (s->linetable () != NULL)
		  linetables++;
	      }
	  }
	blockvectors = std::distance (objfile->compunits ().begin (),
				      objfile->compunits ().end ());
	gdb_printf (_("  Number of symbol tables: %d\n"), i);
	gdb_printf (_("  Number of symbol tables with line tables: %d\n"),
		    linetables);
	gdb_printf (_("  Number of symbol tables with blockvectors: %d\n"),
		    blockvectors);

	objfile->print_stats (false);

	if (OBJSTAT (objfile, sz_strtab) > 0)
	  gdb_printf (_("  Space used by string tables: %d\n"),
		      OBJSTAT (objfile, sz_strtab));
	gdb_printf (_("  Total memory used for objfile obstack: %s\n"),
		    pulongest (obstack_memory_used (&objfile
						    ->objfile_obstack)));
	gdb_printf (_("  Total memory used for BFD obstack: %s\n"),
		    pulongest (obstack_memory_used (&objfile->per_bfd
						    ->storage_obstack)));

	gdb_printf (_("  Total memory used for string cache: %d\n"),
		    objfile->per_bfd->string_cache.memory_used ());
	gdb_printf (_("Byte cache statistics for '%s':\n"),
		    objfile_name (objfile));
	objfile->per_bfd->string_cache.print_statistics ("string cache");
	objfile->print_stats (true);
      }
}

static void
dump_objfile (struct objfile *objfile)
{
  gdb_printf ("\nObject file %s:  ", objfile_name (objfile));
  gdb_printf ("Objfile at %s, bfd at %s, %d minsyms\n\n",
	      host_address_to_string (objfile),
	      host_address_to_string (objfile->obfd.get ()),
	      objfile->per_bfd->minimal_symbol_count);

  objfile->dump ();

  if (objfile->compunit_symtabs != NULL)
    {
      gdb_printf ("Symtabs:\n");
      for (compunit_symtab *cu : objfile->compunits ())
	{
	  for (symtab *symtab : cu->filetabs ())
	    {
	      gdb_printf ("%s at %s",
			  symtab_to_filename_for_display (symtab),
			  host_address_to_string (symtab));
	      if (symtab->compunit ()->objfile () != objfile)
		gdb_printf (", NOT ON CHAIN!");
	      gdb_printf ("\n");
	    }
	}
      gdb_printf ("\n\n");
    }
}

/* Print minimal symbols from this objfile.  */

static void
dump_msymbols (struct objfile *objfile, struct ui_file *outfile)
{
  struct gdbarch *gdbarch = objfile->arch ();
  int index;
  char ms_type;

  gdb_printf (outfile, "\nObject file %s:\n\n", objfile_name (objfile));
  if (objfile->per_bfd->minimal_symbol_count == 0)
    {
      gdb_printf (outfile, "No minimal symbols found.\n");
      return;
    }
  index = 0;
  for (minimal_symbol *msymbol : objfile->msymbols ())
    {
      struct obj_section *section = msymbol->obj_section (objfile);

      switch (msymbol->type ())
	{
	case mst_unknown:
	  ms_type = 'u';
	  break;
	case mst_text:
	  ms_type = 'T';
	  break;
	case mst_text_gnu_ifunc:
	case mst_data_gnu_ifunc:
	  ms_type = 'i';
	  break;
	case mst_solib_trampoline:
	  ms_type = 'S';
	  break;
	case mst_data:
	  ms_type = 'D';
	  break;
	case mst_bss:
	  ms_type = 'B';
	  break;
	case mst_abs:
	  ms_type = 'A';
	  break;
	case mst_file_text:
	  ms_type = 't';
	  break;
	case mst_file_data:
	  ms_type = 'd';
	  break;
	case mst_file_bss:
	  ms_type = 'b';
	  break;
	default:
	  ms_type = '?';
	  break;
	}
      gdb_printf (outfile, "[%2d] %c ", index, ms_type);

      /* Use the relocated address as shown in the symbol here -- do
	 not try to respect copy relocations.  */
      CORE_ADDR addr = (CORE_ADDR (msymbol->unrelocated_address ())
			+ objfile->section_offsets[msymbol->section_index ()]);
      gdb_puts (paddress (gdbarch, addr), outfile);
      gdb_printf (outfile, " %s", msymbol->linkage_name ());
      if (section)
	{
	  if (section->the_bfd_section != NULL)
	    gdb_printf (outfile, " section %s",
			bfd_section_name (section->the_bfd_section));
	  else
	    gdb_printf (outfile, " spurious section %ld",
			(long) (section - objfile->sections_start));
	}
      if (msymbol->demangled_name () != NULL)
	{
	  gdb_printf (outfile, "  %s", msymbol->demangled_name ());
	}
      if (msymbol->filename)
	gdb_printf (outfile, "  %s", msymbol->filename);
      gdb_puts ("\n", outfile);
      index++;
    }
  if (objfile->per_bfd->minimal_symbol_count != index)
    {
      warning (_("internal error:  minimal symbol count %d != %d"),
	       objfile->per_bfd->minimal_symbol_count, index);
    }
  gdb_printf (outfile, "\n");
}

static void
dump_symtab_1 (struct symtab *symtab, struct ui_file *outfile)
{
  struct objfile *objfile = symtab->compunit ()->objfile ();
  struct gdbarch *gdbarch = objfile->arch ();
  const struct linetable *l;
  int depth;

  gdb_printf (outfile, "\nSymtab for file %s at %s\n",
	      symtab_to_filename_for_display (symtab),
	      host_address_to_string (symtab));

  if (symtab->compunit ()->dirname () != NULL)
    gdb_printf (outfile, "Compilation directory is %s\n",
		symtab->compunit ()->dirname ());
  gdb_printf (outfile, "Read from object file %s (%s)\n",
	      objfile_name (objfile),
	      host_address_to_string (objfile));
  gdb_printf (outfile, "Language: %s\n",
	      language_str (symtab->language ()));

  /* First print the line table.  */
  l = symtab->linetable ();
  if (l)
    {
      gdb_printf (outfile, "\nLine table:\n\n");
      int len = l->nitems;
      for (int i = 0; i < len; i++)
	{
	  gdb_printf (outfile, " line %d at ", l->item[i].line);
	  gdb_puts (paddress (gdbarch, l->item[i].pc (objfile)), outfile);
	  if (l->item[i].is_stmt)
	    gdb_printf (outfile, "\t(stmt)");
	  gdb_printf (outfile, "\n");
	}
    }
  /* Now print the block info, but only for compunit symtabs since we will
     print lots of duplicate info otherwise.  */
  if (is_main_symtab_of_compunit_symtab (symtab))
    {
      gdb_printf (outfile, "\nBlockvector:\n\n");
      const blockvector *bv = symtab->compunit ()->blockvector ();
      for (int i = 0; i < bv->num_blocks (); i++)
	{
	  const block *b = bv->block (i);
	  depth = block_depth (b) * 2;
	  gdb_printf (outfile, "%*sblock #%03d, object at %s",
		      depth, "", i,
		      host_address_to_string (b));
	  if (b->superblock ())
	    gdb_printf (outfile, " under %s",
			host_address_to_string (b->superblock ()));
	  /* drow/2002-07-10: We could save the total symbols count
	     even if we're using a hashtable, but nothing else but this message
	     wants it.  */
	  gdb_printf (outfile, ", %d symbols in ",
		      mdict_size (b->multidict ()));
	  gdb_puts (paddress (gdbarch, b->start ()), outfile);
	  gdb_printf (outfile, "..");
	  gdb_puts (paddress (gdbarch, b->end ()), outfile);
	  if (b->function ())
	    {
	      gdb_printf (outfile, ", function %s",
			  b->function ()->linkage_name ());
	      if (b->function ()->demangled_name () != NULL)
		{
		  gdb_printf (outfile, ", %s",
			      b->function ()->demangled_name ());
		}
	    }
	  gdb_printf (outfile, "\n");
	  /* Now print each symbol in this block (in no particular order, if
	     we're using a hashtable).  Note that we only want this
	     block, not any blocks from included symtabs.  */
	  for (struct symbol *sym : b->multidict_symbols ())
	    {
	      try
		{
		  print_symbol (gdbarch, sym, depth + 1, outfile);
		}
	      catch (const gdb_exception_error &ex)
		{
		  exception_fprintf (gdb_stderr, ex,
				     "Error printing symbol:\n");
		}
	    }
	}
      gdb_printf (outfile, "\n");
    }
  else
    {
      compunit_symtab *compunit = symtab->compunit ();
      const char *compunit_filename
	= symtab_to_filename_for_display (compunit->primary_filetab ());

      gdb_printf (outfile,
		  "\nBlockvector same as owning compunit: %s\n\n",
		  compunit_filename);
    }

  /* Print info about the user of this compunit_symtab, and the
     compunit_symtabs included by this one. */
  if (is_main_symtab_of_compunit_symtab (symtab))
    {
      struct compunit_symtab *cust = symtab->compunit ();

      if (cust->user != nullptr)
	{
	  const char *addr
	    = host_address_to_string (cust->user->primary_filetab ());
	  gdb_printf (outfile, "Compunit user: %s\n", addr);
	}
      if (cust->includes != nullptr)
	for (int i = 0; ; ++i)
	  {
	    struct compunit_symtab *include = cust->includes[i];
	    if (include == nullptr)
	      break;
	    const char *addr
	      = host_address_to_string (include->primary_filetab ());
	    gdb_printf (outfile, "Compunit include: %s\n", addr);
	  }
    }
}

static void
dump_symtab (struct symtab *symtab, struct ui_file *outfile)
{
  /* Set the current language to the language of the symtab we're dumping
     because certain routines used during dump_symtab() use the current
     language to print an image of the symbol.  We'll restore it later.
     But use only real languages, not placeholders.  */
  if (symtab->language () != language_unknown)
    {
      scoped_restore_current_language save_lang;
      set_language (symtab->language ());
      dump_symtab_1 (symtab, outfile);
    }
  else
    dump_symtab_1 (symtab, outfile);
}

static void
maintenance_print_symbols (const char *args, int from_tty)
{
  struct ui_file *outfile = gdb_stdout;
  char *address_arg = NULL, *source_arg = NULL, *objfile_arg = NULL;
  int i, outfile_idx;

  dont_repeat ();

  gdb_argv argv (args);

  for (i = 0; argv != NULL && argv[i] != NULL; ++i)
    {
      if (strcmp (argv[i], "-pc") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing pc value"));
	  address_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-source") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing source file"));
	  source_arg = argv[++i];
	}
      else if (strcmp (argv[i], "-objfile") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing objfile name"));
	  objfile_arg = argv[++i];
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  /* End of options.  */
	  ++i;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  /* Future proofing: Don't allow OUTFILE to begin with "-".  */
	  error (_("Unknown option: %s"), argv[i]);
	}
      else
	break;
    }
  outfile_idx = i;

  if (address_arg != NULL && source_arg != NULL)
    error (_("Must specify at most one of -pc and -source"));

  stdio_file arg_outfile;

  if (argv != NULL && argv[outfile_idx] != NULL)
    {
      if (argv[outfile_idx + 1] != NULL)
	error (_("Junk at end of command"));
      gdb::unique_xmalloc_ptr<char> outfile_name
	(tilde_expand (argv[outfile_idx]));
      if (!arg_outfile.open (outfile_name.get (), FOPEN_WT))
	perror_with_name (outfile_name.get ());
      outfile = &arg_outfile;
    }

  if (address_arg != NULL)
    {
      CORE_ADDR pc = parse_and_eval_address (address_arg);
      struct symtab *s = find_pc_line_symtab (pc);

      if (s == NULL)
	error (_("No symtab for address: %s"), address_arg);
      dump_symtab (s, outfile);
    }
  else
    {
      int found = 0;

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  int print_for_objfile = 1;

	  if (objfile_arg != NULL)
	    print_for_objfile
	      = compare_filenames_for_search (objfile_name (objfile),
					      objfile_arg);
	  if (!print_for_objfile)
	    continue;

	  for (compunit_symtab *cu : objfile->compunits ())
	    {
	      for (symtab *s : cu->filetabs ())
		{
		  int print_for_source = 0;

		  QUIT;
		  if (source_arg != NULL)
		    {
		      print_for_source
			= compare_filenames_for_search
			(symtab_to_filename_for_display (s), source_arg);
		      found = 1;
		    }
		  if (source_arg == NULL
		      || print_for_source)
		    dump_symtab (s, outfile);
		}
	    }
	}

      if (source_arg != NULL && !found)
	error (_("No symtab for source file: %s"), source_arg);
    }
}

/* Print symbol SYMBOL on OUTFILE.  DEPTH says how far to indent.  */

static void
print_symbol (struct gdbarch *gdbarch, struct symbol *symbol,
	      int depth, ui_file *outfile)
{
  struct obj_section *section;

  if (symbol->is_objfile_owned ())
    section = symbol->obj_section (symbol->objfile ());
  else
    section = NULL;

  print_spaces (depth, outfile);
  if (symbol->domain () == LABEL_DOMAIN)
    {
      gdb_printf (outfile, "label %s at ", symbol->print_name ());
      gdb_puts (paddress (gdbarch, symbol->value_address ()),
		outfile);
      if (section)
	gdb_printf (outfile, " section %s\n",
		    bfd_section_name (section->the_bfd_section));
      else
	gdb_printf (outfile, "\n");
      return;
    }

  if (symbol->domain () == STRUCT_DOMAIN)
    {
      if (symbol->type ()->name ())
	{
	  current_language->print_type (symbol->type (), "", outfile, 1, depth,
					&type_print_raw_options);
	}
      else
	{
	  gdb_printf (outfile, "%s %s = ",
		      (symbol->type ()->code () == TYPE_CODE_ENUM
		       ? "enum"
		       : (symbol->type ()->code () == TYPE_CODE_STRUCT
			  ? "struct" : "union")),
		      symbol->linkage_name ());
	  current_language->print_type (symbol->type (), "", outfile, 1, depth,
					&type_print_raw_options);
	}
      gdb_printf (outfile, ";\n");
    }
  else
    {
      if (symbol->aclass () == LOC_TYPEDEF)
	gdb_printf (outfile, "typedef ");
      if (symbol->type ())
	{
	  /* Print details of types, except for enums where it's clutter.  */
	  current_language->print_type (symbol->type (), symbol->print_name (),
					outfile,
					symbol->type ()->code () != TYPE_CODE_ENUM,
					depth,
					&type_print_raw_options);
	  gdb_printf (outfile, "; ");
	}
      else
	gdb_printf (outfile, "%s ", symbol->print_name ());

      switch (symbol->aclass ())
	{
	case LOC_CONST:
	  gdb_printf (outfile, "const %s (%s)",
		      plongest (symbol->value_longest ()),
		      hex_string (symbol->value_longest ()));
	  break;

	case LOC_CONST_BYTES:
	  {
	    unsigned i;
	    struct type *type = check_typedef (symbol->type ());

	    gdb_printf (outfile, "const %s hex bytes:",
			pulongest (type->length ()));
	    for (i = 0; i < type->length (); i++)
	      gdb_printf (outfile, " %02x",
			  (unsigned) symbol->value_bytes ()[i]);
	  }
	  break;

	case LOC_STATIC:
	  gdb_printf (outfile, "static at ");
	  gdb_puts (paddress (gdbarch, symbol->value_address ()), outfile);
	  if (section)
	    gdb_printf (outfile, " section %s",
			bfd_section_name (section->the_bfd_section));
	  break;

	case LOC_REGISTER:
	  if (symbol->is_argument ())
	    gdb_printf (outfile, "parameter register %s",
			plongest (symbol->value_longest ()));
	  else
	    gdb_printf (outfile, "register %s",
			plongest (symbol->value_longest ()));
	  break;

	case LOC_ARG:
	  gdb_printf (outfile, "arg at offset %s",
		      hex_string (symbol->value_longest ()));
	  break;

	case LOC_REF_ARG:
	  gdb_printf (outfile, "reference arg at %s",
		      hex_string (symbol->value_longest ()));
	  break;

	case LOC_REGPARM_ADDR:
	  gdb_printf (outfile, "address parameter register %s",
		      plongest (symbol->value_longest ()));
	  break;

	case LOC_LOCAL:
	  gdb_printf (outfile, "local at offset %s",
		      hex_string (symbol->value_longest ()));
	  break;

	case LOC_TYPEDEF:
	  break;

	case LOC_LABEL:
	  gdb_printf (outfile, "label at ");
	  gdb_puts (paddress (gdbarch, symbol->value_address ()), outfile);
	  if (section)
	    gdb_printf (outfile, " section %s",
			bfd_section_name (section->the_bfd_section));
	  break;

	case LOC_BLOCK:
	  gdb_printf
	    (outfile, "block object %s, %s..%s",
	     host_address_to_string (symbol->value_block ()),
	     paddress (gdbarch, symbol->value_block()->start ()),
	     paddress (gdbarch, symbol->value_block()->end ()));
	  if (section)
	    gdb_printf (outfile, " section %s",
			bfd_section_name (section->the_bfd_section));
	  break;

	case LOC_COMPUTED:
	  gdb_printf (outfile, "computed at runtime");
	  break;

	case LOC_UNRESOLVED:
	  gdb_printf (outfile, "unresolved");
	  break;

	case LOC_OPTIMIZED_OUT:
	  gdb_printf (outfile, "optimized out");
	  break;

	default:
	  gdb_printf (outfile, "botched symbol class %x",
		      symbol->aclass ());
	  break;
	}
    }
  gdb_printf (outfile, "\n");
}

static void
maintenance_print_msymbols (const char *args, int from_tty)
{
  struct ui_file *outfile = gdb_stdout;
  char *objfile_arg = NULL;
  int i, outfile_idx;

  dont_repeat ();

  gdb_argv argv (args);

  for (i = 0; argv != NULL && argv[i] != NULL; ++i)
    {
      if (strcmp (argv[i], "-objfile") == 0)
	{
	  if (argv[i + 1] == NULL)
	    error (_("Missing objfile name"));
	  objfile_arg = argv[++i];
	}
      else if (strcmp (argv[i], "--") == 0)
	{
	  /* End of options.  */
	  ++i;
	  break;
	}
      else if (argv[i][0] == '-')
	{
	  /* Future proofing: Don't allow OUTFILE to begin with "-".  */
	  error (_("Unknown option: %s"), argv[i]);
	}
      else
	break;
    }
  outfile_idx = i;

  stdio_file arg_outfile;

  if (argv != NULL && argv[outfile_idx] != NULL)
    {
      if (argv[outfile_idx + 1] != NULL)
	error (_("Junk at end of command"));
      gdb::unique_xmalloc_ptr<char> outfile_name
	(tilde_expand (argv[outfile_idx]));
      if (!arg_outfile.open (outfile_name.get (), FOPEN_WT))
	perror_with_name (outfile_name.get ());
      outfile = &arg_outfile;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      QUIT;
      if (objfile_arg == NULL
	  || compare_filenames_for_search (objfile_name (objfile), objfile_arg))
	dump_msymbols (objfile, outfile);
    }
}

static void
maintenance_print_objfiles (const char *regexp, int from_tty)
{
  dont_repeat ();

  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	QUIT;
	if (! regexp
	    || re_exec (objfile_name (objfile)))
	  dump_objfile (objfile);
      }
}

/* List all the symbol tables whose names match REGEXP (optional).  */

static void
maintenance_info_symtabs (const char *regexp, int from_tty)
{
  dont_repeat ();

  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	/* We don't want to print anything for this objfile until we
	   actually find a symtab whose name matches.  */
	int printed_objfile_start = 0;

	for (compunit_symtab *cust : objfile->compunits ())
	  {
	    int printed_compunit_symtab_start = 0;

	    for (symtab *symtab : cust->filetabs ())
	      {
		QUIT;

		if (! regexp
		    || re_exec (symtab_to_filename_for_display (symtab)))
		  {
		    if (! printed_objfile_start)
		      {
			gdb_printf ("{ objfile %s ", objfile_name (objfile));
			gdb_stdout->wrap_here (2);
			gdb_printf ("((struct objfile *) %s)\n",
				    host_address_to_string (objfile));
			printed_objfile_start = 1;
		      }
		    if (! printed_compunit_symtab_start)
		      {
			gdb_printf ("  { ((struct compunit_symtab *) %s)\n",
				    host_address_to_string (cust));
			gdb_printf ("    debugformat %s\n",
				    cust->debugformat ());
			gdb_printf ("    producer %s\n",
				    (cust->producer () != nullptr
				     ? cust->producer () : "(null)"));
			gdb_printf ("    name %s\n", cust->name);
			gdb_printf ("    dirname %s\n",
				    (cust->dirname () != NULL
				     ? cust->dirname () : "(null)"));
			gdb_printf ("    blockvector"
				    " ((struct blockvector *) %s)\n",
				    host_address_to_string
				    (cust->blockvector ()));
			gdb_printf ("    user"
				    " ((struct compunit_symtab *) %s)\n",
				    cust->user != nullptr
				    ? host_address_to_string (cust->user)
				    : "(null)");
			if (cust->includes != nullptr)
			  {
			    gdb_printf ("    ( includes\n");
			    for (int i = 0; ; ++i)
			      {
				struct compunit_symtab *include
				  = cust->includes[i];
				if (include == nullptr)
				  break;
				const char *addr
				  = host_address_to_string (include);
				gdb_printf ("      (%s %s)\n",
					    "(struct compunit_symtab *)",
					    addr);
			      }
			    gdb_printf ("    )\n");
			  }
			printed_compunit_symtab_start = 1;
		      }

		    gdb_printf ("\t{ symtab %s ",
				symtab_to_filename_for_display (symtab));
		    gdb_stdout->wrap_here (4);
		    gdb_printf ("((struct symtab *) %s)\n",
				host_address_to_string (symtab));
		    gdb_printf ("\t  fullname %s\n",
				symtab->fullname != NULL
				? symtab->fullname
				: "(null)");
		    gdb_printf ("\t  "
				"linetable ((struct linetable *) %s)\n",
				host_address_to_string
				(symtab->linetable ()));
		    gdb_printf ("\t}\n");
		  }
	      }

	    if (printed_compunit_symtab_start)
	      gdb_printf ("  }\n");
	  }

	if (printed_objfile_start)
	  gdb_printf ("}\n");
      }
}

/* Check consistency of symtabs.
   An example of what this checks for is NULL blockvectors.
   They can happen if there's a bug during debug info reading.
   GDB assumes they are always non-NULL.

   Note: This does not check for psymtab vs symtab consistency.
   Use "maint check-psymtabs" for that.  */

static void
maintenance_check_symtabs (const char *ignore, int from_tty)
{
  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	/* We don't want to print anything for this objfile until we
	   actually find something worth printing.  */
	int printed_objfile_start = 0;

	for (compunit_symtab *cust : objfile->compunits ())
	  {
	    int found_something = 0;
	    struct symtab *symtab = cust->primary_filetab ();

	    QUIT;

	    if (cust->blockvector () == NULL)
	      found_something = 1;
	    /* Add more checks here.  */

	    if (found_something)
	      {
		if (! printed_objfile_start)
		  {
		    gdb_printf ("{ objfile %s ", objfile_name (objfile));
		    gdb_stdout->wrap_here (2);
		    gdb_printf ("((struct objfile *) %s)\n",
				host_address_to_string (objfile));
		    printed_objfile_start = 1;
		  }
		gdb_printf ("  { symtab %s\n",
			    symtab_to_filename_for_display (symtab));
		if (cust->blockvector () == NULL)
		  gdb_printf ("    NULL blockvector\n");
		gdb_printf ("  }\n");
	      }
	  }

	if (printed_objfile_start)
	  gdb_printf ("}\n");
      }
}

/* Expand all symbol tables whose name matches an optional regexp.  */

static void
maintenance_expand_symtabs (const char *args, int from_tty)
{
  char *regexp = NULL;

  /* We use buildargv here so that we handle spaces in the regexp
     in a way that allows adding more arguments later.  */
  gdb_argv argv (args);

  if (argv != NULL)
    {
      if (argv[0] != NULL)
	{
	  regexp = argv[0];
	  if (argv[1] != NULL)
	    error (_("Extra arguments after regexp."));
	}
    }

  if (regexp)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      objfile->expand_symtabs_matching
	([&] (const char *filename, bool basenames)
	 {
	   /* KISS: Only apply the regexp to the complete file name.  */
	   return (!basenames
		   && (regexp == NULL || re_exec (filename)));
	 },
	 NULL,
	 NULL,
	 NULL,
	 SEARCH_GLOBAL_BLOCK | SEARCH_STATIC_BLOCK,
	 UNDEF_DOMAIN,
	 ALL_DOMAIN);
}


/* Return the nexting depth of a block within other blocks in its symtab.  */

static int
block_depth (const struct block *block)
{
  int i = 0;

  while ((block = block->superblock ()) != NULL)
    {
      i++;
    }
  return i;
}


/* Used by MAINTENANCE_INFO_LINE_TABLES to print the information about a
   single line table.  */

static int
maintenance_print_one_line_table (struct symtab *symtab, void *data)
{
  const struct linetable *linetable;
  struct objfile *objfile;

  objfile = symtab->compunit ()->objfile ();
  gdb_printf (_("objfile: %ps ((struct objfile *) %s)\n"),
	      styled_string (file_name_style.style (),
			     objfile_name (objfile)),
	      host_address_to_string (objfile));
  gdb_printf (_("compunit_symtab: %s ((struct compunit_symtab *) %s)\n"),
	      symtab->compunit ()->name,
	      host_address_to_string (symtab->compunit ()));
  gdb_printf (_("symtab: %ps ((struct symtab *) %s)\n"),
	      styled_string (file_name_style.style (),
			     symtab_to_fullname (symtab)),
	      host_address_to_string (symtab));
  linetable = symtab->linetable ();
  gdb_printf (_("linetable: ((struct linetable *) %s):\n"),
	      host_address_to_string (linetable));

  if (linetable == NULL)
    gdb_printf (_("No line table.\n"));
  else if (linetable->nitems <= 0)
    gdb_printf (_("Line table has no lines.\n"));
  else
    {
      /* Leave space for 6 digits of index and line number.  After that the
	 tables will just not format as well.  */
      struct ui_out *uiout = current_uiout;
      ui_out_emit_table table_emitter (uiout, 7, -1, "line-table");
      uiout->table_header (6, ui_left, "index", _("INDEX"));
      uiout->table_header (6, ui_left, "line", _("LINE"));
      uiout->table_header (18, ui_left, "rel-address", _("REL-ADDRESS"));
      uiout->table_header (18, ui_left, "unrel-address", _("UNREL-ADDRESS"));
      uiout->table_header (7, ui_left, "is-stmt", _("IS-STMT"));
      uiout->table_header (12, ui_left, "prologue-end", _("PROLOGUE-END"));
      uiout->table_header (14, ui_left, "epilogue-begin", _("EPILOGUE-BEGIN"));
      uiout->table_body ();

      for (int i = 0; i < linetable->nitems; ++i)
	{
	  const linetable_entry *item;

	  item = &linetable->item [i];
	  ui_out_emit_tuple tuple_emitter (uiout, nullptr);
	  uiout->field_signed ("index", i);
	  if (item->line > 0)
	    uiout->field_signed ("line", item->line);
	  else
	    uiout->field_string ("line", _("END"));
	  uiout->field_core_addr ("rel-address", objfile->arch (),
				  item->pc (objfile));
	  uiout->field_core_addr ("unrel-address", objfile->arch (),
				  CORE_ADDR (item->unrelocated_pc ()));
	  uiout->field_string ("is-stmt", item->is_stmt ? "Y" : "");
	  uiout->field_string ("prologue-end", item->prologue_end ? "Y" : "");
	  uiout->field_string ("epilogue-begin", item->epilogue_begin ? "Y" : "");
	  uiout->text ("\n");
	}
    }

  return 0;
}

/* Implement the 'maint info line-table' command.  */

static void
maintenance_info_line_tables (const char *regexp, int from_tty)
{
  dont_repeat ();

  if (regexp != NULL)
    re_comp (regexp);

  for (struct program_space *pspace : program_spaces)
    for (objfile *objfile : pspace->objfiles ())
      {
	for (compunit_symtab *cust : objfile->compunits ())
	  {
	    for (symtab *symtab : cust->filetabs ())
	      {
		QUIT;

		if (regexp == NULL
		    || re_exec (symtab_to_filename_for_display (symtab)))
		  {
		    maintenance_print_one_line_table (symtab, NULL);
		    gdb_printf ("\n");
		  }
	      }
	  }
      }
}



/* Do early runtime initializations.  */

void _initialize_symmisc ();
void
_initialize_symmisc ()
{
  add_cmd ("symbols", class_maintenance, maintenance_print_symbols, _("\
Print dump of current symbol definitions.\n\
Usage: mt print symbols [-pc ADDRESS] [--] [OUTFILE]\n\
       mt print symbols [-objfile OBJFILE] [-source SOURCE] [--] [OUTFILE]\n\
Entries in the full symbol table are dumped to file OUTFILE,\n\
or the terminal if OUTFILE is unspecified.\n\
If ADDRESS is provided, dump only the symbols for the file with code at that address.\n\
If SOURCE is provided, dump only that file's symbols.\n\
If OBJFILE is provided, dump only that object file's symbols."),
	   &maintenanceprintlist);

  add_cmd ("msymbols", class_maintenance, maintenance_print_msymbols, _("\
Print dump of current minimal symbol definitions.\n\
Usage: mt print msymbols [-objfile OBJFILE] [--] [OUTFILE]\n\
Entries in the minimal symbol table are dumped to file OUTFILE,\n\
or the terminal if OUTFILE is unspecified.\n\
If OBJFILE is provided, dump only that file's minimal symbols."),
	   &maintenanceprintlist);

  add_cmd ("objfiles", class_maintenance, maintenance_print_objfiles,
	   _("Print dump of current object file definitions.\n\
With an argument REGEXP, list the object files with matching names."),
	   &maintenanceprintlist);

  add_cmd ("symtabs", class_maintenance, maintenance_info_symtabs, _("\
List the full symbol tables for all object files.\n\
This does not include information about individual symbols, blocks, or\n\
linetables --- just the symbol table structures themselves.\n\
With an argument REGEXP, list the symbol tables with matching names."),
	   &maintenanceinfolist);

  add_cmd ("line-table", class_maintenance, maintenance_info_line_tables, _("\
List the contents of all line tables, from all symbol tables.\n\
With an argument REGEXP, list just the line tables for the symbol\n\
tables with matching names."),
	   &maintenanceinfolist);

  add_cmd ("check-symtabs", class_maintenance, maintenance_check_symtabs,
	   _("\
Check consistency of currently expanded symtabs."),
	   &maintenancelist);

  add_cmd ("expand-symtabs", class_maintenance, maintenance_expand_symtabs,
	   _("Expand symbol tables.\n\
With an argument REGEXP, only expand the symbol tables with matching names."),
	   &maintenancelist);
}
