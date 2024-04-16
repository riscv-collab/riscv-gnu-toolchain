/* Target-dependent code for GNU/Linux i386.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "frame.h"
#include "value.h"
#include "regcache.h"
#include "regset.h"
#include "inferior.h"
#include "osabi.h"
#include "reggroups.h"
#include "dwarf2/frame.h"
#include "i386-tdep.h"
#include "i386-linux-tdep.h"
#include "linux-tdep.h"
#include "utils.h"
#include "glibc-tdep.h"
#include "solib-svr4.h"
#include "symtab.h"
#include "arch-utils.h"
#include "xml-syscall.h"
#include "infrun.h"

#include "i387-tdep.h"
#include "gdbsupport/x86-xstate.h"

/* The syscall's XML filename for i386.  */
#define XML_SYSCALL_FILENAME_I386 "syscalls/i386-linux.xml"

#include "record-full.h"
#include "linux-record.h"

#include "arch/i386.h"
#include "target-descriptions.h"

/* Return non-zero, when the register is in the corresponding register
   group.  Put the LINUX_ORIG_EAX register in the system group.  */
static int
i386_linux_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				const struct reggroup *group)
{
  if (regnum == I386_LINUX_ORIG_EAX_REGNUM)
    return (group == system_reggroup
	    || group == save_reggroup
	    || group == restore_reggroup);
  return i386_register_reggroup_p (gdbarch, regnum, group);
}


/* Recognizing signal handler frames.  */

/* GNU/Linux has two flavors of signals.  Normal signal handlers, and
   "realtime" (RT) signals.  The RT signals can provide additional
   information to the signal handler if the SA_SIGINFO flag is set
   when establishing a signal handler using `sigaction'.  It is not
   unlikely that future versions of GNU/Linux will support SA_SIGINFO
   for normal signals too.  */

/* When the i386 Linux kernel calls a signal handler and the
   SA_RESTORER flag isn't set, the return address points to a bit of
   code on the stack.  This function returns whether the PC appears to
   be within this bit of code.

   The instruction sequence for normal signals is
       pop    %eax
       mov    $0x77, %eax
       int    $0x80
   or 0x58 0xb8 0x77 0x00 0x00 0x00 0xcd 0x80.

   Checking for the code sequence should be somewhat reliable, because
   the effect is to call the system call sigreturn.  This is unlikely
   to occur anywhere other than in a signal trampoline.

   It kind of sucks that we have to read memory from the process in
   order to identify a signal trampoline, but there doesn't seem to be
   any other way.  Therefore we only do the memory reads if no
   function name could be identified, which should be the case since
   the code is on the stack.

   Detection of signal trampolines for handlers that set the
   SA_RESTORER flag is in general not possible.  Unfortunately this is
   what the GNU C Library has been doing for quite some time now.
   However, as of version 2.1.2, the GNU C Library uses signal
   trampolines (named __restore and __restore_rt) that are identical
   to the ones used by the kernel.  Therefore, these trampolines are
   supported too.  */

#define LINUX_SIGTRAMP_INSN0	0x58	/* pop %eax */
#define LINUX_SIGTRAMP_OFFSET0	0
#define LINUX_SIGTRAMP_INSN1	0xb8	/* mov $NNNN, %eax */
#define LINUX_SIGTRAMP_OFFSET1	1
#define LINUX_SIGTRAMP_INSN2	0xcd	/* int */
#define LINUX_SIGTRAMP_OFFSET2	6

static const gdb_byte linux_sigtramp_code[] =
{
  LINUX_SIGTRAMP_INSN0,					/* pop %eax */
  LINUX_SIGTRAMP_INSN1, 0x77, 0x00, 0x00, 0x00,		/* mov $0x77, %eax */
  LINUX_SIGTRAMP_INSN2, 0x80				/* int $0x80 */
};

#define LINUX_SIGTRAMP_LEN (sizeof linux_sigtramp_code)

/* If THIS_FRAME is a sigtramp routine, return the address of the
   start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_linux_sigtramp_start (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[LINUX_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the three instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (!safe_frame_unwind_memory (this_frame, pc, buf))
    return 0;

  if (buf[0] != LINUX_SIGTRAMP_INSN0)
    {
      int adjust;

      switch (buf[0])
	{
	case LINUX_SIGTRAMP_INSN1:
	  adjust = LINUX_SIGTRAMP_OFFSET1;
	  break;
	case LINUX_SIGTRAMP_INSN2:
	  adjust = LINUX_SIGTRAMP_OFFSET2;
	  break;
	default:
	  return 0;
	}

      pc -= adjust;

      if (!safe_frame_unwind_memory (this_frame, pc, buf))
	return 0;
    }

  if (memcmp (buf, linux_sigtramp_code, LINUX_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* This function does the same for RT signals.  Here the instruction
   sequence is
       mov    $0xad, %eax
       int    $0x80
   or 0xb8 0xad 0x00 0x00 0x00 0xcd 0x80.

   The effect is to call the system call rt_sigreturn.  */

#define LINUX_RT_SIGTRAMP_INSN0		0xb8 /* mov $NNNN, %eax */
#define LINUX_RT_SIGTRAMP_OFFSET0	0
#define LINUX_RT_SIGTRAMP_INSN1		0xcd /* int */
#define LINUX_RT_SIGTRAMP_OFFSET1	5

