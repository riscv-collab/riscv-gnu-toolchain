/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2024 Free Software Foundation, Inc.

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

typedef int entier;

int info_qt_inc = 0;
entier info_qt_ent = 0;

static void
setup_done (void)
{
}

static void
setup (char arg_c, int arg_i, int arg_j)
{
  char loc_arg_c = arg_c;
  int loc_arg_i = arg_i;
  int loc_arg_j = arg_j;

  info_qt_inc += loc_arg_c + loc_arg_i + loc_arg_j;
  setup_done ();
}

void info_fun1 (void)
{
  info_qt_inc++;
  info_qt_ent++;
}

int info_fun2 (char c)
{
  info_qt_inc += c;
  return info_qt_inc;
}

int info_fun2bis (char c)
{
  info_qt_inc += c;
  return info_qt_inc;
}

entier info_fun2xxx (char arg_c, int arg_i, int arg_j)
{
  info_qt_inc += arg_c + arg_i + arg_j;
  return info_qt_inc;
}

entier info_fun2yyy (char arg_c, int arg_i, int arg_j)
{
  setup (arg_c, arg_i, arg_j);
  info_qt_inc += arg_c + arg_i + arg_j;
  return info_qt_inc;
}

int
main (int argc, char **argv, char **envp)
{
  info_fun1 ();
  (void) info_fun2 ('a');
  (void) info_fun2bis ('b');
  (void) info_fun2xxx ('c', 1, 2);
  (void) info_fun2yyy ('d', 3, 4);

  return 0;
}
