/* Debug register code for x86 (i386 and x86-64).

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

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

#include "gdbsupport/common-defs.h"
#include "x86-dregs.h"
#include "gdbsupport/break-common.h"

/* Support for hardware watchpoints and breakpoints using the x86
   debug registers.

   This provides several functions for inserting and removing
   hardware-assisted breakpoints and watchpoints, testing if one or
   more of the watchpoints triggered and at what address, checking
   whether a given region can be watched, etc.

   The functions below implement debug registers sharing by reference
   counts, and allow to watch regions up to 16 bytes long.  */

/* Accessor macros for low-level function vector.  */

/* Can we update the inferior's debug registers?  */

static bool
x86_dr_low_can_set_addr ()
{
  return x86_dr_low.set_addr != nullptr;
}

/* Update the inferior's debug register REGNUM from STATE.  */

static void
x86_dr_low_set_addr (struct x86_debug_reg_state *new_state, int i)
{
  x86_dr_low.set_addr (i, new_state->dr_mirror[i]);
}

/* Return the inferior's debug register REGNUM.  */

static CORE_ADDR
x86_dr_low_get_addr (int i)
{
  return x86_dr_low.get_addr (i);
}

/* Can we update the inferior's DR7 control register?  */

static bool
x86_dr_low_can_set_control ()
{
  return x86_dr_low.set_control != nullptr;
}

/* Update the inferior's DR7 debug control register from STATE.  */

static void
x86_dr_low_set_control (struct x86_debug_reg_state *new_state)
{
  x86_dr_low.set_control (new_state->dr_control_mirror);
}

/* Return the value of the inferior's DR7 debug control register.  */

static unsigned long
x86_dr_low_get_control ()
{
  return x86_dr_low.get_control ();
}

/* Return the value of the inferior's DR6 debug status register.  */

static unsigned long
x86_dr_low_get_status ()
{
  return x86_dr_low.get_status ();
}

/* Return the debug register size, in bytes.  */

static int
x86_get_debug_register_length ()
{
  return x86_dr_low.debug_register_length;
}

/* Support for 8-byte wide hw watchpoints.  */
#define TARGET_HAS_DR_LEN_8 (x86_get_debug_register_length () == 8)

/* DR7 Debug Control register fields.  */

/* How many bits to skip in DR7 to get to R/W and LEN fields.  */
#define DR_CONTROL_SHIFT	16
/* How many bits in DR7 per R/W and LEN field for each watchpoint.  */
#define DR_CONTROL_SIZE		4

/* Watchpoint/breakpoint read/write fields in DR7.  */
#define DR_RW_EXECUTE	(0x0)	/* Break on instruction execution.  */
#define DR_RW_WRITE	(0x1)	/* Break on data writes.  */
#define DR_RW_READ	(0x3)	/* Break on data reads or writes.  */

/* This is here for completeness.  No platform supports this
   functionality yet (as of March 2001).  Note that the DE flag in the
   CR4 register needs to be set to support this.  */
#ifndef DR_RW_IORW
#define DR_RW_IORW	(0x2)	/* Break on I/O reads or writes.  */
#endif

/* Watchpoint/breakpoint length fields in DR7.  The 2-bit left shift
   is so we could OR this with the read/write field defined above.  */
#define DR_LEN_1	(0x0 << 2) /* 1-byte region watch or breakpoint.  */
#define DR_LEN_2	(0x1 << 2) /* 2-byte region watch.  */
#define DR_LEN_4	(0x3 << 2) /* 4-byte region watch.  */
#define DR_LEN_8	(0x2 << 2) /* 8-byte region watch (AMD64).  */

/* Local and Global Enable flags in DR7.

   When the Local Enable flag is set, the breakpoint/watchpoint is
   enabled only for the current task; the processor automatically
   clears this flag on every task switch.  When the Global Enable flag
   is set, the breakpoint/watchpoint is enabled for all tasks; the
   processor never clears this flag.

   Currently, all watchpoint are locally enabled.  If you need to
   enable them globally, read the comment which pertains to this in
   x86_insert_aligned_watchpoint below.  */
