! Copyright 2008-2024 Free Software Foundation, Inc.
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
!
! Ihis file is the Fortran source file for dynamic.exp.
! Original file written by Jakub Jelinek <jakub@redhat.com>.
! Modified for the GDB testcase by Jan Kratochvil <jan.kratochvil@redhat.com>.

subroutine in

   INTEGER*4            ix
   REAL*4               iy2
   REAL*8               iz

   INTEGER*4            ix_x
   REAL*4               iy_y
   REAL*8               iz_z2

   common /fo_o/ix,iy2,iz
   common /foo/ix_x,iy_y,iz_z2

   iy = 5
   iz_z = 55

   if (ix .ne. 11 .or. iy2 .ne. 22.0 .or. iz .ne. 33.0) call abort
   if (ix_x .ne. 1 .or. iy_y .ne. 2.0 .or. iz_z2 .ne. 3.0) call abort

   ix = 0					! stop-here-in

end subroutine in

program common_test

   INTEGER*4            ix
   REAL*4               iy
   REAL*8               iz

   INTEGER*4            ix_x
   REAL*4               iy_y
   REAL*8               iz_z

   common /foo/ix,iy,iz
   common /fo_o/ix_x,iy_y,iz_z

   ix = 1
   iy = 2.0
   iz = 3.0

   ix_x = 11
   iy_y = 22.0
   iz_z = 33.0

   call in					! stop-here-out

end program common_test
