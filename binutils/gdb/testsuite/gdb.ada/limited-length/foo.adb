--  This testcase is part of GDB, the GNU debugger.
--
--  Copyright 2023-2024 Free Software Foundation, Inc.
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

procedure Foo is
   Large_1d_Array : array (1..64) of Integer;
   Large_3d_Array : array (1..4,1..4,1..4) of Integer;
   Count : Integer := 1;
begin
   for i in 1 .. 4 loop
     for j in 1 .. 4 loop
        for k in 1 .. 4 loop
           Large_1d_Array (Count) := Count;
           Large_3d_Array (i,j,k) := Count;
           Count := Count + 1;
        end loop;
     end loop;
   end loop;
   Do_Nothing (Large_1d_Array'Address);
   Do_Nothing (Large_3d_Array'Address); -- STOP
end Foo;

