! Copyright 2020-2024 Free Software Foundation, Inc.
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

module type_module
  use, intrinsic :: iso_c_binding, only: c_int, c_float, c_double
  type, bind(C) :: MyType
     real(c_float) :: a
     real(c_float) :: b
  end type MyType
end module type_module

program mixed_stack_main
  implicit none

  ! Set up some locals.

  ! Call a Fortran function.
  call mixed_func_1a

  write(*,*) "All done"
end program mixed_stack_main

subroutine breakpt ()
  implicit none
  write(*,*) "Hello World"         ! Break here.
end subroutine breakpt

subroutine mixed_func_1a()
  use type_module
  implicit none

  TYPE(MyType) :: obj
  complex(kind=4) :: d

  obj%a = 1.5
  obj%b = 2.5
  d = cmplx (4.0, 5.0)

  ! Call a C function.
  call mixed_func_1b (1, 2.0, 3D0, d, "abcdef", obj)
end subroutine mixed_func_1a

! This subroutine is called from the Fortran code.
subroutine mixed_func_1b(a, b, c, d, e, g)
  use type_module
  implicit none

  integer :: a
  real(kind=4) :: b
  real(kind=8) :: c
  complex(kind=4) :: d
  character(len=*) :: e
  character(len=:), allocatable :: f
  TYPE(MyType) :: g

  interface
     subroutine mixed_func_1c (a, b, c, d, f, g) bind(C)
       use, intrinsic :: iso_c_binding, only: c_int, c_float, c_double
       use, intrinsic :: iso_c_binding, only: c_float_complex, c_char
       use type_module
       implicit none
       integer(c_int), value, intent(in) :: a
       real(c_float), value, intent(in) :: b
       real(c_double), value, intent(in) :: c
       complex(c_float_complex), value, intent(in) :: d
       character(c_char), intent(in) :: f(*)
       TYPE(MyType) :: g
     end subroutine mixed_func_1c
  end interface

  ! Create a copy of the string with a NULL terminator on the end.
  f = e//char(0)

  ! Call a C function.
  call mixed_func_1c (a, b, c, d, f, g)
end subroutine mixed_func_1b

! This subroutine is called from the C code.
subroutine mixed_func_1d(a, b, c, d, str)
  use, intrinsic :: iso_c_binding, only: c_int, c_float, c_double
  use, intrinsic :: iso_c_binding, only: c_float_complex
  implicit none
  integer(c_int) :: a
  real(c_float) :: b
  real(c_double) :: c
  complex(c_float_complex) :: d
  character(len=*) :: str

  interface
     subroutine mixed_func_1e () bind(C)
       implicit none
     end subroutine mixed_func_1e
  end interface

  write(*,*) a, b, c, d, str

  ! Call a C++ function (via an extern "C" wrapper).
  call mixed_func_1e
end subroutine mixed_func_1d

! This is called from C++ code.
subroutine mixed_func_1h ()
  call breakpt
end subroutine mixed_func_1h
