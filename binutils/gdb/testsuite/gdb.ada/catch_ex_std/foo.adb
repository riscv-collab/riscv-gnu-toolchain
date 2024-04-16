--  Copyright 2019-2024 Free Software Foundation, Inc.
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

with Some_Package;

procedure Foo is
   Some_Val : Integer := 0;
begin
   Some_Package.Do_Something (Some_Val);
   if Some_Val = 1 then
      raise Some_Package.Some_Kind_Of_Error;
   end if;
end Foo;
