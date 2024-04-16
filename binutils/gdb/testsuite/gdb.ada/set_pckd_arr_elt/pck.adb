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
   function New_Variant (Size : Integer) return Variant_Access is
      Result : Variant (Size => Size) :=
        (Size => Size, A => 11, T => (others => 13));
   begin
      return new Variant'(Result);
   end New_Variant;

   procedure Update_Small (S : in out Small) is
   begin
      null;
   end Update_Small;
end Pck;
