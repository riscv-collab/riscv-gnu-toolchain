--  Copyright 2008-2024 Free Software Foundation, Inc.
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

--  Test Ada additions to core GDB evaluation.

with System;
with Text_IO; use Text_IO;

procedure P is
   type Int is range System.Min_Int .. System.Max_Int;

   X, Z : Int;
   Y : Integer;

begin
   X := 0;
   -- Set X to 7 by disguised means lest a future optimizer interfere.
   for I in 1 .. 7 loop
      X := X + 1;
   end loop;
   Z := 1;
   Y := 0;
   while Z < Int'Last / X loop
      Z := Z * X;
      Y := Y + 1;
   end loop;

   Put_Line (Int'Image (X ** Y));  -- START
end P;
