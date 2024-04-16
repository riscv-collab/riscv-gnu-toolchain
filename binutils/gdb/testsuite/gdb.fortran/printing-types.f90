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

program prog
  integer(1) :: oneByte
  integer(2) :: twoBytes
  character  :: chValue
  logical(1) :: logValue
  real(kind=16) :: rVal

  oneByte  = 1
  twoBytes = 2
  chValue  = 'a'
  logValue = .true.
  rVal = 2000
  write(*,*) s
end
