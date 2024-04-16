/* Copyright 2008-2024 Free Software Foundation, Inc.

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

#include <stdio.h>

/* Start a transaction.  To avoid the need for FPR save/restore, assume
   that no FP- or vector registers are modified within the transaction.
   Thus invoke TBEGIN with the "allow floating-point operation" flag set
   to zero, which forces a transaction abort when hitting an FP- or vector
   instruction.  Also assume that TBEGIN will eventually succeed, so just
   retry indefinitely.  */

static void
my_tbegin ()
{
  __asm__ volatile
    ( "1:  .byte 0xe5,0x60,0x00,0x00,0xff,0x00\n"
      "    jnz 1b"
      : /* no return value */
      : /* no inputs */
      : "cc", "memory" );
}

/* End a transaction.  */

static void
my_tend ()
{
  __asm__ volatile
    ( "    .byte 0xb2,0xf8,0x00,0x00"
      : /* no return value */
      : /* no inputs */
      : "cc", "memory" );
}

void
try_transaction (void)
{
  my_tbegin ();
  my_tend ();
}

void
crash_in_transaction (void)
{
  volatile char *p = 0;

  my_tbegin ();
  *p = 5;			/* FAULT */
  my_tend ();
}

int
main (int argc, char *argv[])
{
  try_transaction ();
  crash_in_transaction ();
  return 0;
}
