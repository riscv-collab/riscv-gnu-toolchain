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

--  This program declares a bunch of unconstrained objects and
--  discrinimated records; the goal is to check that GDB does not crash
--  when printing them even if they are not initialized.

with Parse_Controlled;

procedure Parse is -- START

   A  : aliased Integer := 1;

   type Access_Type is access all Integer;

   type String_Access is access String;

   type My_Record is record
      Field1 : Access_Type;
      Field2 : String (1 .. 2);
   end record;

   type Discriminants_Record (A : Integer; B : Boolean) is record
      C : Float;
   end record;
   Z : Discriminants_Record := (A => 1, B => False, C => 2.0);

   type Variable_Record (A : Boolean := True) is record
      case A is
         when True =>
            B : Integer;
         when False =>
            C : Float;
            D : Integer;
      end case;
   end record;
   Y  : Variable_Record := (A => True, B => 1);
   Y2 : Variable_Record := (A => False, C => 1.0, D => 2);
   Nv : Parse_Controlled.Null_Variant;

   type Union_Type (A : Boolean := False) is record
      case A is
         when True  => B : Integer;
         when False => C : Float;
      end case;
   end record;
   pragma Unchecked_Union (Union_Type);
   Ut : Union_Type := (A => True, B => 3);

   type Tagged_Type is tagged record
      A : Integer;
      B : Character;
   end record;
   Tt : Tagged_Type := (A => 2, B => 'C');

   type Child_Tagged_Type is new Tagged_Type with record
      C : Float;
   end record;
   Ctt : Child_Tagged_Type := (Tt with C => 4.5);

   type Child_Tagged_Type2 is new Tagged_Type with null record;
   Ctt2 : Child_Tagged_Type2 := (Tt with null record);

   type My_Record_Array is array (Natural range <>) of My_Record;
   W : My_Record_Array := ((Field1 => A'Access, Field2 => "ab"),
                           (Field1 => A'Access, Field2 => "rt"));

   type Discriminant_Record (Num1, Num2,
                             Num3, Num4 : Natural) is record
      Field1 : My_Record_Array (1 .. Num2);
      Field2 : My_Record_Array (Num1 .. 10);
      Field3 : My_Record_Array (Num1 .. Num2);
      Field4 : My_Record_Array (Num3 .. Num2);
      Field5 : My_Record_Array (Num4 .. Num2);
   end record;
   Dire : Discriminant_Record (1, 7, 3, 0);

   type Null_Variant_Part (Discr : Integer) is record
      case Discr is
         when 1 => Var_1 : Integer;
         when 2 => Var_2 : Boolean;
         when others => null;
      end case;
   end record;
   Nvp : Null_Variant_Part (3);

   type T_Type is array (Positive range <>) of Integer;
   type T_Ptr_Type is access T_Type;

   T_Ptr : T_Ptr_Type := new T_Type' (13, 17);
   T_Ptr2 : T_Ptr_Type := new T_Type' (2 => 13, 3 => 17);

   function Foos return String is
   begin
      return "string";
   end Foos;

   My_Str : String := Foos;

   type Value_Var_Type is ( V_Null, V_Boolean, V_Integer );
   type Value_Type( Var : Value_Var_Type := V_Null ) is
      record
         case Var is
            when V_Null =>
               null;
            when V_Boolean =>
               Boolean_Value : Boolean;
            when V_Integer =>
               Integer_Value : Integer;
         end case;
      end record;
   NBI_N : Value_Type := (Var => V_Null);
   NBI_I : Value_Type := (Var => V_Integer, Integer_Value => 18);
   NBI_B : Value_Type := (Var => V_Boolean, Boolean_Value => True);

begin
   null;
end Parse;
