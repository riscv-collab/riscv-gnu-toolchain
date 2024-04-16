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

package Pck is

   type Rec_Type (C : Character := 'd') is record
      case C is
         when Character'First     => X_First : Integer;
         when Character'Val (127) => X_127   : Integer;
         when Character'Val (128) => X_128   : Integer;
         when Character'Last      => X_Last  : Integer;
         when others              => null;
      end case;
   end record;

   type Second_Type (I : Integer) is record
      One: Integer;
      case I is
         when -5 .. 5 =>
	   X : Integer;
         when others =>
	   Y : Integer;
      end case;
   end record;

   type Nested_And_Variable (One, Two: Integer) is record
       Str : String (1 .. One);
       case One is
          when 0 =>
	     null;
          when others =>
	     OneValue : Integer;
             Str2 : String (1 .. Two);
             case Two is
	        when 0 =>
		   null;
		when others =>
		   TwoValue : Integer;
             end case;
       end case;
   end record;
end Pck;
