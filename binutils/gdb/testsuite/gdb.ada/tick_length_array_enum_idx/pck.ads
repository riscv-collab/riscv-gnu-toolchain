--  Copyright 2014-2024 Free Software Foundation, Inc.
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

   type Full_Table is array (Color) of Boolean;
   type Primary_Table is array (Color range Red .. Blue) of Boolean;
   type Variable_Table is array (Color range <>) of Boolean;

   type Full_PT is array (Color) of Boolean;
   pragma Pack (Full_PT);

   function New_Variable_Table (Low: Color; High: Color) return Variable_Table;

   procedure Do_Nothing (A : System.Address);
end Pck;

