/* This testcase is part of GDB, the GNU debugger.

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

#include "attributes.h"

/* Our fake dynamic object.  */
void *dyn_object;

void __attribute__((noinline)) ATTRIBUTE_NOCLONE
marker ()
{ /* Nothing.  */ }

int
main ()
{
  asm ("main_label: .globl main_label");

  /* Initialise the dynamic object.  */
  dyn_object = 0;

  asm ("marker_label: .globl marker_label");
  marker ();	/* Break here.  */

  return 0;
}

