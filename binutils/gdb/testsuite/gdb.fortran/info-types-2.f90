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

! Comment just to change the line number on which
! mod2 is defined.
module mod2
  integer :: mod2_var_1 = 123
  real, parameter :: mod2_var_2 = 0.5
contains
  subroutine sub_m2_a(a, b)
    integer :: a
    logical :: b
    print*, "sub_m2_a = ", abc
    print*, "a = ", a
    print*, "b = ", b
  end subroutine sub_m2_a

  logical function sub_m2_b(x)
    real :: x
    print*, "sub_m2_b = ", cde
    print*, "x = ", x
    sub_m2_b = .true.
  end function sub_m2_b
end module mod2
