! Copyright 2015-2024 Free Software Foundation, Inc.
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

program vla
  real, target, allocatable :: vla1 (:, :, :)
  real, target, allocatable :: vla2 (:, :, :)
  real, target, allocatable :: vla3 (:, :)
  real, pointer :: pvla (:, :, :)
  logical :: l
  nullify(pvla)

  allocate (vla1 (10,10,10))          ! vla1-init
  l = allocated(vla1)

  allocate (vla2 (1:7,42:50,13:35))   ! vla1-allocated
  l = allocated(vla2)

  vla1(:, :, :) = 1311                ! vla2-allocated
  vla1(3, 6, 9) = 42
  vla1(1, 3, 8) = 1001
  vla1(6, 2, 7) = 13

  vla2(:, :, :) = 1311                ! vla1-filled
  vla2(5, 45, 20) = 42

  pvla => vla1                        ! vla2-filled
  l = associated(pvla)

  pvla => vla2                        ! pvla-associated
  l = associated(pvla)
  pvla(5, 45, 20) = 1
  pvla(7, 45, 14) = 2

  pvla => null()                      ! pvla-re-associated
  l = associated(pvla)

  deallocate (vla1)                   ! pvla-deassociated
  l = allocated(vla1)

  deallocate (vla2)                   ! vla1-deallocated
  l = allocated(vla2)

  allocate (vla3 (2,2))               ! vla2-deallocated
  vla3(:,:) = 13

  allocate (vla1 (-2:-1, -5:-2, -3:-1))
  vla1(:, :, :) = 1
  vla1(-2, -3, -1) = -231

  deallocate (vla1)                   ! vla1-neg-bounds-v1
  l = allocated(vla1)

  allocate (vla1 (-2:1, -5:2, -3:1))
  vla1(:, :, :) = 2
  vla1(-2, -4, -2) = -242

  deallocate (vla1)                   ! vla1-neg-bounds-v2
  l = allocated(vla1)

end program vla
