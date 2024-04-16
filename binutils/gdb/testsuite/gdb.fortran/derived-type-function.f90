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
! This file is the Fortran source file for derived-type-function.exp.
! It was contributed by Walfred Tedeschi (walfred.tedeschi@intel.com).

module class_Rectangle
  implicit none
  private

  type, public :: Rectangle
     real :: a
     real :: b
   contains
     procedure :: area => rectangle_area
     procedure :: print_area => print_area
  end type Rectangle
contains

  function rectangle_area (this) result (area)
    class (Rectangle), intent (in) :: this

    real :: area
    area = this%a * this%b
  end function rectangle_area

  subroutine print_area (this)
    class (Rectangle), intent (in) :: this
    real :: area

    area = this%area ()
    print *, ' area = ', area
  end subroutine print_area

end module class_Rectangle


program rectangle_Test
  use class_Rectangle
  implicit none

  type (Rectangle) :: aRec
  real areaE

  aRec = Rectangle (2., 3.)
  ! bp
  call aRec%print_area

end program rectangle_Test

