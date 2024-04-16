/* Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "progspace-and-thread.h"
#include "inferior.h"

/* See progspace-and-thread.h  */

void
switch_to_program_space_and_thread (program_space *pspace)
{
  inferior *inf = find_inferior_for_program_space (pspace);
  gdb_assert (inf != nullptr);

  if (inf->pid != 0)
    {
      thread_info *tp = any_live_thread_of_inferior (inf);

      if (tp != NULL)
	{
	  switch_to_thread (tp);
	  /* Switching thread switches pspace implicitly.  We're
	     done.  */
	  return;
	}
    }

  switch_to_inferior_no_thread (inf);
}
