--  Copyright 2013-2024 Free Software Foundation, Inc.
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
   Global_Float : Float := 0.0;
   Global_Double : Long_Float := 0.0;
   Global_Long_Double : Long_Long_Float := 0.0;

   type Small_Struct is record
      I : Integer;
   end record;
   Global_Small_Struct : Small_Struct := (I => 0);

   procedure Set_Float (F : Float);
   procedure Set_Double (Dummy : Integer; D : Long_Float);
   procedure Set_Long_Double (Dummy : Integer;
                              DS: Small_Struct;
                              LD : Long_Long_Float);
end Pck;
