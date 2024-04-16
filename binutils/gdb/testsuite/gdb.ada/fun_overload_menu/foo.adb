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

procedure Foo is

   type New_Integer is new Integer;
   type Integer_Access is access Integer;

   function F (I : Integer; A : Integer_Access) return Boolean is
   begin
      return True;
   end F;

   function F (I : New_Integer; A : Integer_Access) return Boolean is
   begin
      return False;
   end F;

   procedure P (I : Integer; A : Integer_Access) is
   begin
      null;
   end P;

   procedure P (I : New_Integer; A : Integer_Access) is
   begin
      null;
   end P;

   B1 : constant Boolean := F (Integer'(1), null); --  BREAK
   B2 : constant Boolean := F (New_Integer'(2), null);

begin
   P (Integer'(3), null);
   P (New_Integer'(4), null);
end Foo;
