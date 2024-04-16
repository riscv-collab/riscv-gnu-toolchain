--  Copyright 2012-2024 Free Software Foundation, Inc.
--
--  This program is free software; you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 3 of the License, or
--  (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this program.  If not, see <http://www.gnu.org/licenses/>.

With Classes; use Classes;

procedure P is
   SP_Access : Shape_Access := new Circle'(My_Circle);
   DP_Access : Drawable_Access := new Circle'(My_Circle);
   SP_Array : Shape_Array := (others => S_Access);
   DP_Array : Drawable_Array := (others => D_Access);
begin
   null; --  BREAK
end P;
