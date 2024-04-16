-- Copyright 2022-2024 Free Software Foundation, Inc.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

package Pack is

   type Index_Type_1 is new Natural;
   type Index_Type_2 is new Natural;
   type Index_Type_3 is new Natural;

   type CA_Simple_Type is array (Index_Type_1 range 1 .. 2) of Integer;

   --  Array types we test

   type Simple_Type is
     array (Index_Type_1 range <>, Index_Type_2 range <>)
     of Integer;

   type Nested_Type is
     array (Index_Type_1 range <>, Index_Type_2 range <>)
     of CA_Simple_Type;

end Pack;
