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
!
! Check that GDB can handle block data with no global name.
!
! MAIN
        PROGRAM bdata
        DOUBLE PRECISION doub1, doub2
        CHARACTER*6 char1, char2

        COMMON /BLK1/ doub1, char1
        COMMON /BLK2/ doub2, char2

        doub1 = 11.111
        doub2 = 22.222
        char1 = 'ABCDEF'
        char2 = 'GHIJKL'
        CALL sub_block_data      ! BP_BEFORE_SUB
        STOP
        END

! BLOCK DATA
        BLOCK DATA

        DOUBLE PRECISION doub1, doub2
        CHARACTER*6 char1, char2

        COMMON /BLK1/ doub1, char1
        COMMON /BLK2/ doub2, char2
        DATA doub1, doub2 /1.111, 2.222/
        DATA char1, char2 /'abcdef', 'ghijkl'/
        END

! SUBROUTINE
        SUBROUTINE sub_block_data

        DOUBLE PRECISION doub1, doub2
        CHARACTER*6 char1, char2

        COMMON /BLK1/ doub1, char1
        COMMON /BLK2/ doub2, char2

        char1 = char2;    ! BP_SUB
        END
