/* Scheme interface to architecture.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "arch-utils.h"
#include "disasm.h"
#include "dis-asm.h"
#include "gdbarch.h"
#include "gdbcore.h"
#include "guile-internal.h"

static SCM port_keyword;
static SCM offset_keyword;
static SCM size_keyword;
static SCM count_keyword;

static SCM address_symbol;
static SCM asm_symbol;
static SCM length_symbol;

class gdbscm_disassembler : public gdb_disassembler
{
public:
  gdbscm_disassembler (struct gdbarch *gdbarch,
		       struct ui_file *stream,
		       SCM port, ULONGEST offset);

  SCM port;
  /* The offset of the address of the first instruction in PORT.  */
  ULONGEST offset;
};

/* Struct used to pass data from gdbscm_disasm_read_memory to
   gdbscm_disasm_read_memory_worker.  */

struct gdbscm_disasm_read_data
{
  bfd_vma memaddr;
  bfd_byte *myaddr;
  unsigned int length;
  gdbscm_disassembler *dinfo;
};

/* Subroutine of gdbscm_arch_disassemble to simplify it.
   Return the result for one instruction.  */

static SCM
dascm_make_insn (CORE_ADDR pc, const char *assembly, int insn_len)
{
  return scm_list_3 (scm_cons (address_symbol,
			       gdbscm_scm_from_ulongest (pc)),
		     scm_cons (asm_symbol,
			       gdbscm_scm_from_c_string (assembly)),
		     scm_cons (length_symbol,
			       scm_from_int (insn_len)));
}

/* Helper function for gdbscm_disasm_read_memory to safely read from a
   Scheme port.  Called via gdbscm_call_guile.
   The result is a statically allocated error message or NULL if success.  */

static const char *
gdbscm_disasm_read_memory_worker (void *datap)
{
  struct gdbscm_disasm_read_data *data
    = (struct gdbscm_disasm_read_data *) datap;
  gdbscm_disassembler *dinfo = data->dinfo;
  SCM seekto, newpos, port = dinfo->port;
  size_t bytes_read;

  seekto = gdbscm_scm_from_ulongest (data->memaddr - dinfo->offset);
  newpos = scm_seek (port, seekto, scm_from_int (SEEK_SET));
  if (!scm_is_eq (seekto, newpos))
    return "seek error";

  bytes_read = scm_c_read (port, data->myaddr, data->length);

  if (bytes_read != data->length)
    return "short read";

  /* If we get here the read succeeded.  */
  return NULL;
}

/* disassemble_info.read_memory_func for gdbscm_print_insn_from_port.  */

static int
gdbscm_disasm_read_memory (bfd_vma memaddr, bfd_byte *myaddr,
			   unsigned int length,
			   struct disassemble_info *dinfo) noexcept
{
  gdbscm_disassembler *self
    = static_cast<gdbscm_disassembler *> (dinfo->application_data);
  struct gdbscm_disasm_read_data data;
  const char *status;

  data.memaddr = memaddr;
  data.myaddr = myaddr;
  data.length = length;
  data.dinfo = self;

  status = gdbscm_with_guile (gdbscm_disasm_read_memory_worker, &data);

  /* TODO: IWBN to distinguish problems reading target memory versus problems
     with the port (e.g., EOF).  */
  return status != NULL ? -1 : 0;
}

gdbscm_disassembler::gdbscm_disassembler (struct gdbarch *gdbarch,
					  struct ui_file *stream,
					  SCM port_, ULONGEST offset_)
  : gdb_disassembler (gdbarch, stream, gdbscm_disasm_read_memory),
    port (port_), offset (offset_)
{
}

/* Subroutine of gdbscm_arch_disassemble to simplify it.
   Call gdbarch_print_insn using a port for input.
   PORT must be seekable.
   OFFSET is the offset in PORT from which addresses begin.
   For example, when printing from a bytevector, addresses passed to the
   bv seek routines must be in the range [0,size).  However, the bytevector
   may represent an instruction at address 0x1234.  To handle this case pass
   0x1234 for OFFSET.
   This is based on gdb_print_insn, see it for details.  */

static int
gdbscm_print_insn_from_port (struct gdbarch *gdbarch,
			     SCM port, ULONGEST offset, CORE_ADDR memaddr,
			     string_file *stream, int *branch_delay_insns)
{
  gdbscm_disassembler di (gdbarch, stream, port, offset);

  return di.print_insn (memaddr, branch_delay_insns);
}

/* (arch-disassemble <gdb:arch> address
     [#:port port] [#:offset address] [#:size integer] [#:count integer])
     -> list

   Returns a list of disassembled instructions.
   If PORT is provided, read bytes from it.  Otherwise read target memory.
   If PORT is #f, read target memory.
   PORT must be seekable.  IWBN to remove this restriction, and a future
   release may.  For now the restriction is in place because it's not clear
   all disassemblers are strictly sequential.
   If SIZE is provided, limit the number of bytes read to this amount.
   If COUNT is provided, limit the number of instructions to this amount.

   Each instruction in the result is an alist:
   (('address . address) ('asm . disassembly) ('length . length)).
   We could use a hash table (dictionary) but there aren't that many fields. */

