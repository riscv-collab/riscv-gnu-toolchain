--  Copyright 2023-2024 Free Software Foundation, Inc.
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

procedure Overloads is

   type Packed_Array is array (4 .. 7) of Boolean;
   pragma pack (Packed_Array);

   type Char_Array is array (1 .. 4) of Character;

   function Oload (P : Packed_Array) return Integer is
   begin
      return 23;
   end Oload;

   function Oload (C : Char_Array) return Integer is
   begin
      return 91;
   end Oload;

   PA : Packed_Array := (True, False, True, False);
   CA : Char_Array := ('A', 'B', 'C', 'D');

   B1 : constant Integer := Oload (PA);
   B2 : constant Integer := Oload (CA);

begin
   null; -- START
end Overloads;
