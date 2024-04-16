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

program main
  implicit none

  interface
     integer function some_func (arg)
       integer :: arg
     end function some_func

     integer function string_func (str)
       character(len=*) :: str
     end function string_func
  end interface

  integer :: val

  val = some_func (1)
  print *, val
  val = string_func ('hello')
  print *, val
end program main
