--  Copyright 2008-2024 Free Software Foundation, Inc.
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

with Pck; use Pck;

package body Homonym is

   type Integer_Range is new Integer range -100 .. 100;
   type Positive_Range is new Positive range 1 .. 19740804;

   ---------------
   -- Get_Value --
   ---------------

   function Get_Value return Integer_Range
   is
      subtype Local_Type is Integer_Range;
      subtype Local_Type_Subtype is Local_Type;
      subtype Int_Type   is Integer_Range;
      Lcl : Local_Type := 29;
      Some_Local_Type_Subtype : Local_Type_Subtype := Lcl;
      I : Int_Type := 1;
   begin
      Do_Nothing (Some_Local_Type_Subtype'Address);
      Do_Nothing (I'Address);
      return Lcl;  --  BREAK_1
   end Get_Value;

   ---------------
   -- Get_Value --
   ---------------

   function Get_Value return Positive_Range
   is
      subtype Local_Type is Positive_Range;
      subtype Local_Type_Subtype is Local_Type;
      subtype Pos_Type is Positive_Range;
      Lcl : Local_Type := 17;
      Some_Local_Type_Subtype : Local_Type_Subtype := Lcl;
      P : Pos_Type := 2;
   begin
      Do_Nothing (Some_Local_Type_Subtype'Address);
      Do_Nothing (P'Address);
      return Lcl;  --  BREAK_2
   end Get_Value;

   ----------------
   -- Start_Test --
   ----------------

   procedure Start_Test is
      Int : Integer_Range;
      Pos : Positive_Range;
   begin
      Int := Get_Value;
      Pos := Get_Value;
   end Start_Test;

end Homonym;
