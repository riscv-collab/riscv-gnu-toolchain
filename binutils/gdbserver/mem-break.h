/* Memory breakpoint interfaces for the remote server for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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

#ifndef GDBSERVER_MEM_BREAK_H
#define GDBSERVER_MEM_BREAK_H

#include "gdbsupport/break-common.h"

/* Breakpoints are opaque.  */
struct breakpoint;
struct gdb_breakpoint;
struct fast_tracepoint_jump;
struct raw_breakpoint;
struct process_info;

#define Z_PACKET_SW_BP '0'
#define Z_PACKET_HW_BP '1'
#define Z_PACKET_WRITE_WP '2'
#define Z_PACKET_READ_WP '3'
#define Z_PACKET_ACCESS_WP '4'

/* The low level breakpoint types.  */

enum raw_bkpt_type
  {
    /* Software/memory breakpoint.  */
    raw_bkpt_type_sw,

    /* Hardware-assisted breakpoint.  */
    raw_bkpt_type_hw,

    /* Hardware-assisted write watchpoint.  */
    raw_bkpt_type_write_wp,

    /* Hardware-assisted read watchpoint.  */
    raw_bkpt_type_read_wp,

    /* Hardware-assisted access watchpoint.  */
    raw_bkpt_type_access_wp
  };

/* Map the protocol breakpoint/watchpoint type Z_TYPE to the internal
   raw breakpoint type.  */

enum raw_bkpt_type Z_packet_to_raw_bkpt_type (char z_type);

/* Map a raw breakpoint type to an enum target_hw_bp_type.  */

enum target_hw_bp_type raw_bkpt_type_to_target_hw_bp_type
  (enum raw_bkpt_type raw_type);

/* Create a new GDB breakpoint of type Z_TYPE at ADDR with kind KIND.
   Returns a pointer to the newly created breakpoint on success.  On
   failure returns NULL and sets *ERR to either -1 for error, or 1 if
   Z_TYPE breakpoints are not supported on this target.  */

struct gdb_breakpoint *set_gdb_breakpoint (char z_type, CORE_ADDR addr,
					   int kind, int *err);

/* Delete a GDB breakpoint of type Z_TYPE and kind KIND previously
   inserted at ADDR with set_gdb_breakpoint_at.  Returns 0 on success,
   -1 on error, and 1 if Z_TYPE breakpoints are not supported on this
   target.  */

int delete_gdb_breakpoint (char z_type, CORE_ADDR addr, int kind);

/* Returns TRUE if there's a software or hardware (code) breakpoint at
   ADDR in our tables, inserted, or not.  */

int breakpoint_here (CORE_ADDR addr);

/* Returns TRUE if there's any inserted software or hardware (code)
   breakpoint set at ADDR.  */

int breakpoint_inserted_here (CORE_ADDR addr);

/* Returns TRUE if there's any inserted software breakpoint at
   ADDR.  */

int software_breakpoint_inserted_here (CORE_ADDR addr);

/* Returns TRUE if there's any inserted hardware (code) breakpoint at
   ADDR.  */

int hardware_breakpoint_inserted_here (CORE_ADDR addr);

/* Returns TRUE if there's any single-step breakpoint at ADDR.  */

int single_step_breakpoint_inserted_here (CORE_ADDR addr);

/* Clear all breakpoint conditions and commands associated with a
   breakpoint.  */

void clear_breakpoint_conditions_and_commands (struct gdb_breakpoint *bp);

/* Set target-side condition CONDITION to the breakpoint at ADDR.
   Returns false on failure.  On success, advances CONDITION pointer
   past the condition and returns true.  */

int add_breakpoint_condition (struct gdb_breakpoint *bp,
			      const char **condition);

/* Set target-side commands COMMANDS to the breakpoint at ADDR.
   Returns false on failure.  On success, advances COMMANDS past the
   commands and returns true.  If PERSIST, the commands should run
   even while GDB is disconnected.  */

int add_breakpoint_commands (struct gdb_breakpoint *bp, const char **commands,
			     int persist);

/* Return true if PROC has any persistent command.  */
bool any_persistent_commands (process_info *proc);

/* Evaluation condition (if any) at breakpoint BP.  Return 1 if
   true and 0 otherwise.  */

int gdb_condition_true_at_breakpoint (CORE_ADDR where);

int gdb_no_commands_at_breakpoint (CORE_ADDR where);

void run_breakpoint_commands (CORE_ADDR where);

/* Returns TRUE if there's a GDB breakpoint (Z0 or Z1) set at
   WHERE.  */

int gdb_breakpoint_here (CORE_ADDR where);

