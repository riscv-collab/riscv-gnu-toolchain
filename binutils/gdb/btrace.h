/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>.

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

#ifndef BTRACE_H
#define BTRACE_H

/* Branch tracing (btrace) is a per-thread control-flow execution trace of the
   inferior.  For presentation purposes, the branch trace is represented as a
   list of sequential control-flow blocks, one such list per thread.  */

#include "gdbsupport/btrace-common.h"
#include "target/waitstatus.h"
#include "gdbsupport/enum-flags.h"

#if defined (HAVE_LIBIPT)
#  include <intel-pt.h>
#endif

#include <vector>

struct thread_info;
struct btrace_function;

/* A coarse instruction classification.  */
enum btrace_insn_class
{
  /* The instruction is something not listed below.  */
  BTRACE_INSN_OTHER,

  /* The instruction is a function call.  */
  BTRACE_INSN_CALL,

  /* The instruction is a function return.  */
  BTRACE_INSN_RETURN,

  /* The instruction is an unconditional jump.  */
  BTRACE_INSN_JUMP
};

/* Instruction flags.  */
enum btrace_insn_flag
{
  /* The instruction has been executed speculatively.  */
  BTRACE_INSN_FLAG_SPECULATIVE = (1 << 0)
};
DEF_ENUM_FLAGS_TYPE (enum btrace_insn_flag, btrace_insn_flags);

/* A branch trace instruction.

   This represents a single instruction in a branch trace.  */
struct btrace_insn
{
  /* The address of this instruction.  */
  CORE_ADDR pc;

  /* The size of this instruction in bytes.  */
  gdb_byte size;

  /* The instruction class of this instruction.  */
  enum btrace_insn_class iclass;

  /* A bit vector of BTRACE_INSN_FLAGS.  */
  btrace_insn_flags flags;
};

/* Flags for btrace function segments.  */
enum btrace_function_flag
{
  /* The 'up' link interpretation.
     If set, it points to the function segment we returned to.
     If clear, it points to the function segment we called from.  */
  BFUN_UP_LINKS_TO_RET = (1 << 0),

  /* The 'up' link points to a tail call.  This obviously only makes sense
     if bfun_up_links_to_ret is clear.  */
  BFUN_UP_LINKS_TO_TAILCALL = (1 << 1)
};
DEF_ENUM_FLAGS_TYPE (enum btrace_function_flag, btrace_function_flags);

/* Decode errors for the BTS recording format.  */
enum btrace_bts_error
{
  /* The instruction trace overflowed the end of the trace block.  */
  BDE_BTS_OVERFLOW = 1,

  /* The instruction size could not be determined.  */
  BDE_BTS_INSN_SIZE
};

/* Decode errors for the Intel Processor Trace recording format.  */
enum btrace_pt_error
{
  /* The user cancelled trace processing.  */
  BDE_PT_USER_QUIT = 1,

  /* Tracing was temporarily disabled.  */
  BDE_PT_DISABLED,

  /* Trace recording overflowed.  */
  BDE_PT_OVERFLOW

  /* Negative numbers are used by the decoder library.  */
};

/* A branch trace function segment.

   This represents a function segment in a branch trace, i.e. a consecutive
   number of instructions belonging to the same function.

   In case of decode errors, we add an empty function segment to indicate
   the gap in the trace.

   We do not allow function segments without instructions otherwise.  */
struct btrace_function
{
  btrace_function (struct minimal_symbol *msym_, struct symbol *sym_,
		   unsigned int number_, unsigned int insn_offset_, int level_)
    : msym (msym_), sym (sym_), insn_offset (insn_offset_), number (number_),
      level (level_)
  {
  }

  /* The full and minimal symbol for the function.  Both may be NULL.  */
  struct minimal_symbol *msym;
  struct symbol *sym;

  /* The function segment numbers of the previous and next segment belonging to
     the same function.  If a function calls another function, the former will
     have at least two segments: one before the call and another after the
     return.  Will be zero if there is no such function segment.  */
  unsigned int prev = 0;
  unsigned int next = 0;

  /* The function segment number of the directly preceding function segment in
     a (fake) call stack.  Will be zero if there is no such function segment in
     the record.  */
  unsigned int up = 0;