#define DR_LOCAL_ENABLE_SHIFT	0 /* Extra shift to the local enable bit.  */
#define DR_GLOBAL_ENABLE_SHIFT	1 /* Extra shift to the global enable bit.  */
#define DR_ENABLE_SIZE		2 /* Two enable bits per debug register.  */

/* Local and global exact breakpoint enable flags (a.k.a. slowdown
   flags).  These are only required on i386, to allow detection of the
   exact instruction which caused a watchpoint to break; i486 and
   later processors do that automatically.  We set these flags for
   backwards compatibility.  */
#define DR_LOCAL_SLOWDOWN	(0x100)
#define DR_GLOBAL_SLOWDOWN	(0x200)

/* Fields reserved by Intel.  This includes the GD (General Detect
   Enable) flag, which causes a debug exception to be generated when a
   MOV instruction accesses one of the debug registers.

   FIXME: My Intel manual says we should use 0xF800, not 0xFC00.  */
#define DR_CONTROL_RESERVED	(0xFC00)

/* Auxiliary helper macros.  */

/* A value that masks all fields in DR7 that are reserved by Intel.  */
#define X86_DR_CONTROL_MASK	(~DR_CONTROL_RESERVED)

/* The I'th debug register is vacant if its Local and Global Enable
   bits are reset in the Debug Control register.  */
#define X86_DR_VACANT(state, i) \
  (((state)->dr_control_mirror & (3 << (DR_ENABLE_SIZE * (i)))) == 0)

