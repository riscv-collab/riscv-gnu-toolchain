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

program allocate_array

  type l_buffer
     real, dimension(:), pointer :: alpha
  end type l_buffer
  type(l_buffer), pointer :: buffer

  allocate (buffer)
  allocate (buffer%alpha (5))

  buffer%alpha (1) = 1.5
  buffer%alpha (2) = 2.5
  buffer%alpha (3) = 3.5
  buffer%alpha (4) = 4.5
  buffer%alpha (5) = 5.5

  print *, buffer%alpha	! Break Here.

end program allocate_array
