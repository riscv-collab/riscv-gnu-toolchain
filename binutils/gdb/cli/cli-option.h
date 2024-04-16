/* CLI options framework, for GDB.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#ifndef CLI_OPTION_H
#define CLI_OPTION_H 1

#include <optional>
#include "gdbsupport/array-view.h"
#include "completer.h"
#include <string>
#include "command.h"

namespace gdb {
namespace option {

/* A type-erased option definition.  The actual type of the option is
   stored in the TYPE field.  Client code cannot define objects of
   this type directly (the ctor is protected).  Instead, one of the
   wrapper types below that extends this (boolean_option_def,
   flag_option_def, uinteger_option_def, etc.) should be defined.  */
struct option_def
{
  /* The ctor is protected because you're supposed to construct using
     one of bool_option_def, etc. below.  */
protected:
  typedef void *(erased_get_var_address_ftype) ();

  /* Construct an option.  NAME_ is the option's name.  VAR_TYPE_
     defines the option's type.  ERASED_GET_VAR_ADDRESS_ is a pointer
     to a function that returns the option's control variable.
     SHOW_CMD_CB_ is a pointer to callback for the "show" command that
     is installed for this option.  SET_DOC_, SHOW_DOC_, HELP_DOC_ are
     used to create the option's "set/show" commands.  */
  constexpr option_def (const char *name_,
			var_types var_type_,
			const literal_def *extra_literals_,
			erased_get_var_address_ftype *erased_get_var_address_,
			show_value_ftype *show_cmd_cb_,
			const char *set_doc_,
			const char *show_doc_,
			const char *help_doc_)
    : name (name_), type (var_type_), extra_literals (extra_literals_),
      erased_get_var_address (erased_get_var_address_),
      var_address {},
      show_cmd_cb (show_cmd_cb_),
      set_doc (set_doc_), show_doc (show_doc_), help_doc (help_doc_)
  {}

public:
  /* The option's name.  */
  const char *name;

  /* The option's type.  */
  var_types type;

  /* Extra literals, such as `unlimited', accepted in lieu of a number.  */
  const literal_def *extra_literals;

  /* A function that gets the controlling variable's address, type
     erased.  */
  erased_get_var_address_ftype *erased_get_var_address;

  /* Get the controlling variable's address.  Each type of variable
     uses a different union member.  We do this instead of having a
     single hook that return a "void *", for better type safety.  This
     way, actual instances of concrete option_def types
     (boolean_option_def, etc.) fail to compile if you pass in a
     function with incorrect return type.  CTX here is the aggregate
     object that groups the option variables from which the callback
     returns the address of some member.  */
  union
    {
      bool *(*boolean) (const option_def &, void *ctx);
      unsigned int *(*uinteger) (const option_def &, void *ctx);
      int *(*integer) (const option_def &, void *ctx);
      const char **(*enumeration) (const option_def &, void *ctx);
      std::string *(*string) (const option_def &, void *ctx);
    }
  var_address;

  /* Pointer to null terminated list of enumerated values (like argv).
     Only used by var_enum options.  */
  const char *const *enums = nullptr;

  /* True if the option takes an argument.  */
  bool have_argument = true;

  /* The "show" callback to use in the associated "show" command.
     E.g., "show print elements".  */
  show_value_ftype *show_cmd_cb;

  /* The set/show/help strings.  These are shown in both the help of
     commands that use the option group this option belongs to (e.g.,
     "help print"), and in the associated commands (e.g., "set/show
     print elements", "help set print elements").  */
  const char *set_doc;
  const char *show_doc;
  const char *help_doc;

  /* Convenience method that returns THIS as an option_def.  Useful
     when you're putting an option_def subclass in an option_def
     array_view.  */
  const option_def &def () const
  {
    return *this;
  }
};

namespace detail
{

/* Get the address of the option's value, cast to the right type.
   RetType is the restored type of the variable, and Context is the
   restored type of the context pointer.  */
template<typename RetType, typename Context>
static inline RetType *
get_var_address (const option_def &option, void *ctx)
{
  using unerased_ftype = RetType *(Context *);
  unerased_ftype *fun = (unerased_ftype *) option.erased_get_var_address;
  return fun ((Context *) ctx);
}

/* Convenience identity helper that just returns SELF.  */

template<typename T>
static T *
return_self (T *self)
{
  return self;
}

} /* namespace detail */

