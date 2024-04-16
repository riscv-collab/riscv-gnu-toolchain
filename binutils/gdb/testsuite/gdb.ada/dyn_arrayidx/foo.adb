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

procedure Foo is
   function Range_Count (L, U : Integer) return Natural
   is
      type Array_Type is array (L .. U) of Natural;
      A : Array_Type := (others => 1);
      Result : Natural := 0;
   begin
      for I of A loop  -- START
         Result := Result + I;
      end loop;
      return Result;
   end Range_Count;

   R2 : constant Natural := Range_Count (5, 10);
begin
   null;
end Foo;