/* Create a new breakpoint at WHERE, and call HANDLER when
   it is hit.  HANDLER should return 1 if the breakpoint
   should be deleted, 0 otherwise.  The type of the created
   breakpoint is other_breakpoint.  */

struct breakpoint *set_breakpoint_at (CORE_ADDR where,
				      int (*handler) (CORE_ADDR));

/* Delete a breakpoint.  */

int delete_breakpoint (struct breakpoint *bkpt);

/* Set a single-step breakpoint at STOP_AT for thread represented by
   PTID.  */

void set_single_step_breakpoint (CORE_ADDR stop_at, ptid_t ptid);

/* Delete all single-step breakpoints of THREAD.  */

void delete_single_step_breakpoints (struct thread_info *thread);

/* Reinsert all single-step breakpoints of THREAD.  */

void reinsert_single_step_breakpoints (struct thread_info *thread);

/* Uninsert all single-step breakpoints of THREAD.  This still leaves
   the single-step breakpoints in the table.  */

void uninsert_single_step_breakpoints (struct thread_info *thread);

/* Reinsert breakpoints at WHERE (and change their status to
   inserted).  */

void reinsert_breakpoints_at (CORE_ADDR where);

/* The THREAD has single-step breakpoints or not.  */

int has_single_step_breakpoints (struct thread_info *thread);

/* Uninsert breakpoints at WHERE (and change their status to
   uninserted).  This still leaves the breakpoints in the table.  */

void uninsert_breakpoints_at (CORE_ADDR where);

/* Reinsert all breakpoints of the current process (and change their
   status to inserted).  */

void reinsert_all_breakpoints (void);

/* Uninsert all breakpoints of the current process (and change their
   status to uninserted).  This still leaves the breakpoints in the
   table.  */

void uninsert_all_breakpoints (void);

/* See if any breakpoint claims ownership of STOP_PC.  Call the handler for
   the breakpoint, if found.  */

void check_breakpoints (CORE_ADDR stop_pc);

/* See if any breakpoints shadow the target memory area from MEM_ADDR
   to MEM_ADDR + MEM_LEN.  Update the data already read from the target
   (in BUF) if necessary.  */

void check_mem_read (CORE_ADDR mem_addr, unsigned char *buf, int mem_len);

/* See if any breakpoints shadow the target memory area from MEM_ADDR
   to MEM_ADDR + MEM_LEN.  Update the data to be written to the target
   (in BUF, a copy of MYADDR on entry) if necessary, as well as the
   original data for any breakpoints.  */

void check_mem_write (CORE_ADDR mem_addr,
		      unsigned char *buf, const unsigned char *myaddr, int mem_len);

/* Delete all breakpoints.  */

void delete_all_breakpoints (void);

/* Clear the "inserted" flag in all breakpoints of PROC.  */

void mark_breakpoints_out (struct process_info *proc);

/* Delete all breakpoints, but do not try to un-insert them from the
   inferior.  */

void free_all_breakpoints (struct process_info *proc);

/* Check if breakpoints still seem to be inserted in the inferior.  */

void validate_breakpoints (void);

/* Insert a fast tracepoint jump at WHERE, using instruction INSN, of
   LENGTH bytes.  */

struct fast_tracepoint_jump *set_fast_tracepoint_jump (CORE_ADDR where,
						       unsigned char *insn,
						       ULONGEST length);

/* Increment reference counter of JP.  */
void inc_ref_fast_tracepoint_jump (struct fast_tracepoint_jump *jp);

/* Delete fast tracepoint jump TODEL from our tables, and uninsert if
   from memory.  */

int delete_fast_tracepoint_jump (struct fast_tracepoint_jump *todel);

/* Returns true if there's fast tracepoint jump set at WHERE.  */

int fast_tracepoint_jump_here (CORE_ADDR);

/* Uninsert fast tracepoint jumps at WHERE (and change their status to
   uninserted).  This still leaves the tracepoints in the table.  */

void uninsert_fast_tracepoint_jumps_at (CORE_ADDR pc);

/* Reinsert fast tracepoint jumps at WHERE (and change their status to
   inserted).  */

void reinsert_fast_tracepoint_jumps_at (CORE_ADDR where);

/* Insert a memory breakpoint.  */

int insert_memory_breakpoint (struct raw_breakpoint *bp);

/* Remove a previously inserted memory breakpoint.  */

int remove_memory_breakpoint (struct raw_breakpoint *bp);

/* Create a new breakpoint list in CHILD_THREAD's process that is a
   copy of breakpoint list in PARENT_THREAD's process.  */

void clone_all_breakpoints (struct thread_info *child_thread,
			    const struct thread_info *parent_thread);

#endif /* GDBSERVER_MEM_BREAK_H */
