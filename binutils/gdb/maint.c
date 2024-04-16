/* Support for GDB maintenance commands.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

   Written by Fred Fish at Cygnus Support.

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
#include "arch-utils.h"
#include <ctype.h>
#include <cmath>
#include <signal.h>
#include "command.h"
#include "gdbcmd.h"
#include "symtab.h"
#include "block.h"
#include "gdbtypes.h"
#include "demangle.h"
#include "gdbcore.h"
#include "expression.h"
#include "language.h"
#include "symfile.h"
#include "objfiles.h"
#include "value.h"
#include "top.h"
#include "maint.h"
#include "gdbsupport/selftest.h"
#include "inferior.h"
#include "gdbsupport/thread-pool.h"

#include "cli/cli-decode.h"
#include "cli/cli-utils.h"
#include "cli/cli-setshow.h"
#include "cli/cli-cmds.h"

static void maintenance_do_deprecate (const char *, int);

#ifndef _WIN32
static void
maintenance_dump_me (const char *args, int from_tty)
{
  if (query (_("Should GDB dump core? ")))
    {
#ifdef __DJGPP__
      /* SIGQUIT by default is ignored, so use SIGABRT instead.  */
      signal (SIGABRT, SIG_DFL);
      kill (getpid (), SIGABRT);
#else
      signal (SIGQUIT, SIG_DFL);
      kill (getpid (), SIGQUIT);
#endif
    }
}
#endif

/* Stimulate the internal error mechanism that GDB uses when an
   internal problem is detected.  Allows testing of the mechanism.
   Also useful when the user wants to drop a core file but not exit
   GDB.  */

static void
maintenance_internal_error (const char *args, int from_tty)
{
  internal_error ("%s", (args == NULL ? "" : args));
}

/* Stimulate the internal error mechanism that GDB uses when an
   internal problem is detected.  Allows testing of the mechanism.
   Also useful when the user wants to drop a core file but not exit
   GDB.  */

static void
maintenance_internal_warning (const char *args, int from_tty)
{
  internal_warning ("%s", (args == NULL ? "" : args));
}

/* Stimulate the internal error mechanism that GDB uses when an
   demangler problem is detected.  Allows testing of the mechanism.  */

static void
maintenance_demangler_warning (const char *args, int from_tty)
{
  demangler_warning (__FILE__, __LINE__, "%s", (args == NULL ? "" : args));
}

/* Old command to demangle a string.  The command has been moved to "demangle".
   It is kept for now because otherwise "mt demangle" gets interpreted as
   "mt demangler-warning" which artificially creates an internal gdb error.  */

static void
maintenance_demangle (const char *args, int from_tty)
{
  gdb_printf (_("This command has been moved to \"demangle\".\n"));
}

static void
maintenance_time_display (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    gdb_printf (_("\"maintenance time\" takes a numeric argument.\n"));
  else
    set_per_command_time (strtol (args, NULL, 10));
}

static void
maintenance_space_display (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    gdb_printf ("\"maintenance space\" takes a numeric argument.\n");
  else
    set_per_command_space (strtol (args, NULL, 10));
}

/* Mini tokenizing lexer for 'maint info sections' command.  */

static bool
match_substring (const char *string, const char *substr)
{
  int substr_len = strlen (substr);
  const char *tok;

  while ((tok = strstr (string, substr)) != NULL)
    {
      /* Got a partial match.  Is it a whole word?  */
      if (tok == string
	  || tok[-1] == ' '
	  || tok[-1] == '\t')
      {
	/* Token is delimited at the front...  */
	if (tok[substr_len] == ' '
	    || tok[substr_len] == '\t'
	    || tok[substr_len] == '\0')
	{
	  /* Token is delimited at the rear.  Got a whole-word match.  */
	  return true;
	}
      }
      /* Token didn't match as a whole word.  Advance and try again.  */
      string = tok + 1;
    }
  return false;
}

/* Structure holding information about a single bfd section flag.  This is
   used by the "maintenance info sections" command to print the sections,
   and for filtering which sections are printed.  */

struct single_bfd_flag_info
{
  /* The name of the section.  This is what is printed for the flag, and
     what the user enter in order to filter by flag.  */
  const char *name;

  /* The bfd defined SEC_* flagword value for this flag.  */
  flagword value;
};

/* Vector of all the known bfd flags.  */

static const single_bfd_flag_info bfd_flag_info[] =
  {
    { "ALLOC", SEC_ALLOC },
    { "LOAD", SEC_LOAD },
    { "RELOC", SEC_RELOC },
    { "READONLY", SEC_READONLY },
    { "CODE", SEC_CODE },
    { "DATA", SEC_DATA },
    { "ROM", SEC_ROM },
    { "CONSTRUCTOR", SEC_CONSTRUCTOR },
    { "HAS_CONTENTS", SEC_HAS_CONTENTS },
    { "NEVER_LOAD", SEC_NEVER_LOAD },
    { "COFF_SHARED_LIBRARY", SEC_COFF_SHARED_LIBRARY },
    { "IS_COMMON", SEC_IS_COMMON }
  };

/* For each flag in the global BFD_FLAG_INFO list, if FLAGS has a flag's
   flagword value set, and STRING contains the flag's name then return
   true, otherwise return false.  STRING is never nullptr.  */

static bool
match_bfd_flags (const char *string, flagword flags)
{
  gdb_assert (string != nullptr);

  for (const auto &f : bfd_flag_info)
    {
      if (flags & f.value
	  && match_substring (string, f.name))
	return true;
    }

  return false;
}

/* Print the names of all flags set in FLAGS.  The names are taken from the
   BFD_FLAG_INFO global.  */

