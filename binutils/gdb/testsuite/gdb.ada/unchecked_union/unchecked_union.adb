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

with System;
with Pck; use Pck;

procedure Unchecked_Union is
   type Key is (Alpha, Beta, Omega);

   type Inner(Disc : Key := Omega) is record
      case Disc is
         when Alpha =>
            Small : Integer range 0..255;
            Second : Integer range 0..255;
         when Beta =>
            Bval : Integer range 0..255;
         when others =>
            Large : Integer range 255..510;
            More : Integer range 255..510;
      end case;
   end record;
   pragma Unchecked_Union (Inner);

   type Outer(Disc : Key := Alpha) is record
      case Disc is
         when Alpha =>
            Field_One : Integer range 0..255;
         when others =>
            Field_Two : Integer range 255..510;
      end case;
   end record;
   pragma Unchecked_Union (Outer);

   type Pair is record
      Pone : Inner;
      Ptwo : Outer;
   end record;

   Value : Pair;

begin
   Do_Nothing (Value'Address);          -- BREAK
end Unchecked_Union;
