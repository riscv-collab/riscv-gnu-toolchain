/* Header file for command creation.

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

#if !defined (COMMAND_H)
#define COMMAND_H 1

#include "gdbsupport/gdb_vecs.h"
#include "gdbsupport/scoped_restore.h"

struct completion_tracker;

/* This file defines the public interface for any code wanting to
   create commands.  */

/* Command classes are top-level categories into which commands are
   broken down for "help" purposes.

   The class_alias is used for the user-defined aliases, defined
   using the "alias" command.

   Aliases pre-defined by GDB (e.g. the alias "bt" of the "backtrace" command)
   are not using the class_alias.
   Different pre-defined aliases of the same command do not necessarily
   have the same classes.  For example, class_stack is used for the
   "backtrace" and its "bt" alias", while "info stack" (also an alias
   of "backtrace" uses class_info.  */

enum command_class
{
  /* Classes of commands followed by a comment giving the name
     to use in "help <classname>".
     Note that help accepts unambiguous abbreviated class names.  */

  /* Special classes to help_list */
  all_classes = -2,  /* help without <classname> */
  all_commands = -1, /* all */

  /* Classes of commands */
  no_class = -1,
  class_run = 0,     /* running */
  class_vars,        /* data */
  class_stack,       /* stack */
  class_files,       /* files */
  class_support,     /* support */
  class_info,        /* status */
  class_breakpoint,  /* breakpoints */
  class_trace,       /* tracepoints */
  class_alias,       /* aliases */
  class_bookmark,
  class_obscure,     /* obscure */
  class_maintenance, /* internals */
  class_tui,         /* text-user-interface */
  class_user,        /* user-defined */

  /* Used for "show" commands that have no corresponding "set" command.  */
  no_set_class
};

/* Types of "set" or "show" command.  */
enum var_types
  {
    /* "on" or "off".  *VAR is a bool which is true for on,
       false for off.  */
    var_boolean,

    /* "on" / "true" / "enable" or "off" / "false" / "disable" or
       "auto.  *VAR is an ``enum auto_boolean''.  NOTE: In general a
       custom show command will need to be implemented - one that for
       "auto" prints both the "auto" and the current auto-selected
       value.  */
    var_auto_boolean,

    /* Unsigned Integer.  *VAR is an unsigned int.  In the Guile and Python
       APIs 0 means unlimited, which is stored in *VAR as UINT_MAX.  */
    var_uinteger,

    /* Like var_uinteger but signed.  *VAR is an int.  In the Guile and
       Python APIs 0 means unlimited, which is stored in *VAR as INT_MAX.  */
    var_integer,

    /* Like var_integer but negative numbers are not allowed,
       except for special values.  *VAR is an int.  */
    var_pinteger,

    /* String which the user enters with escapes (e.g. the user types
       \n and it is a real newline in the stored string).
       *VAR is a std::string, "" if the string is empty.  */
    var_string,
    /* String which stores what the user types verbatim.
       *VAR is std::string, "" if the string is empty.  */
    var_string_noescape,
    /* String which stores a filename.  (*VAR) is a std::string,
       "" if the string was empty.  */
    var_optional_filename,
    /* String which stores a filename.  (*VAR) is a std::string.  */
    var_filename,
    /* Enumerated type.  Can only have one of the specified values.
       *VAR is a char pointer to the name of the element that we
       find.  */
    var_enum
  };

/* A structure describing an extra literal accepted and shown in place
   of a number.  */
struct literal_def
{
  /* The literal to define, e.g. "unlimited".  */
  const char *literal;

  /* The number to substitute internally for LITERAL or VAL;
     the use of this number is not allowed (unless the same as VAL).  */
  LONGEST use;

  /* An optional number accepted that stands for the literal.  */
  std::optional<LONGEST> val;
};

/* Return true if a setting of type VAR_TYPE is backed with type T.

   This function is left without definition intentionally.  This template is
   specialized for all valid types that are used to back var_types.  Therefore
   if one tries to instantiate this un-specialized template it means the T
   parameter is not a type used to back a var_type and it is most likely a
   programming error.  */
