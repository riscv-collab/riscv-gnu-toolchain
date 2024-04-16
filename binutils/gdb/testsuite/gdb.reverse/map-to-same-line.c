/* Copyright 2023-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://urldefense.proofpoint.com/v2/url?u=http-3A__www.gnu.org_licenses_&d=DwIDAg&c=jf_iaSHvJObTbx-siA1ZOg&r=RFEmMkZAk--_wFGN5tkM_A&m=hvslrRyYSFfiB2uOFjd7I62ZBKNJkGFWTdsHWVjwDIkK3MWESdWS4tI89FoblXn9&s=Ety3VhMg8aZcBncPPcPCS5XzUde9hjKVulkt8r7mD2k&e= >.  */

/* The purpose of this test is to create a DWARF line table that contains two
   or more entries for the same line.  When stepping (forwards or backwards),
   GDB should step over the entire line and not just a particular entry in
   the line table.  */

int
main (void)
{     /* TAG: main prologue */
  asm ("main_label: .globl main_label");
  int i = 1, j = 2, k;
  float f1 = 2.0, f2 = 4.1, f3;
  const char *str_1 = "foo", *str_2 = "bar", *str_3;

  asm ("line1: .globl line1");
  k = i; f3 = f1; str_3 = str_1;    /* TAG: line 1 */

  asm ("line2: .globl line2");
  k = j; f3 = f2; str_3 = str_2;    /* TAG: line 2 */

  asm ("line3: .globl line3");
  k = i; f3 = f1; str_3 = str_1;    /* TAG: line 3 */

  asm ("line4: .globl line4");
  k = j; f3 = f2; str_3 = str_2;    /* TAG: line 4 */

  asm ("line5: .globl line5");
  k = i; f3 = f1; str_3 = str_1;    /* TAG: line 5 */

  asm ("line6: .globl line6");
  k = j; f3 = f2; str_3 = str_2;    /* TAG: line 6 */

  asm ("line7: .globl line7");
  k = i; f3 = f1; str_3 = str_1;    /* TAG: line 7 */

  asm ("line8: .globl line8");
  k = j; f3 = f2; str_3 = str_2;    /* TAG: line 8 */

  asm ("main_return: .globl main_return");
  k = j; f3 = f2; str_3 = str_2;    /* TAG: main return */

  asm ("end_of_sequence: .globl end_of_sequence");
  return 0; /* TAG: main return */
}
