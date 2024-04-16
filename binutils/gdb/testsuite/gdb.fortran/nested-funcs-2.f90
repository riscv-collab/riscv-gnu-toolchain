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

module container
    implicit none
    integer :: a
    contains
    subroutine print_from_module()
       print *, "hello."
    end subroutine
end module

program contains_keyword
    use container
    implicit none
    integer :: program_i, program_j
    program_j = 12 ! pre_init
    program_i = 7
    program_j = increment(program_j) ! pre_increment
    program_i = increment_program_global() ! pre_increment_program_global
    call subroutine_to_call()
    call step() ! pre_step
    call hidden_variable()
    call print_from_module()
    print '(I2)', program_j, program_i ! post_init

contains
    subroutine subroutine_to_call()
       print *, "called"
    end subroutine
    integer function increment(i)
       integer :: i
       increment = i + 1
       print *, i ! post_increment
    end function
    integer function increment_program_global()
       increment_program_global = program_i + 1
       ! Need to put in a dummy print here to break on as on some systems the
       ! variables leave scope at "end function", but on others they do not.
       print *, program_i ! post_increment_global
    end function
    subroutine step()
       print '(A)', "step" ! post_step
    end subroutine
    subroutine hidden_variable()
       integer :: program_i
       program_i = 30
       print *, program_i ! post_hidden
    end subroutine
end program contains_keyword
