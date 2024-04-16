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

package Pack is
   type Rec_Type is record
      I : Integer;
      B : Boolean;
   end record;

   type Vec_Type is array (1 .. 4) of Rec_Type;

   type Array_Type is array (Positive range <>) of Vec_Type;

   procedure Do_Nothing (A : Array_Type);
   function Identity (I : Integer) return Integer;

end Pack;