template<typename T>
bool var_type_uses (var_types var_type) = delete;

/* Return true if a setting of type T is backed by a bool variable.  */
template<>
inline bool var_type_uses<bool> (var_types t)
{
  return t == var_boolean;
};

/* Return true if a setting of type T is backed by a auto_boolean variable.
*/
template<>
inline bool var_type_uses<enum auto_boolean> (var_types t)
{
  return t == var_auto_boolean;
}

/* Return true if a setting of type T is backed by an unsigned int variable.
*/
template<>
inline bool var_type_uses<unsigned int> (var_types t)
{
  return t == var_uinteger;
}

/* Return true if a setting of type T is backed by an int variable.  */
template<>
inline bool var_type_uses<int> (var_types t)
{
  return t == var_integer || t == var_pinteger;
}

/* Return true if a setting of type T is backed by a std::string variable.  */
template<>
inline bool var_type_uses<std::string> (var_types t)
{
  return (t == var_string || t == var_string_noescape
	  || t == var_optional_filename || t == var_filename);
}

/* Return true if a setting of type T is backed by a const char * variable.
*/
template<>
inline bool var_type_uses<const char *> (var_types t)
{
  return t == var_enum;
}

template<bool is_scalar, typename T> struct setting_func_types_1;

template<typename T>
struct setting_func_types_1<true, T>
{
  using type = T;
  using set = void (*) (type);
  using get = type (*) ();
};

template<typename T>
struct setting_func_types_1<false, T>
{
  using type = const T &;
  using set = void (*) (type);
  using get = type (*) ();
};

template<typename T>
struct setting_func_types
{
  using type = typename setting_func_types_1<std::is_scalar<T>::value, T>::type;
  using set = typename setting_func_types_1<std::is_scalar<T>::value, T>::set;
  using get = typename setting_func_types_1<std::is_scalar<T>::value, T>::get;
};

/* Generic/type-erased function pointer.  */

using erased_func = void (*) ();

/* Interface for getting and setting a setting's value.

   The underlying data can be of any VAR_TYPES type.  */
struct setting
{
  /* Create a setting backed by a variable of type T.

     Type T must match the var type VAR_TYPE (see VAR_TYPE_USES).  */
  template<typename T>
  setting (var_types var_type, T *var,
	   const literal_def *extra_literals = nullptr)
    : m_var_type (var_type), m_var (var), m_extra_literals (extra_literals)
  {
    gdb_assert (var != nullptr);
    gdb_assert (var_type_uses<T> (var_type));
  }

  /* A setting can also be constructed with a pre-validated
     type-erased variable.  Use the following function to
     validate & type-erase said variable/function pointers.  */

  struct erased_args
  {
    void *var;
    erased_func setter;
    erased_func getter;
  };

  template<typename T>
  static erased_args erase_args (var_types var_type,
				 T *var,
				 typename setting_func_types<T>::set set_setting_func,
				 typename setting_func_types<T>::get get_setting_func)
  {
    gdb_assert (var_type_uses<T> (var_type));
  /* The getter and the setter must be both provided or both omitted.  */
    gdb_assert
      ((set_setting_func == nullptr) == (get_setting_func == nullptr));

  /* The caller must provide a pointer to a variable or get/set functions, but
     not both.  */
    gdb_assert ((set_setting_func == nullptr) != (var == nullptr));

    return {
	var,
	reinterpret_cast<erased_func> (set_setting_func),
	reinterpret_cast<erased_func> (get_setting_func)
    };
  }

  /* Create a setting backed by pre-validated type-erased args and using
     EXTRA_LITERALS.  ERASED_VAR's fields' real types must match the var
     type VAR_TYPE (see VAR_TYPE_USES).  */
  setting (var_types var_type, const literal_def *extra_literals,
	   const erased_args &args)
    : m_var_type (var_type),
      m_var (args.var),
      m_extra_literals (extra_literals),
      m_getter (args.getter),
      m_setter (args.setter)
  {
  }

