--  Copyright 2019-2024 Free Software Foundation, Inc.
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
with System.Storage_Elements; use System.Storage_Elements;

procedure Storage is
   subtype Some_Range is Natural range 0..127;
   subtype Another_Range is Natural range 0..15;

   type Rec is record
      Value : Some_Range;
      Another_Value : Another_Range;
   end record;
   
   for Rec use record
      Value at 0 range 0..6;
      Another_Value at 0 range 7..10;
   end record;

   type Rec_LE is new Rec;
   for Rec_LE'Bit_Order use System.Low_Order_First;
   for Rec_LE'Scalar_Storage_Order use System.Low_Order_First;

   type Rec_BE is new Rec;
   for Rec_BE'Bit_Order use System.High_Order_First;
   for Rec_BE'Scalar_Storage_Order use System.High_Order_First;

   V_LE : Rec_LE;
   V_BE : Rec_BE;

begin
   V_LE := (126, 12);
   V_BE := (126, 12);

   Do_Nothing (V_LE'Address);  --  START
   Do_Nothing (V_BE'Address);
end Storage;
