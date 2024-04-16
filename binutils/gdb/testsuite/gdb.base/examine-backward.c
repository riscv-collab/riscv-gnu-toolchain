/* This testcase is part of GDB, the GNU debugger.

   Copyright 2015-2024 Free Software Foundation, Inc.

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

/*
Define TestStrings, TestStringsH, and TestStringsW to test utf8, utf16,
and utf32 strings respectively.
To avoid compile errors due to old compiler mode, we don't use string
literals.  The content of each array is the same as followings:

  const char TestStrings[] = {
      "ABCD"
      "EFGHIJKLMNOPQRSTUVWXYZ\0"
      "\0"
      "\0"
      "\u307B\u3052\u307B\u3052\0"
      "012345678901234567890123456789\0"
      "!!!!!!\0"
  };
*/

unsigned char TestStringsBase[] = {
  /* This is here just to ensure we have a null character before
     TestStrings, to avoid showing garbage when we look for strings
     backwards from TestStrings.  */
  0x0,

  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
  0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
  0x59, 0x5a, 0x00, 0x00, 0x00, 0xe3, 0x81, 0xbb,
  0xe3, 0x81, 0x92, 0xe3, 0x81, 0xbb, 0xe3, 0x81,
  0x92, 0x00, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
  0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
  0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  0x00, 0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x00,
  0x00
};

unsigned char *TestStrings = &TestStringsBase[1];

short TestStringsHBase[] = {
  /* This is here just to ensure we have a null character before
     TestStringsH, to avoid showing garbage when we look for strings
     backwards from TestStringsH.  */
  0x0,

  0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047, 0x0048,
  0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f, 0x0050,
  0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057, 0x0058,
  0x0059, 0x005a, 0x0000, 0x0000, 0x0000, 0x307b, 0x3052, 0x307b,
  0x3052, 0x0000, 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035,
  0x0036, 0x0037, 0x0038, 0x0039, 0x0030, 0x0031, 0x0032, 0x0033,
  0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x0030, 0x0031,
  0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039,
  0x0000, 0x0021, 0x0021, 0x0021, 0x0021, 0x0021, 0x0021, 0x0000,
  0x0000
};

short *TestStringsH = &TestStringsHBase[1];

int TestStringsWBase[] = {
  /* This is here just to ensure we have a null character before
     TestStringsW, to avoid showing garbage when we look for strings
     backwards from TestStringsW.  */
  0x0,

  0x00000041, 0x00000042, 0x00000043, 0x00000044,
  0x00000045, 0x00000046, 0x00000047, 0x00000048,
  0x00000049, 0x0000004a, 0x0000004b, 0x0000004c,
  0x0000004d, 0x0000004e, 0x0000004f, 0x00000050,
  0x00000051, 0x00000052, 0x00000053, 0x00000054,
  0x00000055, 0x00000056, 0x00000057, 0x00000058,
  0x00000059, 0x0000005a, 0x00000000, 0x00000000,
  0x00000000, 0x0000307b, 0x00003052, 0x0000307b,
  0x00003052, 0x00000000, 0x00000030, 0x00000031,
  0x00000032, 0x00000033, 0x00000034, 0x00000035,
  0x00000036, 0x00000037, 0x00000038, 0x00000039,
  0x00000030, 0x00000031, 0x00000032, 0x00000033,
  0x00000034, 0x00000035, 0x00000036, 0x00000037,
  0x00000038, 0x00000039, 0x00000030, 0x00000031,
  0x00000032, 0x00000033, 0x00000034, 0x00000035,
  0x00000036, 0x00000037, 0x00000038, 0x00000039,
  0x00000000, 0x00000021, 0x00000021, 0x00000021,
  0x00000021, 0x00000021, 0x00000021, 0x00000000,
  0x00000000
};

int *TestStringsW = &TestStringsWBase[1];

int
main (void)
{
  /* Clang++ eliminates the variables if nothing references them.  */
  int dummy = TestStrings[0] + TestStringsH[0] + TestStringsW[0];

  /* Backward disassemble test requires at least 20 instructions in
     this function.  Adding a simple bubble sort.  */
  int i, j;
  int n[] = {3, 1, 4, 1, 5, 9};
  int len = sizeof (n) / sizeof (n[0]);

  for (i = 0; i < len - 1; ++i)
    {
      for (j = i; j < len; ++j)
        {
          if (n[j] < n[i])
            {
              int tmp = n[i];
              n[i] = n[j];
              n[j] = tmp;
            }
        }
    }
  return 42;
}
