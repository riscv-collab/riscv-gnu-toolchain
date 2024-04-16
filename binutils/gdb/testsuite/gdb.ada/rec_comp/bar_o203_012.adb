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

procedure Bar_O203_012 is
   type Int_Access is access Integer;
   type Record_Type is record
      IA : Int_Access;
   end record;

   R : Record_Type;
   IA : Int_Access;
begin
   R.IA := new Integer'(3); -- STOP
   IA := R.IA;
   Do_Nothing (R'Address);
   Do_Nothing (IA'Address);
end Bar_O203_012;

