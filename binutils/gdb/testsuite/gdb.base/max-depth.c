/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

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

struct s1
{
  int x;
  int y;
} s1;

struct s2
{
  int x;
  int y;
  struct
  {
    int z;
    int a;
  };
} s2;

struct s3
{
  int x;
  int y;
  struct
  {
    int z;
    int a;
    struct
    {
      int b;
      int c;
    };
  };
} s3;

struct s4
{
  int x;
  int y;
  struct
  {
    int x;
    int y;
    struct
    {
      int x;
      int y;
    } l2;
  } l1;
} s4;

struct s5
{
  union
  {
    int raw[3];
    struct
    {
      int x;
      int y;
      int z;
    };
  };
} s5;

typedef struct
{
  union
  {
    int raw[3];
    struct
    {
      int x;
      int y;
      int z;
    };
  };
} s6_t;

s6_t s6;

struct s7
{
  struct
  {
    int x;
    int y;
  };
  struct
  {
    int z;
    int a;
  };
  struct
  {
    int b;
    int c;
  };
} s7;

struct s8
{
  int x;
  int y;
  struct
  {
    int z;
    int a;
    struct
    {
      int b;
      int c;
    };
  } d1;
} s8;

struct s9
{
  int x;
  int y;
  struct
  {
    int k;
    int j;
    struct
    {
      int z;
      int a;
      struct
      {
        int b;
        int c;
      };
    } d1;
  };
  struct
  {
    int z;
    int a;
    struct
    {
      int b;
      int c;
    };
  } d2;
} s9;

struct s10
{
  int x[10];
  int y;
  struct
  {
    int k[10];
    int j;
    struct
    {
      int z;
      int a;
      struct
      {
        int b[10];
        int c;
      };
    } d1;
  };
  struct
  {
    int z;
    int a;
    struct
    {
      int b[10];
      int c;
    };
  } d2;
} s10;

struct s11
{
  int x;
  char s[10];
  struct
  {
    int z;
    int a;
  };
} s11;

/* The following are C++ inheritance testing.  */
#ifdef __cplusplus

/* This is non-virtual inheritance.  */
struct C1 { int c1 = 1; } c1;
struct C2 { int c2 = 2; } c2;
struct C3 : C2 { int c3 = 3; } c3;
struct C4 { int c4 = 4; } c4;
struct C5 : C4 { int c5 = 5; } c5;
struct C6 : C5 { int c6 = 6; } c6;
struct C7 : C1, C3, C6 { int c7 = 7; } c7;

/* This is virtual inheritance.  */
struct V1 { int v1 = 1; } v1;
struct V2 : virtual V1 { int v2 = 2; } v2;
struct V3 : virtual V1 { int v3 = 3; } v3;
struct V4 : virtual V2 { int v4 = 4; } v4;
struct V5 : virtual V2 { int v5 = 1; } v5;
struct V6 : virtual V2, virtual V3 { int v6 = 1; } v6;
struct V7 : virtual V4, virtual V5, virtual V6 { int v7 = 1; } v7;

#endif /* __cplusplus */

int
main ()
{
  return 0;
}
