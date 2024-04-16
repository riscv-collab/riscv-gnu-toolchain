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

subroutine show_elem (array)
  integer :: array

  print *, ""
  print *, "Expected GDB Output:"
  print *, ""

  write(*, fmt="(A)", advance="no") "GDB = "
  write(*, fmt="(I0)", advance="no") array
  write(*, fmt="(A)", advance="yes") ""

  print *, ""	! Display Element
end subroutine show_elem

subroutine show_str (array)
  character (len=*) :: array

  print *, ""
  print *, "Expected GDB Output:"
  print *, ""
  write (*, fmt="(A)", advance="no") "GDB = '"
  write (*, fmt="(A)", advance="no") array
  write (*, fmt="(A)", advance="yes") "'"

  print *, ""	! Display String
end subroutine show_str

subroutine show_1d (array)
  integer, dimension (:) :: array

  print *, "Array Contents:"
  print *, ""

  do i=LBOUND (array, 1), UBOUND (array, 1), 1
     write(*, fmt="(i4)", advance="no") array (i)
  end do

  print *, ""
  print *, "Expected GDB Output:"
  print *, ""

  write(*, fmt="(A)", advance="no") "GDB = ("
  do i=LBOUND (array, 1), UBOUND (array, 1), 1
     if (i > LBOUND (array, 1)) then
        write(*, fmt="(A)", advance="no") ", "
     end if
     write(*, fmt="(I0)", advance="no") array (i)
  end do
  write(*, fmt="(A)", advance="yes") ")"

  print *, ""	! Display Array Slice 1D
end subroutine show_1d

subroutine show_2d (array)
  integer, dimension (:,:) :: array

  print *, "Array Contents:"
  print *, ""

  do i=LBOUND (array, 2), UBOUND (array, 2), 1
     do j=LBOUND (array, 1), UBOUND (array, 1), 1
        write(*, fmt="(i4)", advance="no") array (j, i)
     end do
     print *, ""
  end do

  print *, ""
  print *, "Expected GDB Output:"
  print *, ""

  write(*, fmt="(A)", advance="no") "GDB = ("
  do i=LBOUND (array, 2), UBOUND (array, 2), 1
     if (i > LBOUND (array, 2)) then
        write(*, fmt="(A)", advance="no") " "
     end if
     write(*, fmt="(A)", advance="no") "("
     do j=LBOUND (array, 1), UBOUND (array, 1), 1
        if (j > LBOUND (array, 1)) then
           write(*, fmt="(A)", advance="no") ", "
        end if
        write(*, fmt="(I0)", advance="no") array (j, i)
     end do
     write(*, fmt="(A)", advance="no") ")"
  end do
  write(*, fmt="(A)", advance="yes") ")"

  print *, ""	! Display Array Slice 2D
end subroutine show_2d

subroutine show_3d (array)
  integer, dimension (:,:,:) :: array

  print *, ""
  print *, "Expected GDB Output:"
  print *, ""

  write(*, fmt="(A)", advance="no") "GDB = ("
  do i=LBOUND (array, 3), UBOUND (array, 3), 1
     if (i > LBOUND (array, 3)) then
        write(*, fmt="(A)", advance="no") " "
     end if
     write(*, fmt="(A)", advance="no") "("
     do j=LBOUND (array, 2), UBOUND (array, 2), 1
        if (j > LBOUND (array, 2)) then
           write(*, fmt="(A)", advance="no") " "
        end if
        write(*, fmt="(A)", advance="no") "("
        do k=LBOUND (array, 1), UBOUND (array, 1), 1
           if (k > LBOUND (array, 1)) then
              write(*, fmt="(A)", advance="no") ", "
           end if
           write(*, fmt="(I0)", advance="no") array (k, j, i)
        end do
        write(*, fmt="(A)", advance="no") ")"
     end do
     write(*, fmt="(A)", advance="no") ")"
  end do
  write(*, fmt="(A)", advance="yes") ")"

  print *, ""	! Display Array Slice 3D
end subroutine show_3d

subroutine show_4d (array)
  integer, dimension (:,:,:,:) :: array

  print *, ""
  print *, "Expected GDB Output:"
  print *, ""

  write(*, fmt="(A)", advance="no") "GDB = ("
  do i=LBOUND (array, 4), UBOUND (array, 4), 1
     if (i > LBOUND (array, 4)) then
        write(*, fmt="(A)", advance="no") " "
     end if
     write(*, fmt="(A)", advance="no") "("
     do j=LBOUND (array, 3), UBOUND (array, 3), 1
        if (j > LBOUND (array, 3)) then
           write(*, fmt="(A)", advance="no") " "
        end if
        write(*, fmt="(A)", advance="no") "("

        do k=LBOUND (array, 2), UBOUND (array, 2), 1
           if (k > LBOUND (array, 2)) then
              write(*, fmt="(A)", advance="no") " "
           end if
           write(*, fmt="(A)", advance="no") "("
           do l=LBOUND (array, 1), UBOUND (array, 1), 1
              if (l > LBOUND (array, 1)) then
                 write(*, fmt="(A)", advance="no") ", "
              end if
              write(*, fmt="(I0)", advance="no") array (l, k, j, i)
           end do
           write(*, fmt="(A)", advance="no") ")"
        end do
        write(*, fmt="(A)", advance="no") ")"
     end do
     write(*, fmt="(A)", advance="no") ")"
  end do
  write(*, fmt="(A)", advance="yes") ")"

  print *, ""	! Display Array Slice 4D