  /* Create a setting backed by setter and getter functions.

     Type T must match the var type VAR_TYPE (see VAR_TYPE_USES).  */
  template<typename T>
  setting (var_types var_type,
	   typename setting_func_types<T>::set setter,
	   typename setting_func_types<T>::get getter)
    : m_var_type (var_type)
  {
    gdb_assert (var_type_uses<T> (var_type));

    /* Getters and setters are cast to and from the arbitrary `void (*) ()`
       function pointer type.  Make sure that the two types are really of the
       same size.  */
    static_assert (sizeof (m_getter) == sizeof (getter));
    static_assert (sizeof (m_setter) == sizeof (setter));

    m_getter = reinterpret_cast<erased_func> (getter);
    m_setter = reinterpret_cast<erased_func> (setter);
  }

  /* Access the type of the current setting.  */
  var_types type () const
  { return m_var_type; }

  /* Access any extra literals accepted.  */
  const literal_def *extra_literals () const
  { return m_extra_literals; }

  /* Return the current value.

     The template parameter T is the type of the variable used to store the
     setting.  */
  template<typename T>
  typename setting_func_types<T>::type get () const
  {
    gdb_assert (var_type_uses<T> (m_var_type));

    if (m_var == nullptr)
      {
	gdb_assert (m_getter != nullptr);
	auto getter = reinterpret_cast<typename setting_func_types<T>::get> (m_getter);
	return getter ();
      }
    else
      return *static_cast<const T *> (m_var);
  }

  /* Sets the value of the setting to V.  Returns true if the setting was
     effectively changed, false if the update failed and the setting is left
     unchanged.

     If we have a user-provided setter, use it to set the setting.  Otherwise
     copy the value V to the internally referenced buffer.

     The template parameter T indicates the type of the variable used to store
     the setting.

     The var_type of the setting must match T.  */
  template<typename T>
  bool set (const T &v)
  {
    /* Check that the current instance is of one of the supported types for
       this instantiation.  */
    gdb_assert (var_type_uses<T> (m_var_type));

    const T old_value = this->get<T> ();

    if (m_var == nullptr)
      {
	gdb_assert (m_setter != nullptr);
	auto setter = reinterpret_cast<typename setting_func_types<T>::set> (m_setter);
	setter (v);
      }
    else
      *static_cast<T *> (m_var) = v;

    return old_value != this->get<T> ();
  }

private:
  /* The type of the variable M_VAR is pointing to, or that M_GETTER / M_SETTER
     get or set.  */
  var_types m_var_type;

  /* Pointer to the enclosed variable

     Either M_VAR is non-nullptr, or both M_GETTER and M_SETTER are
     non-nullptr.  */
  void *m_var = nullptr;

  /* Any extra literals accepted.  */
  const literal_def *m_extra_literals = nullptr;

  /* Pointer to a user provided getter.  */
  erased_func m_getter = nullptr;

  /* Pointer to a user provided setter.  */
  erased_func m_setter = nullptr;
};

/* This structure records one command'd definition.  */
struct cmd_list_element;

/* The "simple" signature of command callbacks, which doesn't include a
   cmd_list_element parameter.  */

typedef void cmd_simple_func_ftype (const char *args, int from_tty);

/* This structure specifies notifications to be suppressed by a cli
   command interpreter.  */

struct cli_suppress_notification
{
  /* Inferior, thread, frame selected notification suppressed?  */
  bool user_selected_context = false;

  /* Normal stop event suppressed? */
  bool normal_stop = false;
};

extern struct cli_suppress_notification cli_suppress_notification;

/* Forward-declarations of the entry-points of cli/cli-decode.c.  */

/* API to the manipulation of command lists.  */

/* Return TRUE if NAME is a valid user-defined command name.
   This is a stricter subset of all gdb commands,
   see find_command_name_length.  */