/* Locally enable the break/watchpoint in the I'th debug register.  */
#define X86_DR_LOCAL_ENABLE(state, i) \
  do { \
    (state)->dr_control_mirror |= \
      (1 << (DR_LOCAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Globally enable the break/watchpoint in the I'th debug register.  */
#define X86_DR_GLOBAL_ENABLE(state, i) \
  do { \
    (state)->dr_control_mirror |= \
      (1 << (DR_GLOBAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Disable the break/watchpoint in the I'th debug register.  */
#define X86_DR_DISABLE(state, i) \
  do { \
    (state)->dr_control_mirror &= \
      ~(3 << (DR_ENABLE_SIZE * (i))); \
  } while (0)

/* Set in DR7 the RW and LEN fields for the I'th debug register.  */
#define X86_DR_SET_RW_LEN(state, i, rwlen) \
  do { \
    (state)->dr_control_mirror &= \
      ~(0x0f << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))); \
    (state)->dr_control_mirror |= \
      ((rwlen) << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))); \
  } while (0)

/* Get from DR7 the RW and LEN fields for the I'th debug register.  */
#define X86_DR_GET_RW_LEN(dr7, i) \
  (((dr7) \
    >> (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (i))) & 0x0f)

/* Did the watchpoint whose address is in the I'th register break?  */
#define X86_DR_WATCH_HIT(dr6, i) ((dr6) & (1 << (i)))

/* Types of operations supported by x86_handle_nonaligned_watchpoint.  */
enum x86_wp_op_t { WP_INSERT, WP_REMOVE, WP_COUNT };

/* Print the values of the mirrored debug registers.  */

static void
x86_show_dr (struct x86_debug_reg_state *state,
	     const char *func, CORE_ADDR addr,
	     int len, enum target_hw_bp_type type)
{
  int i;

  debug_printf ("%s", func);
  if (addr || len)
    debug_printf (" (addr=%s, len=%d, type=%s)",
		  phex (addr, 8), len,
		  type == hw_write ? "data-write"
		  : (type == hw_read ? "data-read"
		     : (type == hw_access ? "data-read/write"
			: (type == hw_execute ? "instruction-execute"
			   /* FIXME: if/when I/O read/write
			      watchpoints are supported, add them
			      here.  */
			   : "??unknown??"))));
  debug_printf (":\n");

  debug_printf ("\tCONTROL (DR7): 0x%s\n", phex (state->dr_control_mirror, 8));
  debug_printf ("\tSTATUS (DR6): 0x%s\n", phex (state->dr_status_mirror, 8));

  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      debug_printf ("\tDR%d: addr=0x%s, ref.count=%d\n",
		    i, phex (state->dr_mirror[i],
			     x86_get_debug_register_length ()),
		    state->dr_ref_count[i]);
    }
}

/* Return the value of a 4-bit field for DR7 suitable for watching a
   region of LEN bytes for accesses of type TYPE.  LEN is assumed to
   have the value of 1, 2, or 4.  */

static unsigned
x86_length_and_rw_bits (int len, enum target_hw_bp_type type)
{
  unsigned rw;

  switch (type)
    {
      case hw_execute:
	rw = DR_RW_EXECUTE;
	break;
      case hw_write:
	rw = DR_RW_WRITE;
	break;
      case hw_read:
	internal_error (_("The i386 doesn't support "
			  "data-read watchpoints.\n"));
      case hw_access:
	rw = DR_RW_READ;
	break;
#if 0
	/* Not yet supported.  */
      case hw_io_access:
	rw = DR_RW_IORW;
	break;
#endif
      default:
	internal_error (_("\
Invalid hardware breakpoint type %d in x86_length_and_rw_bits.\n"),
			(int) type);
    }

  switch (len)
    {
      case 1:
	return (DR_LEN_1 | rw);
      case 2:
	return (DR_LEN_2 | rw);
      case 4:
	return (DR_LEN_4 | rw);
      case 8:
	if (TARGET_HAS_DR_LEN_8)
	  return (DR_LEN_8 | rw);
	[[fallthrough]];
      default:
	internal_error (_("\
Invalid hardware breakpoint length %d in x86_length_and_rw_bits.\n"), len);
    }
}

/* Insert a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region to be watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */

static int
x86_insert_aligned_watchpoint (struct x86_debug_reg_state *state,
			       CORE_ADDR addr, unsigned len_rw_bits)
{
  int i;

  if (!x86_dr_low_can_set_addr () || !x86_dr_low_can_set_control ())
    return -1;

  /* First, look for an occupied debug register with the same address
     and the same RW and LEN definitions.  If we find one, we can
     reuse it for this watchpoint as well (and save a register).  */
  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      if (!X86_DR_VACANT (state, i)
	  && state->dr_mirror[i] == addr
	  && X86_DR_GET_RW_LEN (state->dr_control_mirror, i) == len_rw_bits)
	{
	  state->dr_ref_count[i]++;
	  return 0;
	}
    }

  /* Next, look for a vacant debug register.  */
  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      if (X86_DR_VACANT (state, i))
	break;
    }

  /* No more debug registers!  */
  if (i >= DR_NADDR)
    return -1;

  /* Now set up the register I to watch our region.  */

  /* Record the info in our local mirrored array.  */
  state->dr_mirror[i] = addr;
  state->dr_ref_count[i] = 1;
  X86_DR_SET_RW_LEN (state, i, len_rw_bits);
  /* Note: we only enable the watchpoint locally, i.e. in the current
     task.  Currently, no x86 target allows or supports global
     watchpoints; however, if any target would want that in the
     future, GDB should probably provide a command to control whether
     to enable watchpoints globally or locally, and the code below
     should use global or local enable and slow-down flags as
     appropriate.  */
  X86_DR_LOCAL_ENABLE (state, i);
  state->dr_control_mirror |= DR_LOCAL_SLOWDOWN;
  state->dr_control_mirror &= X86_DR_CONTROL_MASK;

  return 0;
}

/* Remove a watchpoint at address ADDR, which is assumed to be aligned
   according to the length of the region to watch.  LEN_RW_BITS is the
   value of the bits from DR7 which describes the length and access
   type of the region watched by this watchpoint.  Return 0 on
   success, -1 on failure.  */

