/* General Compile and inject code

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
#include "ui.h"
#include "ui-out.h"
#include "command.h"
#include "cli/cli-script.h"
#include "cli/cli-utils.h"
#include "cli/cli-option.h"
#include "completer.h"
#include "gdbcmd.h"
#include "compile.h"
#include "compile-internal.h"
#include "compile-object-load.h"
#include "compile-object-run.h"
#include "language.h"
#include "frame.h"
#include "source.h"
#include "block.h"
#include "arch-utils.h"
#include "gdbsupport/filestuff.h"
#include "target.h"
#include "osabi.h"
#include "gdbsupport/gdb_wait.h"
#include "valprint.h"
#include <optional>
#include "gdbsupport/gdb_unlinker.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_ignore_signal.h"
#include "gdbsupport/buildargv.h"



/* Initial filename for temporary files.  */

#define TMP_PREFIX "/tmp/gdbobj-"

/* Hold "compile" commands.  */

static struct cmd_list_element *compile_command_list;

/* Debug flag for "compile" commands.  */

bool compile_debug;

/* Object of this type are stored in the compiler's symbol_err_map.  */

struct symbol_error
{
  /* The symbol.  */

  const struct symbol *sym;

  /* The error message to emit.  This is malloc'd and owned by the
     hash table.  */

  char *message;
};

/* An object that maps a gdb type to a gcc type.  */

struct type_map_instance
{
  /* The gdb type.  */

  struct type *type;

  /* The corresponding gcc type handle.  */

  gcc_type gcc_type_handle;
};

/* Hash a type_map_instance.  */

static hashval_t
hash_type_map_instance (const void *p)
{
  const struct type_map_instance *inst = (const struct type_map_instance *) p;

  return htab_hash_pointer (inst->type);
}

/* Check two type_map_instance objects for equality.  */

static int
eq_type_map_instance (const void *a, const void *b)
{
  const struct type_map_instance *insta = (const struct type_map_instance *) a;
  const struct type_map_instance *instb = (const struct type_map_instance *) b;

  return insta->type == instb->type;
}

/* Hash function for struct symbol_error.  */

static hashval_t
hash_symbol_error (const void *a)
{
  const struct symbol_error *se = (const struct symbol_error *) a;

  return htab_hash_pointer (se->sym);
}

/* Equality function for struct symbol_error.  */

static int
eq_symbol_error (const void *a, const void *b)
{
  const struct symbol_error *sea = (const struct symbol_error *) a;
  const struct symbol_error *seb = (const struct symbol_error *) b;

  return sea->sym == seb->sym;
}

/* Deletion function for struct symbol_error.  */

static void
del_symbol_error (void *a)
{
  struct symbol_error *se = (struct symbol_error *) a;

  xfree (se->message);
  xfree (se);
}

/* Constructor for compile_instance.  */

compile_instance::compile_instance (struct gcc_base_context *gcc_fe,
				    const char *options)
  : m_gcc_fe (gcc_fe), m_gcc_target_options (options),
    m_type_map (htab_create_alloc (10, hash_type_map_instance,
				   eq_type_map_instance,
				   xfree, xcalloc, xfree)),
    m_symbol_err_map (htab_create_alloc (10, hash_symbol_error,
					 eq_symbol_error, del_symbol_error,
					 xcalloc, xfree))
{
}

/* See compile-internal.h.  */

bool
compile_instance::get_cached_type (struct type *type, gcc_type *ret) const
{
  struct type_map_instance inst, *found;

  inst.type = type;
  found = (struct type_map_instance *) htab_find (m_type_map.get (), &inst);
  if (found != NULL)
    {
      *ret = found->gcc_type_handle;
      return true;
    }

  return false;
}

/* See compile-internal.h.  */

void
compile_instance::insert_type (struct type *type, gcc_type gcc_type)
{
  struct type_map_instance inst, *add;
  void **slot;

  inst.type = type;
  inst.gcc_type_handle = gcc_type;
  slot = htab_find_slot (m_type_map.get (), &inst, INSERT);

  add = (struct type_map_instance *) *slot;
  /* The type might have already been inserted in order to handle
     recursive types.  */
  if (add != NULL && add->gcc_type_handle != gcc_type)
    error (_("Unexpected type id from GCC, check you use recent enough GCC."));

  if (add == NULL)
    {
      add = XNEW (struct type_map_instance);
      *add = inst;
      *slot = add;
    }
}

