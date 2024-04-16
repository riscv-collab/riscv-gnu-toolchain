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

  ! Things to perform tests on.
  integer, target :: array_1d (1:10) = 0
  integer, target :: array_2d (1:4, 1:3) = 0
  integer :: an_integer = 0
  real :: a_real = 0.0
  integer, pointer :: array_1d_p (:) => null ()
  integer, pointer :: array_2d_p (:,:) => null ()
  integer, allocatable :: allocatable_array_1d (:)
  integer, allocatable :: allocatable_array_2d (:,:)

  call test_shape (shape (array_1d))
  call test_shape (shape (array_2d))
  call test_shape (shape (an_integer))
  call test_shape (shape (a_real))

  call test_shape (shape (array_1d (1:10:2)))
  call test_shape (shape (array_1d (1:10:3)))

  call test_shape (shape (array_2d (4:1:-1, 3:1:-1)))
  call test_shape (shape (array_2d (4:1:-1, 1:3:2)))

  allocate (allocatable_array_1d (-10:-5))
  allocate (allocatable_array_2d (-3:3, 8:12))

  call test_shape (shape (allocatable_array_1d))
  call test_shape (shape (allocatable_array_2d))

  call test_shape (shape (allocatable_array_2d (-2, 10:12)))

  array_1d_p => array_1d
  array_2d_p => array_2d

  call test_shape (shape (array_1d_p))
  call test_shape (shape (array_2d_p))

  deallocate (allocatable_array_1d)
  deallocate (allocatable_array_2d)
  array_1d_p => null ()
  array_2d_p => null ()

  print *, "" ! Final Breakpoint
  print *, an_integer
  print *, a_real
  print *, associated (array_1d_p)
  print *, associated (array_2d_p)
  print *, allocated (allocatable_array_1d)
  print *, allocated (allocatable_array_2d)

contains

  subroutine test_shape (answer)
    integer, dimension (:) :: answer

    print *,answer	! Test Breakpoint
  end subroutine test_shape

end program test
