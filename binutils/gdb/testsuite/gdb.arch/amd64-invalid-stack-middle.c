/* This testcase is part of GDB, the GNU debugger.

   Copyright 2014-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

void
breakpt (void)
{
  /* Nothing.  */
}

void
func5 (void)
{
  breakpt ();
}

void
func4 (void)
{
  func5 ();
}

void
func3 (void)
{
  func4 ();
}

void
func2 (void *ptr)
{
  func3 ();
}

void
func1 (void *ptr)
{
  func2 (ptr);
}

/* Finds and returns an invalid pointer, mmaps in a page, grabs a pointer
   to it then unmaps the page again.  This is almost certainly "undefined"
   behaviour, but should be good enough for this small test program.  */

static void *
make_invalid_ptr (void)
{
  int page_size, ans;
  void *ptr;
  
  page_size = getpagesize ();
  ptr =  mmap (0, page_size, PROT_NONE,
	       MAP_PRIVATE | MAP_ANONYMOUS,
	       -1, 0);
  assert (ptr != MAP_FAILED);
  ans = munmap (ptr, page_size);
  assert (ans == 0);

  return ptr;
}

int 
main (void)
{
  void *invalid_ptr;

  invalid_ptr = make_invalid_ptr ();
  func1 (invalid_ptr);
  
  return 0;
}
