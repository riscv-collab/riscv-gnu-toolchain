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

with Ops; use Ops;
procedure Ops_Test is

begin
   Dummy (Make (31) + Make (11)); -- BEGIN
   Dummy (Make (31) - Make (11));
   Dummy (Make (31) * Make (11));
   Dummy (Make (31) / Make (11));
   Dummy (Make (31) mod Make (11));
   Dummy (Make (31) rem Make (11));
   Dummy (Make (31) ** Make (11));
   Dummy (Make (31) < Make (11));
   Dummy (Make (31) <= Make (11));
   Dummy (Make (31) > Make (11));
   Dummy (Make (31) >= Make (11));
   Dummy (Make (31) = Make (11));
   Dummy (Make (31) and Make (11));
   Dummy (Make (31) or Make (11));
   Dummy (Make (31) xor Make (11));
   Dummy (Make (31) & Make (11));
   Dummy (abs (Make (42)));
   Dummy (not Make (11));
   Dummy (+ Make (11));
   Dummy (- Make (11));
end Ops_Test;
