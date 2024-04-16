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
! along with this program.  If not, see <http://www.gnu.org/licenses/>.

program derived_type_member_stride
    type cartesian
        integer(kind=8) :: x
        integer(kind=8) :: y
        integer(kind=8) :: z
    end type
    type mixed_cartesian
        integer(kind=8) :: x
        integer(kind=4) :: y
        integer(kind=8) :: z
    end type
    type(cartesian), dimension(10), target :: cloud
    type(mixed_cartesian), dimension(10), target :: mixed_cloud
    integer(kind=8), dimension(:), pointer :: point_dimension => null()
    integer(kind=8), dimension(:), pointer :: point_mixed_dimension => null()
    type(cartesian), dimension(:), pointer :: cloud_slice => null()
    cloud(:)%x = 1
    cloud(:)%y = 2
    cloud(:)%z = 3
    cloud_slice => cloud(3:2:-2)
    point_dimension => cloud(1:9)%y
    mixed_cloud(:)%x = 1
    mixed_cloud(:)%y = 2
    mixed_cloud(:)%z = 3
    point_mixed_dimension => mixed_cloud(1:4)%z
    ! Prevent the compiler from optimising the work out.
    print *, cloud(:)%x ! post_init
    print *, point_dimension
    print *, point_mixed_dimension
end program
