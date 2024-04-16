--  Copyright 2005-2024 Free Software Foundation, Inc.
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

procedure PA is

   type Packed_Array is array (4 .. 8) of Boolean;
   pragma pack (Packed_Array);

   Var : Packed_Array;

   --  Unconstrained packed array (bounds are dynamic).
   type Unconstrained_Packed_Array is array (Integer range <>) of Boolean;

   U_Var : Unconstrained_Packed_Array (1 .. Ident (6));

   -- Note that this array is not packed.
   type Outer_Array is array (1 .. 4) of Packed_Array;
   O_Var : Outer_Array := ((true, false, true, false, true),
                           (true, false, true, false, true),
                           (true, false, true, false, true),
                           (true, false, true, false, true));

   type Outer_Array2 is array (1 .. 4) of Packed_Array;
   pragma pack (Outer_Array2);
   O2_Var : Outer_Array2 := ((true, false, true, false, true),
                           (true, false, true, false, true),
                           (true, false, true, false, true),
                           (true, false, true, false, true));

begin

   Var := (True, False, True, False, True);
   U_Var := (True, False, False, True, True, False);

   Var (8) := False;  -- STOP
   U_Var (U_Var'Last) := True;

end PA;