  /* The instructions in this function segment.
     The instruction vector will be empty if the function segment
     represents a decode error.  */
  std::vector<btrace_insn> insn;

  /* The error code of a decode error that led to a gap.
     Must be zero unless INSN is empty; non-zero otherwise.  */
  int errcode = 0;

  /* The instruction number offset for the first instruction in this
     function segment.
     If INSN is empty this is the insn_offset of the succeding function
     segment in control-flow order.  */
  unsigned int insn_offset;

  /* The 1-based function number in control-flow order.
     If INSN is empty indicating a gap in the trace due to a decode error,
     we still count the gap as a function.  */
  unsigned int number;

  /* The function level in a back trace across the entire branch trace.
     A caller's level is one lower than the level of its callee.

     Levels can be negative if we see returns for which we have not seen
     the corresponding calls.  The branch trace thread information provides
     a fixup to normalize function levels so the smallest level is zero.  */
  int level;

  /* A bit-vector of btrace_function_flag.  */
  btrace_function_flags flags = 0;
};

/* A branch trace instruction iterator.  */
struct btrace_insn_iterator
{
  /* The branch trace information for this thread.  Will never be NULL.  */
  const struct btrace_thread_info *btinfo;

  /* The index of the function segment in BTINFO->FUNCTIONS.  */
  unsigned int call_index;

  /* The index into the function segment's instruction vector.  */
  unsigned int insn_index;
};

/* A branch trace function call iterator.  */
struct btrace_call_iterator
{
  /* The branch trace information for this thread.  Will never be NULL.  */
  const struct btrace_thread_info *btinfo;

  /* The index of the function segment in BTINFO->FUNCTIONS.  */
  unsigned int index;
};

/* Branch trace iteration state for "record instruction-history".  */
struct btrace_insn_history
{
  /* The branch trace instruction range from BEGIN (inclusive) to
     END (exclusive) that has been covered last time.  */
  struct btrace_insn_iterator begin;
  struct btrace_insn_iterator end;
};

/* Branch trace iteration state for "record function-call-history".  */
struct btrace_call_history
{
  /* The branch trace function range from BEGIN (inclusive) to END (exclusive)
     that has been covered last time.  */
  struct btrace_call_iterator begin;
  struct btrace_call_iterator end;
};

/* Branch trace thread flags.  */
enum btrace_thread_flag : unsigned
{
  /* The thread is to be stepped forwards.  */
  BTHR_STEP = (1 << 0),

  /* The thread is to be stepped backwards.  */
  BTHR_RSTEP = (1 << 1),

  /* The thread is to be continued forwards.  */
  BTHR_CONT = (1 << 2),

  /* The thread is to be continued backwards.  */
  BTHR_RCONT = (1 << 3),

  /* The thread is to be moved.  */
  BTHR_MOVE = (BTHR_STEP | BTHR_RSTEP | BTHR_CONT | BTHR_RCONT),

  /* The thread is to be stopped.  */
  BTHR_STOP = (1 << 4)
};
DEF_ENUM_FLAGS_TYPE (enum btrace_thread_flag, btrace_thread_flags);

#if defined (HAVE_LIBIPT)
/* A packet.  */
struct btrace_pt_packet
{
  /* The offset in the trace stream.  */
  uint64_t offset;

  /* The decode error code.  */
  enum pt_error_code errcode;

  /* The decoded packet.  Only valid if ERRCODE == pte_ok.  */
  struct pt_packet packet;
};

#endif /* defined (HAVE_LIBIPT)  */

/* Branch trace iteration state for "maintenance btrace packet-history".  */
struct btrace_maint_packet_history
{
  /* The branch trace packet range from BEGIN (inclusive) to
     END (exclusive) that has been covered last time.  */
  unsigned int begin;
  unsigned int end;
};

/* Branch trace maintenance information per thread.

   This information is used by "maintenance btrace" commands.  */
