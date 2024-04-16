/* Handle lists of commands, their decoding and documentation, for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include <ctype.h>
#include "gdbsupport/gdb_regex.h"
#include "completer.h"
#include "ui-out.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-style.h"
#include <optional>

/* Prototypes for local functions.  */

static void undef_cmd_error (const char *, const char *);

static cmd_list_element::aliases_list_type delete_cmd
  (const char *name, cmd_list_element **list, cmd_list_element **prehook,
   cmd_list_element **prehookee, cmd_list_element **posthook,
   cmd_list_element **posthookee);

static struct cmd_list_element *find_cmd (const char *command,
					  int len,
					  struct cmd_list_element *clist,
					  int ignore_help_classes,
					  int *nfound);

static void help_cmd_list (struct cmd_list_element *list,
			   enum command_class theclass,
			   bool recurse,
			   struct ui_file *stream);

static void help_all (struct ui_file *stream);

static int lookup_cmd_composition_1 (const char *text,
				     struct cmd_list_element **alias,
				     struct cmd_list_element **prefix_cmd,
				     struct cmd_list_element **cmd,
				     struct cmd_list_element *cur_list);

/* Look up a command whose 'subcommands' field is SUBCOMMANDS.  Return the
   command if found, otherwise return NULL.  */

static struct cmd_list_element *
lookup_cmd_with_subcommands (cmd_list_element **subcommands,
			     cmd_list_element *list)
{
  struct cmd_list_element *p = NULL;

  for (p = list; p != NULL; p = p->next)
    {
      struct cmd_list_element *q;

      if (!p->is_prefix ())
	continue;

      else if (p->subcommands == subcommands)
	{
	  /* If we found an alias, we must return the aliased
	     command.  */
	  return p->is_alias () ? p->alias_target : p;
	}

      q = lookup_cmd_with_subcommands (subcommands, *(p->subcommands));
      if (q != NULL)
	return q;
    }

  return NULL;
}

static void
print_help_for_command (const cmd_list_element &c,
			bool recurse, struct ui_file *stream);

static void
do_simple_func (const char *args, int from_tty, cmd_list_element *c)
{
  c->function.simple_func (args, from_tty);
}

static void
set_cmd_simple_func (struct cmd_list_element *cmd, cmd_simple_func_ftype *simple_func)
{
  if (simple_func == NULL)
    cmd->func = NULL;
  else
    cmd->func = do_simple_func;

  cmd->function.simple_func = simple_func;
}

int
cmd_simple_func_eq (struct cmd_list_element *cmd, cmd_simple_func_ftype *simple_func)
{
  return (cmd->func == do_simple_func
	  && cmd->function.simple_func == simple_func);
}

void
set_cmd_completer (struct cmd_list_element *cmd, completer_ftype *completer)
{
  cmd->completer = completer; /* Ok.  */
}

/* See definition in commands.h.  */

void
set_cmd_completer_handle_brkchars (struct cmd_list_element *cmd,
				   completer_handle_brkchars_ftype *func)
{
  cmd->completer_handle_brkchars = func;
}

std::string
cmd_list_element::prefixname () const
{
  if (!this->is_prefix ())
    /* Not a prefix command.  */
    return "";

  std::string prefixname;
  if (this->prefix != nullptr)
    prefixname = this->prefix->prefixname ();

  prefixname += this->name;
  prefixname += " ";

  return prefixname;
}

/* See cli/cli-decode.h.  */

std::vector<std::string>
cmd_list_element::command_components () const
{
  std::vector<std::string> result;

  if (this->prefix != nullptr)
    result = this->prefix->command_components ();

  result.emplace_back (this->name);
  return result;
}

/* Add element named NAME.
   Space for NAME and DOC must be allocated by the caller.
   THECLASS is the top level category into which commands are broken down
   for "help" purposes.
   FUN should be the function to execute the command;
   it will get a character string as argument, with leading
   and trailing blanks already eliminated.

   DOC is a documentation string for the command.
   Its first line should be a complete sentence.
   It should start with ? for a command that is an abbreviation
   or with * for a command that most users don't need to know about.

   Add this command to command list *LIST.

   Returns a pointer to the added command (not necessarily the head 
   of *LIST).  */

static struct cmd_list_element *
do_add_cmd (const char *name, enum command_class theclass,
	    const char *doc, struct cmd_list_element **list)
{
  struct cmd_list_element *c = new struct cmd_list_element (name, theclass,
							    doc);

  /* Turn each alias of the old command into an alias of the new
     command.  */
  c->aliases = delete_cmd (name, list, &c->hook_pre, &c->hookee_pre,
			   &c->hook_post, &c->hookee_post);

  for (cmd_list_element &alias : c->aliases)
    alias.alias_target = c;

  if (c->hook_pre)
    c->hook_pre->hookee_pre = c;

  if (c->hookee_pre)
    c->hookee_pre->hook_pre = c;

  if (c->hook_post)
    c->hook_post->hookee_post = c;

  if (c->hookee_post)
    c->hookee_post->hook_post = c;

  if (*list == NULL || strcmp ((*list)->name, name) >= 0)
    {
      c->next = *list;
      *list = c;
    }
  else
    {
      cmd_list_element *p = *list;
      while (p->next && strcmp (p->next->name, name) <= 0)
	{
	  p = p->next;
	}
      c->next = p->next;
      p->next = c;
    }

  /* Search the prefix cmd of C, and assigns it to C->prefix.
     See also add_prefix_cmd and update_prefix_field_of_prefixed_commands.  */
  cmd_list_element *prefixcmd = lookup_cmd_with_subcommands (list, cmdlist);
  c->prefix = prefixcmd;


  return c;
}

struct cmd_list_element *
add_cmd (const char *name, enum command_class theclass,
	 const char *doc, struct cmd_list_element **list)
{
  cmd_list_element *result = do_add_cmd (name, theclass, doc, list);
  result->func = NULL;
  result->function.simple_func = NULL;
  return result;
}

struct cmd_list_element *
add_cmd (const char *name, enum command_class theclass,
	 cmd_simple_func_ftype *fun,
	 const char *doc, struct cmd_list_element **list)
{
  cmd_list_element *result = do_add_cmd (name, theclass, doc, list);
  set_cmd_simple_func (result, fun);
  return result;
}

/* Add an element with a suppress notification to the LIST of commands.  */

struct cmd_list_element *
add_cmd_suppress_notification (const char *name, enum command_class theclass,
			       cmd_simple_func_ftype *fun, const char *doc,
			       struct cmd_list_element **list,
			       bool *suppress_notification)
{
  struct cmd_list_element *element;

  element = add_cmd (name, theclass, fun, doc, list);
  element->suppress_notification = suppress_notification;

  return element;
}


/* Deprecates a command CMD.
   REPLACEMENT is the name of the command which should be used in
   place of this command, or NULL if no such command exists.

   This function does not check to see if command REPLACEMENT exists
   since gdb may not have gotten around to adding REPLACEMENT when
   this function is called.

   Returns a pointer to the deprecated command.  */

struct cmd_list_element *
deprecate_cmd (struct cmd_list_element *cmd, const char *replacement)
{
  cmd->cmd_deprecated = 1;
  cmd->deprecated_warn_user = 1;

  if (replacement != NULL)
    cmd->replacement = replacement;
  else
    cmd->replacement = NULL;

  return cmd;
}

struct cmd_list_element *
add_alias_cmd (const char *name, cmd_list_element *target,
	       enum command_class theclass, int abbrev_flag,
	       struct cmd_list_element **list)
{
  gdb_assert (target != nullptr);

  struct cmd_list_element *c = add_cmd (name, theclass, target->doc, list);

  /* If TARGET->DOC can be freed, we should make another copy.  */
  if (target->doc_allocated)
    {
      c->doc = xstrdup (target->doc);
      c->doc_allocated = 1;
    }
  /* NOTE: Both FUNC and all the FUNCTIONs need to be copied.  */
  c->func = target->func;
  c->function = target->function;
  c->subcommands = target->subcommands;
  c->allow_unknown = target->allow_unknown;
  c->abbrev_flag = abbrev_flag;
  c->alias_target = target;
  target->aliases.push_front (*c);

  return c;
}

/* Update the prefix field of all sub-commands of the prefix command C.
   We must do this when a prefix command is defined as the GDB init sequence
   does not guarantee that a prefix command is created before its sub-commands.
   For example, break-catch-sig.c initialization runs before breakpoint.c
   initialization, but it is breakpoint.c that creates the "catch" command used
   by the "catch signal" command created by break-catch-sig.c.  */

static void
update_prefix_field_of_prefixed_commands (struct cmd_list_element *c)
{
  for (cmd_list_element *p = *c->subcommands; p != NULL; p = p->next)
    {
      p->prefix = c;

      /* We must recursively update the prefix field to cover
	 e.g.  'info auto-load libthread-db' where the creation
	 order was:
	   libthread-db
	   auto-load
	   info
	 In such a case, when 'auto-load' was created by do_add_cmd,
	 the 'libthread-db' prefix field could not be updated, as the
	 'auto-load' command was not yet reachable by
	    lookup_cmd_for_subcommands (list, cmdlist)
	    that searches from the top level 'cmdlist'.  */
      if (p->is_prefix ())
	update_prefix_field_of_prefixed_commands (p);
    }
}


/* Like add_cmd but adds an element for a command prefix: a name that
   should be followed by a subcommand to be looked up in another
   command list.  SUBCOMMANDS should be the address of the variable
   containing that list.  */

struct cmd_list_element *
add_prefix_cmd (const char *name, enum command_class theclass,
		cmd_simple_func_ftype *fun,
		const char *doc, struct cmd_list_element **subcommands,
		int allow_unknown, struct cmd_list_element **list)
{
  struct cmd_list_element *c = add_cmd (name, theclass, fun, doc, list);

  c->subcommands = subcommands;
  c->allow_unknown = allow_unknown;

  /* Now that prefix command C is defined, we need to set the prefix field
     of all prefixed commands that were defined before C itself was defined.  */
  update_prefix_field_of_prefixed_commands (c);

  return c;
}

