/* Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#include "server.h"
#include "dll.h"

#include <algorithm>

/* An "unspecified" CORE_ADDR, for match_dll.  */
#define UNSPECIFIED_CORE_ADDR (~(CORE_ADDR) 0)

/* Record a newly loaded DLL at BASE_ADDR for the current process.  */

void
loaded_dll (const char *name, CORE_ADDR base_addr)
{
  loaded_dll (current_process (), name, base_addr);
}

/* Record a newly loaded DLL at BASE_ADDR for PROC.  */

void
loaded_dll (process_info *proc, const char *name, CORE_ADDR base_addr)
{
  gdb_assert (proc != nullptr);
  proc->all_dlls.emplace_back (name != nullptr ? name : "", base_addr);
  proc->dlls_changed = true;
}

/* Record that the DLL with NAME and BASE_ADDR has been unloaded
   from the current process.  */

void
unloaded_dll (const char *name, CORE_ADDR base_addr)
{
  unloaded_dll (current_process (), name, base_addr);
}

/* Record that the DLL with NAME and BASE_ADDR has been unloaded
   from PROC.  */

void
unloaded_dll (process_info *proc, const char *name, CORE_ADDR base_addr)
{
  gdb_assert (proc != nullptr);
  auto pred = [&] (const dll_info &dll)
    {
      if (base_addr != UNSPECIFIED_CORE_ADDR
	  && base_addr == dll.base_addr)
	return true;

      if (name != NULL && dll.name == name)
	return true;

      return false;
    };

  auto iter = std::find_if (proc->all_dlls.begin (), proc->all_dlls.end (),
			    pred);

  if (iter == proc->all_dlls.end ())
    /* For some inferiors we might get unloaded_dll events without having
       a corresponding loaded_dll.  In that case, the dll cannot be found
       in ALL_DLL, and there is nothing further for us to do.

       This has been observed when running 32bit executables on Windows64
       (i.e. through WOW64, the interface between the 32bits and 64bits
       worlds).  In that case, the inferior always does some strange
       unloading of unnamed dll.  */
    return;
  else
    {
      /* DLL has been found so remove the entry and free associated
	 resources.  */
      proc->all_dlls.erase (iter);
      proc->dlls_changed = true;
    }
}

void
clear_dlls (void)
{
  for_each_process ([] (process_info *proc)
    {
      proc->all_dlls.clear ();
    });
}
