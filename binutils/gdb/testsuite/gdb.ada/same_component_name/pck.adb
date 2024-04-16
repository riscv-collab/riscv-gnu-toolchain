--  Copyright 2010-2024 Free Software Foundation, Inc.
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

with System;

package body Pck is
   package body Top is
      procedure Assign (Obj: in out Top_T; TV : Integer) is
      begin
         Do_Nothing (Obj'Address); -- BREAK_TOP
      end Assign;
   end Top;

   package body Middle is
      procedure Assign (Obj: in out Middle_T; MV : Character) is
      begin
         Do_Nothing (Obj'Address); -- BREAK_MIDDLE
      end Assign;
   end Middle;

   procedure Assign (Obj: in out Bottom_T; BV : Float) is
   begin
      Do_Nothing (Obj'Address); -- BREAK_BOTTOM
   end Assign;

   procedure Do_Nothing (A : System.Address) is
   begin
      null;
   end Do_Nothing;

   package body Dyn_Top is
      procedure Assign (Obj: in out Dyn_Top_T; TV : Integer) is
      begin
         Do_Nothing (Obj'Address); -- BREAK_DYN_TOP
      end Assign;
   end Dyn_Top;

   package body Dyn_Middle is
      procedure Assign (Obj: in out Dyn_Middle_T; MV : Character) is
      begin
         Do_Nothing (Obj'Address); -- BREAK_DYN_MIDDLE
      end Assign;
   end Dyn_Middle;

end Pck;
