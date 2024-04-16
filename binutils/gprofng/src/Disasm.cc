/* Copyright (C) 2021-2024 Free Software Foundation, Inc.
   Contributed by Oracle.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include "disassemble.h"
#include "dis-asm.h"
#include "demangle.h"
#include "dbe_types.h"
#include "DbeSession.h"
#include "Elf.h"
#include "Disasm.h"
#include "Stabs.h"
#include "i18n.h"
#include "util.h"
#include "StringBuilder.h"
#include "Function.h"

struct DisContext
{
  bool is_Intel;
  Stabs *stabs;
  uint64_t pc;          // first_pc <= pc < last_pc
  uint64_t first_pc;
  uint64_t last_pc;
  uint64_t f_offset;    // file offset for first_pc
  int codeptr[4];       // longest instruction length may not be > 16
  Data_window *elf;
};

static const int MAX_DISASM_STR     = 2048;
static const int MAX_INSTR_SIZE     = 8;

Disasm::Disasm (char *fname)
{
  dwin = NULL;
  dis_str = NULL;
  need_swap_endian = false;
  my_stabs = Stabs::NewStabs (fname, fname);
  if (my_stabs == NULL)
    return;
  stabs = my_stabs;
  platform = stabs->get_platform ();
  disasm_open ();
}

Disasm::Disasm (Platform_t _platform, Stabs *_stabs)
{
  dwin = NULL;
  dis_str = NULL;
  need_swap_endian = false;
  stabs = _stabs;
  platform = _platform;
  my_stabs = NULL;
  disasm_open ();
}

static int
fprintf_func (void *arg, const char *fmt, ...)
{
  char buf[512];
  va_list vp;
  va_start (vp, fmt);
  int cnt = vsnprintf (buf, sizeof (buf), fmt, vp);
  va_end (vp);

  Disasm *dis = (Disasm *) arg;
  dis->dis_str->append (buf);
  return cnt;
}

static int
fprintf_styled_func (void *arg, enum disassembler_style st ATTRIBUTE_UNUSED,
		      const char *fmt, ...)
{
  char buf[512];
  va_list vp;
  va_start (vp, fmt);
  int cnt = vsnprintf (buf, sizeof (buf), fmt, vp);
  va_end (vp);

  Disasm *dis = (Disasm *) arg;
  dis->dis_str->append (buf);
  return cnt;
}

/* Get LENGTH bytes from info's buffer, at target address memaddr.
   Transfer them to myaddr.  */
static int
read_memory_func (bfd_vma memaddr, bfd_byte *myaddr, unsigned int length,
		  disassemble_info *info)
{
  unsigned int opb = info->octets_per_byte;
  size_t end_addr_offset = length / opb;
  size_t max_addr_offset = info->buffer_length / opb;
  size_t octets = (memaddr - info->buffer_vma) * opb;
  if (memaddr < info->buffer_vma
      || memaddr - info->buffer_vma > max_addr_offset
      || memaddr - info->buffer_vma + end_addr_offset > max_addr_offset
      || (info->stop_vma && (memaddr >= info->stop_vma
			     || memaddr + end_addr_offset > info->stop_vma)))
    return -1;
  memcpy (myaddr, info->buffer + octets, length);
  return 0;
}