static void
print_bfd_flags (flagword flags)
{
  for (const auto &f : bfd_flag_info)
    {
      if (flags & f.value)
	gdb_printf (" %s", f.name);
    }
}

static void
maint_print_section_info (const char *name, flagword flags,
			  CORE_ADDR addr, CORE_ADDR endaddr,
			  unsigned long filepos, int addr_size)
{
  gdb_printf ("    %s", hex_string_custom (addr, addr_size));
  gdb_printf ("->%s", hex_string_custom (endaddr, addr_size));
  gdb_printf (" at %s",
	      hex_string_custom ((unsigned long) filepos, 8));
  gdb_printf (": %s", name);
  print_bfd_flags (flags);
  gdb_printf ("\n");
}

/* Return the number of digits required to display COUNT in decimal.

   Used when pretty printing index numbers to ensure all of the indexes line
   up.*/

static int
index_digits (int count)
{
  return ((int) log10 ((float) count)) + 1;
}

/* Helper function to pretty-print the section index of ASECT from ABFD.
   The INDEX_DIGITS is the number of digits in the largest index that will
   be printed, and is used to pretty-print the resulting string.  */

static void
print_section_index (bfd *abfd,
		     asection *asect,
		     int index_digits)
{
  std::string result
    = string_printf (" [%d] ", gdb_bfd_section_index (abfd, asect));
  /* The '+ 4' for the leading and trailing characters.  */
  gdb_printf ("%-*s", (index_digits + 4), result.c_str ());
}

/* Print information about ASECT from ABFD.  The section will be printed using
   the VMA's from the bfd, which will not be the relocated addresses for bfds
   that should be relocated.  The information must be printed with the same
   layout as PRINT_OBJFILE_SECTION_INFO below.

   ARG is the argument string passed by the user to the top level maintenance
   info sections command.  Used for filtering which sections are printed.  */

static void
print_bfd_section_info (bfd *abfd, asection *asect, const char *arg,
			int index_digits)
{
  flagword flags = bfd_section_flags (asect);
  const char *name = bfd_section_name (asect);

  if (arg == NULL || *arg == '\0'
      || match_substring (arg, name)
      || match_bfd_flags (arg, flags))
    {
      struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
      int addr_size = gdbarch_addr_bit (gdbarch) / 8;
      CORE_ADDR addr, endaddr;

      addr = bfd_section_vma (asect);
      endaddr = addr + bfd_section_size (asect);
      print_section_index (abfd, asect, index_digits);
      maint_print_section_info (name, flags, addr, endaddr,
				asect->filepos, addr_size);
    }
}

/* Print information about ASECT which is GDB's wrapper around a section
   from ABFD.  The information must be printed with the same layout as
   PRINT_BFD_SECTION_INFO above.  PRINT_DATA holds information used to
   filter which sections are printed, and for formatting the output.

   ARG is the argument string passed by the user to the top level maintenance
   info sections command.  Used for filtering which sections are printed.  */

static void
print_objfile_section_info (bfd *abfd, struct obj_section *asect,
			    const char *arg, int index_digits)
{
  flagword flags = bfd_section_flags (asect->the_bfd_section);
  const char *name = bfd_section_name (asect->the_bfd_section);

  if (arg == NULL || *arg == '\0'
      || match_substring (arg, name)
      || match_bfd_flags (arg, flags))
    {
      struct gdbarch *gdbarch = gdbarch_from_bfd (abfd);
      int addr_size = gdbarch_addr_bit (gdbarch) / 8;

      print_section_index (abfd, asect->the_bfd_section, index_digits);
      maint_print_section_info (name, flags,
				asect->addr (), asect->endaddr (),
				asect->the_bfd_section->filepos,
				addr_size);
    }
}

/* Find an obj_section, GDB's wrapper around a bfd section for ASECTION
   from ABFD.  It might be that no such wrapper exists (for example debug
   sections don't have such wrappers) in which case nullptr is returned.  */

obj_section *
maint_obj_section_from_bfd_section (bfd *abfd,
				    asection *asection,
				    objfile *ofile)
{
  if (ofile->sections_start == nullptr)
    return nullptr;

  obj_section *osect
    = &ofile->sections_start[gdb_bfd_section_index (abfd, asection)];

  if (osect >= ofile->sections_end)
    return nullptr;

  return osect;
}

/* Print information about all sections from ABFD, which is the bfd
   corresponding to OBJFILE.  It is fine for OBJFILE to be nullptr, but
   ABFD must never be nullptr.  If OBJFILE is provided then the sections of
   ABFD will (potentially) be displayed relocated (i.e. the object file was
   loaded with add-symbol-file and custom offsets were provided).

   HEADER is a string that describes this file, e.g. 'Exec file: ', or
   'Core file: '.

   ARG is a string used for filtering which sections are printed, this can
   be nullptr for no filtering.  See the top level 'maint info sections'
   for a fuller description of the possible filtering strings.  */

static void
maint_print_all_sections (const char *header, bfd *abfd, objfile *objfile,
			  const char *arg)
{
  gdb_puts (header);
  gdb_stdout->wrap_here (8);
  gdb_printf ("`%s', ", bfd_get_filename (abfd));
  gdb_stdout->wrap_here (8);
  gdb_printf (_("file type %s.\n"), bfd_get_target (abfd));

  int section_count = gdb_bfd_count_sections (abfd);
  int digits = index_digits (section_count);

  for (asection *sect : gdb_bfd_sections (abfd))
    {
      obj_section *osect = nullptr;

      if (objfile != nullptr)
	{
	  gdb_assert (objfile->sections_start != nullptr);
	  osect
	    = maint_obj_section_from_bfd_section (abfd, sect, objfile);
	  if (osect->the_bfd_section == nullptr)
	    osect = nullptr;
	}

      if (osect == nullptr)
	print_bfd_section_info (abfd, sect, arg, digits);
      else
	print_objfile_section_info (abfd, osect, arg, digits);
    }
}

