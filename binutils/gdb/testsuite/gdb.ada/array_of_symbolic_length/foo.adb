--  Copyright 2021-2024 Free Software Foundation, Inc.
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

with Pck; use Pck;
with System;

procedure Foo is
   My_Value : My_Array := (others => 23);
   Rt : Outer := (A => (others => (A1 => (others => 4),
                                   A2 => (others => 8))));
begin
   Do_Nothing (My_Value'Address); --  BREAK
end Foo;
