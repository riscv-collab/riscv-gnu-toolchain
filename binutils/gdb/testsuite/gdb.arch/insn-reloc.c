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

#include <stddef.h>
#include <stdint.h>

typedef void (*testcase_ftype)(void);

/* Each function checks the correctness of the instruction being
   relocated due to a fast tracepoint.  Call function pass if it is
   correct, otherwise call function fail.  GDB sets a breakpoints on
   pass and fail in order to check the correctness.  */

static void
pass (void)
{
}

static void
fail (void)
{
}

#if (defined __x86_64__ || defined __i386__)

#ifdef SYMBOL_PREFIX
#define SYMBOL(str)     SYMBOL_PREFIX #str
#else
#define SYMBOL(str)     #str
#endif

/* Make sure we can relocate a CALL instruction.  CALL instructions are
   5 bytes long so we can always set a fast tracepoints on them.

     JMP set_point0
   f:
     MOV $1, %[ok]
     RET
   set_point0:
     CALL f ; tracepoint here.

   */

static void
can_relocate_call (void)
{
  int ok = 0;

  asm ("    .global " SYMBOL (set_point0) "\n"
       "  jmp " SYMBOL (set_point0) "\n"
       "0:\n"
       "  mov $1, %[ok]\n"
       "  ret\n"
       SYMBOL (set_point0) ":\n"
       "  call 0b\n"
       : [ok] "=r" (ok));

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a JMP instruction.  We need the JMP
   instruction to be 5 bytes long in order to set a fast tracepoint on
   it.  To do this, we emit the opcode directly.

     JMP next ; tracepoint here.
   next:
     MOV $1, %[ok]

   */

static void
can_relocate_jump (void)
{
  int ok = 0;

  asm ("    .global " SYMBOL (set_point1) "\n"
       SYMBOL (set_point1) ":\n"
       ".byte 0xe9\n"  /* jmp  */
       ".byte 0x00\n"
       ".byte 0x00\n"
       ".byte 0x00\n"
       ".byte 0x00\n"
       "  mov $1, %[ok]\n"
       : [ok] "=r" (ok));

  if (ok == 1)
    pass ();
  else
    fail ();
}
#elif (defined __aarch64__)

/* Make sure we can relocate a B instruction.

     B set_point0
   set_ok:
     MOV %[ok], #1
     B end
   set_point0:
     B set_ok ; tracepoint here.
     MOV %[ok], #0
   end

   */

static void
can_relocate_b (void)
{
  int ok = 0;

  asm ("  b set_point0\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point0:\n"
       "  b 0b\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok));

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a B.cond instruction.

     MOV x0, #8
     TST x0, #8 ; Clear the Z flag.
     B set_point1
   set_ok:
     MOV %[ok], #1
     B end
   set_point1:
     B.NE set_ok ; tracepoint here.
     MOV %[ok], #0
   end

   */

static void
can_relocate_bcond_true (void)
{
  int ok = 0;

  asm ("  mov x0, #8\n"
       "  tst x0, #8\n"
       "  b set_point1\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point1:\n"
       "  b.ne 0b\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0", "cc");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a CBZ instruction.

     MOV x0, #0
     B set_point2
   set_ok:
     MOV %[ok], #1
     B end
   set_point2:
     CBZ x0, set_ok ; tracepoint here.
     MOV %[ok], #0
   end

   */

static void
can_relocate_cbz (void)
{
  int ok = 0;

  asm ("  mov x0, #0\n"
       "  b set_point2\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point2:\n"
       "  cbz x0, 0b\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a CBNZ instruction.

     MOV x0, #8
     B set_point3
   set_ok:
     MOV %[ok], #1
     B end
   set_point3:
     CBNZ x0, set_ok ; tracepoint here.
     MOV %[ok], #0
   end

   */

static void
can_relocate_cbnz (void)
{
  int ok = 0;

  asm ("  mov x0, #8\n"
       "  b set_point3\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point3:\n"
       "  cbnz x0, 0b\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a TBZ instruction.

     MOV x0, #8
     MVN x0, x0 ; Clear bit 3.
     B set_point4
   set_ok:
     MOV %[ok], #1
     B end
   set_point4:
     TBZ x0, #3, set_ok ; tracepoint here.
     MOV %[ok], #0
   end

   */

static void
can_relocate_tbz (void)
{
  int ok = 0;

  asm ("  mov x0, #8\n"
       "  mvn x0, x0\n"
       "  b set_point4\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point4:\n"
       "  tbz x0, #3, 0b\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a TBNZ instruction.

     MOV x0, #8 ; Set bit 3.
     B set_point5
   set_ok:
     MOV %[ok], #1
     B end
   set_point5:
     TBNZ x0, #3, set_ok ; tracepoint here.
     MOV %[ok], #0
   end

   */

static void
can_relocate_tbnz (void)
{
  int ok = 0;

  asm ("  mov x0, #8\n"
       "  b set_point5\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point5:\n"
       "  tbnz x0, #3, 0b\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate an ADR instruction with a positive offset.

   set_point6:
     ADR x0, target ; tracepoint here.
     BR x0 ; jump to target
     MOV %[ok], #0
     B end
   target:
     MOV %[ok], #1
   end

   */

static void
can_relocate_adr_forward (void)
{
  int ok = 0;

  asm ("set_point6:\n"
       "  adr x0, 0f\n"
       "  br x0\n"
       "  mov %[ok], #0\n"
       "  b 1f\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate an ADR instruction with a negative offset.

     B set_point7
   target:
     MOV %[ok], #1
     B end
   set_point7:
     ADR x0, target ; tracepoint here.
     BR x0 ; jump to target
     MOV %[ok], #0
   end

   */

static void
can_relocate_adr_backward (void)
{
  int ok = 0;

  asm ("b set_point7\n"
       "0:\n"
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "set_point7:\n"
       "  adr x0, 0b\n"
       "  br x0\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0");

  if (ok == 1)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate an ADRP instruction.

   set_point8:
     ADRP %[addr], set_point8 ; tracepoint here.
     ADR %[pc], set_point8

   ADR computes the address of the given label.  While ADRP gives us its
   page, on a 4K boundary.  We can check ADRP executed normally by
   making sure the result of ADR and ADRP are equivalent, except for the
   12 lowest bits which should be cleared.

   */

static void
can_relocate_adrp (void)
{
  uintptr_t page;
  uintptr_t pc;

  asm ("set_point8:\n"
       "  adrp %[page], set_point8\n"
       "  adr %[pc], set_point8\n"
       : [page] "=r" (page), [pc] "=r" (pc));

  if (page == (pc & ~0xfff))
    pass ();
  else
    fail ();
}

/* Make sure we can relocate an LDR instruction, where the memory to
   read is an offset from the current PC.

     B set_point9
   data:
     .word 0x0cabba9e
   set_point9:
     LDR %[result], data ; tracepoint here.

   */

static void
can_relocate_ldr (void)
{
  uint32_t result = 0;

  asm ("b set_point9\n"
       "0:\n"
       "  .word 0x0cabba9e\n"
       "set_point9:\n"
       "  ldr %w[result], 0b\n"
       : [result] "=r" (result));

  if (result == 0x0cabba9e)
    pass ();
  else
    fail ();
}

/* Make sure we can relocate a B.cond instruction and condition is false.  */

static void
can_relocate_bcond_false (void)
{
  int ok = 0;

  asm ("  mov x0, #8\n"
       "  tst x0, #8\n" /* Clear the Z flag.  */
       "set_point10:\n" /* Set tracepoint here.  */
       "  b.eq 0b\n"    /* Condition is false.  */
       "  mov %[ok], #1\n"
       "  b 1f\n"
       "0:\n"
       "  mov %[ok], #0\n"
       "1:\n"
       : [ok] "=r" (ok)
       :
       : "0", "cc");

  if (ok == 1)
    pass ();
  else
    fail ();
}

static void
foo (void)
{
}

/* Make sure we can relocate a BL instruction.  */

static void
can_relocate_bl (void)
{
  asm ("set_point11:\n"
       "  bl foo\n"
       "  bl pass\n"
       : : : "x30"); /* Test that LR is updated correctly.  */
}

/* Make sure we can relocate a BR instruction.

     ... Set x0 to target
   set_point12:
     BR x0 ; jump to target (tracepoint here).
     fail()
     return
   target:
     pass()
   end

   */

static void
can_relocate_br (void)
{
  int ok = 0;

  asm goto ("  adr x0, %l0\n"
            "set_point12:\n"
            "  br x0\n"
            :
            :
            : "x0"
            : madejump);

  fail ();
  return;
madejump:
  pass ();
}

/* Make sure we can relocate a BLR instruction.

   We use two different functions since the test runner expects one breakpoint
   per function and we want to test two different things.
   For BLR we want to test that the BLR actually jumps to the relevant
   function, *and* that it sets the LR register correctly.

   Hence we create one testcase that jumps to `pass` using BLR, and one
   testcase that jumps to `pass` if BLR has set the LR correctly.

  -- can_relocate_blr_jumps
     ... Set x0 to pass
   set_point13:
     BLR x0        ; jump to pass (tracepoint here).

  -- can_relocate_blr_sets_lr
     ... Set x0 to foo
   set_point14:
     BLR x0        ; jumps somewhere else (tracepoint here).
     BL pass       ; ensures the LR was set correctly by the BLR.

   */

static void
can_relocate_blr_jumps (void)
{
  int ok = 0;

  /* Test BLR indeed jumps to the target.  */
  asm ("set_point13:\n"
       "  blr %[address]\n"
       : : [address] "r" (&pass) : "x30");
}

static void
can_relocate_blr_sets_lr (void)
{
  int ok = 0;

  /* Test BLR sets the LR correctly.  */
  asm ("set_point14:\n"
       "  blr %[address]\n"
       "  bl pass\n"
       : : [address] "r" (&foo) : "x30");
}

#endif

/* Functions testing relocations need to be placed here.  GDB will read
   n_testcases to know how many fast tracepoints to place.  It will look
   for symbols in the form of 'set_point\[0-9\]+' so each functions
   needs one, starting at 0.  */

static testcase_ftype testcases[] = {
#if (defined __x86_64__ || defined __i386__)
  can_relocate_call,
  can_relocate_jump
#elif (defined __aarch64__)
  can_relocate_b,
  can_relocate_bcond_true,
  can_relocate_cbz,
  can_relocate_cbnz,
  can_relocate_tbz,
  can_relocate_tbnz,
  can_relocate_adr_forward,
  can_relocate_adr_backward,
  can_relocate_adrp,
  can_relocate_ldr,
  can_relocate_bcond_false,
  can_relocate_bl,
  can_relocate_br,
  can_relocate_blr_jumps,
  can_relocate_blr_sets_lr,
#endif
};

static size_t n_testcases = (sizeof (testcases) / sizeof (testcase_ftype));

int
main ()
{
  int i = 0;

  for (i = 0; i < n_testcases; i++)
    testcases[i] ();

  return 0;
}
