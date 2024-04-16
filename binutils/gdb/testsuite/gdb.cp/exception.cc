/* This testcase is part of GDB, the GNU debugger.

   Copyright 1997-2024 Free Software Foundation, Inc.

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

// Test file for exception handling support.

int foo (int i)
{
  if (i < 32)
    throw (int) 13;
  else
    return i * 2;
}

extern "C" int bar (int k, unsigned long eharg, int flag);
    
int bar (int k, unsigned long eharg, int flag)
{
  return 1;
}

int catcher (int x)
{
  return x;
}

int main()
{
  int j;

  try {
    j = foo (20);
  }
  catch (int x) {
    catcher (x);
  }
  
  try {
    try {
      j = foo (20);
    }
    catch (int x) {
      catcher (x);
      throw;
    }
  }
  catch (int y) {
    catcher (y);
  }

  // Not caught 
  foo (20);
}