/* The options for the "maintenance info sections" command.  */

struct maint_info_sections_opts
{
  /* For "-all-objects".  */
  bool all_objects = false;
};

static const gdb::option::option_def maint_info_sections_option_defs[] = {

  gdb::option::flag_option_def<maint_info_sections_opts> {
    "all-objects",
    [] (maint_info_sections_opts *opts) { return &opts->all_objects; },
    N_("Display information from all loaded object files."),
  },
};

/* Create an option_def_group for the "maintenance info sections" options,
   with CC_OPTS as context.  */

static inline gdb::option::option_def_group
make_maint_info_sections_options_def_group (maint_info_sections_opts *cc_opts)
{
  return {{maint_info_sections_option_defs}, cc_opts};
}

/* Completion for the "maintenance info sections" command.  */

static void
maint_info_sections_completer (struct cmd_list_element *cmd,
			       completion_tracker &tracker,
			       const char *text, const char * /* word */)
{
  /* Complete command options.  */
  const auto group = make_maint_info_sections_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group))
    return;
  const char *word = advance_to_expression_complete_word_point (tracker, text);

  /* Offer completion for section flags, but not section names.  This is
     only a maintenance command after all, no point going over the top.  */
  std::vector<const char *> flags;
  for (const auto &f : bfd_flag_info)
    flags.push_back (f.name);
  flags.push_back (nullptr);
  complete_on_enum (tracker, flags.data (), text, word);
}

/* Implement the "maintenance info sections" command.  */

static void
maintenance_info_sections (const char *arg, int from_tty)
{
  /* Check if the "-all-objects" flag was passed.  */
  maint_info_sections_opts opts;
  const auto group = make_maint_info_sections_options_def_group (&opts);
  gdb::option::process_options
    (&arg, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group);

  for (objfile *ofile : current_program_space->objfiles ())
    {
      if (ofile->obfd == current_program_space->exec_bfd ())
	maint_print_all_sections (_("Exec file: "), ofile->obfd.get (),
				  ofile, arg);
      else if (opts.all_objects)
	maint_print_all_sections (_("Object file: "), ofile->obfd.get (),
				  ofile, arg);
    }

  if (core_bfd)
    maint_print_all_sections (_("Core file: "), core_bfd, nullptr, arg);
}

/* Implement the "maintenance info target-sections" command.  */

static void
maintenance_info_target_sections (const char *arg, int from_tty)
{
  bfd *abfd = nullptr;
  int digits = 0;
  const std::vector<target_section> *table
    = target_get_section_table (current_inferior ()->top_target ());
  if (table == nullptr)
    return;

  for (const target_section &sec : *table)
    {
      if (abfd == nullptr || sec.the_bfd_section->owner != abfd)
	{
	  abfd = sec.the_bfd_section->owner;
	  digits = std::max (index_digits (gdb_bfd_count_sections (abfd)),
			     digits);
	}
    }

  struct gdbarch *gdbarch = nullptr;
  int addr_size = 0;
  abfd = nullptr;
  for (const target_section &sec : *table)
   {
      if (sec.the_bfd_section->owner != abfd)
	{
	  abfd = sec.the_bfd_section->owner;
	  gdbarch = gdbarch_from_bfd (abfd);
	  addr_size = gdbarch_addr_bit (gdbarch) / 8;

	  gdb_printf (_("From '%s', file type %s:\n"),
		      bfd_get_filename (abfd), bfd_get_target (abfd));
	}
      print_bfd_section_info (abfd,
			      sec.the_bfd_section,
			      nullptr,
			      digits);
      /* The magic '8 + digits' here ensures that the 'Start' is aligned
	 with the output of print_bfd_section_info.  */
      gdb_printf ("%*sStart: %s, End: %s, Owner token: %p\n",
		  (8 + digits), "",
		  hex_string_custom (sec.addr, addr_size),
		  hex_string_custom (sec.endaddr, addr_size),
		  sec.owner.v ());
    }
}

static void
maintenance_print_statistics (const char *args, int from_tty)
{
  print_objfile_statistics ();
}

static void
maintenance_print_architecture (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (args == NULL)
    gdbarch_dump (gdbarch, gdb_stdout);
  else
    {
      stdio_file file;

      if (!file.open (args, "w"))
	perror_with_name (_("maintenance print architecture"));
      gdbarch_dump (gdbarch, &file);
    }
}

/* The "maintenance translate-address" command converts a section and address
   to a symbol.  This can be called in two ways:
   maintenance translate-address <secname> <addr>
   or   maintenance translate-address <addr>.  */

