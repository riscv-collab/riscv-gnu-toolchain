--  Copyright 2020-2024 Free Software Foundation, Inc.
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

package P is

   type Kind_Type is (No_Kind, A_Kind, B_Kind);

   type PID_Type is new Integer;
   Default_Value : constant PID_Type := 0;

   type Name_Type is array (1 ..3) of Character;
   Name_Default_Value : constant Name_Type := "AAA";

   type Variable_Record_Type(Kind : Kind_Type := No_Kind) is record
      case Kind is
         when A_Kind =>
            Variable_Record_A : PID_Type := Default_Value;

         when B_Kind =>
            Variable_Record_B : Name_Type := Name_Default_Value;

         when No_Kind =>
            null;

      end case;
   end record;

   type Complex_Variable_Record_Type (Kind : Kind_Type := No_Kind) is record
      Complex_Variable_Record_Variable_Record : Variable_Record_Type(Kind);
   end record;

   type Top_Level_Record_Type is record
      Top_Level_Record_Complex_Record : Complex_Variable_Record_Type;
   end record;

end P;
