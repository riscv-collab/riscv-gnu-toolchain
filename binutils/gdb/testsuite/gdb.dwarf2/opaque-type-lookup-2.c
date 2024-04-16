/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

/* These are actually struct struct_{a,b}, but that's handled by the dwarf
   in opaque-type-lookup.exp.
   IWBN to give these a different name than what's in the dwarf so that minsym
   lookup doesn't interfere with the testing.  However, that currently doesn't
   work (we don't record the linkage name of the symbol).  */
char variable_a = 'a';
char variable_b = 'b';