static void
maintenance_translate_address (const char *arg, int from_tty)
{
  CORE_ADDR address;
  struct obj_section *sect;
  const char *p;
  struct bound_minimal_symbol sym;

  if (arg == NULL || *arg == 0)
    error (_("requires argument (address or section + address)"));

  sect = NULL;
  p = arg;

  if (!isdigit (*p))
    {				/* See if we have a valid section name.  */
      while (*p && !isspace (*p))	/* Find end of section name.  */
	p++;
      if (*p == '\000')		/* End of command?  */
	error (_("Need to specify section name and address"));

      int arg_len = p - arg;
      p = skip_spaces (p + 1);

      for (objfile *objfile : current_program_space->objfiles ())
	for (obj_section *iter : objfile->sections ())
	  {
	    if (strncmp (iter->the_bfd_section->name, arg, arg_len) == 0)
	      goto found;
	  }

      error (_("Unknown section %s."), arg);
    found: ;
    }

  address = parse_and_eval_address (p);

  if (sect)
    sym = lookup_minimal_symbol_by_pc_section (address, sect);
  else
    sym = lookup_minimal_symbol_by_pc (address);

  if (sym.minsym)
    {
      const char *symbol_name = sym.minsym->print_name ();
      const char *symbol_offset
	= pulongest (address - sym.value_address ());

      sect = sym.obj_section ();
      if (sect != NULL)
	{
	  const char *section_name;
	  const char *obj_name;

	  gdb_assert (sect->the_bfd_section && sect->the_bfd_section->name);
	  section_name = sect->the_bfd_section->name;

	  gdb_assert (sect->objfile && objfile_name (sect->objfile));
	  obj_name = objfile_name (sect->objfile);

	  if (current_program_space->multi_objfile_p ())
	    gdb_printf (_("%s + %s in section %s of %s\n"),
			symbol_name, symbol_offset,
			section_name, obj_name);
	  else
	    gdb_printf (_("%s + %s in section %s\n"),
			symbol_name, symbol_offset, section_name);
	}
      else
	gdb_printf (_("%s + %s\n"), symbol_name, symbol_offset);
    }
  else if (sect)
    gdb_printf (_("no symbol at %s:%s\n"),
		sect->the_bfd_section->name, hex_string (address));
  else
    gdb_printf (_("no symbol at %s\n"), hex_string (address));

  return;
}


/* When a command is deprecated the user will be warned the first time
   the command is used.  If possible, a replacement will be
   offered.  */

static void
maintenance_deprecate (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    {
      gdb_printf (_("\"maintenance deprecate\" takes an argument,\n\
the command you want to deprecate, and optionally the replacement command\n\
enclosed in quotes.\n"));
    }

  maintenance_do_deprecate (args, 1);
}


static void
maintenance_undeprecate (const char *args, int from_tty)
{
  if (args == NULL || *args == '\0')
    {
      gdb_printf (_("\"maintenance undeprecate\" takes an argument, \n\
the command you want to undeprecate.\n"));
    }

  maintenance_do_deprecate (args, 0);
}

/* You really shouldn't be using this.  It is just for the testsuite.
   Rather, you should use deprecate_cmd() when the command is created
   in _initialize_blah().

   This function deprecates a command and optionally assigns it a
   replacement.  */

static void
maintenance_do_deprecate (const char *text, int deprecate)
{
  struct cmd_list_element *alias = NULL;
  struct cmd_list_element *prefix_cmd = NULL;
  struct cmd_list_element *cmd = NULL;

  const char *start_ptr = NULL;
  const char *end_ptr = NULL;
  int len;
  char *replacement = NULL;

  if (text == NULL)
    return;

  if (!lookup_cmd_composition (text, &alias, &prefix_cmd, &cmd))
    {
      gdb_printf (_("Can't find command '%s' to deprecate.\n"), text);
      return;
    }

  if (deprecate)
    {
      /* Look for a replacement command.  */
      start_ptr = strchr (text, '\"');
      if (start_ptr != NULL)
	{
	  start_ptr++;
	  end_ptr = strrchr (start_ptr, '\"');
	  if (end_ptr != NULL)
	    {
	      len = end_ptr - start_ptr;
	      replacement = savestring (start_ptr, len);
	    }
	}
    }

  if (!start_ptr || !end_ptr)
    replacement = NULL;


  /* If they used an alias, we only want to deprecate the alias.

     Note the MALLOCED_REPLACEMENT test.  If the command's replacement
     string was allocated at compile time we don't want to free the
     memory.  */
  if (alias)
    {
      if (alias->malloced_replacement)
	xfree ((char *) alias->replacement);

      if (deprecate)
	{
	  alias->deprecated_warn_user = 1;
	  alias->cmd_deprecated = 1;
	}
      else
	{
	  alias->deprecated_warn_user = 0;
	  alias->cmd_deprecated = 0;
	}
      alias->replacement = replacement;
      alias->malloced_replacement = 1;
      return;
    }
  else if (cmd)
    {
      if (cmd->malloced_replacement)
	xfree ((char *) cmd->replacement);

      if (deprecate)
	{
	  cmd->deprecated_warn_user = 1;
	  cmd->cmd_deprecated = 1;
	}
      else
	{
	  cmd->deprecated_warn_user = 0;
	  cmd->cmd_deprecated = 0;
	}
      cmd->replacement = replacement;
      cmd->malloced_replacement = 1;
      return;
    }
  xfree (replacement);
}

/* Maintenance set/show framework.  */

struct cmd_list_element *maintenance_set_cmdlist;
struct cmd_list_element *maintenance_show_cmdlist;

/* "maintenance with" command.  */

static void
maintenance_with_cmd (const char *args, int from_tty)
{
  with_command_1 ("maintenance set ", maintenance_set_cmdlist, args, from_tty);
}

/* "maintenance with" command completer.  */

static void
maintenance_with_cmd_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char * /*word*/)
{
  with_command_completer_1 ("maintenance set ", tracker,  text);
}

/* Profiling support.  */

static bool maintenance_profile_p;
static void
show_maintenance_profile_p (struct ui_file *file, int from_tty,
			    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Internal profiling is %s.\n"), value);
}

#ifdef HAVE__ETEXT
extern char _etext;
#define TEXTEND &_etext
#elif defined (HAVE_ETEXT)
extern char etext;
#define TEXTEND &etext
#endif

#if defined (HAVE_MONSTARTUP) && defined (HAVE__MCLEANUP) && defined (TEXTEND)

static int profiling_state;

extern "C" void _mcleanup (void);