/* A helper function for add_basic_prefix_cmd.  This is a command
   function that just forwards to help_list.  */

static void
do_prefix_cmd (const char *args, int from_tty, struct cmd_list_element *c)
{
  /* Look past all aliases.  */
  while (c->is_alias ())
    c = c->alias_target;

  help_list (*c->subcommands, c->prefixname ().c_str (),
	     all_commands, gdb_stdout);
}

/* See command.h.  */

struct cmd_list_element *
add_basic_prefix_cmd (const char *name, enum command_class theclass,
		      const char *doc, struct cmd_list_element **subcommands,
		      int allow_unknown, struct cmd_list_element **list)
{
  struct cmd_list_element *cmd = add_prefix_cmd (name, theclass, nullptr,
						 doc, subcommands,
						 allow_unknown, list);
  cmd->func = do_prefix_cmd;
  return cmd;
}

/* A helper function for add_show_prefix_cmd.  This is a command
   function that just forwards to cmd_show_list.  */

static void
do_show_prefix_cmd (const char *args, int from_tty, struct cmd_list_element *c)
{
  cmd_show_list (*c->subcommands, from_tty);
}

/* See command.h.  */

struct cmd_list_element *
add_show_prefix_cmd (const char *name, enum command_class theclass,
		     const char *doc, struct cmd_list_element **subcommands,
		     int allow_unknown, struct cmd_list_element **list)
{
  struct cmd_list_element *cmd = add_prefix_cmd (name, theclass, nullptr,
						 doc, subcommands,
						 allow_unknown, list);
  cmd->func = do_show_prefix_cmd;
  return cmd;
}

/* See command.h.  */

set_show_commands
add_setshow_prefix_cmd (const char *name, command_class theclass,
			const char *set_doc, const char *show_doc,
			cmd_list_element **set_subcommands_list,
			cmd_list_element **show_subcommands_list,
			cmd_list_element **set_list,
			cmd_list_element **show_list)
{
  set_show_commands cmds;

  cmds.set = add_basic_prefix_cmd (name, theclass, set_doc,
				   set_subcommands_list, 0,
				   set_list);
  cmds.show = add_show_prefix_cmd (name, theclass, show_doc,
				   show_subcommands_list, 0,
				   show_list);

  return cmds;
}

/* Like ADD_PREFIX_CMD but sets the suppress_notification pointer on the
   new command list element.  */

struct cmd_list_element *
add_prefix_cmd_suppress_notification
	       (const char *name, enum command_class theclass,
		cmd_simple_func_ftype *fun,
		const char *doc, struct cmd_list_element **subcommands,
		int allow_unknown, struct cmd_list_element **list,
		bool *suppress_notification)
{
  struct cmd_list_element *element
    = add_prefix_cmd (name, theclass, fun, doc, subcommands,
		      allow_unknown, list);
  element->suppress_notification = suppress_notification;
  return element;
}

/* Like add_prefix_cmd but sets the abbrev_flag on the new command.  */

struct cmd_list_element *
add_abbrev_prefix_cmd (const char *name, enum command_class theclass,
		       cmd_simple_func_ftype *fun, const char *doc,
		       struct cmd_list_element **subcommands,
		       int allow_unknown, struct cmd_list_element **list)
{
  struct cmd_list_element *c = add_cmd (name, theclass, fun, doc, list);

  c->subcommands = subcommands;
  c->allow_unknown = allow_unknown;
  c->abbrev_flag = 1;
  return c;
}

/* This is an empty "simple func".  */
void
not_just_help_class_command (const char *args, int from_tty)
{
}

/* This is an empty cmd func.  */

static void
empty_func (const char *args, int from_tty, cmd_list_element *c)
{
}

/* Add element named NAME to command list LIST (the list for set/show
   or some sublist thereof).
   TYPE is set_cmd or show_cmd.
   THECLASS is as in add_cmd.
   VAR_TYPE is the kind of thing we are setting.
   EXTRA_LITERALS if non-NULL define extra literals to be accepted in lieu of
   a number for integer variables.
   ARGS is a pre-validated type-erased reference to the variable being
   controlled by this command.
   DOC is the documentation string.  */

static struct cmd_list_element *
add_set_or_show_cmd (const char *name,
		     enum cmd_types type,
		     enum command_class theclass,
		     var_types var_type,
		     const literal_def *extra_literals,
		     const setting::erased_args &arg,
		     const char *doc,
		     struct cmd_list_element **list)
{
  struct cmd_list_element *c = add_cmd (name, theclass, doc, list);

  gdb_assert (type == set_cmd || type == show_cmd);
  c->type = type;
  c->var.emplace (var_type, extra_literals, arg);

  /* This needs to be something besides NULL so that this isn't
     treated as a help class.  */
  c->func = empty_func;
  return c;
}

/* Add element named NAME to both command lists SET_LIST and SHOW_LIST.
   THECLASS is as in add_cmd.  VAR_TYPE is the kind of thing we are
   setting.  EXTRA_LITERALS if non-NULL define extra literals to be
   accepted in lieu of a number for integer variables.  ARGS is a
   pre-validated type-erased reference to the variable being controlled
   by this command.  SET_FUNC and SHOW_FUNC are the callback functions
   (if non-NULL).  SET_DOC, SHOW_DOC and HELP_DOC are the documentation
   strings.

   Return the newly created set and show commands.  */

static set_show_commands
add_setshow_cmd_full_erased (const char *name,
			     enum command_class theclass,
			     var_types var_type,
			     const literal_def *extra_literals,
			     const setting::erased_args &args,
			     const char *set_doc, const char *show_doc,
			     const char *help_doc,
			     cmd_func_ftype *set_func,
			     show_value_ftype *show_func,
			     struct cmd_list_element **set_list,
			     struct cmd_list_element **show_list)
{
  struct cmd_list_element *set;
  struct cmd_list_element *show;
  gdb::unique_xmalloc_ptr<char> full_set_doc;
  gdb::unique_xmalloc_ptr<char> full_show_doc;

  if (help_doc != NULL)
    {
      full_set_doc = xstrprintf ("%s\n%s", set_doc, help_doc);
      full_show_doc = xstrprintf ("%s\n%s", show_doc, help_doc);
    }
  else
    {
      full_set_doc = make_unique_xstrdup (set_doc);
      full_show_doc = make_unique_xstrdup (show_doc);
    }
  set = add_set_or_show_cmd (name, set_cmd, theclass, var_type,
			     extra_literals, args,
			     full_set_doc.release (), set_list);
  set->doc_allocated = 1;

  if (set_func != NULL)
    set->func = set_func;

  show = add_set_or_show_cmd (name, show_cmd, theclass, var_type,
			      extra_literals, args,
			      full_show_doc.release (), show_list);
  show->doc_allocated = 1;
  show->show_value_func = show_func;
  /* Disable the default symbol completer.  Doesn't make much sense
     for the "show" command to complete on anything.  */
  set_cmd_completer (show, nullptr);

  return {set, show};
}

/* Completes on integer commands that support extra literals.  */

static void
integer_literals_completer (struct cmd_list_element *c,
			    completion_tracker &tracker,
			    const char *text, const char *word)
{
  const literal_def *extra_literals = c->var->extra_literals ();

  if (*text == '\0')
    {
      tracker.add_completion (make_unique_xstrdup ("NUMBER"));
      for (const literal_def *l = extra_literals;
	   l->literal != nullptr;
	   l++)
	tracker.add_completion (make_unique_xstrdup (l->literal));
    }
  else
    for (const literal_def *l = extra_literals;
	 l->literal != nullptr;
	 l++)
      if (startswith (l->literal, text))
	tracker.add_completion (make_unique_xstrdup (l->literal));
}

/* Add element named NAME to both command lists SET_LIST and SHOW_LIST.
   THECLASS is as in add_cmd.  VAR_TYPE is the kind of thing we are
   setting.  VAR is address of the variable being controlled by this
   command.  EXTRA_LITERALS if non-NULL define extra literals to be
   accepted in lieu of a number for integer variables.  If nullptr is
   given as VAR, then both SET_SETTING_FUNC and GET_SETTING_FUNC must
   be provided.  SET_SETTING_FUNC and GET_SETTING_FUNC are callbacks
   used to access and modify the underlying property, whatever its
   storage is.  SET_FUNC and SHOW_FUNC are the callback functions
   (if non-NULL).  SET_DOC, SHOW_DOC and HELP_DOC are the
   documentation strings.

   Return the newly created set and show commands.  */

template<typename T>
static set_show_commands
add_setshow_cmd_full (const char *name,
		      enum command_class theclass,
		      var_types var_type, T *var,
		      const literal_def *extra_literals,
		      const char *set_doc, const char *show_doc,
		      const char *help_doc,
		      typename setting_func_types<T>::set set_setting_func,
		      typename setting_func_types<T>::get get_setting_func,
		      cmd_func_ftype *set_func,
		      show_value_ftype *show_func,
		      struct cmd_list_element **set_list,
		      struct cmd_list_element **show_list)
{
  auto erased_args
    = setting::erase_args (var_type, var,
			   set_setting_func, get_setting_func);
  auto cmds = add_setshow_cmd_full_erased (name,
					   theclass,
					   var_type, extra_literals,
					   erased_args,
					   set_doc, show_doc,
					   help_doc,
					   set_func,
					   show_func,
					   set_list,
					   show_list);

  if (extra_literals != nullptr)
    set_cmd_completer (cmds.set, integer_literals_completer);

  return cmds;
}

/* Same as above but omitting EXTRA_LITERALS.  */

