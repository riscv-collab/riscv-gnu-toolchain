/* This testcase is part of GDB, the GNU debugger.

   Copyright 2010-2024 Free Software Foundation, Inc.

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

class class1
{
 public:
  class1 (int _x) : x (_x) {}
  int x;
};

class1 class1_obj (42);
const class1 const_class1_obj (42);
volatile class1 volatile_class1_obj (42);
const volatile class1 const_volatile_class1_obj (42);

typedef class1 typedef_class1;

typedef_class1 typedef_class1_obj (42);

class1& class1_ref_obj (class1_obj);

typedef const typedef_class1 typedef_const_typedef_class1;

typedef_const_typedef_class1 typedef_const_typedef_class1_obj (42);

typedef typedef_const_typedef_class1& typedef_const_typedef_class1_ref;

typedef_const_typedef_class1_ref typedef_const_typedef_class1_ref_obj (typedef_const_typedef_class1_obj);

class subclass1 : public class1
{
 public:
  subclass1 (int _x, int _y) : class1 (_x), y (_y) {}
  int y;
};

subclass1 subclass1_obj (42, 43);

enum enum1 { A, B, C };

enum1 enum1_obj (A);

struct A
{
	int a;
	union {
		int b0;
		int b1;
		union {
			int bb0;
			int bb1;
			union {
				int bbb0;
				int bbb1;
			};
		};
	};
	int c;
	union {
		union {
			int dd0;
			int dd1;
		};
		int d2;
		int d3;
	};
};

struct A a = {1,20,3,40};

int
main ()
{
  return 0;
}