static const gdb_byte linux_rt_sigtramp_code[] =
{
  LINUX_RT_SIGTRAMP_INSN0, 0xad, 0x00, 0x00, 0x00,	/* mov $0xad, %eax */
  LINUX_RT_SIGTRAMP_INSN1, 0x80				/* int $0x80 */
};

#define LINUX_RT_SIGTRAMP_LEN (sizeof linux_rt_sigtramp_code)

/* If THIS_FRAME is an RT sigtramp routine, return the address of the
   start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_linux_rt_sigtramp_start (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[LINUX_RT_SIGTRAMP_LEN];

  /* We only recognize a signal trampoline if PC is at the start of
     one of the two instructions.  We optimize for finding the PC at
     the start, as will be the case when the trampoline is not the
     first frame on the stack.  We assume that in the case where the
     PC is not at the start of the instruction sequence, there will be
     a few trailing readable bytes on the stack.  */

  if (!safe_frame_unwind_memory (this_frame, pc, buf))
    return 0;

  if (buf[0] != LINUX_RT_SIGTRAMP_INSN0)
    {
      if (buf[0] != LINUX_RT_SIGTRAMP_INSN1)
	return 0;

      pc -= LINUX_RT_SIGTRAMP_OFFSET1;

      if (!safe_frame_unwind_memory (this_frame, pc,
				     buf))
	return 0;
    }

  if (memcmp (buf, linux_rt_sigtramp_code, LINUX_RT_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* Return whether THIS_FRAME corresponds to a GNU/Linux sigtramp
   routine.  */

static int
i386_linux_sigtramp_p (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If we have NAME, we can optimize the search.  The trampolines are
     named __restore and __restore_rt.  However, they aren't dynamically
     exported from the shared C library, so the trampoline may appear to
     be part of the preceding function.  This should always be sigaction,
     __sigaction, or __libc_sigaction (all aliases to the same function).  */
  if (name == NULL || strstr (name, "sigaction") != NULL)
    return (i386_linux_sigtramp_start (this_frame) != 0
	    || i386_linux_rt_sigtramp_start (this_frame) != 0);

  return (strcmp ("__restore", name) == 0
	  || strcmp ("__restore_rt", name) == 0);
}

/* Return one if the PC of THIS_FRAME is in a signal trampoline which
   may have DWARF-2 CFI.  */

static int
i386_linux_dwarf_signal_frame_p (struct gdbarch *gdbarch,
				 frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If a vsyscall DSO is in use, the signal trampolines may have these
     names.  */
  if (name && (strcmp (name, "__kernel_sigreturn") == 0
	       || strcmp (name, "__kernel_rt_sigreturn") == 0))
    return 1;

  return 0;
}

/* Offset to struct sigcontext in ucontext, from <asm/ucontext.h>.  */
#define I386_LINUX_UCONTEXT_SIGCONTEXT_OFFSET 20

/* Assuming THIS_FRAME is a GNU/Linux sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
i386_linux_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc;
  CORE_ADDR sp;
  gdb_byte buf[4];

  get_frame_register (this_frame, I386_ESP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4, byte_order);

  pc = i386_linux_sigtramp_start (this_frame);
  if (pc)
    {
      /* The sigcontext structure lives on the stack, right after
	 the signum argument.  We determine the address of the
	 sigcontext structure by looking at the frame's stack
	 pointer.  Keep in mind that the first instruction of the
	 sigtramp code is "pop %eax".  If the PC is after this
	 instruction, adjust the returned value accordingly.  */
      if (pc == get_frame_pc (this_frame))
	return sp + 4;
      return sp;
    }

  pc = i386_linux_rt_sigtramp_start (this_frame);
  if (pc)
    {
      CORE_ADDR ucontext_addr;

      /* The sigcontext structure is part of the user context.  A
	 pointer to the user context is passed as the third argument
	 to the signal handler.  */
      read_memory (sp + 8, buf, 4);
      ucontext_addr = extract_unsigned_integer (buf, 4, byte_order);
      return ucontext_addr + I386_LINUX_UCONTEXT_SIGCONTEXT_OFFSET;
    }

  error (_("Couldn't recognize signal trampoline."));
  return 0;
}

/* Set the program counter for process PTID to PC.  */

static void
i386_linux_write_pc (struct regcache *regcache, CORE_ADDR pc)
{
  regcache_cooked_write_unsigned (regcache, I386_EIP_REGNUM, pc);

  /* We must be careful with modifying the program counter.  If we
     just interrupted a system call, the kernel might try to restart
     it when we resume the inferior.  On restarting the system call,
     the kernel will try backing up the program counter even though it
     no longer points at the system call.  This typically results in a
     SIGSEGV or SIGILL.  We can prevent this by writing `-1' in the
     "orig_eax" pseudo-register.

     Note that "orig_eax" is saved when setting up a dummy call frame.
     This means that it is properly restored when that frame is
     popped, and that the interrupted system call will be restarted
     when we resume the inferior on return from a function call from
     within GDB.  In all other cases the system call will not be
     restarted.  */
  regcache_cooked_write_unsigned (regcache, I386_LINUX_ORIG_EAX_REGNUM, -1);
}