template<typename T>
static set_show_commands
add_setshow_cmd_full (const char *name,
		      enum command_class theclass,
		      var_types var_type, T *var,
		      const char *set_doc, const char *show_doc,
		      const char *help_doc,
		      typename setting_func_types<T>::set set_setting_func,
		      typename setting_func_types<T>::get get_setting_func,
		      cmd_func_ftype *set_func,
		      show_value_ftype *show_func,
		      struct cmd_list_element **set_list,
		      struct cmd_list_element **show_list)
{
  return add_setshow_cmd_full (name, theclass, var_type, var, nullptr,
			       set_doc, show_doc, help_doc,
			       set_setting_func, get_setting_func,
			       set_func, show_func, set_list, show_list);
}

/* Add element named NAME to command list LIST (the list for set or
   some sublist thereof).  THECLASS is as in add_cmd.  ENUMLIST is a list
   of strings which may follow NAME.  VAR is address of the variable
   which will contain the matching string (from ENUMLIST).  */

set_show_commands
add_setshow_enum_cmd (const char *name,
		      enum command_class theclass,
		      const char *const *enumlist,
		      const char **var,
		      const char *set_doc,
		      const char *show_doc,
		      const char *help_doc,
		      cmd_func_ftype *set_func,
		      show_value_ftype *show_func,
		      struct cmd_list_element **set_list,
		      struct cmd_list_element **show_list)
{
  /* We require *VAR to be initialized before this call, and
     furthermore it must be == to one of the values in ENUMLIST.  */
  gdb_assert (var != nullptr && *var != nullptr);
  for (int i = 0; ; ++i)
    {
      gdb_assert (enumlist[i] != nullptr);
      if (*var == enumlist[i])
	break;
    }