/* See compile-internal.h.  */

void
compile_instance::insert_symbol_error (const struct symbol *sym,
				       const char *text)
{
  struct symbol_error e;
  void **slot;

  e.sym = sym;
  slot = htab_find_slot (m_symbol_err_map.get (), &e, INSERT);
  if (*slot == NULL)
    {
      struct symbol_error *ep = XNEW (struct symbol_error);

      ep->sym = sym;
      ep->message = xstrdup (text);
      *slot = ep;
    }
}

/* See compile-internal.h.  */

void
compile_instance::error_symbol_once (const struct symbol *sym)
{
  struct symbol_error search;
  struct symbol_error *err;

  if (m_symbol_err_map == NULL)
    return;

  search.sym = sym;
  err = (struct symbol_error *) htab_find (m_symbol_err_map.get (), &search);
  if (err == NULL || err->message == NULL)
    return;

  gdb::unique_xmalloc_ptr<char> message (err->message);
  err->message = NULL;
  error (_("%s"), message.get ());
}

/* Implement "show debug compile".  */

static void
show_compile_debug (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Compile debugging is %s.\n"), value);
}



/* Options for the compile command.  */

struct compile_options
{
  /* For -raw.  */
  bool raw = false;
};

using compile_flag_option_def
  = gdb::option::flag_option_def<compile_options>;

static const gdb::option::option_def compile_command_option_defs[] = {

  compile_flag_option_def {
    "raw",
    [] (compile_options *opts) { return &opts->raw; },
    N_("Suppress automatic 'void _gdb_expr () { CODE }' wrapping."),
  },

};

/* Create an option_def_group for the "compile" command's options,
   with OPTS as context.  */

static gdb::option::option_def_group
make_compile_options_def_group (compile_options *opts)
{
  return {{compile_command_option_defs}, opts};
}

/* Handle the input from the 'compile file' command.  The "compile
   file" command is used to evaluate an expression contained in a file
   that may contain calls to the GCC compiler.  */

static void
compile_file_command (const char *args, int from_tty)
{
  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  /* Check if a -raw option is provided.  */

  compile_options options;

  const gdb::option::option_def_group group
    = make_compile_options_def_group (&options);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR,
     group);

  enum compile_i_scope_types scope
    = options.raw ? COMPILE_I_RAW_SCOPE : COMPILE_I_SIMPLE_SCOPE;

  args = skip_spaces (args);

  /* After processing options, check whether we have a filename.  */
  if (args == nullptr || args[0] == '\0')
    error (_("You must provide a filename for this command."));

  args = skip_spaces (args);
  std::string abspath = gdb_abspath (args);
  std::string buffer = string_printf ("#include \"%s\"\n", abspath.c_str ());
  eval_compile_command (NULL, buffer.c_str (), scope, NULL);
}

/* Completer for the "compile file" command.  */

static void
compile_file_command_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char *word)
{
  const gdb::option::option_def_group group
    = make_compile_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group))
    return;

  word = advance_to_filename_complete_word_point (tracker, text);
  filename_completer (ignore, tracker, text, word);
}

/* Handle the input from the 'compile code' command.  The
   "compile code" command is used to evaluate an expression that may
   contain calls to the GCC compiler.  The language expected in this
   compile command is the language currently set in GDB.  */

static void
compile_code_command (const char *args, int from_tty)
{
  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  compile_options options;

  const gdb::option::option_def_group group
    = make_compile_options_def_group (&options);
  gdb::option::process_options
    (&args, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group);

  enum compile_i_scope_types scope
    = options.raw ? COMPILE_I_RAW_SCOPE : COMPILE_I_SIMPLE_SCOPE;

  if (args && *args)
    eval_compile_command (NULL, args, scope, NULL);
  else
    {
      counted_command_line l = get_command_line (compile_control, "");

      l->control_u.compile.scope = scope;
      execute_control_command_untraced (l.get ());
    }
}

/* Completer for the "compile code" command.  */

