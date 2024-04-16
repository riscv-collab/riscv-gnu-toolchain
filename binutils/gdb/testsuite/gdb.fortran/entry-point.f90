! Copyright 2023-2024 Free Software Foundation, Inc.
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

module mod
implicit none

contains
  subroutine mod_bar
    integer :: I = 3

    goto 100

    entry mod_foo
    I = 33

100 print *, I
  end subroutine mod_bar
end module mod


subroutine bar(I,J,K,I1)
  integer :: I,J,K,L,I1
  integer :: A
  real :: C

  A = 0
  C = 0.0

  A = I + K + I1
  goto 300

  entry foo(J,K,L,I1)
  A = J + K + L + I1

200 C = J
  goto 300

  entry foobar(J)
  goto 200

300 A = C + 1
  C = J * 1.5

  return
end subroutine

program TestEntryPoint
  use mod

  call foo(11,22,33,44)
  call bar(444,555,666,777)
  call foobar(1)

  call mod_foo()
end program TestEntryPoint
