/* This testcase is part of GDB, the GNU debugger.

   Copyright 2013-2024 Free Software Foundation, Inc.

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

struct s
{
  unsigned char a;
  unsigned char b;
  unsigned char c;
};

struct t
{
  /* First, a complete byte.  */
  unsigned char a;
  /* Next, 8 single bits.  */
  unsigned char b : 1;
  unsigned char c : 1;
  unsigned char d : 1;
  unsigned char e : 1;
  unsigned char f : 1;
  unsigned char g : 1;
  unsigned char h : 1;
  unsigned char i : 1;
  /* Now another byte.  */
  unsigned char j;
};

void
end (void)
{
  /* Nothing.  */
}

void
dummy (void)
{
  /* Nothing.  */
}

int
foo (struct s x, struct s y, struct s z)
{
  asm (".global foo_start_lbl\nfoo_start_lbl:");
  dummy ();
  asm (".global foo_end_lbl\nfoo_end_lbl:");
  return 0;
}

int
bar (struct t x, struct t y, struct t z)
{
  asm (".global bar_start_lbl\nbar_start_lbl:");
  dummy ();
  asm (".global bar_end_lbl\nbar_end_lbl:");
  return 0;
}

int
main (void)
{
  struct s v = { 0, 1, 2 };
  struct t w = { 5, 0, 1, 0, 1, 0, 1, 0, 1, 7 };
  int ans;

  ans = foo (v, v, v);

  end ();

  ans = bar (w, w, w);

  end ();

  return ans;
}
