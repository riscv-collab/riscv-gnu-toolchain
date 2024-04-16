--  Copyright 2022-2024 Free Software Foundation, Inc. -*- coding: iso-latin-1 -*-
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
   -- This should be var_Ufe.
   VAR_Þ : Integer := FUNC_Þ (23);
begin
   Do_Nothing (var_þ'Address); --  BREAK
end Prog;
