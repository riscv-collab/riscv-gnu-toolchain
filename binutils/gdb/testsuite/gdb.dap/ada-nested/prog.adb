--  Copyright 2023-2024 Free Software Foundation, Inc.
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
   X  : Integer := 12;

   procedure Outer (Outer_Arg : Integer) is
      procedure Bump (Stride : Integer) is
      begin
         X := X + Stride;          -- STOP
      end;
   begin
      Bump (Outer_Arg);
   end;

begin
   for I in 1 .. 20 loop
      Outer (1);
   end loop;
end Foo;
