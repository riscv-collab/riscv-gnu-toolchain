--  Copyright 2016-2024 Free Software Foundation, Inc.
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

procedure Foo_OC22_002 is
   type Small is new Integer range Ident (1) .. Ident (10);
   type Table is array (1 .. 3) of Small;

   A1 : Table := (3, 5, 8);
begin
   Do_Nothing (A1'Address);  -- STOP
end Foo_OC22_002;