end subroutine show_4d

!
! Start of test program.
!
program test
  interface
     subroutine show_str (array)
       character (len=*) :: array
     end subroutine show_str

     subroutine show_1d (array)
       integer, dimension (:) :: array
     end subroutine show_1d

     subroutine show_2d (array)
       integer, dimension(:,:) :: array
     end subroutine show_2d

     subroutine show_3d (array)
       integer, dimension(:,:,:) :: array
     end subroutine show_3d

     subroutine show_4d (array)
       integer, dimension(:,:,:,:) :: array
     end subroutine show_4d
  end interface

  ! Declare variables used in this test.
  integer, dimension (-10:-1,-10:-2) :: neg_array
  integer, dimension (1:10,1:10) :: array
  integer, allocatable :: other (:, :)
  character (len=26) :: str_1 = "abcdefghijklmnopqrstuvwxyz"
  integer, dimension (-2:2,-2:2,-2:2) :: array3d
  integer, dimension (-3:3,7:10,-3:3,-10:-7) :: array4d
  integer, dimension (10:20) :: array1d
  integer, dimension(:,:), pointer :: pointer2d => null()
  integer, dimension(-1:9,-1:9), target :: tarray

  ! Allocate or associate any variables as needed.
  allocate (other (-5:4, -2:7))
  pointer2d => tarray

  ! Fill arrays with contents ready for testing.
  call fill_array_1d (array1d)

  call fill_array_2d (neg_array)
  call fill_array_2d (array)
  call fill_array_2d (other)
  call fill_array_2d (tarray)

  call fill_array_3d (array3d)
  call fill_array_4d (array4d)

  ! The tests.  Each call to a show_* function must have a unique set
  ! of arguments as GDB uses the arguments are part of the test name
  ! string, so duplicate arguments will result in duplicate test
  ! names.
  !
  ! If a show_* line ends with VARS=... where '...' is a comma
  ! separated list of variable names, these variables are assumed to
  ! be part of the call line, and will be expanded by the test script,
  ! for example:
  !
  !     do x=1,9,1
  !       do y=x,10,1
  !         call show_1d (some_array (x,y))	! VARS=x,y
  !       end do
  !     end do
  !
  ! In this example the test script will automatically expand 'x' and
  ! 'y' in order to better test different aspects of GDB.  Do take
  ! care, the expansion is not very "smart", so try to avoid clashing
  ! with other text on the line, in the example above, avoid variables
  ! named 'some' or 'array', as these will likely clash with
  ! 'some_array'.
  call show_str (str_1)
  call show_str (str_1 (1:20))
  call show_str (str_1 (10:20))

  call show_elem (array1d (11))
  call show_elem (pointer2d (2,3))

  call show_1d (array1d)
  call show_1d (array1d (13:17))
  call show_1d (array1d (17:13:-1))
  call show_1d (array (1:5,1))
  call show_1d (array4d (1,7,3,:))
  call show_1d (pointer2d (-1:3, 2))
  call show_1d (pointer2d (-1, 2:4))

  ! Enclosing the array slice argument in (...) causess gfortran to
  ! repack the array.
  call show_1d ((array (1:5,1)))

  call show_2d (pointer2d)
  call show_2d (array)
  call show_2d (array (1:5,1:5))
  do i=1,10,2
     do j=1,10,3
        call show_2d (array (1:10:i,1:10:j))	! VARS=i,j
        call show_2d (array (10:1:-i,1:10:j))	! VARS=i,j
        call show_2d (array (10:1:-i,10:1:-j))	! VARS=i,j
        call show_2d (array (1:10:i,10:1:-j))	! VARS=i,j
     end do
  end do
  call show_2d (array (6:2:-1,3:9))
  call show_2d (array (1:10:2, 1:10:2))
  call show_2d (other)
  call show_2d (other (-5:0, -2:0))
  call show_2d (other (-5:4:2, -2:7:3))
  call show_2d (neg_array)
  call show_2d (neg_array (-10:-3,-8:-4:2))

  ! Enclosing the array slice argument in (...) causess gfortran to
  ! repack the array.
  call show_2d ((array (1:10:3, 1:10:2)))
  call show_2d ((neg_array (-10:-3,-8:-4:2)))

  call show_3d (array3d)
  call show_3d (array3d(-1:1,-1:1,-1:1))
  call show_3d (array3d(1:-1:-1,1:-1:-1,1:-1:-1))

  ! Enclosing the array slice argument in (...) causess gfortran to
  ! repack the array.
  call show_3d ((array3d(1:-1:-1,1:-1:-1,1:-1:-1)))

  call show_4d (array4d)
  call show_4d (array4d (-3:0,10:7:-1,0:3,-7:-10:-1))
  call show_4d (array4d (3:0:-1, 10:7:-1, :, -7:-10:-1))

  ! Enclosing the array slice argument in (...) causess gfortran to
  ! repack the array.
  call show_4d ((array4d (3:-2:-2, 10:7:-2, :, -7:-10:-1)))

  ! All done.  Deallocate.
  deallocate (other)

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
