/* Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

/* This file contains generic functions for writing ELF based core files.  */

#if !defined (GCORE_ELF_H)
#define GCORE_ELF_H 1

#include "gdb_bfd.h"
#include "gdbsupport/gdb_signals.h"
#include "gcore.h"

struct gdbarch;
struct thread_info;

/* Add content to *NOTE_DATA (and update *NOTE_SIZE) to describe the
   registers of thread INFO.  Report the thread as having stopped with
   STOP_SIGNAL.  The core file is being written to OBFD, and GDBARCH is the
   architecture for which the core file is being generated.  */

extern void gcore_elf_build_thread_register_notes
  (struct gdbarch *gdbarch, struct thread_info *info, gdb_signal stop_signal,
   bfd *obfd, gdb::unique_xmalloc_ptr<char> *note_data, int *note_size);

/* Add content to *NOTE_DATA (and update *NOTE_SIZE) to include a note
   containing the target description for GDBARCH.  The core file is
   being written to OBFD.  If something goes wrong then *NOTE_DATA can be
   set to nullptr.  */

extern void gcore_elf_make_tdesc_note
  (struct gdbarch *gdbarch, bfd *obfd,
   gdb::unique_xmalloc_ptr<char> *note_data, int *note_size);

#endif /* GCORE_ELF_H */
