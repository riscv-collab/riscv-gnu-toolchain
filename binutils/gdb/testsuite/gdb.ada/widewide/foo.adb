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

with Pck; use Pck;

procedure Foo is
   Some_Easy : Wide_Wide_Character := 'J';
   Some_Larger : Wide_Wide_Character := Wide_Wide_Character'Val(16#beef#);
   Some_Big : Wide_Wide_Character := Wide_Wide_Character'Val(16#00dabeef#);
   My_Ws : Wide_String := "wide";
   My_WWS : Wide_Wide_String := " helo";
begin
   Do_Nothing (Some_Easy'Address);  -- START
   Do_Nothing (Some_Larger'Address);
   Do_Nothing (My_Ws);
   Do_Nothing (My_WWS);
   Do_Nothing (Some_Big'Address);
end Foo;
