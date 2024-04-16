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

package Pck is
  type Array_Type is array (1 .. 64) of Integer;

  type Maybe_Array (Defined : Boolean := False) is
    record
      Arr : Array_Type;
      Arr2 : Array_Type;
    end record;

  type Result_T (Defined : Boolean := False) is
    record
      case Defined is
        when False =>
	  Arr : Maybe_Array;
        when True =>
          Payload : Boolean;
      end case;
    end record;

  function Get (Value: Boolean) return Result_T;
end Pck;
