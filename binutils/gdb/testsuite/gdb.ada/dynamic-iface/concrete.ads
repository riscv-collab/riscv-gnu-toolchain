--  Copyright 2022-2024 Free Software Foundation, Inc.
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

package Concrete is
   type Iface is interface;

   type Base (N : Integer) is tagged record
      A : String (1 .. N);
   end record;

   -- An empty extension of Base.  The compiler sources claimed there
   -- was a special case for this, and while that doesn't seem to be
   -- true in practice, it's worth checking.
   type Intermediate is new Base with record
      null;
   end record;

   type Object is new Intermediate and Iface with record
      Value: Integer;
   end record;

   procedure Accept_Iface (Obj: Iface'Class);

end Concrete;
