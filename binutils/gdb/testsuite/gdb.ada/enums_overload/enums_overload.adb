--  Copyright 2021-2024 Free Software Foundation, Inc.
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

package body Enums_Overload is

   subtype Reddish is Color range Red .. Yellow;

   procedure Test_Enums_Overload is
      X: Reddish := Orange;
      Y: Traffic_Signal := Yellow;
   begin
      --gdb: next
      X := Orange;
      --gdb: next
      Y := Yellow;
      --gdb: ptype x range red .. yellow
      --gdb: set x := red
      --gdb: print x red
      --gdb: print enums_overload.reddish'(red) red
      --gdb: set y := red
      --gdb: print y red
      --gdb: cont
      null; -- STOP
   end Test_Enums_Overload;

end Enums_Overload;
