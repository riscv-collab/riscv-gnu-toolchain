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

package body Pck is

   procedure Set_Float (F : Float) is
   begin
      Global_Float := F;
   end Set_Float;

   procedure Set_Double (Dummy : Integer; D : Long_Float) is
   begin
      Global_Double := D;
   end Set_Double;

   procedure Set_Long_Double (Dummy : Integer;
                              DS : Small_Struct;
                              LD : Long_Long_Float) is
   begin
      Global_Long_Double := LD;
   end Set_Long_Double;

end Pck;
