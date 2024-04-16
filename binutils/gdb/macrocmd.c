/* C preprocessor macro expansion commands for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

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
#include "macrotab.h"
#include "macroexp.h"
#include "macroscope.h"
#include "cli/cli-style.h"
#include "cli/cli-utils.h"
#include "command.h"
#include "gdbcmd.h"
#include "linespec.h"


/* The `macro' prefix command.  */

static struct cmd_list_element *macrolist;


/* Macro expansion commands.  */


/* Prints an informational message regarding the lack of macro information.  */
static void
macro_inform_no_debuginfo (void)
{
  gdb_puts ("GDB has no preprocessor macro information for that code.\n");
}

static void
macro_expand_command (const char *exp, int from_tty)
{
  /* You know, when the user doesn't specify any expression, it would be
     really cool if this defaulted to the last expression evaluated.
     Then it would be easy to ask, "Hey, what did I just evaluate?"  But
     at the moment, the `print' commands don't save the last expression
     evaluated, just its value.  */
  if (! exp || ! *exp)
    error (_("You must follow the `macro expand' command with the"
	   " expression you\n"
	   "want to expand."));

  gdb::unique_xmalloc_ptr<macro_scope> ms = default_macro_scope ();

  if (ms != nullptr)
    {
      gdb::unique_xmalloc_ptr<char> expanded = macro_expand (exp, *ms);

      gdb_puts ("expands to: ");
      gdb_puts (expanded.get ());
      gdb_puts ("\n");
    }
  else
    macro_inform_no_debuginfo ();
}


static void
macro_expand_once_command (const char *exp, int from_tty)
{
  /* You know, when the user doesn't specify any expression, it would be
     really cool if this defaulted to the last expression evaluated.
     And it should set the once-expanded text as the new `last
     expression'.  That way, you could just hit return over and over and
     see the expression expanded one level at a time.  */
  if (! exp || ! *exp)
    error (_("You must follow the `macro expand-once' command with"
	   " the expression\n"
	   "you want to expand."));

  gdb::unique_xmalloc_ptr<macro_scope> ms = default_macro_scope ();

  if (ms != nullptr)
    {
      gdb::unique_xmalloc_ptr<char> expanded = macro_expand_once (exp, *ms);

      gdb_puts ("expands to: ");
      gdb_puts (expanded.get ());
      gdb_puts ("\n");
    }
  else
    macro_inform_no_debuginfo ();
}

/*  Outputs the include path of a macro starting at FILE and LINE to STREAM.

    Care should be taken that this function does not cause any lookups into
    the splay tree so that it can be safely used while iterating.  */
static void
show_pp_source_pos (struct ui_file *stream,
		    struct macro_source_file *file,
		    int line)
{
  std::string fullname = macro_source_fullname (file);
  gdb_printf (stream, "%ps:%d\n",
	      styled_string (file_name_style.style (),
			     fullname.c_str ()),
	      line);

  while (file->included_by)
    {
      fullname = macro_source_fullname (file->included_by);
      gdb_puts (_("  included at "), stream);
      fputs_styled (fullname.c_str (), file_name_style.style (), stream);
      gdb_printf (stream, ":%d\n", file->included_at_line);
      file = file->included_by;
    }
}

/* Outputs a macro for human consumption, detailing the include path
   and macro definition.  NAME is the name of the macro.
   D the definition.  FILE the start of the include path, and LINE the
   line number in FILE.

   Care should be taken that this function does not cause any lookups into
   the splay tree so that it can be safely used while iterating.  */
static void
print_macro_definition (const char *name,
			const struct macro_definition *d,
			struct macro_source_file *file,
			int line)
{
  gdb_printf ("Defined at ");
  show_pp_source_pos (gdb_stdout, file, line);

  if (line != 0)
    gdb_printf ("#define %s", name);
  else
    gdb_printf ("-D%s", name);

  if (d->kind == macro_function_like)
    {
      int i;

      gdb_puts ("(");
      for (i = 0; i < d->argc; i++)
	{
	  gdb_puts (d->argv[i]);
	  if (i + 1 < d->argc)
	    gdb_puts (", ");
	}
      gdb_puts (")");
    }

  if (line != 0)
    gdb_printf (" %s\n", d->replacement);
  else
    gdb_printf ("=%s\n", d->replacement);
}

