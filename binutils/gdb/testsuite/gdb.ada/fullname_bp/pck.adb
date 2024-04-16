--  Copyright 2011-2024 Free Software Foundation, Inc.
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

with Dn; use Dn;

package body Pck is
   procedure Hello is
      procedure Nested is
         I : Integer := 0;
      begin
         Do_Nothing (I'Address);
      end Nested;
   begin
      Nested;
   end Hello;

   procedure There is
   begin
      null;
   end There;
end Pck;


