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

with Pck; use Pck;

package body Mixed is
   --  We are importing symbols from foo.o, so make sure this object file
   --  gets linked in.
   Pragma Linker_Options ("foo.o");

   type Color is (Red, Green, Blue);

   procedure C_Function;
   pragma Import (C, C_Function, "c_function");

   procedure Callme;
   pragma Export (C, Callme, "callme");

   procedure Break_Me (Light : Color) is
   begin
      Put_Line ("Light: " & Color'Image (Light));  --  STOP
   end Break_Me;

   procedure Callme is
   begin
      Break_Me (Red);
      Break_Me (Green);
      Break_Me (Blue);
   end Callme;

   procedure Start_Test is
   begin
      --  Call C_Function, which will call Callme.
      C_Function;
   end Start_Test;

end Mixed;
