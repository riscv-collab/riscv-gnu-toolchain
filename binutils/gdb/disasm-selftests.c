/* Self tests for disassembler for GDB, the GNU debugger.

   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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
#include "disasm.h"
#include "gdbsupport/selftest.h"
#include "selftest-arch.h"
#include "gdbarch.h"

namespace selftests {

/* Return a pointer to a buffer containing an instruction that can be
   disassembled for architecture GDBARCH.  *LEN will be set to the length
   of the returned buffer.

   If there's no known instruction to disassemble for GDBARCH (because we
   haven't figured on out, not because no instructions exist) then nullptr
   is returned, and *LEN is set to 0.  */

static const gdb_byte *
get_test_insn (struct gdbarch *gdbarch, size_t *len)
{
  *len = 0;
  const gdb_byte *insn = nullptr;

  switch (gdbarch_bfd_arch_info (gdbarch)->arch)
    {
    case bfd_arch_bfin:
      /* M3.L = 0xe117 */
      static const gdb_byte bfin_insn[] = {0x17, 0xe1, 0xff, 0xff};

      insn = bfin_insn;
      *len = sizeof (bfin_insn);
      break;
    case bfd_arch_arm:
      /* mov     r0, #0 */
      static const gdb_byte arm_insn[] = {0x0, 0x0, 0xa0, 0xe3};

      insn = arm_insn;
      *len = sizeof (arm_insn);
      break;
    case bfd_arch_ia64:
      /* We get:
	 internal-error: gdbarch_sw_breakpoint_from_kind:
	 Assertion `gdbarch->sw_breakpoint_from_kind != NULL' failed.  */
      return insn;
    case bfd_arch_mep:
      /* Disassembles as '*unknown*' insn, then len self-check fails.  */
      return insn;
    case bfd_arch_mips:
      if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_mips16)
	/* Disassembles insn, but len self-check fails.  */
	return insn;
      goto generic_case;
    case bfd_arch_tic6x:
      /* Disassembles as '<undefined instruction 0x56454314>' insn, but len
	 self-check passes, so let's allow it.  */
      goto generic_case;
    case bfd_arch_xtensa:
      /* Disassembles insn, but len self-check fails.  */
      return insn;
    case bfd_arch_or1k:
      /* Disassembles as '*unknown*' insn, but len self-check passes, so let's
	 allow it.  */
      goto generic_case;
    case bfd_arch_s390:
      /* nopr %r7 */
      static const gdb_byte s390_insn[] = {0x07, 0x07};

      insn = s390_insn;
      *len = sizeof (s390_insn);
      break;
    case bfd_arch_xstormy16:
      /* nop */
      static const gdb_byte xstormy16_insn[] = {0x0, 0x0};

      insn = xstormy16_insn;
      *len = sizeof (xstormy16_insn);
      break;
    case bfd_arch_nios2:
    case bfd_arch_score:
    case bfd_arch_riscv:
      /* nios2, riscv, and score need to know the current instruction
	 to select breakpoint instruction.  Give the breakpoint
	 instruction kind explicitly.  */
      {
	int bplen;
	insn = gdbarch_sw_breakpoint_from_kind (gdbarch, 4, &bplen);
	*len = bplen;
      }
      break;
    case bfd_arch_arc:
      /* PR 21003 */
      if (gdbarch_bfd_arch_info (gdbarch)->mach == bfd_mach_arc_arc601)
	return insn;
      goto generic_case;
    case bfd_arch_z80:
      {
	int bplen;
	insn = gdbarch_sw_breakpoint_from_kind (gdbarch, 0x0008, &bplen);
	*len = bplen;
      }
      break;
    case bfd_arch_i386:
      {
	const struct bfd_arch_info *info = gdbarch_bfd_arch_info (gdbarch);
	/* The disassembly tests will fail on x86-linux because
	   opcodes rejects an attempt to disassemble for an arch with
	   a 64-bit address size when bfd_vma is 32-bit.  */
	if (info->bits_per_address > sizeof (bfd_vma) * CHAR_BIT)
	  return insn;
      }
      [[fallthrough]];
    default:
    generic_case:
      {
	/* Test disassemble breakpoint instruction.  */
	CORE_ADDR pc = 0;
	int kind;
	int bplen;

	struct gdbarch_info info;
	info.bfd_arch_info = gdbarch_bfd_arch_info (gdbarch);

	enum gdb_osabi it;
	bool found = false;
	for (it = GDB_OSABI_UNKNOWN; it != GDB_OSABI_INVALID;
	     it = static_cast<enum gdb_osabi>(static_cast<int>(it) + 1))
	  {
	    if (it == GDB_OSABI_UNKNOWN)
	      continue;

	    info.osabi = it;

	    if (it != GDB_OSABI_NONE)
	      {
		if (!has_gdb_osabi_handler (info))
		  /* Unsupported.  Skip to prevent warnings like:
		     A handler for the OS ABI <x> is not built into this
		     configuration of GDB.  Attempting to continue with the
		     default <y> settings.  */
		  continue;
	      }

	    gdbarch = gdbarch_find_by_info (info);
	    SELF_CHECK (gdbarch != NULL);

	    try
	      {
		kind = gdbarch_breakpoint_kind_from_pc (gdbarch, &pc);
		insn = gdbarch_sw_breakpoint_from_kind (gdbarch, kind, &bplen);
	      }
	    catch (...)
	      {
		continue;
	      }
	    found = true;
	    break;
	  }

	/* Assert that we have found an instruction to disassemble.  */
	SELF_CHECK (found);

	*len = bplen;
	break;
      }
    }
  SELF_CHECK (*len > 0);

  return insn;
}

