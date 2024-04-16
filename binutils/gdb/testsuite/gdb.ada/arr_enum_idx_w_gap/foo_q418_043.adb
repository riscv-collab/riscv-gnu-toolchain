--  Copyright 2018-2024 Free Software Foundation, Inc.
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

procedure Foo_Q418_043 is
   type Index is (Index1, Index2);
   Size : constant Integer := 10;
   for Index use (Index1 => 1, Index2 => Size);
   type Array_Index_Enum is array (Index) of Integer;
   A : Array_Index_Enum :=(others => 42);
begin
   A(Index2) := 4242; --BREAK
end Foo_Q418_043;
