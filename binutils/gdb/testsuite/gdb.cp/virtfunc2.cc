 /* This test script is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

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

class interface
{
  virtual int do_print3() { return 111111; }
};
 
class Obj : virtual public interface
{
public:
  virtual int do_print() { return 123456; }
};

class Obj2 : Obj,  virtual public interface
{
  virtual int do_print2() { return 654321; }
};

int main(int argc, char** argv) {
  Obj o;
  Obj2 o2;
  return 0;	// marker 1
}