static void
compile_code_command_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char *word)
{
  const gdb::option::option_def_group group
    = make_compile_options_def_group (nullptr);
  if (gdb::option::complete_options
      (tracker, &text, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_ERROR, group))
    return;

  word = advance_to_expression_complete_word_point (tracker, text);
  symbol_completer (ignore, tracker, text, word);
}

/* Callback for compile_print_command.  */

void
compile_print_value (struct value *val, void *data_voidp)
{
  const value_print_options *print_opts = (value_print_options *) data_voidp;

  print_value (val, *print_opts);
}

/* Handle the input from the 'compile print' command.  The "compile
   print" command is used to evaluate and print an expression that may
   contain calls to the GCC compiler.  The language expected in this
   compile command is the language currently set in GDB.  */

static void
compile_print_command (const char *arg, int from_tty)
{
  enum compile_i_scope_types scope = COMPILE_I_PRINT_ADDRESS_SCOPE;
  value_print_options print_opts;

  scoped_restore save_async = make_scoped_restore (&current_ui->async, 0);

  get_user_print_options (&print_opts);
  /* Override global settings with explicit options, if any.  */
  auto group = make_value_print_options_def_group (&print_opts);
  gdb::option::process_options
    (&arg, gdb::option::PROCESS_OPTIONS_REQUIRE_DELIMITER, group);

  print_command_parse_format (&arg, "compile print", &print_opts);

  /* Passing &PRINT_OPTS as SCOPE_DATA is safe as do_module_cleanup
     will not touch the stale pointer if compile_object_run has
     already quit.  */

  if (arg && *arg)
    eval_compile_command (NULL, arg, scope, &print_opts);
  else
    {
      counted_command_line l = get_command_line (compile_control, "");

      l->control_u.compile.scope = scope;
      l->control_u.compile.scope_data = &print_opts;
      execute_control_command_untraced (l.get ());
    }
}

/* A cleanup function to remove a directory and all its contents.  */

static void
do_rmdir (void *arg)
{
  const char *dir = (const char *) arg;
  char *zap;
  int wstat;

  gdb_assert (startswith (dir, TMP_PREFIX));
  zap = concat ("rm -rf ", dir, (char *) NULL);
  wstat = system (zap);
  if (wstat == -1 || !WIFEXITED (wstat) || WEXITSTATUS (wstat) != 0)
    warning (_("Could not remove temporary directory %s"), dir);
  XDELETEVEC (zap);
}

/* Return the name of the temporary directory to use for .o files, and
   arrange for the directory to be removed at shutdown.  */

static const char *
get_compile_file_tempdir (void)
{
  static char *tempdir_name;

#define TEMPLATE TMP_PREFIX "XXXXXX"
  char tname[sizeof (TEMPLATE)];

  if (tempdir_name != NULL)
    return tempdir_name;

  strcpy (tname, TEMPLATE);
#undef TEMPLATE
  tempdir_name = mkdtemp (tname);
  if (tempdir_name == NULL)
    perror_with_name (_("Could not make temporary directory"));

  tempdir_name = xstrdup (tempdir_name);
  make_final_cleanup (do_rmdir, tempdir_name);
  return tempdir_name;
}

/* Compute the names of source and object files to use.  */

static compile_file_names
get_new_file_names ()
{
  static int seq;
  const char *dir = get_compile_file_tempdir ();

  ++seq;

  return compile_file_names (string_printf ("%s%sout%d.c",
					    dir, SLASH_STRING, seq),
			     string_printf ("%s%sout%d.o",
					    dir, SLASH_STRING, seq));
}

/* Get the block and PC at which to evaluate an expression.  */

static const struct block *
get_expr_block_and_pc (CORE_ADDR *pc)
{
  const struct block *block = get_selected_block (pc);

  if (block == NULL)
    {
      struct symtab_and_line cursal = get_current_source_symtab_and_line ();

      if (cursal.symtab)
	block = cursal.symtab->compunit ()->blockvector ()->static_block ();

      if (block != NULL)
	*pc = block->entry_pc ();
    }
  else
    *pc = block->entry_pc ();

  return block;
}