extern bool valid_user_defined_cmd_name_p (const char *name);

/* Return TRUE if C is a valid command character.  */

extern bool valid_cmd_char_p (int c);

/* Return value type for the add_setshow_* functions.  */

struct set_show_commands
{
  cmd_list_element *set, *show;
};

/* Const-correct variant of the above.  */

extern struct cmd_list_element *add_cmd (const char *, enum command_class,
					 cmd_simple_func_ftype *fun,
					 const char *,
					 struct cmd_list_element **);

/* Like add_cmd, but no command function is specified.  */

extern struct cmd_list_element *add_cmd (const char *, enum command_class,
					 const char *,
					 struct cmd_list_element **);

extern struct cmd_list_element *add_cmd_suppress_notification
			(const char *name, enum command_class theclass,
			 cmd_simple_func_ftype *fun, const char *doc,
			 struct cmd_list_element **list,
			 bool *suppress_notification);

extern struct cmd_list_element *add_alias_cmd (const char *,
					       cmd_list_element *,
					       enum command_class, int,
					       struct cmd_list_element **);


extern struct cmd_list_element *add_prefix_cmd (const char *, enum command_class,
						cmd_simple_func_ftype *fun,
						const char *,
						struct cmd_list_element **,
						int,
						struct cmd_list_element **);

/* Like add_prefix_cmd, but sets the callback to a function that
   simply calls help_list.  */

extern struct cmd_list_element *add_basic_prefix_cmd
  (const char *, enum command_class, const char *, struct cmd_list_element **,
   int, struct cmd_list_element **);

/* Like add_prefix_cmd, but useful for "show" prefixes.  This sets the
   callback to a function that simply calls cmd_show_list.  */

extern struct cmd_list_element *add_show_prefix_cmd
  (const char *, enum command_class, const char *, struct cmd_list_element **,
   int, struct cmd_list_element **);

/* Add matching set and show commands using add_basic_prefix_cmd and
   add_show_prefix_cmd.  */

extern set_show_commands add_setshow_prefix_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc,
   cmd_list_element **set_subcommands_list,
   cmd_list_element **show_subcommands_list,
   cmd_list_element **set_list,
   cmd_list_element **show_list);

extern struct cmd_list_element *add_prefix_cmd_suppress_notification
			(const char *name, enum command_class theclass,
			 cmd_simple_func_ftype *fun,
			 const char *doc, struct cmd_list_element **subcommands,
			 int allow_unknown,
			 struct cmd_list_element **list,
			 bool *suppress_notification);

extern struct cmd_list_element *add_abbrev_prefix_cmd (const char *,
						       enum command_class,
						       cmd_simple_func_ftype *fun,
						       const char *,
						       struct cmd_list_element
						       **, int,
						       struct cmd_list_element
						       **);

typedef void cmd_func_ftype (const char *args, int from_tty,
			     cmd_list_element *c);

/* A completion routine.  Add possible completions to tracker.

   TEXT is the text beyond what was matched for the command itself
   (leading whitespace is skipped).  It stops where we are supposed to
   stop completing (rl_point) and is '\0' terminated.  WORD points in
   the same buffer as TEXT, and completions should be returned
   relative to this position.  For example, suppose TEXT is "foo" and
   we want to complete to "foobar".  If WORD is "oo", return "oobar";
   if WORD is "baz/foo", return "baz/foobar".  */
typedef void completer_ftype (struct cmd_list_element *,
			      completion_tracker &tracker,
			      const char *text, const char *word);

/* Same, but for set_cmd_completer_handle_brkchars.  */
typedef void completer_handle_brkchars_ftype (struct cmd_list_element *,
					      completion_tracker &tracker,
					      const char *text, const char *word);

extern void set_cmd_completer (struct cmd_list_element *, completer_ftype *);

/* Set the completer_handle_brkchars callback.  */

extern void set_cmd_completer_handle_brkchars (struct cmd_list_element *,
					       completer_handle_brkchars_ftype *);

