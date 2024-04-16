/* This test is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

struct S
{
  int field1;
  int field2;
};

struct S g_var_s = { 1, 2 };

static int g_var = 1;

#ifdef __cplusplus
/* So that the alias attribute below work without having to figure out
   this function's mangled name.  */
static struct S *func (void) asm ("func");
#endif

static struct S *
func (void)
{
  return &g_var_s;
}

struct S *func_alias (void) __attribute__ ((alias ("func")));

extern struct S g_var_s_alias __attribute__ ((alias ("g_var_s")));

extern struct S g_var_s_alias2 __attribute__ ((alias ("g_var_s_alias")));

extern int g_var_alias __attribute__ ((alias ("g_var")));

extern int g_def_var_alias __attribute__ ((alias ("g_def_var")));

int g_def_var = 2;

extern int g_def_var_alias2 __attribute__ ((alias ("g_def_var_alias")));
