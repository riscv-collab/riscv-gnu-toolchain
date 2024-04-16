--  Copyright 2009-2024 Free Software Foundation, Inc.
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

   type Small is range -32 .. 31;

   type SomePackedArray is array (Integer range <>) of Small;
   pragma Pack (SomePackedArray);

   type SomePackedRecord is record
      Y: SomePackedArray (1 .. 10);
   end record;
   pragma Pack (SomePackedRecord);

   Suite : SomePackedArray := (-1, -2, -3, -4, -5, -6, -7, -8, -9, -10);
   XP: SomePackedRecord := (Y => Suite);

   Slice : SomePackedArray renames XP.Y (3 .. 5);

begin
   Slice (4) := 4;  --  START
end Foo;