/* HACK: cagney/2002-02-23: Code, mostly in tracepoints.c, grubs
   around in cmd objects to test the value of the commands sfunc().  */
extern int cmd_simple_func_eq (struct cmd_list_element *cmd,
			 cmd_simple_func_ftype *cfun);

/* Execute CMD's pre/post hook.  Throw an error if the command fails.
   If already executing this pre/post hook, or there is no pre/post
   hook, the call is silently ignored.  */
extern void execute_cmd_pre_hook (struct cmd_list_element *cmd);
extern void execute_cmd_post_hook (struct cmd_list_element *cmd);

/* Flag for an ambiguous cmd_list result.  */
#define CMD_LIST_AMBIGUOUS ((struct cmd_list_element *) -1)

extern struct cmd_list_element *lookup_cmd (const char **,
					    struct cmd_list_element *,
					    const char *,
					    std::string *,
					    int, int);

/* This routine takes a line of TEXT and a CLIST in which to start the
   lookup.  When it returns it will have incremented the text pointer past
   the section of text it matched, set *RESULT_LIST to point to the list in
   which the last word was matched, and will return a pointer to the cmd
   list element which the text matches.  It will return NULL if no match at
   all was possible.  It will return -1 (cast appropriately, ick) if ambigous
   matches are possible; in this case *RESULT_LIST will be set to point to
   the list in which there are ambiguous choices (and *TEXT will be set to
   the ambiguous text string).

   if DEFAULT_ARGS is not null, *DEFAULT_ARGS is set to the found command
   default args (possibly empty).

   If the located command was an abbreviation, this routine returns the base
   command of the abbreviation.  Note that *DEFAULT_ARGS will contain the
   default args defined for the alias.

   It does no error reporting whatsoever; control will always return
   to the superior routine.

   In the case of an ambiguous return (-1), *RESULT_LIST will be set to point
   at the prefix_command (ie. the best match) *or* (special case) will be NULL
   if no prefix command was ever found.  For example, in the case of "info a",
   "info" matches without ambiguity, but "a" could be "args" or "address", so
   *RESULT_LIST is set to the cmd_list_element for "info".  So in this case
   RESULT_LIST should not be interpreted as a pointer to the beginning of a
   list; it simply points to a specific command.  In the case of an ambiguous
   return *TEXT is advanced past the last non-ambiguous prefix (e.g.
   "info t" can be "info types" or "info target"; upon return *TEXT has been
   advanced past "info ").

   If RESULT_LIST is NULL, don't set *RESULT_LIST (but don't otherwise
   affect the operation).

   This routine does *not* modify the text pointed to by TEXT.

   If IGNORE_HELP_CLASSES is nonzero, ignore any command list elements which
   are actually help classes rather than commands (i.e. the function field of
   the struct cmd_list_element is NULL).

   When LOOKUP_FOR_COMPLETION_P is true the completion is being requested
   for the completion engine, no warnings should be printed.  */

extern struct cmd_list_element *lookup_cmd_1
	(const char **text, struct cmd_list_element *clist,
	 struct cmd_list_element **result_list, std::string *default_args,
	 int ignore_help_classes, bool lookup_for_completion_p = false);

/* Look up the command called NAME in the command list LIST.

   Unlike LOOKUP_CMD, partial matches are ignored and only exact matches
   on NAME are considered.

   LIST is a chain of struct cmd_list_element's.

   If IGNORE_HELP_CLASSES is true (the default), ignore any command list
   elements which are actually help classes rather than commands (i.e.
   the function field of the struct cmd_list_element is null).

   If found, return the struct cmd_list_element for that command,
   otherwise return NULLPTR.  */

extern struct cmd_list_element *lookup_cmd_exact
			(const char *name,
			 struct cmd_list_element *list,
			 bool ignore_help_classes = true);

extern struct cmd_list_element *deprecate_cmd (struct cmd_list_element *,
					       const char * );

extern void deprecated_cmd_warning (const char *, struct cmd_list_element *);