  set_show_commands commands
    =  add_setshow_cmd_full<const char *> (name, theclass, var_enum, var,
					   set_doc, show_doc, help_doc,
					   nullptr, nullptr, set_func,
					   show_func, set_list, show_list);
  commands.set->enums = enumlist;
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_enum_cmd (const char *name, command_class theclass,
		      const char *const *enumlist, const char *set_doc,
		      const char *show_doc, const char *help_doc,
		      setting_func_types<const char *>::set set_func,
		      setting_func_types<const char *>::get get_func,
		      show_value_ftype *show_func,
		      cmd_list_element **set_list,
		      cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<const char *> (name, theclass, var_enum,
						  nullptr, set_doc, show_doc,
						  help_doc, set_func, get_func,
						  nullptr, show_func, set_list,
						  show_list);

  cmds.set->enums = enumlist;

  return cmds;
}

/* See cli-decode.h.  */
const char * const auto_boolean_enums[] = { "on", "off", "auto", NULL };

/* Add an auto-boolean command named NAME to both the set and show
   command list lists.  THECLASS is as in add_cmd.  VAR is address of the
   variable which will contain the value.  DOC is the documentation
   string.  FUNC is the corresponding callback.  */

set_show_commands
add_setshow_auto_boolean_cmd (const char *name,
			      enum command_class theclass,
			      enum auto_boolean *var,
			      const char *set_doc, const char *show_doc,
			      const char *help_doc,
			      cmd_func_ftype *set_func,
			      show_value_ftype *show_func,
			      struct cmd_list_element **set_list,
			      struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<enum auto_boolean> (name, theclass, var_auto_boolean,
					       var, set_doc, show_doc, help_doc,
					       nullptr, nullptr, set_func,
					       show_func, set_list, show_list);

  commands.set->enums = auto_boolean_enums;

  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_auto_boolean_cmd (const char *name, command_class theclass,
			      const char *set_doc, const char *show_doc,
			      const char *help_doc,
			      setting_func_types<enum auto_boolean>::set set_func,
			      setting_func_types<enum auto_boolean>::get get_func,
			      show_value_ftype *show_func,
			      cmd_list_element **set_list,
			      cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<enum auto_boolean> (name, theclass,
						       var_auto_boolean,
						       nullptr, set_doc,
						       show_doc, help_doc,
						       set_func, get_func,
						       nullptr, show_func,
						       set_list, show_list);

  cmds.set->enums = auto_boolean_enums;

  return cmds;
}

/* See cli-decode.h.  */
const char * const boolean_enums[] = { "on", "off", NULL };

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  THECLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.
   Returns the new command element.  */

set_show_commands
add_setshow_boolean_cmd (const char *name, enum command_class theclass, bool *var,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 cmd_func_ftype *set_func,
			 show_value_ftype *show_func,
			 struct cmd_list_element **set_list,
			 struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<bool> (name, theclass, var_boolean, var,
				  set_doc, show_doc, help_doc,
				  nullptr, nullptr, set_func, show_func,
				  set_list, show_list);

  commands.set->enums = boolean_enums;

  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_boolean_cmd (const char *name, command_class theclass,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 setting_func_types<bool>::set set_func,
			 setting_func_types<bool>::get get_func,
			 show_value_ftype *show_func,
			 cmd_list_element **set_list,
			 cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<bool> (name, theclass, var_boolean, nullptr,
					  set_doc, show_doc, help_doc,
					  set_func, get_func, nullptr,
					  show_func, set_list, show_list);

  cmds.set->enums = boolean_enums;

  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */

set_show_commands
add_setshow_filename_cmd (const char *name, enum command_class theclass,
			  std::string *var,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_func_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<std::string> (name, theclass, var_filename, var,
					 set_doc, show_doc, help_doc,
					 nullptr, nullptr, set_func,
					 show_func, set_list, show_list);

  set_cmd_completer (commands.set, filename_completer);

  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_filename_cmd (const char *name, command_class theclass,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  setting_func_types<std::string>::set set_func,
			  setting_func_types<std::string>::get get_func,
			  show_value_ftype *show_func,
			  cmd_list_element **set_list,
			  cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<std::string> (name, theclass, var_filename,
						 nullptr, set_doc, show_doc,
						 help_doc, set_func, get_func,
						 nullptr, show_func, set_list,
						 show_list);

  set_cmd_completer (cmds.set, filename_completer);

  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */

set_show_commands
add_setshow_string_cmd (const char *name, enum command_class theclass,
			std::string *var,
			const char *set_doc, const char *show_doc,
			const char *help_doc,
			cmd_func_ftype *set_func,
			show_value_ftype *show_func,
			struct cmd_list_element **set_list,
			struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<std::string> (name, theclass, var_string, var,
					set_doc, show_doc, help_doc,
					nullptr, nullptr, set_func,
					show_func, set_list, show_list);

  /* Disable the default symbol completer.  */
  set_cmd_completer (commands.set, nullptr);

  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_string_cmd (const char *name, command_class theclass,
			const char *set_doc, const char *show_doc,
			const char *help_doc,
			setting_func_types<std::string>::set set_func,
			setting_func_types<std::string>::get get_func,
			show_value_ftype *show_func,
			cmd_list_element **set_list,
			cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<std::string> (name, theclass, var_string,
						 nullptr, set_doc, show_doc,
						 help_doc, set_func, get_func,
						 nullptr, show_func, set_list,
						 show_list);

  /* Disable the default symbol completer.  */
  set_cmd_completer (cmds.set, nullptr);

  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */

set_show_commands
add_setshow_string_noescape_cmd (const char *name, enum command_class theclass,
				 std::string *var,
				 const char *set_doc, const char *show_doc,
				 const char *help_doc,
				 cmd_func_ftype *set_func,
				 show_value_ftype *show_func,
				 struct cmd_list_element **set_list,
				 struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<std::string> (name, theclass, var_string_noescape,
					 var, set_doc, show_doc, help_doc,
					 nullptr, nullptr, set_func, show_func,
					 set_list, show_list);

  /* Disable the default symbol completer.  */
  set_cmd_completer (commands.set, nullptr);

  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_string_noescape_cmd (const char *name, command_class theclass,
				 const char *set_doc, const char *show_doc,
				 const char *help_doc,
				 setting_func_types<std::string>::set set_func,
				 setting_func_types<std::string>::get get_func,
				 show_value_ftype *show_func,
				 cmd_list_element **set_list,
				 cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<std::string> (name, theclass,
						 var_string_noescape, nullptr,
						 set_doc, show_doc, help_doc,
						 set_func, get_func,
						 nullptr, show_func, set_list,
						 show_list);

  /* Disable the default symbol completer.  */
  set_cmd_completer (cmds.set, nullptr);

  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  */

set_show_commands
add_setshow_optional_filename_cmd (const char *name, enum command_class theclass,
				   std::string *var,
				   const char *set_doc, const char *show_doc,
				   const char *help_doc,
				   cmd_func_ftype *set_func,
				   show_value_ftype *show_func,
				   struct cmd_list_element **set_list,
				   struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<std::string> (name, theclass, var_optional_filename,
					 var, set_doc, show_doc, help_doc,
					 nullptr, nullptr, set_func, show_func,
					 set_list, show_list);

  set_cmd_completer (commands.set, filename_completer);

  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_optional_filename_cmd (const char *name, command_class theclass,
				   const char *set_doc, const char *show_doc,
				   const char *help_doc,
				   setting_func_types<std::string>::set set_func,
				   setting_func_types<std::string>::get get_func,
				   show_value_ftype *show_func,
				   cmd_list_element **set_list,
				   cmd_list_element **show_list)
{
  auto cmds =
    add_setshow_cmd_full<std::string> (name, theclass, var_optional_filename,
				       nullptr, set_doc, show_doc, help_doc,
				       set_func, get_func, nullptr, show_func,
				       set_list,show_list);

  set_cmd_completer (cmds.set, filename_completer);

  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  THECLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  This
   function is only used in Python API.  Please don't use it elsewhere.  */

set_show_commands
add_setshow_integer_cmd (const char *name, enum command_class theclass,
			 int *var, const literal_def *extra_literals,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 cmd_func_ftype *set_func,
			 show_value_ftype *show_func,
			 struct cmd_list_element **set_list,
			 struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<int> (name, theclass, var_integer, var,
				 extra_literals, set_doc, show_doc,
				 help_doc, nullptr, nullptr, set_func,
				 show_func, set_list, show_list);
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_integer_cmd (const char *name, command_class theclass,
			 const literal_def *extra_literals,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 setting_func_types<int>::set set_func,
			 setting_func_types<int>::get get_func,
			 show_value_ftype *show_func,
			 cmd_list_element **set_list,
			 cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<int> (name, theclass, var_integer, nullptr,
					 extra_literals, set_doc, show_doc,
					 help_doc, set_func, get_func, nullptr,
					 show_func, set_list, show_list);
  return cmds;
}

/* Accept `unlimited' or 0, translated internally to INT_MAX.  */
const literal_def integer_unlimited_literals[] =
  {
    { "unlimited", INT_MAX, 0 },
    { nullptr }
  };

/* Same as above but using `integer_unlimited_literals', with a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_integer_cmd (const char *name, enum command_class theclass,
			 int *var,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 cmd_func_ftype *set_func,
			 show_value_ftype *show_func,
			 struct cmd_list_element **set_list,
			 struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<int> (name, theclass, var_integer, var,
				 integer_unlimited_literals,
				 set_doc, show_doc, help_doc,
				 nullptr, nullptr, set_func,
				 show_func, set_list, show_list);
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_integer_cmd (const char *name, command_class theclass,
			 const char *set_doc, const char *show_doc,
			 const char *help_doc,
			 setting_func_types<int>::set set_func,
			 setting_func_types<int>::get get_func,
			 show_value_ftype *show_func,
			 cmd_list_element **set_list,
			 cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<int> (name, theclass, var_integer, nullptr,
					 integer_unlimited_literals,
					 set_doc, show_doc, help_doc, set_func,
					 get_func, nullptr, show_func, set_list,
					 show_list);
  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  CLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */

set_show_commands
add_setshow_pinteger_cmd (const char *name, enum command_class theclass,
			  int *var, const literal_def *extra_literals,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_func_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<int> (name, theclass, var_pinteger, var,
				 extra_literals, set_doc, show_doc,
				 help_doc, nullptr, nullptr, set_func,
				 show_func, set_list, show_list);
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_pinteger_cmd (const char *name, command_class theclass,
			  const literal_def *extra_literals,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  setting_func_types<int>::set set_func,
			  setting_func_types<int>::get get_func,
			  show_value_ftype *show_func,
			  cmd_list_element **set_list,
			  cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<int> (name, theclass, var_pinteger, nullptr,
					 extra_literals, set_doc, show_doc,
					 help_doc, set_func, get_func, nullptr,
					 show_func, set_list, show_list);
  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  THECLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */

set_show_commands
add_setshow_uinteger_cmd (const char *name, enum command_class theclass,
			  unsigned int *var, const literal_def *extra_literals,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_func_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<unsigned int> (name, theclass, var_uinteger, var,
					  extra_literals, set_doc, show_doc,
					  help_doc, nullptr, nullptr, set_func,
					  show_func, set_list, show_list);
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_uinteger_cmd (const char *name, command_class theclass,
			  const literal_def *extra_literals,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  setting_func_types<unsigned int>::set set_func,
			  setting_func_types<unsigned int>::get get_func,
			  show_value_ftype *show_func,
			  cmd_list_element **set_list,
			  cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<unsigned int> (name, theclass, var_uinteger,
						  nullptr, extra_literals,
						  set_doc, show_doc, help_doc,
						  set_func, get_func, nullptr,
						  show_func, set_list,
						  show_list);
  return cmds;
}

/* Accept `unlimited' or 0, translated internally to UINT_MAX.  */
const literal_def uinteger_unlimited_literals[] =
  {
    { "unlimited", UINT_MAX, 0 },
    { nullptr }
  };

/* Same as above but using `uinteger_unlimited_literals', with a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_uinteger_cmd (const char *name, enum command_class theclass,
			  unsigned int *var,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_func_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<unsigned int> (name, theclass, var_uinteger, var,
					  uinteger_unlimited_literals,
					  set_doc, show_doc, help_doc, nullptr,
					  nullptr, set_func, show_func,
					  set_list, show_list);
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_uinteger_cmd (const char *name, command_class theclass,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  setting_func_types<unsigned int>::set set_func,
			  setting_func_types<unsigned int>::get get_func,
			  show_value_ftype *show_func,
			  cmd_list_element **set_list,
			  cmd_list_element **show_list)
{
  auto cmds = add_setshow_cmd_full<unsigned int> (name, theclass, var_uinteger,
						  nullptr,
						  uinteger_unlimited_literals,
						  set_doc, show_doc, help_doc,
						  set_func, get_func, nullptr,
						  show_func, set_list,
						  show_list);
  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  THECLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */

set_show_commands
add_setshow_zinteger_cmd (const char *name, enum command_class theclass,
			  int *var,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  cmd_func_ftype *set_func,
			  show_value_ftype *show_func,
			  struct cmd_list_element **set_list,
			  struct cmd_list_element **show_list)
{
  return add_setshow_cmd_full<int> (name, theclass, var_integer, var,
				    set_doc, show_doc, help_doc,
				    nullptr, nullptr, set_func,
				    show_func, set_list, show_list);
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_zinteger_cmd (const char *name, command_class theclass,
			  const char *set_doc, const char *show_doc,
			  const char *help_doc,
			  setting_func_types<int>::set set_func,
			  setting_func_types<int>::get get_func,
			  show_value_ftype *show_func,
			  cmd_list_element **set_list,
			  cmd_list_element **show_list)
{
  return add_setshow_cmd_full<int> (name, theclass, var_integer, nullptr,
				    set_doc, show_doc, help_doc, set_func,
				    get_func, nullptr, show_func, set_list,
				    show_list);
}

/* Accept `unlimited' or -1, using -1 internally.  */
const literal_def pinteger_unlimited_literals[] =
  {
    { "unlimited", -1, -1 },
    { nullptr }
  };

/* Same as above but using `pinteger_unlimited_literals', with a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_zuinteger_unlimited_cmd (const char *name,
				     enum command_class theclass,
				     int *var,
				     const char *set_doc,
				     const char *show_doc,
				     const char *help_doc,
				     cmd_func_ftype *set_func,
				     show_value_ftype *show_func,
				     struct cmd_list_element **set_list,
				     struct cmd_list_element **show_list)
{
  set_show_commands commands
    = add_setshow_cmd_full<int> (name, theclass, var_pinteger, var,
				 pinteger_unlimited_literals,
				 set_doc, show_doc, help_doc, nullptr,
				 nullptr, set_func, show_func, set_list,
				 show_list);
  return commands;
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_zuinteger_unlimited_cmd (const char *name, command_class theclass,
				     const char *set_doc, const char *show_doc,
				     const char *help_doc,
				     setting_func_types<int>::set set_func,
				     setting_func_types<int>::get get_func,
				     show_value_ftype *show_func,
				     cmd_list_element **set_list,
				     cmd_list_element **show_list)
{
  auto cmds
    = add_setshow_cmd_full<int> (name, theclass, var_pinteger, nullptr,
				 pinteger_unlimited_literals,
				 set_doc, show_doc, help_doc, set_func,
				 get_func, nullptr, show_func, set_list,
				 show_list);
  return cmds;
}

/* Add element named NAME to both the set and show command LISTs (the
   list for set/show or some sublist thereof).  THECLASS is as in
   add_cmd.  VAR is address of the variable which will contain the
   value.  SET_DOC and SHOW_DOC are the documentation strings.  */

set_show_commands
add_setshow_zuinteger_cmd (const char *name, enum command_class theclass,
			   unsigned int *var,
			   const char *set_doc, const char *show_doc,
			   const char *help_doc,
			   cmd_func_ftype *set_func,
			   show_value_ftype *show_func,
			   struct cmd_list_element **set_list,
			   struct cmd_list_element **show_list)
{
  return add_setshow_cmd_full<unsigned int> (name, theclass, var_uinteger,
					     var, set_doc, show_doc, help_doc,
					     nullptr, nullptr, set_func,
					     show_func, set_list, show_list);
}

/* Same as above but using a getter and a setter function instead of a pointer
   to a global storage buffer.  */

set_show_commands
add_setshow_zuinteger_cmd (const char *name, command_class theclass,
			   const char *set_doc, const char *show_doc,
			   const char *help_doc,
			   setting_func_types<unsigned int>::set set_func,
			   setting_func_types<unsigned int>::get get_func,
			   show_value_ftype *show_func,
			   cmd_list_element **set_list,
			   cmd_list_element **show_list)
{
  return add_setshow_cmd_full<unsigned int> (name, theclass, var_uinteger,
					     nullptr, set_doc, show_doc,
					     help_doc, set_func, get_func,
					     nullptr, show_func, set_list,
					     show_list);
}

/* Remove the command named NAME from the command list.  Return the list
   commands which were aliased to the deleted command.  The various *HOOKs are
   set to the pre- and post-hook commands for the deleted command.  If the
   command does not have a hook, the corresponding out parameter is set to
   NULL.  */

static cmd_list_element::aliases_list_type
delete_cmd (const char *name, struct cmd_list_element **list,
	    struct cmd_list_element **prehook,
	    struct cmd_list_element **prehookee,
	    struct cmd_list_element **posthook,
	    struct cmd_list_element **posthookee)
{
  struct cmd_list_element *iter;
  struct cmd_list_element **previous_chain_ptr;
  cmd_list_element::aliases_list_type aliases;

  *prehook = NULL;
  *prehookee = NULL;
  *posthook = NULL;
  *posthookee = NULL;
  previous_chain_ptr = list;

  for (iter = *previous_chain_ptr; iter; iter = *previous_chain_ptr)
    {
      if (strcmp (iter->name, name) == 0)
	{
	  if (iter->destroyer)
	    iter->destroyer (iter, iter->context ());

	  if (iter->hookee_pre)
	    iter->hookee_pre->hook_pre = 0;
	  *prehook = iter->hook_pre;
	  *prehookee = iter->hookee_pre;
	  if (iter->hookee_post)
	    iter->hookee_post->hook_post = 0;
	  *posthook = iter->hook_post;
	  *posthookee = iter->hookee_post;

	  /* Update the link.  */
	  *previous_chain_ptr = iter->next;

	  aliases = std::move (iter->aliases);

	  /* If this command was an alias, remove it from the list of
	     aliases.  */
	  if (iter->is_alias ())
	    {
	      auto it = iter->alias_target->aliases.iterator_to (*iter);
	      iter->alias_target->aliases.erase (it);
	    }

	  delete iter;

	  /* We won't see another command with the same name.  */
	  break;
	}
      else
	previous_chain_ptr = &iter->next;
    }

  return aliases;
}

/* Shorthands to the commands above.  */

/* Add an element to the list of info subcommands.  */

struct cmd_list_element *
add_info (const char *name, cmd_simple_func_ftype *fun, const char *doc)
{
  return add_cmd (name, class_info, fun, doc, &infolist);
}

/* Add an alias to the list of info subcommands.  */

cmd_list_element *
add_info_alias (const char *name, cmd_list_element *target, int abbrev_flag)
{
  return add_alias_cmd (name, target, class_run, abbrev_flag, &infolist);
}

/* Add an element to the list of commands.  */

struct cmd_list_element *
add_com (const char *name, enum command_class theclass,
	 cmd_simple_func_ftype *fun,
	 const char *doc)
{
  return add_cmd (name, theclass, fun, doc, &cmdlist);
}

/* Add an alias or abbreviation command to the list of commands.
   For aliases predefined by GDB (such as bt), THECLASS must be
   different of class_alias, as class_alias is used to identify
   user defined aliases.  */

cmd_list_element *
add_com_alias (const char *name, cmd_list_element *target,
	       command_class theclass, int abbrev_flag)
{
  return add_alias_cmd (name, target, theclass, abbrev_flag, &cmdlist);
}

/* Add an element with a suppress notification to the list of commands.  */

struct cmd_list_element *
add_com_suppress_notification (const char *name, enum command_class theclass,
			       cmd_simple_func_ftype *fun, const char *doc,
			       bool *suppress_notification)
{
  return add_cmd_suppress_notification (name, theclass, fun, doc,
					&cmdlist, suppress_notification);
}

/* Print the prefix of C followed by name of C in title style.  */

static void
fput_command_name_styled (const cmd_list_element &c, struct ui_file *stream)
{
  std::string prefixname
    = c.prefix == nullptr ? "" : c.prefix->prefixname ();

  fprintf_styled (stream, title_style.style (), "%s%s",
		  prefixname.c_str (), c.name);
}

/* True if ALIAS has a user-defined documentation.  */

static bool
user_documented_alias (const cmd_list_element &alias)
{
  gdb_assert (alias.is_alias ());
  /* Alias is user documented if it has an allocated documentation
     that differs from the aliased command.  */
  return (alias.doc_allocated
	  && strcmp (alias.doc, alias.alias_target->doc) != 0);
}

/* Print the definition of alias C using title style for alias
   and aliased command.  */

static void
fput_alias_definition_styled (const cmd_list_element &c,
			      struct ui_file *stream)
{
  gdb_assert (c.is_alias ());
  gdb_puts ("  alias ", stream);
  fput_command_name_styled (c, stream);
  gdb_printf (stream, " = ");
  fput_command_name_styled (*c.alias_target, stream);
  gdb_printf (stream, " %s\n", c.default_args.c_str ());
}

/* Print the definition of CMD aliases not deprecated and having default args
   and not specifically documented by the user.  */

static void
fput_aliases_definition_styled (const cmd_list_element &cmd,
				struct ui_file *stream)
{
  for (const cmd_list_element &alias : cmd.aliases)
    if (!alias.cmd_deprecated
	&& !user_documented_alias (alias)
	&& !alias.default_args.empty ())
      fput_alias_definition_styled (alias, stream);
}

/* If C has one or more aliases, style print the name of C and the name of its
   aliases not documented specifically by the user, separated by commas.
   If ALWAYS_FPUT_C_NAME, print the name of C even if it has no aliases.
   If one or more names are printed, POSTFIX is printed after the last name.
*/

static void
fput_command_names_styled (const cmd_list_element &c,
			   bool always_fput_c_name, const char *postfix,
			   struct ui_file *stream)
{
  /* First, check if we are going to print something.  That is, either if
     ALWAYS_FPUT_C_NAME is true or if there exists at least one non-deprecated
     alias not documented specifically by the user.  */

  auto print_alias = [] (const cmd_list_element &alias)
    {
      return !alias.cmd_deprecated && !user_documented_alias (alias);
    };

  bool print_something = always_fput_c_name;
  if (!print_something)
    for (const cmd_list_element &alias : c.aliases)
      {
	if (!print_alias (alias))
	  continue;

	print_something = true;
	break;
      }

  if (print_something)
    fput_command_name_styled (c, stream);

  for (const cmd_list_element &alias : c.aliases)
    {
      if (!print_alias (alias))
	continue;

      gdb_puts (", ", stream);
      stream->wrap_here (3);
      fput_command_name_styled (alias, stream);
    }

  if (print_something)
    gdb_puts (postfix, stream);
}

/* If VERBOSE, print the full help for command C and highlight the
   documentation parts matching HIGHLIGHT,
   otherwise print only one-line help for command C.  */

static void
print_doc_of_command (const cmd_list_element &c, bool verbose,
		      compiled_regex &highlight, struct ui_file *stream)
{
  /* When printing the full documentation, add a line to separate
     this documentation from the previous command help, in the likely
     case that apropos finds several commands.  */
  if (verbose)
    gdb_puts ("\n", stream);

  fput_command_names_styled (c, true,
			     verbose ? "" : " -- ", stream);
  if (verbose)
    {
      gdb_puts ("\n", stream);
      fput_aliases_definition_styled (c, stream);
      fputs_highlighted (c.doc, highlight, stream);
      gdb_puts ("\n", stream);
    }
  else
    {
      print_doc_line (stream, c.doc, false);
      gdb_puts ("\n", stream);
      fput_aliases_definition_styled (c, stream);
    }
}

/* Recursively walk the commandlist structures, and print out the
   documentation of commands that match our regex in either their
   name, or their documentation.
   If VERBOSE, prints the complete documentation and highlight the
   documentation parts matching REGEX, otherwise prints only
   the first line.
*/
void
apropos_cmd (struct ui_file *stream,
	     struct cmd_list_element *commandlist,
	     bool verbose, compiled_regex &regex)
{
  struct cmd_list_element *c;
  int returnvalue;

  /* Walk through the commands.  */
  for (c=commandlist;c;c=c->next)
    {
      if (c->is_alias () && !user_documented_alias (*c))
	{
	  /* Command aliases/abbreviations not specifically documented by the
	     user are skipped to ensure we print the doc of a command only once,
	     when encountering the aliased command.  */
	  continue;
	}

      returnvalue = -1; /* Needed to avoid double printing.  */
      if (c->name != NULL)
	{
	  size_t name_len = strlen (c->name);

	  /* Try to match against the name.  */
	  returnvalue = regex.search (c->name, name_len, 0, name_len, NULL);
	  if (returnvalue >= 0)
	    print_doc_of_command (*c, verbose, regex, stream);

	  /* Try to match against the name of the aliases.  */
	  for (const cmd_list_element &alias : c->aliases)
	    {
	      name_len = strlen (alias.name);
	      returnvalue = regex.search (alias.name, name_len, 0, name_len, NULL);
	      if (returnvalue >= 0)
		{
		  print_doc_of_command (*c, verbose, regex, stream);
		  break;
		}
	    }
	}
      if (c->doc != NULL && returnvalue < 0)
	{
	  size_t doc_len = strlen (c->doc);

	  /* Try to match against documentation.  */
	  if (regex.search (c->doc, doc_len, 0, doc_len, NULL) >= 0)
	    print_doc_of_command (*c, verbose, regex, stream);
	}
      /* Check if this command has subcommands.  */
      if (c->is_prefix ())
	{
	  /* Recursively call ourselves on the subcommand list,
	     passing the right prefix in.  */
	  apropos_cmd (stream, *c->subcommands, verbose, regex);
	}
    }
}

/* This command really has to deal with two things:
   1) I want documentation on *this string* (usually called by
      "help commandname").

   2) I want documentation on *this list* (usually called by giving a
      command that requires subcommands.  Also called by saying just
      "help".)

   I am going to split this into two separate commands, help_cmd and
   help_list.  */

void
help_cmd (const char *command, struct ui_file *stream)
{
  struct cmd_list_element *c, *alias, *prefix_cmd, *c_cmd;

  if (!command)
    {
      help_list (cmdlist, "", all_classes, stream);
      return;
    }

  if (strcmp (command, "all") == 0)
    {
      help_all (stream);
      return;
    }

  const char *orig_command = command;
  c = lookup_cmd (&command, cmdlist, "", NULL, 0, 0);

  if (c == 0)
    return;

  lookup_cmd_composition (orig_command, &alias, &prefix_cmd, &c_cmd);

  /* There are three cases here.
     If c->subcommands is nonzero, we have a prefix command.
     Print its documentation, then list its subcommands.

     If c->func is non NULL, we really have a command.  Print its
     documentation and return.

     If c->func is NULL, we have a class name.  Print its
     documentation (as if it were a command) and then set class to the
     number of this class so that the commands in the class will be
     listed.  */

  if (alias == nullptr || !user_documented_alias (*alias))
    {
      /* Case of a normal command, or an alias not explicitly
	 documented by the user.  */
      /* If the user asked 'help somecommand' and there is no alias,
	 the false indicates to not output the (single) command name.  */
      fput_command_names_styled (*c, false, "\n", stream);
      fput_aliases_definition_styled (*c, stream);
      gdb_puts (c->doc, stream);
    }
  else
    {
      /* Case of an alias explicitly documented by the user.
	 Only output the alias definition and its explicit documentation.  */
      fput_alias_definition_styled (*alias, stream);
      fput_command_names_styled (*alias, false, "\n", stream);
      gdb_puts (alias->doc, stream);
    }
  gdb_puts ("\n", stream);

  if (!c->is_prefix () && !c->is_command_class_help ())
    return;

  gdb_printf (stream, "\n");

  /* If this is a prefix command, print it's subcommands.  */
  if (c->is_prefix ())
    help_list (*c->subcommands, c->prefixname ().c_str (),
	       all_commands, stream);

  /* If this is a class name, print all of the commands in the class.  */
  if (c->is_command_class_help ())
    help_list (cmdlist, "", c->theclass, stream);

  if (c->hook_pre || c->hook_post)
    gdb_printf (stream,
		"\nThis command has a hook (or hooks) defined:\n");

  if (c->hook_pre)
    gdb_printf (stream,
		"\tThis command is run after  : %s (pre hook)\n",
		c->hook_pre->name);
  if (c->hook_post)
    gdb_printf (stream,
		"\tThis command is run before : %s (post hook)\n",
		c->hook_post->name);
}

/*
 * Get a specific kind of help on a command list.
 *
 * LIST is the list.
 * CMDTYPE is the prefix to use in the title string.
 * THECLASS is the class with which to list the nodes of this list (see
 * documentation for help_cmd_list below),  As usual, ALL_COMMANDS for
 * everything, ALL_CLASSES for just classes, and non-negative for only things
 * in a specific class.
 * and STREAM is the output stream on which to print things.
 * If you call this routine with a class >= 0, it recurses.
 */
void
help_list (struct cmd_list_element *list, const char *cmdtype,
	   enum command_class theclass, struct ui_file *stream)
{
  int len;
  char *cmdtype1, *cmdtype2;

  /* If CMDTYPE is "foo ", CMDTYPE1 gets " foo" and CMDTYPE2 gets "foo sub".
   */
  len = strlen (cmdtype);
  cmdtype1 = (char *) alloca (len + 1);
  cmdtype1[0] = 0;
  cmdtype2 = (char *) alloca (len + 4);
  cmdtype2[0] = 0;
  if (len)
    {
      cmdtype1[0] = ' ';
      memcpy (cmdtype1 + 1, cmdtype, len - 1);
      cmdtype1[len] = 0;
      memcpy (cmdtype2, cmdtype, len - 1);
      strcpy (cmdtype2 + len - 1, " sub");
    }

  if (theclass == all_classes)
    gdb_printf (stream, "List of classes of %scommands:\n\n", cmdtype2);
  else
    gdb_printf (stream, "List of %scommands:\n\n", cmdtype2);

  help_cmd_list (list, theclass, theclass >= 0, stream);

  if (theclass == all_classes)
    {
      gdb_printf (stream, "\n\
Type \"help%s\" followed by a class name for a list of commands in ",
		  cmdtype1);
      stream->wrap_here (0);
      gdb_printf (stream, "that class.");

      gdb_printf (stream, "\n\
Type \"help all\" for the list of all commands.");
    }

  gdb_printf (stream, "\nType \"help%s\" followed by %scommand name ",
	      cmdtype1, cmdtype2);
  stream->wrap_here (0);
  gdb_puts ("for ", stream);
  stream->wrap_here (0);
  gdb_puts ("full ", stream);
  stream->wrap_here (0);
  gdb_puts ("documentation.\n", stream);
  gdb_puts ("Type \"apropos word\" to search "
	    "for commands related to \"word\".\n", stream);
  gdb_puts ("Type \"apropos -v word\" for full documentation", stream);
  stream->wrap_here (0);
  gdb_puts (" of commands related to \"word\".\n", stream);
  gdb_puts ("Command name abbreviations are allowed if unambiguous.\n",
	    stream);
}

static void
help_all (struct ui_file *stream)
{
  struct cmd_list_element *c;
  int seen_unclassified = 0;

  for (c = cmdlist; c; c = c->next)
    {
      if (c->abbrev_flag)
	continue;
      /* If this is a class name, print all of the commands in the
	 class.  */

      if (c->is_command_class_help ())
	{
	  gdb_printf (stream, "\nCommand class: %s\n\n", c->name);
	  help_cmd_list (cmdlist, c->theclass, true, stream);
	}
    }

  /* While it's expected that all commands are in some class,
     as a safety measure, we'll print commands outside of any
     class at the end.  */

  for (c = cmdlist; c; c = c->next)
    {
      if (c->abbrev_flag)
	continue;

      if (c->theclass == no_class)
	{
	  if (!seen_unclassified)
	    {
	      gdb_printf (stream, "\nUnclassified commands\n\n");
	      seen_unclassified = 1;
	    }
	  print_help_for_command (*c, true, stream);
	}
    }

}

/* See cli-decode.h.  */

void
print_doc_line (struct ui_file *stream, const char *str,
		bool for_value_prefix)
{
  static char *line_buffer = 0;
  static int line_size;
  const char *p;

  if (!line_buffer)
    {
      line_size = 80;
      line_buffer = (char *) xmalloc (line_size);
    }

  /* Searches for the first end of line or the end of STR.  */
  p = str;
  while (*p && *p != '\n')
    p++;
  if (p - str > line_size - 1)
    {
      line_size = p - str + 1;
      xfree (line_buffer);
      line_buffer = (char *) xmalloc (line_size);
    }
  strncpy (line_buffer, str, p - str);
  if (for_value_prefix)
    {
      if (islower (line_buffer[0]))
	line_buffer[0] = toupper (line_buffer[0]);
      gdb_assert (p > str);
      if (line_buffer[p - str - 1] == '.')
	line_buffer[p - str - 1] = '\0';
      else
	line_buffer[p - str] = '\0';
    }
  else
    line_buffer[p - str] = '\0';
  gdb_puts (line_buffer, stream);
}

/* Print one-line help for command C.
   If RECURSE is non-zero, also print one-line descriptions
   of all prefixed subcommands.  */
static void
print_help_for_command (const cmd_list_element &c,
			bool recurse, struct ui_file *stream)
{
  fput_command_names_styled (c, true, " -- ", stream);
  print_doc_line (stream, c.doc, false);
  gdb_puts ("\n", stream);
  if (!c.default_args.empty ())
    fput_alias_definition_styled (c, stream);
  fput_aliases_definition_styled (c, stream);

  if (recurse
      && c.is_prefix ()
      && c.abbrev_flag == 0)
    /* Subcommands of a prefix command typically have 'all_commands'
       as class.  If we pass CLASS to recursive invocation,
       most often we won't see anything.  */
    help_cmd_list (*c.subcommands, all_commands, true, stream);
}

/*
 * Implement a help command on command list LIST.
 * RECURSE should be non-zero if this should be done recursively on
 * all sublists of LIST.
 * STREAM is the stream upon which the output should be written.
 * THECLASS should be:
 *      A non-negative class number to list only commands in that
 *      ALL_COMMANDS to list all commands in list.
 *      ALL_CLASSES  to list all classes in list.
 *
 *   Note that aliases are only shown when THECLASS is class_alias.
 *   In the other cases, the aliases will be shown together with their
 *   aliased command.
 *
 *   Note that RECURSE will be active on *all* sublists, not just the
 * ones selected by the criteria above (ie. the selection mechanism
 * is at the low level, not the high-level).
 */

static void
help_cmd_list (struct cmd_list_element *list, enum command_class theclass,
	       bool recurse, struct ui_file *stream)
{
  struct cmd_list_element *c;

  for (c = list; c; c = c->next)
    {
      if (c->abbrev_flag == 1 || c->cmd_deprecated)
	{
	  /* Do not show abbreviations or deprecated commands.  */
	  continue;
	}

      if (c->is_alias () && theclass != class_alias)
	{
	  /* Do not show an alias, unless specifically showing the
	     list of aliases:  for all other classes, an alias is
	     shown (if needed) together with its aliased command.  */
	  continue;
	}

      if (theclass == all_commands
	  || (theclass == all_classes && c->is_command_class_help ())
	  || (theclass == c->theclass && !c->is_command_class_help ()))
	{
	  /* show C when
	     - showing all commands
	     - showing all classes and C is a help class
	     - showing commands of THECLASS and C is not the help class  */

	  /* If we show the class_alias and C is an alias, do not recurse,
	     as this would show the (possibly very long) not very useful
	     list of sub-commands of the aliased command.  */
	  print_help_for_command
	    (*c,
	     recurse && (theclass != class_alias || !c->is_alias ()),
	     stream);
	  continue;
	}

      if (recurse
	  && (theclass == class_user || theclass == class_alias)
	  && c->is_prefix ())
	{
	  /* User-defined commands or aliases may be subcommands.  */
	  help_cmd_list (*c->subcommands, theclass, recurse, stream);
	  continue;
	}

      /* Do not show C or recurse on C, e.g. because C does not belong to
	 THECLASS or because C is a help class.  */
    }
}


/* Search the input clist for 'command'.  Return the command if
   found (or NULL if not), and return the number of commands
   found in nfound.  */

static struct cmd_list_element *
find_cmd (const char *command, int len, struct cmd_list_element *clist,
	  int ignore_help_classes, int *nfound)
{
  struct cmd_list_element *found, *c;

  found = NULL;
  *nfound = 0;
  for (c = clist; c; c = c->next)
    if (!strncmp (command, c->name, len)
	&& (!ignore_help_classes || !c->is_command_class_help ()))
      {
	found = c;
	(*nfound)++;
	if (c->name[len] == '\0')
	  {
	    *nfound = 1;
	    break;
	  }
      }
  return found;
}

/* Return the length of command name in TEXT.  */

int
find_command_name_length (const char *text)
{
  const char *p = text;

  /* Treating underscores as part of command words is important
     so that "set args_foo()" doesn't get interpreted as
     "set args _foo()".  */
  /* Some characters are only used for TUI specific commands.
     However, they are always allowed for the sake of consistency.

     Note that this is larger than the character set allowed when
     creating user-defined commands.  */

  /* Recognize the single character commands so that, e.g., "!ls"
     works as expected.  */
  if (*p == '!' || *p == '|')
    return 1;

  while (valid_cmd_char_p (*p)
	 /* Characters used by TUI specific commands.  */
	 || *p == '+' || *p == '<' || *p == '>' || *p == '$')
    p++;

  return p - text;
}

/* See command.h.  */

bool
valid_cmd_char_p (int c)
{
  /* Alas "42" is a legitimate user-defined command.
     In the interests of not breaking anything we preserve that.  */

  return isalnum (c) || c == '-' || c == '_' || c == '.';
}

/* See command.h.  */

bool
valid_user_defined_cmd_name_p (const char *name)
{
  const char *p;

  if (*name == '\0')
    return false;

  for (p = name; *p != '\0'; ++p)
    {
      if (valid_cmd_char_p (*p))
	; /* Ok.  */
      else
	return false;
    }

  return true;
}

/* See command.h.  */

struct cmd_list_element *
lookup_cmd_1 (const char **text, struct cmd_list_element *clist,
	      struct cmd_list_element **result_list, std::string *default_args,
	      int ignore_help_classes, bool lookup_for_completion_p)
{
  char *command;
  int len, nfound;
  struct cmd_list_element *found, *c;
  bool found_alias = false;
  const char *line = *text;

  while (**text == ' ' || **text == '\t')
    (*text)++;

  /* Identify the name of the command.  */
  len = find_command_name_length (*text);

  /* If nothing but whitespace, return 0.  */
  if (len == 0)
    return 0;

  /* *text and p now bracket the first command word to lookup (and
     it's length is len).  We copy this into a local temporary.  */


  command = (char *) alloca (len + 1);
  memcpy (command, *text, len);
  command[len] = '\0';

  /* Look it up.  */
  found = 0;
  nfound = 0;
  found = find_cmd (command, len, clist, ignore_help_classes, &nfound);

  /* If nothing matches, we have a simple failure.  */
  if (nfound == 0)
    return 0;

  if (nfound > 1)
    {
      if (result_list != nullptr)
	/* Will be modified in calling routine
	   if we know what the prefix command is.  */
	*result_list = 0;
      if (default_args != nullptr)
	*default_args = std::string ();
      return CMD_LIST_AMBIGUOUS;	/* Ambiguous.  */
    }

  /* We've matched something on this list.  Move text pointer forward.  */

  *text += len;

  if (found->is_alias ())
    {
      /* We drop the alias (abbreviation) in favor of the command it
       is pointing to.  If the alias is deprecated, though, we need to
       warn the user about it before we drop it.  Note that while we
       are warning about the alias, we may also warn about the command
       itself and we will adjust the appropriate DEPRECATED_WARN_USER
       flags.  */

      if (found->deprecated_warn_user && !lookup_for_completion_p)
	deprecated_cmd_warning (line, clist);


      /* Return the default_args of the alias, not the default_args
	 of the command it is pointing to.  */
      if (default_args != nullptr)
	*default_args = found->default_args;
      found = found->alias_target;
      found_alias = true;
    }
  /* If we found a prefix command, keep looking.  */

  if (found->is_prefix ())
    {
      c = lookup_cmd_1 (text, *found->subcommands, result_list, default_args,
			ignore_help_classes, lookup_for_completion_p);
      if (!c)
	{
	  /* Didn't find anything; this is as far as we got.  */
	  if (result_list != nullptr)
	    *result_list = clist;
	  if (!found_alias && default_args != nullptr)
	    *default_args = found->default_args;
	  return found;
	}
      else if (c == CMD_LIST_AMBIGUOUS)
	{
	  /* We've gotten this far properly, but the next step is
	     ambiguous.  We need to set the result list to the best
	     we've found (if an inferior hasn't already set it).  */
	  if (result_list != nullptr)
	    if (!*result_list)
	      /* This used to say *result_list = *found->subcommands.
		 If that was correct, need to modify the documentation
		 at the top of this function to clarify what is
		 supposed to be going on.  */
	      *result_list = found;
	  /* For ambiguous commands, do not return any default_args args.  */
	  if (default_args != nullptr)
	    *default_args = std::string ();
	  return c;
	}
      else
	{
	  /* We matched!  */
	  return c;
	}
    }
  else
    {
      if (result_list != nullptr)
	*result_list = clist;
      if (!found_alias && default_args != nullptr)
	*default_args = found->default_args;
      return found;
    }
}

/* All this hair to move the space to the front of cmdtype */

static void
undef_cmd_error (const char *cmdtype, const char *q)
{
  error (_("Undefined %scommand: \"%s\".  Try \"help%s%.*s\"."),
	 cmdtype,
	 q,
	 *cmdtype ? " " : "",
	 (int) strlen (cmdtype) - 1,
	 cmdtype);
}

/* Look up the contents of *LINE as a command in the command list LIST.
   LIST is a chain of struct cmd_list_element's.
   If it is found, return the struct cmd_list_element for that command,
   update *LINE to point after the command name, at the first argument
   and update *DEFAULT_ARGS (if DEFAULT_ARGS is not null) to the default
   args to prepend to the user provided args when running the command.
   Note that if the found cmd_list_element is found via an alias,
   the default args of the alias are returned.

   If not found, call error if ALLOW_UNKNOWN is zero
   otherwise (or if error returns) return zero.
   Call error if specified command is ambiguous,
   unless ALLOW_UNKNOWN is negative.
   CMDTYPE precedes the word "command" in the error message.

   If IGNORE_HELP_CLASSES is nonzero, ignore any command list
   elements which are actually help classes rather than commands (i.e.
   the function field of the struct cmd_list_element is 0).  */

struct cmd_list_element *
lookup_cmd (const char **line, struct cmd_list_element *list,
	    const char *cmdtype,
	    std::string *default_args,
	    int allow_unknown, int ignore_help_classes)
{
  struct cmd_list_element *last_list = 0;
  struct cmd_list_element *c;

  /* Note: Do not remove trailing whitespace here because this
     would be wrong for complete_command.  Jim Kingdon  */

  if (!*line)
    error (_("Lack of needed %scommand"), cmdtype);

  c = lookup_cmd_1 (line, list, &last_list, default_args, ignore_help_classes);

  if (!c)
    {
      if (!allow_unknown)
	{
	  char *q;
	  int len = find_command_name_length (*line);

	  q = (char *) alloca (len + 1);
	  strncpy (q, *line, len);
	  q[len] = '\0';
	  undef_cmd_error (cmdtype, q);
	}
      else
	return 0;
    }
  else if (c == CMD_LIST_AMBIGUOUS)
    {
      /* Ambigous.  Local values should be off subcommands or called
	 values.  */
      int local_allow_unknown = (last_list ? last_list->allow_unknown :
				 allow_unknown);
      std::string local_cmdtype
	= last_list ? last_list->prefixname () : cmdtype;
      struct cmd_list_element *local_list =
	(last_list ? *(last_list->subcommands) : list);

      if (local_allow_unknown < 0)
	{
	  if (last_list)
	    return last_list;	/* Found something.  */
	  else
	    return 0;		/* Found nothing.  */
	}
      else
	{
	  /* Report as error.  */
	  int amb_len;
	  char ambbuf[100];

	  for (amb_len = 0;
	       ((*line)[amb_len] && (*line)[amb_len] != ' '
		&& (*line)[amb_len] != '\t');
	       amb_len++)
	    ;

	  ambbuf[0] = 0;
	  for (c = local_list; c; c = c->next)
	    if (!strncmp (*line, c->name, amb_len))
	      {
		if (strlen (ambbuf) + strlen (c->name) + 6
		    < (int) sizeof ambbuf)
		  {
		    if (strlen (ambbuf))
		      strcat (ambbuf, ", ");
		    strcat (ambbuf, c->name);
		  }
		else
		  {
		    strcat (ambbuf, "..");
		    break;
		  }
	      }
	  error (_("Ambiguous %scommand \"%s\": %s."),
		 local_cmdtype.c_str (), *line, ambbuf);
	}
    }
  else
    {
      if (c->type == set_cmd && **line != '\0' && !isspace (**line))
	error (_("Argument must be preceded by space."));

      /* We've got something.  It may still not be what the caller
	 wants (if this command *needs* a subcommand).  */
      while (**line == ' ' || **line == '\t')
	(*line)++;

      if (c->is_prefix () && **line && !c->allow_unknown)
	undef_cmd_error (c->prefixname ().c_str (), *line);

      /* Seems to be what he wants.  Return it.  */
      return c;
    }
  return 0;
}

/* See command.h.  */

struct cmd_list_element *
lookup_cmd_exact (const char *name,
		  struct cmd_list_element *list,
		  bool ignore_help_classes)
{
  const char *tem = name;
  struct cmd_list_element *cmd = lookup_cmd (&tem, list, "", NULL, -1,
					     ignore_help_classes);
  if (cmd != nullptr && strcmp (name, cmd->name) != 0)
    cmd = nullptr;
  return cmd;
}

/* We are here presumably because an alias or command in TEXT is
   deprecated and a warning message should be generated.  This
   function decodes TEXT and potentially generates a warning message
   as outlined below.
   
   Example for 'set endian big' which has a fictitious alias 'seb'.
   
   If alias wasn't used in TEXT, and the command is deprecated:
   "warning: 'set endian big' is deprecated." 
   
   If alias was used, and only the alias is deprecated:
   "warning: 'seb' an alias for the command 'set endian big' is deprecated."
   
   If alias was used and command is deprecated (regardless of whether
   the alias itself is deprecated:
   
   "warning: 'set endian big' (seb) is deprecated."

   After the message has been sent, clear the appropriate flags in the
   command and/or the alias so the user is no longer bothered.
   
*/
void
deprecated_cmd_warning (const char *text, struct cmd_list_element *list)
{
  struct cmd_list_element *alias = nullptr;
  struct cmd_list_element *cmd = nullptr;

  /* Return if text doesn't evaluate to a command.  We place this lookup
     within its own scope so that the PREFIX_CMD local is not visible
     later in this function.  The value returned in PREFIX_CMD is based on
     the prefix found in TEXT, and is our case this prefix can be missing
     in some situations (when LIST is not the global CMDLIST).

     It is better for our purposes to use the prefix commands directly from
     the ALIAS and CMD results.  */
  {
    struct cmd_list_element *prefix_cmd = nullptr;
    if (!lookup_cmd_composition_1 (text, &alias, &prefix_cmd, &cmd, list))
      return;
  }

  /* Return if nothing is deprecated.  */
  if (!((alias != nullptr ? alias->deprecated_warn_user : 0)
	|| cmd->deprecated_warn_user))
    return;

  /* Join command prefix (if any) and the command name.  */
  std::string tmp_cmd_str;
  if (cmd->prefix != nullptr)
    tmp_cmd_str += cmd->prefix->prefixname ();
  tmp_cmd_str += std::string (cmd->name);

  /* Display the appropriate first line, this warns that the thing the user
     entered is deprecated.  */
  if (alias != nullptr)
    {
      /* Join the alias prefix (if any) and the alias name.  */
      std::string tmp_alias_str;
      if (alias->prefix != nullptr)
	tmp_alias_str += alias->prefix->prefixname ();
      tmp_alias_str += std::string (alias->name);

      if (cmd->cmd_deprecated)
	gdb_printf (_("Warning: command '%ps' (%ps) is deprecated.\n"),
		    styled_string (title_style.style (),
				   tmp_cmd_str.c_str ()),
		    styled_string (title_style.style (),
				   tmp_alias_str.c_str ()));
      else
	gdb_printf (_("Warning: '%ps', an alias for the command '%ps', "
		      "is deprecated.\n"),
		    styled_string (title_style.style (),
				   tmp_alias_str.c_str ()),
		    styled_string (title_style.style (),
				   tmp_cmd_str.c_str ()));
    }
  else
    gdb_printf (_("Warning: command '%ps' is deprecated.\n"),
		styled_string (title_style.style (),
			       tmp_cmd_str.c_str ()));

  /* Now display a second line indicating what the user should use instead.
     If it is only the alias that is deprecated, we want to indicate the
     new alias, otherwise we'll indicate the new command.  */
  const char *replacement;
  if (alias != nullptr && !cmd->cmd_deprecated)
    replacement = alias->replacement;
  else
    replacement = cmd->replacement;
  if (replacement != nullptr)
    gdb_printf (_("Use '%ps'.\n\n"),
		styled_string (title_style.style (),
			       replacement));
  else
    gdb_printf (_("No alternative known.\n\n"));

  /* We've warned you, now we'll keep quiet.  */
  if (alias != nullptr)
    alias->deprecated_warn_user = 0;
  cmd->deprecated_warn_user = 0;
}

/* Look up the contents of TEXT as a command in the command list CUR_LIST.
   Return 1 on success, 0 on failure.

   If TEXT refers to an alias, *ALIAS will point to that alias.

   If TEXT is a subcommand (i.e. one that is preceded by a prefix
   command) set *PREFIX_CMD.

   Set *CMD to point to the command TEXT indicates.

   If any of *ALIAS, *PREFIX_CMD, or *CMD cannot be determined or do not
   exist, they are NULL when we return.

*/

static int
lookup_cmd_composition_1 (const char *text,
			  struct cmd_list_element **alias,
			  struct cmd_list_element **prefix_cmd,
			  struct cmd_list_element **cmd,
			  struct cmd_list_element *cur_list)
{
  *alias = nullptr;
  *prefix_cmd = cur_list->prefix;
  *cmd = nullptr;

  text = skip_spaces (text);

  /* Go through as many command lists as we need to, to find the command
     TEXT refers to.  */
  while (1)
    {
      /* Identify the name of the command.  */
      int len = find_command_name_length (text);

      /* If nothing but whitespace, return.  */
      if (len == 0)
	return 0;

      /* TEXT is the start of the first command word to lookup (and
	 it's length is LEN).  We copy this into a local temporary.  */
      std::string command (text, len);

      /* Look it up.  */
      int nfound = 0;
      *cmd = find_cmd (command.c_str (), len, cur_list, 1, &nfound);

      /* We only handle the case where a single command was found.  */
      if (*cmd == CMD_LIST_AMBIGUOUS || *cmd == nullptr)
	return 0;
      else
	{
	  if ((*cmd)->is_alias ())
	    {
	      /* If the command was actually an alias, we note that an
		 alias was used (by assigning *ALIAS) and we set *CMD.  */
	      *alias = *cmd;
	      *cmd = (*cmd)->alias_target;
	    }
	}

      text += len;
      text = skip_spaces (text);

      if ((*cmd)->is_prefix () && *text != '\0')
	{
	  cur_list = *(*cmd)->subcommands;
	  *prefix_cmd = *cmd;
	}
      else
	return 1;
    }
}

/* Look up the contents of TEXT as a command in the command list 'cmdlist'.
   Return 1 on success, 0 on failure.

   If TEXT refers to an alias, *ALIAS will point to that alias.

   If TEXT is a subcommand (i.e. one that is preceded by a prefix
   command) set *PREFIX_CMD.

   Set *CMD to point to the command TEXT indicates.

   If any of *ALIAS, *PREFIX_CMD, or *CMD cannot be determined or do not
   exist, they are NULL when we return.

*/

int
lookup_cmd_composition (const char *text,
			struct cmd_list_element **alias,
			struct cmd_list_element **prefix_cmd,
			struct cmd_list_element **cmd)
{
  return lookup_cmd_composition_1 (text, alias, prefix_cmd, cmd, cmdlist);
}

/* Helper function for SYMBOL_COMPLETION_FUNCTION.  */

/* Return a vector of char pointers which point to the different
   possible completions in LIST of TEXT.

   WORD points in the same buffer as TEXT, and completions should be
   returned relative to this position.  For example, suppose TEXT is
   "foo" and we want to complete to "foobar".  If WORD is "oo", return
   "oobar"; if WORD is "baz/foo", return "baz/foobar".  */

void
complete_on_cmdlist (struct cmd_list_element *list,
		     completion_tracker &tracker,
		     const char *text, const char *word,
		     int ignore_help_classes)
{
  struct cmd_list_element *ptr;
  int textlen = strlen (text);
  int pass;
  int saw_deprecated_match = 0;

  /* We do one or two passes.  In the first pass, we skip deprecated
     commands.  If we see no matching commands in the first pass, and
     if we did happen to see a matching deprecated command, we do
     another loop to collect those.  */
  for (pass = 0; pass < 2; ++pass)
    {
      bool got_matches = false;

      for (ptr = list; ptr; ptr = ptr->next)
	if (!strncmp (ptr->name, text, textlen)
	    && !ptr->abbrev_flag
	    && (!ignore_help_classes || !ptr->is_command_class_help ()
		|| ptr->is_prefix ()))
	  {
	    if (pass == 0)
	      {
		if (ptr->cmd_deprecated)
		  {
		    saw_deprecated_match = 1;
		    continue;
		  }
	      }

	    tracker.add_completion
	      (make_completion_match_str (ptr->name, text, word));
	    got_matches = true;
	  }

      if (got_matches)
	break;

      /* If we saw no matching deprecated commands in the first pass,
	 just bail out.  */
      if (!saw_deprecated_match)
	break;
    }
}

/* Helper function for SYMBOL_COMPLETION_FUNCTION.  */

/* Add the different possible completions in ENUMLIST of TEXT.

   WORD points in the same buffer as TEXT, and completions should be
   returned relative to this position.  For example, suppose TEXT is "foo"
   and we want to complete to "foobar".  If WORD is "oo", return
   "oobar"; if WORD is "baz/foo", return "baz/foobar".  */

void
complete_on_enum (completion_tracker &tracker,
		  const char *const *enumlist,
		  const char *text, const char *word)
{
  int textlen = strlen (text);
  int i;
  const char *name;

  for (i = 0; (name = enumlist[i]) != NULL; i++)
    if (strncmp (name, text, textlen) == 0)
      tracker.add_completion (make_completion_match_str (name, text, word));
}

/* Call the command function.  */
void
cmd_func (struct cmd_list_element *cmd, const char *args, int from_tty)
{
  if (!cmd->is_command_class_help ())
    {
      std::optional<scoped_restore_tmpl<bool>> restore_suppress;

      if (cmd->suppress_notification != NULL)
	restore_suppress.emplace (cmd->suppress_notification, true);

      cmd->func (args, from_tty, cmd);
    }
  else
    error (_("Invalid command"));
}

int
cli_user_command_p (struct cmd_list_element *cmd)
{
  return cmd->theclass == class_user && cmd->func == do_simple_func;
}