/* The implementation of the `info macro' command.  */
static void
info_macro_command (const char *args, int from_tty)
{
  gdb::unique_xmalloc_ptr<struct macro_scope> ms;
  const char *name;
  int show_all_macros_named = 0;
  const char *arg_start = args;
  int processing_args = 1;

  while (processing_args
	 && arg_start && *arg_start == '-' && *arg_start != '\0')
    {
      const char *p = skip_to_space (arg_start);

      if (strncmp (arg_start, "-a", p - arg_start) == 0
	  || strncmp (arg_start, "-all", p - arg_start) == 0)
	show_all_macros_named = 1;
      else if (strncmp (arg_start, "--", p - arg_start) == 0)
	  /* Our macro support seems rather C specific but this would
	     seem necessary for languages allowing - in macro names.
	     e.g. Scheme's (defmacro ->foo () "bar\n")  */
	processing_args = 0;
      else
	report_unrecognized_option_error ("info macro", arg_start);

      arg_start = skip_spaces (p);
    }

  name = arg_start;

  if (! name || ! *name)
    error (_("You must follow the `info macro' command with the name"
	     " of the macro\n"
	     "whose definition you want to see."));

  ms = default_macro_scope ();

  if (! ms)
    macro_inform_no_debuginfo ();
  else if (show_all_macros_named)
    macro_for_each (ms->file->table, [&] (const char *macro_name,
					  const macro_definition *macro,
					  macro_source_file *source,
					  int line)
      {
	if (strcmp (name, macro_name) == 0)
	  print_macro_definition (name, macro, source, line);
      });
  else
    {
      struct macro_definition *d;

      d = macro_lookup_definition (ms->file, ms->line, name);
      if (d)
	{
	  int line;
	  struct macro_source_file *file
	    = macro_definition_location (ms->file, ms->line, name, &line);

	  print_macro_definition (name, d, file, line);
	}
      else
	{
	  gdb_printf ("The symbol `%s' has no definition as a C/C++"
		      " preprocessor macro\n"
		      "at ", name);
	  show_pp_source_pos (gdb_stdout, ms->file, ms->line);
	}
    }
}

/* Implementation of the "info macros" command. */
static void
info_macros_command (const char *args, int from_tty)
{
  gdb::unique_xmalloc_ptr<struct macro_scope> ms;

  if (args == NULL)
    ms = default_macro_scope ();
  else
    {
      std::vector<symtab_and_line> sals
	= decode_line_with_current_source (args, 0);

      if (!sals.empty ())
	ms = sal_macro_scope (sals[0]);
    }

  if (! ms || ! ms->file || ! ms->file->table)
    macro_inform_no_debuginfo ();
  else
    macro_for_each_in_scope (ms->file, ms->line, print_macro_definition);
}


/* User-defined macros.  */

static void
skip_ws (const char **expp)
{
  while (macro_is_whitespace (**expp))
    ++*expp;
}

/* Try to find the bounds of an identifier.  If an identifier is
   found, returns a newly allocated string; otherwise returns NULL.
   EXPP is a pointer to an input string; it is updated to point to the
   text following the identifier.  If IS_PARAMETER is true, this
   function will also allow "..." forms as used in varargs macro
   parameters.  */

static gdb::unique_xmalloc_ptr<char>
extract_identifier (const char **expp, int is_parameter)
{
  char *result;
  const char *p = *expp;
  unsigned int len;

  if (is_parameter && startswith (p, "..."))
    {
      /* Ok.  */
    }
  else
    {
      if (! *p || ! macro_is_identifier_nondigit (*p))
	return NULL;
      for (++p;
	   *p && (macro_is_identifier_nondigit (*p) || macro_is_digit (*p));
	   ++p)
	;
    }

  if (is_parameter && startswith (p, "..."))      
    p += 3;

  len = p - *expp;
  result = (char *) xmalloc (len + 1);
  memcpy (result, *expp, len);
  result[len] = '\0';
  *expp += len;
  return gdb::unique_xmalloc_ptr<char> (result);
}

struct temporary_macro_definition : public macro_definition
{
  temporary_macro_definition ()
  {
    table = nullptr;
    kind = macro_object_like;
    argc = 0;
    argv = nullptr;
    replacement = nullptr;
  }

  ~temporary_macro_definition ()
  {
    int i;

    for (i = 0; i < argc; ++i)
      xfree ((char *) argv[i]);
    xfree ((char *) argv);
    /* Note that the 'replacement' field is not allocated.  */
  }
};

