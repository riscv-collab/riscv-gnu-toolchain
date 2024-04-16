/* load.c --- loading object files into the RX simulator.

Copyright (C) 2005-2024 Free Software Foundation, Inc.
Contributed by Red Hat, Inc.

This file is part of the GNU simulators.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bfd.h"
#include "cpu.h"
#include "mem.h"
#include "load.h"
#include "bfd/elf-bfd.h"

/* Helper function for invoking a GDB-specified printf.  */
static void
xprintf (host_callback *callback, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);

  (*callback->vprintf_filtered) (callback, fmt, ap);

  va_end (ap);
}

/* Given a file offset, look up the section name.  */
static const char *
find_section_name_by_offset (bfd *abfd, file_ptr filepos)
{
  asection *s;

  for (s = abfd->sections; s; s = s->next)
    if (s->filepos == filepos)
      return bfd_section_name (s);

  return "(unknown)";
}

/* A note about endianness and swapping...

   The RX chip is CISC-like in that the opcodes are variable length
   and are read as a stream of bytes.  However, the chip itself shares
   the code prefetch block with the data fetch block, so when it's
   configured for big endian mode, the memory fetched for opcodes is
   word-swapped.  To compensate for this, the ELF file has the code
   sections pre-swapped.  Our BFD knows this, and for the convenience
   of all the other tools, hides this swapping at a very low level.
   I.e.  it swaps words on the way out and on the way back in.

   Fortunately the iovector routines are unaffected by this, so we
   can use them to read in the segments directly, without having
   to worry about byte swapping anything.

   However, our opcode decoder and disassemblers need to swap the data
   after reading it from the chip memory, just like the chip does.
   All in all, the code words are swapped four times between the
   assembler and our decoder.

   If the chip is running in little-endian mode, no swapping is done
   anywhere.  Note also that the *operands* within opcodes are always
   encoded in little-endian format.  */

void
rx_load (bfd *prog, host_callback *callback)
{
  unsigned long highest_addr_loaded = 0;
  Elf_Internal_Phdr * phdrs;
  long sizeof_phdrs;
  int num_headers;
  int i;

  rx_big_endian = bfd_big_endian (prog);

  /* Note we load by ELF program header not by BFD sections.
     This is because BFD sections get their information from
     the ELF section structure, which only includes a VMA value
     and not an LMA value.  */
  sizeof_phdrs = bfd_get_elf_phdr_upper_bound (prog);
  if (sizeof_phdrs == 0)
    {
      fprintf (stderr, "Failed to get size of program headers\n");
      return;
    }
  phdrs = malloc (sizeof_phdrs);
  if (phdrs == NULL)
    {
      fprintf (stderr, "Failed allocate memory to hold program headers\n");
      return;
    }
  num_headers = bfd_get_elf_phdrs (prog, phdrs);
  if (num_headers < 1)
    {
      fprintf (stderr, "Failed to read program headers\n");
      return;
    }
  
  for (i = 0; i < num_headers; i++)
    {
      Elf_Internal_Phdr * p = phdrs + i;
      char *buf;
      bfd_vma size;
      bfd_vma base;
      file_ptr offset;

      size = p->p_filesz;
      if (size <= 0)
	continue;

      base = p->p_paddr;
      if (verbose > 1)
	fprintf (stderr,
		 "[load segment: lma=%08" PRIx64 " vma=%08" PRIx64 " "
		 "size=%08" PRIx64 "]\n",
		 (uint64_t) base, (uint64_t) p->p_vaddr, (uint64_t) size);
      if (callback)
	xprintf (callback,
	         "Loading section %s, size %#" PRIx64 " lma %08" PRIx64
		 " vma %08" PRIx64 "\n",
	         find_section_name_by_offset (prog, p->p_offset),
		 (uint64_t) size, (uint64_t) base, (uint64_t) p->p_vaddr);

      buf = malloc (size);
      if (buf == NULL)
	{
	  fprintf (stderr, "Failed to allocate buffer to hold program segment\n");
	  continue;
	}
      
      offset = p->p_offset;
      if (bfd_seek (prog, offset, SEEK_SET) != 0)
	{
	  fprintf (stderr, "Failed to seek to offset %lx\n", (long) offset);
	  continue;
	}
      if (bfd_read (buf, size, prog) != size)
	{
	  fprintf (stderr, "Failed to read %" PRIx64 " bytes\n",
		   (uint64_t) size);
	  continue;
	}

      mem_put_blk (base, buf, size);
      free (buf);
      if (highest_addr_loaded < base + size - 1 && size >= 4)
	highest_addr_loaded = base + size - 1;
    }

  free (phdrs);

  regs.r_pc = prog->start_address;

  if (strcmp (bfd_get_target (prog), "srec") == 0
      || regs.r_pc == 0)
    {
      regs.r_pc = mem_get_si (0xfffffffc);
      heaptop = heapbottom = 0;
    }

  reset_decoder ();

  if (verbose > 1)
    fprintf (stderr, "[start pc=%08x %s]\n",
	     (unsigned int) regs.r_pc,
	     rx_big_endian ? "BE" : "LE");
}
