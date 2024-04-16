/* Utility for handling interrupted syscalls by signals.

   Copyright (C) 2020-2024 Free Software Foundation, Inc.

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

#ifndef GDBSUPPORT_EINTR_H
#define GDBSUPPORT_EINTR_H

#include <cerrno>

namespace gdb
{
/* Repeat a system call interrupted with a signal.

   A utility for handling interrupted syscalls, which return with error
   and set the errno to EINTR.  The interrupted syscalls can be repeated,
   until successful completion.  This utility avoids wrapping code with
   manual checks for such errors which are highly repetitive.

   For example, with:

   ssize_t ret;
   do
     {
       errno = 0;
       ret = ::write (pipe[1], "+", 1);
     }
   while (ret == -1 && errno == EINTR);

   You could wrap it by writing the wrapped form:

   ssize_t ret = gdb::handle_eintr (-1, ::write, pipe[1], "+", 1);

   ERRVAL specifies the failure value indicating that the call to the
   F function with ARGS... arguments was possibly interrupted with a
   signal.  */

template<typename ErrorValType, typename Fun, typename... Args>
inline auto
handle_eintr (ErrorValType errval, const Fun &f, const Args &... args)
  -> decltype (f (args...))
{
  decltype (f (args...)) ret;

  do
    {
      errno = 0;
      ret = f (args...);
    }
  while (ret == errval && errno == EINTR);

  return ret;
}

} /* namespace gdb */

#endif /* GDBSUPPORT_EINTR_H */
