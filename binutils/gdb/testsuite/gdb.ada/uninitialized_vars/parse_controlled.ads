--  Copyright 2009-2024 Free Software Foundation, Inc.
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

with Ada.Finalization;

package Parse_Controlled is

   type Variant_Kind is (VK_Null, VK_Num, VK_String);
   type Null_Variant_Record (Kind : Variant_Kind := VK_Null) is record
      case Kind is
         when VK_Null =>
            null;
         when VK_Num =>
            Num_Value : Long_Float;
         when VK_String =>
            String_Value : Natural;
      end case;
   end record;
   type Null_Variant is new Ada.Finalization.Controlled with record
      V : Null_Variant_Record;
   end record;

end Parse_Controlled;