/* Record all registers but IP register for process-record.  */

static int
i386_all_but_ip_registers_record (struct regcache *regcache)
{
  if (record_full_arch_list_add_reg (regcache, I386_EAX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_ECX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EDX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EBX_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_ESP_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EBP_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_ESI_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EDI_REGNUM))
    return -1;
  if (record_full_arch_list_add_reg (regcache, I386_EFLAGS_REGNUM))
    return -1;

  return 0;
}

/* i386_canonicalize_syscall maps from the native i386 Linux set
   of syscall ids into a canonical set of syscall ids used by
   process record (a mostly trivial mapping, since the canonical
   set was originally taken from the i386 set).  */

static enum gdb_syscall
i386_canonicalize_syscall (int syscall)
{
  enum { i386_syscall_max = 499 };

  if (syscall <= i386_syscall_max)
    return (enum gdb_syscall) syscall;
  else
    return gdb_sys_no_syscall;
}

/* Value of the sigcode in case of a boundary fault.  */

#define SIG_CODE_BOUNDARY_FAULT 3

/* i386 GNU/Linux implementation of the report_signal_info
   gdbarch hook.  Displays information related to MPX bound
   violations.  */
void
i386_linux_report_signal_info (struct gdbarch *gdbarch, struct ui_out *uiout,
			       enum gdb_signal siggnal)
{
  /* -Wmaybe-uninitialized  */
  CORE_ADDR lower_bound = 0, upper_bound = 0, access = 0;
  int is_upper;
  long sig_code = 0;

  if (!i386_mpx_enabled () || siggnal != GDB_SIGNAL_SEGV)
    return;

  try
    {
      /* Sigcode evaluates if the actual segfault is a boundary violation.  */
      sig_code = parse_and_eval_long ("$_siginfo.si_code\n");

      lower_bound
	= parse_and_eval_long ("$_siginfo._sifields._sigfault._addr_bnd._lower");
      upper_bound
	= parse_and_eval_long ("$_siginfo._sifields._sigfault._addr_bnd._upper");
      access
	= parse_and_eval_long ("$_siginfo._sifields._sigfault.si_addr");
    }
  catch (const gdb_exception_error &exception)
    {
      return;
    }

  /* If this is not a boundary violation just return.  */
  if (sig_code != SIG_CODE_BOUNDARY_FAULT)
    return;

  is_upper = (access > upper_bound ? 1 : 0);

  uiout->text ("\n");
  if (is_upper)
    uiout->field_string ("sigcode-meaning", _("Upper bound violation"));
  else
    uiout->field_string ("sigcode-meaning", _("Lower bound violation"));

  uiout->text (_(" while accessing address "));
  uiout->field_core_addr ("bound-access", gdbarch, access);

  uiout->text (_("\nBounds: [lower = "));
  uiout->field_core_addr ("lower-bound", gdbarch, lower_bound);

  uiout->text (_(", upper = "));
  uiout->field_core_addr ("upper-bound", gdbarch, upper_bound);

  uiout->text (_("]"));
}

/* Parse the arguments of current system call instruction and record
   the values of the registers and memory that will be changed into
   "record_arch_list".  This instruction is "int 0x80" (Linux
   Kernel2.4) or "sysenter" (Linux Kernel 2.6).

   Return -1 if something wrong.  */

static struct linux_record_tdep i386_linux_record_tdep;

static int
i386_linux_intx80_sysenter_syscall_record (struct regcache *regcache)
{
  int ret;
  LONGEST syscall_native;
  enum gdb_syscall syscall_gdb;

  regcache_raw_read_signed (regcache, I386_EAX_REGNUM, &syscall_native);

  syscall_gdb = i386_canonicalize_syscall (syscall_native);

  if (syscall_gdb < 0)
    {
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %s\n"), 
		  plongest (syscall_native));
      return -1;
    }

  if (syscall_gdb == gdb_sys_sigreturn
      || syscall_gdb == gdb_sys_rt_sigreturn)
   {
     if (i386_all_but_ip_registers_record (regcache))
       return -1;
     return 0;
   }

  ret = record_linux_system_call (syscall_gdb, regcache,
				  &i386_linux_record_tdep);
  if (ret)
    return ret;

  /* Record the return value of the system call.  */
  if (record_full_arch_list_add_reg (regcache, I386_EAX_REGNUM))
    return -1;

  return 0;
}

#define I386_LINUX_xstate	270
#define I386_LINUX_frame_size	732

static int
i386_linux_record_signal (struct gdbarch *gdbarch,
			  struct regcache *regcache,
			  enum gdb_signal signal)
{
  ULONGEST esp;

  if (i386_all_but_ip_registers_record (regcache))
    return -1;

  if (record_full_arch_list_add_reg (regcache, I386_EIP_REGNUM))
    return -1;

  /* Record the change in the stack.  */
  regcache_raw_read_unsigned (regcache, I386_ESP_REGNUM, &esp);
  /* This is for xstate.
     sp -= sizeof (struct _fpstate);  */
  esp -= I386_LINUX_xstate;
  /* This is for frame_size.
     sp -= sizeof (struct rt_sigframe);  */
  esp -= I386_LINUX_frame_size;
  if (record_full_arch_list_add_mem (esp,
				     I386_LINUX_xstate + I386_LINUX_frame_size))
    return -1;

