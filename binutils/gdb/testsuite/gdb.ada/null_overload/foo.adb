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

procedure Foo is

   type R_Type is null record;
   type R_Access is access R_Type;

   type U_0 is mod 1;
   type U_P_T is access all U_0;

   function F (R : R_Access) return Boolean is
   begin
      return True;
   end F;

   function F (I : Integer) return Boolean is
   begin
      return False;
   end F;

   B1 : constant Boolean := F (null);
   B2 : constant Boolean := F (0);

   U : U_0 := 0;
   U_Ptr : U_P_T := null;

begin
   null; -- START
end Foo;
