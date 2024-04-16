/* This testcase is part of GDB, the GNU debugger.

   Copyright 2016-2024 Free Software Foundation, Inc.

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

static int *kernel_user_helper_version = (int *) 0xffff0ffc;

typedef void * (kernel_user_func_t)(void);
#define kernel_user_get_tls (*(kernel_user_func_t *) 0xffff0fe0)

int
main (void)
{
  int i;

  for (i = 0; i < 8; i++)
    kernel_user_get_tls ();
}