struct btrace_maint_info
{
  /* Most information is format-specific.
     The format can be found in the BTRACE.DATA.FORMAT field of each thread.  */
  union
  {
    /* BTRACE.DATA.FORMAT == BTRACE_FORMAT_BTS  */
    struct
    {
      /* The packet history iterator.
	 We are iterating over BTRACE.DATA.FORMAT.VARIANT.BTS.BLOCKS.  */
      struct btrace_maint_packet_history packet_history;
    } bts;

#if defined (HAVE_LIBIPT)
    /* BTRACE.DATA.FORMAT == BTRACE_FORMAT_PT  */
    struct
    {
      /* A vector of decoded packets.  */
      std::vector<btrace_pt_packet> *packets;

      /* The packet history iterator.
	 We are iterating over the above PACKETS vector.  */
      struct btrace_maint_packet_history packet_history;
    } pt;
#endif /* defined (HAVE_LIBIPT)  */
  } variant;
};

/* Branch trace information per thread.

   This represents the branch trace configuration as well as the entry point
   into the branch trace data.  For the latter, it also contains the index into
   an array of branch trace blocks used for iterating though the branch trace
   blocks of a thread.  */
struct btrace_thread_info
{
  /* The target branch trace information for this thread.

     This contains the branch trace configuration as well as any
     target-specific information necessary for implementing branch tracing on
     the underlying architecture.  */
  struct btrace_target_info *target;

  /* The raw branch trace data for the below branch trace.  */
  struct btrace_data data;

  /* Vector of decoded function segments in execution flow order.
     Note that the numbering for btrace function segments starts with 1, so
     function segment i will be at index (i - 1).  */
  std::vector<btrace_function> functions;

  /* The function level offset.  When added to each function's LEVEL,
     this normalizes the function levels such that the smallest level
     becomes zero.  */
  int level;

  /* The number of gaps in the trace.  */
  unsigned int ngaps;

  /* A bit-vector of btrace_thread_flag.  */
  btrace_thread_flags flags;

  /* The instruction history iterator.  */
  struct btrace_insn_history *insn_history;

  /* The function call history iterator.  */
  struct btrace_call_history *call_history;

  /* The current replay position.  NULL if not replaying.
     Gaps are skipped during replay, so REPLAY always points to a valid
     instruction.  */
  struct btrace_insn_iterator *replay;

  /* Why the thread stopped, if we need to track it.  */
  enum target_stop_reason stop_reason;

  /* Maintenance information.  */
  struct btrace_maint_info maint;
};

/* Enable branch tracing for a thread.  */
extern void btrace_enable (struct thread_info *tp,
			   const struct btrace_config *conf);

/* Get the branch trace configuration for a thread.
   Return NULL if branch tracing is not enabled for that thread.  */
extern const struct btrace_config *
  btrace_conf (const struct btrace_thread_info *);

/* Disable branch tracing for a thread.
   This will also delete the current branch trace data.  */
extern void btrace_disable (struct thread_info *);

/* Disable branch tracing for a thread during teardown.
   This is similar to btrace_disable, except that it will use
   target_teardown_btrace instead of target_disable_btrace.  */
extern void btrace_teardown (struct thread_info *);

/* Return a human readable error string for the given ERRCODE in FORMAT.
   The pointer will never be NULL and must not be freed.  */

extern const char *btrace_decode_error (enum btrace_format format, int errcode);

/* Fetch the branch trace for a single thread.  If CPU is not NULL, assume
   CPU for trace decode.  */
extern void btrace_fetch (struct thread_info *,
			  const struct btrace_cpu *cpu);

/* Clear the branch trace for a single thread.  */
extern void btrace_clear (struct thread_info *);

/* Clear the branch trace for all threads when an object file goes away.  */
extern void btrace_free_objfile (struct objfile *);

/* Dereference a branch trace instruction iterator.  Return a pointer to the
   instruction the iterator points to.
   May return NULL if the iterator points to a gap in the trace.  */
extern const struct btrace_insn *
  btrace_insn_get (const struct btrace_insn_iterator *);

/* Return the error code for a branch trace instruction iterator.  Returns zero
   if there is no error, i.e. the instruction is valid.  */
extern int btrace_insn_get_error (const struct btrace_insn_iterator *);

/* Return the instruction number for a branch trace iterator.
   Returns one past the maximum instruction number for the end iterator.  */
