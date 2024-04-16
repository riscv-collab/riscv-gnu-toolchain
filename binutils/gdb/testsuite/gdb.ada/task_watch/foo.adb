--  Copyright 2009-2024 Free Software Foundation, Inc.
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

procedure Foo is

   Value : Integer := 0;

   task type Caller is
      entry Initialize;
      entry Call_Break_Me;
      entry Finalize;
   end Caller;
   type Caller_Ptr is access Caller;

   procedure Break_Me is
   begin
      Value := Value + 1;
   end Break_Me;

   task body Caller is
   begin
      accept Initialize do
         null;
      end Initialize;
      accept Call_Break_Me do
         Break_Me;
      end Call_Break_Me;
      accept Finalize do
         null;
      end Finalize;
   end Caller;

   Task_List : array (1 .. 3) of Caller_Ptr;

begin

   --  Start all our tasks, and call the "Initialize" entry to make
   --  sure all of them have now been started.  We call that entry
   --  immediately after having created the task in order to make sure
   --  that we wait for that task to be created before we try to create
   --  another one.  That way, we know that the order in our Task_List
   --  corresponds to the order in the GNAT runtime.
   for J in Task_List'Range loop
      Task_List (J) := new Caller;
      Task_List (J).Initialize;
   end loop;

   --  Next, call their Call_Break_Me entry of each task, using the same
   --  order as the order used to create them.
   for J in Task_List'Range loop  -- STOP_HERE
      Task_List (J).Call_Break_Me;
   end loop;

   --  And finally, let all the tasks die...
   for J in Task_List'Range loop
      Task_List (J).Finalize;
   end loop;

   null; -- STOP_HERE_2

end Foo;
