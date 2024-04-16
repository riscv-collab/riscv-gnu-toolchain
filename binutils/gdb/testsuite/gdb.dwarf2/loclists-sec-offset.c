/* Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

static int
func1 (void)
{
  asm ("func1_label: .global func1_label\n");
  return 1;
}

static int
func2 (void)
{
  asm ("func2_label: .global func2_label\n");
  return 2;
}

static int
func3 (void)
{
  asm ("func3_label: .global func3_label\n");
  return 3;
}

static int
func4 (void)
{
  asm ("func4_label: .global func4_label\n");
  return 4;
}

static int
func5 (void)
{
  asm ("func5_label: .global func5_label\n");
  return 5;
}

static int
func6 (void)
{
  asm ("func6_label: .global func6_label\n");
  return 6;
}

int
main (void)
{
  func1 ();
  func2 ();
  func3 ();
  func4 ();
  func5 ();
  func6 ();
}