/* String for 'set compile-args' and 'show compile-args'.  */
static std::string compile_args =
  /* Override flags possibly coming from DW_AT_producer.  */
  "-O0 -gdwarf-4"
  /* We use -fPIE Otherwise GDB would need to reserve space large enough for
     any object file in the inferior in advance to get the final address when
     to link the object file to and additionally the default system linker
     script would need to be modified so that one can specify there the
     absolute target address.
     -fPIC is not used at is would require from GDB to generate .got.  */
  " -fPIE"
  /* We want warnings, except for some commonly happening for GDB commands.  */
  " -Wall "
  " -Wno-unused-but-set-variable"
  " -Wno-unused-variable"
  /* Override CU's possible -fstack-protector-strong.  */
  " -fno-stack-protector";

/* Parsed form of COMPILE_ARGS.  */
static gdb_argv compile_args_argv;

/* Implement 'set compile-args'.  */

static void
set_compile_args (const char *args, int from_tty, struct cmd_list_element *c)
{
  compile_args_argv = gdb_argv (compile_args.c_str ());
}

/* Implement 'show compile-args'.  */

static void
show_compile_args (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Compile command command-line arguments "
		      "are \"%s\".\n"),
	      value);
}

/* String for 'set compile-gcc' and 'show compile-gcc'.  */
static std::string compile_gcc;

/* Implement 'show compile-gcc'.  */

static void
show_compile_gcc (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Compile command GCC driver filename is \"%s\".\n"),
	      value);
}

/* Return DW_AT_producer parsed for get_selected_frame () (if any).
   Return NULL otherwise.

   GCC already filters its command-line arguments only for the suitable ones to
   put into DW_AT_producer - see GCC function gen_producer_string.  */

static const char *
get_selected_pc_producer_options (void)
{
  CORE_ADDR pc = get_frame_pc (get_selected_frame (NULL));
  struct compunit_symtab *symtab = find_pc_compunit_symtab (pc);
  const char *cs;

  if (symtab == NULL || symtab->producer () == NULL
      || !startswith (symtab->producer (), "GNU "))
    return NULL;

  cs = symtab->producer ();
  while (*cs != 0 && *cs != '-')
    cs = skip_spaces (skip_to_space (cs));
  if (*cs != '-')
    return NULL;
  return cs;
}

/* Filter out unwanted options from ARGV.  */

static void
filter_args (char **argv)
{
  char **destv;

  for (destv = argv; *argv != NULL; argv++)
    {
      /* -fpreprocessed may get in commonly from ccache.  */
      if (strcmp (*argv, "-fpreprocessed") == 0)
	{
	  xfree (*argv);
	  continue;
	}
      *destv++ = *argv;
    }
  *destv = NULL;
}

/* Produce final vector of GCC compilation options.

   The first element of the combined argument vector are arguments
   relating to the target size ("-m64", "-m32" etc.).  These are
   sourced from the inferior's architecture.

   The second element of the combined argument vector are arguments
   stored in the inferior DW_AT_producer section.  If these are stored
   in the inferior (there is no guarantee that they are), they are
   added to the vector.

   The third element of the combined argument vector are argument
   supplied by the language implementation provided by
   compile-{lang}-support.  These contain language specific arguments.

   The final element of the combined argument vector are arguments
   supplied by the "set compile-args" command.  These are always
   appended last so as to override any of the arguments automatically
   generated above.  */

static gdb_argv
get_args (const compile_instance *compiler, struct gdbarch *gdbarch)
{
  const char *cs_producer_options;
  gdb_argv result;

  std::string gcc_options = gdbarch_gcc_target_options (gdbarch);

  /* Make sure we have a non-empty set of options, otherwise GCC will
     error out trying to look for a filename that is an empty string.  */
  if (!gcc_options.empty ())
    result = gdb_argv (gcc_options.c_str ());

  cs_producer_options = get_selected_pc_producer_options ();
  if (cs_producer_options != NULL)
    {
      gdb_argv argv_producer (cs_producer_options);
      filter_args (argv_producer.get ());

      result.append (std::move (argv_producer));
    }

  result.append (gdb_argv (compiler->gcc_target_options ().c_str ()));
  result.append (compile_args_argv);

  return result;
}

/* A helper function suitable for use as the "print_callback" in the
   compiler object.  */

static void
print_callback (void *ignore, const char *message)
{
  gdb_puts (message, gdb_stderr);
}

