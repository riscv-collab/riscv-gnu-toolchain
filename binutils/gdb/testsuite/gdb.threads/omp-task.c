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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Tests which verify (or not) that GDB can access shared and private
   clauses of OpenMP task construct.
*/

#include <stdio.h>
#include <omp.h>

int foo(int n) {
  int share1 = 9, share2 = 11, share3 = 13, priv1, priv2, fpriv;
  fpriv = n + 4;

  if (n < 2)
    return n;
  else {
#pragma omp task shared(share1, share2) private(priv1, priv2) firstprivate(fpriv) shared(share3)
    {
      priv1 = n;
      priv2 = n + 2;
      share2 += share3;
      printf("share1 = %d, share2 = %d, share3 = %d\n", share1, share2, share3);
      share1 = priv1 + priv2 + fpriv + foo(n - 1) + share2 + share3;
    }
#pragma omp taskwait
    return share1 + share2 + share3;
  }
}

int main() {
  int n = 10;
  printf("foo(%d) = %d\n", n, foo(n));
  return 0;
}
