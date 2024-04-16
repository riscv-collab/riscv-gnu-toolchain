--  Copyright 2017-2024 Free Software Foundation, Inc.
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

with Ada.Text_IO; use Ada.Text_IO;

procedure Crash is

   procedure Request_For_Crash is
   begin
      null; -- Just an anchor for the debugger...
   end Request_For_Crash;

   task type T is
      entry Done;
   end T;

   task body T is
   begin
      accept Done do
         null;
      end Done;
      Put_Line ("Task T: Rendez-vous completed.");
   end T;

   My_T : T;

begin

   --  Give some time for the task to be created, and start its execution,
   --  so that it reaches the accept statement.
   delay 0.01;

   Request_For_Crash;

   delay 0.01;
   Put_Line ("*** We didn't crash !?!");

   --  Complete the rendez-vous with our task, so it can complete.
   My_T.Done;

end Crash;
