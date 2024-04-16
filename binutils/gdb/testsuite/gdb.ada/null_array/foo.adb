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

procedure Foo is
   type Table is array (Integer range <>) of Integer;

   type Matrix is array (1 .. 10, 1 .. 0) of Character;
   type Wrapper is record
      M : Matrix;
   end record;

   My_Table : Table (Ident (10) .. Ident (1));
   My_Matrix : Wrapper := (M => (others => (others => 'a')));
begin
   Do_Nothing (My_Table'Address);  -- START
   Do_Nothing (My_Matrix'Address);
end Foo;
