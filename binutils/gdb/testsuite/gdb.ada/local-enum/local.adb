--  Copyright 2021-2024 Free Software Foundation, Inc.
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

procedure Local is
  type E1 is (one, two, three);
  type E2 is (three, four, five);

  type A1 is array (E1) of Integer;
  type A2 is array (E2) of Integer;

  V1 : A1 := (0, 1, 2);
  V2 : A2 := (3, 4, 5);

begin
  null; -- STOP
end Local;
