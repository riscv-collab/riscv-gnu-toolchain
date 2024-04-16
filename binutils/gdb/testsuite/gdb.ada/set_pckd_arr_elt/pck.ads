--  Copyright 2012-2024 Free Software Foundation, Inc.
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

package Pck is
   type Small is new Integer range 0 .. 2 ** 6 - 1;
   type Simple_Array is array (1 .. 4) of Small;
   pragma Pack (Simple_Array);

   type Buffer is array (Integer range <>) of Small;
   pragma Pack (Buffer);

   type Variant (Size : Integer := 1) is
   record
      A : Small;
      T : Buffer (1 .. Size);
   end record;
   pragma Pack (Variant);

   type Variant_Access is access all Variant;

   function New_Variant (Size : Integer) return Variant_Access;

   procedure Update_Small (S : in out Small);
end Pck;
