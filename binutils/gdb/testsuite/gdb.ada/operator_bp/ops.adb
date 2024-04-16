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

package body Ops is

   function Make (X: Natural) return Int is
   begin
      return Int (X);
   end Make;

   function "+" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) + IntRep (I2));
   end;

   function "-" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) - IntRep (I2));
   end;

   function "*" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) * IntRep (I2));
   end;

   function "/" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) / IntRep (I2));
   end;

   function "mod" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) mod IntRep (I2));
   end;

   function "rem" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) rem IntRep (I2));
   end;

   function "**" (I1, I2 : Int) return Int is
      Result : IntRep := 1;
   begin
      for J in 1 .. IntRep (I2) loop
         Result := IntRep (I1) * Result;
      end loop;
      return Int (Result);
   end;

   function "<" (I1, I2 : Int) return Boolean is
   begin
      return IntRep (I1) < IntRep (I2);
   end;

   function "<=" (I1, I2 : Int) return Boolean is
   begin
      return IntRep (I1) <= IntRep (I2);
   end;

   function ">" (I1, I2 : Int) return Boolean is
   begin
      return IntRep (I1) > IntRep (I2);
   end;

   function ">=" (I1, I2 : Int) return Boolean is
   begin
      return IntRep (I1) >= IntRep (I2);
   end;

   function "=" (I1, I2 : Int) return Boolean is
   begin
      return IntRep (I1) = IntRep (I2);
   end;

   function "and" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) and IntRep (I2));
   end;

   function "or" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) or IntRep (I2));
   end;

   function "xor" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) xor IntRep (I2));
   end;

   function "&" (I1, I2 : Int) return Int is
   begin
      return Int (IntRep (I1) and IntRep (I2));
   end;

   function "abs" (I1 : Int) return Int is
   begin
      return Int (abs IntRep (I1));
   end;

   function "not" (I1 : Int) return Int is
   begin
      return Int (not IntRep (I1));
   end;

   function "+" (I1 : Int) return Int is
   begin
      return Int (IntRep (I1));
   end;

   function "-" (I1 : Int) return Int is
   begin
      return Int (-IntRep (I1));
   end;

   procedure Dummy (I1 : Int) is
   begin
      null;
   end Dummy;

   procedure Dummy (B1 : Boolean) is
   begin
      null;
   end Dummy;

end Ops;



