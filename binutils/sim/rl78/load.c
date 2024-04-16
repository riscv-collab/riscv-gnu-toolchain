/* load.c --- loading object files into the RL78 simulator.

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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libiberty.h"
#include "bfd.h"
#include "bfd/elf-bfd.h"
#include "elf/rl78.h"
#include "cpu.h"
#include "mem.h"
#include "load.h"
#include "elf/internal.h"
#include "elf/common.h"

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

void
rl78_load (bfd *prog, host_callback *callbacks, const char * const simname)
{
  Elf_Internal_Phdr * phdrs;
  long sizeof_phdrs;
  int num_headers;
  int i;
  int max_rom = 0;

  init_cpu ();

  /* Note we load by ELF program header not by BFD sections.
     This is because BFD sections get their information from
     the ELF section structure, which only includes a VMA value
     and not an LMA value.  */
  sizeof_phdrs = bfd_get_elf_phdr_upper_bound (prog);
  if (sizeof_phdrs == 0)
    {
      fprintf (stderr, "%s: Failed to get size of program headers\n", simname);
      return;
    }
  phdrs = xmalloc (sizeof_phdrs);

  num_headers = bfd_get_elf_phdrs (prog, phdrs);
  if (num_headers < 1)
    {
      fprintf (stderr, "%s: Failed to read program headers\n", simname);
      return;
    }

  switch (elf_elfheader (prog)->e_flags & E_FLAG_RL78_CPU_MASK)
    {
    case E_FLAG_RL78_G10:
      rl78_g10_mode = 1;
      g13_multiply = 0;
      g14_multiply = 0;
      mem_set_mirror (0, 0xf8000, 4096);
      break;
    case E_FLAG_RL78_G13:
      rl78_g10_mode = 0;
      g13_multiply = 1;
      g14_multiply = 0;
      break;
    case E_FLAG_RL78_G14:
      rl78_g10_mode = 0;
      g13_multiply = 0;
      g14_multiply = 1;
      break;
    default:
      /* Keep whatever was manually specified.  */
      break;
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
      if (callbacks)
	xprintf (callbacks,
		 "Loading section %s, size %#" PRIx64 " "
		 "lma %08" PRIx64 " vma %08" PRIx64 "\n",
		 find_section_name_by_offset (prog, p->p_offset),
		 (uint64_t) size, (uint64_t) base, (uint64_t) p->p_vaddr);

      buf = xmalloc (size);

      offset = p->p_offset;
      if (bfd_seek (prog, offset, SEEK_SET) != 0)
	{
	  fprintf (stderr, "%s, Failed to seek to offset %lx\n", simname, (long) offset);
	  continue;
	}

      if (bfd_read (buf, size, prog) != size)
	{
	  fprintf (stderr, "%s: Failed to read %" PRIx64 " bytes\n",
		   simname, (uint64_t) size);
	  continue;
	}

      if (base > 0xeffff || base + size > 0xeffff)
	{
	  fprintf (stderr,
		   "%s, Can't load image to RAM/SFR space: 0x%" PRIx64 " "
		   "- 0x%" PRIx64 "\n",
		   simname, (uint64_t) base, (uint64_t) (base + size));
	  continue;
	}
      if (max_rom < base + size)
	max_rom = base + size;

      mem_put_blk (base, buf, size);
      free (buf);
    }

  free (phdrs);

  mem_rom_size (max_rom);

  pc = prog->start_address;

  if (strcmp (bfd_get_target (prog), "srec") == 0
      || pc == 0)
    {
      pc = mem_get_hi (0);
    }

  if (verbose > 1)
    fprintf (stderr, "[start pc=%08x]\n", (unsigned int) pc);
}
