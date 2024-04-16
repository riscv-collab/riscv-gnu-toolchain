--  Copyright 2023-2024 Free Software Foundation, Inc.
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

   type Data_Large is record
      I : Integer;
      J : Integer;
      K : Integer;
      L : Integer;
      M : Integer;
      N : Integer;
      O : Integer;
      P : Integer;
      Q : Integer;
      R : Integer;
      S : Integer;
      T : Integer;
   end record;

   function Create_Large return Data_Large;

   procedure Break_Me;

end Pck;
