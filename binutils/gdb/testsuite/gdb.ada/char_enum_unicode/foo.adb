--  Copyright 2011-2024 Free Software Foundation, Inc.
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

with Pck; use Pck;

procedure Foo is
   type Char_Enum_Type is (alpha, 'x', 'Ÿ', '🨀', 'Þ');
   Char_Alpha : Char_Enum_Type := alpha;
   Char_X : Char_Enum_Type := 'x';
   Char_Thorn : Char_Enum_Type := 'Þ';
   Char_Y : Char_Enum_Type := 'Ÿ';
   Char_King : Char_Enum_Type := '🨀';
begin
   Do_Nothing (Char_Alpha'Address);  -- STOP
   Do_Nothing (Char_X'Address);
   Do_Nothing (Char_Y'Address);
   Do_Nothing (Char_King'Address);
end Foo;