/* Follows the definitions of the option types that client code should
   define.  Note that objects of these types are placed in option_def
   arrays, by design, so they must not have data fields of their
   own.  */

/* A var_boolean command line option.  */

template<typename Context>
struct boolean_option_def : option_def
{
  boolean_option_def (const char *long_option_,
		      bool *(*get_var_address_cb_) (Context *),
		      show_value_ftype *show_cmd_cb_,
		      const char *set_doc_,
		      const char *show_doc_ = nullptr,
		      const char *help_doc_ = nullptr)
    : option_def (long_option_, var_boolean, nullptr,
		  (erased_get_var_address_ftype *) get_var_address_cb_,
		  show_cmd_cb_,
		  set_doc_, show_doc_, help_doc_)
  {
    var_address.boolean = detail::get_var_address<bool, Context>;
  }
};

/* A flag command line option.  This is a var_boolean option under the
   hood, but unlike boolean options, flag options don't take an on/off
   argument.  */

template<typename Context = bool>
struct flag_option_def : boolean_option_def<Context>
{
  flag_option_def (const char *long_option_,
		     bool *(*var_address_cb_) (Context *),
		     const char *set_doc_,
		     const char *help_doc_ = nullptr)
    : boolean_option_def<Context> (long_option_,
				   var_address_cb_,
				   NULL,
				   set_doc_, NULL, help_doc_)
  {
    this->have_argument = false;
  }

  flag_option_def (const char *long_option_,
		     const char *set_doc_,
		     const char *help_doc_ = nullptr)
    : boolean_option_def<Context> (long_option_,
				   gdb::option::detail::return_self,
				   NULL,
				   set_doc_, nullptr, help_doc_)
  {
    this->have_argument = false;
  }
};

/* A var_uinteger command line option.  */

template<typename Context>
struct uinteger_option_def : option_def
{
  uinteger_option_def (const char *long_option_,
		       unsigned int *(*get_var_address_cb_) (Context *),
		       const literal_def *extra_literals_,
		       show_value_ftype *show_cmd_cb_,
		       const char *set_doc_,
		       const char *show_doc_ = nullptr,
		       const char *help_doc_ = nullptr)
    : option_def (long_option_, var_uinteger, extra_literals_,
		  (erased_get_var_address_ftype *) get_var_address_cb_,
		  show_cmd_cb_,
		  set_doc_, show_doc_, help_doc_)
  {
    var_address.uinteger = detail::get_var_address<unsigned int, Context>;
  }

  uinteger_option_def (const char *long_option_,
		       unsigned int *(*get_var_address_cb_) (Context *),
		       show_value_ftype *show_cmd_cb_,
		       const char *set_doc_,
		       const char *show_doc_ = nullptr,
		       const char *help_doc_ = nullptr)
    : uinteger_option_def (long_option_, get_var_address_cb_, nullptr,
			   show_cmd_cb_, set_doc_, show_doc_, help_doc_)
  { /* Nothing.  */ }
};

/* A var_pinteger command line option.  */

template<typename Context>
struct pinteger_option_def : option_def
{
  pinteger_option_def (const char *long_option_,
		       int *(*get_var_address_cb_) (Context *),
		       const literal_def *extra_literals_,
		       show_value_ftype *show_cmd_cb_,
		       const char *set_doc_,
		       const char *show_doc_ = nullptr,
		       const char *help_doc_ = nullptr)
    : option_def (long_option_, var_pinteger, extra_literals_,
		  (erased_get_var_address_ftype *) get_var_address_cb_,
		  show_cmd_cb_,
		  set_doc_, show_doc_, help_doc_)
  {
    var_address.integer = detail::get_var_address<int, Context>;
  }

  pinteger_option_def (const char *long_option_,
		       int *(*get_var_address_cb_) (Context *),
		       show_value_ftype *show_cmd_cb_,
		       const char *set_doc_,
		       const char *show_doc_ = nullptr,
		       const char *help_doc_ = nullptr)
    : pinteger_option_def (long_option_, get_var_address_cb_, nullptr,
			   show_cmd_cb_, set_doc_, show_doc_, help_doc_)
  { /* Nothing.  */ }
};

