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

package Pck is
   type FP1_Type is delta 0.1 range -1.0 .. +1.0;
   FP1_Var : FP1_Type := 0.25;

   type FP2_Type is delta 0.01 digits 14;
   FP2_Var : FP2_Type := -0.01;

   type FP3_Type is delta 0.1 range 0.0 .. 1.0 with Small => 0.1/3.0;
   FP3_Var : FP3_Type := 0.1;

   Delta4 : constant := 0.000_000_1;
   type FP4_Type is delta Delta4 range 0.0 .. Delta4 * 10
      with Small => Delta4 / 3.0;
   FP4_Var : FP4_Type := 2 * Delta4;

   Delta5 : constant := 0.000_000_000_000_000_000_1;
   type FP5_Type is delta Delta5 range 0.0 .. Delta5 * 10
      with Small => Delta5 / 3.0;

   procedure Do_Nothing (A : System.Address);
end pck;