static int
x86_remove_aligned_watchpoint (struct x86_debug_reg_state *state,
			       CORE_ADDR addr, unsigned len_rw_bits)
{
  int i, retval = -1;
  int all_vacant = 1;

  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      if (!X86_DR_VACANT (state, i)
	  && state->dr_mirror[i] == addr
	  && X86_DR_GET_RW_LEN (state->dr_control_mirror, i) == len_rw_bits)
	{
	  if (--state->dr_ref_count[i] == 0) /* No longer in use?  */
	    {
	      /* Reset our mirror.  */
	      state->dr_mirror[i] = 0;
	      X86_DR_DISABLE (state, i);
	      /* Even though not strictly necessary, clear out all
		 bits in DR_CONTROL related to this debug register.
		 Debug output is clearer when we don't have stale bits
		 in place.  This also allows the assertion below.  */
	      X86_DR_SET_RW_LEN (state, i, 0);
	    }
	  retval = 0;
	}

      if (!X86_DR_VACANT (state, i))
	all_vacant = 0;
    }

  if (all_vacant)
    {
      /* Even though not strictly necessary, clear out all of
	 DR_CONTROL, so that when we have no debug registers in use,
	 we end up with DR_CONTROL == 0.  The Linux support relies on
	 this for an optimization.  Plus, it makes for clearer debug
	 output.  */
      state->dr_control_mirror &= ~DR_LOCAL_SLOWDOWN;

      gdb_assert (state->dr_control_mirror == 0);
    }
  return retval;
}

/* Insert or remove a (possibly non-aligned) watchpoint, or count the
   number of debug registers required to watch a region at address
   ADDR whose length is LEN for accesses of type TYPE.  Return 0 on
   successful insertion or removal, a positive number when queried
   about the number of registers, or -1 on failure.  If WHAT is not a
   valid value, bombs through internal_error.  */

static int
x86_handle_nonaligned_watchpoint (struct x86_debug_reg_state *state,
				  x86_wp_op_t what, CORE_ADDR addr, int len,
				  enum target_hw_bp_type type)
{
  int retval = 0;
  int max_wp_len = TARGET_HAS_DR_LEN_8 ? 8 : 4;

  static const int size_try_array[8][8] =
  {
    {1, 1, 1, 1, 1, 1, 1, 1},	/* Trying size one.  */
    {2, 1, 2, 1, 2, 1, 2, 1},	/* Trying size two.  */
    {2, 1, 2, 1, 2, 1, 2, 1},	/* Trying size three.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size four.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size five.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size six.  */
    {4, 1, 2, 1, 4, 1, 2, 1},	/* Trying size seven.  */
    {8, 1, 2, 1, 4, 1, 2, 1},	/* Trying size eight.  */
  };

  while (len > 0)
    {
      int align = addr % max_wp_len;
      /* Four (eight on AMD64) is the maximum length a debug register
	 can watch.  */
      int attempt = (len > max_wp_len ? (max_wp_len - 1) : len - 1);
      int size = size_try_array[attempt][align];

      if (what == WP_COUNT)
	{
	  /* size_try_array[] is defined such that each iteration
	     through the loop is guaranteed to produce an address and a
	     size that can be watched with a single debug register.
	     Thus, for counting the registers required to watch a
	     region, we simply need to increment the count on each
	     iteration.  */
	  retval++;
	}
      else
	{
	  unsigned len_rw = x86_length_and_rw_bits (size, type);

	  if (what == WP_INSERT)
	    retval = x86_insert_aligned_watchpoint (state, addr, len_rw);
	  else if (what == WP_REMOVE)
	    retval = x86_remove_aligned_watchpoint (state, addr, len_rw);
	  else
	    internal_error (_("\
Invalid value %d of operation in x86_handle_nonaligned_watchpoint.\n"),
			    (int) what);
	  if (retval)
	    break;
	}

      addr += size;
      len -= size;
    }

  return retval;
}

/* Update the inferior debug registers state, in STATE, with the
   new debug registers state, in NEW_STATE.  */

