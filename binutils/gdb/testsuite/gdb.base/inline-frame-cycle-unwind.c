/* This testcase is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

static void inline_func (void);
static void normal_func (void);

volatile int global_var;
volatile int level_counter;

static void __attribute__((noinline))
normal_func (void)
{
  /* Do some work.  */
  ++global_var;

  /* Now the inline function.  */
  --level_counter;
  inline_func ();
  ++level_counter;

  /* Do some work.  */
  ++global_var;
}

static inline void __attribute__((__always_inline__))
inline_func (void)
{
  if (level_counter > 1)
    {
      --level_counter;
      normal_func ();
      ++level_counter;
    }
  else
    ++global_var;	/* Break here.  */
}

int
main ()
{
  level_counter = 6;
  normal_func ();
  return 0;
}
