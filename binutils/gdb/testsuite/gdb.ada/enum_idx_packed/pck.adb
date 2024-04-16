--  Copyright 2012-2024 Free Software Foundation, Inc.
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

package body Pck is

   function New_Small_Table (Low: Color; High: Color) return Small_Table is
      Result : Small_Table (Low .. High);
   begin
      for J in Low .. High loop
         Result (J) := (J = Black or J = Green or J = White);
      end loop;
      return Result;
   end New_Small_Table;

   function New_Multi_Table (Low, High: Color; LS, HS: Strength)
     return Multi_Table is
      Result : Multi_Table (Low .. High, LS .. HS);
      Next : Boolean := True;
   begin
      for J in Low .. High loop
         for K in LS .. HS loop
            Result (J, K) := Next;
            Next := not Next;
         end loop;
      end loop;
      return Result;
   end New_Multi_Table;

   function New_Multi_Multi_Table (L1, H1, L2, H2, L3, H3: Positive)
     return Multi_Multi_Table is
      Result : Multi_Multi_Table (L1 .. H1, L2 .. H2, L3 .. H3);
      Next : Boolean := True;
   begin
      for J in L1 .. H1 loop
         for K in L2 .. H2 loop
	    for L in L3 .. H3 loop
	       Result (J, K, L) := Next;
               Next := not Next;
            end loop;
         end loop;
      end loop;
      return Result;
   end New_Multi_Multi_Table;

   procedure Do_Nothing (A : System.Address) is
   begin
      null;
   end Do_Nothing;
end Pck;
