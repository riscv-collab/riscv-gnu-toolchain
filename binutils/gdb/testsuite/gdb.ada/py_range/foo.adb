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

with Pck; use Pck;

procedure Foo is
   type Small is range -128 .. 127;
   SR : Small := 48;

   type Small_Integer is range -2 ** 31 .. 2 ** 31 - 1;
   SI : Small_Integer := 740804;

   type Integer4_T is range -2 ** 31 .. 2 ** 31 - 1;
   for Integer4_T'Size use 32;
   IR : Integer4_T := 974;
begin
   Do_Nothing (SR'Address);  -- STOP
   Do_Nothing (SI'Address);
   Do_Nothing (IR'Address);
end Foo;
