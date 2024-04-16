/* Process record and replay target for GDB, the GNU debugger.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef RECORD_H
#define RECORD_H

#include "target/waitstatus.h"
#include "gdbsupport/enum-flags.h"

struct address_space;
struct cmd_list_element;
struct inferior;

extern unsigned int record_debug;

/* Allow record targets to add their own sub-commands.  */
extern struct cmd_list_element *record_cmdlist;
extern struct cmd_list_element *set_record_cmdlist;
extern struct cmd_list_element *show_record_cmdlist;
extern struct cmd_list_element *info_record_cmdlist;

/* Unwinders for some record targets.  */
extern const struct frame_unwind record_btrace_frame_unwind;
extern const struct frame_unwind record_btrace_tailcall_frame_unwind;

/* A list of different recording methods.  */
enum record_method
{
  /* No or unknown record method.  */
  RECORD_METHOD_NONE,

  /* Record method "full".  */
  RECORD_METHOD_FULL,

  /* Record method "btrace".  */
  RECORD_METHOD_BTRACE
};

/* A list of flags specifying what record target methods should print.  */
enum record_print_flag
{
  /* Print the source file and line (if applicable).  */
  RECORD_PRINT_SRC_LINE = (1 << 0),

  /* Print the instruction number range (if applicable).  */
  RECORD_PRINT_INSN_RANGE = (1 << 1),

  /* Indent based on call stack depth (if applicable).  */
  RECORD_PRINT_INDENT_CALLS = (1 << 2)
};
DEF_ENUM_FLAGS_TYPE (enum record_print_flag, record_print_flags);

/* Determined whether the target is stopped at a software or hardware
   breakpoint, based on PC and the breakpoint tables.  The breakpoint
   type is translated to the appropriate target_stop_reason and
   written to REASON.  Returns true if stopped at a breakpoint, false
   otherwise.  */

extern int
  record_check_stopped_by_breakpoint (const address_space *aspace,
				      CORE_ADDR pc,
				      enum target_stop_reason *reason);

/* Wrapper for target_read_memory that prints a debug message if
   reading memory fails.  */
extern int record_read_memory (struct gdbarch *gdbarch,
			       CORE_ADDR memaddr, gdb_byte *myaddr,
			       ssize_t len);

/* A wrapper for target_goto_record that parses ARG as a number.  */
extern void record_goto (const char *arg);

/* The default "to_disconnect" target method for record targets.  */
extern void record_disconnect (struct target_ops *, const char *, int);

/* The default "to_detach" target method for record targets.  */
extern void record_detach (struct target_ops *, inferior *, int);

/* The default "to_mourn_inferior" target method for record targets.  */
extern void record_mourn_inferior (struct target_ops *);

/* The default "to_kill" target method for record targets.  */
extern void record_kill (struct target_ops *);

/* Find the record_stratum target in the current target stack.
   Returns NULL if none is found.  */
extern struct target_ops *find_record_target (void);

/* This is to be called by record_stratum targets' open routine before
   it does anything.  */
extern void record_preopen (void);

/* Start recording with the given METHOD and FORMAT.  NULL means default method
   or format.  Throw on failure or invalid method / format.  */
extern void record_start (const char *method, const char *format,
			  int from_tty);

/* Stop recording.  Throw on failure.  */
extern void record_stop (int from_tty);

#endif /* RECORD_H */
