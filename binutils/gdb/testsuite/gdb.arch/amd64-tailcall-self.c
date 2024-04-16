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

__attribute__((noinline,noclone)) static void b(void) {
  asm volatile("");
}

int i;

__attribute__((noinline,noclone)) int c(int q) {
  return q+1;
}

__attribute__((noinline,noclone)) void a(int q) {
  asm volatile("nop;nop;nop"::"g"(q):"memory");
  if (i>=1&&i<=10) {
    i|=1;
    a(i);
  } else if (i==0)
    b();
}

int main(int argc,char **argv) {
  a(argc);
  return 0;
}
