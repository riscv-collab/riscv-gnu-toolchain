! Copyright 2010-2024 Free Software Foundation, Inc.
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

module lib
        integer :: var_i = 1
contains
        subroutine lib_func
        if (var_i .ne. 1) call abort
        var_i = 2
        var_i = 2                 ! i-is-2-in-lib
        end subroutine lib_func
end module lib

module libmany
        integer :: var_j = 3
        integer :: var_k = 4
end module libmany
