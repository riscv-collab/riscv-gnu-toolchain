! Copyright 2009-2024 Free Software Foundation, Inc.
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

module mod3
  integer :: mod2 = 3
  integer :: mod1 = 3
  integer :: var_i = 3
contains
  subroutine check_all
    if (mod2 .ne. 3) call abort
    if (mod1 .ne. 3) call abort
    if (var_i .ne. 3) call abort
  end subroutine check_all

  subroutine check_mod2
    if (mod2 .ne. 3) call abort
  end subroutine check_mod2
end module mod3

module modmany
  integer :: var_a = 10, var_b = 11, var_c = 12, var_i = 14
contains
  subroutine check_some
    if (var_a .ne. 10) call abort
    if (var_b .ne. 11) call abort
  end subroutine check_some
end module modmany

module moduse
  integer :: var_x = 30, var_y = 31
contains
  subroutine check_all
    if (var_x .ne. 30) call abort
    if (var_y .ne. 31) call abort
  end subroutine check_all

  subroutine check_var_x
    if (var_x .ne. 30) call abort
  end subroutine check_var_x
end module moduse

subroutine sub1
  use mod1
  if (var_i .ne. 1) call abort
  var_i = var_i                         ! i-is-1
end subroutine sub1

subroutine sub2
  use mod2
  if (var_i .ne. 2) call abort
  var_i = var_i                         ! i-is-2
end subroutine sub2

subroutine sub3
  use mod3
  var_i = var_i                         ! i-is-3
end subroutine sub3

program module

  use modmany, only: var_b, var_d => var_c, var_i
  use moduse, var_z => var_y

  call sub1
  call sub2
  call sub3

  if (var_b .ne. 11) call abort
  if (var_d .ne. 12) call abort
  if (var_i .ne. 14) call abort
  if (var_x .ne. 30) call abort
  if (var_z .ne. 31) call abort
  var_b = var_b                         ! a-b-c-d

end program module
