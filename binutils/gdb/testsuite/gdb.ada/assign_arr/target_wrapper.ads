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

with System;

package target_wrapper is

   type Float_Array_3 is array (1 .. 3) of Float;

   type parameters is record
      u2 : Float_Array_3;
   end record;

   Assign_Arr_Input : parameters;

   type IArray is array (Integer range <>) of Integer;

   procedure Put (A : in out IArray);

   procedure Do_Nothing (A : System.Address);

end target_wrapper;
