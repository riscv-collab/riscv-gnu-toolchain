/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

/* Cheezy hack to prevent set_initial_language from trying to look up main.
   We do this so that gdb won't try to open the dwp file when the file is
   first selected.  This gives us a chance to do a chdir before attempting
   to access the debug info.  */
asm (".globl main.main");
asm ("main.main: .byte 0");

int
main (int argc, char **argv)
{
  return 0;
}
