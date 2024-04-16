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

procedure Prog is
  type Index is range 0 .. 31;
  type Char_Array is array ( Index range <>) of Character;

  type Rec (Length : Index) is
    record
      TV_Description        : Char_Array (1 .. Length);
      Note                  : Char_Array (1 .. Length);
    end record;

  X : Rec (7);
begin
  null; -- BREAK
end Prog;
