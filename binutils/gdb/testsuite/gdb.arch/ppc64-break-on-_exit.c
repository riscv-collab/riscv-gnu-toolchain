/* This file is part of GDB, the GNU debugger.

   Copyright 2021-2024 Free Software Foundation, Inc.

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

/* This file was generated from glibc's 2.31 _exit.c, by doing a glibc build
   on ppc64le-linux, copying the command line, adding -g0 -save-temps and
   reducing the _exit.i file.  */

void _exit (int status);

extern __thread int __libc_errno;

void
_exit (int status)
{
  while (1)
    {
      ({
	long int sc_err __attribute__ ((unused));
	long int sc_ret
	  = ({
	      register long int r0 __asm__ ("r0");
	      register long int r3 __asm__ ("r3");
	      register long int r4 __asm__ ("r4");
	      register long int r5 __asm__ ("r5");
	      register long int r6 __asm__ ("r6");
	      register long int r7 __asm__ ("r7");
	      register long int r8 __asm__ ("r8");
	      long int arg1 = (long int) (status);

	      r0 = 234;

	      extern void __illegally_sized_syscall_arg1 (void);
	      if (__builtin_classify_type (status) != 5 && sizeof (status) > 8)
		__illegally_sized_syscall_arg1 ();

	      r3 = arg1;
	      __asm__ __volatile__ ("sc\n\t" "mfcr  %0\n\t" "0:"
				    : "=&r" (r0), "=&r" (r3), "=&r" (r4),
				      "=&r" (r5), "=&r" (r6), "=&r" (r7),
				      "=&r" (r8) : "0" (r0), "1" (r3)
				    : "r9", "r10", "r11", "r12", "cr0", "ctr", "memory");
	      sc_err = r0;

	      r3;
	    });

	if (((void) (sc_ret), __builtin_expect ((sc_err) & (1 << 28), 0)))
	  {
	    (__libc_errno = ((sc_ret)));
	    sc_ret = -1L;
	  }

	sc_ret;
      });

      ({
	long int sc_err __attribute__ ((unused));
	long int sc_ret
	  = ({
	      register long int r0 __asm__ ("r0");
	      register long int r3 __asm__ ("r3");
	      register long int r4 __asm__ ("r4");
	      register long int r5 __asm__ ("r5");
	      register long int r6 __asm__ ("r6");
	      register long int r7 __asm__ ("r7");
	      register long int r8 __asm__ ("r8");
	      long int arg1 = (long int) (status);

	      r0 = 1;

	      extern void __illegally_sized_syscall_arg1 (void);
	      if (__builtin_classify_type (status) != 5 && sizeof (status) > 8)
		__illegally_sized_syscall_arg1 ();

	      r3 = arg1;
	      __asm__ __volatile__ ("sc\n\t" "mfcr  %0\n\t" "0:"
				    : "=&r" (r0), "=&r" (r3), "=&r" (r4),
				      "=&r" (r5), "=&r" (r6), "=&r" (r7),
				      "=&r" (r8) : "0" (r0), "1" (r3)
				    : "r9", "r10", "r11", "r12", "cr0", "ctr", "memory");
	      sc_err = r0;

	      r3;
	    });

	if (((void) (sc_ret), __builtin_expect ((sc_err) & (1 << 28), 0)))
	  {
	    (__libc_errno = ((sc_ret)));
	    sc_ret = -1L;
	  }

	sc_ret;
      });


      asm (".long 0");
    }
}
