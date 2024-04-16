/* Target waitstatus implementations.

   Copyright (C) 1990-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "waitstatus.h"

/* See waitstatus.h.  */

std::string
target_waitstatus::to_string () const
{
  std::string str = string_printf
    ("status->kind = %s", target_waitkind_str (this->kind ()));

/* Make sure the compiler warns if a new TARGET_WAITKIND enumerator is added
   but not handled here.  */
DIAGNOSTIC_PUSH
DIAGNOSTIC_ERROR_SWITCH
  switch (this->kind ())
    {
    case TARGET_WAITKIND_EXITED:
    case TARGET_WAITKIND_THREAD_EXITED:
      return string_appendf (str, ", exit_status = %d", this->exit_status ());

    case TARGET_WAITKIND_STOPPED:
    case TARGET_WAITKIND_SIGNALLED:
      return string_appendf (str, ", sig = %s",
			     gdb_signal_to_symbol_string (this->sig ()));

    case TARGET_WAITKIND_FORKED:
    case TARGET_WAITKIND_VFORKED:
    case TARGET_WAITKIND_THREAD_CLONED:
      return string_appendf (str, ", child_ptid = %s",
			     this->child_ptid ().to_string ().c_str ());

    case TARGET_WAITKIND_EXECD:
      return string_appendf (str, ", execd_pathname = %s",
			     this->execd_pathname ());

    case TARGET_WAITKIND_LOADED:
    case TARGET_WAITKIND_VFORK_DONE:
    case TARGET_WAITKIND_SPURIOUS:
    case TARGET_WAITKIND_SYSCALL_ENTRY:
    case TARGET_WAITKIND_SYSCALL_RETURN:
    case TARGET_WAITKIND_IGNORE:
    case TARGET_WAITKIND_NO_HISTORY:
    case TARGET_WAITKIND_NO_RESUMED:
    case TARGET_WAITKIND_THREAD_CREATED:
      return str;
    }
DIAGNOSTIC_POP

  gdb_assert_not_reached ("invalid target_waitkind value: %d",
			  (int) this->kind ());
}
