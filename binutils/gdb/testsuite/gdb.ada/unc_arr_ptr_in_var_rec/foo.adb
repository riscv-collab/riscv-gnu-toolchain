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

with Pck; use Pck;

procedure Foo is

   type Table is array (Positive range <>) of Integer;
   type Table_Access is access Table;

   type Object (N : Integer) is record
       Ptr  : Table_Access;
       Data : Table (1 .. N);
   end record;

   My_Object : Object := (N => 3, Ptr => null, Data => (3, 5, 8));

   --  Same as above, but with a pointer to an unconstrained packed array.

   type Byte is range 0 .. 255;

   type P_Table is array (Positive range <>) of Byte;
   pragma Pack (P_Table);
   type P_Table_Access is access P_Table;

   type P_Object (N : Integer) is record
       Ptr  : P_Table_Access;
       Data : P_Table (1 .. N);
   end record;

   My_P_Object : P_Object := (N => 3, Ptr => null, Data => (3, 5, 8));

begin
   My_Object.Ptr := new Table'(13, 21, 34);    -- STOP1
   My_P_Object.Ptr := new P_Table'(13, 21, 34);
   Do_Nothing (My_Object'Address);             -- STOP2
   Do_Nothing (My_P_Object'Address);
end Foo;

