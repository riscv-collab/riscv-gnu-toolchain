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

  ! Things to ask questions about.
  integer, target :: array_1d (8:10) = 0
  integer, target :: array_2d (1:3, 4:7) = 0
  integer :: other_1d (4:5, -3:-1, 99:101) = 0
  integer, pointer :: array_1d_p (:) => null ()
  integer, pointer :: array_2d_p (:,:) => null ()

  integer :: an_integer = 0
  real :: a_real = 0.0

  ! The start of the tests.
  call test_rank (rank (array_1d))
  call test_rank (rank (array_2d))
  call test_rank (rank (other_1d))
  call test_rank (rank (array_1d_p))
  call test_rank (rank (array_2d_p))

  array_1d_p => array_1d
  array_2d_p => array_2d

  call test_rank (rank (array_1d_p))
  call test_rank (rank (array_2d_p))

  call test_rank (rank (an_integer))
  call test_rank (rank (a_real))

  print *, "" ! Final Breakpoint

contains

  subroutine test_rank (answer)
    integer :: answer

    print *,answer	! Test Breakpoint
  end subroutine test_rank

end program test
