/* This testcase is part of GDB, the GNU debugger.

   Copyright 2008-2024 Free Software Foundation, Inc.

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
 * Test restoration of machine state
 */

extern void hide (int);

/* Test register variable
   Requires -- compiler honors 'register'.  */

void 
register_state (void)
{
  register int a = 0;

  hide (a);	/* External function to defeat optimization.  */
  a++;		/* register_state: set breakpoint here */
  hide (a);	/* register post-change */
}

/* Test auto variable (whatever that means).  */

void
auto_state (void)
{
  auto int a = 0;

  hide (a);	/* External function to defeat optimization.  */
  a++;		/* auto_state: set breakpoint here */
  hide (a);	/* auto post-change */
}

/* Test function-static variable.  */

void
function_static_state (void)
{
  static int a = 0;

  hide (a);	/* External function to defeat optimization.  */
  a++;		/* function_static_state: set breakpoint here */
  hide (a);	/* function static post-change */
}

/* Test module-static variable.  */

static int astatic;

void
module_static_state (void)
{
  astatic = 0;

  hide (astatic);	/* External function to defeat optimization.  */
  astatic++;		/* module_static_state: set breakpoint here */
  hide (astatic);	/* module static post-change */
}

/* Test module-global variable.  */

int aglobal;

void
module_global_state (void)
{
  aglobal = 0;

  hide (aglobal);	/* External function to defeat optimization.  */
  aglobal++;		/* module_global_state: set breakpoint here */
  hide (aglobal);	/* module global post-change */
}

/* main test driver */

int 
main (int argc, char **argv)
{
  register_state ();	/* begin main */
  auto_state ();
  function_static_state ();
  module_static_state ();
  module_global_state ();
  
  return 0;		/* end main */
}