/* An var_enum command line option.  */

template<typename Context>
struct enum_option_def : option_def
{
  enum_option_def (const char *long_option_,
		   const char *const *enumlist,
		   const char **(*get_var_address_cb_) (Context *),
		   show_value_ftype *show_cmd_cb_,
		   const char *set_doc_,
		   const char *show_doc_ = nullptr,
		   const char *help_doc_ = nullptr)
    : option_def (long_option_, var_enum, nullptr,
		  (erased_get_var_address_ftype *) get_var_address_cb_,
		  show_cmd_cb_,
		  set_doc_, show_doc_, help_doc_)
  {
    var_address.enumeration = detail::get_var_address<const char *, Context>;
    this->enums = enumlist;
  }
};

/* A var_string command line option.  */

template<typename Context>
struct string_option_def : option_def
{
  string_option_def (const char *long_option_,
		     std::string *(*get_var_address_cb_) (Context *),
		     show_value_ftype *show_cmd_cb_,
		     const char *set_doc_,
		     const char *show_doc_ = nullptr,
		     const char *help_doc_ = nullptr)
    : option_def (long_option_, var_string, nullptr,
		  (erased_get_var_address_ftype *) get_var_address_cb_,
		  show_cmd_cb_,
		  set_doc_, show_doc_, help_doc_)
  {
    var_address.string = detail::get_var_address<std::string, Context>;
  }
};

/* A group of options that all share the same context pointer to pass
   to the options' get-current-value callbacks.  */
struct option_def_group
{
  /* The list of options.  */
  gdb::array_view<const option_def> options;

  /* The context pointer to pass to the options' get-current-value
     callbacks.  */
  void *ctx;
};

/* Modes of operation for process_options.  */
enum process_options_mode
{
  /* In this mode, options are only processed if we find a "--"
     delimiter.  Throws an error if unknown options are found.  */
  PROCESS_OPTIONS_REQUIRE_DELIMITER,

  /* In this mode, a "--" delimiter is not required.  Throws an error
     if unknown options are found, regardless of whether a delimiter
     is present.  */
  PROCESS_OPTIONS_UNKNOWN_IS_ERROR,

  /* In this mode, a "--" delimiter is not required.  If an unknown
     option is found, assume it is the command's argument/operand.  */
  PROCESS_OPTIONS_UNKNOWN_IS_OPERAND,
};

/* Process ARGS, using OPTIONS_GROUP as valid options.  Returns true
   if the string has been fully parsed and there are no operands to
   handle by the caller.  Return false if options were parsed, and
   *ARGS now points at the first operand.  */
extern bool process_options
  (const char **args,
   process_options_mode mode,
   gdb::array_view<const option_def_group> options_group);

/* Complete ARGS on options listed by OPTIONS_GROUP.  Returns true if
   the string has been fully parsed and there are no operands to
   handle by the caller.  Return false if options were parsed, and
   *ARGS now points at the first operand.  */
extern bool complete_options
  (completion_tracker &tracker,
   const char **args,
   process_options_mode mode,
   gdb::array_view<const option_def_group> options_group);

/* Complete on all options listed by OPTIONS_GROUP.  */
extern void
  complete_on_all_options (completion_tracker &tracker,
			   gdb::array_view<const option_def_group> options_group);

/* Return a string with the result of replacing %OPTIONS% in HELP_TMLP
   with an auto-generated "help" string fragment for all the options
   in OPTIONS_GROUP.  */
extern std::string build_help
  (const char *help_tmpl,
   gdb::array_view<const option_def_group> options_group);

/* Install set/show commands for options defined in OPTIONS.  DATA is
   a pointer to the structure that holds the data associated with the
   OPTIONS array.  */
extern void add_setshow_cmds_for_options (command_class cmd_class, void *data,
					  gdb::array_view<const option_def> options,
					  struct cmd_list_element **set_list,
					  struct cmd_list_element **show_list);

} /* namespace option */
} /* namespace gdb */

#endif /* CLI_OPTION_H */
