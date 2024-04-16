--  Copyright 2010-2024 Free Software Foundation, Inc.
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

with Ada.Text_IO; use Ada.Text_IO;

package body Pack is

   procedure Print (I1 : Positive; I2 : Positive) is
      type My_String is array (I1 .. I2) of Character;
      I : My_String := (others => 'A');
      S : String (1 .. I2 + 3) := (others => ' ');
   begin
      S (I1 .. I2) := String (I); --  BREAK
      Put_Line (S);
   end Print;

end Pack;
