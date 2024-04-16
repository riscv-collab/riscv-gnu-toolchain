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

with System;

package Pck is

   type Float_Array is array (Integer range <>) of Integer;
   type Float_Ptr   is access Float_Array;

   type Table_Type is (One, Two, Three, Four, Five);
   type New_Table_Array is array (Table_Type) of Float_Ptr;

   My_Table : New_Table_Array := (others => new Float_Array'((4 => 16#DE#,
                                                              5 => 16#AD#)));

   My_F : Float_Ptr := new Float_Array'(4 => 16#BE#,
                                        5 => 16#EF#);

   procedure Do_Nothing (A : System.Address);

end Pck;

