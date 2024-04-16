! Copyright 2021-2024 Free Software Foundation, Inc.
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

!
! Start of test program.
!
program test

  ! Things to point at.
  integer, target :: array_1d (1:10) = 0
  integer, target :: array_2d (1:10, 1:10) = 0
  integer, target :: an_integer = 0
  integer, target :: other_integer = 0
  real, target :: a_real = 0.0

  ! Things to point with.
  integer, pointer :: array_1d_p (:) => null ()
  integer, pointer :: other_1d_p (:) => null ()
  integer, pointer :: array_2d_p (:,:) => null ()
  integer, pointer :: an_integer_p => null ()
  integer, pointer :: other_integer_p => null ()
  real, pointer :: a_real_p => null ()

  ! The start of the tests.
  call test_associated (associated (array_1d_p))
  call test_associated (associated (array_1d_p, array_1d))

  array_1d_p => array_1d
  call test_associated (associated (array_1d_p, array_1d))

  array_1d_p => array_1d (2:10)
  call test_associated (associated (array_1d_p, array_1d))

  array_1d_p => array_1d (1:9)
  call test_associated (associated (array_1d_p, array_1d))

  array_1d_p => array_2d (3, :)
  call test_associated (associated (array_1d_p, array_1d))
  call test_associated (associated (array_1d_p, array_2d (2, :)))
  call test_associated (associated (array_1d_p, array_2d (3, :)))

  array_1d_p => null ()
  call test_associated (associated (array_1d_p))
  call test_associated (associated (array_1d_p, array_2d (3, :)))

  call test_associated (associated (an_integer_p))
  call test_associated (associated (an_integer_p, an_integer))
  an_integer_p => an_integer
  call test_associated (associated (an_integer_p))
  call test_associated (associated (an_integer_p, an_integer))

  call test_associated (associated (an_integer_p, other_integer_p))
  other_integer_p => other_integer
  call test_associated (associated (other_integer_p))
  call test_associated (associated (an_integer_p, other_integer_p))
  call test_associated (associated (other_integer_p, an_integer_p))
  call test_associated (associated (other_integer_p, an_integer))

  other_integer_p = an_integer_p
  call test_associated (associated (an_integer_p, other_integer_p))
  call test_associated (associated (other_integer_p, an_integer_p))

  call test_associated (associated (a_real_p))
  call test_associated (associated (a_real_p, a_real))
  a_real_p => a_real
  call test_associated (associated (a_real_p, a_real))

  ! Setup for final tests, these are performed at the print line
  ! below.  These final tests are all error conditon checks,
  ! i.e. things that can't be compiled into Fortran.
  array_1d_p => array_1d

  print *, "" ! Final Breakpoint
  print *, an_integer
  print *, a_real

contains

  subroutine test_associated (answer)
    logical :: answer

    print *,answer	! Test Breakpoint
  end subroutine test_associated

end program test
