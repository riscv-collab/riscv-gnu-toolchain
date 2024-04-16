/* Target-dependent code for the GNU Hurd.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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
#include "osabi.h"
#include "solib-svr4.h"

#include "i386-tdep.h"

/* Recognizing signal handler frames.  */

/* When the GNU/Hurd libc calls a signal handler, the return address points
   inside the trampoline assembly snippet.

   If the trampoline function name can not be identified, we resort to reading
   memory from the process in order to identify it.  */

static const gdb_byte gnu_sigtramp_code[] =
{
/* rpc_wait_trampoline: */
  0xb8, 0xe7, 0xff, 0xff, 0xff,			/* mov    $-25,%eax */
  0x9a, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,	/* lcall  $7,$0 */
  0x89, 0x01,					/* movl   %eax, (%ecx) */
  0x89, 0xdc,					/* movl   %ebx, %esp */

/* trampoline: */
  0xff, 0xd2,					/* call   *%edx */
/* RA HERE */
  0x83, 0xc4, 0x0c,				/* addl   $12, %esp */
  0xc3,						/* ret */

/* firewall: */
  0xf4,						/* hlt */
};

#define GNU_SIGTRAMP_LEN (sizeof gnu_sigtramp_code)
#define GNU_SIGTRAMP_TAIL 5			/* length of tail after RA */

/* If THIS_FRAME is a sigtramp routine, return the address of the
   start of the routine.  Otherwise, return 0.  */

static CORE_ADDR
i386_gnu_sigtramp_start (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  gdb_byte buf[GNU_SIGTRAMP_LEN];

  if (!safe_frame_unwind_memory (this_frame,
				 pc + GNU_SIGTRAMP_TAIL - GNU_SIGTRAMP_LEN,
				 buf))
    return 0;

  if (memcmp (buf, gnu_sigtramp_code, GNU_SIGTRAMP_LEN) != 0)
    return 0;

  return pc;
}

/* Return whether THIS_FRAME corresponds to a GNU/Linux sigtramp
   routine.  */

static int
i386_gnu_sigtramp_p (frame_info_ptr this_frame)
{
  CORE_ADDR pc = get_frame_pc (this_frame);
  const char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);

  /* If we have a NAME, we can check for the trampoline function */
  if (name != NULL && strcmp (name, "trampoline") == 0)
    return 1;

  return i386_gnu_sigtramp_start (this_frame) != 0;
}

/* Offset to sc_i386_thread_state in sigcontext, from <bits/sigcontext.h>.  */
#define I386_GNU_SIGCONTEXT_THREAD_STATE_OFFSET 20

/* Assuming THIS_FRAME is a GNU/Linux sigtramp routine, return the
   address of the associated sigcontext structure.  */

static CORE_ADDR
i386_gnu_sigcontext_addr (frame_info_ptr this_frame)
{
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR pc;
  CORE_ADDR sp;
  gdb_byte buf[4];

  get_frame_register (this_frame, I386_ESP_REGNUM, buf);
  sp = extract_unsigned_integer (buf, 4, byte_order);

  pc = i386_gnu_sigtramp_start (this_frame);
  if (pc)
    {
      CORE_ADDR sigcontext_addr;

      /* The sigcontext structure address is passed as the third argument to
	 the signal handler. */
      read_memory (sp + 8, buf, 4);
      sigcontext_addr = extract_unsigned_integer (buf, 4, byte_order);
      return sigcontext_addr + I386_GNU_SIGCONTEXT_THREAD_STATE_OFFSET;
    }

  error (_("Couldn't recognize signal trampoline."));
  return 0;
}

/* Mapping between the general-purpose registers in `struct
   sigcontext' format (starting at sc_i386_thread_state)
   and GDB's register cache layout.  */

/* From <bits/sigcontext.h>.  */
static int i386_gnu_sc_reg_offset[] =
{
  11 * 4,			/* %eax */
  10 * 4,			/* %ecx */
  9 * 4,			/* %edx */
  8 * 4,			/* %ebx */
  7 * 4,			/* %esp */
  6 * 4,			/* %ebp */
  5 * 4,			/* %esi */
  4 * 4,			/* %edi */
  12 * 4,			/* %eip */
  14 * 4,			/* %eflags */
  13 * 4,			/* %cs */
  16 * 4,			/* %ss */
  3 * 4,			/* %ds */
  2 * 4,			/* %es */
  1 * 4,			/* %fs */
  0 * 4				/* %gs */
};

/* From <sys/ucontext.h>.  */
static int i386gnu_gregset_reg_offset[] =
{
  11 * 4,		/* %eax */
  10 * 4,		/* %ecx */
  9 * 4,		/* %edx */
  8 * 4,		/* %ebx */
  17 * 4,		/* %uesp */
  6 * 4,		/* %ebp */
  5 * 4,		/* %esi */
  4 * 4,		/* %edi */
  14 * 4,		/* %eip */
  16 * 4,		/* %efl */
  15 * 4,		/* %cs */
  18 * 4,		/* %ss */
  3 * 4,		/* %ds */
  2 * 4,		/* %es */
  1 * 4,		/* %fs */
  0 * 4,		/* %gs */
};

static void
i386gnu_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch)
{
  i386_gdbarch_tdep *tdep = gdbarch_tdep<i386_gdbarch_tdep> (gdbarch);

  /* GNU uses ELF.  */
  i386_elf_init_abi (info, gdbarch);

  set_solib_svr4_fetch_link_map_offsets
    (gdbarch, svr4_ilp32_fetch_link_map_offsets);

  tdep->gregset_reg_offset = i386gnu_gregset_reg_offset;
  tdep->gregset_num_regs = ARRAY_SIZE (i386gnu_gregset_reg_offset);
  tdep->sizeof_gregset = 19 * 4;

  tdep->jb_pc_offset = 20;	/* From <bits/setjmp.h>.  */

  tdep->sigtramp_p = i386_gnu_sigtramp_p;
  tdep->sigcontext_addr = i386_gnu_sigcontext_addr;
  tdep->sc_reg_offset = i386_gnu_sc_reg_offset;
  tdep->sc_num_regs = ARRAY_SIZE (i386_gnu_sc_reg_offset);
}

void _initialize_i386gnu_tdep ();
void
_initialize_i386gnu_tdep ()
{
  gdbarch_register_osabi (bfd_arch_i386, 0, GDB_OSABI_HURD, i386gnu_init_abi);
}