static void
x86_update_inferior_debug_regs (struct x86_debug_reg_state *state,
				struct x86_debug_reg_state *new_state)
{
  int i;

  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      if (X86_DR_VACANT (new_state, i) != X86_DR_VACANT (state, i))
	x86_dr_low_set_addr (new_state, i);
      else
	gdb_assert (new_state->dr_mirror[i] == state->dr_mirror[i]);
    }

  if (new_state->dr_control_mirror != state->dr_control_mirror)
    x86_dr_low_set_control (new_state);

  *state = *new_state;
}

/* Insert a watchpoint to watch a memory region which starts at
   address ADDR and whose length is LEN bytes.  Watch memory accesses
   of the type TYPE.  Return 0 on success, -1 on failure.  */

int
x86_dr_insert_watchpoint (struct x86_debug_reg_state *state,
			  enum target_hw_bp_type type,
			  CORE_ADDR addr, int len)
{
  int retval;
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct x86_debug_reg_state local_state = *state;

  if (type == hw_read)
    return 1; /* unsupported */

  if (((len != 1 && len != 2 && len != 4)
       && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0)
    {
      retval = x86_handle_nonaligned_watchpoint (&local_state,
						 WP_INSERT,
						 addr, len, type);
    }
  else
    {
      unsigned len_rw = x86_length_and_rw_bits (len, type);

      retval = x86_insert_aligned_watchpoint (&local_state,
					      addr, len_rw);
    }

  if (retval == 0)
    x86_update_inferior_debug_regs (state, &local_state);

  if (show_debug_regs)
    x86_show_dr (state, "insert_watchpoint", addr, len, type);

  return retval;
}

/* Remove a watchpoint that watched the memory region which starts at
   address ADDR, whose length is LEN bytes, and for accesses of the
   type TYPE.  Return 0 on success, -1 on failure.  */

int
x86_dr_remove_watchpoint (struct x86_debug_reg_state *state,
			  enum target_hw_bp_type type,
			  CORE_ADDR addr, int len)
{
  int retval;
  /* Work on a local copy of the debug registers, and on success,
     commit the change back to the inferior.  */
  struct x86_debug_reg_state local_state = *state;

  if (((len != 1 && len != 2 && len != 4)
       && !(TARGET_HAS_DR_LEN_8 && len == 8))
      || addr % len != 0)
    {
      retval = x86_handle_nonaligned_watchpoint (&local_state,
						 WP_REMOVE,
						 addr, len, type);
    }
  else
    {
      unsigned len_rw = x86_length_and_rw_bits (len, type);

      retval = x86_remove_aligned_watchpoint (&local_state,
					      addr, len_rw);
    }

  if (retval == 0)
    x86_update_inferior_debug_regs (state, &local_state);

  if (show_debug_regs)
    x86_show_dr (state, "remove_watchpoint", addr, len, type);

  return retval;
}

/* Return non-zero if we can watch a memory region that starts at
   address ADDR and whose length is LEN bytes.  */

int
x86_dr_region_ok_for_watchpoint (struct x86_debug_reg_state *state,
				 CORE_ADDR addr, int len)
{
  int nregs;

  /* Compute how many aligned watchpoints we would need to cover this
     region.  */
  nregs = x86_handle_nonaligned_watchpoint (state, WP_COUNT,
					     addr, len, hw_write);
  return nregs <= DR_NADDR ? 1 : 0;
}

/* If the inferior has some break/watchpoint that triggered, set the
   address associated with that break/watchpoint and return non-zero.
   Otherwise, return zero.  */

int
x86_dr_stopped_data_address (struct x86_debug_reg_state *state,
			     CORE_ADDR *addr_p)
{
  CORE_ADDR addr = 0;
  int i;
  int rc = 0;
  /* The current thread's DR_STATUS.  We always need to read this to
     check whether some watchpoint caused the trap.  */
  unsigned status;
  /* We need DR_CONTROL as well, but only iff DR_STATUS indicates a
     data breakpoint trap.  Only fetch it when necessary, to avoid an
     unnecessary extra syscall when no watchpoint triggered.  */
  int control_p = 0;
  unsigned control = 0;

  /* In non-stop/async, threads can be running while we change the
     global dr_mirror (and friends).  Say, we set a watchpoint, and
     let threads resume.  Now, say you delete the watchpoint, or
     add/remove watchpoints such that dr_mirror changes while threads
     are running.  On targets that support non-stop,
     inserting/deleting watchpoints updates the global dr_mirror only.
     It does not update the real thread's debug registers; that's only
     done prior to resume.  Instead, if threads are running when the
     mirror changes, a temporary and transparent stop on all threads
     is forced so they can get their copy of the debug registers
     updated on re-resume.  Now, say, a thread hit a watchpoint before
     having been updated with the new dr_mirror contents, and we
     haven't yet handled the corresponding SIGTRAP.  If we trusted
     dr_mirror below, we'd mistake the real trapped address (from the
     last time we had updated debug registers in the thread) with
     whatever was currently in dr_mirror.  So to fix this, dr_mirror
     always represents intention, what we _want_ threads to have in
     debug registers.  To get at the address and cause of the trap, we
     need to read the state the thread still has in its debug
     registers.

     In sum, always get the current debug register values the current
     thread has, instead of trusting the global mirror.  If the thread
     was running when we last changed watchpoints, the mirror no
     longer represents what was set in this thread's debug
     registers.  */
  status = x86_dr_low_get_status ();

  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      if (!X86_DR_WATCH_HIT (status, i))
	continue;

      if (!control_p)
	{
	  control = x86_dr_low_get_control ();
	  control_p = 1;
	}

      /* This second condition makes sure DRi is set up for a data
	 watchpoint, not a hardware breakpoint.  The reason is that
	 GDB doesn't call the target_stopped_data_address method
	 except for data watchpoints.  In other words, I'm being
	 paranoiac.  */
      if (X86_DR_GET_RW_LEN (control, i) != 0)
	{
	  addr = x86_dr_low_get_addr (i);
	  rc = 1;
	  if (show_debug_regs)
	    x86_show_dr (state, "watchpoint_hit", addr, -1, hw_write);
	}
    }

  if (show_debug_regs && addr == 0)
    x86_show_dr (state, "stopped_data_addr", 0, 0, hw_write);

  if (rc)
    *addr_p = addr;
  return rc;
}

