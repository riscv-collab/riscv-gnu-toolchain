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

#ifdef JIT_H
#error "We don't include jit.h directly since we'd like the jit-reader unit  \
        tests to break if we make ABI incompatible changes to the structures \
        re-declared here."
#endif

#ifndef JIT_PROTOCOL_H
#define JIT_PROTOCOL_H

#include <stdint.h>

typedef enum
{
  JIT_NOACTION = 0,
  JIT_REGISTER,
  JIT_UNREGISTER
} jit_actions_t;


struct jit_code_entry
{
  struct jit_code_entry *next_entry;
  struct jit_code_entry *prev_entry;
  const void *symfile_addr;
  uint64_t symfile_size;
};


struct jit_descriptor
{
  uint32_t version;
  uint32_t action_flag;
  struct jit_code_entry *relevant_entry;
  struct jit_code_entry *first_entry;
};

struct jit_descriptor __jit_debug_descriptor = { 1, 0, 0, 0 };

void __attribute__((noinline)) __jit_debug_register_code()
{
}

#endif /* JIT_PROTOCOL_H */
