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

with Aux_Pck; use Aux_Pck;
with Pck;     use Pck;

procedure Foo is
   Some_Local_Variable : Integer := 1;
   External_Identical_Two : Integer := 74;
begin
   My_Global_Variable := Some_Local_Variable + 1; -- START
   Proc (External_Identical_Two);
   Aux_Pck.Ambiguous_Func;
   Aux_Pck.Ambiguous_Proc;
   Pck.Ambiguous_Func;
end Foo;