static SCM
gdbscm_arch_disassemble (SCM self, SCM start_scm, SCM rest)
{
  arch_smob *a_smob
    = arscm_get_arch_smob_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  struct gdbarch *gdbarch = arscm_get_gdbarch (a_smob);
  const SCM keywords[] = {
    port_keyword, offset_keyword, size_keyword, count_keyword, SCM_BOOL_F
  };
  int port_arg_pos = -1, offset_arg_pos = -1;
  int size_arg_pos = -1, count_arg_pos = -1;
  SCM port = SCM_BOOL_F;
  ULONGEST offset = 0;
  unsigned int count = 1;
  unsigned int size;
  ULONGEST start_arg;
  CORE_ADDR start, end;
  CORE_ADDR pc;
  unsigned int i;
  int using_port;
  SCM result;

  gdbscm_parse_function_args (FUNC_NAME, SCM_ARG2, keywords, "U#OUuu",
			      start_scm, &start_arg, rest,
			      &port_arg_pos, &port,
			      &offset_arg_pos, &offset,
			      &size_arg_pos, &size,
			      &count_arg_pos, &count);
  /* START is first stored in a ULONGEST because we don't have a format char
     for CORE_ADDR, and it's not really worth it to have one yet.  */
  start = start_arg;

  if (port_arg_pos > 0)
    {
      SCM_ASSERT_TYPE (gdbscm_is_false (port)
		       || gdbscm_is_true (scm_input_port_p (port)),
		       port, port_arg_pos, FUNC_NAME, _("input port"));
    }
  using_port = gdbscm_is_true (port);

  if (offset_arg_pos > 0
      && (port_arg_pos < 0
	  || gdbscm_is_false (port)))
    {
      gdbscm_out_of_range_error (FUNC_NAME, offset_arg_pos,
				 gdbscm_scm_from_ulongest (offset),
				 _("offset provided but port is missing"));
    }

  if (size_arg_pos > 0)
    {
      if (size == 0)
	return SCM_EOL;
      /* For now be strict about start+size overflowing.  If it becomes
	 a nuisance we can relax things later.  */
      if (start + size < start)
	{
	  gdbscm_out_of_range_error (FUNC_NAME, 0,
				scm_list_2 (gdbscm_scm_from_ulongest (start),
					    gdbscm_scm_from_ulongest (size)),
				     _("start+size overflows"));
	}
      end = start + size - 1;
    }
  else
    end = ~(CORE_ADDR) 0;

  if (count == 0)
    return SCM_EOL;

  result = SCM_EOL;

  for (pc = start, i = 0; pc <= end && i < count; )
    {
      int insn_len = 0;
      string_file buf;

      gdbscm_gdb_exception exc {};
      try
	{
	  if (using_port)
	    {
	      insn_len = gdbscm_print_insn_from_port (gdbarch, port, offset,
						      pc, &buf, NULL);
	    }
	  else
	    insn_len = gdb_print_insn (gdbarch, pc, &buf, NULL);
	}
      catch (const gdb_exception &except)
	{
	  exc = unpack (except);
	}

      GDBSCM_HANDLE_GDB_EXCEPTION (exc);
      result = scm_cons (dascm_make_insn (pc, buf.c_str (), insn_len),
			 result);

      pc += insn_len;
      i++;
    }

  return scm_reverse_x (result, SCM_EOL);
}

/* Initialize the Scheme architecture support.  */

static const scheme_function disasm_functions[] =
{
  { "arch-disassemble", 2, 0, 1, as_a_scm_t_subr (gdbscm_arch_disassemble),
    "\
Return list of disassembled instructions in memory.\n\
\n\
  Arguments: <gdb:arch> start-address\n\
      [#:port port] [#:offset address]\n\
      [#:size <integer>] [#:count <integer>]\n\
    port: If non-#f, it is an input port to read bytes from.\n\
    offset: Specifies the address offset of the first byte in the port.\n\
      This is useful if the input is from something other than memory\n\
      (e.g., a bytevector) and you want the result to be as if the bytes\n\
      came from that address.  The value to pass for start-address is\n\
      then also the desired disassembly address, not the offset in, e.g.,\n\
      the bytevector.\n\
    size: Limit the number of bytes read to this amount.\n\
    count: Limit the number of instructions to this amount.\n\
\n\
  Returns:\n\
    Each instruction in the result is an alist:\n\
      (('address . address) ('asm . disassembly) ('length . length))." },

  END_FUNCTIONS
};

void
gdbscm_initialize_disasm (void)
{
  gdbscm_define_functions (disasm_functions, 1);

  port_keyword = scm_from_latin1_keyword ("port");
  offset_keyword = scm_from_latin1_keyword ("offset");
  size_keyword = scm_from_latin1_keyword ("size");
  count_keyword = scm_from_latin1_keyword ("count");

  address_symbol = scm_from_latin1_symbol ("address");
  asm_symbol = scm_from_latin1_symbol ("asm");
  length_symbol = scm_from_latin1_symbol ("length");
}
