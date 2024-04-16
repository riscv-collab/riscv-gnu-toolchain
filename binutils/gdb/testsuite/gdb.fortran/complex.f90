! Copyright 2007-2024 Free Software Foundation, Inc.
!
! This program is free software; you can redistribute it and/or modify
! it under the terms of the GNU General Public License as published by
! the Free Software Foundation; either version 3 of the License, or
! (at your option) any later version.
!
! This program is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
! GNU General Public License for more details.
!
! You should have received a copy of the GNU General Public License
! along with this program.  If not, see <http://www.gnu.org/licenses/>.

program test_complex
  real*4 r4a, r4b
  real*8 r8a, r8b
  real*16 r16a, r16b
  integer ia, ib

  complex c, ci
  complex(kind=4) c4
  complex(kind=8) c8
  double complex dc
  complex(kind=16) c16

  r4a = 1000
  r4b = -50
  r8a = 321
  r8b = -22
  r16a = -874
  r16b = 19
  ia = -4
  ib = 12

  c = cmplx(r4a,r4b)
  c4 = cmplx(r4a,r4b)
  c8 = cmplx(r8a, r8b)
  dc = cmplx(r8a, r8b)
  c16 = cmplx(r16a, r16b)
  ci = cmplx(ia, ib)

  print *, c, c4, c8, dc, c16	! stop
  print *, r4a, r4b
  print *, r8a, r8b
  print *, r16a, r16b
  print *, ia, ib
end program test_complex
