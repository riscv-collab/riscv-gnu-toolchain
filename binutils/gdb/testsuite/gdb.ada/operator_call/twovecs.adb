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

package body Twovecs is

   function Pt (X, Y : My_Integer) return Twovec is
   begin
      return Twovec'(X, Y);
   end Pt;

   function "+" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X + P1.X, P0.Y + P1.Y);
   end "+";

   function "-" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X - P1.X, P0.Y - P1.Y);
   end "-";

   function "*" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X * P1.X, P0.Y * P1.Y);
   end "*";

   function "/" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X / P1.X, P0.Y / P1.Y);
   end "/";

   function "mod" (P0, P1 : Twovec) return Twovec is
   begin
      -- Make sure we get a different answer than "-".
      return Twovec' (17, 18);
   end "mod";

   function "rem" (P0, P1 : Twovec) return Twovec is
   begin
      -- Make sure we get a different answer than "-".
      return Twovec' (38, 39);
   end "rem";

   function "**" (P0, P1 : Twovec) return Twovec is
   begin
      -- It just has to do something recognizable.
      return Twovec' (20 * P0.X + P1.X, 20 * P0.Y + P1.Y);
   end "**";

   function "<" (P0, P1 : Twovec) return Boolean is
   begin
      return P0.X < P1.X and then P0.Y < P1.Y;
   end "<";

   function "<=" (P0, P1 : Twovec) return Boolean is
   begin
      return P0.X <= P1.X and then P0.Y <= P1.Y;
   end "<=";

   function ">" (P0, P1 : Twovec) return Boolean is
   begin
      return P0.X > P1.X and then P0.Y > P1.Y;
   end ">";

   function ">=" (P0, P1 : Twovec) return Boolean is
   begin
      return P0.X >= P1.X and then P0.Y >= P1.Y;
   end ">=";

   function "=" (P0, P1 : Twovec) return Boolean is
   begin
      return P0.X = P1.X and then P0.Y = P1.Y;
   end "=";

   function "and" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X and P1.X, P0.Y and P1.Y);
   end "and";

   function "or" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X or P1.X, P0.Y or P1.Y);
   end "or";

   function "xor" (P0, P1 : Twovec) return Twovec is
   begin
      return Twovec' (P0.X xor P1.X, P0.Y xor P1.Y);
   end "xor";

   function "&" (P0, P1 : Twovec) return Twovec is
   begin
      -- It just has to do something recognizable.
      return Twovec' (10 * P0.X + P1.X, 10 * P0.Y + P1.Y);
   end "&";

   function "abs" (P0 : Twovec) return Twovec is
   begin
      return Twovec' (abs (P0.X), abs (P0.Y));
   end "abs";

   function "not" (P0 : Twovec) return Twovec is
   begin
      return Twovec' (not (P0.X), not (P0.Y));
   end "not";

   function "+" (P0 : Twovec) return Twovec is
   begin
      -- It just has to do something recognizable.
      return Twovec' (+ (P0.Y), + (P0.X));
   end "+";

   function "-" (P0 : Twovec) return Twovec is
   begin
      return Twovec' (- (P0.X), - (P0.Y));
   end "-";

   procedure Do_Nothing (P : Twovec) is
   begin
      null;
   end Do_Nothing;

end Twovecs;
