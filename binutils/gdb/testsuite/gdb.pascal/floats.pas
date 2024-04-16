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


program floats;

var
  i, j, k, l : integer;
  r, s, t, u : real;

begin
  i := 0;
  j := 0;
  k := 0;
  r := 0.0;
  s := 0.0;
  t := 0.0;
  u := 0.0;
  l := 0;
  i := 1; { set breakpoint 1 here }
  r := 1.25;
  s := 2.2;
  t := -3.2;
  u := 78.3;
  l := 1;
  i := 1;
  u := pi; { set breakpoint 2 here }
  r := cos(u);
  s := sin(u);
  u := pi / 2;
  r := cos(u);
  s := sin(u);
end.
