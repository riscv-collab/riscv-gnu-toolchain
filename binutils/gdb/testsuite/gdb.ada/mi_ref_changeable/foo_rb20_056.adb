--  Copyright 2018-2024 Free Software Foundation, Inc.
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

procedure Foo_RB20_056 is
   A : String := Get_Name;
   B : String := Get_Name;
begin
   Do_Nothing (A'Address);
   Do_Nothing (B'Address);
   B (B'First) := 's'; -- STOP_1
   Do_Nothing (A'Address); -- STOP_2
   Do_Nothing (B'Address);
end Foo_RB20_056;