/* Test disassembly of one instruction.  */

static void
print_one_insn_test (struct gdbarch *gdbarch)
{
  size_t len;
  const gdb_byte *insn = get_test_insn (gdbarch, &len);

  if (insn == nullptr)
    return;

  /* Test gdb_disassembler for a given gdbarch by reading data from a
     pre-allocated buffer.  If you want to see the disassembled
     instruction printed to gdb_stdout, use maint selftest -verbose.  */

  class gdb_disassembler_test : public gdb_disassembler
  {
  public:

    explicit gdb_disassembler_test (struct gdbarch *gdbarch,
				    const gdb_byte *insn,
				    size_t len)
      : gdb_disassembler (gdbarch,
			  (run_verbose () ? gdb_stdlog : &null_stream),
			  gdb_disassembler_test::read_memory),
	m_insn (insn), m_len (len)
    {
    }

    int
    print_insn (CORE_ADDR memaddr)
    {
      int len = gdb_disassembler::print_insn (memaddr);

      if (run_verbose ())
	debug_printf ("\n");

      return len;
    }

  private:
    /* A buffer contain one instruction.  */
    const gdb_byte *m_insn;

    /* Length of the buffer.  */
    size_t m_len;

    static int read_memory (bfd_vma memaddr, gdb_byte *myaddr,
			    unsigned int len,
			    struct disassemble_info *info) noexcept
    {
      gdb_disassembler_test *self
	= static_cast<gdb_disassembler_test *>(info->application_data);

      /* The disassembler in opcodes may read more data than one
	 instruction.  Supply infinite consecutive copies
	 of the same instruction.  */
      for (size_t i = 0; i < len; i++)
	myaddr[i] = self->m_insn[(memaddr + i) % self->m_len];

      return 0;
    }
  };

  gdb_disassembler_test di (gdbarch, insn, len);

  SELF_CHECK (di.print_insn (0) == len);
}

/* Test the gdb_buffered_insn_length function.  */

static void
buffered_insn_length_test (struct gdbarch *gdbarch)
{
  size_t buf_len;
  const gdb_byte *insn = get_test_insn (gdbarch, &buf_len);

  if (insn == nullptr)
    return;

  /* The tic6x architecture is VLIW.  Disassembling requires that the
     entire instruction bundle be available.  However, the buffer we got
     back from get_test_insn only contains a single instruction, which is
     just part of an instruction bundle.  As a result, the disassemble will
     fail.  To avoid this, skip tic6x tests now.  */
  if (gdbarch_bfd_arch_info (gdbarch)->arch == bfd_arch_tic6x)
    return;

  CORE_ADDR insn_address = 0;
  int calculated_len = gdb_buffered_insn_length (gdbarch, insn, buf_len,
						 insn_address);

  SELF_CHECK (calculated_len == buf_len);
}

/* Test disassembly on memory error.  */

static void
memory_error_test (struct gdbarch *gdbarch)
{
  class gdb_disassembler_test : public gdb_disassembler
  {
  public:
    gdb_disassembler_test (struct gdbarch *gdbarch)
      : gdb_disassembler (gdbarch, &null_stream,
			  gdb_disassembler_test::read_memory)
    {
    }

    static int read_memory (bfd_vma memaddr, gdb_byte *myaddr,
			    unsigned int len,
			    struct disassemble_info *info) noexcept
    {
      /* Always return an error.  */
      return -1;
    }
  };

  if (gdbarch_bfd_arch_info (gdbarch)->arch == bfd_arch_i386)
    {
      const struct bfd_arch_info *info = gdbarch_bfd_arch_info (gdbarch);
      /* This test will fail on x86-linux because opcodes rejects an
	 attempt to disassemble for an arch with a 64-bit address size
	 when bfd_vma is 32-bit.  */
      if (info->bits_per_address > sizeof (bfd_vma) * CHAR_BIT)
	return;
    }

  gdb_disassembler_test di (gdbarch);
  bool saw_memory_error = false;

  try
    {
      di.print_insn (0);
    }
  catch (const gdb_exception_error &ex)
    {
      if (ex.error == MEMORY_ERROR)
	saw_memory_error = true;
    }

  /* Expect MEMORY_ERROR.  */
  SELF_CHECK (saw_memory_error);
}

} // namespace selftests

void _initialize_disasm_selftests ();
void
_initialize_disasm_selftests ()
{
  selftests::register_test_foreach_arch ("print_one_insn",
					 selftests::print_one_insn_test);
  selftests::register_test_foreach_arch ("memory_error",
					 selftests::memory_error_test);
  selftests::register_test_foreach_arch ("buffered_insn_length",
					 selftests::buffered_insn_length_test);
}