static void
print_address_func (bfd_vma addr, disassemble_info *info)
{
  bfd_signed_vma off;
  unsigned long long ta;
  Disasm *dis;
  switch (info->insn_type)
    {
    case dis_branch:
    case dis_condbranch:
      off = (bfd_signed_vma) addr;
      dis = (Disasm *) info->stream;
      ta = dis->inst_addr + off;
      (*info->fprintf_func) (info->stream, ".%c0x%llx [ 0x%llx ]",
		off > 0 ? '+' : '-', (long long) (off > 0 ? off : -off), ta);
      return;
    case dis_jsr:
      off = (bfd_signed_vma) addr;
      dis = (Disasm *) info->stream;
      ta = dis->inst_addr + off;
      const char *nm = NULL;
      Function *f = dis->map_PC_to_func (ta);
      if (f)
	{
	  if (dis->inst_addr >= f->img_offset
	      && dis->inst_addr < f->img_offset + f->size)
	    {	// Same function
	      (*info->fprintf_func) (info->stream, ".%c0x%llx [ 0x%llx ]",
		  off > 0 ? '+' : '-', (long long) (off > 0 ? off : -off), ta);
	      return;
	    }
	  if (f->flags & FUNC_FLAG_PLT)
	    nm = dis->get_funcname_in_plt(ta);
	  if (nm == NULL)
	    nm = f->get_name ();
	}
      if (nm)
	(*info->fprintf_func) (info->stream, "%s [ 0x%llx, .%c0x%llx]",
	    nm, ta, off > 0 ? '+' : '-', (long long) (off > 0 ? off : -off));
      else
	(*info->fprintf_func) (info->stream,
		".%c0x%llx [ 0x%llx ]  // Unable to determine target symbol",
		off > 0 ? '+' : '-', (long long) (off > 0 ? off : -off), ta);
      return;
    }
  (*info->fprintf_func) (info->stream, "0x%llx", (long long) addr);
}

static asymbol *
symbol_at_address_func (bfd_vma addr ATTRIBUTE_UNUSED,
			disassemble_info *info ATTRIBUTE_UNUSED)
{
  return NULL;
}

static bfd_boolean
symbol_is_valid (asymbol * sym ATTRIBUTE_UNUSED,
		 disassemble_info *info ATTRIBUTE_UNUSED)
{
  return TRUE;
}

static void
memory_error_func (int status, bfd_vma addr, disassemble_info *info)
{
  info->fprintf_func (info->stream, "Address 0x%llx is out of bounds.\n",
		      (unsigned long long) addr);
}

void
Disasm::disasm_open ()
{
  hex_visible = 1;
  snprintf (addr_fmt, sizeof (addr_fmt), NTXT ("%s"), NTXT ("%8llx:  "));
  if (dis_str == NULL)
    dis_str = new StringBuilder;

  switch (platform)
    {
    case Aarch64:
    case Intel:
    case Amd64:
      need_swap_endian = (DbeSession::platform == Sparc);
      break;
    case Sparcv8plus:
    case Sparcv9:
    case Sparc:
    default:
      need_swap_endian = (DbeSession::platform != Sparc);
      break;
    }

  memset (&dis_info, 0, sizeof (dis_info));
  dis_info.flavour = bfd_target_unknown_flavour;
  dis_info.endian = BFD_ENDIAN_UNKNOWN;
  dis_info.endian_code = dis_info.endian;
  dis_info.octets_per_byte = 1;
  dis_info.disassembler_needs_relocs = FALSE;
  dis_info.fprintf_func = fprintf_func;
  dis_info.fprintf_styled_func = fprintf_styled_func;
  dis_info.stream = this;
  dis_info.disassembler_options = NULL;
  dis_info.read_memory_func = read_memory_func;
  dis_info.memory_error_func = memory_error_func;
  dis_info.print_address_func = print_address_func;
  dis_info.symbol_at_address_func = symbol_at_address_func;
  dis_info.symbol_is_valid = symbol_is_valid;
  dis_info.display_endian = BFD_ENDIAN_UNKNOWN;
  dis_info.symtab = NULL;
  dis_info.symtab_size = 0;
  dis_info.buffer_vma = 0;
  switch (platform)
    {
    case Aarch64:
      dis_info.arch = bfd_arch_aarch64;
      dis_info.mach = bfd_mach_aarch64;
      break;
    case Intel:
    case Amd64:
      dis_info.arch = bfd_arch_i386;
      dis_info.mach = bfd_mach_x86_64;
      break;
    case Sparcv8plus:
    case Sparcv9:
    case Sparc:
    default:
      dis_info.arch = bfd_arch_unknown;
      dis_info.endian = BFD_ENDIAN_UNKNOWN;
      break;
    }
  dis_info.display_endian = dis_info.endian = BFD_ENDIAN_BIG;
  dis_info.display_endian = dis_info.endian = BFD_ENDIAN_LITTLE;
  dis_info.display_endian = dis_info.endian = BFD_ENDIAN_UNKNOWN;
  disassemble_init_for_target (&dis_info);
}

