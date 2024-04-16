--  Copyright 2018-2024 Free Software Foundation, Inc.
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

with Global_Pack; use Global_Pack;
procedure Proc_In_Ada is
  procedure Something_In_C
    with Import, Convention => C, External_Name => "proc_in_c";
   pragma Linker_Options ("some_c.o");
begin
  Something_In_C;
  Some_Number_In_Ada := Some_Number_In_Ada + 1;
end Proc_In_Ada;
