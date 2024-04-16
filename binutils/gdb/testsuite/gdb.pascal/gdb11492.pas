{
 Copyright 2010-2024 Free Software Foundation, Inc.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
}


program test_gdb_11492;

const
  LowBound = 1;
  HighBound = 8;
var
  integer_array : array[LowBound..HighBound] of integer;
  char_array : array[LowBound..HighBound] of char;
  i : integer;

begin
  for i:=LowBound to HighBound do
    begin
      integer_array[i]:=49+i;
      char_array[i]:=char(49+i);
    end;
  i:=0; { set breakpoint 1 here }
  char_array[5] := 'X';
  Writeln('integer array, index 5 is ',integer_array[5]);
  Writeln('char array, index 5 is ',char_array[5]);
end.

