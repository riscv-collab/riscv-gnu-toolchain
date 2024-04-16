/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

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

/* This file is not used directly; instead, it has been used to
   generate the .S file.  See comments on the .S file for more info.  */

#include <sys/sdt.h>

int main ()
{
  int a = 40;

  STAP_PROBE1 (foo, bar, a);
  return 0; 
}
