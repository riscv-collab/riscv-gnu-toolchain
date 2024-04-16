--  Copyright 2023-2024 Free Software Foundation, Inc.
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

package Pck is
   pragma Linker_Options ("cstuff.o");

   procedure C_Procedure (Value : Integer);
   pragma Import(C, C_Procedure, "c_procedure");

   type Small is new Integer range 0 .. 2 ** 6 - 1;

   type Buffer is array (Integer range <>) of Small;
   pragma Pack (Buffer);

   type Enum_With_Gaps is
     (
      LIT0,
      LIT1,
      LIT2,
      LIT3,
      LIT4
     );

   for Enum_With_Gaps use
     (
      LIT0 => 3,
      LIT1 => 5,
      LIT2 => 8,
      LIT3 => 13,
      LIT4 => 21
     );
   for Enum_With_Gaps'size use 16;

   type Enum_Subrange is new Enum_With_Gaps range Lit1 .. Lit3;

   type AR is array (Enum_With_Gaps range <>) of Integer;

   procedure Do_Nothing (The_Buffer : in out Buffer; The_AR : in out AR; Hello: in out String);
end Pck;