extern int lookup_cmd_composition (const char *text,
				   struct cmd_list_element **alias,
				   struct cmd_list_element **prefix_cmd,
				   struct cmd_list_element **cmd);

extern struct cmd_list_element *add_com (const char *, enum command_class,
					 cmd_simple_func_ftype *fun,
					 const char *);

extern cmd_list_element *add_com_alias (const char *name,
					cmd_list_element *target,
					command_class theclass,
					int abbrev_flag);

extern struct cmd_list_element *add_com_suppress_notification
		       (const char *name, enum command_class theclass,
			cmd_simple_func_ftype *fun, const char *doc,
			bool *suppress_notification);

extern struct cmd_list_element *add_info (const char *,
					  cmd_simple_func_ftype *fun,
					  const char *);

extern cmd_list_element *add_info_alias (const char *name,
					 cmd_list_element *target,
					 int abbrev_flag);

extern void complete_on_cmdlist (struct cmd_list_element *,
				 completion_tracker &tracker,
				 const char *, const char *, int);

extern void complete_on_enum (completion_tracker &tracker,
			      const char *const *enumlist,
			      const char *, const char *);

/* Functions that implement commands about CLI commands.  */

extern void help_list (struct cmd_list_element *, const char *,
		       enum command_class, struct ui_file *);

/* Method for show a set/show variable's VALUE on FILE.  */
typedef void (show_value_ftype) (struct ui_file *file,
				 int from_tty,
				 struct cmd_list_element *cmd,
				 const char *value);

/* Various sets of extra literals accepted.  */
extern const literal_def integer_unlimited_literals[];
extern const literal_def uinteger_unlimited_literals[];
extern const literal_def pinteger_unlimited_literals[];

