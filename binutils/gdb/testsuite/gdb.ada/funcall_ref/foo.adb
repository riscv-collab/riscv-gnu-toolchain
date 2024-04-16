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

procedure Foo is
   type Bar (N : Natural) is record
      S : String (1 .. N);
   end record;

   function Get (S : String) return Bar is
   begin
      return (N => S'Length, S => S);
   end Get;

   procedure Do_Nothing (B : Bar) is
   begin
      null;
   end Do_Nothing;

   B : Bar := Get ("Foo");
begin
   Do_Nothing (B); --  STOP
end Foo;
