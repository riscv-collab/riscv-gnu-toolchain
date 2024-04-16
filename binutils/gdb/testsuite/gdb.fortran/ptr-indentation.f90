! Copyright 2017-2024 Free Software Foundation, Inc.
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

program main
  implicit none

  type tuserdef
    integer :: i
    real, pointer :: ptr
  end type tuserdef
  real, target :: rval

  type(tuserdef), target:: tinsta,tinstb,tinstc

  tinsta%i = 4
  tinsta%ptr => rval                        !BP1

end program main
