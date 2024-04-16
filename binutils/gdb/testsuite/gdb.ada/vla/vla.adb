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

procedure Vla is
   type Array_Type is array (Natural range <>) of Integer;
   type Record_Type (L1, L2 : Natural) is record
      I1 : Integer;
      A1 : Array_Type (1 .. L1);
      I2 : Integer;
      A2 : Array_Type (1 .. L2);
      I3 : Integer;
   end record;

   -- Some versions of GCC emit the members in the incorrect order.
   -- Since this isn't relevant to the bug at hand, disable
   -- reordering to get consistent results.
   pragma No_Component_Reordering (Record_Type);

   procedure Process (R : Record_Type) is
   begin
      null;
   end Process;

   R00 : Record_Type :=
     (L1 => 0, L2 => 0,
      I1 => 1, A1 => (others => 10),
      I2 => 2, A2 => (others => 20),
      I3 => 3);
   R01 : Record_Type :=
     (L1 => 0, L2 => 1,
      I1 => 1, A1 => (others => 10),
      I2 => 2, A2 => (others => 20),
      I3 => 3);
   R10 : Record_Type :=
     (L1 => 1, L2 => 0,
      I1 => 1, A1 => (others => 10),
      I2 => 2, A2 => (others => 20),
      I3 => 3);
   R22 : Record_Type :=
     (L1 => 2, L2 => 2,
      I1 => 1, A1 => (others => 10),
      I2 => 2, A2 => (others => 20),
      I3 => 3);

begin
   Process (R00); -- Set breakpoint here
   Process (R01);
   Process (R10);
   Process (R22);
end Vla;
