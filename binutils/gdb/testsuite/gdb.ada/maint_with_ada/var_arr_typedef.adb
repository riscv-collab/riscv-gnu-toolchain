--  Copyright 2015-2024 Free Software Foundation, Inc.
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

procedure Var_Arr_Typedef is
   RA : constant Rec_Type := (3, False);
   RB : constant Rec_Type := (2, True);

   VA : constant Vec_Type := (RA, RA, RB, RB);
   VB : constant Vec_Type := (RB, RB, RA, RA);

   A : constant Array_Type (1 .. Identity (4)) := (VA, VA, VB, VB);
begin
   Do_Nothing (A); --  BREAK
end Var_Arr_Typedef;
