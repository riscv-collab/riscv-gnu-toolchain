! Copyright 2016-2024 Free Software Foundation, Inc.
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

module mod1
  integer :: var_i = 1
  integer :: var_const
  parameter (var_const = 20)

CONTAINS

  SUBROUTINE sub_nested_outer
    integer :: local_int
    character (len=20) :: name

    name = 'sub_nested_outer_mod1'
    local_int = 11

  END SUBROUTINE sub_nested_outer
end module mod1

! Public sub_nested_outer
SUBROUTINE sub_nested_outer
  integer :: local_int
  character (len=16) :: name

  name = 'sub_nested_outer external'
  local_int = 11
END SUBROUTINE sub_nested_outer

! Needed indirection to call public sub_nested_outer from main
SUBROUTINE sub_nested_outer_ind
  character (len=20) :: name

  name = 'sub_nested_outer_ind'
  CALL sub_nested_outer
END SUBROUTINE sub_nested_outer_ind

! public routine with internal subroutine
SUBROUTINE sub_with_sub_nested_outer()
  integer :: local_int
  character (len=16) :: name

  name = 'subroutine_with_int_sub'
  local_int = 1

  CALL sub_nested_outer  ! Should call the internal fct

CONTAINS

  SUBROUTINE sub_nested_outer
    integer :: local_int
    local_int = 11
  END SUBROUTINE sub_nested_outer

END SUBROUTINE sub_with_sub_nested_outer

! Main
program TestNestedFuncs
  USE mod1, sub_nested_outer_use_mod1 => sub_nested_outer
  IMPLICIT NONE

  TYPE :: t_State
    integer :: code
  END TYPE t_State

  TYPE (t_State) :: v_state
  integer index, local_int

  index = 13
  CALL sub_nested_outer            ! Call internal sub_nested_outer
  CALL sub_nested_outer_ind        ! Call external sub_nested_outer via sub_nested_outer_ind
  CALL sub_with_sub_nested_outer   ! Call external routine with nested sub_nested_outer
  CALL sub_nested_outer_use_mod1   ! Call sub_nested_outer imported via module
  index = 11              ! BP_main
  v_state%code = 27

CONTAINS

  SUBROUTINE sub_nested_outer
    integer local_int
    local_int = 19
    v_state%code = index + local_int   ! BP_outer
    call sub_nested_inner
    local_int = 22                     ! BP_outer_2
    RETURN
  END SUBROUTINE sub_nested_outer

  SUBROUTINE sub_nested_inner
    integer local_int
    local_int = 17
    v_state%code = index + local_int   ! BP_inner
    RETURN
  END SUBROUTINE sub_nested_inner

end program TestNestedFuncs
