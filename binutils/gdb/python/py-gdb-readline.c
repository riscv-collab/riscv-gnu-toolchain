/* Readline support for Python.

   Copyright (C) 2012-2024 Free Software Foundation, Inc.

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

#include "defs.h"
#include "python-internal.h"
#include "top.h"
#include "cli/cli-utils.h"

/* Readline function suitable for PyOS_ReadlineFunctionPointer, which
   is used for Python's interactive parser and raw_input.  In both
   cases, sys_stdin and sys_stdout are always stdin and stdout
   respectively, as far as I can tell; they are ignored and
   command_line_input is used instead.  */

static char *
gdbpy_readline_wrapper (FILE *sys_stdin, FILE *sys_stdout,
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 4
			const char *prompt)
#else
			char *prompt)
#endif
{
  int n;
  const char *p = NULL;
  std::string buffer;
  char *q;

  try
    {
      p = command_line_input (buffer, prompt, "python");
    }
  /* Handle errors by raising Python exceptions.  */
  catch (const gdb_exception_forced_quit &e)
    {
      quit_force (NULL, 0);
    }
  catch (const gdb_exception &except)
    {
      /* Detect user interrupt (Ctrl-C).  */
      if (except.reason == RETURN_QUIT)
	return NULL;


      /* This readline callback is called without the GIL held.  */
      gdbpy_gil gil;

      gdbpy_convert_exception (except);
      return NULL;
    }

  /* Detect EOF (Ctrl-D).  */
  if (p == NULL)
    {
      q = (char *) PyMem_RawMalloc (1);
      if (q != NULL)
	q[0] = '\0';
      return q;
    }

  n = strlen (p);

  /* Copy the line to Python and return.  */
  q = (char *) PyMem_RawMalloc (n + 2);
  if (q != NULL)
    {
      strcpy (q, p);
      q[n] = '\n';
      q[n + 1] = '\0';
    }
  return q;
}

/* Initialize Python readline support.  */

static int CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
gdbpy_initialize_gdb_readline (void)
{
  /* Python's readline module conflicts with GDB's use of readline
     since readline is not reentrant.  Ideally, a reentrant wrapper to
     GDB's readline should be implemented to replace Python's readline
     and prevent conflicts.  For now, this file implements a
     sys.meta_path finder that simply fails to import the readline
     module.  */
  if (PyRun_SimpleString ("\
import sys\n\
\n\
class GdbRemoveReadlineFinder:\n\
  def find_module(self, fullname, path=None):\n\
    if fullname == 'readline' and path is None:\n\
      return self\n\
    return None\n\
\n\
  def load_module(self, fullname):\n\
    raise ImportError('readline module disabled under GDB')\n\
\n\
sys.meta_path.append(GdbRemoveReadlineFinder())\n\
") == 0)
    PyOS_ReadlineFunctionPointer = gdbpy_readline_wrapper;

  return 0;
}

GDBPY_INITIALIZE_FILE (gdbpy_initialize_gdb_readline);
