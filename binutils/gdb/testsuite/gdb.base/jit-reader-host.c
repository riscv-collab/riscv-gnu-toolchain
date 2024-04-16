/* Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h>

#include JIT_READER_H  /* Please see jit-reader.exp for an explanation.  */
#include "jit-reader-host.h"
#include "jit-protocol.h"

struct jit_code_entry only_entry;

typedef void (jit_function_stack_mangle_t) (void);
typedef long (jit_function_add_t) (long a, long b);

/* The code of the jit_function_00 function that is copied into an
   mmapped buffer in the inferior at run time.

   The second instruction mangles the stack pointer, meaning that when
   stopped at the third instruction, GDB needs assistance from the JIT
   unwinder in order to be able to unwind successfully.  */
static const unsigned char jit_function_stack_mangle_code[] = {
  0xcc,				/* int3 */
  0x48, 0x83, 0xf4, 0xff,	/* xor $0xffffffffffffffff, %rsp */
  0x48, 0x83, 0xf4, 0xff,	/* xor $0xffffffffffffffff, %rsp */
  0xc3				/* ret */
};

/* And another "JIT-ed" function, with the prototype `jit_function_add_t`.  */
static const unsigned char jit_function_add_code[] = {
  0x48, 0x01, 0xfe,		/* add %rdi,%rsi */
  0x48, 0x89, 0xf0,		/* mov %rsi,%rax */
  0xc3,				/* retq */
};

int
main (int argc, char **argv)
{
  struct jithost_abi *symfile = malloc (sizeof (struct jithost_abi));
  char *code = mmap (NULL, getpagesize (), PROT_WRITE | PROT_EXEC,
		     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  char *code_end = code;

  assert (code != MAP_FAILED);

  /* "JIT" function_stack_mangle.  */
  memcpy (code_end, jit_function_stack_mangle_code,
	  sizeof (jit_function_stack_mangle_code));
  jit_function_stack_mangle_t *function_stack_mangle
    = (jit_function_stack_mangle_t *) code_end;
  symfile->function_stack_mangle.begin = code_end;
  code_end += sizeof (jit_function_stack_mangle_code);
  symfile->function_stack_mangle.end = code_end;

  /* "JIT" function_add.  */
  memcpy (code_end, jit_function_add_code, sizeof (jit_function_add_code));
  jit_function_add_t *function_add = (jit_function_add_t *) code_end;
  symfile->function_add.begin = code_end;
  code_end += sizeof (jit_function_add_code);
  symfile->function_add.end = code_end;

  /* Bounds of the whole object.  */
  symfile->object.begin = code;
  symfile->object.end = code_end;

  only_entry.symfile_addr = symfile;
  only_entry.symfile_size = sizeof (struct jithost_abi);

  __jit_debug_descriptor.first_entry = &only_entry;
  __jit_debug_descriptor.relevant_entry = &only_entry;
  __jit_debug_descriptor.action_flag = JIT_REGISTER;
  __jit_debug_descriptor.version = 1;
  __jit_debug_register_code ();

  function_stack_mangle ();
  function_add (5, 6);

  return 0;
}