/* Process the compilation request.  On success it returns the object
   and source file names.  On an error condition, error () is
   called.  */

static compile_file_names
compile_to_object (struct command_line *cmd, const char *cmd_string,
		   enum compile_i_scope_types scope)
{
  const struct block *expr_block;
  CORE_ADDR trash_pc, expr_pc;
  int ok;
  struct gdbarch *gdbarch = get_current_arch ();
  std::string triplet_rx;

  if (!target_has_execution ())
    error (_("The program must be running for the compile command to "\
	     "work."));

  expr_block = get_expr_block_and_pc (&trash_pc);
  expr_pc = get_frame_address_in_block (get_selected_frame (NULL));

  /* Set up instance and context for the compiler.  */
  std::unique_ptr<compile_instance> compiler
    = current_language->get_compile_instance ();
  if (compiler == nullptr)
    error (_("No compiler support for language %s."),
	   current_language->name ());
  compiler->set_print_callback (print_callback, NULL);
  compiler->set_scope (scope);
  compiler->set_block (expr_block);

  /* From the provided expression, build a scope to pass to the
     compiler.  */

  string_file input_buf;
  const char *input;

  if (cmd != NULL)
    {
      struct command_line *iter;

      for (iter = cmd->body_list_0.get (); iter; iter = iter->next)
	{
	  input_buf.puts (iter->line);
	  input_buf.puts ("\n");
	}

      input = input_buf.c_str ();
    }
  else if (cmd_string != NULL)
    input = cmd_string;
  else
    error (_("Neither a simple expression, or a multi-line specified."));

  std::string code
    = current_language->compute_program (compiler.get (), input, gdbarch,
					 expr_block, expr_pc);
  if (compile_debug)
    gdb_printf (gdb_stdlog, "debug output:\n\n%s", code.c_str ());

  compiler->set_verbose (compile_debug);

  if (!compile_gcc.empty ())
    {
      if (compiler->version () < GCC_FE_VERSION_1)
	error (_("Command 'set compile-gcc' requires GCC version 6 or higher "
		 "(libcc1 interface version 1 or higher)"));

      compiler->set_driver_filename (compile_gcc.c_str ());
    }
  else
    {
      const char *os_rx = osabi_triplet_regexp (gdbarch_osabi (gdbarch));
      const char *arch_rx = gdbarch_gnu_triplet_regexp (gdbarch);

      /* Allow triplets with or without vendor set.  */
      triplet_rx = std::string (arch_rx) + "(-[^-]*)?-";
      if (os_rx != nullptr)
	triplet_rx += os_rx;
      compiler->set_triplet_regexp (triplet_rx.c_str ());
    }

  /* Set compiler command-line arguments.  */
  gdb_argv argv_holder = get_args (compiler.get (), gdbarch);
  int argc = argv_holder.count ();
  char **argv = argv_holder.get ();

  gdb::unique_xmalloc_ptr<char> error_message
    = compiler->set_arguments (argc, argv, triplet_rx.c_str ());

  if (error_message != NULL)
    error ("%s", error_message.get ());

  if (compile_debug)
    {
      int argi;

      gdb_printf (gdb_stdlog, "Passing %d compiler options:\n", argc);
      for (argi = 0; argi < argc; argi++)
	gdb_printf (gdb_stdlog, "Compiler option %d: <%s>\n",
		    argi, argv[argi]);
    }

  compile_file_names fnames = get_new_file_names ();

  std::optional<gdb::unlinker> source_remover;

  {
    gdb_file_up src = gdb_fopen_cloexec (fnames.source_file (), "w");
    if (src == NULL)
      perror_with_name (_("Could not open source file for writing"));

    source_remover.emplace (fnames.source_file ());

    if (fputs (code.c_str (), src.get ()) == EOF)
      perror_with_name (_("Could not write to source file"));
  }

  if (compile_debug)
    gdb_printf (gdb_stdlog, "source file produced: %s\n\n",
		fnames.source_file ());

  /* If we don't do this, then GDB simply exits
     when the compiler dies.  */
  scoped_ignore_sigpipe ignore_sigpipe;

  /* Call the compiler and start the compilation process.  */
  compiler->set_source_file (fnames.source_file ());
  ok = compiler->compile (fnames.object_file (), compile_debug);
  if (!ok)
    error (_("Compilation failed."));

