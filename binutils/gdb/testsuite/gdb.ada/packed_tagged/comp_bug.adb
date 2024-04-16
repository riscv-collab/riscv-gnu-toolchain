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

procedure Comp_Bug is

   type Number_T (Exists : Boolean := False) is
      record
         case Exists is
            when True =>
               Value : Natural range 0 .. 255;
            when False =>
               null;
         end case;
      end record;
   pragma Pack (Number_T);

   X : Number_T;
   --  brobecker/2007-09-06: At the time when this issue (G904-017) was
   --  reported, the problem only reproduced if the variable was declared
   --  inside a function (in other words, stored on stack).  Although
   --  the issue probably still existed when I tried moving this variable
   --  to a package spec, the symptoms inside GDB disappeared.
begin
   X := (Exists => True, Value => 10);
   if X.Exists then -- STOP
      X.Value := X.Value + 1;
   end if;
end Comp_Bug;