static void
mcleanup_wrapper (void)
{
  if (profiling_state)
    _mcleanup ();
}

extern "C" void monstartup (unsigned long, unsigned long);
extern int main (int, char **);

static void
maintenance_set_profile_cmd (const char *args, int from_tty,
			     struct cmd_list_element *c)
{
  if (maintenance_profile_p == profiling_state)
    return;

  profiling_state = maintenance_profile_p;

  if (maintenance_profile_p)
    {
      static int profiling_initialized;

      if (!profiling_initialized)
	{
	  atexit (mcleanup_wrapper);
	  profiling_initialized = 1;
	}

      /* "main" is now always the first function in the text segment, so use
	 its address for monstartup.  */
      monstartup ((unsigned long) &main, (unsigned long) TEXTEND);
    }
  else
    {
      extern void _mcleanup (void);

      _mcleanup ();
    }
}
#else
static void
maintenance_set_profile_cmd (const char *args, int from_tty,
			     struct cmd_list_element *c)
{
  error (_("Profiling support is not available on this system."));
}
#endif

static int n_worker_threads = -1;

/* See maint.h.  */

void
update_thread_pool_size ()
{
#if CXX_STD_THREAD
  int n_threads = n_worker_threads;

  if (n_threads < 0)
    {
      const int hardware_threads = std::thread::hardware_concurrency ();
      /* Testing in PR gdb/29959 indicates that parallel efficiency drops
	 between n_threads=5 to 8.  Therefore, use no more than 8 threads
	 to avoid an excessive number of threads in the pool on many-core
	 systems.  */
      const int max_thread_count = 8;
      n_threads = std::min (hardware_threads, max_thread_count);
    }

  gdb::thread_pool::g_thread_pool->set_thread_count (n_threads);
#endif
}

static void
maintenance_set_worker_threads (const char *args, int from_tty,
				struct cmd_list_element *c)
{
  update_thread_pool_size ();
}

static void
maintenance_show_worker_threads (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
#if CXX_STD_THREAD
  if (n_worker_threads == -1)
    {
      gdb_printf (file, _("The number of worker threads GDB "
			  "can use is the default (currently %zu).\n"),
		  gdb::thread_pool::g_thread_pool->thread_count ());
      return;
    }
#endif

  int report_threads = 0;
#if CXX_STD_THREAD
  report_threads = n_worker_threads;
#endif
  gdb_printf (file, _("The number of worker threads GDB "
		      "can use is %d.\n"),
	      report_threads);
}


/* If true, display time usage both at startup and for each command.  */

static bool per_command_time;

/* If true, display space usage both at startup and for each command.  */

static bool per_command_space;

/* If true, display basic symtab stats for each command.  */

static bool per_command_symtab;

/* mt per-command commands.  */

static struct cmd_list_element *per_command_setlist;
static struct cmd_list_element *per_command_showlist;

/* Set whether to display time statistics to NEW_VALUE
   (non-zero means true).  */

void
set_per_command_time (int new_value)
{
  per_command_time = new_value;
}

/* Set whether to display space statistics to NEW_VALUE
   (non-zero means true).  */

void
set_per_command_space (int new_value)
{
  per_command_space = new_value;
}

/* Count the number of symtabs and blocks.  */

static void
count_symtabs_and_blocks (int *nr_symtabs_ptr, int *nr_compunit_symtabs_ptr,
			  int *nr_blocks_ptr)
{
  int nr_symtabs = 0;
  int nr_compunit_symtabs = 0;
  int nr_blocks = 0;

  /* When collecting statistics during startup, this is called before
     pretty much anything in gdb has been initialized, and thus
     current_program_space may be NULL.  */
  if (current_program_space != NULL)
    {
      for (objfile *o : current_program_space->objfiles ())
	{
	  for (compunit_symtab *cu : o->compunits ())
	    {
	      ++nr_compunit_symtabs;
	      nr_blocks += cu->blockvector ()->num_blocks ();
	      nr_symtabs += std::distance (cu->filetabs ().begin (),
					   cu->filetabs ().end ());
	    }
	}
    }

  *nr_symtabs_ptr = nr_symtabs;
  *nr_compunit_symtabs_ptr = nr_compunit_symtabs;
  *nr_blocks_ptr = nr_blocks;
}

/* As indicated by display_time and display_space, report GDB's
   elapsed time and space usage from the base time and space recorded
   in this object.  */

scoped_command_stats::~scoped_command_stats ()
{
  /* Early exit if we're not reporting any stats.  It can be expensive to
     compute the pre-command values so don't collect them at all if we're
     not reporting stats.  Alas this doesn't work in the startup case because
     we don't know yet whether we will be reporting the stats.  For the
     startup case collect the data anyway (it should be cheap at this point),
     and leave it to the reporter to decide whether to print them.  */
  if (m_msg_type
      && !per_command_time
      && !per_command_space
      && !per_command_symtab)
    return;

  if (m_time_enabled && per_command_time)
    {
      print_time (_("command finished"));

      using namespace std::chrono;

      run_time_clock::duration cmd_time
	= run_time_clock::now () - m_start_cpu_time;

      steady_clock::duration wall_time
	= steady_clock::now () - m_start_wall_time;
      /* Subtract time spend in prompt_for_continue from walltime.  */
      wall_time -= get_prompt_for_continue_wait_time ();

      gdb_printf (gdb_stdlog,
		  !m_msg_type
		  ? _("Startup time: %.6f (cpu), %.6f (wall)\n")
		  : _("Command execution time: %.6f (cpu), %.6f (wall)\n"),
		  duration<double> (cmd_time).count (),
		  duration<double> (wall_time).count ());
    }

  if (m_space_enabled && per_command_space)
    {
#ifdef HAVE_USEFUL_SBRK
      char *lim = (char *) sbrk (0);

      long space_now = lim - lim_at_start;
      long space_diff = space_now - m_start_space;

      gdb_printf (gdb_stdlog,
		  !m_msg_type
		  ? _("Space used: %ld (%s%ld during startup)\n")
		  : _("Space used: %ld (%s%ld for this command)\n"),
		  space_now,
		  (space_diff >= 0 ? "+" : ""),
		  space_diff);
#endif
    }

  if (m_symtab_enabled && per_command_symtab)
    {
      int nr_symtabs, nr_compunit_symtabs, nr_blocks;

      count_symtabs_and_blocks (&nr_symtabs, &nr_compunit_symtabs, &nr_blocks);
      gdb_printf (gdb_stdlog,
		  _("#symtabs: %d (+%d),"
		    " #compunits: %d (+%d),"
		    " #blocks: %d (+%d)\n"),
		  nr_symtabs,
		  nr_symtabs - m_start_nr_symtabs,
		  nr_compunit_symtabs,
		  (nr_compunit_symtabs
		   - m_start_nr_compunit_symtabs),
		  nr_blocks,
		  nr_blocks - m_start_nr_blocks);
    }
}

