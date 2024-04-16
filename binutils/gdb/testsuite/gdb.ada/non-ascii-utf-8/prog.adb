--  Copyright 2022-2024 Free Software Foundation, Inc.
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

with Pack; use Pack;

procedure Prog is
   -- This should be var_Ufc.
   VAR_Ü : Integer := FUNC_Ü (23);
   -- This should be var_W01b8, because with UTF-8, non-ASCII
   -- letters are upper-cased.
   VAR_Ƹ : Integer := FUNC_Ƹ (24);
   -- This should be var_WW00010401, because with UTF-8, non-ASCII
   -- letters are upper-cased.
   VAR_𐐁 : Integer := FUNC_𐐁 (25);
   -- This is the same name as the corresponding Latin 3 test,
   -- and helps show the peculiarity of the case folding rule.
   -- This winds up as var_W017b, the upper-case variant.
   VAR_Ż : Integer := FUNC_Ż (26);
begin
   Do_Nothing (var_ü'Address); --  BREAK
   Do_Nothing (var_ƹ'Address);
   Do_Nothing (var_𐐩'Address);
   Do_Nothing (var_ż'Address);
end Prog;
