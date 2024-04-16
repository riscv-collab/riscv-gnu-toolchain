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

  integer, allocatable :: array (:, :)
  logical is_allocated

  is_allocated = allocated (array)
  print *, is_allocated ! Breakpoint 1

  ! Allocate or associate any variables as needed.
  allocate (array (-5:4, -2:7))

  is_allocated = allocated (array)
  print *, is_allocated ! Breakpoint 2

  deallocate (array)

  is_allocated = allocated (array)
  print *, is_allocated ! Breakpoint 3

  allocate (array (3:8, 2:7))

  is_allocated = allocated (array)
  print *, is_allocated ! Breakpoint 4

  ! All done.  Deallocate.
  deallocate (array)

  is_allocated = allocated (array)
  print *, is_allocated ! Breakpoint 5

end program test