scoped_command_stats::scoped_command_stats (bool msg_type)
: m_msg_type (msg_type)
{
  if (!m_msg_type || per_command_space)
    {
#ifdef HAVE_USEFUL_SBRK
      char *lim = (char *) sbrk (0);
      m_start_space = lim - lim_at_start;
      m_space_enabled = true;
#endif
    }
  else
    m_space_enabled = false;

  if (msg_type == 0 || per_command_time)
    {
      using namespace std::chrono;

      m_start_cpu_time = run_time_clock::now ();
      m_start_wall_time = steady_clock::now ();
      m_time_enabled = true;

      if (per_command_time)
	print_time (_("command started"));
    }
  else
    m_time_enabled = false;

  if (msg_type == 0 || per_command_symtab)
    {
      int nr_symtabs, nr_compunit_symtabs, nr_blocks;

      count_symtabs_and_blocks (&nr_symtabs, &nr_compunit_symtabs, &nr_blocks);
      m_start_nr_symtabs = nr_symtabs;
      m_start_nr_compunit_symtabs = nr_compunit_symtabs;
      m_start_nr_blocks = nr_blocks;
      m_symtab_enabled = true;
    }
  else
    m_symtab_enabled = false;

  /* Initialize timer to keep track of how long we waited for the user.  */
  reset_prompt_for_continue_wait_time ();
}

/* See maint.h.  */

void
scoped_command_stats::print_time (const char *msg)
{
  using namespace std::chrono;

  auto now = system_clock::now ();
  auto ticks = now.time_since_epoch ().count () / (1000 * 1000);
  auto millis = ticks % 1000;

  std::time_t as_time = system_clock::to_time_t (now);
  struct tm tm;
  localtime_r (&as_time, &tm);

  char out[100];
  strftime (out, sizeof (out), "%F %H:%M:%S", &tm);

  gdb_printf (gdb_stdlog, "%s.%03d - %s\n", out, (int) millis, msg);
}

/* Handle unknown "mt set per-command" arguments.
   In this case have "mt set per-command on|off" affect every setting.  */

static void
set_per_command_cmd (const char *args, int from_tty)
{
  struct cmd_list_element *list;
  int val;

  val = parse_cli_boolean_value (args);
  if (val < 0)
    error (_("Bad value for 'mt set per-command no'."));

  for (list = per_command_setlist; list != NULL; list = list->next)
    if (list->var->type () == var_boolean)
      {
	gdb_assert (list->type == set_cmd);
	do_set_command (args, from_tty, list);
      }
}

/* Options affecting the "maintenance selftest" command.  */

struct maintenance_selftest_options
{
  bool verbose = false;
} user_maintenance_selftest_options;

static const gdb::option::option_def maintenance_selftest_option_defs[] = {
  gdb::option::boolean_option_def<maintenance_selftest_options> {
    "verbose",
    [] (maintenance_selftest_options *opt) { return &opt->verbose; },
    nullptr,
    N_("Set whether selftests run in verbose mode."),
    N_("Show whether selftests run in verbose mode."),
    N_("\
When on, selftests may print verbose information."),
  },
};

/* Make option groups for the "maintenance selftest" command.  */

static std::array<gdb::option::option_def_group, 1>
make_maintenance_selftest_option_group (maintenance_selftest_options *opts)
{
  return {{
    {{maintenance_selftest_option_defs}, opts},
  }};
}

/* The "maintenance selftest" command.  */

static void
maintenance_selftest (const char *args, int from_tty)
{
#if GDB_SELF_TEST
  maintenance_selftest_options opts = user_maintenance_selftest_options;
  auto grp = make_maintenance_selftest_option_group (&opts);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp);
  const gdb_argv argv (args);
  selftests::run_tests (argv.as_array_view (), opts.verbose);
#else
  gdb_printf (_("\
Selftests have been disabled for this build.\n"));
#endif
}

/* Completer for the "maintenance selftest" command.  */

static void
maintenance_selftest_completer (cmd_list_element *cmd,
				completion_tracker &tracker,
				const char *text,
				const char *word)
{
  auto grp = make_maintenance_selftest_option_group (nullptr);

  if (gdb::option::complete_options
	(tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, grp))
    return;

#if GDB_SELF_TEST
  for (const auto &test : selftests::all_selftests ())
    {
      if (startswith (test.name.c_str (), text))
	tracker.add_completion (make_unique_xstrdup (test.name.c_str ()));
    }
#endif
}

