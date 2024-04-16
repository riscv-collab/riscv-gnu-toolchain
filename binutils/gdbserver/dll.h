/* Copyright (C) 1993-2024 Free Software Foundation, Inc.

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

#ifndef GDBSERVER_DLL_H
#define GDBSERVER_DLL_H

#include <list>

struct process_info;

struct dll_info
{
  dll_info (const std::string &name_, CORE_ADDR base_addr_)
  : name (name_), base_addr (base_addr_)
  {}

  std::string name;
  CORE_ADDR base_addr;
};

extern void clear_dlls (void);
extern void loaded_dll (const char *name, CORE_ADDR base_addr);
extern void loaded_dll (process_info *proc, const char *name,
			CORE_ADDR base_addr);
extern void unloaded_dll (const char *name, CORE_ADDR base_addr);
extern void unloaded_dll (process_info *proc, const char *name,
			  CORE_ADDR base_addr);

#endif /* GDBSERVER_DLL_H */
