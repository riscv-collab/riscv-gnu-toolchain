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

procedure Foo is

   procedure Nested (L, U : Integer) is
      subtype Small_Type is Integer range L .. U;
      type Record_Type (I : Small_Type := L) is record
         S : String (1 .. I);
      end record;
      type Array_Type is array (Integer range <>) of Record_Type;

      A1 : Array_Type :=
        (1 => (I => 0, S => <>),
         2 => (I => 1, S => "A"),
         3 => (I => 2, S => "AB"));

      procedure Discard (R : Record_Type) is
      begin
         null;
      end Discard;

   begin
      Discard (A1 (1));  -- STOP
   end;

begin
   Nested (0, 10);
   Nested (-10, 10);
end Foo;
