/* This testcase is part of GDB, the GNU debugger.

   Copyright 2022-2024 Free Software Foundation, Inc.

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

#include <assert.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/wait.h>

int
main (void)
{
  pid_t pid = fork ();
  if (pid == 0)
    {
      void *shlib = dlopen (SHLIB_PATH, RTLD_NOW);
      int (*add) (int, int) = dlsym (shlib, "add");

      return add (-2, 2);
    }

  int wstatus;
  if (waitpid (pid, &wstatus, 0) == -1)
    assert (WIFEXITED (wstatus) && WEXITSTATUS (wstatus) == 0);

  return 0;
}
