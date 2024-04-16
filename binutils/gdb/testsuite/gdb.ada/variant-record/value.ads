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

package Value is
  package Value_Name is
    Length : constant Positive := 8;
    subtype Name_T is String (1 .. Length);

    type A_Record_T is
      record
        X1 : Natural;
        X2 : Natural;
      end record;

    type Yes_No_T is (Yes, No);
    type T (Well : Yes_No_T := Yes) is
      record
        case Well is
          when Yes =>
            Name : Name_T;
          when No =>
            Unique_Name : A_Record_T;
        end case;
      end record;
  end;

  type T is private;
  function Create return T;
  function Name (Of_Value : T) return Value_Name.T;
private
  type T is
    record
      One : Value_Name.T (Well => Value_Name.No);
      Two : Value_Name.T (Well => Value_Name.Yes);
    end record;
end;
