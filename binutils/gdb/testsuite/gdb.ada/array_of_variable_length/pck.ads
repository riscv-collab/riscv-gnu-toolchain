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

package Pck is

   subtype Small_Int is Natural range 0 .. 100;

   type R_Type (L : Small_Int := 0) is record
      S : String (1 .. L);
   end record;

   type A_Type is array (Natural range <>) of R_Type;

   A : A_Type :=
     (1 => (L => 0, S => ""),
      2 => (L => 2, S => "ab"));

   procedure Do_Nothing (A : A_Type);

end Pck;