  if (record_full_arch_list_add_end ())
    return -1;

  return 0;
}


/* Core of the implementation for gdbarch get_syscall_number.  Get pending
   syscall number from REGCACHE.  If there is no pending syscall -1 will be
   returned.  Pending syscall means ptrace has stepped into the syscall but
   another ptrace call will step out.  PC is right after the int $0x80
   / syscall / sysenter instruction in both cases, PC does not change during
   the second ptrace step.  */

static LONGEST
i386_linux_get_syscall_number_from_regcache (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  /* The content of a register.  */
  gdb_byte buf[4];
  /* The result.  */
  LONGEST ret;

  /* Getting the system call number from the register.
     When dealing with x86 architecture, this information
     is stored at %eax register.  */
  regcache->cooked_read (I386_LINUX_ORIG_EAX_REGNUM, buf);

  ret = extract_signed_integer (buf, byte_order);

  return ret;
}

/* Wrapper for i386_linux_get_syscall_number_from_regcache to make it
   compatible with gdbarch get_syscall_number method prototype.  */

static LONGEST
i386_linux_get_syscall_number (struct gdbarch *gdbarch,
			       thread_info *thread)
{
  struct regcache *regcache = get_thread_regcache (thread);

  return i386_linux_get_syscall_number_from_regcache (regcache);
}

/* The register sets used in GNU/Linux ELF core-dumps are identical to
   the register sets in `struct user' that are used for a.out
   core-dumps.  These are also used by ptrace(2).  The corresponding
   types are `elf_gregset_t' for the general-purpose registers (with
   `elf_greg_t' the type of a single GP register) and `elf_fpregset_t'
   for the floating-point registers.

   Those types used to be available under the names `gregset_t' and
   `fpregset_t' too, and GDB used those names in the past.  But those
   names are now used for the register sets used in the `mcontext_t'
   type, which have a different size and layout.  */

/* Mapping between the general-purpose registers in `struct user'
   format and GDB's register cache layout.  */

/* From <sys/reg.h>.  */
int i386_linux_gregset_reg_offset[] =
{
  6 * 4,			/* %eax */
  1 * 4,			/* %ecx */
  2 * 4,			/* %edx */
  0 * 4,			/* %ebx */
  15 * 4,			/* %esp */
  5 * 4,			/* %ebp */
  3 * 4,			/* %esi */
  4 * 4,			/* %edi */
  12 * 4,			/* %eip */
  14 * 4,			/* %eflags */
  13 * 4,			/* %cs */
  16 * 4,			/* %ss */
  7 * 4,			/* %ds */
  8 * 4,			/* %es */
  9 * 4,			/* %fs */
  10 * 4,			/* %gs */
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1,
  -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1,		  /* MPX registers BND0 ... BND3.  */
  -1, -1,			  /* MPX registers BNDCFGU, BNDSTATUS.  */
  -1, -1, -1, -1, -1, -1, -1, -1, /* k0 ... k7 (AVX512)  */
  -1, -1, -1, -1, -1, -1, -1, -1, /* zmm0 ... zmm7 (AVX512)  */
  -1,				  /* PKRU register  */
  11 * 4,			  /* "orig_eax"  */
};

/* Mapping between the general-purpose registers in `struct
   sigcontext' format and GDB's register cache layout.  */

