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

program internal_var_test
  type :: some_type
     integer, allocatable :: array_one (:,:)
     integer :: a_field
     integer, allocatable :: array_two (:,:)
  end type some_type

  type(some_type) :: some_var

  allocate (some_var%array_one (2,3))
  allocate (some_var%array_two (3,2))
  some_var%array_one (:,:) = 1
  some_var%a_field = 5
  some_var%array_two (:,:) = 2
  deallocate (some_var%array_one) ! Break here.
  deallocate (some_var%array_two)
end program internal_var_test
