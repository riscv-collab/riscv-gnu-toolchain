/* Declarations for debug printing functions.

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

#ifndef COMMON_COMMON_DEBUG_H
#define COMMON_COMMON_DEBUG_H

#include <optional>
#include "gdbsupport/preprocessor.h"

#include <stdarg.h>

/* Set to true to enable debugging of hardware breakpoint/
   watchpoint support code.  */

extern bool show_debug_regs;

/* Print a formatted message to the appropriate channel for
   debugging output for the client.  */

extern void debug_printf (const char *format, ...)
     ATTRIBUTE_PRINTF (1, 2);

/* Print a formatted message to the appropriate channel for
   debugging output for the client.  This function must be
   provided by the client.  */

extern void debug_vprintf (const char *format, va_list ap)
     ATTRIBUTE_PRINTF (1, 0);

/* Print a debug statement prefixed with the module and function name, and
   with a newline at the end.  */

extern void ATTRIBUTE_PRINTF (3, 4) debug_prefixed_printf
  (const char *module, const char *func, const char *format, ...);

/* Print a debug statement prefixed with the module and function name, and
   with a newline at the end.  */

extern void ATTRIBUTE_PRINTF (3, 0) debug_prefixed_vprintf
  (const char *module, const char *func, const char *format, va_list args);

/* Helper to define "_debug_print" macros.

   DEBUG_ENABLED_COND is an expression that evaluates to true if the debugging
   statement is enabled and should be printed.

   The other arguments, as well as the name of the current function, are
   forwarded to debug_prefixed_printf.  */

