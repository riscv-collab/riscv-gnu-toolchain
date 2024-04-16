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

with System;
package Pck is

   type Table is array (Positive range <>) of Integer;

   type Object (N : Integer) is record
       Data : Table (1 .. N);
   end record;

   type Small is new Integer range 0 .. 255;
   for Small'Size use 8;

   type Small_Table is array (Positive range <>) of Small;
   pragma Pack (Small_Table);

   type Small_Object (N : Integer) is record
       Data : Table (1 .. N);
   end record;

   procedure Do_Nothing (A : System.Address);
end Pck;