extern set_show_commands add_setshow_enum_cmd
  (const char *name, command_class theclass, const char *const *enumlist,
   const char **var, const char *set_doc, const char *show_doc,
   const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_enum_cmd
  (const char *name, command_class theclass, const char *const *enumlist,
   const char *set_doc, const char *show_doc,
   const char *help_doc, setting_func_types<const char *>::set set_func,
   setting_func_types<const char *>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_auto_boolean_cmd
  (const char *name, command_class theclass, auto_boolean *var,
   const char *set_doc, const char *show_doc, const char *help_doc,
   cmd_func_ftype *set_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_auto_boolean_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<enum auto_boolean>::set set_func,
   setting_func_types<enum auto_boolean>::get get_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_boolean_cmd
  (const char *name, command_class theclass, bool *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_boolean_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<bool>::set set_func,
   setting_func_types<bool>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_filename_cmd
  (const char *name, command_class theclass, std::string *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_filename_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<std::string>::set set_func,
   setting_func_types<std::string>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_string_cmd
  (const char *name, command_class theclass, std::string *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_string_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<std::string>::set set_func,
   setting_func_types<std::string>::get get_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_string_noescape_cmd
  (const char *name, command_class theclass, std::string *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_string_noescape_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<std::string>::set set_func,
   setting_func_types<std::string>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_optional_filename_cmd
  (const char *name, command_class theclass, std::string *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_optional_filename_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<std::string>::set set_func,
   setting_func_types<std::string>::get get_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_integer_cmd
  (const char *name, command_class theclass, int *var,
   const literal_def *extra_literals, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_integer_cmd
  (const char *name, command_class theclass, const literal_def *extra_literals,
   const char *set_doc, const char *show_doc, const char *help_doc,
   setting_func_types<int>::set set_func,
   setting_func_types<int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_integer_cmd
  (const char *name, command_class theclass, int *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_integer_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<int>::set set_func,
   setting_func_types<int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_pinteger_cmd
  (const char *name, command_class theclass, int *var,
   const literal_def *extra_literals, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_pinteger_cmd
  (const char *name, command_class theclass, const literal_def *extra_literals,
   const char *set_doc, const char *show_doc, const char *help_doc,
   setting_func_types<int>::set set_func,
   setting_func_types<int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_uinteger_cmd
  (const char *name, command_class theclass, unsigned int *var,
   const literal_def *extra_literals,
   const char *set_doc, const char *show_doc, const char *help_doc,
   cmd_func_ftype *set_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_uinteger_cmd
  (const char *name, command_class theclass, const literal_def *extra_literals,
   const char *set_doc, const char *show_doc, const char *help_doc,
   setting_func_types<unsigned int>::set set_func,
   setting_func_types<unsigned int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_uinteger_cmd
  (const char *name, command_class theclass, unsigned int *var,
   const char *set_doc, const char *show_doc, const char *help_doc,
   cmd_func_ftype *set_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_uinteger_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<unsigned int>::set set_func,
   setting_func_types<unsigned int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_zinteger_cmd
  (const char *name, command_class theclass, int *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_zinteger_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<int>::set set_func,
   setting_func_types<int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_zuinteger_cmd
  (const char *name, command_class theclass, unsigned int *var,
   const char *set_doc, const char *show_doc, const char *help_doc,
   cmd_func_ftype *set_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_zuinteger_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<unsigned int>::set set_func,
   setting_func_types<unsigned int>::get get_func, show_value_ftype *show_func,
   cmd_list_element **set_list, cmd_list_element **show_list);

extern set_show_commands add_setshow_zuinteger_unlimited_cmd
  (const char *name, command_class theclass, int *var, const char *set_doc,
   const char *show_doc, const char *help_doc, cmd_func_ftype *set_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

extern set_show_commands add_setshow_zuinteger_unlimited_cmd
  (const char *name, command_class theclass, const char *set_doc,
   const char *show_doc, const char *help_doc,
   setting_func_types<int>::set set_func, setting_func_types<int>::get get_func,
   show_value_ftype *show_func, cmd_list_element **set_list,
   cmd_list_element **show_list);

/* Do a "show" command for each thing on a command list.  */

extern void cmd_show_list (struct cmd_list_element *, int);

/* Used everywhere whenever at least one parameter is required and
   none is specified.  */

extern void error_no_arg (const char *) ATTRIBUTE_NORETURN;


/* Command line saving and repetition.
   Each input line executed is saved to possibly be repeated either
   when the user types an empty line, or be repeated by a command
   that wants to repeat the previously executed command.  The below
   functions control command repetition.  */

/* Commands call dont_repeat if they do not want to be repeated by null
   lines or by repeat_previous ().  */

extern void dont_repeat ();

/* Commands call repeat_previous if they want to repeat the previous
   command.  Such commands that repeat the previous command must
   indicate to not repeat themselves, to avoid recursive repeat.
   repeat_previous marks the current command as not repeating, and
   ensures get_saved_command_line returns the previous command, so
   that the currently executing command can repeat it.  If there's no
   previous command, throws an error.  Otherwise, returns the result
   of get_saved_command_line, which now points at the command to
   repeat.  */

extern const char *repeat_previous ();

/* Prevent dont_repeat from working, and return a cleanup that
   restores the previous state.  */

extern scoped_restore_tmpl<int> prevent_dont_repeat (void);

/* Set the arguments that will be passed if the current command is
   repeated.  Note that the passed-in string must be a constant.  */

extern void set_repeat_arguments (const char *args);

/* Returns the saved command line to repeat.
   When a command is being executed, this is the currently executing
   command line, unless the currently executing command has called
   repeat_previous (): in this case, get_saved_command_line returns
   the previously saved command line.  */

extern char *get_saved_command_line ();

/* Takes a copy of CMD, for possible repetition.  */

extern void save_command_line (const char *cmd);

/* Used to mark commands that don't do anything.  If we just leave the
   function field NULL, the command is interpreted as a help topic, or
   as a class of commands.  */

extern void not_just_help_class_command (const char *, int);

/* Call the command function.  */
extern void cmd_func (struct cmd_list_element *cmd,
		      const char *args, int from_tty);

#endif /* !defined (COMMAND_H) */