/* From <asm/sigcontext.h>.  */
static int i386_linux_sc_reg_offset[] =
{
  11 * 4,			/* %eax */
  10 * 4,			/* %ecx */
  9 * 4,			/* %edx */
  8 * 4,			/* %ebx */
  7 * 4,			/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  14 * 4,			/* %eip */
  16 * 4,			/* %eflags */
  15 * 4,			/* %cs */
  18 * 4,			/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

/* See i386-linux-tdep.h.  */

uint64_t
i386_linux_core_read_xsave_info (bfd *abfd, x86_xsave_layout &layout)
{
  asection *xstate = bfd_get_section_by_name (abfd, ".reg-xstate");
  if (xstate == nullptr)
    return 0;

  /* Check extended state size.  */
  size_t size = bfd_section_size (xstate);
  if (size < X86_XSTATE_AVX_SIZE)
    return 0;

  char contents[8];
  if (! bfd_get_section_contents (abfd, xstate, contents,
				  I386_LINUX_XSAVE_XCR0_OFFSET, 8))
    {
      warning (_("Couldn't read `xcr0' bytes from "
		 "`.reg-xstate' section in core file."));
      return 0;
    }

  uint64_t xcr0 = bfd_get_64 (abfd, contents);

  if (!i387_guess_xsave_layout (xcr0, size, layout))
    return 0;

  return xcr0;
}

/* See i386-linux-tdep.h.  */

bool
i386_linux_core_read_x86_xsave_layout (struct gdbarch *gdbarch,
				       x86_xsave_layout &layout)
{
  return i386_linux_core_read_xsave_info (core_bfd, layout) != 0;
}

/* See i386-linux-tdep.h.  */

const struct target_desc *
i386_linux_read_description (uint64_t xcr0)
{
  if (xcr0 == 0)
    return NULL;

  static struct target_desc *i386_linux_tdescs \
    [2/*X87*/][2/*SSE*/][2/*AVX*/][2/*MPX*/][2/*AVX512*/][2/*PKRU*/] = {};
  struct target_desc **tdesc;

  tdesc = &i386_linux_tdescs[(xcr0 & X86_XSTATE_X87) ? 1 : 0]
    [(xcr0 & X86_XSTATE_SSE) ? 1 : 0]
    [(xcr0 & X86_XSTATE_AVX) ? 1 : 0]
    [(xcr0 & X86_XSTATE_MPX) ? 1 : 0]
    [(xcr0 & X86_XSTATE_AVX512) ? 1 : 0]
    [(xcr0 & X86_XSTATE_PKRU) ? 1 : 0];

  if (*tdesc == NULL)
    *tdesc = i386_create_target_description (xcr0, true, false);

  return *tdesc;
}

/* Get Linux/x86 target description from core dump.  */

static const struct target_desc *
i386_linux_core_read_description (struct gdbarch *gdbarch,
				  struct target_ops *target,
				  bfd *abfd)
{
  /* Linux/i386.  */
  x86_xsave_layout layout;
  uint64_t xcr0 = i386_linux_core_read_xsave_info (abfd, layout);
  const struct target_desc *tdesc = i386_linux_read_description (xcr0);

  if (tdesc != NULL)
    return tdesc;

  if (bfd_get_section_by_name (abfd, ".reg-xfp") != NULL)
    return i386_linux_read_description (X86_XSTATE_SSE_MASK);
  else
    return i386_linux_read_description (X86_XSTATE_X87_MASK);
}

/* Similar to i386_supply_fpregset, but use XSAVE extended state.  */

static void
i386_linux_supply_xstateregset (const struct regset *regset,
				struct regcache *regcache, int regnum,
				const void *xstateregs, size_t len)
{
  i387_supply_xsave (regcache, regnum, xstateregs);
}

struct type *
x86_linux_get_siginfo_type (struct gdbarch *gdbarch)
{
  return linux_get_siginfo_type_with_fields (gdbarch, LINUX_SIGINFO_FIELD_ADDR_BND);
}

/* Similar to i386_collect_fpregset, but use XSAVE extended state.  */

static void
i386_linux_collect_xstateregset (const struct regset *regset,
				 const struct regcache *regcache,
				 int regnum, void *xstateregs, size_t len)
{
  i387_collect_xsave (regcache, regnum, xstateregs, 1);
}

/* Register set definitions.  */

static const struct regset i386_linux_xstateregset =
  {
    NULL,
    i386_linux_supply_xstateregset,
    i386_linux_collect_xstateregset
  };

/* Iterate over core file register note sections.  */

static void
i386_linux_iterate_over_regset_sections (struct gdbarch *gdbarch,
					 iterate_over_regset_sections_cb *cb,
					 void *cb_data,
					 const struct regcache *regcache)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  cb (".reg", 68, 68, &i386_gregset, NULL, cb_data);

  if (tdep->xsave_layout.sizeof_xsave != 0)
    cb (".reg-xstate", tdep->xsave_layout.sizeof_xsave,
	tdep->xsave_layout.sizeof_xsave, &i386_linux_xstateregset,
	"XSAVE extended state", cb_data);
  else if (tdep->xcr0 & X86_XSTATE_SSE)
    cb (".reg-xfp", 512, 512, &i386_fpregset, "extended floating-point",
	cb_data);
  else
    cb (".reg2", 108, 108, &i386_fpregset, NULL, cb_data);
}

/* Linux kernel shows PC value after the 'int $0x80' instruction even if
   inferior is still inside the syscall.  On next PTRACE_SINGLESTEP it will
   finish the syscall but PC will not change.
   
   Some vDSOs contain 'int $0x80; ret' and during stepping out of the syscall
   i386_displaced_step_fixup would keep PC at the displaced pad location.
   As PC is pointing to the 'ret' instruction before the step
   i386_displaced_step_fixup would expect inferior has just executed that 'ret'
   and PC should not be adjusted.  In reality it finished syscall instead and
   PC should get relocated back to its vDSO address.  Hide the 'ret'
   instruction by 'nop' so that i386_displaced_step_fixup is not confused.
   
   It is not fully correct as the bytes in struct
   displaced_step_copy_insn_closure will not match the inferior code.  But we
   would need some new flag in displaced_step_copy_insn_closure otherwise to
   keep the state that syscall is finishing for the later
   i386_displaced_step_fixup execution as the syscall execution is already no
   longer detectable there.  The new flag field would mean i386-linux-tdep.c
   needs to wrap all the displacement methods of i386-tdep.c which does not seem
   worth it.  The same effect is achieved by patching that 'nop' instruction
   there instead.  */

static displaced_step_copy_insn_closure_up
i386_linux_displaced_step_copy_insn (struct gdbarch *gdbarch,
				     CORE_ADDR from, CORE_ADDR to,
				     struct regcache *regs)
{
  displaced_step_copy_insn_closure_up closure_
    =  i386_displaced_step_copy_insn (gdbarch, from, to, regs);

