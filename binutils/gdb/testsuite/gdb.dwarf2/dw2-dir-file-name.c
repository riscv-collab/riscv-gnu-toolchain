/* This testcase is part of GDB, the GNU debugger.

   Copyright 2012-2024 Free Software Foundation, Inc.

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

volatile int v;

static void
marker (void)
{
  v++;
}

/* *R* marks possibly invalid compiler output as the first path component is
   not absolute.  Still DWARF-4 does not forbid such DWARF; GCC does not
   produce it.  */

#define FUNCBLOCK						\
FUNC (compdir_missing__ldir_missing__file_basename)	   /*R*/\
FUNC (compdir_missing__ldir_missing__file_relative)	   /*R*/\
FUNC (compdir_missing__ldir_missing__file_absolute)		\
FUNC (compdir_missing__ldir_relative_file_basename)	   /*R*/\
FUNC (compdir_missing__ldir_relative_file_relative)	   /*R*/\
FUNC (compdir_missing__ldir_relative_file_absolute)	   /*R*/\
FUNC (compdir_missing__ldir_absolute_file_basename)		\
FUNC (compdir_missing__ldir_absolute_file_relative)		\
FUNC (compdir_missing__ldir_absolute_file_absolute_same)	\
FUNC (compdir_missing__ldir_absolute_file_absolute_different)	\
FUNC (compdir_relative_ldir_missing__file_basename)	   /*R*/\
FUNC (compdir_relative_ldir_missing__file_relative)	   /*R*/\
FUNC (compdir_relative_ldir_missing__file_absolute)	   /*R*/\
FUNC (compdir_relative_ldir_relative_file_basename)	   /*R*/\
FUNC (compdir_relative_ldir_relative_file_relative)	   /*R*/\
FUNC (compdir_relative_ldir_relative_file_absolute)	   /*R*/\
FUNC (compdir_relative_ldir_absolute_file_basename)	   /*R*/\
FUNC (compdir_relative_ldir_absolute_file_relative)	   /*R*/\
FUNC (compdir_relative_ldir_absolute_file_absolute_same)   /*R*/\
FUNC (compdir_relative_ldir_absolute_file_absolute_different) /*R*/\
FUNC (compdir_absolute_ldir_missing__file_basename)		\
FUNC (compdir_absolute_ldir_missing__file_relative)		\
FUNC (compdir_absolute_ldir_missing__file_absolute_same)	\
FUNC (compdir_absolute_ldir_missing__file_absolute_different)	\
FUNC (compdir_absolute_ldir_relative_file_basename)		\
FUNC (compdir_absolute_ldir_relative_file_relative)		\
FUNC (compdir_absolute_ldir_relative_file_absolute_same)	\
FUNC (compdir_absolute_ldir_relative_file_absolute_different)	\
FUNC (compdir_absolute_ldir_absolute_file_basename_same)	\
FUNC (compdir_absolute_ldir_absolute_file_basename_different)	\
FUNC (compdir_absolute_ldir_absolute_file_relative_same)	\
FUNC (compdir_absolute_ldir_absolute_file_relative_different)	\
FUNC (compdir_absolute_ldir_absolute_file_absolute_same)	\
FUNC (compdir_absolute_ldir_absolute_file_absolute_different)

#define FUNC(name)					\
  static void						\
  name (void)						\
  {							\
    asm (#name "_label: .globl " #name "_label\n");	\
    v++;						\
  }
FUNCBLOCK
#undef FUNC

int
main (void)
{

#define FUNC(name)					\
  name ();
FUNCBLOCK
#undef FUNC

  return 0;
}
