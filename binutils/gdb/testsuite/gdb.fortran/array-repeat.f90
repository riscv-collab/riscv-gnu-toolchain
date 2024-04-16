! Copyright 2022-2024 Free Software Foundation, Inc.
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

  ! Declare variables used in this test.
  integer, dimension (-2:2) :: array_1d
  integer, dimension (-2:3) :: array_1d9
  integer, dimension (-2:2, -2:2) :: array_2d
  integer, dimension (-2:3, -2:3) :: array_2d9
  integer, dimension (-2:2, -2:2, -2:2) :: array_3d
  integer, dimension (-2:3, -2:3, -2:3) :: array_3d9

  array_1d = 1
  array_1d9 = 1
  array_1d9 (3) = 9
  array_2d = 2
  array_2d9 = 2
  array_2d9 (3, :) = 9
  array_2d9 (:, 3) = 9
  array_3d = 3
  array_3d9 = 3
  array_3d9 (3, :, :) = 9
  array_3d9 (:, 3, :) = 9
  array_3d9 (:, :, 3) = 9

  print *, ""           ! Break here
  print *, array_1d
  print *, array_1d9
  print *, array_2d
  print *, array_2d9
  print *, array_3d
  print *, array_3d9

end program test
