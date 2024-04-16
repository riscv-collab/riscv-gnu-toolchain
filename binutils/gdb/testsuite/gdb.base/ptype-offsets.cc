/* This testcase is part of GDB, the GNU debugger.

   Copyright 2017-2024 Free Software Foundation, Inc.

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

#include <stdint.h>

/* A struct with many types of fields, in order to test 'ptype
   /o'.  */

struct abc
{
  /* Virtual destructor.  */
  virtual ~abc ()
  {}

  /* 8-byte address.  Because of the virtual destructor above, this
     field's offset will be 8.  */
  void *field1;

  /* No hole here.  */

  /* 4-byte int bitfield of 1-bit.  */
  unsigned int field2 : 1;

  /* 31-bit hole here.  */

  /* 4-byte int.  */
  signed int field3;

  /* No hole here.  */

  /* 1-byte char.  */
  signed char field4;

  /* 7-byte hole here.  */

  /* 8-byte int.  */
  uint64_t field5;

  /* We just print the offset and size of a union, ignoring its
     fields.  */
  union
  {
    /* 8-byte address.  */
    void *field6;

    /* 4-byte int.  */
    signed int field7;
  } field8;

  /* Empty constructor.  */
  abc ()
  {}

  /* Typedef defined in-struct.  */
  typedef short my_int_type;

  my_int_type field9;
};

/* This struct will be nested inside 'struct xyz'.  */

struct tuv
{
  signed int a1;

  signed char *a2;

  signed int a3;
};

/* This struct will be nested inside 'struct pqr'.  */

struct xyz
{
  signed int f1;

  signed char f2;

  void *f3;

  struct tuv f4;
};

/* A struct with a nested struct.  */

struct pqr
{
  signed int ff1;

  struct xyz ff2;

  signed char ff3;
};

/* A union with two nested structs.  */

union qwe
{
  struct tuv fff1;

  struct xyz fff2;
};

/* A struct with an union.  */

struct poi
{
  signed int f1;

  union qwe f2;

  uint16_t f3;

  struct pqr f4;
};

/* A struct with bitfields.  */

struct tyu
{
  signed int a1 : 1;

  signed int a2 : 3;

  signed int a3 : 23;

  signed char a4 : 2;

  int64_t a5;

  signed int a6 : 5;

  int64_t a7 : 3;
};

/* A struct with structs and unions.  */

struct asd
{
  struct jkl
  {
      signed char *f1;
      union
      {
	void *ff1;
      } f2;
      union
      {
	signed char *ff2;
      } f3;
      int f4 : 5;
      unsigned int f5 : 1;
      short f6;
  } f7;
  unsigned long f8;
  signed char *f9;
  int f10 : 4;
  unsigned int f11 : 1;
  unsigned int f12 : 1;
  unsigned int f13 : 1;
  unsigned int f14 : 1;
  void *f15;
  void *f16;
};

/* See PR c++/23373.  */

struct static_member
{
  static static_member Empty;
  int abc;
};

/* Work around PR gcc/101452.  */
static_member static_member::Empty;

struct empty_member
{
  struct { } empty;
  int an_int;
};

int
main (int argc, char *argv[])
{
  struct abc foo;
  struct pqr bar;
  union qwe c;
  struct poi d;
  struct tyu e;
  struct asd f;
  uint8_t i;
  static_member stmember;
  empty_member emember;

  return 0;
}
