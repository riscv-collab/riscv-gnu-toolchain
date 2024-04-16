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
with Pck; use Pck;

procedure Foo is
   type Octal is new Integer range 0 .. 7;
   type Octal_Array is array (Positive range <>) of Octal;
   pragma Pack (Octal_Array);

   type Octal_Buffer (Size : Positive) is record
      Buffer : Octal_Array (1 .. Size);
      Length : Integer;
   end record;

   My_Buffer : Octal_Buffer (Size => 8);
begin
   My_Buffer.Buffer := (1, 2, 3, 4, 5, 6, 7, 0);
   My_Buffer.Length := My_Buffer.Size;
   Do_Nothing (My_Buffer'Address);  -- START
end Foo;
