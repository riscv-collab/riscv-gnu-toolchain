-- Copyright 2022-2024 Free Software Foundation, Inc.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 3 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

with Pack; use Pack;

procedure P is

   procedure Nop is
   begin
      null;
   end Nop;

   procedure Discard
     (Arg_Simple  : Simple_Type;
      Arg_Nested  : Nested_Type) is
   begin
      null;
   end Discard;

   Simple : Simple_Type :=
      (1 => (5 => 1, 6 => 2),
       2 => (5 => 3, 6 => 4),
       3 => (5 => 5, 6 => 6));
   Nested : Nested_Type :=
      (1 => (5 => (1, 2),  6 => (3, 4)),
       2 => (5 => (5, 6),  6 => (7, 8)),
       3 => (5 => (9, 10), 6 => (11, 12)));

begin
   Nop; --  START

   Discard (Simple, Nested);

end P;