static void
maintenance_info_selftests (const char *arg, int from_tty)
{
#if GDB_SELF_TEST
  gdb_printf ("Registered selftests:\n");
  for (const auto &test : selftests::all_selftests ())
    gdb_printf (" - %s\n", test.name.c_str ());
#else
  gdb_printf (_("\
Selftests have been disabled for this build.\n"));
#endif
}


void _initialize_maint_cmds ();
void
_initialize_maint_cmds ()
{
  struct cmd_list_element *cmd;

  cmd_list_element *maintenance_cmd
    = add_basic_prefix_cmd ("maintenance", class_maintenance, _("\
Commands for use by GDB maintainers.\n\
Includes commands to dump specific internal GDB structures in\n\
a human readable form, to cause GDB to deliberately dump core, etc."),
			    &maintenancelist, 0,
			    &cmdlist);

  add_com_alias ("mt", maintenance_cmd, class_maintenance, 1);

  cmd_list_element *maintenance_info_cmd
    = add_basic_prefix_cmd ("info", class_maintenance, _("\
Commands for showing internal info about the program being debugged."),
			    &maintenanceinfolist, 0,
			    &maintenancelist);
  add_alias_cmd ("i", maintenance_info_cmd, class_maintenance, 1,
		 &maintenancelist);

  const auto opts = make_maint_info_sections_options_def_group (nullptr);
  static std::string maint_info_sections_command_help
    = gdb::option::build_help (_("\
List the BFD sections of the exec and core files.\n\
\n\
Usage: maintenance info sections [-all-objects] [FILTERS]\n\
\n\
FILTERS is a list of words, each word is either:\n\
  + A section name - any section with this name will be printed, or\n\
  + A section flag - any section with this flag will be printed.  The\n\
	known flags are:\n\
	  ALLOC LOAD RELOC READONLY CODE DATA ROM CONSTRUCTOR\n\
	  HAS_CONTENTS NEVER_LOAD COFF_SHARED_LIBRARY IS_COMMON\n\
\n\
Sections matching any of the FILTERS will be listed (no FILTERS implies\n\
all sections should be printed).\n\
\n\
Options:\n\
%OPTIONS%"), opts);
  cmd = add_cmd ("sections", class_maintenance, maintenance_info_sections,
		 maint_info_sections_command_help.c_str (),
		 &maintenanceinfolist);
  set_cmd_completer_handle_brkchars (cmd, maint_info_sections_completer);

  add_cmd ("target-sections", class_maintenance,
	   maintenance_info_target_sections, _("\
List GDB's internal section table.\n\
\n\
Print the current targets section list.  This is a sub-set of all\n\
sections, from all objects currently loaded.  Usually the ALLOC\n\
sections."),
	   &maintenanceinfolist);

  add_basic_prefix_cmd ("print", class_maintenance,
			_("Maintenance command for printing GDB internal state."),
			&maintenanceprintlist, 0,
			&maintenancelist);

  add_basic_prefix_cmd ("flush", class_maintenance,
			_("Maintenance command for flushing GDB internal caches."),
			&maintenanceflushlist, 0,
			&maintenancelist);

  add_basic_prefix_cmd ("set", class_maintenance, _("\
Set GDB internal variables used by the GDB maintainer.\n\
Configure variables internal to GDB that aid in GDB's maintenance"),
			&maintenance_set_cmdlist,
			0/*allow-unknown*/,
			&maintenancelist);

  add_show_prefix_cmd ("show", class_maintenance, _("\
Show GDB internal variables used by the GDB maintainer.\n\
Configure variables internal to GDB that aid in GDB's maintenance"),
		       &maintenance_show_cmdlist,
		       0/*allow-unknown*/,
		       &maintenancelist);

  cmd = add_cmd ("with", class_maintenance, maintenance_with_cmd, _("\
Like \"with\", but works with \"maintenance set\" variables.\n\
Usage: maintenance with SETTING [VALUE] [-- COMMAND]\n\
With no COMMAND, repeats the last executed command.\n\
SETTING is any setting you can change with the \"maintenance set\"\n\
subcommands."),
		 &maintenancelist);
  set_cmd_completer_handle_brkchars (cmd, maintenance_with_cmd_completer);

#ifndef _WIN32
  add_cmd ("dump-me", class_maintenance, maintenance_dump_me, _("\
Get fatal error; make debugger dump its core.\n\
GDB sets its handling of SIGQUIT back to SIG_DFL and then sends\n\
itself a SIGQUIT signal."),
	   &maintenancelist);
#endif

  add_cmd ("internal-error", class_maintenance,
	   maintenance_internal_error, _("\
Give GDB an internal error.\n\
Cause GDB to behave as if an internal error was detected."),
	   &maintenancelist);

  add_cmd ("internal-warning", class_maintenance,
	   maintenance_internal_warning, _("\
Give GDB an internal warning.\n\
Cause GDB to behave as if an internal warning was reported."),
	   &maintenancelist);

  add_cmd ("demangler-warning", class_maintenance,
	   maintenance_demangler_warning, _("\
Give GDB a demangler warning.\n\
Cause GDB to behave as if a demangler warning was reported."),
	   &maintenancelist);

  cmd = add_cmd ("demangle", class_maintenance, maintenance_demangle, _("\
This command has been moved to \"demangle\"."),
		 &maintenancelist);
  deprecate_cmd (cmd, "demangle");

  add_prefix_cmd ("per-command", class_maintenance, set_per_command_cmd, _("\
Per-command statistics settings."),
		    &per_command_setlist,
		    1/*allow-unknown*/, &maintenance_set_cmdlist);

  add_show_prefix_cmd ("per-command", class_maintenance, _("\
Show per-command statistics settings."),
		       &per_command_showlist,
		       0/*allow-unknown*/, &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("time", class_maintenance,
			   &per_command_time, _("\
Set whether to display per-command execution time."), _("\
Show whether to display per-command execution time."),
			   _("\
If enabled, the execution time for each command will be\n\
displayed following the command's output."),
			   NULL, NULL,
			   &per_command_setlist, &per_command_showlist);

  add_setshow_boolean_cmd ("space", class_maintenance,
			   &per_command_space, _("\
Set whether to display per-command space usage."), _("\
Show whether to display per-command space usage."),
			   _("\
If enabled, the space usage for each command will be\n\
displayed following the command's output."),
			   NULL, NULL,
			   &per_command_setlist, &per_command_showlist);

  add_setshow_boolean_cmd ("symtab", class_maintenance,
			   &per_command_symtab, _("\
Set whether to display per-command symtab statistics."), _("\
Show whether to display per-command symtab statistics."),
			   _("\
If enabled, the basic symtab statistics for each command will be\n\
displayed following the command's output."),
			   NULL, NULL,
			   &per_command_setlist, &per_command_showlist);

  /* This is equivalent to "mt set per-command time on".
     Kept because some people are used to typing "mt time 1".  */
  add_cmd ("time", class_maintenance, maintenance_time_display, _("\
Set the display of time usage.\n\
If nonzero, will cause the execution time for each command to be\n\
displayed, following the command's output."),
	   &maintenancelist);

  /* This is equivalent to "mt set per-command space on".
     Kept because some people are used to typing "mt space 1".  */
  add_cmd ("space", class_maintenance, maintenance_space_display, _("\
Set the display of space usage.\n\
If nonzero, will cause the execution space for each command to be\n\
displayed, following the command's output."),
	   &maintenancelist);

  cmd = add_cmd ("type", class_maintenance, maintenance_print_type, _("\
Print a type chain for a given symbol.\n\
For each node in a type chain, print the raw data for each member of\n\
the type structure, and the interpretation of the data."),
	   &maintenanceprintlist);
  set_cmd_completer (cmd, expression_completer);

  add_cmd ("statistics", class_maintenance, maintenance_print_statistics,
	   _("Print statistics about internal gdb state."),
	   &maintenanceprintlist);

  add_cmd ("architecture", class_maintenance,
	   maintenance_print_architecture, _("\
Print the internal architecture configuration.\n\
Takes an optional file parameter."),
	   &maintenanceprintlist);

  add_basic_prefix_cmd ("check", class_maintenance, _("\
Commands for checking internal gdb state."),
			&maintenancechecklist, 0,
			&maintenancelist);

  add_cmd ("translate-address", class_maintenance,
	   maintenance_translate_address,
	   _("Translate a section name and address to a symbol."),
	   &maintenancelist);

  add_cmd ("deprecate", class_maintenance, maintenance_deprecate, _("\
Deprecate a command (for testing purposes).\n\
Usage: maintenance deprecate COMMANDNAME [\"REPLACEMENT\"]\n\
This is used by the testsuite to check the command deprecator.\n\
You probably shouldn't use this,\n\
rather you should use the C function deprecate_cmd()."), &maintenancelist);

  add_cmd ("undeprecate", class_maintenance, maintenance_undeprecate, _("\
Undeprecate a command (for testing purposes).\n\
Usage: maintenance undeprecate COMMANDNAME\n\
This is used by the testsuite to check the command deprecator.\n\
You probably shouldn't use this."),
	   &maintenancelist);

  cmd_list_element *maintenance_selftest_cmd
    = add_cmd ("selftest", class_maintenance, maintenance_selftest, _("\
Run gdb's unit tests.\n\
Usage: maintenance selftest [FILTER]\n\
This will run any unit tests that were built in to gdb.\n\
If a filter is given, only the tests with that value in their name will ran."),
	       &maintenancelist);
  set_cmd_completer_handle_brkchars (maintenance_selftest_cmd,
				     maintenance_selftest_completer);

  add_cmd ("selftests", class_maintenance, maintenance_info_selftests,
	 _("List the registered selftests."), &maintenanceinfolist);

  add_setshow_boolean_cmd ("profile", class_maintenance,
			   &maintenance_profile_p, _("\
Set internal profiling."), _("\
Show internal profiling."), _("\
When enabled GDB is profiled."),
			   maintenance_set_profile_cmd,
			   show_maintenance_profile_p,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_zuinteger_unlimited_cmd ("worker-threads",
				       class_maintenance,
				       &n_worker_threads, _("\
Set the number of worker threads GDB can use."), _("\
Show the number of worker threads GDB can use."), _("\
GDB may use multiple threads to speed up certain CPU-intensive operations,\n\
such as demangling symbol names."),
				       maintenance_set_worker_threads,
				       maintenance_show_worker_threads,
				       &maintenance_set_cmdlist,
				       &maintenance_show_cmdlist);

  /* Add the "maint set/show selftest" commands.  */
  static cmd_list_element *set_selftest_cmdlist = nullptr;
  static cmd_list_element *show_selftest_cmdlist = nullptr;

  add_setshow_prefix_cmd ("selftest", class_maintenance,
			  _("Self tests-related settings."),
			  _("Self tests-related settings."),
			  &set_selftest_cmdlist, &show_selftest_cmdlist,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  /* Add setting commands matching "maintenance selftest" options.  */
  gdb::option::add_setshow_cmds_for_options (class_maintenance,
					     &user_maintenance_selftest_options,
					     maintenance_selftest_option_defs,
					     &set_selftest_cmdlist,
					     &show_selftest_cmdlist);
}
