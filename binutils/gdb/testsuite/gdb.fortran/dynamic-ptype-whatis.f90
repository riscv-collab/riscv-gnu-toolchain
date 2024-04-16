! Copyright 2021-2024 Free Software Foundation, Inc.
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

program main

  ! A non-dynamic type.
  type type1
     integer(kind=4) :: spacer
     integer(kind=4) t1_i
  end type type1

  ! A first dynamic type.  The array is of a static type.
  type type2
     integer(kind=4) :: spacer
     type(type1), allocatable :: t2_array(:)
  end type type2

  ! Another dynamic type, the array is again a static type.
  type type3
     integer(kind=4) :: spacer
     type(type1), pointer :: t3_array(:)
  end type type3

  ! A dynamic type, this time the array contains a dynamic type.
  type type4
     integer(kind=4) :: spacer
     type(type2), allocatable :: t4_array(:)
  end type type4

  ! A static type, the array though contains dynamic types.
  type type5
     integer(kind=4) :: spacer
     type(type2) :: t5_array (4)
  end type type5

  ! A static type containing pointers to a type that contains a
  ! dynamic array.
  type type6
     type(type2), pointer :: ptr_1
     type(type2), pointer :: ptr_2
  end type type6

  real, dimension(:), pointer :: var1
  real, dimension(:), allocatable :: var2
  type(type1) :: var3
  type(type2), target :: var4
  type(type3) :: var5
  type(type4) :: var6
  type(type5) :: var7
  type(type6) :: var8

  allocate (var1 (3))

  allocate (var2 (4))

  allocate (var4%t2_array(3))

  allocate (var5%t3_array(3))

  allocate (var6%t4_array(3))
  allocate (var6%t4_array(1)%t2_array(2))
  allocate (var6%t4_array(2)%t2_array(5))
  allocate (var6%t4_array(3)%t2_array(4))

  allocate (var7%t5_array(1)%t2_array(2))
  allocate (var7%t5_array(2)%t2_array(5))
  allocate (var7%t5_array(3)%t2_array(4))
  allocate (var7%t5_array(4)%t2_array(1))

  var8%ptr_1 => var4
  var8%ptr_2 => var4

  print *, var1		! Break Here
  print *, var2
  print *, var3
  print *, var4%t2_array(1)
  print *, var5%t3_array(2)
  print *, var6%t4_array(1)%t2_array(1)
  print *, var7%t5_array(1)%t2_array(1)

end program main
