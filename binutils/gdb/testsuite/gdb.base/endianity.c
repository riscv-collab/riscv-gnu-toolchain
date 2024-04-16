/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

/* This tests the handling of dwarf attributes:
    DW_AT_endianity, DW_END_big, and DW_END_little.  */
struct otherendian
{
  int v;
  short w;
  unsigned x : 3;
  float f;
  __complex__ float cplx;
  double d;
}
#if TEST_SSO
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
__attribute__( ( scalar_storage_order( "big-endian" ) ) )
#else
__attribute__( ( scalar_storage_order( "little-endian" ) ) )
#endif
#endif
;

void
do_nothing (struct otherendian *c)
{
}

int
main (void)
{
  struct otherendian o = {3, 2, 7, 23.5, 1.25 + 7.25i, 75};

  do_nothing (&o); /* START */

  return 0;
}