  if (compile_debug)
    gdb_printf (gdb_stdlog, "object file produced: %s\n\n",
		fnames.object_file ());

  /* Keep the source file.  */
  source_remover->keep ();
  return fnames;
}

/* The "compile" prefix command.  */

static void
compile_command (const char *args, int from_tty)
{
  /* If a sub-command is not specified to the compile prefix command,
     assume it is a direct code compilation.  */
  compile_code_command (args, from_tty);
}

/* See compile.h.  */

void
eval_compile_command (struct command_line *cmd, const char *cmd_string,
		      enum compile_i_scope_types scope, void *scope_data)
{
  compile_file_names fnames = compile_to_object (cmd, cmd_string, scope);

  gdb::unlinker object_remover (fnames.object_file ());
  gdb::unlinker source_remover (fnames.source_file ());

  compile_module_up compile_module = compile_object_load (fnames, scope,
							  scope_data);
  if (compile_module == NULL)
    {
      gdb_assert (scope == COMPILE_I_PRINT_ADDRESS_SCOPE);
      eval_compile_command (cmd, cmd_string,
			    COMPILE_I_PRINT_VALUE_SCOPE, scope_data);
      return;
    }

  /* Keep the files.  */
  source_remover.keep ();
  object_remover.keep ();

  compile_object_run (std::move (compile_module));
}

/* See compile/compile-internal.h.  */

std::string
compile_register_name_mangled (struct gdbarch *gdbarch, int regnum)
{
  const char *regname = gdbarch_register_name (gdbarch, regnum);

  return string_printf ("__%s", regname);
}

/* See compile/compile-internal.h.  */

int
compile_register_name_demangle (struct gdbarch *gdbarch,
				 const char *regname)
{
  int regnum;

  if (regname[0] != '_' || regname[1] != '_')
    error (_("Invalid register name \"%s\"."), regname);
  regname += 2;

  for (regnum = 0; regnum < gdbarch_num_regs (gdbarch); regnum++)
    if (strcmp (regname, gdbarch_register_name (gdbarch, regnum)) == 0)
      return regnum;

  error (_("Cannot find gdbarch register \"%s\"."), regname);
}

/* Forwards to the plug-in.  */

#define FORWARD(OP,...) (m_gcc_fe->ops->OP (m_gcc_fe, ##__VA_ARGS__))

/* See compile-internal.h.  */

void
compile_instance::set_print_callback
  (void (*print_function) (void *, const char *), void *datum)
{
  FORWARD (set_print_callback, print_function, datum);
}

/* See compile-internal.h.  */

unsigned int
compile_instance::version () const
{
  return m_gcc_fe->ops->version;
}

/* See compile-internal.h.  */

void
compile_instance::set_verbose (int level)
{
  if (version () >= GCC_FE_VERSION_1)
    FORWARD (set_verbose, level);
}

/* See compile-internal.h.  */

void
compile_instance::set_driver_filename (const char *filename)
{
  if (version () >= GCC_FE_VERSION_1)
    FORWARD (set_driver_filename, filename);
}

/* See compile-internal.h.  */

void
compile_instance::set_triplet_regexp (const char *regexp)
{
  if (version () >= GCC_FE_VERSION_1)
    FORWARD (set_triplet_regexp, regexp);
}

/* See compile-internal.h.  */

gdb::unique_xmalloc_ptr<char>
compile_instance::set_arguments (int argc, char **argv, const char *regexp)
{
  if (version () >= GCC_FE_VERSION_1)
    return gdb::unique_xmalloc_ptr<char> (FORWARD (set_arguments, argc, argv));
  else
    return gdb::unique_xmalloc_ptr<char> (FORWARD (set_arguments_v0, regexp,
						   argc, argv));
}

/* See compile-internal.h.  */

void
compile_instance::set_source_file (const char *filename)
{
  FORWARD (set_source_file, filename);
}

/* See compile-internal.h.  */

bool
compile_instance::compile (const char *filename, int verbose_level)
{
  if (version () >= GCC_FE_VERSION_1)
    return FORWARD (compile, filename);
  else
    return FORWARD (compile_v0, filename, verbose_level);
}

#undef FORWARD

