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
  integer, dimension (1:10,1:11) :: array
  character (len=26) :: str = "abcdefghijklmnopqrstuvwxyz"

  call fill_array_2d (array)

  ! GDB catches this final breakpoint to indicate the end of the test.
  print *, "" ! Stop Here

  print *, array
  print *, str

  ! GDB catches this final breakpoint to indicate the end of the test.
  print *, "" ! Final Breakpoint.

contains

  ! Fill a 1D array with a unique positive integer in each element.
  subroutine fill_array_1d (array)
    integer, dimension (:) :: array
    integer :: counter

    counter = 1
    do j=LBOUND (array, 1), UBOUND (array, 1), 1
       array (j) = counter
       counter = counter + 1
    end do
  end subroutine fill_array_1d

  ! Fill a 2D array with a unique positive integer in each element.
  subroutine fill_array_2d (array)
    integer, dimension (:,:) :: array
    integer :: counter

    counter = 1
    do i=LBOUND (array, 2), UBOUND (array, 2), 1
       do j=LBOUND (array, 1), UBOUND (array, 1), 1
          array (j,i) = counter
          counter = counter + 1
       end do
    end do
  end subroutine fill_array_2d

  ! Fill a 3D array with a unique positive integer in each element.
  subroutine fill_array_3d (array)
    integer, dimension (:,:,:) :: array
    integer :: counter

    counter = 1
    do i=LBOUND (array, 3), UBOUND (array, 3), 1
       do j=LBOUND (array, 2), UBOUND (array, 2), 1
          do k=LBOUND (array, 1), UBOUND (array, 1), 1
             array (k, j,i) = counter
             counter = counter + 1
          end do
       end do
    end do
  end subroutine fill_array_3d

  ! Fill a 4D array with a unique positive integer in each element.
  subroutine fill_array_4d (array)
    integer, dimension (:,:,:,:) :: array
    integer :: counter

    counter = 1
    do i=LBOUND (array, 4), UBOUND (array, 4), 1
       do j=LBOUND (array, 3), UBOUND (array, 3), 1
          do k=LBOUND (array, 2), UBOUND (array, 2), 1
             do l=LBOUND (array, 1), UBOUND (array, 1), 1
                array (l, k, j,i) = counter
                counter = counter + 1
             end do
          end do
       end do
    end do
    print *, ""
  end subroutine fill_array_4d
end program test
