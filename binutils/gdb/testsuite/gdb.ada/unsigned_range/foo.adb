--  Copyright 2020-2024 Free Software Foundation, Inc.
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

with Pack; use Pack;

procedure Foo is
   type U_W is range 0 .. 65535;
   for  U_W'Size use 16;

   type R_C is
   record
      One : U_W;
      Two : U_W;
   end record;

   for R_C use
   record at mod 2;
      One at 0 range  0 .. 15;
      Two at 2 range  0 .. 15;
   end record;
   for R_C'size use 2*16;

   Value: R_C := (One => 8000, Two => 51000);

begin
   Do_Nothing (Value'Address); --  BREAK
end Foo;
