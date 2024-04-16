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

with Pck; use Pck;

procedure Bla is
begin
   Global_Var := 1;
   declare
   begin
      pragma Assert (Global_Var /= 1, "Error #1");
   exception
      when others =>
         null;
   end;

   Global_Var := 2;
   declare
   begin
      pragma Assert (Global_Var = 1, "Error #2"); -- STOP
   exception
      when others =>
         null;
   end;

   Global_Var := 3;
   declare
   begin
      pragma Assert (Global_Var = 2, "Error #3");
   exception
      when others =>
         null;
   end;
end Bla;
