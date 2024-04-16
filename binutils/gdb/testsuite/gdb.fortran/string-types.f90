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

subroutine fixed_size_string_v1(s)
  character*3 s
  print *, ""	! First breakpoint.
end subroutine fixed_size_string_v1

subroutine fixed_size_string_v2(s)
  character(3) s
  print *, ""	! Second breakpoint.
end subroutine fixed_size_string_v2

subroutine variable_size_string(s)
  character*(*) s
  print *, ""	! Third breakpoint.
end subroutine variable_size_string

program test
  call fixed_size_string_v1('foo')
  call fixed_size_string_v2('foo')
  call variable_size_string('foo')
  call variable_size_string('foo' // achar(10) // achar(9) // achar(13) // &
			    achar(0) // 'bar')
end program test
