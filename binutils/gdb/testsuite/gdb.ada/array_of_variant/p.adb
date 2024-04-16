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

procedure P is

   type Tag_T is (Unused, Object);

   type Array_T is array (1 .. Five) of Integer;

   type Payload_T (Tag : Tag_T := Unused) is
      record
         case Tag is
            when Object =>
               Values : Array_T := (others => 1);
            when Unused =>
               null;
         end case;
      end record;

   Objects : array (1 .. 2) of Payload_T;

   type Another_Type (Tag : Tag_T := Unused) is
      record
         case Tag is
	    when Unused =>
	       CVal : Character;
            when Object =>
	       IVal : Integer;
	 end case;
      end record;

   type Enclosing is record
      Initial : Integer;
      Rest : Another_Type;
   end record;

   Another_Array : array (1 .. 2) of Enclosing
      := ((Initial => 0, Rest => (Tag => Unused, CVal => 'X')),
          (Initial => 0, Rest => (Tag => Object, IVal => 88)));

begin
   Objects (1) := (Tag => Object, Values => (others => 2));
   Do_Nothing (Objects'Address);  --  START
   Do_Nothing (Another_Array'Address);
end P;
