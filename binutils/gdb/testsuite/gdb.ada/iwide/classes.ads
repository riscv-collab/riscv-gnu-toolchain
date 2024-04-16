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

package Classes is

   type Point is record
      X : Integer;
      Y : Integer;
   end record;

   type Shape is abstract tagged null record;

   type Shape_Access is access all Shape'Class;

   type Drawable is interface;

   type Drawable_Access is access all Drawable'Class;

   procedure Draw (D : Drawable) is abstract;

   type Circle is new Shape and Drawable with record
      Center : Point;
      Radius : Natural;
   end record;

   procedure Draw (R : Circle);

   My_Circle   : Circle := ((1, 2), 3);
   My_Shape    : Shape'Class := Shape'Class (My_Circle);
   My_Drawable : Drawable'Class := Drawable'Class (My_Circle);

   S_Access : Shape_Access := new Circle'(My_Circle);
   D_Access : Drawable_Access := new Circle'(My_Circle);

   type R (MS : Shape_Access; MD : Drawable_Access) is record
      E : Integer;
   end record;

   MR : R := (MS => S_Access, MD => D_Access, E => 42);

   type Shape_Array is array (1 .. 4) of Shape_Access;
   type Drawable_Array is array (1 .. 4) of Drawable_Access;

   S_Array : Shape_Array := (others => S_Access);
   D_Array : Drawable_Array := (others => D_Access);

end Classes;
