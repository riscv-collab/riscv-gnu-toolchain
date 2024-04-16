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

with Array_List_G;

package Reprod is

   package Objects is
      type T is record
         I : Integer;
      end record;
      package List is new Array_List_G (Index_Base_T => Natural,
                                        Component_T => T);
   end Objects;

   type Obj_T (Len : Natural) is record
      Data : Objects.List.T (1 .. Len);
   end record;

   type T is access Obj_T;

   procedure Do_Nothing (Broken : Obj_T);
end Reprod;
