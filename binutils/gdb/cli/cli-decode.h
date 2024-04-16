/* Header file for GDB command decoding library.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef CLI_CLI_DECODE_H
#define CLI_CLI_DECODE_H

/* This file defines the private interfaces for any code implementing
   command internals.  */

/* Include the public interfaces.  */
#include "command.h"
#include "gdbsupport/gdb_regex.h"
#include "cli-script.h"
#include "completer.h"
#include "gdbsupport/intrusive_list.h"
#include "gdbsupport/buildargv.h"

/* Not a set/show command.  Note that some commands which begin with
   "set" or "show" might be in this category, if their syntax does
   not fall into one of the following categories.  */
enum cmd_types
{
  not_set_cmd,
  set_cmd,
  show_cmd
};

/* This structure records one command'd definition.  */


struct cmd_list_element
{
  cmd_list_element (const char *name_, enum command_class theclass_,
		    const char *doc_)
    : name (name_),
      theclass (theclass_),
      cmd_deprecated (0),
      deprecated_warn_user (0),
      malloced_replacement (0),
      doc_allocated (0),
      name_allocated (0),
      hook_in (0),
      allow_unknown (0),
      abbrev_flag (0),
      type (not_set_cmd),
      doc (doc_)
  {
    memset (&function, 0, sizeof (function));
  }

  ~cmd_list_element ()
  {
    if (doc && doc_allocated)
      xfree ((char *) doc);
    if (name_allocated)
      xfree ((char *) name);
  }

  DISABLE_COPY_AND_ASSIGN (cmd_list_element);

  /* For prefix commands, return a string containing prefix commands to
     get here: this one plus any others needed to get to it.  Ends in a
     space.  It is used before the word "command" in describing the
     commands reached through this prefix.

     For non-prefix commands, return an empty string.  */
  std::string prefixname () const;

  /* Return a vector of strings describing the components of the full name
     of this command.  For example, if this command is 'set AA BB CC',
     then the vector will contain 4 elements 'set', 'AA', 'BB', and 'CC'
     in that order.  */
  std::vector<std::string> command_components () const;

  /* Return true if this command is an alias of another command.  */
  bool is_alias () const
  { return this->alias_target != nullptr; }

  /* Return true if this command is a prefix command.  */
  bool is_prefix () const
  { return this->subcommands != nullptr; }

  /* Return true if this command is a "command class help" command.  For
     instance, a "stack" dummy command is registered so that one can do
     "help stack" and show help for all commands of the "stack" class.  */
  bool is_command_class_help () const
  { return this->func == nullptr; }

  void set_context (void *context)
  {
    gdb_assert (m_context == nullptr);
    m_context = context;
  }

  void *context () const
  { return m_context; }

  /* Points to next command in this list.  */
  struct cmd_list_element *next = nullptr;

  /* Name of this command.  */
  const char *name;

  /* Command class; class values are chosen by application program.  */
  enum command_class theclass;

  /* When 1 indicated that this command is deprecated.  It may be
     removed from gdb's command set in the future.  */

  unsigned int cmd_deprecated : 1;

  /* The user needs to be warned that this is a deprecated command.
     The user should only be warned the first time a command is
     used.  */

  unsigned int deprecated_warn_user : 1;

  /* When functions are deprecated at compile time (this is the way
     it should, in general, be done) the memory containing the
     replacement string is statically allocated.  In some cases it
     makes sense to deprecate commands at runtime (the testsuite is
     one example).  In this case the memory for replacement is
     malloc'ed.  When a command is undeprecated or re-deprecated at
     runtime we don't want to risk calling free on statically
     allocated memory, so we check this flag.  */

  unsigned int malloced_replacement : 1;

  /* Set if the doc field should be xfree'd.  */

  unsigned int doc_allocated : 1;

  /* Set if the name field should be xfree'd.  */

  unsigned int name_allocated : 1;

  /* Flag that specifies if this command is already running its hook.  */
  /* Prevents the possibility of hook recursion.  */
  unsigned int hook_in : 1;

  /* For prefix commands only:
     nonzero means do not get an error if subcommand is not
     recognized; call the prefix's own function in that case.  */
  unsigned int allow_unknown : 1;

  /* Nonzero says this is an abbreviation, and should not
     be mentioned in lists of commands.
     This allows "br<tab>" to complete to "break", which it
     otherwise wouldn't.  */
  unsigned int abbrev_flag : 1;

  /* Type of "set" or "show" command (or SET_NOT_SET if not "set"
     or "show").  */
  ENUM_BITFIELD (cmd_types) type : 2;

  /* Function definition of this command.  NULL for command class
     names and for help topics that are not really commands.  NOTE:
     cagney/2002-02-02: This function signature is evolving.  For
     the moment suggest sticking with either set_cmd_cfunc() or
     set_cmd_sfunc().  */
  cmd_func_ftype *func;

