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

#include <cpuid.h>
#include <stdint.h>

/* 0 if the CPU supports rdrand and non-zero otherwise.  */
static unsigned int supports_rdrand;

/* 0 if the CPU supports rdseed and non-zero otherwise.  */
static unsigned int supports_rdseed;

/* Check supported features and set globals accordingly.  The globals
   can be used to prevent unsupported tests from running.  */

static void
check_supported_features (void)
{
  unsigned int rdrand_mask = (1 << 30);
  unsigned int rdseed_mask = (1 << 18);
  unsigned int eax, ebx, ecx, edx;
  unsigned int vendor;
  unsigned int max_level;

  max_level = __get_cpuid_max (0, &vendor);

  if (max_level < 1)
    return;

  __cpuid (1, eax, ebx, ecx, edx);

  supports_rdrand = ((ecx & rdrand_mask) == rdrand_mask);

  if (max_level >= 7)
    {
      __cpuid_count (7, 0, eax, ebx, ecx, edx);
      supports_rdseed = ((ebx & rdseed_mask) == rdseed_mask);
    }
}

/* Test rdrand support for various output registers.  */

void
rdrand (void)
{
  /* Get a random number from the rdrand assembly instruction.  */
  register uint64_t number;

  if (!supports_rdrand)
    return;

  /* 16-bit random numbers.  */
  __asm__ volatile ("rdrand %%ax;": : : "%ax");
  __asm__ volatile ("rdrand %%bx;": : : "%bx");
  __asm__ volatile ("rdrand %%cx;": : : "%cx");
  __asm__ volatile ("rdrand %%dx;": : : "%dx");

  __asm__ volatile ("mov %%di, %%ax;\n\
		     rdrand %%di;\n\
		     mov %%ax, %%di;" : : : "%ax");

  __asm__ volatile ("mov %%si, %%ax;\n\
		     rdrand %%si;\n\
		     mov %%ax, %%si;" : : : "%ax");

  __asm__ volatile ("mov %%bp, %%ax;\n\
		     rdrand %%bp;\n\
		     mov %%ax, %%bp;" : : : "%ax");

  __asm__ volatile ("mov %%sp, %%ax;\n\
		     rdrand %%sp;\n\
		     mov %%ax, %%sp;" : : : "%ax");

#ifdef __x86_64__
  __asm__ volatile ("rdrand %%r8w;": : : "%r8");
  __asm__ volatile ("rdrand %%r9w;": : : "%r9");
  __asm__ volatile ("rdrand %%r10w;": : : "%r10");
  __asm__ volatile ("rdrand %%r11w;": : : "%r11");
  __asm__ volatile ("rdrand %%r12w;": : : "%r12");
  __asm__ volatile ("rdrand %%r13w;": : : "%r13");
  __asm__ volatile ("rdrand %%r14w;": : : "%r14");
  __asm__ volatile ("rdrand %%r15w;": : : "%r15");
#endif

  /* 32-bit random numbers.  */
  __asm__ volatile ("rdrand %%eax;": : : "%eax");
  __asm__ volatile ("rdrand %%ebx;": : : "%ebx");
  __asm__ volatile ("rdrand %%ecx;": : : "%ecx");
  __asm__ volatile ("rdrand %%edx;": : : "%edx");

#ifdef __x86_64__
  __asm__ volatile ("mov %%rdi, %%rax;\n\
		     rdrand %%edi;\n\
		     mov %%rax, %%rdi;" : : : "%rax");

  __asm__ volatile ("mov %%rsi, %%rax;\n\
		     rdrand %%esi;\n\
		     mov %%rax, %%rsi;" : : : "%rax");

  __asm__ volatile ("mov %%rbp, %%rax;\n\
		     rdrand %%ebp;\n\
		     mov %%rax, %%rbp;" : : : "%rax");

  __asm__ volatile ("mov %%rsp, %%rax;\n\
		     rdrand %%esp;\n\
		     mov %%rax, %%rsp;" : : : "%rax");

  __asm__ volatile ("rdrand %%r8d;": : : "%r8");
  __asm__ volatile ("rdrand %%r9d;": : : "%r9");
  __asm__ volatile ("rdrand %%r10d;": : : "%r10");
  __asm__ volatile ("rdrand %%r11d;": : : "%r11");
  __asm__ volatile ("rdrand %%r12d;": : : "%r12");
  __asm__ volatile ("rdrand %%r13d;": : : "%r13");
  __asm__ volatile ("rdrand %%r14d;": : : "%r14");
  __asm__ volatile ("rdrand %%r15d;": : : "%r15");

  /* 64-bit random numbers.  */
  __asm__ volatile ("rdrand %%rax;": : : "%rax");
  __asm__ volatile ("rdrand %%rbx;": : : "%rbx");
  __asm__ volatile ("rdrand %%rcx;": : : "%rcx");
  __asm__ volatile ("rdrand %%rdx;": : : "%rdx");

  __asm__ volatile ("mov %%rdi, %%rax;\n\
		     rdrand %%rdi;\n\
		     mov %%rax, %%rdi;" : : : "%rax");

  __asm__ volatile ("mov %%rsi, %%rax;\n\
		     rdrand %%rsi;\n\
		     mov %%rax, %%rsi;" : : : "%rax");

  __asm__ volatile ("mov %%rbp, %%rax;\n\
		     rdrand %%rbp;\n\
		     mov %%rax, %%rbp;" : : : "%rax");

  __asm__ volatile ("mov %%rsp, %%rax;\n\
		     rdrand %%rsp;\n\
		     mov %%rax, %%rsp;" : : : "%rax");

  __asm__ volatile ("rdrand %%r8;": : : "%r8");
  __asm__ volatile ("rdrand %%r9;": : : "%r9");
  __asm__ volatile ("rdrand %%r10;": : : "%r10");
  __asm__ volatile ("rdrand %%r11;": : : "%r11");
  __asm__ volatile ("rdrand %%r12;": : : "%r12");
  __asm__ volatile ("rdrand %%r13;": : : "%r13");
  __asm__ volatile ("rdrand %%r14;": : : "%r14");
  __asm__ volatile ("rdrand %%r15;": : : "%r15");
#endif
}

