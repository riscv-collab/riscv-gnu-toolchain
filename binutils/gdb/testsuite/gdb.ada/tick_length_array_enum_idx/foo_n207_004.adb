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

with Pck; use Pck;

procedure Foo_n207_004 is
   Full : Full_Table := (False, True, False, True, False);
   Prim : Primary_Table := (True, False, False);
   Cold : Variable_Table := (Green => False, Blue => True, White => True);
   Vars : Variable_Table :=  New_Variable_Table (Low => Red, High => Green);
   PT_Full : Full_PT := (False, True, False, True, False);
begin
   Do_Nothing (Full'Address);  -- STOP
   Do_Nothing (Prim'Address);
   Do_Nothing (Cold'Address);
   Do_Nothing (Vars'Address);
   Do_Nothing (PT_Full'Address);
end Foo_n207_004;
