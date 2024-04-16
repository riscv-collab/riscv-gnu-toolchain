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


#ifndef PROGSPACE_AND_THREAD_H
#define PROGSPACE_AND_THREAD_H

#include "progspace.h"
#include "gdbthread.h"

/* Save/restore the current program space, thread, inferior and frame.
   Use this when you need to call
   switch_to_program_space_and_thread.  */

class scoped_restore_current_pspace_and_thread
{
  scoped_restore_current_program_space m_restore_pspace;
  scoped_restore_current_thread m_restore_thread;
};

/* Switches full context to program space PSPACE.  Switches to the
   first thread found bound to PSPACE, giving preference to the
   current thread, if there's one and it isn't executing.  */
void switch_to_program_space_and_thread (program_space *pspace);

#endif
