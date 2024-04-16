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

with System;

package Pck is

   subtype Small_Type is Integer range 0 .. 10;
   type Record_Type (I : Small_Type := 0) is record
      S : String (1 .. I);
   end record;
   function Ident (R : Record_Type) return Record_Type;

   type Array_Type is array (Integer range <>) of Record_Type;

   procedure Do_Nothing (A : System.Address);

   A1 : Array_Type := (1 => (I => 0, S => <>),
                       2 => (I => 1, S => "A"),
                       3 => (I => 2, S => "AB"));

   A2 : Array_Type := (1 => (I => 2, S => "AB"),
                       2 => (I => 1, S => "A"),
                       3 => (I => 0, S => <>));

end Pck;
