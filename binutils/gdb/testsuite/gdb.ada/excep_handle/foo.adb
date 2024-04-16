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

with Pck; use Pck;

procedure Foo is
begin

   -- Part 1 of the testcase

   begin
      raise Constraint_Error;
   exception
      when Constraint_Error =>
         null;
   end;

   begin
      null;
   exception
      when others =>
         null;
   end;

   begin
      raise Storage_Error;
   exception
      when Storage_Error =>
         null;
   end;

   -- Part 2 of the testcase

   begin
      raise ABORT_SIGNAL;
   exception
      when others =>
         null;
   end;

   begin
      raise Program_Error;
   exception
      when Program_Error =>
         null;
   end;

   begin
      raise Storage_Error;
   exception
      when Storage_Error =>
         null;
   end;

  -- Part 3 of the testcase

   begin
      Global_Var := Global_Var + 1;
      raise ABORT_SIGNAL;
   exception
      when others =>
         null;
   end;

   begin
      Global_Var := Global_Var + 1;
      raise Constraint_Error;
   exception
      when Constraint_Error =>
         null;
   end;

   -- Part 4 of the testcase

   begin
      Global_Var := Global_Var + 1;
      raise Program_Error;
   exception
      when others =>
         null;
   end;

   begin
      Global_Var := Global_Var + 1;
      raise Program_Error;
   exception
      when Program_Error =>
         null;
   end;

end Foo;
