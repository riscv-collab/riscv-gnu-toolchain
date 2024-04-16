/* This test program is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

/* Simulates loading of JIT code by memory mapping a compiled
   shared library binary and doing minimal post-processing.  */

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* ElfW is coming from linux. On other platforms it does not exist.
   Let us define it here. */
#ifndef ElfW
#if (defined(_LP64) || defined(__LP64__))
#define WORDSIZE 64
#else
#define WORDSIZE 32
#endif /* _LP64 || __LP64__  */
#define ElfW(type) _ElfW (Elf, WORDSIZE, type)
#define _ElfW(e, w, t) _ElfW_1 (e, w, _##t)
#define _ElfW_1(e, w, t) e##w##t
#endif /* !ElfW  */

/* Find symbol with the name `sym_name`.  */
static void *
load_symbol (void *addr, const char *sym_name)
{
  const ElfW (Ehdr) *const ehdr = (ElfW (Ehdr) *) addr;
  ElfW (Shdr) *const shdr = (ElfW (Shdr) *) ((char *) addr + ehdr->e_shoff);

  ElfW (Addr) sym_old_addr = 0;
  ElfW (Addr) sym_new_addr = 0;

  /* Find `func_name` in symbol_table and return its address.  */
  int i;
  for (i = 0; i < ehdr->e_shnum; ++i)
    {
      if (shdr[i].sh_type == SHT_SYMTAB)
	{
	  ElfW (Sym) *symtab = (ElfW (Sym) *) (addr + shdr[i].sh_offset);
	  ElfW (Sym) *symtab_end
	      = (ElfW (Sym) *) (addr + shdr[i].sh_offset + shdr[i].sh_size);
	  char *const strtab
	      = (char *) (addr + shdr[shdr[i].sh_link].sh_offset);

	  ElfW (Sym) *p;
	  for (p = symtab; p < symtab_end; ++p)
	    {
	      const char *s = strtab + p->st_name;
	      if (strcmp (s, sym_name) == 0)
	        return (void *) p->st_value;
	    }
	}
    }

  fprintf (stderr, "symbol '%s' not found\n", sym_name);
  exit (1);
  return 0;
}

/* Open an elf binary file and memory map it with execution flag enabled.  */
static void *
load_elf (const char *libname, size_t *size, void *load_addr)
{
  int fd;
  struct stat st;

  if ((fd = open (libname, O_RDONLY)) == -1)
    {
      fprintf (stderr, "open (\"%s\", O_RDONLY): %s\n", libname,
	       strerror (errno));
      exit (1);
    }

  if (fstat (fd, &st) != 0)
    {
      fprintf (stderr, "fstat (\"%d\"): %s\n", fd, strerror (errno));
      exit (1);
    }

  void *addr = mmap (load_addr, st.st_size,
  		     PROT_READ | PROT_WRITE | PROT_EXEC,
		     load_addr != NULL ? MAP_PRIVATE | MAP_FIXED : MAP_PRIVATE,
		     fd, 0);
  close (fd);

  if (addr == MAP_FAILED)
    {
      fprintf (stderr, "mmap: %s\n", strerror (errno));
      exit (1);
    }

  if (size != NULL)
    *size = st.st_size;

  return addr;
}