/* Return non-zero if the inferior has some watchpoint that triggered.
   Otherwise return zero.  */

int
x86_dr_stopped_by_watchpoint (struct x86_debug_reg_state *state)
{
  CORE_ADDR addr = 0;
  return x86_dr_stopped_data_address (state, &addr);
}

/* Return non-zero if the inferior has some hardware breakpoint that
   triggered.  Otherwise return zero.  */

int
x86_dr_stopped_by_hw_breakpoint (struct x86_debug_reg_state *state)
{
  CORE_ADDR addr = 0;
  int i;
  int rc = 0;
  /* The current thread's DR_STATUS.  We always need to read this to
     check whether some watchpoint caused the trap.  */
  unsigned status;
  /* We need DR_CONTROL as well, but only iff DR_STATUS indicates a
     breakpoint trap.  Only fetch it when necessary, to avoid an
     unnecessary extra syscall when no watchpoint triggered.  */
  int control_p = 0;
  unsigned control = 0;

  /* As above, always read the current thread's debug registers rather
     than trusting dr_mirror.  */
  status = x86_dr_low_get_status ();

  ALL_DEBUG_ADDRESS_REGISTERS (i)
    {
      if (!X86_DR_WATCH_HIT (status, i))
	continue;

      if (!control_p)
	{
	  control = x86_dr_low_get_control ();
	  control_p = 1;
	}

      if (X86_DR_GET_RW_LEN (control, i) == 0)
	{
	  addr = x86_dr_low_get_addr (i);
	  rc = 1;
	  if (show_debug_regs)
	    x86_show_dr (state, "watchpoint_hit", addr, -1, hw_execute);
	}
    }

  return rc;
}
