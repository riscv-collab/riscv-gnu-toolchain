/* Native-dependent code for GNU/Linux x86 (i386 and x86-64).

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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
#include "inferior.h"
#include "elf/common.h"
#include "gdb_proc_service.h"
#include "nat/gdb_ptrace.h"
#include <sys/user.h>
#include <sys/procfs.h>
#include <sys/uio.h>

#include "x86-nat.h"
#ifndef __x86_64__
#include "i386-linux-nat.h"
#endif
#include "x86-linux-nat.h"
#include "i386-linux-tdep.h"
#ifdef __x86_64__
#include "amd64-linux-tdep.h"
#endif
#include "gdbsupport/x86-xstate.h"
#include "nat/x86-xstate.h"
#include "nat/linux-btrace.h"
#include "nat/linux-nat.h"
#include "nat/x86-linux.h"
#include "nat/x86-linux-dregs.h"
#include "nat/linux-ptrace.h"

/* linux_nat_target::low_new_fork implementation.  */

void
x86_linux_nat_target::low_new_fork (struct lwp_info *parent, pid_t child_pid)
{
  pid_t parent_pid;
  struct x86_debug_reg_state *parent_state;
  struct x86_debug_reg_state *child_state;

  /* NULL means no watchpoint has ever been set in the parent.  In
     that case, there's nothing to do.  */
  if (parent->arch_private == NULL)
    return;

  /* Linux kernel before 2.6.33 commit
     72f674d203cd230426437cdcf7dd6f681dad8b0d
     will inherit hardware debug registers from parent
     on fork/vfork/clone.  Newer Linux kernels create such tasks with
     zeroed debug registers.

     GDB core assumes the child inherits the watchpoints/hw
     breakpoints of the parent, and will remove them all from the
     forked off process.  Copy the debug registers mirrors into the
     new process so that all breakpoints and watchpoints can be
     removed together.  The debug registers mirror will become zeroed
     in the end before detaching the forked off process, thus making
     this compatible with older Linux kernels too.  */

  parent_pid = parent->ptid.pid ();
  parent_state = x86_debug_reg_state (parent_pid);
  child_state = x86_debug_reg_state (child_pid);
  *child_state = *parent_state;
}


x86_linux_nat_target::~x86_linux_nat_target ()
{
}

/* Implement the virtual inf_ptrace_target::post_startup_inferior method.  */

void
x86_linux_nat_target::post_startup_inferior (ptid_t ptid)
{
  x86_cleanup_dregs ();
  linux_nat_target::post_startup_inferior (ptid);
}

#ifdef __x86_64__
/* Value of CS segment register:
     64bit process: 0x33
     32bit process: 0x23  */
#define AMD64_LINUX_USER64_CS 0x33

/* Value of DS segment register:
     LP64 process: 0x0
     X32 process: 0x2b  */
#define AMD64_LINUX_X32_DS 0x2b
#endif

/* Get Linux/x86 target description from running target.  */

const struct target_desc *
x86_linux_nat_target::read_description ()
{
  int tid;
  int is_64bit = 0;
#ifdef __x86_64__
  int is_x32;
#endif
  static uint64_t xcr0;
  uint64_t xcr0_features_bits;

  if (inferior_ptid == null_ptid)
    return this->beneath ()->read_description ();

  tid = inferior_ptid.pid ();

#ifdef __x86_64__
  {
    unsigned long cs;
    unsigned long ds;

    /* Get CS register.  */
    errno = 0;
    cs = ptrace (PTRACE_PEEKUSER, tid,
		 offsetof (struct user_regs_struct, cs), 0);
    if (errno != 0)
      perror_with_name (_("Couldn't get CS register"));

    is_64bit = cs == AMD64_LINUX_USER64_CS;

    /* Get DS register.  */
    errno = 0;
    ds = ptrace (PTRACE_PEEKUSER, tid,
		 offsetof (struct user_regs_struct, ds), 0);
    if (errno != 0)
      perror_with_name (_("Couldn't get DS register"));

    is_x32 = ds == AMD64_LINUX_X32_DS;

    if (sizeof (void *) == 4 && is_64bit && !is_x32)
      error (_("Can't debug 64-bit process with 32-bit GDB"));
  }
#elif HAVE_PTRACE_GETFPXREGS
  if (have_ptrace_getfpxregs == -1)
    {
      elf_fpxregset_t fpxregs;

      if (ptrace (PTRACE_GETFPXREGS, tid, 0, (int) &fpxregs) < 0)
	{
	  have_ptrace_getfpxregs = 0;
	  have_ptrace_getregset = TRIBOOL_FALSE;
	  return i386_linux_read_description (X86_XSTATE_X87_MASK);
	}
    }
#endif

  if (have_ptrace_getregset == TRIBOOL_UNKNOWN)
    {
      uint64_t xstateregs[(X86_XSTATE_SSE_SIZE / sizeof (uint64_t))];
      struct iovec iov;

      iov.iov_base = xstateregs;
      iov.iov_len = sizeof (xstateregs);

      /* Check if PTRACE_GETREGSET works.  */
      if (ptrace (PTRACE_GETREGSET, tid,
		  (unsigned int) NT_X86_XSTATE, &iov) < 0)
	have_ptrace_getregset = TRIBOOL_FALSE;
      else
	{
	  have_ptrace_getregset = TRIBOOL_TRUE;

	  /* Get XCR0 from XSAVE extended state.  */
	  xcr0 = xstateregs[(I386_LINUX_XSAVE_XCR0_OFFSET
			     / sizeof (uint64_t))];

	  m_xsave_layout = x86_fetch_xsave_layout (xcr0, x86_xsave_length ());
	}
    }

  /* Check the native XCR0 only if PTRACE_GETREGSET is available.  If
     PTRACE_GETREGSET is not available then set xcr0_features_bits to
     zero so that the "no-features" descriptions are returned by the
     switches below.  */
  if (have_ptrace_getregset == TRIBOOL_TRUE)
    xcr0_features_bits = xcr0 & X86_XSTATE_ALL_MASK;
  else
    xcr0_features_bits = 0;

  if (is_64bit)
    {
#ifdef __x86_64__
      return amd64_linux_read_description (xcr0_features_bits, is_x32);
#endif
    }
  else
    {
      const struct target_desc * tdesc
	= i386_linux_read_description (xcr0_features_bits);

      if (tdesc == NULL)
	tdesc = i386_linux_read_description (X86_XSTATE_SSE_MASK);

      return tdesc;
    }

  gdb_assert_not_reached ("failed to return tdesc");
}


