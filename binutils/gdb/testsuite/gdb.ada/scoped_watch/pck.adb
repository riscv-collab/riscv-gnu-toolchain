--  Copyright 2017-2024 Free Software Foundation, Inc.
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

package body Pck is
   function Get_Val (Seed: Integer; Off_By_One: Boolean) return Integer is
      Result : Integer := Seed;
   begin
      if Off_By_One then  --  START
         Result := Result - 1;
      end if;
      Result := Result * 8;
      if Off_By_One then
         Result := Result + 1;
      end if;
      return Result;
   end Get_Val;

   procedure Do_Nothing (Val: in out Integer) is
   begin
      null;
   end Do_Nothing;

   procedure Call_Me is
   begin
      null;
   end Call_Me;

   procedure Increment (Val : in out Integer) is
   begin
      Val := Val + 1;
   end Increment;
end Pck;
