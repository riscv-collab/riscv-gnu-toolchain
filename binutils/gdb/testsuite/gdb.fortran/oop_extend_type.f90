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

! Test fortran extends feature (also for chained extends).
module testmod
    implicit none
    type :: point
	real :: coo(3)
    end type

    type, extends(point) :: waypoint
	real :: angle
    end type

    type, extends(waypoint) :: fancywaypoint
	logical :: is_fancy
    end type
end module

program testprog
    use testmod
    implicit none

    logical l
    type(waypoint) :: wp
    type(fancywaypoint) :: fwp
    type(waypoint), allocatable :: wp_vla(:)

    l = .FALSE.
    allocate(wp_vla(3)) ! Before vla allocation

    l = allocated(wp_vla) ! After vla allocation

    wp%angle = 100.00
    wp%coo(:) = 1.00
    wp%coo(2) = 2.00

    fwp%is_fancy = .TRUE.
    fwp%angle = 10.00
    fwp%coo(:) = 2.00
    fwp%coo(1) = 1.00

    wp_vla(1)%angle = 101.00
    wp_vla(1)%coo(:) = 10.00
    wp_vla(1)%coo(2) = 12.00

    wp_vla(2)%angle = 102.00
    wp_vla(2)%coo(:) = 20.00
    wp_vla(2)%coo(2) = 22.00

    wp_vla(3)%angle = 103.00
    wp_vla(3)%coo(:) = 30.00
    wp_vla(3)%coo(2) = 32.00

    print *, wp, wp_vla, fwp ! After value assignment

end program