  if (i386_linux_get_syscall_number_from_regcache (regs) != -1)
    {
      /* The closure returned by i386_displaced_step_copy_insn is simply a
	 buffer with a copy of the instruction. */
      i386_displaced_step_copy_insn_closure *closure
	= (i386_displaced_step_copy_insn_closure *) closure_.get ();

      /* Fake nop.  */
      closure->buf[0] = 0x90;
    }

  return closure_;
}

static void
i386_linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);
  const struct target_desc *tdesc = info.target_desc;
  struct tdesc_arch_data *tdesc_data = info.tdesc_data;
  const struct tdesc_feature *feature;
  int valid_p;

  gdb_assert (tdesc_data);

  linux_init_abi (info, gdbarch, 1);

  /* GNU/Linux uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  /* Reserve a number for orig_eax.  */
  set_gdbarch_num_regs (gdbarch, I386_LINUX_NUM_REGS);

  if (! tdesc_has_registers (tdesc))
    tdesc = i386_linux_read_description (X86_XSTATE_SSE_MASK);
  tdep->tdesc = tdesc;

  feature = tdesc_find_feature (tdesc, "org.gnu.gdb.i386.linux");
  if (feature == NULL)
    return;

  valid_p = tdesc_numbered_register (feature, tdesc_data,
				     I386_LINUX_ORIG_EAX_REGNUM,
				     "orig_eax");
  if (!valid_p)
    return;

  /* Add the %orig_eax register used for syscall restarting.  */
  set_gdbarch_write_pc (gdbarch, i386_linux_write_pc);

  tdep->register_reggroup_p = i386_linux_register_reggroup_p;

  tdep->gregset_reg_offset = i386_linux_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386_linux_gregset_reg_offset);
  tdep->sizeof_gregset = 17 * 4;

  tdep->jb_pc_offset = 20;	/* From <bits/setjmp.h>.  */

  tdep->sigtramp_p = i386_linux_sigtramp_p;
  tdep->sigcontext_addr = i386_linux_sigcontext_addr;
  tdep->sc_reg_offset = i386_linux_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386_linux_sc_reg_offset);

  tdep->xsave_xcr0_offset = I386_LINUX_XSAVE_XCR0_OFFSET;
  set_gdbarch_core_read_x86_xsave_layout
    (gdbarch, i386_linux_core_read_x86_xsave_layout);

  set_gdbarch_process_record (gdbarch, i386_process_record);
  set_gdbarch_process_record_signal (gdbarch, i386_linux_record_signal);

  /* Initialize the i386_linux_record_tdep.  */
  /* These values are the size of the type that will be used in a system
     call.  They are obtained from Linux Kernel source.  */
  i386_linux_record_tdep.size_pointer
    = gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size__old_kernel_stat = 32;
  i386_linux_record_tdep.size_tms = 16;
  i386_linux_record_tdep.size_loff_t = 8;
  i386_linux_record_tdep.size_flock = 16;
  i386_linux_record_tdep.size_oldold_utsname = 45;
  i386_linux_record_tdep.size_ustat = 20;
  i386_linux_record_tdep.size_old_sigaction = 16;
  i386_linux_record_tdep.size_old_sigset_t = 4;
  i386_linux_record_tdep.size_rlimit = 8;
  i386_linux_record_tdep.size_rusage = 72;
  i386_linux_record_tdep.size_timeval = 8;
  i386_linux_record_tdep.size_timezone = 8;
  i386_linux_record_tdep.size_old_gid_t = 2;
  i386_linux_record_tdep.size_old_uid_t = 2;
  i386_linux_record_tdep.size_fd_set = 128;
  i386_linux_record_tdep.size_old_dirent = 268;
  i386_linux_record_tdep.size_statfs = 64;
  i386_linux_record_tdep.size_statfs64 = 84;
  i386_linux_record_tdep.size_sockaddr = 16;
  i386_linux_record_tdep.size_int
    = gdbarch_int_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size_long
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size_ulong
    = gdbarch_long_bit (gdbarch) / TARGET_CHAR_BIT;
  i386_linux_record_tdep.size_msghdr = 28;
  i386_linux_record_tdep.size_itimerval = 16;
  i386_linux_record_tdep.size_stat = 88;
  i386_linux_record_tdep.size_old_utsname = 325;
  i386_linux_record_tdep.size_sysinfo = 64;
  i386_linux_record_tdep.size_msqid_ds = 88;
  i386_linux_record_tdep.size_shmid_ds = 84;
  i386_linux_record_tdep.size_new_utsname = 390;
  i386_linux_record_tdep.size_timex = 128;
  i386_linux_record_tdep.size_mem_dqinfo = 24;
  i386_linux_record_tdep.size_if_dqblk = 68;
  i386_linux_record_tdep.size_fs_quota_stat = 68;
  i386_linux_record_tdep.size_timespec = 8;
  i386_linux_record_tdep.size_pollfd = 8;
  i386_linux_record_tdep.size_NFS_FHSIZE = 32;
  i386_linux_record_tdep.size_knfsd_fh = 132;
  i386_linux_record_tdep.size_TASK_COMM_LEN = 16;
  i386_linux_record_tdep.size_sigaction = 20;
  i386_linux_record_tdep.size_sigset_t = 8;
  i386_linux_record_tdep.size_siginfo_t = 128;
  i386_linux_record_tdep.size_cap_user_data_t = 12;
  i386_linux_record_tdep.size_stack_t = 12;
  i386_linux_record_tdep.size_off_t = i386_linux_record_tdep.size_long;
  i386_linux_record_tdep.size_stat64 = 96;
  i386_linux_record_tdep.size_gid_t = 4;
  i386_linux_record_tdep.size_uid_t = 4;
  i386_linux_record_tdep.size_PAGE_SIZE = 4096;
  i386_linux_record_tdep.size_flock64 = 24;
  i386_linux_record_tdep.size_user_desc = 16;
  i386_linux_record_tdep.size_io_event = 32;
  i386_linux_record_tdep.size_iocb = 64;
  i386_linux_record_tdep.size_epoll_event = 12;
  i386_linux_record_tdep.size_itimerspec
    = i386_linux_record_tdep.size_timespec * 2;
  i386_linux_record_tdep.size_mq_attr = 32;
  i386_linux_record_tdep.size_termios = 36;
  i386_linux_record_tdep.size_termios2 = 44;
  i386_linux_record_tdep.size_pid_t = 4;
  i386_linux_record_tdep.size_winsize = 8;
  i386_linux_record_tdep.size_serial_struct = 60;
  i386_linux_record_tdep.size_serial_icounter_struct = 80;
  i386_linux_record_tdep.size_hayes_esp_config = 12;
  i386_linux_record_tdep.size_size_t = 4;
  i386_linux_record_tdep.size_iovec = 8;
  i386_linux_record_tdep.size_time_t = 4;

  /* These values are the second argument of system call "sys_ioctl".
     They are obtained from Linux Kernel source.  */
  i386_linux_record_tdep.ioctl_TCGETS = 0x5401;
  i386_linux_record_tdep.ioctl_TCSETS = 0x5402;
  i386_linux_record_tdep.ioctl_TCSETSW = 0x5403;
  i386_linux_record_tdep.ioctl_TCSETSF = 0x5404;
  i386_linux_record_tdep.ioctl_TCGETA = 0x5405;
  i386_linux_record_tdep.ioctl_TCSETA = 0x5406;
  i386_linux_record_tdep.ioctl_TCSETAW = 0x5407;
  i386_linux_record_tdep.ioctl_TCSETAF = 0x5408;
  i386_linux_record_tdep.ioctl_TCSBRK = 0x5409;
  i386_linux_record_tdep.ioctl_TCXONC = 0x540A;
  i386_linux_record_tdep.ioctl_TCFLSH = 0x540B;
  i386_linux_record_tdep.ioctl_TIOCEXCL = 0x540C;
  i386_linux_record_tdep.ioctl_TIOCNXCL = 0x540D;
  i386_linux_record_tdep.ioctl_TIOCSCTTY = 0x540E;
  i386_linux_record_tdep.ioctl_TIOCGPGRP = 0x540F;
  i386_linux_record_tdep.ioctl_TIOCSPGRP = 0x5410;
  i386_linux_record_tdep.ioctl_TIOCOUTQ = 0x5411;
  i386_linux_record_tdep.ioctl_TIOCSTI = 0x5412;
  i386_linux_record_tdep.ioctl_TIOCGWINSZ = 0x5413;
  i386_linux_record_tdep.ioctl_TIOCSWINSZ = 0x5414;
  i386_linux_record_tdep.ioctl_TIOCMGET = 0x5415;
  i386_linux_record_tdep.ioctl_TIOCMBIS = 0x5416;
  i386_linux_record_tdep.ioctl_TIOCMBIC = 0x5417;
  i386_linux_record_tdep.ioctl_TIOCMSET = 0x5418;
  i386_linux_record_tdep.ioctl_TIOCGSOFTCAR = 0x5419;
  i386_linux_record_tdep.ioctl_TIOCSSOFTCAR = 0x541A;
  i386_linux_record_tdep.ioctl_FIONREAD = 0x541B;
  i386_linux_record_tdep.ioctl_TIOCINQ = i386_linux_record_tdep.ioctl_FIONREAD;
  i386_linux_record_tdep.ioctl_TIOCLINUX = 0x541C;
  i386_linux_record_tdep.ioctl_TIOCCONS = 0x541D;
  i386_linux_record_tdep.ioctl_TIOCGSERIAL = 0x541E;
  i386_linux_record_tdep.ioctl_TIOCSSERIAL = 0x541F;
  i386_linux_record_tdep.ioctl_TIOCPKT = 0x5420;
  i386_linux_record_tdep.ioctl_FIONBIO = 0x5421;
  i386_linux_record_tdep.ioctl_TIOCNOTTY = 0x5422;
  i386_linux_record_tdep.ioctl_TIOCSETD = 0x5423;
  i386_linux_record_tdep.ioctl_TIOCGETD = 0x5424;
  i386_linux_record_tdep.ioctl_TCSBRKP = 0x5425;
  i386_linux_record_tdep.ioctl_TIOCTTYGSTRUCT = 0x5426;
  i386_linux_record_tdep.ioctl_TIOCSBRK = 0x5427;
  i386_linux_record_tdep.ioctl_TIOCCBRK = 0x5428;
  i386_linux_record_tdep.ioctl_TIOCGSID = 0x5429;
  i386_linux_record_tdep.ioctl_TCGETS2 = 0x802c542a;
  i386_linux_record_tdep.ioctl_TCSETS2 = 0x402c542b;
  i386_linux_record_tdep.ioctl_TCSETSW2 = 0x402c542c;
  i386_linux_record_tdep.ioctl_TCSETSF2 = 0x402c542d;
  i386_linux_record_tdep.ioctl_TIOCGPTN = 0x80045430;
  i386_linux_record_tdep.ioctl_TIOCSPTLCK = 0x40045431;
  i386_linux_record_tdep.ioctl_FIONCLEX = 0x5450;
  i386_linux_record_tdep.ioctl_FIOCLEX = 0x5451;
  i386_linux_record_tdep.ioctl_FIOASYNC = 0x5452;
  i386_linux_record_tdep.ioctl_TIOCSERCONFIG = 0x5453;
  i386_linux_record_tdep.ioctl_TIOCSERGWILD = 0x5454;
  i386_linux_record_tdep.ioctl_TIOCSERSWILD = 0x5455;
  i386_linux_record_tdep.ioctl_TIOCGLCKTRMIOS = 0x5456;
  i386_linux_record_tdep.ioctl_TIOCSLCKTRMIOS = 0x5457;
  i386_linux_record_tdep.ioctl_TIOCSERGSTRUCT = 0x5458;
  i386_linux_record_tdep.ioctl_TIOCSERGETLSR = 0x5459;
  i386_linux_record_tdep.ioctl_TIOCSERGETMULTI = 0x545A;
  i386_linux_record_tdep.ioctl_TIOCSERSETMULTI = 0x545B;
  i386_linux_record_tdep.ioctl_TIOCMIWAIT = 0x545C;
  i386_linux_record_tdep.ioctl_TIOCGICOUNT = 0x545D;
  i386_linux_record_tdep.ioctl_TIOCGHAYESESP = 0x545E;
  i386_linux_record_tdep.ioctl_TIOCSHAYESESP = 0x545F;
  i386_linux_record_tdep.ioctl_FIOQSIZE = 0x5460;

  /* These values are the second argument of system call "sys_fcntl"
     and "sys_fcntl64".  They are obtained from Linux Kernel source.  */
  i386_linux_record_tdep.fcntl_F_GETLK = 5;
  i386_linux_record_tdep.fcntl_F_GETLK64 = 12;
  i386_linux_record_tdep.fcntl_F_SETLK64 = 13;
  i386_linux_record_tdep.fcntl_F_SETLKW64 = 14;

  i386_linux_record_tdep.arg1 = I386_EBX_REGNUM;
  i386_linux_record_tdep.arg2 = I386_ECX_REGNUM;
  i386_linux_record_tdep.arg3 = I386_EDX_REGNUM;
  i386_linux_record_tdep.arg4 = I386_ESI_REGNUM;
  i386_linux_record_tdep.arg5 = I386_EDI_REGNUM;
  i386_linux_record_tdep.arg6 = I386_EBP_REGNUM;

  tdep->i386_intx80_record = i386_linux_intx80_sysenter_syscall_record;
  tdep->i386_sysenter_record = i386_linux_intx80_sysenter_syscall_record;
  tdep->i386_syscall_record = i386_linux_intx80_sysenter_syscall_record;

  /* N_FUN symbols in shared libraries have 0 for their values and need
     to be relocated.  */
  set_gdbarch_sofun_address_maybe_missing (gdbarch, 1);

  /* GNU/Linux uses SVR4-style shared libraries.  */
  set_gdbarch_skip_trampoline_code (gdbarch, find_solib_trampoline_target);
  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, linux_ilp32_fetch_link_map_offsets);

  /* GNU/Linux uses the dynamic linker included in the GNU C Library.  */
  set_gdbarch_skip_solib_resolver (gdbarch, glibc_skip_solib_resolver);

  dwarf2_frame_set_signal_frame_p (gdbarch, i386_linux_dwarf_signal_frame_p);

  /* Enable TLS support.  */
  set_gdbarch_fetch_tls_load_module_address (gdbarch,
					     svr4_fetch_objfile_link_map);

  /* Core file support.  */
  set_gdbarch_iterate_over_regset_sections
    (gdbarch, i386_linux_iterate_over_regset_sections);
  set_gdbarch_core_read_description (gdbarch,
				     i386_linux_core_read_description);

  /* Displaced stepping.  */
  set_gdbarch_displaced_step_copy_insn (gdbarch,
					i386_linux_displaced_step_copy_insn);
  set_gdbarch_displaced_step_fixup (gdbarch, i386_displaced_step_fixup);

  /* Functions for 'catch syscall'.  */
  set_xml_syscall_file_name (gdbarch, XML_SYSCALL_FILENAME_I386);
  set_gdbarch_get_syscall_number (gdbarch,
				  i386_linux_get_syscall_number);

  set_gdbarch_get_siginfo_type (gdbarch, x86_linux_get_siginfo_type);
  set_gdbarch_report_signal_info (gdbarch, i386_linux_report_signal_info);
}

void _initialize_i386_linux_tdep ();
void
_initialize_i386_linux_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_LINUX,
			  i386_linux_init_abi);
}