extern unsigned int btrace_insn_number (const struct btrace_insn_iterator *);

/* Initialize a branch trace instruction iterator to point to the begin/end of
   the branch trace.  Throws an error if there is no branch trace.  */
extern void btrace_insn_begin (struct btrace_insn_iterator *,
			       const struct btrace_thread_info *);
extern void btrace_insn_end (struct btrace_insn_iterator *,
			     const struct btrace_thread_info *);

/* Increment/decrement a branch trace instruction iterator by at most STRIDE
   instructions.  Return the number of instructions by which the instruction
   iterator has been advanced.
   Returns zero, if the operation failed or STRIDE had been zero.  */
extern unsigned int btrace_insn_next (struct btrace_insn_iterator *,
				      unsigned int stride);
extern unsigned int btrace_insn_prev (struct btrace_insn_iterator *,
				      unsigned int stride);

/* Compare two branch trace instruction iterators.
   Return a negative number if LHS < RHS.
   Return zero if LHS == RHS.
   Return a positive number if LHS > RHS.  */
extern int btrace_insn_cmp (const struct btrace_insn_iterator *lhs,
			    const struct btrace_insn_iterator *rhs);

/* Find an instruction or gap in the function branch trace by its number.
   If the instruction is found, initialize the branch trace instruction
   iterator to point to this instruction and return non-zero.
   Return zero otherwise.  */
extern int btrace_find_insn_by_number (struct btrace_insn_iterator *,
				       const struct btrace_thread_info *,
				       unsigned int number);

/* Dereference a branch trace call iterator.  Return a pointer to the
   function the iterator points to or NULL if the iterator points past
   the end of the branch trace.  */
extern const struct btrace_function *
  btrace_call_get (const struct btrace_call_iterator *);

/* Return the function number for a branch trace call iterator.
   Returns one past the maximum function number for the end iterator.
   Returns zero if the iterator does not point to a valid function.  */
extern unsigned int btrace_call_number (const struct btrace_call_iterator *);

/* Initialize a branch trace call iterator to point to the begin/end of
   the branch trace.  Throws an error if there is no branch trace.  */
extern void btrace_call_begin (struct btrace_call_iterator *,
			       const struct btrace_thread_info *);
extern void btrace_call_end (struct btrace_call_iterator *,
			     const struct btrace_thread_info *);

/* Increment/decrement a branch trace call iterator by at most STRIDE function
   segments.  Return the number of function segments by which the call
   iterator has been advanced.
   Returns zero, if the operation failed or STRIDE had been zero.  */
extern unsigned int btrace_call_next (struct btrace_call_iterator *,
				      unsigned int stride);
extern unsigned int btrace_call_prev (struct btrace_call_iterator *,
				      unsigned int stride);

/* Compare two branch trace call iterators.
   Return a negative number if LHS < RHS.
   Return zero if LHS == RHS.
   Return a positive number if LHS > RHS.  */
extern int btrace_call_cmp (const struct btrace_call_iterator *lhs,
			    const struct btrace_call_iterator *rhs);

/* Find a function in the function branch trace by its NUMBER.
   If the function is found, initialize the branch trace call
   iterator to point to this function and return non-zero.
   Return zero otherwise.  */
extern int btrace_find_call_by_number (struct btrace_call_iterator *,
				       const struct btrace_thread_info *,
				       unsigned int number);

/* Set the branch trace instruction history from BEGIN (inclusive) to
   END (exclusive).  */
extern void btrace_set_insn_history (struct btrace_thread_info *,
				     const struct btrace_insn_iterator *begin,
				     const struct btrace_insn_iterator *end);

/* Set the branch trace function call history from BEGIN (inclusive) to
   END (exclusive).  */
extern void btrace_set_call_history (struct btrace_thread_info *,
				     const struct btrace_call_iterator *begin,
				     const struct btrace_call_iterator *end);

/* Determine if branch tracing is currently replaying TP.  */
extern int btrace_is_replaying (struct thread_info *tp);

/* Return non-zero if the branch trace for TP is empty; zero otherwise.  */
extern int btrace_is_empty (struct thread_info *tp);

#endif /* BTRACE_H */