Disasm::~Disasm ()
{
  delete my_stabs;
  delete dwin;
  delete dis_str;
}

void
Disasm::set_img_name (char *img_fname)
{
  if (stabs == NULL && img_fname && dwin == NULL)
    {
      dwin = new Data_window (img_fname);
      if (dwin->not_opened ())
	{
	  delete dwin;
	  dwin = NULL;
	  return;
	}
      dwin->need_swap_endian = need_swap_endian;
    }
}

void
Disasm::remove_disasm_hndl (void *hndl)
{
  DisContext *ctx = (DisContext *) hndl;
  delete ctx;
}

void
Disasm::set_addr_end (uint64_t end_address)
{
  char buf[32];
  int len = snprintf (buf, sizeof (buf), "%llx", (long long) end_address);
  snprintf (addr_fmt, sizeof (addr_fmt), "%%%dllx:  ", len < 8 ? 8 : len);
}

char *
Disasm::get_disasm (uint64_t inst_address, uint64_t end_address,
		  uint64_t start_address, uint64_t f_offset, int64_t &inst_size)
{
  inst_size = 0;
  if (inst_address >= end_address)
    return NULL;
  Data_window *dw = stabs ? stabs->openElf (false) : dwin;
  if (dw == NULL)
    return NULL;

  unsigned char buffer[MAX_DISASM_STR];
  dis_info.buffer = buffer;
  dis_info.buffer_length = end_address - inst_address;
  if (dis_info.buffer_length > sizeof (buffer))
    dis_info.buffer_length = sizeof (buffer);
  dw->get_data (f_offset + (inst_address - start_address),
		dis_info.buffer_length, dis_info.buffer);

  dis_str->setLength (0);
  bfd abfd;
  disassembler_ftype disassemble = disassembler (dis_info.arch, dis_info.endian,
						 dis_info.mach, &abfd);
  if (disassemble == NULL)
    {
      printf ("ERROR: unsupported disassemble\n");
      return NULL;
    }
  inst_addr = inst_address;
  inst_size = disassemble (0, &dis_info);
  if (inst_size <= 0)
    {
      inst_size = 0;
      return NULL;
    }
  StringBuilder sb;
  sb.appendf (addr_fmt, inst_address); // Write address

  // Write hex bytes of instruction
  if (hex_visible)
    {
      char bytes[64];
      *bytes = '\0';
      for (int i = 0; i < inst_size; i++)
	{
	  unsigned int hex_value = buffer[i] & 0xff;
	  snprintf (bytes + 3 * i, sizeof (bytes) - 3 * i, "%02x ", hex_value);
	}
      const char *fmt = "%s   ";
      if (platform == Intel)
	fmt = "%-21s   "; // 21 = 3 * 7 - maximum instruction length on Intel
      sb.appendf (fmt, bytes);
    }
  sb.append (dis_str);
  return sb.toString ();
}

Function *
Disasm::map_PC_to_func (uint64_t pc)
{
  uint64_t low_pc = 0;
  if (stabs)
    return stabs->map_PC_to_func (pc, low_pc, NULL);
  return NULL;
}

const char *
Disasm::get_funcname_in_plt (uint64_t pc)
{
  if (stabs)
    {
      Elf *elf = stabs->openElf (true);
      if (elf)
	return elf->get_funcname_in_plt (pc);
    }
  return NULL;
}
