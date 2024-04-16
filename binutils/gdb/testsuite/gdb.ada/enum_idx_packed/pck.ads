--  Copyright 2012-2024 Free Software Foundation, Inc.
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
   type Color is (Black, Red, Green, Blue, White);
   type Strength is (None, Low, Medium, High);
   type Short is new Natural range 0 .. 2 ** 8 - 1;

   type Full_Table is array (Color) of Boolean;
   pragma Pack (Full_Table);

   subtype Primary_Color is Color range Red .. Blue;
   type Primary_Table is array (Primary_Color) of Boolean;
   pragma Pack (Primary_Table);

   type Cold_Color is new Color range Green .. Blue;
   type Cold_Table is array (Cold_Color) of Boolean;
   pragma Pack (Cold_Table);

   type Small_Table is array (Color range <>) of Boolean;
   pragma Pack (Small_Table);
   function New_Small_Table (Low: Color; High: Color) return Small_Table;

   type Multi_Table is array (Color range <>, Strength range <>) of Boolean;
   pragma Pack (Multi_Table);
   function New_Multi_Table (Low, High: Color; LS, HS: Strength)
      return Multi_Table;

   type Multi_Multi_Table is array (Positive range <>, Positive range <>, Positive range <>) of Boolean;
   pragma Pack (Multi_Multi_Table);
   function New_Multi_Multi_Table (L1, H1, L2, H2, L3, H3: Positive)
      return Multi_Multi_Table;

   type Multi_Dimension is array (Boolean, Color) of Short;
   pragma Pack (Multi_Dimension);
   type Multi_Dimension_Access is access all Multi_Dimension;

   type My_Enum is (Blue, Red, Green);

   type My_Array_Type is array (My_Enum) of Integer;
   type Confused_Array_Type is array (Color) of My_Array_Type;

   procedure Do_Nothing (A : System.Address);
end Pck;
