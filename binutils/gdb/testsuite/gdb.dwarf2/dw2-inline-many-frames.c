/* Copyright 2019-2024 Free Software Foundation, Inc.

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

/* This test sets up a call stack that looks like this:

   #11     #10    #9     #8     #7     #6     #5     #4     #3     #2     #1     #0
   main -> aaa -> bbb -> ccc -> ddd -> eee -> fff -> ggg -> hhh -> iii -> jjj -> kkk
   \_______________________/    \________/    \______________________/    \________/
      Inline sequence #1          Normal          Inline sequence #2        Normal

   We use the 'start' command to move into main, after that we 'step'
   through each function until we are in kkk.  We then use the 'up' command
   to look back at each from to main.

   The test checks that we can handle and step through sequences of more
   than one inline frame (so 'main .... ccc', and 'fff .... iii'), and also
   that we can move around in a stack that contains more than one disjoint
   sequence of inline frames.

   The order of the functions in this file is deliberately mixed up so that
   the line numbers are not "all ascending" or "all descending" in the line
   table.  */

#define INLINE_FUNCTION __attribute__ ((always_inline)) static inline
#define NON_INLINE_FUNCTION __attribute__ ((noinline))

volatile int global_var = 0;

INLINE_FUNCTION int aaa ();
INLINE_FUNCTION int bbb ();
INLINE_FUNCTION int ccc ();

NON_INLINE_FUNCTION int ddd ();
NON_INLINE_FUNCTION int eee ();
NON_INLINE_FUNCTION int fff ();

INLINE_FUNCTION int ggg ();
INLINE_FUNCTION int hhh ();
INLINE_FUNCTION int iii ();

NON_INLINE_FUNCTION int jjj ();
NON_INLINE_FUNCTION int kkk ();

INLINE_FUNCTION int
aaa ()
{						/* aaa prologue */
  asm ("aaa_label: .globl aaa_label");
  return bbb () + 1;				/* aaa return */
}						/* aaa end */

NON_INLINE_FUNCTION int
jjj ()
{						/* jjj prologue */
  int ans;
  asm ("jjj_label: .globl jjj_label");
  ans = kkk () + 1;				/* jjj return */
  asm ("jjj_label2: .globl jjj_label2");
  return ans;
}						/* jjj end */

INLINE_FUNCTION int
ggg ()
{						/* ggg prologue */
  asm ("ggg_label: .globl ggg_label");
  return hhh () + 1;				/* ggg return */
}						/* ggg end */

INLINE_FUNCTION int
ccc ()
{						/* ccc prologue */
  asm ("ccc_label: .globl ccc_label");
  return ddd () + 1;				/* ccc return */
}						/* ccc end */

NON_INLINE_FUNCTION int
fff ()
{						/* fff prologue */
  int ans;
  asm ("fff_label: .globl fff_label");
  ans = ggg () + 1;				/* fff return */
  asm ("fff_label2: .globl fff_label2");
  return ans;
}						/* fff end */

NON_INLINE_FUNCTION int
kkk ()
{						/* kkk prologue */
  asm ("kkk_label: .globl kkk_label");
  return global_var;				/* kkk return */
}						/* kkk end */

INLINE_FUNCTION int
bbb ()
{						/* bbb prologue */
  asm ("bbb_label: .globl bbb_label");
  return ccc () + 1;				/* bbb return */
}						/* bbb end */

INLINE_FUNCTION int
hhh ()
{						/* hhh prologue */
  asm ("hh_label: .globl hhh_label");
  return iii () + 1;				/* hhh return */
}						/* hhh end */

int
main ()
{						/* main prologue */
  int ans;
  asm ("main_label: .globl main_label");
  global_var = 0;				/* main set global_var */
  asm ("main_label2: .globl main_label2");
  ans = aaa () + 1;				/* main call aaa */
  asm ("main_label3: .globl main_label3");
  return ans;
}						/* main end */

NON_INLINE_FUNCTION int
ddd ()
{						/* ddd prologue */
  int ans;
  asm ("ddd_label: .globl ddd_label");
  ans =  eee () + 1;				/* ddd return */
  asm ("ddd_label2: .globl ddd_label2");
  return ans;
}						/* ddd end */

INLINE_FUNCTION int
iii ()
{						/* iii prologue */
  int ans;
  asm ("iii_label: .globl iii_label");
  ans = jjj () + 1;				/* iii return */
  asm ("iii_label2: .globl iii_label2");
  return ans;
}						/* iii end */

NON_INLINE_FUNCTION int
eee ()
{						/* eee prologue */
  int ans;
  asm ("eee_label: .globl eee_label");
  ans = fff () + 1;				/* eee return */
  asm ("eee_label2: .globl eee_label2");
  return ans;
}						/* eee end */
