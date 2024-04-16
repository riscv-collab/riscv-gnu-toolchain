/* This testcase is part of GDB, the GNU debugger.

   Copyright 1998-2024 Free Software Foundation, Inc.

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

/*
 * Test program for tracing; circular buffer
 */

int n = 6;

int testload[13];

static void func0(void)
{
}

static void func1(void)
{
}

static void func2(void)
{
}

static void func3(void)
{
}

static void func4(void)
{
}

static void func5(void)
{
}

static void func6(void)
{
}

static void func7(void)
{
}

static void func8(void)
{
}

static void func9(void)
{
}

static void begin ()	/* called before anything else */
{
}

static void end ()	/* called after everything else */
{
}

int
main (argc, argv, envp)
     int argc;
     char *argv[], **envp;
{
  int i;

  begin ();
  for (i = 0; i < sizeof(testload) / sizeof(testload[0]); i++)
    testload[i] = i + 1;

  func0 ();
  func1 ();
  func2 ();
  func3 ();
  func4 ();
  func5();
  func6 ();
  func7 ();
  func8 ();
  func9 ();

  end ();

  return 0;
}
