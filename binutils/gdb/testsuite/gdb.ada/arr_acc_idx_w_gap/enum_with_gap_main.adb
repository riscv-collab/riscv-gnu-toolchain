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

with Enum_With_Gap; use Enum_With_Gap;

procedure Enum_With_Gap_Main is
   Indexed_By_Enum : AR_Access :=
     new AR'(LIT1 => 1,  LIT2 => 43, LIT3 => 42, LIT4 => 41);
   S : String_Access := new String'("Hello!");
   V : Enum_Subrange := LIT3;
begin
   Do_Nothing (Indexed_By_Enum); --  BREAK
   Do_Nothing (S);
end Enum_With_Gap_Main;
