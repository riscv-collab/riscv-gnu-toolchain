{
 Copyright 2008-2024 Free Software Foundation, Inc.

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

program integers;


function add(a,b : integer) : integer;
begin
  add:=a+b;
end;

function sub(a,b : integer) : integer;
begin
  sub:=a-b;
end;

var
  i, j, k, l : integer;

begin
  i := 0;
  j := 0;
  k := 0;
  l := 0; { set breakpoint 1 here }
  i := 1;
  j := 2;
  k := 3;
  l := k;

  i := j + k;

  j := 0; { set breakpoint 2 here }
  k := 0;
  l := add(i,j);
  l := sub(i,j);

end.
