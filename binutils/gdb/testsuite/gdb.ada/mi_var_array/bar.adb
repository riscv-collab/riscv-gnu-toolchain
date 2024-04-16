--  Copyright 2015-2024 Free Software Foundation, Inc.
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

procedure Bar is
   M : Integer := 35;
   subtype Int is Integer range 0 .. M;
   type Variant_Type (N : Int := 0) is record
      F : String(1 .. N) := (others => 'x');
   end record;
   type Variant_Type_Access is access all Variant_Type;

   VTA : Variant_Type_Access;
begin
   Do_Nothing (VTA'Address);  --  STOP
end Bar;
