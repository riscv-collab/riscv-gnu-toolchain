--  Copyright 2014-2024 Free Software Foundation, Inc.
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

procedure Failure is

   type Funny_Char is
        (NUL, ' ', '"', '#', '$', TMI, '&', ''',
         '(', ')', SOT, ND,  ',', '-', '.', '/',
         '0', '1', '2', '3', '4', '5', '6', '7',
         '8', '9', ':', ';', UNS, INF, XMT, '?',
         '!', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
         'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
         'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
         'X', 'Y', 'Z', BEL, SND, CR,  LF,  DLT);

   type Funny_String is array (Positive range <>) of Funny_Char;
   pragma Pack (Funny_String);

   type Bounded_Funny_String (Size : Natural := 1) is
      record
         Str    : Funny_String (1 .. Size) := (others => '0');
         Length : Natural := 4;
      end record;

   Test : Bounded_Funny_String (100);
begin
   Test.Str := (1 => 'A', others => NUL);
   Test.Length := 1;
   Do_Nothing (Test'Address); -- START
end;

