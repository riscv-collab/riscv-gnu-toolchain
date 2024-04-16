--  Copyright 2020-2024 Free Software Foundation, Inc.
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

procedure Mi_Access is
   A_String : String (3 .. 5) := "345"; -- STOP
   A_String_Access : String_Access;
   A_Pointer : Pointer;
begin
   Do_Nothing (A_String'Address);
   A_String (4) := '6';
   A_String_Access := Copy (A_String);
   A_Pointer.P := A_String_Access;
   Do_Nothing (A_String_Access'Address); -- STOP2
   A_String_Access (4) := 'a';
   Do_Nothing (A_Pointer'Address);
   A_String_Access := Copy("Hi");
   A_Pointer.P := A_String_Access;
   Do_Nothing (A_String_Access'Address);
   A_String_Access := null;
   A_Pointer.P := null;
   Do_Nothing (A_Pointer'Address); -- STOP3
   Do_Nothing (A_String'Address);
end Mi_Access;