  /* The command's real callback.  At present func() bounces through
     to one of the below.  */
  union
    {
      /* Most commands don't need the cmd_list_element parameter passed to FUNC.
	 They therefore register a command of this type, which doesn't have the
	 cmd_list_element parameter.  do_simple_func is installed as FUNC, and
	 acts as a shim between the two.  */
      cmd_simple_func_ftype *simple_func;
    }
  function;

  /* Documentation of this command (or help topic).
     First line is brief documentation; remaining lines form, with it,
     the full documentation.  First line should end with a period.
     Entire string should also end with a period, not a newline.  */
  const char *doc;

  /* For set/show commands.  A method for printing the output to the
     specified stream.  */
  show_value_ftype *show_value_func = nullptr;

  /* If this command is deprecated, this is the replacement name.  */
  const char *replacement = nullptr;

  /* Hook for another command to be executed before this command.  */
  struct cmd_list_element *hook_pre = nullptr;

  /* Hook for another command to be executed after this command.  */
  struct cmd_list_element *hook_post = nullptr;

  /* Default arguments to automatically prepend to the user
     provided arguments when running this command or alias.  */
  std::string default_args;

  /* Nonzero identifies a prefix command.  For them, the address
     of the variable containing the list of subcommands.  */
  struct cmd_list_element **subcommands = nullptr;

  /* The prefix command of this command.  */
  struct cmd_list_element *prefix = nullptr;

  /* Completion routine for this command.  */
  completer_ftype *completer = symbol_completer;

  /* Handle the word break characters for this completer.  Usually
     this function need not be defined, but for some types of
     completers (e.g., Python completers declared as methods inside
     a class) the word break chars may need to be redefined
     depending on the completer type (e.g., for filename
     completers).  */
  completer_handle_brkchars_ftype *completer_handle_brkchars = nullptr;

  /* Destruction routine for this command.  If non-NULL, this is
     called when this command instance is destroyed.  This may be
     used to finalize the CONTEXT field, if needed.  */
  void (*destroyer) (struct cmd_list_element *self, void *context) = nullptr;

  /* Setting affected by "set" and "show".  Not used if type is not_set_cmd.  */
  std::optional<setting> var;

  /* Pointer to NULL terminated list of enumerated values (like
     argv).  */
  const char *const *enums = nullptr;

  /* Pointer to command strings of user-defined commands */
  counted_command_line user_commands;

  /* Pointer to command that is hooked by this one, (by hook_pre)
     so the hook can be removed when this one is deleted.  */
  struct cmd_list_element *hookee_pre = nullptr;

  /* Pointer to command that is hooked by this one, (by hook_post)
     so the hook can be removed when this one is deleted.  */
  struct cmd_list_element *hookee_post = nullptr;

  /* Pointer to command that is aliased by this one, so the
     aliased command can be located in case it has been hooked.  */
  struct cmd_list_element *alias_target = nullptr;

  /* Node to link aliases on an alias list.  */
  using aliases_list_node_type
    = intrusive_list_node<cmd_list_element>;
  aliases_list_node_type aliases_list_node;

  /* Linked list of all aliases of this command.  */
  using aliases_list_member_node_type
    = intrusive_member_node<cmd_list_element,
			    &cmd_list_element::aliases_list_node>;
  using aliases_list_type
    = intrusive_list<cmd_list_element, aliases_list_member_node_type>;
  aliases_list_type aliases;

  /* If non-null, the pointer to a field in 'struct
     cli_suppress_notification', which will be set to true in cmd_func
     when this command is being executed.  It will be set back to false
     when the command has been executed.  */
  bool *suppress_notification = nullptr;

private:
  /* Local state (context) for this command.  This can be anything.  */
  void *m_context = nullptr;
};

/* Functions that implement commands about CLI commands.  */

extern void help_cmd (const char *, struct ui_file *);

extern void apropos_cmd (struct ui_file *, struct cmd_list_element *,
			 bool verbose, compiled_regex &);

/* Used to mark commands that don't do anything.  If we just leave the
   function field NULL, the command is interpreted as a help topic, or
   as a class of commands.  */

extern void not_just_help_class_command (const char *arg, int from_tty);

/* Print only the first line of STR on STREAM.
   FOR_VALUE_PREFIX true indicates that the first line is output
   to be a prefix to show a value (see deprecated_show_value_hack):
   the first character is printed in uppercase, and the trailing
   dot character is not printed.  */

extern void print_doc_line (struct ui_file *stream, const char *str,
			    bool for_value_prefix);

/* The enums of boolean commands.  */
extern const char * const boolean_enums[];

/* The enums of auto-boolean commands.  */
extern const char * const auto_boolean_enums[];

/* Verify whether a given cmd_list_element is a user-defined command.
   Return 1 if it is user-defined.  Return 0 otherwise.  */

extern int cli_user_command_p (struct cmd_list_element *);

extern int find_command_name_length (const char *);

#endif /* CLI_CLI_DECODE_H */
