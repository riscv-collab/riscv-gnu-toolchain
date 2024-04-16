--  Copyright 2008-2024 Free Software Foundation, Inc.
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

with Interfaces;
with Pck; use Pck;

procedure P is
   subtype Double is Interfaces.IEEE_Float_64;
   D1 : Double := 123.0;
   D2 : Double;
   pragma Import (Ada, D2);
   for D2'Address use D1'Address;
begin
   Do_Nothing (D1'Address);  --  START
   Do_Nothing (D2'Address);
end P;

