/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

/* Architecture tests for intel i386 platform.  */

void
sse_test (void)
{
  char	buf0[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                  9, 10, 11, 12, 13, 14, 15};
  char	buf1[] = {16, 17, 18, 19, 20, 21, 22, 23,
                  24, 25, 26, 27, 28, 29, 30, 31};
  char	buf2[] = {32, 33, 34, 35, 36, 37, 38, 39,
                  40, 41, 42, 43, 44, 45, 46, 47};

  asm ("movupd %0, %%xmm0":"=m"(buf0));
  asm ("movupd %0, %%xmm1":"=m"(buf1));
  asm ("movupd %0, %%xmm2":"=m"(buf2));

  asm ("addpd %xmm0, %xmm1");
  asm ("addps %xmm1, %xmm2");
  asm ("addsd %xmm2, %xmm1");
  asm ("addss %xmm1, %xmm0");
  asm ("addsubpd %xmm0, %xmm2");
  asm ("addsubps %xmm0, %xmm1");
  asm ("andpd %xmm1, %xmm2");
  asm ("andps %xmm2, %xmm1");
  asm ("cmppd $3, %xmm0, %xmm1");
  asm ("cmpps $4, %xmm1, %xmm2");
  asm ("cmpsd $5, %xmm2, %xmm1");
  asm ("cmpss $6, %xmm1, %xmm0");
  asm ("comisd %xmm0, %xmm2");
  asm ("comiss %xmm0, %xmm1");
  asm ("cvtdq2pd %xmm1, %xmm2");
  asm ("cvtdq2ps %xmm2, %xmm1");
  asm ("cvtpd2dq %xmm1, %xmm0");
  asm ("cvtpd2ps %xmm0, %xmm1");
  asm ("divpd %xmm1, %xmm2");
  asm ("divps %xmm2, %xmm1");
  asm ("divsd %xmm1, %xmm0");
  asm ("divss %xmm0, %xmm2");
  asm ("mulpd %xmm0, %xmm1");
  asm ("mulps %xmm1, %xmm2");
  asm ("mulsd %xmm2, %xmm1");
  asm ("mulss %xmm1, %xmm0");
  asm ("orpd %xmm2, %xmm0");
  asm ("orps %xmm0, %xmm1");
  asm ("packsswb %xmm0, %xmm2");
  asm ("packssdw %xmm0, %xmm1");
  asm ("ucomisd %xmm1, %xmm2");
  asm ("ucomiss %xmm2, %xmm1");
  asm ("unpckhpd %xmm1, %xmm0");
  asm ("unpckhps %xmm2, %xmm0");
  asm ("xorpd %xmm0, %xmm1");
  asm ("xorps %xmm1, %xmm2");
} /* end sse_test */

void
ssse3_test (void)
{
  char	buf0[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                  9, 10, 11, 12, 13, 14, 15};
  char	buf1[] = {16, 17, 18, 19, 20, 21, 22, 23,
                  24, 25, 26, 27, 28, 29, 30, 31};
  char	buf2[] = {32, 33, 34, 35, 36, 37, 38, 39,
                  40, 41, 42, 43, 44, 45, 46, 47};

  asm ("movupd %0, %%xmm0":"=m"(buf0));
  asm ("movupd %0, %%xmm1":"=m"(buf1));
  asm ("movupd %0, %%xmm2":"=m"(buf2));

  asm ("pabsb %xmm1, %xmm2");
  asm ("pabsw %xmm2, %xmm1");
  asm ("pabsd %xmm1, %xmm0");
} /* end ssse3_test */

void
sse4_test (void)
{
  char	buf0[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                  9, 10, 11, 12, 13, 14, 15};
  char	buf1[] = {16, 17, 18, 19, 20, 21, 22, 23,
                  24, 25, 26, 27, 28, 29, 30, 31};
  char	buf2[] = {32, 33, 34, 35, 36, 37, 38, 39,
                  40, 41, 42, 43, 44, 45, 46, 47};

  asm ("movupd %0, %%xmm0":"=m"(buf0));
  asm ("movupd %0, %%xmm1":"=m"(buf1));
  asm ("movupd %0, %%xmm2":"=m"(buf2));

  asm ("blendpd $1, %xmm1, %xmm0");
  asm ("blendps $2, %xmm2, %xmm0");
  asm ("blendvpd %xmm0, %xmm1, %xmm2");
  asm ("blendvps %xmm0, %xmm2, %xmm1");
} /* end sse4_test */

int
main ()
{
  sse_test ();
  ssse3_test ();
  sse4_test ();
  return 0;	/* end of main */
}
