--  Copyright 2011-2024 Free Software Foundation, Inc.
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

procedure Task_Switch is

   -------------------
   -- Declaractions --
   -------------------

   task type Callee is
      entry Finito;
   end Callee;
   type Callee_Ptr is access Callee;

   task type Caller is
   end Caller;
   type Caller_Ptr is access Caller;

   procedure Break_Me;

   My_Caller : Caller_Ptr;
   My_Callee : Callee_Ptr;

   ------------
   -- Bodies --
   ------------

   task body Callee is
   begin
      --  Just wait until we are told to terminate this task.
      --  This is just to maintain this task alive.
      accept Finito do
         null;
      end Finito;
   end Callee;

   task body Caller is
   begin
      Break_Me;
      My_Callee.Finito;
   end Caller;

   procedure Break_Me is
   begin
      null;
   end Break_Me;

begin

   --  Make sure to create the Callee task first... And then give it
   --  enough time to complete its activation phase before we start
   --  the Caller task.
   My_Callee := new Callee;
   delay 0.1;

   My_Caller := new Caller;

end Task_Switch;
