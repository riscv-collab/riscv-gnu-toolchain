--  Copyright 2008-2024 Free Software Foundation, Inc.
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

   type Data_T is (One, Two, Three);
   pragma Atomic (Data_T);

   Data_Flag : Data_T := One;

   procedure Increment is
   begin
      if Data_Flag = Data_T'Last then
         Data_Flag := Data_T'First;
      else
         Data_Flag := Data_T'Succ (Data_Flag);
      end if;
   end Increment;

   function Is_First return Boolean is
   begin
      return Data_Flag = Data_T'First;
   end Is_First;

end Pck;
