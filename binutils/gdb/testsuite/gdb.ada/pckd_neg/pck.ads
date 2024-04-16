--  Copyright (C) 2015-2024 Free Software Foundation, Inc.
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

package Pck is
   type Signed_Small is new Integer range - (2 ** 5) .. (2 ** 5 - 1);
   type Signed_Simple_Array is array (1 .. 4) of Signed_Small;
   pragma Pack (Signed_Simple_Array);

   procedure Update_Signed_Small (S : in out Signed_Small);
end Pck;
