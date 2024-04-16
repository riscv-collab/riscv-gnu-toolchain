--  Copyright 2004-2024 Free Software Foundation, Inc.
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
with Pck; use Pck;

procedure Fixed_Points is

   ------------
   -- Test 1 --
   ------------

   --  Fixed point subtypes

   type Base_Fixed_Point_Type is
     delta 1.0 / 16.0
     range -2147483648 * 1.0 / 16.0 ..  2147483647 * 1.0 / 16.0;

   subtype Fixed_Point_Subtype is
     Base_Fixed_Point_Type range -50.0 .. 50.0;

   type New_Fixed_Point_Type is
     new Base_Fixed_Point_Type range -50.0 .. 50.0;

   Base_Object            : Base_Fixed_Point_Type := -50.0;
   Subtype_Object         : Fixed_Point_Subtype := -50.0;
   New_Type_Object        : New_Fixed_Point_Type := -50.0;


   ------------
   -- Test 2 --
   ------------

   --  Overprecise delta

   Overprecise_Delta : constant := 0.135791357913579;
   --  delta whose significant figures cannot be stored into a long.

   type Overprecise_Fixed_Point is
     delta Overprecise_Delta range 0.0 .. 200.0;
   for Overprecise_Fixed_Point'Small use Overprecise_Delta;

   Overprecise_Object : Overprecise_Fixed_Point :=
     Overprecise_Fixed_Point'Small;

   FP5_Var : FP5_Type := 3 * Delta5;


   Another_Delta : constant := 1.0/(2**63);
   type Another_Type is delta Another_Delta range -1.0 .. (1.0 - Another_Delta);
   for  Another_Type'small use Another_Delta;
   for  Another_Type'size  use 64;
   Another_Fixed : Another_Type := Another_Delta * 5;

begin
   Base_Object := 1.0/16.0;   -- Set breakpoint here
   Subtype_Object := 1.0/16.0;
   New_Type_Object := 1.0/16.0;
   Overprecise_Object := Overprecise_Fixed_Point'Small * 2;
   Do_Nothing (FP1_Var'Address);
   Do_Nothing (FP2_Var'Address);
   Do_Nothing (FP3_Var'Address);
   Do_Nothing (FP4_Var'Address);
   Do_Nothing (FP5_Var'Address);
   Do_Nothing (Another_Fixed'Address);
end Fixed_Points;
