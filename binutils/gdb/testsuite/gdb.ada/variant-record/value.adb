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

package body Value is
  function Create return T is
  begin
    return (One => (Well => Value_Name.No,
                    Unique_Name => (X1 => 1, X2 => 2)),
            Two => (Well => Value_Name.Yes,
                    Name => "abcdefgh"));
  end Create;

  function Name (Of_Value : T) return Value_Name.T is
  begin
    return Of_Value.Two;
  end Name;

end Value;