/* Enable branch tracing.  */

struct btrace_target_info *
x86_linux_nat_target::enable_btrace (thread_info *tp,
				     const struct btrace_config *conf)
{
  struct btrace_target_info *tinfo = nullptr;
  ptid_t ptid = tp->ptid;
  try
    {
      tinfo = linux_enable_btrace (ptid, conf);
    }
  catch (const gdb_exception_error &exception)
    {
      error (_("Could not enable branch tracing for %s: %s"),
	     target_pid_to_str (ptid).c_str (), exception.what ());
    }

  return tinfo;
}

/* Disable branch tracing.  */

void
x86_linux_nat_target::disable_btrace (struct btrace_target_info *tinfo)
{
  enum btrace_error errcode = linux_disable_btrace (tinfo);

  if (errcode != BTRACE_ERR_NONE)
    error (_("Could not disable branch tracing."));
}

/* Teardown branch tracing.  */

void
x86_linux_nat_target::teardown_btrace (struct btrace_target_info *tinfo)
{
  /* Ignore errors.  */
  linux_disable_btrace (tinfo);
}

enum btrace_error
x86_linux_nat_target::read_btrace (struct btrace_data *data,
				   struct btrace_target_info *btinfo,
				   enum btrace_read_type type)
{
  return linux_read_btrace (data, btinfo, type);
}

/* See to_btrace_conf in target.h.  */

const struct btrace_config *
x86_linux_nat_target::btrace_conf (const struct btrace_target_info *btinfo)
{
  return linux_btrace_conf (btinfo);
}



/* Helper for ps_get_thread_area.  Sets BASE_ADDR to a pointer to
   the thread local storage (or its descriptor) and returns PS_OK
   on success.  Returns PS_ERR on failure.  */

ps_err_e
x86_linux_get_thread_area (pid_t pid, void *addr, unsigned int *base_addr)
{
  /* NOTE: cagney/2003-08-26: The definition of this buffer is found
     in the kernel header <asm-i386/ldt.h>.  It, after padding, is 4 x
     4 byte integers in size: `entry_number', `base_addr', `limit',
     and a bunch of status bits.

     The values returned by this ptrace call should be part of the
     regcache buffer, and ps_get_thread_area should channel its
     request through the regcache.  That way remote targets could
     provide the value using the remote protocol and not this direct
     call.

     Is this function needed?  I'm guessing that the `base' is the
     address of a descriptor that libthread_db uses to find the
     thread local address base that GDB needs.  Perhaps that
     descriptor is defined by the ABI.  Anyway, given that
     libthread_db calls this function without prompting (gdb
     requesting tls base) I guess it needs info in there anyway.  */
  unsigned int desc[4];

  /* This code assumes that "int" is 32 bits and that
     GET_THREAD_AREA returns no more than 4 int values.  */
  gdb_assert (sizeof (int) == 4);

#ifndef PTRACE_GET_THREAD_AREA
#define PTRACE_GET_THREAD_AREA 25
#endif

  if (ptrace (PTRACE_GET_THREAD_AREA, pid, addr, &desc) < 0)
    return PS_ERR;

  *base_addr = desc[1];
  return PS_OK;
}


void _initialize_x86_linux_nat ();
void
_initialize_x86_linux_nat ()
{
  /* Initialize the debug register function vectors.  */
  x86_dr_low.set_control = x86_linux_dr_set_control;
  x86_dr_low.set_addr = x86_linux_dr_set_addr;
  x86_dr_low.get_addr = x86_linux_dr_get_addr;
  x86_dr_low.get_status = x86_linux_dr_get_status;
  x86_dr_low.get_control = x86_linux_dr_get_control;
  x86_set_debug_register_length (sizeof (void *));
}
