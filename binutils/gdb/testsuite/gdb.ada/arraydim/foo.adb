--  Copyright 2013-2024 Free Software Foundation, Inc.
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
   type Multi is array (1 .. 1, 2 .. 3, 4 .. 6) of Integer;
   M : Multi := (others => (others => (others => 0)));

   --  Use a fake type for importing our C multi-dimensional array.
   --  It's only to make sure the C unit gets linked in, regardless
   --  of possible optimizations.
   type Void_Star is access integer;
   E : Void_Star;
   pragma Import (C, E, "global_3dim_for_gdb_testing");
begin
   Do_Nothing (M'Address);  -- STOP
   Do_Nothing (E'Address);
end Foo;
