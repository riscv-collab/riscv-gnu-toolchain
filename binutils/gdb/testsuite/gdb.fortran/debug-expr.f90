! Copyright 2020-2024 Free Software Foundation, Inc.

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

program main
  type data_item
     integer(kind=4) i, j, k
  end type data_item

  type level_one
     type(data_item) :: one(3)
  end type level_one

  type level_two
     type(level_one) :: two(3)
  end type level_two

  type level_three
     type(level_two) :: three(3)
  end type level_three

  type (level_three) obj

  obj%three(1)%two(1)%one(1)%i = 1

  print *, obj ! Break Here
end program
