/* JIT declarations for GDB, the GNU Debugger.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef JIT_H
#define JIT_H

struct inferior;
struct objfile;
struct minimal_symbol;

/* When the JIT breakpoint fires, the inferior wants us to take one of
   these actions.  These values are used by the inferior, so the
   values of these enums cannot be changed.  */

enum jit_actions_t
{
  JIT_NOACTION = 0,
  JIT_REGISTER,
  JIT_UNREGISTER
};

/* This struct describes a single symbol file in a linked list of
   symbol files describing generated code.  As the inferior generates
   code, it adds these entries to the list, and when we attach to the
   inferior, we read them all.  For the first element prev_entry
   should be NULL, and for the last element next_entry should be
   NULL.  */

struct jit_code_entry
{
  CORE_ADDR next_entry;
  CORE_ADDR prev_entry;
  CORE_ADDR symfile_addr;
  ULONGEST symfile_size;
};

/* This is the global descriptor that the inferior uses to communicate
   information to the debugger.  To alert the debugger to take an
   action, the inferior sets the action_flag to the appropriate enum
   value, updates relevant_entry to point to the relevant code entry,
   and calls the function at the well-known symbol with our
   breakpoint.  We then read this descriptor from another global
   well-known symbol.  */

struct jit_descriptor
{
  uint32_t version;
  /* This should be jit_actions_t, but we want to be specific about the
     bit-width.  */
  uint32_t action_flag;
  CORE_ADDR relevant_entry;
  CORE_ADDR first_entry;
};

/* An objfile that defines the required symbols of the JIT interface has an
   instance of this type attached to it.  */

struct jiter_objfile_data
{
  ~jiter_objfile_data ();

  /* Symbol for __jit_debug_register_code.  */
  minimal_symbol *register_code = nullptr;

  /* Symbol for __jit_debug_descriptor.  */
  minimal_symbol *descriptor = nullptr;

  /* This is the relocated address of the __jit_debug_register_code function
     provided by this objfile.  This is used to detect relocations changes
     requiring the breakpoint to be re-created.  */
  CORE_ADDR cached_code_address = 0;

  /* This is the JIT event breakpoint, or nullptr if it has been deleted.  */
  breakpoint *jit_breakpoint = nullptr;
};

/* An objfile that is the product of JIT compilation and was registered
   using the JIT interface has an instance of this type attached to it.  */

struct jited_objfile_data
{
  jited_objfile_data (CORE_ADDR addr, CORE_ADDR symfile_addr,
		      ULONGEST symfile_size)
    : addr (addr),
      symfile_addr (symfile_addr),
      symfile_size (symfile_size)
  {}

  /* Address of struct jit_code_entry for this objfile.  */
  CORE_ADDR addr;

  /* Value of jit_code_entry->symfile_addr for this objfile.  */
  CORE_ADDR symfile_addr;

  /* Value of jit_code_entry->symfile_size for this objfile.  */
  ULONGEST symfile_size;
};

/* Re-establish the jit breakpoint(s).  */

extern void jit_breakpoint_re_set (void);

/* This function is called by handle_inferior_event when it decides
   that the JIT event breakpoint has fired.  JITER is the objfile
   whose JIT event breakpoint has been hit.  */

extern void jit_event_handler (gdbarch *gdbarch, objfile *jiter);

#endif /* JIT_H */
