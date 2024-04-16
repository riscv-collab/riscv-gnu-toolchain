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

   type Table_Type is
     array (Natural range <>) of Integer;
   type Table_Ptr_Type is access all Table_Type;

   Table     : Table_Type := (1 => 10, 2 => 20);
   Table_Ptr : aliased Table_Ptr_Type := new Table_Type'(3 => 30, 4 => 40);

end Pack;
