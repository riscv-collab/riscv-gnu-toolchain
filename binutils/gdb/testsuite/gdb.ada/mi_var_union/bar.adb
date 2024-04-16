--  Copyright 2018-2024 Free Software Foundation, Inc.
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
   type Union_Type (A : Boolean := False) is record
      case A is
         when True  => B : Integer;
         when False => C : Float;
      end case;
   end record;
   pragma Unchecked_Union (Union_Type);
   Ut : Union_Type := (A => True, B => 3);
begin
   Do_Nothing (Ut'Address);  -- STOP
end Bar;
