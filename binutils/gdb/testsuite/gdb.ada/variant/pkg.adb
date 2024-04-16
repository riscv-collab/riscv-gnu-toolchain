--  Copyright 2020-2024 Free Software Foundation, Inc.
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

with Pck; use Pck;

procedure Pkg is

   R, Q : Rec_Type;

   ST1 : constant Second_Type := (I => -4, One => 1, X => 2);
   ST2 : constant Second_Type := (I => 99, One => 1, Y => 77);

   NAV1 : constant Nested_And_Variable := (One => 0, Two => 93,
                                           Str => (others => 'z'));
   NAV2 : constant Nested_And_Variable := (One => 3, OneValue => 33,
                                           Str => (others => 'z'),
                                           Str2 => (others => 'q'),
                                           Two => 0);
   NAV3 : constant Nested_And_Variable := (One => 3, OneValue => 33,
                                           Str => (others => 'z'),
                                           Str2 => (others => 'q'),
                                           Two => 7, TwoValue => 88);

begin
   R := (C => 'd');
   Q := (C => Character'First, X_First => 27);

   null; -- STOP
end Pkg;
