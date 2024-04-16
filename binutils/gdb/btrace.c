/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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
#include "btrace.h"
#include "gdbthread.h"
#include "inferior.h"
#include "target.h"
#include "record.h"
#include "symtab.h"
#include "disasm.h"
#include "source.h"
#include "filenames.h"
#include "regcache.h"
#include "gdbsupport/rsp-low.h"
#include "gdbcmd.h"
#include "cli/cli-utils.h"
#include "gdbarch.h"

/* For maintenance commands.  */
#include "record-btrace.h"

#include <inttypes.h>
#include <ctype.h>
#include <algorithm>

/* Command lists for btrace maintenance commands.  */
static struct cmd_list_element *maint_btrace_cmdlist;
static struct cmd_list_element *maint_btrace_set_cmdlist;
static struct cmd_list_element *maint_btrace_show_cmdlist;
static struct cmd_list_element *maint_btrace_pt_set_cmdlist;
static struct cmd_list_element *maint_btrace_pt_show_cmdlist;

/* Control whether to skip PAD packets when computing the packet history.  */
static bool maint_btrace_pt_skip_pad = true;

static void btrace_add_pc (struct thread_info *tp);

/* Print a record debug message.  Use do ... while (0) to avoid ambiguities
   when used in if statements.  */

#define DEBUG(msg, args...)						\
  do									\
    {									\
      if (record_debug != 0)						\
	gdb_printf (gdb_stdlog,						\
		    "[btrace] " msg "\n", ##args);			\
    }									\
  while (0)

#define DEBUG_FTRACE(msg, args...) DEBUG ("[ftrace] " msg, ##args)

/* Return the function name of a recorded function segment for printing.
   This function never returns NULL.  */

static const char *
ftrace_print_function_name (const struct btrace_function *bfun)
{
  struct minimal_symbol *msym;
  struct symbol *sym;

  msym = bfun->msym;
  sym = bfun->sym;

  if (sym != NULL)
    return sym->print_name ();

  if (msym != NULL)
    return msym->print_name ();

  return "<unknown>";
}

/* Return the file name of a recorded function segment for printing.
   This function never returns NULL.  */

static const char *
ftrace_print_filename (const struct btrace_function *bfun)
{
  struct symbol *sym;
  const char *filename;

  sym = bfun->sym;

  if (sym != NULL)
    filename = symtab_to_filename_for_display (sym->symtab ());
  else
    filename = "<unknown>";

  return filename;
}

/* Return a string representation of the address of an instruction.
   This function never returns NULL.  */

static const char *
ftrace_print_insn_addr (const struct btrace_insn *insn)
{
  if (insn == NULL)
    return "<nil>";

  return core_addr_to_string_nz (insn->pc);
}

/* Print an ftrace debug status message.  */

static void
ftrace_debug (const struct btrace_function *bfun, const char *prefix)
{
  const char *fun, *file;
  unsigned int ibegin, iend;
  int level;

  fun = ftrace_print_function_name (bfun);
  file = ftrace_print_filename (bfun);
  level = bfun->level;

  ibegin = bfun->insn_offset;
  iend = ibegin + bfun->insn.size ();

  DEBUG_FTRACE ("%s: fun = %s, file = %s, level = %d, insn = [%u; %u)",
		prefix, fun, file, level, ibegin, iend);
}

/* Return the number of instructions in a given function call segment.  */

static unsigned int
ftrace_call_num_insn (const struct btrace_function* bfun)
{
  if (bfun == NULL)
    return 0;

  /* A gap is always counted as one instruction.  */
  if (bfun->errcode != 0)
    return 1;

  return bfun->insn.size ();
}

/* Return the function segment with the given NUMBER or NULL if no such segment
   exists.  BTINFO is the branch trace information for the current thread.  */

static struct btrace_function *
ftrace_find_call_by_number (struct btrace_thread_info *btinfo,
			    unsigned int number)
{
  if (number == 0 || number > btinfo->functions.size ())
    return NULL;

  return &btinfo->functions[number - 1];
}

/* A const version of the function above.  */

static const struct btrace_function *
ftrace_find_call_by_number (const struct btrace_thread_info *btinfo,
			    unsigned int number)
{
  if (number == 0 || number > btinfo->functions.size ())
    return NULL;

  return &btinfo->functions[number - 1];
}

/* Return non-zero if BFUN does not match MFUN and FUN,
   return zero otherwise.  */

static int
ftrace_function_switched (const struct btrace_function *bfun,
			  const struct minimal_symbol *mfun,
			  const struct symbol *fun)
{
  struct minimal_symbol *msym;
  struct symbol *sym;

  msym = bfun->msym;
  sym = bfun->sym;

  /* If the minimal symbol changed, we certainly switched functions.  */
  if (mfun != NULL && msym != NULL
      && strcmp (mfun->linkage_name (), msym->linkage_name ()) != 0)
    return 1;

  /* If the symbol changed, we certainly switched functions.  */
  if (fun != NULL && sym != NULL)
    {
      const char *bfname, *fname;

      /* Check the function name.  */
      if (strcmp (fun->linkage_name (), sym->linkage_name ()) != 0)
	return 1;

      /* Check the location of those functions, as well.  */
      bfname = symtab_to_fullname (sym->symtab ());
      fname = symtab_to_fullname (fun->symtab ());
      if (filename_cmp (fname, bfname) != 0)
	return 1;
    }

  /* If we lost symbol information, we switched functions.  */
  if (!(msym == NULL && sym == NULL) && mfun == NULL && fun == NULL)
    return 1;

  /* If we gained symbol information, we switched functions.  */
  if (msym == NULL && sym == NULL && !(mfun == NULL && fun == NULL))
    return 1;

  return 0;
}

/* Allocate and initialize a new branch trace function segment at the end of
   the trace.
   BTINFO is the branch trace information for the current thread.
   MFUN and FUN are the symbol information we have for this function.
   This invalidates all struct btrace_function pointer currently held.  */

static struct btrace_function *
ftrace_new_function (struct btrace_thread_info *btinfo,
		     struct minimal_symbol *mfun,
		     struct symbol *fun)
{
  int level;
  unsigned int number, insn_offset;

  if (btinfo->functions.empty ())
    {
      /* Start counting NUMBER and INSN_OFFSET at one.  */
      level = 0;
      number = 1;
      insn_offset = 1;
    }
  else
    {
      const struct btrace_function *prev = &btinfo->functions.back ();
      level = prev->level;
      number = prev->number + 1;
      insn_offset = prev->insn_offset + ftrace_call_num_insn (prev);
    }

  btinfo->functions.emplace_back (mfun, fun, number, insn_offset, level);
  return &btinfo->functions.back ();
}

/* Update the UP field of a function segment.  */

static void
ftrace_update_caller (struct btrace_function *bfun,
		      struct btrace_function *caller,
		      btrace_function_flags flags)
{
  if (bfun->up != 0)
    ftrace_debug (bfun, "updating caller");

  bfun->up = caller->number;
  bfun->flags = flags;

  ftrace_debug (bfun, "set caller");
  ftrace_debug (caller, "..to");
}

/* Fix up the caller for all segments of a function.  */

static void
ftrace_fixup_caller (struct btrace_thread_info *btinfo,
		     struct btrace_function *bfun,
		     struct btrace_function *caller,
		     btrace_function_flags flags)
{
  unsigned int prev, next;

  prev = bfun->prev;
  next = bfun->next;
  ftrace_update_caller (bfun, caller, flags);

  /* Update all function segments belonging to the same function.  */
  for (; prev != 0; prev = bfun->prev)
    {
      bfun = ftrace_find_call_by_number (btinfo, prev);
      ftrace_update_caller (bfun, caller, flags);
    }

  for (; next != 0; next = bfun->next)
    {
      bfun = ftrace_find_call_by_number (btinfo, next);
      ftrace_update_caller (bfun, caller, flags);
    }
}

/* Add a new function segment for a call at the end of the trace.
   BTINFO is the branch trace information for the current thread.
   MFUN and FUN are the symbol information we have for this function.  */

static struct btrace_function *
ftrace_new_call (struct btrace_thread_info *btinfo,
		 struct minimal_symbol *mfun,
		 struct symbol *fun)
{
  const unsigned int length = btinfo->functions.size ();
  struct btrace_function *bfun = ftrace_new_function (btinfo, mfun, fun);

  bfun->up = length;
  bfun->level += 1;

  ftrace_debug (bfun, "new call");

  return bfun;
}

/* Add a new function segment for a tail call at the end of the trace.
   BTINFO is the branch trace information for the current thread.
   MFUN and FUN are the symbol information we have for this function.  */

static struct btrace_function *
ftrace_new_tailcall (struct btrace_thread_info *btinfo,
		     struct minimal_symbol *mfun,
		     struct symbol *fun)
{
  const unsigned int length = btinfo->functions.size ();
  struct btrace_function *bfun = ftrace_new_function (btinfo, mfun, fun);

  bfun->up = length;
  bfun->level += 1;
  bfun->flags |= BFUN_UP_LINKS_TO_TAILCALL;

  ftrace_debug (bfun, "new tail call");

  return bfun;
}

/* Return the caller of BFUN or NULL if there is none.  This function skips
   tail calls in the call chain.  BTINFO is the branch trace information for
   the current thread.  */
static struct btrace_function *
ftrace_get_caller (struct btrace_thread_info *btinfo,
		   struct btrace_function *bfun)
{
  for (; bfun != NULL; bfun = ftrace_find_call_by_number (btinfo, bfun->up))
    if ((bfun->flags & BFUN_UP_LINKS_TO_TAILCALL) == 0)
      return ftrace_find_call_by_number (btinfo, bfun->up);

  return NULL;
}

/* Find the innermost caller in the back trace of BFUN with MFUN/FUN
   symbol information.  BTINFO is the branch trace information for the current
   thread.  */

static struct btrace_function *
ftrace_find_caller (struct btrace_thread_info *btinfo,
		    struct btrace_function *bfun,
		    struct minimal_symbol *mfun,
		    struct symbol *fun)
{
  for (; bfun != NULL; bfun = ftrace_find_call_by_number (btinfo, bfun->up))
    {
      /* Skip functions with incompatible symbol information.  */
      if (ftrace_function_switched (bfun, mfun, fun))
	continue;

      /* This is the function segment we're looking for.  */
      break;
    }

  return bfun;
}

/* Find the innermost caller in the back trace of BFUN, skipping all
   function segments that do not end with a call instruction (e.g.
   tail calls ending with a jump).  BTINFO is the branch trace information for
   the current thread.  */

static struct btrace_function *
ftrace_find_call (struct btrace_thread_info *btinfo,
		  struct btrace_function *bfun)
{
  for (; bfun != NULL; bfun = ftrace_find_call_by_number (btinfo, bfun->up))
    {
      /* Skip gaps.  */
      if (bfun->errcode != 0)
	continue;

      btrace_insn &last = bfun->insn.back ();

      if (last.iclass == BTRACE_INSN_CALL)
	break;
    }

  return bfun;
}

/* Add a continuation segment for a function into which we return at the end of
   the trace.
   BTINFO is the branch trace information for the current thread.
   MFUN and FUN are the symbol information we have for this function.  */

static struct btrace_function *
ftrace_new_return (struct btrace_thread_info *btinfo,
		   struct minimal_symbol *mfun,
		   struct symbol *fun)
{
  struct btrace_function *prev, *bfun, *caller;

  bfun = ftrace_new_function (btinfo, mfun, fun);
  prev = ftrace_find_call_by_number (btinfo, bfun->number - 1);

  /* It is important to start at PREV's caller.  Otherwise, we might find
     PREV itself, if PREV is a recursive function.  */
  caller = ftrace_find_call_by_number (btinfo, prev->up);
  caller = ftrace_find_caller (btinfo, caller, mfun, fun);
  if (caller != NULL)
    {
      /* The caller of PREV is the preceding btrace function segment in this
	 function instance.  */
      gdb_assert (caller->next == 0);

      caller->next = bfun->number;
      bfun->prev = caller->number;

      /* Maintain the function level.  */
      bfun->level = caller->level;

      /* Maintain the call stack.  */
      bfun->up = caller->up;
      bfun->flags = caller->flags;

      ftrace_debug (bfun, "new return");
    }
  else
    {
      /* We did not find a caller.  This could mean that something went
	 wrong or that the call is simply not included in the trace.  */

      /* Let's search for some actual call.  */
      caller = ftrace_find_call_by_number (btinfo, prev->up);
      caller = ftrace_find_call (btinfo, caller);
      if (caller == NULL)
	{
	  /* There is no call in PREV's back trace.  We assume that the
	     branch trace did not include it.  */

	  /* Let's find the topmost function and add a new caller for it.
	     This should handle a series of initial tail calls.  */
	  while (prev->up != 0)
	    prev = ftrace_find_call_by_number (btinfo, prev->up);

	  bfun->level = prev->level - 1;

	  /* Fix up the call stack for PREV.  */
	  ftrace_fixup_caller (btinfo, prev, bfun, BFUN_UP_LINKS_TO_RET);

	  ftrace_debug (bfun, "new return - no caller");
	}
      else
	{
	  /* There is a call in PREV's back trace to which we should have
	     returned but didn't.  Let's start a new, separate back trace
	     from PREV's level.  */
	  bfun->level = prev->level - 1;

	  /* We fix up the back trace for PREV but leave other function segments
	     on the same level as they are.
	     This should handle things like schedule () correctly where we're
	     switching contexts.  */
	  prev->up = bfun->number;
	  prev->flags = BFUN_UP_LINKS_TO_RET;

	  ftrace_debug (bfun, "new return - unknown caller");
	}
    }

  return bfun;
}

/* Add a new function segment for a function switch at the end of the trace.
   BTINFO is the branch trace information for the current thread.
   MFUN and FUN are the symbol information we have for this function.  */

static struct btrace_function *
ftrace_new_switch (struct btrace_thread_info *btinfo,
		   struct minimal_symbol *mfun,
		   struct symbol *fun)
{
  struct btrace_function *prev, *bfun;

  /* This is an unexplained function switch.  We can't really be sure about the
     call stack, yet the best I can think of right now is to preserve it.  */
  bfun = ftrace_new_function (btinfo, mfun, fun);
  prev = ftrace_find_call_by_number (btinfo, bfun->number - 1);
  bfun->up = prev->up;
  bfun->flags = prev->flags;

  ftrace_debug (bfun, "new switch");

  return bfun;
}

/* Add a new function segment for a gap in the trace due to a decode error at
   the end of the trace.
   BTINFO is the branch trace information for the current thread.
   ERRCODE is the format-specific error code.  */

static struct btrace_function *
ftrace_new_gap (struct btrace_thread_info *btinfo, int errcode,
		std::vector<unsigned int> &gaps)
{
  struct btrace_function *bfun;

  if (btinfo->functions.empty ())
    bfun = ftrace_new_function (btinfo, NULL, NULL);
  else
    {
      /* We hijack the previous function segment if it was empty.  */
      bfun = &btinfo->functions.back ();
      if (bfun->errcode != 0 || !bfun->insn.empty ())
	bfun = ftrace_new_function (btinfo, NULL, NULL);
    }

  bfun->errcode = errcode;
  gaps.push_back (bfun->number);

  ftrace_debug (bfun, "new gap");

  return bfun;
}

/* Update the current function segment at the end of the trace in BTINFO with
   respect to the instruction at PC.  This may create new function segments.
   Return the chronologically latest function segment, never NULL.  */

static struct btrace_function *
ftrace_update_function (struct btrace_thread_info *btinfo, CORE_ADDR pc)
{
  struct bound_minimal_symbol bmfun;
  struct minimal_symbol *mfun;
  struct symbol *fun;
  struct btrace_function *bfun;

  /* Try to determine the function we're in.  We use both types of symbols
     to avoid surprises when we sometimes get a full symbol and sometimes
     only a minimal symbol.  */
  fun = find_pc_function (pc);
  bmfun = lookup_minimal_symbol_by_pc (pc);
  mfun = bmfun.minsym;

  if (fun == NULL && mfun == NULL)
    DEBUG_FTRACE ("no symbol at %s", core_addr_to_string_nz (pc));

  /* If we didn't have a function, we create one.  */
  if (btinfo->functions.empty ())
    return ftrace_new_function (btinfo, mfun, fun);

  /* If we had a gap before, we create a function.  */
  bfun = &btinfo->functions.back ();
  if (bfun->errcode != 0)
    return ftrace_new_function (btinfo, mfun, fun);

  /* Check the last instruction, if we have one.
     We do this check first, since it allows us to fill in the call stack
     links in addition to the normal flow links.  */
  btrace_insn *last = NULL;
  if (!bfun->insn.empty ())
    last = &bfun->insn.back ();

  if (last != NULL)
    {
      switch (last->iclass)
	{
	case BTRACE_INSN_RETURN:
	  {
	    const char *fname;

	    /* On some systems, _dl_runtime_resolve returns to the resolved
	       function instead of jumping to it.  From our perspective,
	       however, this is a tailcall.
	       If we treated it as return, we wouldn't be able to find the
	       resolved function in our stack back trace.  Hence, we would
	       lose the current stack back trace and start anew with an empty
	       back trace.  When the resolved function returns, we would then
	       create a stack back trace with the same function names but
	       different frame id's.  This will confuse stepping.  */
	    fname = ftrace_print_function_name (bfun);
	    if (strcmp (fname, "_dl_runtime_resolve") == 0)
	      return ftrace_new_tailcall (btinfo, mfun, fun);

	    return ftrace_new_return (btinfo, mfun, fun);
	  }

	case BTRACE_INSN_CALL:
	  /* Ignore calls to the next instruction.  They are used for PIC.  */
	  if (last->pc + last->size == pc)
	    break;

	  return ftrace_new_call (btinfo, mfun, fun);

	case BTRACE_INSN_JUMP:
	  {
	    CORE_ADDR start;

	    start = get_pc_function_start (pc);

	    /* A jump to the start of a function is (typically) a tail call.  */
	    if (start == pc)
	      return ftrace_new_tailcall (btinfo, mfun, fun);

	    /* Some versions of _Unwind_RaiseException use an indirect
	       jump to 'return' to the exception handler of the caller
	       handling the exception instead of a return.  Let's restrict
	       this heuristic to that and related functions.  */
	    const char *fname = ftrace_print_function_name (bfun);
	    if (strncmp (fname, "_Unwind_", strlen ("_Unwind_")) == 0)
	      {
		struct btrace_function *caller
		  = ftrace_find_call_by_number (btinfo, bfun->up);
		caller = ftrace_find_caller (btinfo, caller, mfun, fun);
		if (caller != NULL)
		  return ftrace_new_return (btinfo, mfun, fun);
	      }

	    /* If we can't determine the function for PC, we treat a jump at
	       the end of the block as tail call if we're switching functions
	       and as an intra-function branch if we don't.  */
	    if (start == 0 && ftrace_function_switched (bfun, mfun, fun))
	      return ftrace_new_tailcall (btinfo, mfun, fun);

	    break;
	  }
	}
    }

  /* Check if we're switching functions for some other reason.  */
  if (ftrace_function_switched (bfun, mfun, fun))
    {
      DEBUG_FTRACE ("switching from %s in %s at %s",
		    ftrace_print_insn_addr (last),
		    ftrace_print_function_name (bfun),
		    ftrace_print_filename (bfun));

      return ftrace_new_switch (btinfo, mfun, fun);
    }

  return bfun;
}

/* Add the instruction at PC to BFUN's instructions.  */

static void
ftrace_update_insns (struct btrace_function *bfun, const btrace_insn &insn)
{
  bfun->insn.push_back (insn);

  if (record_debug > 1)
    ftrace_debug (bfun, "update insn");
}

/* Classify the instruction at PC.  */

static enum btrace_insn_class
ftrace_classify_insn (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  enum btrace_insn_class iclass;

  iclass = BTRACE_INSN_OTHER;
  try
    {
      if (gdbarch_insn_is_call (gdbarch, pc))
	iclass = BTRACE_INSN_CALL;
      else if (gdbarch_insn_is_ret (gdbarch, pc))
	iclass = BTRACE_INSN_RETURN;
      else if (gdbarch_insn_is_jump (gdbarch, pc))
	iclass = BTRACE_INSN_JUMP;
    }
  catch (const gdb_exception_error &error)
    {
    }

  return iclass;
}

/* Try to match the back trace at LHS to the back trace at RHS.  Returns the
   number of matching function segments or zero if the back traces do not
   match.  BTINFO is the branch trace information for the current thread.  */

static int
ftrace_match_backtrace (struct btrace_thread_info *btinfo,
			struct btrace_function *lhs,
			struct btrace_function *rhs)
{
  int matches;

  for (matches = 0; lhs != NULL && rhs != NULL; ++matches)
    {
      if (ftrace_function_switched (lhs, rhs->msym, rhs->sym))
	return 0;

      lhs = ftrace_get_caller (btinfo, lhs);
      rhs = ftrace_get_caller (btinfo, rhs);
    }

  return matches;
}

/* Add ADJUSTMENT to the level of BFUN and succeeding function segments.
   BTINFO is the branch trace information for the current thread.  */

static void
ftrace_fixup_level (struct btrace_thread_info *btinfo,
		    struct btrace_function *bfun, int adjustment)
{
  if (adjustment == 0)
    return;

  DEBUG_FTRACE ("fixup level (%+d)", adjustment);
  ftrace_debug (bfun, "..bfun");

  while (bfun != NULL)
    {
      bfun->level += adjustment;
      bfun = ftrace_find_call_by_number (btinfo, bfun->number + 1);
    }
}

/* Recompute the global level offset.  Traverse the function trace and compute
   the global level offset as the negative of the minimal function level.  */

static void
ftrace_compute_global_level_offset (struct btrace_thread_info *btinfo)
{
  int level = INT_MAX;

  if (btinfo == NULL)
    return;

  if (btinfo->functions.empty ())
    return;

  unsigned int length = btinfo->functions.size() - 1;
  for (unsigned int i = 0; i < length; ++i)
    level = std::min (level, btinfo->functions[i].level);

  /* The last function segment contains the current instruction, which is not
     really part of the trace.  If it contains just this one instruction, we
     ignore the segment.  */
  struct btrace_function *last = &btinfo->functions.back();
  if (last->insn.size () != 1)
    level = std::min (level, last->level);

  DEBUG_FTRACE ("setting global level offset: %d", -level);
  btinfo->level = -level;
}

/* Connect the function segments PREV and NEXT in a bottom-to-top walk as in
   ftrace_connect_backtrace.  BTINFO is the branch trace information for the
   current thread.  */

static void
ftrace_connect_bfun (struct btrace_thread_info *btinfo,
		     struct btrace_function *prev,
		     struct btrace_function *next)
{
  DEBUG_FTRACE ("connecting...");
  ftrace_debug (prev, "..prev");
  ftrace_debug (next, "..next");

  /* The function segments are not yet connected.  */
  gdb_assert (prev->next == 0);
  gdb_assert (next->prev == 0);

  prev->next = next->number;
  next->prev = prev->number;

  /* We may have moved NEXT to a different function level.  */
  ftrace_fixup_level (btinfo, next, prev->level - next->level);

  /* If we run out of back trace for one, let's use the other's.  */
  if (prev->up == 0)
    {
      const btrace_function_flags flags = next->flags;

      next = ftrace_find_call_by_number (btinfo, next->up);
      if (next != NULL)
	{
	  DEBUG_FTRACE ("using next's callers");
	  ftrace_fixup_caller (btinfo, prev, next, flags);
	}
    }
  else if (next->up == 0)
    {
      const btrace_function_flags flags = prev->flags;

      prev = ftrace_find_call_by_number (btinfo, prev->up);
      if (prev != NULL)
	{
	  DEBUG_FTRACE ("using prev's callers");
	  ftrace_fixup_caller (btinfo, next, prev, flags);
	}
    }
  else
    {
      /* PREV may have a tailcall caller, NEXT can't.  If it does, fixup the up
	 link to add the tail callers to NEXT's back trace.

	 This removes NEXT->UP from NEXT's back trace.  It will be added back
	 when connecting NEXT and PREV's callers - provided they exist.

	 If PREV's back trace consists of a series of tail calls without an
	 actual call, there will be no further connection and NEXT's caller will
	 be removed for good.  To catch this case, we handle it here and connect
	 the top of PREV's back trace to NEXT's caller.  */
      if ((prev->flags & BFUN_UP_LINKS_TO_TAILCALL) != 0)
	{
	  struct btrace_function *caller;
	  btrace_function_flags next_flags, prev_flags;

	  /* We checked NEXT->UP above so CALLER can't be NULL.  */
	  caller = ftrace_find_call_by_number (btinfo, next->up);
	  next_flags = next->flags;
	  prev_flags = prev->flags;

	  DEBUG_FTRACE ("adding prev's tail calls to next");

	  prev = ftrace_find_call_by_number (btinfo, prev->up);
	  ftrace_fixup_caller (btinfo, next, prev, prev_flags);

	  for (; prev != NULL; prev = ftrace_find_call_by_number (btinfo,
								  prev->up))
	    {
	      /* At the end of PREV's back trace, continue with CALLER.  */
	      if (prev->up == 0)
		{
		  DEBUG_FTRACE ("fixing up link for tailcall chain");
		  ftrace_debug (prev, "..top");
		  ftrace_debug (caller, "..up");

		  ftrace_fixup_caller (btinfo, prev, caller, next_flags);

		  /* If we skipped any tail calls, this may move CALLER to a
		     different function level.

		     Note that changing CALLER's level is only OK because we
		     know that this is the last iteration of the bottom-to-top
		     walk in ftrace_connect_backtrace.

		     Otherwise we will fix up CALLER's level when we connect it
		     to PREV's caller in the next iteration.  */
		  ftrace_fixup_level (btinfo, caller,
				      prev->level - caller->level - 1);
		  break;
		}

	      /* There's nothing to do if we find a real call.  */
	      if ((prev->flags & BFUN_UP_LINKS_TO_TAILCALL) == 0)
		{
		  DEBUG_FTRACE ("will fix up link in next iteration");
		  break;
		}
	    }
	}
    }
}

/* Connect function segments on the same level in the back trace at LHS and RHS.
   The back traces at LHS and RHS are expected to match according to
   ftrace_match_backtrace.  BTINFO is the branch trace information for the
   current thread.  */

static void
ftrace_connect_backtrace (struct btrace_thread_info *btinfo,
			  struct btrace_function *lhs,
			  struct btrace_function *rhs)
{
  while (lhs != NULL && rhs != NULL)
    {
      struct btrace_function *prev, *next;

      gdb_assert (!ftrace_function_switched (lhs, rhs->msym, rhs->sym));

      /* Connecting LHS and RHS may change the up link.  */
      prev = lhs;
      next = rhs;

      lhs = ftrace_get_caller (btinfo, lhs);
      rhs = ftrace_get_caller (btinfo, rhs);

      ftrace_connect_bfun (btinfo, prev, next);
    }
}

/* Bridge the gap between two function segments left and right of a gap if their
   respective back traces match in at least MIN_MATCHES functions.  BTINFO is
   the branch trace information for the current thread.

   Returns non-zero if the gap could be bridged, zero otherwise.  */

static int
ftrace_bridge_gap (struct btrace_thread_info *btinfo,
		   struct btrace_function *lhs, struct btrace_function *rhs,
		   int min_matches)
{
  struct btrace_function *best_l, *best_r, *cand_l, *cand_r;
  int best_matches;

  DEBUG_FTRACE ("checking gap at insn %u (req matches: %d)",
		rhs->insn_offset - 1, min_matches);

  best_matches = 0;
  best_l = NULL;
  best_r = NULL;

  /* We search the back traces of LHS and RHS for valid connections and connect
     the two function segments that give the longest combined back trace.  */

  for (cand_l = lhs; cand_l != NULL;
       cand_l = ftrace_get_caller (btinfo, cand_l))
    for (cand_r = rhs; cand_r != NULL;
	 cand_r = ftrace_get_caller (btinfo, cand_r))
      {
	int matches;

	matches = ftrace_match_backtrace (btinfo, cand_l, cand_r);
	if (best_matches < matches)
	  {
	    best_matches = matches;
	    best_l = cand_l;
	    best_r = cand_r;
	  }
      }

  /* We need at least MIN_MATCHES matches.  */
  gdb_assert (min_matches > 0);
  if (best_matches < min_matches)
    return 0;

  DEBUG_FTRACE ("..matches: %d", best_matches);

  /* We will fix up the level of BEST_R and succeeding function segments such
     that BEST_R's level matches BEST_L's when we connect BEST_L to BEST_R.

     This will ignore the level of RHS and following if BEST_R != RHS.  I.e. if
     BEST_R is a successor of RHS in the back trace of RHS (phases 1 and 3).

     To catch this, we already fix up the level here where we can start at RHS
     instead of at BEST_R.  We will ignore the level fixup when connecting
     BEST_L to BEST_R as they will already be on the same level.  */
  ftrace_fixup_level (btinfo, rhs, best_l->level - best_r->level);

  ftrace_connect_backtrace (btinfo, best_l, best_r);

  return best_matches;
}

/* Try to bridge gaps due to overflow or decode errors by connecting the
   function segments that are separated by the gap.  */

static void
btrace_bridge_gaps (struct thread_info *tp, std::vector<unsigned int> &gaps)
{
  struct btrace_thread_info *btinfo = &tp->btrace;
  std::vector<unsigned int> remaining;
  int min_matches;

  DEBUG ("bridge gaps");

  /* We require a minimum amount of matches for bridging a gap.  The number of
     required matches will be lowered with each iteration.

     The more matches the higher our confidence that the bridging is correct.
     For big gaps or small traces, however, it may not be feasible to require a
     high number of matches.  */
  for (min_matches = 5; min_matches > 0; --min_matches)
    {
      /* Let's try to bridge as many gaps as we can.  In some cases, we need to
	 skip a gap and revisit it again after we closed later gaps.  */
      while (!gaps.empty ())
	{
	  for (const unsigned int number : gaps)
	    {
	      struct btrace_function *gap, *lhs, *rhs;
	      int bridged;

	      gap = ftrace_find_call_by_number (btinfo, number);

	      /* We may have a sequence of gaps if we run from one error into
		 the next as we try to re-sync onto the trace stream.  Ignore
		 all but the leftmost gap in such a sequence.

		 Also ignore gaps at the beginning of the trace.  */
	      lhs = ftrace_find_call_by_number (btinfo, gap->number - 1);
	      if (lhs == NULL || lhs->errcode != 0)
		continue;

	      /* Skip gaps to the right.  */
	      rhs = ftrace_find_call_by_number (btinfo, gap->number + 1);
	      while (rhs != NULL && rhs->errcode != 0)
		rhs = ftrace_find_call_by_number (btinfo, rhs->number + 1);

	      /* Ignore gaps at the end of the trace.  */
	      if (rhs == NULL)
		continue;

	      bridged = ftrace_bridge_gap (btinfo, lhs, rhs, min_matches);

	      /* Keep track of gaps we were not able to bridge and try again.
		 If we just pushed them to the end of GAPS we would risk an
		 infinite loop in case we simply cannot bridge a gap.  */
	      if (bridged == 0)
		remaining.push_back (number);
	    }

	  /* Let's see if we made any progress.  */
	  if (remaining.size () == gaps.size ())
	    break;

	  gaps.clear ();
	  gaps.swap (remaining);
	}

      /* We get here if either GAPS is empty or if GAPS equals REMAINING.  */
      if (gaps.empty ())
	break;

      remaining.clear ();
    }

  /* We may omit this in some cases.  Not sure it is worth the extra
     complication, though.  */
  ftrace_compute_global_level_offset (btinfo);
}

/* Compute the function branch trace from BTS trace.  */

static void
btrace_compute_ftrace_bts (struct thread_info *tp,
			   const struct btrace_data_bts *btrace,
			   std::vector<unsigned int> &gaps)
{
 /* We may end up doing target calls that require the current thread to be TP,
    for example reading memory through gdb_insn_length.  Make sure TP is the
    current thread.  */
  scoped_restore_current_thread restore_thread;
  switch_to_thread (tp);

  struct btrace_thread_info *btinfo;
  unsigned int blk;
  int level;

  gdbarch *gdbarch = current_inferior ()->arch ();
  btinfo = &tp->btrace;
  blk = btrace->blocks->size ();

  if (btinfo->functions.empty ())
    level = INT_MAX;
  else
    level = -btinfo->level;

  while (blk != 0)
    {
      CORE_ADDR pc;

      blk -= 1;

      const btrace_block &block = btrace->blocks->at (blk);
      pc = block.begin;

      for (;;)
	{
	  struct btrace_function *bfun;
	  struct btrace_insn insn;
	  int size;

	  /* We should hit the end of the block.  Warn if we went too far.  */
	  if (block.end < pc)
	    {
	      /* Indicate the gap in the trace.  */
	      bfun = ftrace_new_gap (btinfo, BDE_BTS_OVERFLOW, gaps);

	      warning (_("Recorded trace may be corrupted at instruction "
			 "%u (pc = %s)."), bfun->insn_offset - 1,
		       core_addr_to_string_nz (pc));

	      break;
	    }

	  bfun = ftrace_update_function (btinfo, pc);

	  /* Maintain the function level offset.
	     For all but the last block, we do it here.  */
	  if (blk != 0)
	    level = std::min (level, bfun->level);

	  size = 0;
	  try
	    {
	      size = gdb_insn_length (gdbarch, pc);
	    }
	  catch (const gdb_exception_error &error)
	    {
	    }

	  insn.pc = pc;
	  insn.size = size;
	  insn.iclass = ftrace_classify_insn (gdbarch, pc);
	  insn.flags = 0;

	  ftrace_update_insns (bfun, insn);

	  /* We're done once we pushed the instruction at the end.  */
	  if (block.end == pc)
	    break;

	  /* We can't continue if we fail to compute the size.  */
	  if (size <= 0)
	    {
	      /* Indicate the gap in the trace.  We just added INSN so we're
		 not at the beginning.  */
	      bfun = ftrace_new_gap (btinfo, BDE_BTS_INSN_SIZE, gaps);

	      warning (_("Recorded trace may be incomplete at instruction %u "
			 "(pc = %s)."), bfun->insn_offset - 1,
		       core_addr_to_string_nz (pc));

	      break;
	    }

	  pc += size;

	  /* Maintain the function level offset.
	     For the last block, we do it here to not consider the last
	     instruction.
	     Since the last instruction corresponds to the current instruction
	     and is not really part of the execution history, it shouldn't
	     affect the level.  */
	  if (blk == 0)
	    level = std::min (level, bfun->level);
	}
    }

  /* LEVEL is the minimal function level of all btrace function segments.
     Define the global level offset to -LEVEL so all function levels are
     normalized to start at zero.  */
  btinfo->level = -level;
}

#if defined (HAVE_LIBIPT)

static enum btrace_insn_class
pt_reclassify_insn (enum pt_insn_class iclass)
{
  switch (iclass)
    {
    case ptic_call:
      return BTRACE_INSN_CALL;

    case ptic_return:
      return BTRACE_INSN_RETURN;

    case ptic_jump:
      return BTRACE_INSN_JUMP;

    default:
      return BTRACE_INSN_OTHER;
    }
}

/* Return the btrace instruction flags for INSN.  */

static btrace_insn_flags
pt_btrace_insn_flags (const struct pt_insn &insn)
{
  btrace_insn_flags flags = 0;

  if (insn.speculative)
    flags |= BTRACE_INSN_FLAG_SPECULATIVE;

  return flags;
}

/* Return the btrace instruction for INSN.  */

static btrace_insn
pt_btrace_insn (const struct pt_insn &insn)
{
  return {(CORE_ADDR) insn.ip, (gdb_byte) insn.size,
	  pt_reclassify_insn (insn.iclass),
	  pt_btrace_insn_flags (insn)};
}

/* Handle instruction decode events (libipt-v2).  */

static int
handle_pt_insn_events (struct btrace_thread_info *btinfo,
		       struct pt_insn_decoder *decoder,
		       std::vector<unsigned int> &gaps, int status)
{
#if defined (HAVE_PT_INSN_EVENT)
  while (status & pts_event_pending)
    {
      struct btrace_function *bfun;
      struct pt_event event;
      uint64_t offset;

      status = pt_insn_event (decoder, &event, sizeof (event));
      if (status < 0)
	break;

      switch (event.type)
	{
	default:
	  break;

	case ptev_enabled:
	  if (event.status_update != 0)
	    break;

	  if (event.variant.enabled.resumed == 0 && !btinfo->functions.empty ())
	    {
	      bfun = ftrace_new_gap (btinfo, BDE_PT_DISABLED, gaps);

	      pt_insn_get_offset (decoder, &offset);

	      warning (_("Non-contiguous trace at instruction %u (offset = 0x%"
			 PRIx64 ")."), bfun->insn_offset - 1, offset);
	    }

	  break;

	case ptev_overflow:
	  bfun = ftrace_new_gap (btinfo, BDE_PT_OVERFLOW, gaps);

	  pt_insn_get_offset (decoder, &offset);

	  warning (_("Overflow at instruction %u (offset = 0x%" PRIx64 ")."),
		   bfun->insn_offset - 1, offset);

	  break;
	}
    }
#endif /* defined (HAVE_PT_INSN_EVENT) */

  return status;
}

/* Handle events indicated by flags in INSN (libipt-v1).  */

static void
handle_pt_insn_event_flags (struct btrace_thread_info *btinfo,
			    struct pt_insn_decoder *decoder,
			    const struct pt_insn &insn,
			    std::vector<unsigned int> &gaps)
{
#if defined (HAVE_STRUCT_PT_INSN_ENABLED)
  /* Tracing is disabled and re-enabled each time we enter the kernel.  Most
     times, we continue from the same instruction we stopped before.  This is
     indicated via the RESUMED instruction flag.  The ENABLED instruction flag
     means that we continued from some other instruction.  Indicate this as a
     trace gap except when tracing just started.  */
  if (insn.enabled && !btinfo->functions.empty ())
    {
      struct btrace_function *bfun;
      uint64_t offset;

      bfun = ftrace_new_gap (btinfo, BDE_PT_DISABLED, gaps);

      pt_insn_get_offset (decoder, &offset);

      warning (_("Non-contiguous trace at instruction %u (offset = 0x%" PRIx64
		 ", pc = 0x%" PRIx64 ")."), bfun->insn_offset - 1, offset,
	       insn.ip);
    }
#endif /* defined (HAVE_STRUCT_PT_INSN_ENABLED) */

#if defined (HAVE_STRUCT_PT_INSN_RESYNCED)
  /* Indicate trace overflows.  */
  if (insn.resynced)
    {
      struct btrace_function *bfun;
      uint64_t offset;

      bfun = ftrace_new_gap (btinfo, BDE_PT_OVERFLOW, gaps);

      pt_insn_get_offset (decoder, &offset);

      warning (_("Overflow at instruction %u (offset = 0x%" PRIx64 ", pc = 0x%"
		 PRIx64 ")."), bfun->insn_offset - 1, offset, insn.ip);
    }
#endif /* defined (HAVE_STRUCT_PT_INSN_RESYNCED) */
}

/* Add function branch trace to BTINFO using DECODER.  */

static void
ftrace_add_pt (struct btrace_thread_info *btinfo,
	       struct pt_insn_decoder *decoder,
	       int *plevel,
	       std::vector<unsigned int> &gaps)
{
  struct btrace_function *bfun;
  uint64_t offset;
  int status;

  for (;;)
    {
      struct pt_insn insn;

      status = pt_insn_sync_forward (decoder);
      if (status < 0)
	{
	  if (status != -pte_eos)
	    warning (_("Failed to synchronize onto the Intel Processor "
		       "Trace stream: %s."), pt_errstr (pt_errcode (status)));
	  break;
	}

      for (;;)
	{
	  /* Handle events from the previous iteration or synchronization.  */
	  status = handle_pt_insn_events (btinfo, decoder, gaps, status);
	  if (status < 0)
	    break;

	  status = pt_insn_next (decoder, &insn, sizeof(insn));
	  if (status < 0)
	    break;

	  /* Handle events indicated by flags in INSN.  */
	  handle_pt_insn_event_flags (btinfo, decoder, insn, gaps);

	  bfun = ftrace_update_function (btinfo, insn.ip);

	  /* Maintain the function level offset.  */
	  *plevel = std::min (*plevel, bfun->level);

	  ftrace_update_insns (bfun, pt_btrace_insn (insn));
	}

      if (status == -pte_eos)
	break;

      /* Indicate the gap in the trace.  */
      bfun = ftrace_new_gap (btinfo, status, gaps);

      pt_insn_get_offset (decoder, &offset);

      warning (_("Decode error (%d) at instruction %u (offset = 0x%" PRIx64
		 ", pc = 0x%" PRIx64 "): %s."), status, bfun->insn_offset - 1,
	       offset, insn.ip, pt_errstr (pt_errcode (status)));
    }
}

/* A callback function to allow the trace decoder to read the inferior's
   memory.  */

static int
btrace_pt_readmem_callback (gdb_byte *buffer, size_t size,
			    const struct pt_asid *asid, uint64_t pc,
			    void *context)
{
  int result, errcode;

  result = (int) size;
  try
    {
      errcode = target_read_code ((CORE_ADDR) pc, buffer, size);
      if (errcode != 0)
	result = -pte_nomap;
    }
  catch (const gdb_exception_error &error)
    {
      result = -pte_nomap;
    }

  return result;
}

/* Translate the vendor from one enum to another.  */

static enum pt_cpu_vendor
pt_translate_cpu_vendor (enum btrace_cpu_vendor vendor)
{
  switch (vendor)
    {
    default:
      return pcv_unknown;

    case CV_INTEL:
      return pcv_intel;
    }
}

/* Finalize the function branch trace after decode.  */

static void btrace_finalize_ftrace_pt (struct pt_insn_decoder *decoder,
				       struct thread_info *tp, int level)
{
  pt_insn_free_decoder (decoder);

  /* LEVEL is the minimal function level of all btrace function segments.
     Define the global level offset to -LEVEL so all function levels are
     normalized to start at zero.  */
  tp->btrace.level = -level;

  /* Add a single last instruction entry for the current PC.
     This allows us to compute the backtrace at the current PC using both
     standard unwind and btrace unwind.
     This extra entry is ignored by all record commands.  */
  btrace_add_pc (tp);
}

/* Compute the function branch trace from Intel Processor Trace
   format.  */

static void
btrace_compute_ftrace_pt (struct thread_info *tp,
			  const struct btrace_data_pt *btrace,
			  std::vector<unsigned int> &gaps)
{
 /* We may end up doing target calls that require the current thread to be TP,
    for example reading memory through btrace_pt_readmem_callback.  Make sure
    TP is the current thread.  */
  scoped_restore_current_thread restore_thread;
  switch_to_thread (tp);

  struct btrace_thread_info *btinfo;
  struct pt_insn_decoder *decoder;
  struct pt_config config;
  int level, errcode;

  if (btrace->size == 0)
    return;

  btinfo = &tp->btrace;
  if (btinfo->functions.empty ())
    level = INT_MAX;
  else
    level = -btinfo->level;

  pt_config_init(&config);
  config.begin = btrace->data;
  config.end = btrace->data + btrace->size;

  /* We treat an unknown vendor as 'no errata'.  */
  if (btrace->config.cpu.vendor != CV_UNKNOWN)
    {
      config.cpu.vendor
	= pt_translate_cpu_vendor (btrace->config.cpu.vendor);
      config.cpu.family = btrace->config.cpu.family;
      config.cpu.model = btrace->config.cpu.model;
      config.cpu.stepping = btrace->config.cpu.stepping;

      errcode = pt_cpu_errata (&config.errata, &config.cpu);
      if (errcode < 0)
	error (_("Failed to configure the Intel Processor Trace "
		 "decoder: %s."), pt_errstr (pt_errcode (errcode)));
    }

  decoder = pt_insn_alloc_decoder (&config);
  if (decoder == NULL)
    error (_("Failed to allocate the Intel Processor Trace decoder."));

  try
    {
      struct pt_image *image;

      image = pt_insn_get_image(decoder);
      if (image == NULL)
	error (_("Failed to configure the Intel Processor Trace decoder."));

      errcode = pt_image_set_callback(image, btrace_pt_readmem_callback, NULL);
      if (errcode < 0)
	error (_("Failed to configure the Intel Processor Trace decoder: "
		 "%s."), pt_errstr (pt_errcode (errcode)));

      ftrace_add_pt (btinfo, decoder, &level, gaps);
    }
  catch (const gdb_exception &error)
    {
      /* Indicate a gap in the trace if we quit trace processing.  */
      if (error.reason == RETURN_QUIT && !btinfo->functions.empty ())
	ftrace_new_gap (btinfo, BDE_PT_USER_QUIT, gaps);

      btrace_finalize_ftrace_pt (decoder, tp, level);

      throw;
    }

  btrace_finalize_ftrace_pt (decoder, tp, level);
}

#else /* defined (HAVE_LIBIPT)  */

static void
btrace_compute_ftrace_pt (struct thread_info *tp,
			  const struct btrace_data_pt *btrace,
			  std::vector<unsigned int> &gaps)
{
  internal_error (_("Unexpected branch trace format."));
}

#endif /* defined (HAVE_LIBIPT)  */

/* Compute the function branch trace from a block branch trace BTRACE for
   a thread given by BTINFO.  If CPU is not NULL, overwrite the cpu in the
   branch trace configuration.  This is currently only used for the PT
   format.  */

static void
btrace_compute_ftrace_1 (struct thread_info *tp,
			 struct btrace_data *btrace,
			 const struct btrace_cpu *cpu,
			 std::vector<unsigned int> &gaps)
{
  DEBUG ("compute ftrace");

  switch (btrace->format)
    {
    case BTRACE_FORMAT_NONE:
      return;

    case BTRACE_FORMAT_BTS:
      btrace_compute_ftrace_bts (tp, &btrace->variant.bts, gaps);
      return;

    case BTRACE_FORMAT_PT:
      /* Overwrite the cpu we use for enabling errata workarounds.  */
      if (cpu != nullptr)
	btrace->variant.pt.config.cpu = *cpu;

      btrace_compute_ftrace_pt (tp, &btrace->variant.pt, gaps);
      return;
    }

  internal_error (_("Unknown branch trace format."));
}

static void
btrace_finalize_ftrace (struct thread_info *tp, std::vector<unsigned int> &gaps)
{
  if (!gaps.empty ())
    {
      tp->btrace.ngaps += gaps.size ();
      btrace_bridge_gaps (tp, gaps);
    }
}

static void
btrace_compute_ftrace (struct thread_info *tp, struct btrace_data *btrace,
		       const struct btrace_cpu *cpu)
{
  std::vector<unsigned int> gaps;

  try
    {
      btrace_compute_ftrace_1 (tp, btrace, cpu, gaps);
    }
  catch (const gdb_exception &error)
    {
      btrace_finalize_ftrace (tp, gaps);

      throw;
    }

  btrace_finalize_ftrace (tp, gaps);
}

/* Add an entry for the current PC.  */

static void
btrace_add_pc (struct thread_info *tp)
{
  struct btrace_data btrace;
  struct regcache *regcache;
  CORE_ADDR pc;

  regcache = get_thread_regcache (tp);
  pc = regcache_read_pc (regcache);

  btrace.format = BTRACE_FORMAT_BTS;
  btrace.variant.bts.blocks = new std::vector<btrace_block>;

  btrace.variant.bts.blocks->emplace_back (pc, pc);

  btrace_compute_ftrace (tp, &btrace, NULL);
}

/* See btrace.h.  */

void
btrace_enable (struct thread_info *tp, const struct btrace_config *conf)
{
  if (tp->btrace.target != NULL)
    error (_("Recording already enabled on thread %s (%s)."),
	   print_thread_id (tp), target_pid_to_str (tp->ptid).c_str ());

#if !defined (HAVE_LIBIPT)
  if (conf->format == BTRACE_FORMAT_PT)
    error (_("Intel Processor Trace support was disabled at compile time."));
#endif /* !defined (HAVE_LIBIPT) */

  DEBUG ("enable thread %s (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str ());

  tp->btrace.target = target_enable_btrace (tp, conf);

  if (tp->btrace.target == NULL)
    error (_("Failed to enable recording on thread %s (%s)."),
	   print_thread_id (tp), target_pid_to_str (tp->ptid).c_str ());

  /* We need to undo the enable in case of errors.  */
  try
    {
      /* Add an entry for the current PC so we start tracing from where we
	 enabled it.

	 If we can't access TP's registers, TP is most likely running.  In this
	 case, we can't really say where tracing was enabled so it should be
	 safe to simply skip this step.

	 This is not relevant for BTRACE_FORMAT_PT since the trace will already
	 start at the PC at which tracing was enabled.  */
      if (conf->format != BTRACE_FORMAT_PT
	  && can_access_registers_thread (tp))
	btrace_add_pc (tp);
    }
  catch (const gdb_exception &exception)
    {
      btrace_disable (tp);

      throw;
    }
}

/* See btrace.h.  */

const struct btrace_config *
btrace_conf (const struct btrace_thread_info *btinfo)
{
  if (btinfo->target == NULL)
    return NULL;

  return target_btrace_conf (btinfo->target);
}

/* See btrace.h.  */

void
btrace_disable (struct thread_info *tp)
{
  struct btrace_thread_info *btp = &tp->btrace;

  if (btp->target == NULL)
    error (_("Recording not enabled on thread %s (%s)."),
	   print_thread_id (tp), target_pid_to_str (tp->ptid).c_str ());

  DEBUG ("disable thread %s (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str ());

  target_disable_btrace (btp->target);
  btp->target = NULL;

  btrace_clear (tp);
}

/* See btrace.h.  */

void
btrace_teardown (struct thread_info *tp)
{
  struct btrace_thread_info *btp = &tp->btrace;

  if (btp->target == NULL)
    return;

  DEBUG ("teardown thread %s (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str ());

  target_teardown_btrace (btp->target);
  btp->target = NULL;

  btrace_clear (tp);
}

/* Stitch branch trace in BTS format.  */

static int
btrace_stitch_bts (struct btrace_data_bts *btrace, struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;
  struct btrace_function *last_bfun;
  btrace_block *first_new_block;

  btinfo = &tp->btrace;
  gdb_assert (!btinfo->functions.empty ());
  gdb_assert (!btrace->blocks->empty ());

  last_bfun = &btinfo->functions.back ();

  /* If the existing trace ends with a gap, we just glue the traces
     together.  We need to drop the last (i.e. chronologically first) block
     of the new trace,  though, since we can't fill in the start address.*/
  if (last_bfun->insn.empty ())
    {
      btrace->blocks->pop_back ();
      return 0;
    }

  /* Beware that block trace starts with the most recent block, so the
     chronologically first block in the new trace is the last block in
     the new trace's block vector.  */
  first_new_block = &btrace->blocks->back ();
  const btrace_insn &last_insn = last_bfun->insn.back ();

  /* If the current PC at the end of the block is the same as in our current
     trace, there are two explanations:
       1. we executed the instruction and some branch brought us back.
       2. we have not made any progress.
     In the first case, the delta trace vector should contain at least two
     entries.
     In the second case, the delta trace vector should contain exactly one
     entry for the partial block containing the current PC.  Remove it.  */
  if (first_new_block->end == last_insn.pc && btrace->blocks->size () == 1)
    {
      btrace->blocks->pop_back ();
      return 0;
    }

  DEBUG ("stitching %s to %s", ftrace_print_insn_addr (&last_insn),
	 core_addr_to_string_nz (first_new_block->end));

  /* Do a simple sanity check to make sure we don't accidentally end up
     with a bad block.  This should not occur in practice.  */
  if (first_new_block->end < last_insn.pc)
    {
      warning (_("Error while trying to read delta trace.  Falling back to "
		 "a full read."));
      return -1;
    }

  /* We adjust the last block to start at the end of our current trace.  */
  gdb_assert (first_new_block->begin == 0);
  first_new_block->begin = last_insn.pc;

  /* We simply pop the last insn so we can insert it again as part of
     the normal branch trace computation.
     Since instruction iterators are based on indices in the instructions
     vector, we don't leave any pointers dangling.  */
  DEBUG ("pruning insn at %s for stitching",
	 ftrace_print_insn_addr (&last_insn));

  last_bfun->insn.pop_back ();

  /* The instructions vector may become empty temporarily if this has
     been the only instruction in this function segment.
     This violates the invariant but will be remedied shortly by
     btrace_compute_ftrace when we add the new trace.  */

  /* The only case where this would hurt is if the entire trace consisted
     of just that one instruction.  If we remove it, we might turn the now
     empty btrace function segment into a gap.  But we don't want gaps at
     the beginning.  To avoid this, we remove the entire old trace.  */
  if (last_bfun->number == 1 && last_bfun->insn.empty ())
    btrace_clear (tp);

  return 0;
}

/* Adjust the block trace in order to stitch old and new trace together.
   BTRACE is the new delta trace between the last and the current stop.
   TP is the traced thread.
   May modifx BTRACE as well as the existing trace in TP.
   Return 0 on success, -1 otherwise.  */

static int
btrace_stitch_trace (struct btrace_data *btrace, struct thread_info *tp)
{
  /* If we don't have trace, there's nothing to do.  */
  if (btrace->empty ())
    return 0;

  switch (btrace->format)
    {
    case BTRACE_FORMAT_NONE:
      return 0;

    case BTRACE_FORMAT_BTS:
      return btrace_stitch_bts (&btrace->variant.bts, tp);

    case BTRACE_FORMAT_PT:
      /* Delta reads are not supported.  */
      return -1;
    }

  internal_error (_("Unknown branch trace format."));
}

/* Clear the branch trace histories in BTINFO.  */

static void
btrace_clear_history (struct btrace_thread_info *btinfo)
{
  xfree (btinfo->insn_history);
  xfree (btinfo->call_history);
  xfree (btinfo->replay);

  btinfo->insn_history = NULL;
  btinfo->call_history = NULL;
  btinfo->replay = NULL;
}

/* Clear the branch trace maintenance histories in BTINFO.  */

static void
btrace_maint_clear (struct btrace_thread_info *btinfo)
{
  switch (btinfo->data.format)
    {
    default:
      break;

    case BTRACE_FORMAT_BTS:
      btinfo->maint.variant.bts.packet_history.begin = 0;
      btinfo->maint.variant.bts.packet_history.end = 0;
      break;

#if defined (HAVE_LIBIPT)
    case BTRACE_FORMAT_PT:
      delete btinfo->maint.variant.pt.packets;

      btinfo->maint.variant.pt.packets = NULL;
      btinfo->maint.variant.pt.packet_history.begin = 0;
      btinfo->maint.variant.pt.packet_history.end = 0;
      break;
#endif /* defined (HAVE_LIBIPT)  */
    }
}

/* See btrace.h.  */

const char *
btrace_decode_error (enum btrace_format format, int errcode)
{
  switch (format)
    {
    case BTRACE_FORMAT_BTS:
      switch (errcode)
	{
	case BDE_BTS_OVERFLOW:
	  return _("instruction overflow");

	case BDE_BTS_INSN_SIZE:
	  return _("unknown instruction");

	default:
	  break;
	}
      break;

#if defined (HAVE_LIBIPT)
    case BTRACE_FORMAT_PT:
      switch (errcode)
	{
	case BDE_PT_USER_QUIT:
	  return _("trace decode cancelled");

	case BDE_PT_DISABLED:
	  return _("disabled");

	case BDE_PT_OVERFLOW:
	  return _("overflow");

	default:
	  if (errcode < 0)
	    return pt_errstr (pt_errcode (errcode));
	  break;
	}
      break;
#endif /* defined (HAVE_LIBIPT)  */

    default:
      break;
    }

  return _("unknown");
}

/* See btrace.h.  */

void
btrace_fetch (struct thread_info *tp, const struct btrace_cpu *cpu)
{
  struct btrace_thread_info *btinfo;
  struct btrace_target_info *tinfo;
  struct btrace_data btrace;
  int errcode;

  DEBUG ("fetch thread %s (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str ());

  btinfo = &tp->btrace;
  tinfo = btinfo->target;
  if (tinfo == NULL)
    return;

  /* There's no way we could get new trace while replaying.
     On the other hand, delta trace would return a partial record with the
     current PC, which is the replay PC, not the last PC, as expected.  */
  if (btinfo->replay != NULL)
    return;

  /* With CLI usage, TP is always the current thread when we get here.
     However, since we can also store a gdb.Record object in Python
     referring to a different thread than the current one, we need to
     temporarily set the current thread.  */
  scoped_restore_current_thread restore_thread;
  switch_to_thread (tp);

  /* We should not be called on running or exited threads.  */
  gdb_assert (can_access_registers_thread (tp));

  /* Let's first try to extend the trace we already have.  */
  if (!btinfo->functions.empty ())
    {
      errcode = target_read_btrace (&btrace, tinfo, BTRACE_READ_DELTA);
      if (errcode == 0)
	{
	  /* Success.  Let's try to stitch the traces together.  */
	  errcode = btrace_stitch_trace (&btrace, tp);
	}
      else
	{
	  /* We failed to read delta trace.  Let's try to read new trace.  */
	  errcode = target_read_btrace (&btrace, tinfo, BTRACE_READ_NEW);

	  /* If we got any new trace, discard what we have.  */
	  if (errcode == 0 && !btrace.empty ())
	    btrace_clear (tp);
	}

      /* If we were not able to read the trace, we start over.  */
      if (errcode != 0)
	{
	  btrace_clear (tp);
	  errcode = target_read_btrace (&btrace, tinfo, BTRACE_READ_ALL);
	}
    }
  else
    errcode = target_read_btrace (&btrace, tinfo, BTRACE_READ_ALL);

  /* If we were not able to read the branch trace, signal an error.  */
  if (errcode != 0)
    error (_("Failed to read branch trace."));

  /* Compute the trace, provided we have any.  */
  if (!btrace.empty ())
    {
      /* Store the raw trace data.  The stored data will be cleared in
	 btrace_clear, so we always append the new trace.  */
      btrace_data_append (&btinfo->data, &btrace);
      btrace_maint_clear (btinfo);

      btrace_clear_history (btinfo);
      btrace_compute_ftrace (tp, &btrace, cpu);
    }
}

/* See btrace.h.  */

void
btrace_clear (struct thread_info *tp)
{
  struct btrace_thread_info *btinfo;

  DEBUG ("clear thread %s (%s)", print_thread_id (tp),
	 tp->ptid.to_string ().c_str ());

  /* Make sure btrace frames that may hold a pointer into the branch
     trace data are destroyed.  */
  reinit_frame_cache ();

  btinfo = &tp->btrace;

  btinfo->functions.clear ();
  btinfo->ngaps = 0;

  /* Must clear the maint data before - it depends on BTINFO->DATA.  */
  btrace_maint_clear (btinfo);
  btinfo->data.clear ();
  btrace_clear_history (btinfo);
}

/* See btrace.h.  */

void
btrace_free_objfile (struct objfile *objfile)
{
  DEBUG ("free objfile");

  for (thread_info *tp : all_non_exited_threads ())
    btrace_clear (tp);
}

/* See btrace.h.  */

const struct btrace_insn *
btrace_insn_get (const struct btrace_insn_iterator *it)
{
  const struct btrace_function *bfun;
  unsigned int index, end;

  index = it->insn_index;
  bfun = &it->btinfo->functions[it->call_index];

  /* Check if the iterator points to a gap in the trace.  */
  if (bfun->errcode != 0)
    return NULL;

  /* The index is within the bounds of this function's instruction vector.  */
  end = bfun->insn.size ();
  gdb_assert (0 < end);
  gdb_assert (index < end);

  return &bfun->insn[index];
}

/* See btrace.h.  */

int
btrace_insn_get_error (const struct btrace_insn_iterator *it)
{
  return it->btinfo->functions[it->call_index].errcode;
}

/* See btrace.h.  */

unsigned int
btrace_insn_number (const struct btrace_insn_iterator *it)
{
  return it->btinfo->functions[it->call_index].insn_offset + it->insn_index;
}

/* See btrace.h.  */

void
btrace_insn_begin (struct btrace_insn_iterator *it,
		   const struct btrace_thread_info *btinfo)
{
  if (btinfo->functions.empty ())
    error (_("No trace."));

  it->btinfo = btinfo;
  it->call_index = 0;
  it->insn_index = 0;
}

/* See btrace.h.  */

void
btrace_insn_end (struct btrace_insn_iterator *it,
		 const struct btrace_thread_info *btinfo)
{
  const struct btrace_function *bfun;
  unsigned int length;

  if (btinfo->functions.empty ())
    error (_("No trace."));

  bfun = &btinfo->functions.back ();
  length = bfun->insn.size ();

  /* The last function may either be a gap or it contains the current
     instruction, which is one past the end of the execution trace; ignore
     it.  */
  if (length > 0)
    length -= 1;

  it->btinfo = btinfo;
  it->call_index = bfun->number - 1;
  it->insn_index = length;
}

/* See btrace.h.  */

unsigned int
btrace_insn_next (struct btrace_insn_iterator *it, unsigned int stride)
{
  const struct btrace_function *bfun;
  unsigned int index, steps;

  bfun = &it->btinfo->functions[it->call_index];
  steps = 0;
  index = it->insn_index;

  while (stride != 0)
    {
      unsigned int end, space, adv;

      end = bfun->insn.size ();

      /* An empty function segment represents a gap in the trace.  We count
	 it as one instruction.  */
      if (end == 0)
	{
	  const struct btrace_function *next;

	  next = ftrace_find_call_by_number (it->btinfo, bfun->number + 1);
	  if (next == NULL)
	    break;

	  stride -= 1;
	  steps += 1;

	  bfun = next;
	  index = 0;

	  continue;
	}

      gdb_assert (0 < end);
      gdb_assert (index < end);

      /* Compute the number of instructions remaining in this segment.  */
      space = end - index;

      /* Advance the iterator as far as possible within this segment.  */
      adv = std::min (space, stride);
      stride -= adv;
      index += adv;
      steps += adv;

      /* Move to the next function if we're at the end of this one.  */
      if (index == end)
	{
	  const struct btrace_function *next;

	  next = ftrace_find_call_by_number (it->btinfo, bfun->number + 1);
	  if (next == NULL)
	    {
	      /* We stepped past the last function.

		 Let's adjust the index to point to the last instruction in
		 the previous function.  */
	      index -= 1;
	      steps -= 1;
	      break;
	    }

	  /* We now point to the first instruction in the new function.  */
	  bfun = next;
	  index = 0;
	}

      /* We did make progress.  */
      gdb_assert (adv > 0);
    }

  /* Update the iterator.  */
  it->call_index = bfun->number - 1;
  it->insn_index = index;

  return steps;
}

/* See btrace.h.  */

unsigned int
btrace_insn_prev (struct btrace_insn_iterator *it, unsigned int stride)
{
  const struct btrace_function *bfun;
  unsigned int index, steps;

  bfun = &it->btinfo->functions[it->call_index];
  steps = 0;
  index = it->insn_index;

  while (stride != 0)
    {
      unsigned int adv;

      /* Move to the previous function if we're at the start of this one.  */
      if (index == 0)
	{
	  const struct btrace_function *prev;

	  prev = ftrace_find_call_by_number (it->btinfo, bfun->number - 1);
	  if (prev == NULL)
	    break;

	  /* We point to one after the last instruction in the new function.  */
	  bfun = prev;
	  index = bfun->insn.size ();

	  /* An empty function segment represents a gap in the trace.  We count
	     it as one instruction.  */
	  if (index == 0)
	    {
	      stride -= 1;
	      steps += 1;

	      continue;
	    }
	}

      /* Advance the iterator as far as possible within this segment.  */
      adv = std::min (index, stride);

      stride -= adv;
      index -= adv;
      steps += adv;

      /* We did make progress.  */
      gdb_assert (adv > 0);
    }

  /* Update the iterator.  */
  it->call_index = bfun->number - 1;
  it->insn_index = index;

  return steps;
}

/* See btrace.h.  */

int
btrace_insn_cmp (const struct btrace_insn_iterator *lhs,
		 const struct btrace_insn_iterator *rhs)
{
  gdb_assert (lhs->btinfo == rhs->btinfo);

  if (lhs->call_index != rhs->call_index)
    return lhs->call_index - rhs->call_index;

  return lhs->insn_index - rhs->insn_index;
}

/* See btrace.h.  */

int
btrace_find_insn_by_number (struct btrace_insn_iterator *it,
			    const struct btrace_thread_info *btinfo,
			    unsigned int number)
{
  const struct btrace_function *bfun;
  unsigned int upper, lower;

  if (btinfo->functions.empty ())
      return 0;

  lower = 0;
  bfun = &btinfo->functions[lower];
  if (number < bfun->insn_offset)
    return 0;

  upper = btinfo->functions.size () - 1;
  bfun = &btinfo->functions[upper];
  if (number >= bfun->insn_offset + ftrace_call_num_insn (bfun))
    return 0;

  /* We assume that there are no holes in the numbering.  */
  for (;;)
    {
      const unsigned int average = lower + (upper - lower) / 2;

      bfun = &btinfo->functions[average];

      if (number < bfun->insn_offset)
	{
	  upper = average - 1;
	  continue;
	}

      if (number >= bfun->insn_offset + ftrace_call_num_insn (bfun))
	{
	  lower = average + 1;
	  continue;
	}

      break;
    }

  it->btinfo = btinfo;
  it->call_index = bfun->number - 1;
  it->insn_index = number - bfun->insn_offset;
  return 1;
}

/* Returns true if the recording ends with a function segment that
   contains only a single (i.e. the current) instruction.  */

static bool
btrace_ends_with_single_insn (const struct btrace_thread_info *btinfo)
{
  const btrace_function *bfun;

  if (btinfo->functions.empty ())
    return false;

  bfun = &btinfo->functions.back ();
  if (bfun->errcode != 0)
    return false;

  return ftrace_call_num_insn (bfun) == 1;
}

/* See btrace.h.  */

const struct btrace_function *
btrace_call_get (const struct btrace_call_iterator *it)
{
  if (it->index >= it->btinfo->functions.size ())
    return NULL;

  return &it->btinfo->functions[it->index];
}

/* See btrace.h.  */

unsigned int
btrace_call_number (const struct btrace_call_iterator *it)
{
  const unsigned int length = it->btinfo->functions.size ();

  /* If the last function segment contains only a single instruction (i.e. the
     current instruction), skip it.  */
  if ((it->index == length) && btrace_ends_with_single_insn (it->btinfo))
    return length;

  return it->index + 1;
}

/* See btrace.h.  */

void
btrace_call_begin (struct btrace_call_iterator *it,
		   const struct btrace_thread_info *btinfo)
{
  if (btinfo->functions.empty ())
    error (_("No trace."));

  it->btinfo = btinfo;
  it->index = 0;
}

/* See btrace.h.  */

void
btrace_call_end (struct btrace_call_iterator *it,
		 const struct btrace_thread_info *btinfo)
{
  if (btinfo->functions.empty ())
    error (_("No trace."));

  it->btinfo = btinfo;
  it->index = btinfo->functions.size ();
}

/* See btrace.h.  */

unsigned int
btrace_call_next (struct btrace_call_iterator *it, unsigned int stride)
{
  const unsigned int length = it->btinfo->functions.size ();

  if (it->index + stride < length - 1)
    /* Default case: Simply advance the iterator.  */
    it->index += stride;
  else if (it->index + stride == length - 1)
    {
      /* We land exactly at the last function segment.  If it contains only one
	 instruction (i.e. the current instruction) it is not actually part of
	 the trace.  */
      if (btrace_ends_with_single_insn (it->btinfo))
	it->index = length;
      else
	it->index = length - 1;
    }
  else
    {
      /* We land past the last function segment and have to adjust the stride.
	 If the last function segment contains only one instruction (i.e. the
	 current instruction) it is not actually part of the trace.  */
      if (btrace_ends_with_single_insn (it->btinfo))
	stride = length - it->index - 1;
      else
	stride = length - it->index;

      it->index = length;
    }

  return stride;
}

/* See btrace.h.  */

unsigned int
btrace_call_prev (struct btrace_call_iterator *it, unsigned int stride)
{
  const unsigned int length = it->btinfo->functions.size ();
  int steps = 0;

  gdb_assert (it->index <= length);

  if (stride == 0 || it->index == 0)
    return 0;

  /* If we are at the end, the first step is a special case.  If the last
     function segment contains only one instruction (i.e. the current
     instruction) it is not actually part of the trace.  To be able to step
     over this instruction, we need at least one more function segment.  */
  if ((it->index == length)  && (length > 1))
    {
      if (btrace_ends_with_single_insn (it->btinfo))
	it->index = length - 2;
      else
	it->index = length - 1;

      steps = 1;
      stride -= 1;
    }

  stride = std::min (stride, it->index);

  it->index -= stride;
  return steps + stride;
}

/* See btrace.h.  */

int
btrace_call_cmp (const struct btrace_call_iterator *lhs,
		 const struct btrace_call_iterator *rhs)
{
  gdb_assert (lhs->btinfo == rhs->btinfo);
  return (int) (lhs->index - rhs->index);
}

/* See btrace.h.  */

int
btrace_find_call_by_number (struct btrace_call_iterator *it,
			    const struct btrace_thread_info *btinfo,
			    unsigned int number)
{
  const unsigned int length = btinfo->functions.size ();

  if ((number == 0) || (number > length))
    return 0;

  it->btinfo = btinfo;
  it->index = number - 1;
  return 1;
}

/* See btrace.h.  */

void
btrace_set_insn_history (struct btrace_thread_info *btinfo,
			 const struct btrace_insn_iterator *begin,
			 const struct btrace_insn_iterator *end)
{
  if (btinfo->insn_history == NULL)
    btinfo->insn_history = XCNEW (struct btrace_insn_history);

  btinfo->insn_history->begin = *begin;
  btinfo->insn_history->end = *end;
}

/* See btrace.h.  */

void
btrace_set_call_history (struct btrace_thread_info *btinfo,
			 const struct btrace_call_iterator *begin,
			 const struct btrace_call_iterator *end)
{
  gdb_assert (begin->btinfo == end->btinfo);

  if (btinfo->call_history == NULL)
    btinfo->call_history = XCNEW (struct btrace_call_history);

  btinfo->call_history->begin = *begin;
  btinfo->call_history->end = *end;
}

/* See btrace.h.  */

int
btrace_is_replaying (struct thread_info *tp)
{
  return tp->btrace.replay != NULL;
}

/* See btrace.h.  */

int
btrace_is_empty (struct thread_info *tp)
{
  struct btrace_insn_iterator begin, end;
  struct btrace_thread_info *btinfo;

  btinfo = &tp->btrace;

  if (btinfo->functions.empty ())
    return 1;

  btrace_insn_begin (&begin, btinfo);
  btrace_insn_end (&end, btinfo);

  return btrace_insn_cmp (&begin, &end) == 0;
}

#if defined (HAVE_LIBIPT)

/* Print a single packet.  */

static void
pt_print_packet (const struct pt_packet *packet)
{
  switch (packet->type)
    {
    default:
      gdb_printf (("[??: %x]"), packet->type);
      break;

    case ppt_psb:
      gdb_printf (("psb"));
      break;

    case ppt_psbend:
      gdb_printf (("psbend"));
      break;

    case ppt_pad:
      gdb_printf (("pad"));
      break;

    case ppt_tip:
      gdb_printf (("tip %u: 0x%" PRIx64 ""),
		  packet->payload.ip.ipc,
		  packet->payload.ip.ip);
      break;

    case ppt_tip_pge:
      gdb_printf (("tip.pge %u: 0x%" PRIx64 ""),
		  packet->payload.ip.ipc,
		  packet->payload.ip.ip);
      break;

    case ppt_tip_pgd:
      gdb_printf (("tip.pgd %u: 0x%" PRIx64 ""),
		  packet->payload.ip.ipc,
		  packet->payload.ip.ip);
      break;

    case ppt_fup:
      gdb_printf (("fup %u: 0x%" PRIx64 ""),
		  packet->payload.ip.ipc,
		  packet->payload.ip.ip);
      break;

    case ppt_tnt_8:
      gdb_printf (("tnt-8 %u: 0x%" PRIx64 ""),
		  packet->payload.tnt.bit_size,
		  packet->payload.tnt.payload);
      break;

    case ppt_tnt_64:
      gdb_printf (("tnt-64 %u: 0x%" PRIx64 ""),
		  packet->payload.tnt.bit_size,
		  packet->payload.tnt.payload);
      break;

    case ppt_pip:
      gdb_printf (("pip %" PRIx64 "%s"), packet->payload.pip.cr3,
		  packet->payload.pip.nr ? (" nr") : (""));
      break;

    case ppt_tsc:
      gdb_printf (("tsc %" PRIx64 ""), packet->payload.tsc.tsc);
      break;

    case ppt_cbr:
      gdb_printf (("cbr %u"), packet->payload.cbr.ratio);
      break;

    case ppt_mode:
      switch (packet->payload.mode.leaf)
	{
	default:
	  gdb_printf (("mode %u"), packet->payload.mode.leaf);
	  break;

	case pt_mol_exec:
	  gdb_printf (("mode.exec%s%s"),
		      packet->payload.mode.bits.exec.csl
		      ? (" cs.l") : (""),
		      packet->payload.mode.bits.exec.csd
		      ? (" cs.d") : (""));
	  break;

	case pt_mol_tsx:
	  gdb_printf (("mode.tsx%s%s"),
		      packet->payload.mode.bits.tsx.intx
		      ? (" intx") : (""),
		      packet->payload.mode.bits.tsx.abrt
		      ? (" abrt") : (""));
	  break;
	}
      break;

    case ppt_ovf:
      gdb_printf (("ovf"));
      break;

    case ppt_stop:
      gdb_printf (("stop"));
      break;

    case ppt_vmcs:
      gdb_printf (("vmcs %" PRIx64 ""), packet->payload.vmcs.base);
      break;

    case ppt_tma:
      gdb_printf (("tma %x %x"), packet->payload.tma.ctc,
		  packet->payload.tma.fc);
      break;

    case ppt_mtc:
      gdb_printf (("mtc %x"), packet->payload.mtc.ctc);
      break;

    case ppt_cyc:
      gdb_printf (("cyc %" PRIx64 ""), packet->payload.cyc.value);
      break;

    case ppt_mnt:
      gdb_printf (("mnt %" PRIx64 ""), packet->payload.mnt.payload);
      break;
    }
}

/* Decode packets into MAINT using DECODER.  */

static void
btrace_maint_decode_pt (struct btrace_maint_info *maint,
			struct pt_packet_decoder *decoder)
{
  int errcode;

  if (maint->variant.pt.packets == NULL)
    maint->variant.pt.packets = new std::vector<btrace_pt_packet>;

  for (;;)
    {
      struct btrace_pt_packet packet;

      errcode = pt_pkt_sync_forward (decoder);
      if (errcode < 0)
	break;

      for (;;)
	{
	  pt_pkt_get_offset (decoder, &packet.offset);

	  errcode = pt_pkt_next (decoder, &packet.packet,
				 sizeof(packet.packet));
	  if (errcode < 0)
	    break;

	  if (maint_btrace_pt_skip_pad == 0 || packet.packet.type != ppt_pad)
	    {
	      packet.errcode = pt_errcode (errcode);
	      maint->variant.pt.packets->push_back (packet);
	    }
	}

      if (errcode == -pte_eos)
	break;

      packet.errcode = pt_errcode (errcode);
      maint->variant.pt.packets->push_back (packet);

      warning (_("Error at trace offset 0x%" PRIx64 ": %s."),
	       packet.offset, pt_errstr (packet.errcode));
    }

  if (errcode != -pte_eos)
    warning (_("Failed to synchronize onto the Intel Processor Trace "
	       "stream: %s."), pt_errstr (pt_errcode (errcode)));
}

/* Update the packet history in BTINFO.  */

static void
btrace_maint_update_pt_packets (struct btrace_thread_info *btinfo)
{
  struct pt_packet_decoder *decoder;
  const struct btrace_cpu *cpu;
  struct btrace_data_pt *pt;
  struct pt_config config;
  int errcode;

  pt = &btinfo->data.variant.pt;

  /* Nothing to do if there is no trace.  */
  if (pt->size == 0)
    return;

  memset (&config, 0, sizeof(config));

  config.size = sizeof (config);
  config.begin = pt->data;
  config.end = pt->data + pt->size;

  cpu = record_btrace_get_cpu ();
  if (cpu == nullptr)
    cpu = &pt->config.cpu;

  /* We treat an unknown vendor as 'no errata'.  */
  if (cpu->vendor != CV_UNKNOWN)
    {
      config.cpu.vendor = pt_translate_cpu_vendor (cpu->vendor);
      config.cpu.family = cpu->family;
      config.cpu.model = cpu->model;
      config.cpu.stepping = cpu->stepping;

      errcode = pt_cpu_errata (&config.errata, &config.cpu);
      if (errcode < 0)
	error (_("Failed to configure the Intel Processor Trace "
		 "decoder: %s."), pt_errstr (pt_errcode (errcode)));
    }

  decoder = pt_pkt_alloc_decoder (&config);
  if (decoder == NULL)
    error (_("Failed to allocate the Intel Processor Trace decoder."));

  try
    {
      btrace_maint_decode_pt (&btinfo->maint, decoder);
    }
  catch (const gdb_exception &except)
    {
      pt_pkt_free_decoder (decoder);

      if (except.reason < 0)
	throw;
    }

  pt_pkt_free_decoder (decoder);
}

#endif /* !defined (HAVE_LIBIPT)  */

/* Update the packet maintenance information for BTINFO and store the
   low and high bounds into BEGIN and END, respectively.
   Store the current iterator state into FROM and TO.  */

static void
btrace_maint_update_packets (struct btrace_thread_info *btinfo,
			     unsigned int *begin, unsigned int *end,
			     unsigned int *from, unsigned int *to)
{
  switch (btinfo->data.format)
    {
    default:
      *begin = 0;
      *end = 0;
      *from = 0;
      *to = 0;
      break;

    case BTRACE_FORMAT_BTS:
      /* Nothing to do - we operate directly on BTINFO->DATA.  */
      *begin = 0;
      *end = btinfo->data.variant.bts.blocks->size ();
      *from = btinfo->maint.variant.bts.packet_history.begin;
      *to = btinfo->maint.variant.bts.packet_history.end;
      break;

#if defined (HAVE_LIBIPT)
    case BTRACE_FORMAT_PT:
      if (btinfo->maint.variant.pt.packets == nullptr)
	btinfo->maint.variant.pt.packets = new std::vector<btrace_pt_packet>;

      if (btinfo->maint.variant.pt.packets->empty ())
	btrace_maint_update_pt_packets (btinfo);

      *begin = 0;
      *end = btinfo->maint.variant.pt.packets->size ();
      *from = btinfo->maint.variant.pt.packet_history.begin;
      *to = btinfo->maint.variant.pt.packet_history.end;
      break;
#endif /* defined (HAVE_LIBIPT)  */
    }
}

/* Print packets in BTINFO from BEGIN (inclusive) until END (exclusive) and
   update the current iterator position.  */

static void
btrace_maint_print_packets (struct btrace_thread_info *btinfo,
			    unsigned int begin, unsigned int end)
{
  switch (btinfo->data.format)
    {
    default:
      break;

    case BTRACE_FORMAT_BTS:
      {
	const std::vector<btrace_block> &blocks
	  = *btinfo->data.variant.bts.blocks;
	unsigned int blk;

	for (blk = begin; blk < end; ++blk)
	  {
	    const btrace_block &block = blocks.at (blk);

	    gdb_printf ("%u\tbegin: %s, end: %s\n", blk,
			core_addr_to_string_nz (block.begin),
			core_addr_to_string_nz (block.end));
	  }

	btinfo->maint.variant.bts.packet_history.begin = begin;
	btinfo->maint.variant.bts.packet_history.end = end;
      }
      break;

#if defined (HAVE_LIBIPT)
    case BTRACE_FORMAT_PT:
      {
	const std::vector<btrace_pt_packet> &packets
	  = *btinfo->maint.variant.pt.packets;
	unsigned int pkt;

	for (pkt = begin; pkt < end; ++pkt)
	  {
	    const struct btrace_pt_packet &packet = packets.at (pkt);

	    gdb_printf ("%u\t", pkt);
	    gdb_printf ("0x%" PRIx64 "\t", packet.offset);

	    if (packet.errcode == pte_ok)
	      pt_print_packet (&packet.packet);
	    else
	      gdb_printf ("[error: %s]", pt_errstr (packet.errcode));

	    gdb_printf ("\n");
	  }

	btinfo->maint.variant.pt.packet_history.begin = begin;
	btinfo->maint.variant.pt.packet_history.end = end;
      }
      break;
#endif /* defined (HAVE_LIBIPT)  */
    }
}

/* Read a number from an argument string.  */

static unsigned int
get_uint (const char **arg)
{
  const char *begin, *pos;
  char *end;
  unsigned long number;

  begin = *arg;
  pos = skip_spaces (begin);

  if (!isdigit (*pos))
    error (_("Expected positive number, got: %s."), pos);

  number = strtoul (pos, &end, 10);
  if (number > UINT_MAX)
    error (_("Number too big."));

  *arg += (end - begin);

  return (unsigned int) number;
}

/* Read a context size from an argument string.  */

static int
get_context_size (const char **arg)
{
  const char *pos = skip_spaces (*arg);

  if (!isdigit (*pos))
    error (_("Expected positive number, got: %s."), pos);

  char *end;
  long result = strtol (pos, &end, 10);
  *arg = end;
  return result;
}

/* Complain about junk at the end of an argument string.  */

static void
no_chunk (const char *arg)
{
  if (*arg != 0)
    error (_("Junk after argument: %s."), arg);
}

/* The "maintenance btrace packet-history" command.  */

static void
maint_btrace_packet_history_cmd (const char *arg, int from_tty)
{
  struct btrace_thread_info *btinfo;
  unsigned int size, begin, end, from, to;

  thread_info *tp = current_inferior ()->find_thread (inferior_ptid);
  if (tp == NULL)
    error (_("No thread."));

  size = 10;
  btinfo = &tp->btrace;

  btrace_maint_update_packets (btinfo, &begin, &end, &from, &to);
  if (begin == end)
    {
      gdb_printf (_("No trace.\n"));
      return;
    }

  if (arg == NULL || *arg == 0 || strcmp (arg, "+") == 0)
    {
      from = to;

      if (end - from < size)
	size = end - from;
      to = from + size;
    }
  else if (strcmp (arg, "-") == 0)
    {
      to = from;

      if (to - begin < size)
	size = to - begin;
      from = to - size;
    }
  else
    {
      from = get_uint (&arg);
      if (end <= from)
	error (_("'%u' is out of range."), from);

      arg = skip_spaces (arg);
      if (*arg == ',')
	{
	  arg = skip_spaces (++arg);

	  if (*arg == '+')
	    {
	      arg += 1;
	      size = get_context_size (&arg);

	      no_chunk (arg);

	      if (end - from < size)
		size = end - from;
	      to = from + size;
	    }
	  else if (*arg == '-')
	    {
	      arg += 1;
	      size = get_context_size (&arg);

	      no_chunk (arg);

	      /* Include the packet given as first argument.  */
	      from += 1;
	      to = from;

	      if (to - begin < size)
		size = to - begin;
	      from = to - size;
	    }
	  else
	    {
	      to = get_uint (&arg);

	      /* Include the packet at the second argument and silently
		 truncate the range.  */
	      if (to < end)
		to += 1;
	      else
		to = end;

	      no_chunk (arg);
	    }
	}
      else
	{
	  no_chunk (arg);

	  if (end - from < size)
	    size = end - from;
	  to = from + size;
	}

      dont_repeat ();
    }

  btrace_maint_print_packets (btinfo, from, to);
}

/* The "maintenance btrace clear-packet-history" command.  */

static void
maint_btrace_clear_packet_history_cmd (const char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  if (inferior_ptid == null_ptid)
    error (_("No thread."));

  thread_info *tp = inferior_thread ();
  btrace_thread_info *btinfo = &tp->btrace;

  /* Must clear the maint data before - it depends on BTINFO->DATA.  */
  btrace_maint_clear (btinfo);
  btinfo->data.clear ();
}

/* The "maintenance btrace clear" command.  */

static void
maint_btrace_clear_cmd (const char *args, int from_tty)
{
  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  if (inferior_ptid == null_ptid)
    error (_("No thread."));

  thread_info *tp = inferior_thread ();
  btrace_clear (tp);
}

/* The "maintenance info btrace" command.  */

static void
maint_info_btrace_cmd (const char *args, int from_tty)
{
  struct btrace_thread_info *btinfo;
  const struct btrace_config *conf;

  if (args != NULL && *args != 0)
    error (_("Invalid argument."));

  if (inferior_ptid == null_ptid)
    error (_("No thread."));

  thread_info *tp = inferior_thread ();

  btinfo = &tp->btrace;

  conf = btrace_conf (btinfo);
  if (conf == NULL)
    error (_("No btrace configuration."));

  gdb_printf (_("Format: %s.\n"),
	      btrace_format_string (conf->format));

  switch (conf->format)
    {
    default:
      break;

    case BTRACE_FORMAT_BTS:
      gdb_printf (_("Number of packets: %zu.\n"),
		  btinfo->data.variant.bts.blocks->size ());
      break;

#if defined (HAVE_LIBIPT)
    case BTRACE_FORMAT_PT:
      {
	struct pt_version version;

	version = pt_library_version ();
	gdb_printf (_("Version: %u.%u.%u%s.\n"), version.major,
		    version.minor, version.build,
		    version.ext != NULL ? version.ext : "");

	btrace_maint_update_pt_packets (btinfo);
	gdb_printf (_("Number of packets: %zu.\n"),
		    ((btinfo->maint.variant.pt.packets == nullptr)
		     ? 0 : btinfo->maint.variant.pt.packets->size ()));
      }
      break;
#endif /* defined (HAVE_LIBIPT)  */
    }
}

/* The "maint show btrace pt skip-pad" show value function. */

static void
show_maint_btrace_pt_skip_pad  (struct ui_file *file, int from_tty,
				  struct cmd_list_element *c,
				  const char *value)
{
  gdb_printf (file, _("Skip PAD packets is %s.\n"), value);
}


/* Initialize btrace maintenance commands.  */

void _initialize_btrace ();
void
_initialize_btrace ()
{
  add_cmd ("btrace", class_maintenance, maint_info_btrace_cmd,
	   _("Info about branch tracing data."), &maintenanceinfolist);

  add_basic_prefix_cmd ("btrace", class_maintenance,
			_("Branch tracing maintenance commands."),
			&maint_btrace_cmdlist, 0, &maintenancelist);

  add_setshow_prefix_cmd ("btrace", class_maintenance,
			  _("Set branch tracing specific variables."),
			  _("Show branch tracing specific variables."),
			  &maint_btrace_set_cmdlist,
			  &maint_btrace_show_cmdlist,
			  &maintenance_set_cmdlist,
			  &maintenance_show_cmdlist);

  add_setshow_prefix_cmd ("pt", class_maintenance,
			  _("Set Intel Processor Trace specific variables."),
			  _("Show Intel Processor Trace specific variables."),
			  &maint_btrace_pt_set_cmdlist,
			  &maint_btrace_pt_show_cmdlist,
			  &maint_btrace_set_cmdlist,
			  &maint_btrace_show_cmdlist);

  add_setshow_boolean_cmd ("skip-pad", class_maintenance,
			   &maint_btrace_pt_skip_pad, _("\
Set whether PAD packets should be skipped in the btrace packet history."), _("\
Show whether PAD packets should be skipped in the btrace packet history."),_("\
When enabled, PAD packets are ignored in the btrace packet history."),
			   NULL, show_maint_btrace_pt_skip_pad,
			   &maint_btrace_pt_set_cmdlist,
			   &maint_btrace_pt_show_cmdlist);

  add_cmd ("packet-history", class_maintenance, maint_btrace_packet_history_cmd,
	   _("Print the raw branch tracing data.\n\
With no argument, print ten more packets after the previous ten-line print.\n\
With '-' as argument print ten packets before a previous ten-line print.\n\
One argument specifies the starting packet of a ten-line print.\n\
Two arguments with comma between specify starting and ending packets to \
print.\n\
Preceded with '+'/'-' the second argument specifies the distance from the \
first."),
	   &maint_btrace_cmdlist);

  add_cmd ("clear-packet-history", class_maintenance,
	   maint_btrace_clear_packet_history_cmd,
	   _("Clears the branch tracing packet history.\n\
Discards the raw branch tracing data but not the execution history data."),
	   &maint_btrace_cmdlist);

  add_cmd ("clear", class_maintenance, maint_btrace_clear_cmd,
	   _("Clears the branch tracing data.\n\
Discards the raw branch tracing data and the execution history data.\n\
The next 'record' command will fetch the branch tracing data anew."),
	   &maint_btrace_cmdlist);

}
