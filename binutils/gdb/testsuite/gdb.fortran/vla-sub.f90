! Copyright 2015-2024 Free Software Foundation, Inc.
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
! Original file written by Jakub Jelinek <jakub@redhat.com> and
! Jan Kratochvil <jan.kratochvil@redhat.com>.
! Modified for the GDB testcases by Keven Boell <keven.boell@intel.com>.

subroutine foo (array1, array2)
  integer :: array1 (:, :)
  real    :: array2 (:, :, :)

  array1(:,:) = 5                       ! not-filled
  array1(1, 1) = 30

  array2(:,:,:) = 6                     ! array1-filled
  array2(:,:,:) = 3
  array2(1,1,1) = 30
  array2(3,3,3) = 90                    ! array2-almost-filled
end subroutine

subroutine bar (array1, array2)
  integer :: array1 (*)
  integer :: array2 (4:9, 10:*)

  array1(5:10) = 1311
  array1(7) = 1
  array1(100) = 100
  array2(4,10) = array1(7)
  array2(4,100) = array1(7)
  return                                ! end-of-bar
end subroutine

program vla_sub
  interface
    subroutine foo (array1, array2)
      integer :: array1 (:, :)
      real :: array2 (:, :, :)
    end subroutine
  end interface
  interface
    subroutine bar (array1, array2)
      integer :: array1 (*)
      integer :: array2 (4:9, 10:*)
    end subroutine
  end interface

  real, allocatable :: vla1 (:, :, :)
  integer, allocatable :: vla2 (:, :)

  ! used for subroutine
  integer :: sub_arr1(42, 42)
  real    :: sub_arr2(42, 42, 42)
  integer :: sub_arr3(42)

  sub_arr1(:,:) = 1                   ! vla2-deallocated
  sub_arr2(:,:,:) = 2
  sub_arr3(:) = 3

  call foo(sub_arr1, sub_arr2)
  call foo(sub_arr1(5:10, 5:10), sub_arr2(10:15,10:15,10:15))

  allocate (vla1 (10,10,10))
  allocate (vla2 (20,20))
  vla1(:,:,:) = 1311
  vla2(:,:) = 42
  call foo(vla2, vla1)

  call bar(sub_arr3, sub_arr1)
end program vla_sub
