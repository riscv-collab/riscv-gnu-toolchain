/* This test program is part of GDB, the GNU debugger.

   Copyright 2011-2024 Free Software Foundation, Inc.

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

/* Simulate loading of JIT code.  */

#include <elf.h>
#include <link.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jit-protocol.h"
#include "jit-elf-util.h"

static void
usage (void)
{
  fprintf (stderr, "Usage: jit-elf-main libraries...\n");
  exit (1);
}

/* Must be defined by .exp file when compiling to know
   what address to map the ELF binary to.  */
#ifndef LOAD_ADDRESS
#error "Must define LOAD_ADDRESS"
#endif
#ifndef LOAD_INCREMENT
#error "Must define LOAD_INCREMENT"
#endif

int
main (int argc, char *argv[])
{
  int i;
  alarm (300);
  /* Used as backing storage for GDB to populate argv.  */
  char *fake_argv[10];

  if (argc < 2)
    {
      usage ();
      exit (1);
    }

  for (i = 1; i < argc; ++i)
    {
      size_t obj_size;
      void *load_addr = (void *) (size_t) (LOAD_ADDRESS + (i - 1) * LOAD_INCREMENT);
      printf ("Loading %s as JIT at %p\n", argv[i], load_addr);
      void *addr = load_elf (argv[i], &obj_size, load_addr);

      char name[32];
      sprintf (name, "jit_function_%04d", i);
      int (*jit_function) (void) = (int (*) (void)) load_symbol (addr, name);

      /* Link entry at the end of the list.  */
      struct jit_code_entry *const entry = calloc (1, sizeof (*entry));
      entry->symfile_addr = (const char *)addr;
      entry->symfile_size = obj_size;
      entry->prev_entry = __jit_debug_descriptor.relevant_entry;
      __jit_debug_descriptor.relevant_entry = entry;

      if (entry->prev_entry != NULL)
	entry->prev_entry->next_entry = entry;
      else
	__jit_debug_descriptor.first_entry = entry;

      /* Notify GDB.  */
      __jit_debug_descriptor.action_flag = JIT_REGISTER;
      __jit_debug_register_code ();

      if (jit_function () != 42)
	{
	  fprintf (stderr, "unexpected return value\n");
	  exit (1);
	}
    }

  i = 0;  /* break before fork */

  fork ();

  i = 0;  /* break after fork */

  /* Now unregister them all in reverse order.  */
  while (__jit_debug_descriptor.relevant_entry != NULL)
    {
      struct jit_code_entry *const entry =
	__jit_debug_descriptor.relevant_entry;
      struct jit_code_entry *const prev_entry = entry->prev_entry;

      if (prev_entry != NULL)
	{
	  prev_entry->next_entry = NULL;
	  entry->prev_entry = NULL;
	}
      else
	__jit_debug_descriptor.first_entry = NULL;

      /* Notify GDB.  */
      __jit_debug_descriptor.action_flag = JIT_UNREGISTER;
      __jit_debug_register_code ();

      __jit_debug_descriptor.relevant_entry = prev_entry;
      free (entry);
    }

  return 0;  /* break before return  */
}
