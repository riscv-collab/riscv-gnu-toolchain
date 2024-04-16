! Copyright 2020-2024 Free Software Foundation, Inc.
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

  ! Declare variables used in this test.
  integer, dimension (1:10,1:10) :: array
  integer, allocatable :: other (:, :)
  integer, dimension(:,:), pointer :: pointer2d => null()
  integer, dimension(1:10,1:10), target :: tarray

  print *, "" ! First Breakpoint.

  ! Allocate or associate any variables as needed.
  allocate (other (1:10, 1:10))
  pointer2d => tarray
  array = 0

  print *, "" ! Second Breakpoint.

  ! All done.  Deallocate.
  deallocate (other)

  ! GDB catches this final breakpoint to indicate the end of the test.
  print *, "" ! Final Breakpoint.

end program test
