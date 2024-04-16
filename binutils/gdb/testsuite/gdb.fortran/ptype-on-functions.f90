! Copyright 2019-2024 Free Software Foundation, Inc.
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

module some_module
  implicit none

  type, public :: Number
     integer :: a
   contains
     procedure :: get => get_number
     procedure :: set => set_number
  end type Number

contains

  function get_number (this) result (val)
    class (Number), intent (in) :: this
    integer :: val
    val = this%a
  end function get_number

  subroutine set_number (this, val)
    class (Number), intent (inout) :: this
    integer :: val
    this%a = val
  end subroutine set_number

end module some_module

logical function is_bigger (a,b)
  integer, intent(in) :: a
  integer, intent(in) :: b
  is_bigger = a > b
end function is_bigger

subroutine say_numbers (v1,v2,v3)
  integer,intent(in) :: v1
  integer,intent(in) :: v2
  integer,intent(in) :: v3
  print *, v1,v2,v3
end subroutine say_numbers

program test
  use some_module

  interface
     integer function fun1 (x)
       integer :: x
     end function fun1

     integer function fun2 (x)
       integer :: x
     end function fun2

     subroutine say_array (arr)
       integer, dimension (:,:) :: arr
     end subroutine say_array
  end interface

  type (Number) :: n1
  type (Number) :: n2

  procedure(fun1), pointer:: fun_ptr => NULL()

  integer, dimension (5,5) :: array
  array = 0

  call say_numbers (1,2,3)	! stop here
  call say_string ('hello world')
  call say_array (array (2:3, 2:4))
  print *, fun_ptr (3)

end program test

integer function fun1 (x)
  implicit none
  integer :: x
  fun1 = x + 1
end function fun1

integer function fun2 (x)
  implicit none
  integer :: x
  fun2 = x + 2
end function fun2

subroutine say_string (str)
  character(len=*) :: str
  print *, str
end subroutine say_string

subroutine say_array (arr)
  integer, dimension (:,:) :: arr
  do i=LBOUND (arr, 2), UBOUND (arr, 2), 1
     do j=LBOUND (arr, 1), UBOUND (arr, 1), 1
        write(*, fmt="(i4)", advance="no") arr (j, i)
     end do
     print *, ""
  end do
end subroutine say_array
