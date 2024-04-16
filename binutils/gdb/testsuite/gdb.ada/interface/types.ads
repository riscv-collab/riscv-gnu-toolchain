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

package Types is

   type Object_Int is interface;

   type Another_Int is interface;

   type Object_Root is abstract tagged record
      X : Natural;
      Y : Natural;
   end record;

   type Object is abstract new Object_Root and Object_Int and Another_Int
     with null record;
   function Ident (O : Object'Class) return Object'Class;
   procedure Do_Nothing (O : in out Object'Class);

   type Rectangle is new Object with record
      W : Natural;
      H : Natural;
   end record;

   type Circle is new Object with record
      R : Natural;
   end record;

end Types;

