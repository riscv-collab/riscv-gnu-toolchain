--  Copyright 2019-2024 Free Software Foundation, Inc.
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

procedure Bias is
   type Small is range -7 .. -4;
   for Small'Size use 2;
   Y : Small := -5;
   Y1 : Small := -7;

   type Repeat_Count_T is range 1 .. 2 ** 6;
   for Repeat_Count_T'Size use 6;
   X : Repeat_Count_T := 60;
   X1 : Repeat_Count_T := 1;

   type Char_Range is range 65 .. 68;
   for Char_Range'Size use 2;
   Cval : Char_Range := 65;

   type Some_Packed_Record is record
      R: Small;
      S: Small;
   end record;
   pragma Pack (Some_Packed_Record);
   SPR : Some_Packed_Record := (R => -4, S => -5);

   type Packed_Array is array (1 .. 3) of Small;
   pragma pack (Packed_Array);
   A : Packed_Array := (-7, -5, -4);

begin
   Do_Nothing (Y'Address);		--  STOP
   Do_Nothing (Y1'Address);
   Do_Nothing (X'Address);
   Do_Nothing (X1'Address);
   Do_Nothing (Cval'Address);
   Do_Nothing (SPR'Address);
   Do_Nothing (A'Address);
end Bias;
