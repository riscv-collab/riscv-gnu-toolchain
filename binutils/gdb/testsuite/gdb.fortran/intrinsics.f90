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
! along with this program; if not, see <http://www.gnu.org/licenses/>.

program test
  logical :: l
  logical (kind=1) :: l1
  logical (kind=2) :: l2
  logical (kind=4) :: l4
  logical (kind=8) :: l8

  type :: a_struct
     logical :: a1
     logical :: a2
  end type a_struct

  type (a_struct) :: s1

  s1%a1 = .TRUE.
  s1%a2 = .FALSE.

  l1 = .TRUE.
  l2 = .TRUE.
  l4 = .TRUE.
  l8 = .TRUE.

  l = .FALSE.					! stop-here
end
