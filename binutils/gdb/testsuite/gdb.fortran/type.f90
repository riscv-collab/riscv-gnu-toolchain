! Copyright 2013-2024 Free Software Foundation, Inc.
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

program type
  implicit none

  type :: t1
    integer :: t1_i
    real    :: t1_r
  end type t1

  type :: t2
    integer :: t2_i
    type (t1) :: t1_n
  end type t2

  type :: t3
    integer :: t3_i
    type (t2) :: t2_n
  end type t3

  type (t1) :: t1v
  type (t2) :: t2v
  type (t3), target :: t3v
  type(t3), pointer :: t3p

  nullify (t3p)

  t1v%t1_i = 42
  t1v%t1_r = 42.24

  t2v%t2_i = 2
  t2v%t1_n%t1_i = 21
  t3v%t3_i = 3
  t3v%t2_n%t2_i = 32
  t3v%t2_n%t1_n%t1_i = 321

  t3p => t3v
  nullify (t3p)    ! bp1

end program type
