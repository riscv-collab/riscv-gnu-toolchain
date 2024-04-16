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
! along with this program.  If not, see <http://www.gnu.org/licenses/> .

! Source code for function-calls.exp.

subroutine no_arg_subroutine()
end subroutine

logical function no_arg()
    no_arg = .TRUE.
end function

subroutine run(a)
    external :: a
    call a()
end subroutine

logical function one_arg(x)
    logical, intent(in) :: x
    one_arg = x
end function

integer(kind=4) function one_arg_value(x)
    integer(kind=4), value :: x
    one_arg_value = x
end function

integer(kind=4) function several_arguments(a, b, c)
    integer(kind=4), intent(in) :: a
    integer(kind=4), intent(in) :: b
    integer(kind=4), intent(in) :: c
    several_arguments = a + b + c
end function

integer(kind=4) function mix_of_scalar_arguments(a, b, c)
    integer(kind=4), intent(in) :: a
    logical(kind=4), intent(in) :: b
    real(kind=8), intent(in) :: c
    mix_of_scalar_arguments = a + floor(c)
    if (b) then
        mix_of_scalar_arguments=mix_of_scalar_arguments+1
    end if
end function

real(kind=4) function real4_argument(a)
    real(kind=4), intent(in) :: a
    real4_argument = a
end function

integer(kind=4) function return_constant()
    return_constant = 17
end function

character(40) function return_string()
    return_string='returned in hidden first argument'
end function

recursive function fibonacci(n) result(item)
    integer(kind=4) :: item
    integer(kind=4), intent(in) :: n
    select case (n)
        case (0:1)
            item = n
        case default
            item = fibonacci(n-1) + fibonacci(n-2)
    end select
end function

complex function complex_argument(a)
    complex, intent(in) :: a
    complex_argument = a
end function

integer(kind=4) function array_function(a)
    integer(kind=4), dimension(11) :: a
    array_function = a(ubound(a, 1, 4))
end function

integer(kind=4) function pointer_function(int_pointer)
    integer, pointer :: int_pointer
    pointer_function = int_pointer
end function

integer(kind=4) function hidden_string_length(string)
  character*(*) :: string
  hidden_string_length = len(string)
end function

integer(kind=4) function sum_some(a, b, c)
    integer :: a, b
    integer, optional :: c
    sum_some = a + b
    if (present(c)) then
        sum_some = sum_some + c
    end if
end function

module derived_types_and_module_calls
    type cart
        integer :: x
        integer :: y
    end type
    type cart_nd
        integer :: x
        integer, allocatable :: d(:)
    end type
    type nested_cart_3d
        type(cart) :: d
        integer :: z
    end type
contains
    type(cart) function pass_cart(c)
        type(cart) :: c
        pass_cart = c
    end function
    integer(kind=4) function pass_cart_nd(c)
        type(cart_nd) :: c
        pass_cart_nd = ubound(c%d,1,4)
    end function
    type(nested_cart_3d) function pass_nested_cart(c)
        type(nested_cart_3d) :: c
        pass_nested_cart = c
    end function
    type(cart) function build_cart(x,y)
        integer :: x, y
        build_cart%x = x
        build_cart%y = y
    end function
end module

program function_calls
    use derived_types_and_module_calls
    implicit none
    interface
        logical function no_arg()
        end function
        logical function one_arg(x)
            logical, intent(in) :: x
        end function
        integer(kind=4) function pointer_function(int_pointer)
            integer, pointer :: int_pointer
        end function
        integer(kind=4) function several_arguments(a, b, c)
            integer(kind=4), intent(in) :: a
            integer(kind=4), intent(in) :: b
            integer(kind=4), intent(in) :: c
        end function
        complex function complex_argument(a)
            complex, intent(in) :: a
        end function
            real(kind=4) function real4_argument(a)
            real(kind=4), intent(in) :: a
        end function
        integer(kind=4) function return_constant()
        end function
        character(40) function return_string()
        end function
        integer(kind=4) function one_arg_value(x)
            integer(kind=4), value :: x
        end function
        integer(kind=4) function sum_some(a, b, c)
            integer :: a, b
            integer, optional :: c
        end function
        integer(kind=4) function mix_of_scalar_arguments(a, b, c)
            integer(kind=4), intent(in) :: a
            logical(kind=4), intent(in) :: b
            real(kind=8), intent(in) :: c
        end function
        integer(kind=4) function array_function(a)
            integer(kind=4), dimension(11) :: a
        end function
        integer(kind=4) function hidden_string_length(string)
            character*(*) :: string
        end function
    end interface
    logical :: untrue, no_arg_return
    complex :: fft, fft_result
    integer(kind=4), dimension (11) :: integer_array
    real(kind=8) :: real8
    real(kind=4) :: real4
    integer, pointer :: int_pointer
    integer, target :: pointee, several_arguments_return
    integer(kind=4) :: integer_return
    type(cart) :: c, cout
    type(cart_nd) :: c_nd
    type(nested_cart_3d) :: nested_c
    character(40) :: returned_string, returned_string_debugger
    external no_arg_subroutine
    real8 = 3.00
    real4 = 9.3
    integer_array = 17
    fft = cmplx(2.1, 3.3)
    print *, fft
    untrue = .FALSE.
    int_pointer => pointee
    pointee = 87
    c%x = 2
    c%y = 4
    c_nd%x = 4
    allocate(c_nd%d(4))
    c_nd%d = 6
    nested_c%z = 3
    nested_c%d%x = 1
    nested_c%d%y = 2
    ! Use everything so it is not elided by the compiler.
    call no_arg_subroutine()
    no_arg_return = no_arg() .AND. one_arg(.FALSE.)
    several_arguments_return = several_arguments(1,2,3) + return_constant()
    integer_return = array_function(integer_array)
    integer_return = mix_of_scalar_arguments(2, untrue, real8)
    real4 = real4_argument(3.4)
    integer_return = pointer_function(int_pointer)
    c = pass_cart(c)
    integer_return = pass_cart_nd(c_nd)
    nested_c = pass_nested_cart(nested_c)
    integer_return = hidden_string_length('string of implicit length')
    call run(no_arg_subroutine)
    integer_return = one_arg_value(10)
    integer_return = sum_some(1,2,3)
    returned_string = return_string()
    cout = build_cart(4,5)
    fft_result = complex_argument(fft)
    print *, cout
    print *, several_arguments_return
    print *, fft_result
    print *, real4
    print *, integer_return
    print *, returned_string_debugger
    deallocate(c_nd%d) ! post_init
end program
