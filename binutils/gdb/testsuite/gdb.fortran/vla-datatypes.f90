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

program vla_primitives
  integer, allocatable    :: intvla(:, :, :)
  real, allocatable       :: realvla(:, :, :)
  complex, allocatable    :: complexvla(:, :, :)
  logical, allocatable    :: logicalvla(:, :, :)
  character, allocatable  :: charactervla(:, :, :)
  logical                 :: l

  allocate (intvla (11,22,33))
  allocate (realvla (11,22,33))
  allocate (complexvla (11,22,33))
  allocate (logicalvla (11,22,33))
  allocate (charactervla (11,22,33))

  l = allocated(intvla)                   ! vlas-allocated
  l = allocated(realvla)
  l = allocated(complexvla)
  l = allocated(logicalvla)
  l = allocated(charactervla)

  intvla(:,:,:) = 1
  realvla(:,:,:) = 3.14
  complexvla(:,:,:) = cmplx(2.0,-3.0)
  logicalvla(:,:,:) = .TRUE.
  charactervla(:,:,:) = char(75)

  intvla(5,5,5) = 42                      ! vlas-initialized
  realvla(5,5,5) = 4.13
  complexvla(5,5,5) = cmplx(-3.0,2.0)
  logicalvla(5,5,5) = .FALSE.
  charactervla(5,5,5) = 'X'

  ! dummy statement for bp
  l = .FALSE.                             ! vlas-modified
end program vla_primitives
