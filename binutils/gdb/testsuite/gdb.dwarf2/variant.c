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

unsigned char buffer[8];
unsigned char buffer2[8];

static void
func (void)
{
}

int
main (void)
{
  /* Write the expected values into the buffer.  */
  unsigned int x = 23;
  if (*(char *) &x)
    {
      /* Little endian.  */
      buffer[0] = 23;
      buffer[4] = 23;
      buffer2[0] = 255;
      buffer2[4] = 23;
    }
  else
    {
      buffer[3] = 23;
      buffer[7] = 23;
      buffer2[0] = 255;
      buffer2[7] = 23;
    }

  func ();
  return 0;
}
