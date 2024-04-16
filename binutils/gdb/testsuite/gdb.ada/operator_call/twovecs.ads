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

package Twovecs is
   type My_Integer is mod 2**16	;

   type Twovec is private;

   function Pt (X, Y : My_Integer) return Twovec;

   function "+" (P0, P1 : Twovec) return Twovec;
   function "-" (P0, P1 : Twovec) return Twovec;
   function "*" (P0, P1 : Twovec) return Twovec;
   function "/" (P0, P1 : Twovec) return Twovec;
   function "mod" (P0, P1 : Twovec) return Twovec;
   function "rem" (P0, P1 : Twovec) return Twovec;
   function "**" (P0, P1 : Twovec) return Twovec;

   function "<" (P0, P1 : Twovec) return Boolean;
   function "<=" (P0, P1 : Twovec) return Boolean;
   function ">" (P0, P1 : Twovec) return Boolean;
   function ">=" (P0, P1 : Twovec) return Boolean;
   function "=" (P0, P1 : Twovec) return Boolean;

   function "and" (P0, P1 : Twovec) return Twovec;
   function "or" (P0, P1 : Twovec) return Twovec;
   function "xor" (P0, P1 : Twovec) return Twovec;
   function "&" (P0, P1 : Twovec) return Twovec;

   function "abs" (P0 : Twovec) return Twovec;
   function "not" (P0 : Twovec) return Twovec;
   function "+" (P0 : Twovec) return Twovec;
   function "-" (P0 : Twovec) return Twovec;

   procedure Do_Nothing (P : Twovec);

private

   type Twovec is record
      X, Y : My_Integer;
   end record;

end Twovecs;
