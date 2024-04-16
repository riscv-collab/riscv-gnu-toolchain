/* Copyright 2018-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* A class that does not have RTTI and is small enough to fit in a
   hardware watchpoint on all targets.  */
class smallstuff { public: int n; };

smallstuff watchme[5];

int
main ()
{
  for (int i = 0; i < sizeof (watchme) / sizeof (watchme[0]); i++)
    watchme[i].n = i;
  return 0;
}
