! Copyright 2012-2024 Free Software Foundation, Inc.

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

      dimension foo(4294967296_8:4294967297_8)
      dimension bar(-4294967297_8:-4294967296_8)
      integer(8) :: lb, ub
      bar = 42
      foo = bar
      lb = lbound (foo, dim = 1, kind = 8)
      ub = ubound (foo, dim = 1, kind = 8)
      print *, 'bounds of foo - ', lb, ':', ub
      stop
      end

