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

package Ops is
   type Int is private;

   function Make (X: Natural) return Int;

   function "+" (I1, I2 : Int) return Int;
   function "-" (I1, I2 : Int) return Int;
   function "*" (I1, I2 : Int) return Int;
   function "/" (I1, I2 : Int) return Int;
   function "mod" (I1, I2 : Int) return Int;
   function "rem" (I1, I2 : Int) return Int;
   function "**" (I1, I2 : Int) return Int;
   function "<" (I1, I2 : Int) return Boolean;
   function "<=" (I1, I2 : Int) return Boolean;
   function ">" (I1, I2 : Int) return Boolean;
   function ">=" (I1, I2 : Int) return Boolean;
   function "=" (I1, I2 : Int) return Boolean;
   function "and" (I1, I2 : Int) return Int;
   function "or" (I1, I2 : Int) return Int;
   function "xor" (I1, I2 : Int) return Int;
   function "&" (I1, I2 : Int) return Int;
   function "abs" (I1 : Int) return Int;
   function "not" (I1 : Int) return Int;
   function "+" (I1 : Int) return Int;
   function "-" (I1 : Int) return Int;

   procedure Dummy (B1 : Boolean);
   procedure Dummy (I1 : Int);

private

   type IntRep is mod 2**31;
   type Int is new IntRep;

end Ops;