static void
macro_define_command (const char *exp, int from_tty)
{
  temporary_macro_definition new_macro;

  if (!exp)
    error (_("usage: macro define NAME[(ARGUMENT-LIST)] [REPLACEMENT-LIST]"));

  skip_ws (&exp);
  gdb::unique_xmalloc_ptr<char> name = extract_identifier (&exp, 0);
  if (name == NULL)
    error (_("Invalid macro name."));
  if (*exp == '(')
    {
      /* Function-like macro.  */
      int alloced = 5;
      char **argv = XNEWVEC (char *, alloced);

      new_macro.kind = macro_function_like;
      new_macro.argc = 0;
      new_macro.argv = (const char * const *) argv;

      /* Skip the '(' and whitespace.  */
      ++exp;
      skip_ws (&exp);

      while (*exp != ')')
	{
	  int i;

	  if (new_macro.argc == alloced)
	    {
	      alloced *= 2;
	      argv = (char **) xrealloc (argv, alloced * sizeof (char *));
	      /* Must update new_macro as well...  */
	      new_macro.argv = (const char * const *) argv;
	    }
	  argv[new_macro.argc] = extract_identifier (&exp, 1).release ();
	  if (! argv[new_macro.argc])
	    error (_("Macro is missing an argument."));
	  ++new_macro.argc;

	  for (i = new_macro.argc - 2; i >= 0; --i)
	    {
	      if (! strcmp (argv[i], argv[new_macro.argc - 1]))
		error (_("Two macro arguments with identical names."));
	    }

	  skip_ws (&exp);
	  if (*exp == ',')
	    {
	      ++exp;
	      skip_ws (&exp);
	    }
	  else if (*exp != ')')
	    error (_("',' or ')' expected at end of macro arguments."));
	}
      /* Skip the closing paren.  */
      ++exp;
      skip_ws (&exp);

      macro_define_function (macro_main (macro_user_macros), -1, name.get (),
			     new_macro.argc, (const char **) new_macro.argv,
			     exp);
    }
  else
    {
      skip_ws (&exp);
      macro_define_object (macro_main (macro_user_macros), -1, name.get (),
			   exp);
    }
}


static void
macro_undef_command (const char *exp, int from_tty)
{
  if (!exp)
    error (_("usage: macro undef NAME"));

  skip_ws (&exp);
  gdb::unique_xmalloc_ptr<char> name = extract_identifier (&exp, 0);
  if (name == nullptr)
    error (_("Invalid macro name."));
  macro_undef (macro_main (macro_user_macros), -1, name.get ());
}


static void
print_one_macro (const char *name, const struct macro_definition *macro,
		 struct macro_source_file *source, int line)
{
  gdb_printf ("macro define %s", name);
  if (macro->kind == macro_function_like)
    {
      int i;

      gdb_printf ("(");
      for (i = 0; i < macro->argc; ++i)
	gdb_printf ("%s%s", (i > 0) ? ", " : "",
			 macro->argv[i]);
      gdb_printf (")");
    }
  gdb_printf (" %s\n", macro->replacement);
}


static void
macro_list_command (const char *exp, int from_tty)
{
  macro_for_each (macro_user_macros, print_one_macro);
}

/* Initializing the `macrocmd' module.  */

void _initialize_macrocmd ();
void
_initialize_macrocmd ()
{
  /* We introduce a new command prefix, `macro', under which we'll put
     the various commands for working with preprocessor macros.  */
  add_basic_prefix_cmd ("macro", class_info,
			_("Prefix for commands dealing with C preprocessor macros."),
			&macrolist, 0, &cmdlist);

  cmd_list_element *macro_expand_cmd
    = add_cmd ("expand", no_class, macro_expand_command, _("\
Fully expand any C/C++ preprocessor macro invocations in EXPRESSION.\n\
Show the expanded expression."),
	       &macrolist);
  add_alias_cmd ("exp", macro_expand_cmd, no_class, 1, &macrolist);

  cmd_list_element *macro_expand_once_cmd
    = add_cmd ("expand-once", no_class, macro_expand_once_command, _("\
Expand C/C++ preprocessor macro invocations appearing directly in EXPRESSION.\n\
Show the expanded expression.\n\
\n\
This command differs from `macro expand' in that it only expands macro\n\
invocations that appear directly in EXPRESSION; if expanding a macro\n\
introduces further macro invocations, those are left unexpanded.\n\
\n\
`macro expand-once' helps you see how a particular macro expands,\n\
whereas `macro expand' shows you how all the macros involved in an\n\
expression work together to yield a pre-processed expression."),
	       &macrolist);
  add_alias_cmd ("exp1", macro_expand_once_cmd, no_class, 1, &macrolist);

  add_info ("macro", info_macro_command,
	    _("Show the definition of MACRO, and it's source location.\n\
Usage: info macro [-a|-all] [--] MACRO\n\
Options: \n\
  -a, --all    Output all definitions of MACRO in the current compilation\
 unit.\n\
  --           Specify the end of arguments and the beginning of the MACRO."));

  add_info ("macros", info_macros_command,
	    _("Show the definitions of all macros at LINESPEC, or the current \
source location.\n\
Usage: info macros [LINESPEC]"));

  add_cmd ("define", no_class, macro_define_command, _("\
Define a new C/C++ preprocessor macro.\n\
The GDB command `macro define DEFINITION' is equivalent to placing a\n\
preprocessor directive of the form `#define DEFINITION' such that the\n\
definition is visible in all the inferior's source files.\n\
For example:\n\
  (gdb) macro define PI (3.1415926)\n\
  (gdb) macro define MIN(x,y) ((x) < (y) ? (x) : (y))"),
	   &macrolist);

  add_cmd ("undef", no_class, macro_undef_command, _("\
Remove the definition of the C/C++ preprocessor macro with the given name."),
	   &macrolist);

  add_cmd ("list", no_class, macro_list_command,
	   _("List all the macros defined using the `macro define' command."),
	   &macrolist);
}
