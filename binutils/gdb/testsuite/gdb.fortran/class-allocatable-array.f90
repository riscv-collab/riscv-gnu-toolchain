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

module test_module
  type test_type
     integer a
     real, allocatable :: b (:, :)
   contains
     procedure :: test_proc
  end type test_type

contains

  subroutine test_proc (this)
    class(test_type), intent (inout) :: this
    allocate (this%b (3, 2))
    call fill_array_2d (this%b)
    print *, ""		! Break Here
  contains
    ! Helper subroutine to fill 2-dimensional array with unique
    ! values.
    subroutine fill_array_2d (array)
      real, dimension (:,:) :: array
      real :: counter

      counter = 1.0
      do i=LBOUND (array, 2), UBOUND (array, 2), 1
         do j=LBOUND (array, 1), UBOUND (array, 1), 1
            array (j,i) = counter
            counter = counter + 1
         end do
      end do
    end subroutine fill_array_2d
  end subroutine test_proc
end module

program test
  use test_module
  implicit none
  type(test_type) :: t
  call t%test_proc ()
end program test