/* See compile.h.  */
cmd_list_element *compile_cmd_element = nullptr;

void _initialize_compile ();
void
_initialize_compile ()
{
  struct cmd_list_element *c = NULL;

  compile_cmd_element = add_prefix_cmd ("compile", class_obscure,
					compile_command, _("\
Command to compile source code and inject it into the inferior."),
		  &compile_command_list, 1, &cmdlist);
  add_com_alias ("expression", compile_cmd_element, class_obscure, 0);

  const auto compile_opts = make_compile_options_def_group (nullptr);

  static const std::string compile_code_help
    = gdb::option::build_help (_("\
Compile, inject, and execute code.\n\
\n\
Usage: compile code [OPTION]... [CODE]\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
The source code may be specified as a simple one line expression, e.g.:\n\
\n\
    compile code printf(\"Hello world\\n\");\n\
\n\
Alternatively, you can type a multiline expression by invoking\n\
this command with no argument.  GDB will then prompt for the\n\
expression interactively; type a line containing \"end\" to\n\
indicate the end of the expression."),
			       compile_opts);

  c = add_cmd ("code", class_obscure, compile_code_command,
	       compile_code_help.c_str (),
	       &compile_command_list);
  set_cmd_completer_handle_brkchars (c, compile_code_command_completer);

static const std::string compile_file_help
    = gdb::option::build_help (_("\
Evaluate a file containing source code.\n\
\n\
Usage: compile file [OPTION].. [FILENAME]\n\
\n\
Options:\n\
%OPTIONS%"),
			       compile_opts);

  c = add_cmd ("file", class_obscure, compile_file_command,
	       compile_file_help.c_str (),
	       &compile_command_list);
  set_cmd_completer_handle_brkchars (c, compile_file_command_completer);

  const auto compile_print_opts = make_value_print_options_def_group (nullptr);

  static const std::string compile_print_help
    = gdb::option::build_help (_("\
Evaluate EXPR by using the compiler and print result.\n\
\n\
Usage: compile print [[OPTION]... --] [/FMT] [EXPR]\n\
\n\
Options:\n\
%OPTIONS%\n\
\n\
Note: because this command accepts arbitrary expressions, if you\n\
specify any command option, you must use a double dash (\"--\")\n\
to mark the end of option processing.  E.g.: \"compile print -o -- myobj\".\n\
\n\
The expression may be specified on the same line as the command, e.g.:\n\
\n\
    compile print i\n\
\n\
Alternatively, you can type a multiline expression by invoking\n\
this command with no argument.  GDB will then prompt for the\n\
expression interactively; type a line containing \"end\" to\n\
indicate the end of the expression.\n\
\n\
EXPR may be preceded with /FMT, where FMT is a format letter\n\
but no count or size letter (see \"x\" command)."),
			       compile_print_opts);

  c = add_cmd ("print", class_obscure, compile_print_command,
	       compile_print_help.c_str (),
	       &compile_command_list);
  set_cmd_completer_handle_brkchars (c, print_command_completer);

  add_setshow_boolean_cmd ("compile", class_maintenance, &compile_debug, _("\
Set compile command debugging."), _("\
Show compile command debugging."), _("\
When on, compile command debugging is enabled."),
			   NULL, show_compile_debug,
			   &setdebuglist, &showdebuglist);

  add_setshow_string_cmd ("compile-args", class_support,
			  &compile_args,
			  _("Set compile command GCC command-line arguments."),
			  _("Show compile command GCC command-line arguments."),
			  _("\
Use options like -I (include file directory) or ABI settings.\n\
String quoting is parsed like in shell, for example:\n\
  -mno-align-double \"-I/dir with a space/include\""),
			  set_compile_args, show_compile_args, &setlist, &showlist);


  /* Initialize compile_args_argv.  */
  set_compile_args (compile_args.c_str (), 0, NULL);

  add_setshow_optional_filename_cmd ("compile-gcc", class_support,
				     &compile_gcc,
				     _("Set compile command "
				       "GCC driver filename."),
				     _("Show compile command "
				       "GCC driver filename."),
				     _("\
It should be absolute filename of the gcc executable.\n\
If empty the default target triplet will be searched in $PATH."),
				     NULL, show_compile_gcc, &setlist,
				     &showlist);
}
