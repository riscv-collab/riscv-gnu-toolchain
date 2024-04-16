--  Copyright 2012-2024 Free Software Foundation, Inc.
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

   task type T is
      entry Finish;
   end T;
   type TA is access T;

   task body T is
   begin
      --  Wait for up to 100 seconds for the Finish Rendez-Vous to occur.
      select
         accept Finish do
            null;
         end Finish;
      or
         delay 100.0;
      end select;
   end T;

   MIT : TA;

begin

   --  Create a task, and give it some time to activate and then start
   --  its execution.
   MIT := new T;
   delay 0.01;

   --  Now, call our anchor. The task we just created should now be
   --  blocked on a timed entry wait.
   Break_Me;

   --  Tell the task to finish before the 100 seconds are up.  The test
   --  is now finished, no need to wait :).
   MIT.Finish;
end Foo;