#define debug_prefixed_printf_cond(debug_enabled_cond, module, fmt, ...) \
  do \
    { \
      if (debug_enabled_cond) \
	debug_prefixed_printf (module, __func__, fmt, ##__VA_ARGS__); \
    } \
  while (0)

#define debug_prefixed_printf_cond_nofunc(debug_enabled_cond, module, fmt, ...) \
  do \
    { \
      if (debug_enabled_cond) \
	debug_prefixed_printf (module, nullptr, fmt, ##__VA_ARGS__); \
    } \
  while (0)

/* Nesting depth of scoped_debug_start_end objects.  */

extern int debug_print_depth;

/* Print a message on construction and destruction, to denote the start and end
   of an operation.  Increment DEBUG_PRINT_DEPTH on construction and decrement
   it on destruction, such that nested debug statements will be printed with
   an indent and appear "inside" this one.  */

template<typename PT>
struct scoped_debug_start_end
{
  /* DEBUG_ENABLED is a reference to a variable that indicates whether debugging
     is enabled, so if the debug statements should be printed.  Is is read
     separately at construction and destruction, such that the start statement
     could be printed but not the end statement, or vice-versa.

     DEBUG_ENABLED should either be of type 'bool &' or should be a type
     that can be invoked.

     MODULE and FUNC are forwarded to debug_prefixed_printf.

     START_PREFIX and END_PREFIX are the statements to print on construction and
     destruction, respectively.

     If the FMT format string is non-nullptr, then a `: ` is appended to the
     messages, followed by the rendering of that format string with ARGS.
     The format string is rendered during construction and is re-used as is
     for the message on exit.  */

  scoped_debug_start_end (PT &debug_enabled, const char *module,
			  const char *func, const char *start_prefix,
			  const char *end_prefix, const char *fmt,
			  va_list args)
    ATTRIBUTE_NULL_PRINTF (7, 0)
    : m_debug_enabled (debug_enabled),
      m_module (module),
      m_func (func),
      m_end_prefix (end_prefix),
      m_with_format (fmt != nullptr)
  {
    if (is_debug_enabled ())
      {
	if (fmt != nullptr)
	  {
	    m_msg = string_vprintf (fmt, args);
	    debug_prefixed_printf (m_module, m_func, "%s: %s",
				   start_prefix, m_msg->c_str ());
	  }
	else
	  debug_prefixed_printf (m_module, m_func, "%s", start_prefix);

	++debug_print_depth;
	m_must_decrement_print_depth = true;
      }
  }

  DISABLE_COPY_AND_ASSIGN (scoped_debug_start_end);

  scoped_debug_start_end (scoped_debug_start_end &&other)
    : m_debug_enabled (other.m_debug_enabled),
      m_module (other.m_module),
      m_func (other.m_func),
      m_end_prefix (other.m_end_prefix),
      m_msg (other.m_msg),
      m_with_format (other.m_with_format),
      m_must_decrement_print_depth (other.m_must_decrement_print_depth),
      m_disabled (other.m_disabled)
  {
    /* Avoid the moved-from object doing the side-effects in its destructor.  */
    other.m_disabled = true;
  }

  ~scoped_debug_start_end ()
  {
    if (m_disabled)
      return;

    if (m_must_decrement_print_depth)
      {
	gdb_assert (debug_print_depth > 0);
	--debug_print_depth;
      }

    if (is_debug_enabled ())
      {
	if (m_with_format)
	  {
	    if (m_msg.has_value ())
	      debug_prefixed_printf (m_module, m_func, "%s: %s",
				     m_end_prefix, m_msg->c_str ());
	    else
	      {
		/* A format string was passed to the constructor, but debug
		   control variable wasn't set at the time, so we don't have the
		   rendering of the format string.  */
		debug_prefixed_printf (m_module, m_func, "%s: <%s debugging was not enabled on entry>",
				       m_end_prefix, m_module);
	      }
	  }
	else
	  debug_prefixed_printf (m_module, m_func, "%s", m_end_prefix);
      }
  }

private:

  /* This function is specialized based on the type PT.  Returns true if
     M_DEBUG_ENABLED indicates this debug setting is enabled, otherwise,
     return false.  */
  bool is_debug_enabled () const;

  /* Reference to the debug setting, or a callback that can read the debug
     setting.  Access the value of this by calling IS_DEBUG_ENABLED.  */
  PT &m_debug_enabled;

  const char *m_module;
  const char *m_func;
  const char *m_end_prefix;

  /* The result of formatting the format string in the constructor.  */
  std::optional<std::string> m_msg;

  /* True is a non-nullptr format was passed to the constructor.  */
  bool m_with_format;

  /* This is used to handle the case where debugging is enabled during
     construction but not during destruction, or vice-versa.  We want to make
     sure there are as many increments are there are decrements.  */
  bool m_must_decrement_print_depth = false;

  /* True if this object was moved from, and the destructor behavior must be
     inhibited.  */
  bool m_disabled = false;
};

/* Implementation of is_debug_enabled when PT is an invokable type.  */

template<typename PT>
inline bool
scoped_debug_start_end<PT>::is_debug_enabled () const
{
  return m_debug_enabled ();
}

/* Implementation of is_debug_enabled when PT is 'bool &'.  */

template<>
inline bool
scoped_debug_start_end<bool &>::is_debug_enabled () const
{
  return m_debug_enabled;
}

/* Wrapper around the scoped_debug_start_end constructor to allow the
   caller to create an object using 'auto' type, the actual type will be
   based on the type of the PRED argument.  All arguments are forwarded to
   the scoped_debug_start_end constructor.  */

template<typename PT>
static inline scoped_debug_start_end<PT &> ATTRIBUTE_NULL_PRINTF (6, 7)
make_scoped_debug_start_end (PT &&pred, const char *module, const char *func,
			     const char *start_prefix,
			     const char *end_prefix, const char *fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  auto res = scoped_debug_start_end<PT &> (pred, module, func, start_prefix,
					   end_prefix, fmt, args);
  va_end (args);

  return res;
}

/* Helper to define a module-specific start/end debug macro.  */

#define scoped_debug_start_end(debug_enabled, module, fmt, ...)		\
  auto CONCAT(scoped_debug_start_end, __LINE__)				\
    = make_scoped_debug_start_end (debug_enabled, module, 	\
				   __func__, "start", "end",	\
				   fmt, ##__VA_ARGS__)

/* Helper to define a module-specific enter/exit debug macro.  This is a special
   case of `scoped_debug_start_end` where the start and end messages are "enter"
   and "exit", to denote entry and exit of a function.  */

#define scoped_debug_enter_exit(debug_enabled, module)	\
  auto CONCAT(scoped_debug_start_end, __LINE__)				\
    = make_scoped_debug_start_end (debug_enabled, module, 	\
				   __func__, "enter", "exit",	\
				   nullptr)

#endif /* COMMON_COMMON_DEBUG_H */
