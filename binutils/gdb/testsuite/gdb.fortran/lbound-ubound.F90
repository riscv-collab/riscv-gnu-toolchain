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

#define DO_TEST(ARRAY)	\
  call do_test (lbound (ARRAY), ubound (ARRAY))

subroutine do_test (lb, ub)
  integer*4, dimension (:) :: lb
  integer*4, dimension (:) :: ub

  print *, ""	! Test Breakpoint
end subroutine do_test

!
! Start of test program.
!
program test
  use ISO_C_BINDING, only: C_NULL_PTR, C_SIZEOF

  interface
     subroutine do_test (lb, ub)
       integer*4, dimension (:) :: lb
       integer*4, dimension (:) :: ub
     end subroutine do_test
  end interface

  ! Declare variables used in this test.
  integer, dimension (-8:-1,-10:-2) :: neg_array
  integer, dimension (2:10,1:9), target :: array
  integer, allocatable :: other (:, :)
  character (len=26) :: str_1 = "abcdefghijklmnopqrstuvwxyz"
  integer, dimension (-2:2,-3:3,-1:5) :: array3d
  integer, dimension (-3:3,7:10,-4:2,-10:-7) :: array4d
  integer, dimension (10:20) :: array1d
  integer, dimension(:,:), pointer :: pointer2d => null()
  integer, dimension(-2:6,-1:9), target :: tarray
  integer :: an_int

  integer, dimension (:), pointer :: pointer1d => null()

  integer, parameter :: b1 = 127 - 10
  integer, parameter :: b1_o = 127 + 2
  integer, parameter :: b2 = 32767 - 10
  integer, parameter :: b2_o = 32767 + 3

  ! This tests the GDB overflow behavior when using a KIND parameter too small
  ! to hold the actual output argument.  This is done for 1, 2, and 4 byte
  ! overflow.  On 32-bit machines most compilers will complain when trying to
  ! allocate an array with ranges outside the 4 byte integer range.
  ! We take the byte size of a C pointer as indication as to whether or not we
  ! are on a 32 bit machine an skip the 4 byte overflow tests in that case.
  integer, parameter :: bytes_c_ptr = C_SIZEOF(C_NULL_PTR)

  integer*8, parameter :: max_signed_4byte_int = 2147483647
  integer*8, parameter :: b4 = max_signed_4byte_int - 10
  integer*8 :: b4_o
  logical :: is_64_bit

  integer, allocatable :: array_1d_1bytes_overflow (:)
  integer, allocatable :: array_1d_2bytes_overflow (:)
  integer, allocatable :: array_1d_4bytes_overflow (:)
  integer, allocatable :: array_2d_1byte_overflow (:,:)
  integer, allocatable :: array_2d_2bytes_overflow (:,:)
  integer, allocatable :: array_3d_1byte_overflow (:,:,:)

  ! Set the 4 byte overflow only on 64 bit machines.
  if (bytes_c_ptr < 8) then
    b4_o = 0
    is_64_bit = .FALSE.
  else
    b4_o = max_signed_4byte_int + 5
    is_64_bit = .TRUE.
  end if

  ! Allocate or associate any variables as needed.
  allocate (other (-5:4, -2:7))
  pointer2d => tarray
  pointer1d => array (3, 2:5)

  allocate (array_1d_1bytes_overflow (-b1_o:-b1))
  allocate (array_1d_2bytes_overflow (b2:b2_o))
  if (is_64_bit) then
    allocate (array_1d_4bytes_overflow (-b4_o:-b4))
  end if
  allocate (array_2d_1byte_overflow (-b1_o:-b1,b1:b1_o))
  allocate (array_2d_2bytes_overflow (b2:b2_o,-b2_o:b2))

  allocate (array_3d_1byte_overflow (-b1_o:-b1,b1:b1_o,-b1_o:-b1))

  DO_TEST (neg_array)
  DO_TEST (neg_array (-7:-3,-5:-4))
  DO_TEST (array)
  ! The following is disabled due to a bug in gfortran:
  !   https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99027
  ! gfortran generates the incorrect expected results.
  ! DO_TEST (array (3, 2:5))
  DO_TEST (pointer1d)
  DO_TEST (other)
  DO_TEST (array3d)
  DO_TEST (array4d)
  DO_TEST (array1d)
  DO_TEST (pointer2d)
  DO_TEST (tarray)

  DO_TEST (array_1d_1bytes_overflow)
  DO_TEST (array_1d_2bytes_overflow)

  if (is_64_bit) then
    DO_TEST (array_1d_4bytes_overflow)
  end if
  DO_TEST (array_2d_1byte_overflow)
  DO_TEST (array_2d_2bytes_overflow)
  DO_TEST (array_3d_1byte_overflow)

  ! All done.  Deallocate.
  print *, "" ! Breakpoint before deallocate.
  deallocate (other)

  deallocate (array_3d_1byte_overflow)

  deallocate (array_2d_2bytes_overflow)
  deallocate (array_2d_1byte_overflow)

  if (is_64_bit) then
    deallocate (array_1d_4bytes_overflow)
  end if
  deallocate (array_1d_2bytes_overflow)
  deallocate (array_1d_1bytes_overflow)

  ! GDB catches this final breakpoint to indicate the end of the test.
  print *, "" ! Final Breakpoint.

  ! Reference otherwise unused locals in order to keep them around.
  ! GDB will make use of these for some tests.
  print *, str_1
  an_int = 1
  print *, an_int

end program test
