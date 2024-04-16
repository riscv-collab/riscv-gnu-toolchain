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

subroutine show (array_1d, array_1d9, array_2d, array_2d9, array_3d, array_3d9)
  integer, dimension (-2:) :: array_1d
  integer, dimension (-2:) :: array_1d9
  integer, dimension (-2:, -2:) :: array_2d
  integer, dimension (-2:, -2:) :: array_2d9
  integer, dimension (-2:, -2:, -2:) :: array_3d
  integer, dimension (-2:, -2:, -2:) :: array_3d9

  print *, ""           ! Break here
  print *, array_1d
  print *, array_1d9
  print *, array_2d
  print *, array_2d9
  print *, array_3d
  print *, array_3d9
end subroutine show

!
! Start of test program.
!
program test
  interface
    subroutine show (array_1d, array_1d9, array_2d, array_2d9, &
		     array_3d, array_3d9)
      integer, dimension (:) :: array_1d
      integer, dimension (:) :: array_1d9
      integer, dimension (:, :) :: array_2d
      integer, dimension (:, :) :: array_2d9
      integer, dimension (:, :, :) :: array_3d
      integer, dimension (:, :, :) :: array_3d9
    end subroutine show
  end interface

  ! Declare variables used in this test.
  integer, dimension (-8:6) :: array_1d
  integer, dimension (-8:9) :: array_1d9
  integer, dimension (-8:6, -8:6) :: array_2d
  integer, dimension (-8:9, -8:9) :: array_2d9
  integer, dimension (-8:6, -8:6, -8:6) :: array_3d
  integer, dimension (-8:9, -8:9, -8:9) :: array_3d9

  integer, parameter :: v6 (6) = [-5, -4, -3, 1, 2, 3]
  integer, parameter :: v9 (9) = [-5, -4, -3, 1, 2, 3, 7, 8, 9]

  ! Intersperse slices selected with varying data to make sure it is
  ! correctly ignored for the purpose of repeated element recognition
  ! in the slices.
  array_1d = 7
  array_1d (::3) = 1
  array_1d9 = 7
  array_1d9 (::3) = 1
  array_1d9 (7) = 9
  array_2d = 7
  array_2d (:, v6) = 6
  array_2d (::3, ::3) = 2
  array_2d9 = 7
  array_2d9 (:, v9) = 6
  array_2d9 (::3, ::3) = 2
  array_2d9 (7, ::3) = 9
  array_2d9 (::3, 7) = 9
  array_3d = 7
  array_3d (:, v6, :) = 6
  array_3d (:, v6, v6) = 5
  array_3d (::3, ::3, ::3) = 3
  array_3d9 = 7
  array_3d9 (:, v9, :) = 6
  array_3d9 (:, v9, v9) = 5
  array_3d9 (::3, ::3, ::3) = 3
  array_3d9 (7, ::3, ::3) = 9
  array_3d9 (::3, 7, ::3) = 9
  array_3d9 (::3, ::3, 7) = 9

  call show (array_1d (::3), array_1d9 (::3), &
	     array_2d (::3, ::3), array_2d9 (::3, ::3), &
	     array_3d (::3, ::3, ::3), array_3d9 (::3, ::3, ::3))

  print *, array_1d
  print *, array_1d9
  print *, array_2d
  print *, array_2d9
  print *, array_3d
  print *, array_3d9

end program test