/* Test rdseed support for various output registers.  */

void
rdseed (void)
{
  /* Get a random seed from the rdseed assembly instruction.  */
  register long seed;

  if (!supports_rdseed)
    return;

  /* 16-bit random seeds.  */
  __asm__ volatile ("rdseed %%ax;": : : "%ax");
  __asm__ volatile ("rdseed %%bx;": : : "%bx");
  __asm__ volatile ("rdseed %%cx;": : : "%cx");
  __asm__ volatile ("rdseed %%dx;": : : "%dx");

  __asm__ volatile ("mov %%di, %%ax;\n\
		     rdseed %%di;\n\
		     mov %%ax, %%di;" : : : "%ax");

  __asm__ volatile ("mov %%si, %%ax;\n\
		     rdseed %%si;\n\
		     mov %%ax, %%si;" : : : "%ax");

  __asm__ volatile ("mov %%bp, %%ax;\n\
		     rdseed %%bp;\n\
		     mov %%ax, %%bp;" : : : "%ax");

  __asm__ volatile ("mov %%sp, %%ax;\n\
		     rdseed %%sp;\n\
		     mov %%ax, %%sp;" : : : "%ax");

#ifdef __x86_64__
  __asm__ volatile ("rdseed %%r8w;": : : "%r8");
  __asm__ volatile ("rdseed %%r9w;": : : "%r9");
  __asm__ volatile ("rdseed %%r10w;": : : "%r10");
  __asm__ volatile ("rdseed %%r11w;": : : "%r11");
  __asm__ volatile ("rdseed %%r12w;": : : "%r12");
  __asm__ volatile ("rdseed %%r13w;": : : "%r13");
  __asm__ volatile ("rdseed %%r14w;": : : "%r14");
  __asm__ volatile ("rdseed %%r15w;": : : "%r15");
#endif

  /* 32-bit random seeds.  */
  __asm__ volatile ("rdseed %%eax;": : : "%eax");
  __asm__ volatile ("rdseed %%ebx;": : : "%ebx");
  __asm__ volatile ("rdseed %%ecx;": : : "%ecx");
  __asm__ volatile ("rdseed %%edx;": : : "%edx");

#ifdef __x86_64__
  __asm__ volatile ("mov %%rdi, %%rax;\n\
		     rdseed %%edi;\n\
		     mov %%rax, %%rdi;" : : : "%rax");

  __asm__ volatile ("mov %%rsi, %%rax;\n\
		     rdseed %%esi;\n\
		     mov %%rax, %%rsi;" : : : "%rax");

  __asm__ volatile ("mov %%rbp, %%rax;\n\
		     rdseed %%ebp;\n\
		     mov %%rax, %%rbp;" : : : "%rax");

  __asm__ volatile ("mov %%rsp, %%rax;\n\
		     rdseed %%esp;\n\
		     mov %%rax, %%rsp;" : : : "%rax");

  __asm__ volatile ("rdseed %%r8d;": : : "%r8");
  __asm__ volatile ("rdseed %%r9d;": : : "%r9");
  __asm__ volatile ("rdseed %%r10d;": : : "%r10");
  __asm__ volatile ("rdseed %%r11d;": : : "%r11");
  __asm__ volatile ("rdseed %%r12d;": : : "%r12");
  __asm__ volatile ("rdseed %%r13d;": : : "%r13");
  __asm__ volatile ("rdseed %%r14d;": : : "%r14");
  __asm__ volatile ("rdseed %%r15d;": : : "%r15");

  /* 64-bit random seeds.  */
  __asm__ volatile ("rdseed %%rax;": : : "%rax");
  __asm__ volatile ("rdseed %%rbx;": : : "%rbx");
  __asm__ volatile ("rdseed %%rcx;": : : "%rcx");
  __asm__ volatile ("rdseed %%rdx;": : : "%rdx");

  __asm__ volatile ("mov %%rdi, %%rax;\n\
		     rdseed %%rdi;\n\
		     mov %%rax, %%rdi;" : : : "%rax");

  __asm__ volatile ("mov %%rsi, %%rax;\n\
		     rdseed %%rsi;\n\
		     mov %%rax, %%rsi;" : : : "%rax");

  __asm__ volatile ("mov %%rbp, %%rax;\n\
		     rdseed %%rbp;\n\
		     mov %%rax, %%rbp;" : : : "%rax");

  __asm__ volatile ("mov %%rsp, %%rax;\n\
		     rdseed %%rsp;\n\
		     mov %%rax, %%rsp;" : : : "%rax");

  __asm__ volatile ("rdseed %%r8;": : : "%r8");
  __asm__ volatile ("rdseed %%r9;": : : "%r9");
  __asm__ volatile ("rdseed %%r10;": : : "%r10");
  __asm__ volatile ("rdseed %%r11;": : : "%r11");
  __asm__ volatile ("rdseed %%r12;": : : "%r12");
  __asm__ volatile ("rdseed %%r13;": : : "%r13");
  __asm__ volatile ("rdseed %%r14;": : : "%r14");
  __asm__ volatile ("rdseed %%r15;": : : "%r15");
#endif
}

/* Test rdtscp support.  */

void
rdtscp (void)
{
#ifdef __x86_64__
  __asm__ volatile ("rdtscp");
#endif
}

/* Initialize arch-specific bits.  */

static void
initialize (void)
{
  /* Initialize supported features.  */
  check_supported_features ();
}

/* Functions testing instruction decodings.  GDB will test all of these.  */
static testcase_ftype testcases[] =
{
  rdrand,
  rdseed,
  rdtscp
};
