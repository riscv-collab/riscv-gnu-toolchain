--  Copyright 2014-2024 Free Software Foundation, Inc.
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

with System;
with Pck; use Pck;

procedure Foo is
   type Array2_First is array (24 .. 26) of Integer;
   type Array2_Second is array (1 .. 2) of Array2_First;
   A2 : Array2_Second := ((10, 11, 12), (13, 14, 15));
begin
   Do_Nothing (A2'Address);  --  START
end Foo;
