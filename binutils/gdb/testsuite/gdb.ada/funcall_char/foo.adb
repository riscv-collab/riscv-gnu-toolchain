--  Copyright 2015-2024 Free Software Foundation, Inc.
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

   function F (C : Character) return Integer is
   begin
      return Character'Pos (C);
   end F;

   function F (I : Integer) return Integer is
   begin
      return -I;
   end F;

   I1 : constant Integer := F ('A'); --  BREAK
   I2 : constant Integer := F (1);

begin
   null;
end Foo;
