--  Copyright 2019-2024 Free Software Foundation, Inc.
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

with Pck;

procedure Length_Cond is
   type Enum is (One, Two, Three);
   Enum_Val : Enum := Three;

   type My_Int is range -23 .. 23;
   Int_Val : My_Int := 0;

   type My_Array is array (0..1, 0..1) of Boolean;
   Array_Val : My_Array := ((True, False), (False, True));

   procedure p (s : String) is
      loc : String := s & ".";
   begin
      Pck.Do_Nothing (loc);		--  BREAKPOINT
   end p;
begin
   for I in 1 .. 25 loop
      p ((1 .. I => 'X'));
   end loop;
end Length_Cond;
