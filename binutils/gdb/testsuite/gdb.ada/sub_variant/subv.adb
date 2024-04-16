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

procedure Subv is
  type Indicator_T is (First, Last);

  type T1 (Indicator : Indicator_T := First) is
    record
      case Indicator is
        when First =>
          Value : Natural;
        when Last =>
          null;
      end case;
    end record;

  type T2 (Indicator : Indicator_T := First) is
    record
      Associated : T1;
      case Indicator is
        when First =>
          Value : Natural;
        when Last =>
          null;
      end case;
    end record;

  Q : T2  := ( First, (First, 42), 51 );
  R : T2  := ( First, (Indicator => Last), 51 );
  S : T2  := ( Last, (First, 42));
begin
  null;  -- STOP
end;
