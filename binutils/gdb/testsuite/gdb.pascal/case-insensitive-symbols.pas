{
 Copyright 2015-2024 Free Software Foundation, Inc.

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


program test_gdb_17815;


type
  TA = class
  public
  x, y : integer;
  constructor Create;
  function check(b : TA) : boolean;
  destructor Done; virtual;
end;

constructor TA.Create;
begin
  x:=-1;
  y:=-1;
end;

destructor TA.Done;
begin
end;

function TA.check (b : TA) : boolean;
begin
  check:=(x < b.x); { set breakpoint here }
end;



var
  a, b : TA;

begin
  a:=TA.Create;
  b:=TA.Create;
  a.x := 67;
  a.y := 33;
  b.x := 11;
  b.y := 35;
  if a.check (b) then
    writeln('Error in check')
  else
    writeln('check OK');
end.

