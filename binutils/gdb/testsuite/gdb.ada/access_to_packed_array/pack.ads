--  Copyright 2015-2024 Free Software Foundation, Inc.
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

package Pack is

   type Small is mod 2 ** 6;
   type Array_Type is array (0 .. 9) of Small
      with Pack;
   type Array_Access is access all Array_Type;

   A  : aliased Array_Type :=
     (1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
   AA : constant Array_Access := A'Access;

   procedure Do_Nothing (A : Array_Access);

end Pack;
