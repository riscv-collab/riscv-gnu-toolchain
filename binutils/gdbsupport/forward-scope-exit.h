/* Copyright (C) 2019-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_FORWARD_SCOPE_EXIT_H
#define COMMON_FORWARD_SCOPE_EXIT_H

#include "gdbsupport/scope-exit.h"
#include <functional>

/* A forward_scope_exit is like scope_exit, but instead of giving it a
   callable, you instead specialize it for a given cleanup function,
   and the generated class automatically has a constructor with the
   same interface as the cleanup function.  forward_scope_exit
   captures the arguments passed to the ctor, and in turn passes those
   as arguments to the wrapped cleanup function, when it is called at
   scope exit time, from within the forward_scope_exit dtor.  The
   forward_scope_exit class can take any number of arguments, and is
   cancelable if needed.

   This allows usage like this:

      void
      delete_longjmp_breakpoint (int arg)
      {
	// Blah, blah, blah...
      }

      using longjmp_breakpoint_cleanup
	= FORWARD_SCOPE_EXIT (delete_longjmp_breakpoint);

   This above created a new cleanup class `longjmp_breakpoint_cleanup`
   than can then be used like this:

      longjmp_breakpoint_cleanup obj (thread);

      // Blah, blah, blah...

      obj.release ();  // Optional cancel if needed.

   forward_scope_exit is also handy when you would need to wrap a
   scope_exit in a std::optional:

      std::optional<longjmp_breakpoint_cleanup> cleanup;
      if (some condition)
	cleanup.emplace (thread);
      ...
      if (cleanup)
	cleanup->release ();

   since with scope exit, you would have to know the scope_exit's
   callable template type when you create the std::optional:

     gdb:optional<scope_exit<what goes here?>>

   The "forward" naming fits both purposes shown above -- the class
   "forwards" ctor arguments to the wrapped cleanup function at scope
   exit time, and can also be used to "forward declare"
   scope_exit-like objects.  */

namespace detail
{

/* Function and Signature are passed in the same type, in order to
   extract Function's arguments' types in the specialization below.
   Those are used to generate the constructor.  */

template<typename Function, Function *function, typename Signature>
struct forward_scope_exit;

template<typename Function, Function *function,
	 typename Res, typename... Args>
class forward_scope_exit<Function, function, Res (Args...)>
  : public scope_exit_base<forward_scope_exit<Function,
					      function,
					      Res (Args...)>>
{
  /* For access to on_exit().  */
  friend scope_exit_base<forward_scope_exit<Function,
					    function,
					    Res (Args...)>>;

public:
  explicit forward_scope_exit (Args ...args)
    : m_bind_function (function, args...)
  {
    /* Nothing.  */
  }

private:
  void on_exit ()
  {
    m_bind_function ();
  }

  /* The function and the arguments passed to the ctor, all packed in
     a std::bind.  */
  decltype (std::bind (function, std::declval<Args> ()...))
    m_bind_function;
};

} /* namespace detail */

/* This is the "public" entry point.  It's a macro to avoid having to
   name FUNC more than once.  */

#define FORWARD_SCOPE_EXIT(FUNC) \
  detail::forward_scope_exit<decltype (FUNC), FUNC, decltype (FUNC)>

#endif /* COMMON_FORWARD_SCOPE_EXIT_H */
