--  Copyright 2015-2024 Free Software Foundation, Inc.
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

with Bar; use Bar;

procedure Foo_O224_021 is
    O1 : constant Object_Type := Get_Str ("Foo");

    procedure Child1 is
        O2 : constant Object_Type := Get_Str ("Foo");

        function Child2 (S : String) return Boolean is -- STOP
        begin
            for C of S loop
                Do_Nothing (C);
                if C = 'o' then
                    return True;
                end if;
            end loop;
            return False;
        end Child2;

        R : Boolean;

    begin
        R := Child2 ("Foo");
        R := Child2 ("Bar");
        R := Child2 ("Foobar");
    end Child1;
begin
    Child1;
end Foo_O224_021;
