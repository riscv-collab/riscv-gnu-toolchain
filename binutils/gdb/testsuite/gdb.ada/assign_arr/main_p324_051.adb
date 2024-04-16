--  Copyright 2016-2024 Free Software Foundation, Inc.
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

with target_wrapper; use target_wrapper;

procedure Main_P324_051 is
   IValue : IArray (1 .. 3) := (8, 10, 12);
begin
   Assign_Arr_Input.u2 := (0.2,0.3,0.4);  -- STOP
   Put (IValue);
end Main_P324_051;
