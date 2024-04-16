--  Copyright 2006-2024 Free Software Foundation, Inc.
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

procedure Watch is

   procedure Foo (X : in out Integer) is
   begin
      --  Reference X in a way that does not change its value.
      Do_Nothing (X'Address);  -- BREAK1
   end Foo;

   X : Integer := 1;

begin
   Foo (X);
   X := 2;  -- BREAK2
end Watch;

