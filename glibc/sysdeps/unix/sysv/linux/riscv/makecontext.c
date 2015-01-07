#include <sysdep.h>
#include <sys/asm.h>
#include <sys/ucontext.h>
#include <stdarg.h>
#include <assert.h>

void __makecontext (ucontext_t *ucp, void (*func) (void), int argc,
		    long a0, long a1, long a2, long a3, long a4, ...)
{
  extern void __start_context(void) attribute_hidden;
  long i, sp;

  assert(REG_NARGS == 8);

  /* Set up the stack. */
  sp = ((long)ucp->uc_stack.ss_sp + ucp->uc_stack.ss_size) & ALMASK;

  /* Set up the register context.
     ra = s0 = 0, terminating the stack for backtracing purposes.
     s1 = the function we must call.
     s2 = the subsequent context to run.  */
  ucp->uc_mcontext.gregs[REG_RA] = 0;
  ucp->uc_mcontext.gregs[REG_S0 + 0] = 0;
  ucp->uc_mcontext.gregs[REG_S0 + 1] = (long)func;
  ucp->uc_mcontext.gregs[REG_S0 + 2] = (long)ucp->uc_link;
  ucp->uc_mcontext.gregs[REG_SP] = sp;
  ucp->uc_mcontext.gregs[REG_PC] = (long)&__start_context;

  /* Put args in a0-a7, then put any remaining args on the stack. */
  ucp->uc_mcontext.gregs[REG_A0 + 0] = a0;
  ucp->uc_mcontext.gregs[REG_A0 + 1] = a1;
  ucp->uc_mcontext.gregs[REG_A0 + 2] = a2;
  ucp->uc_mcontext.gregs[REG_A0 + 3] = a3;
  ucp->uc_mcontext.gregs[REG_A0 + 4] = a4;

  if (__builtin_expect (argc > 5, 0))
    {
      va_list vl;
      va_start(vl, a4);

      long reg_args = argc < REG_NARGS ? argc : REG_NARGS;
      sp = (sp - (argc - reg_args) * sizeof(long)) & ALMASK;
      for (i = 5; i < reg_args; i++)
        ucp->uc_mcontext.gregs[REG_A0 + i] = va_arg(vl, long);
      for (i = 0; i < argc - reg_args; i++)
        ((long*)sp)[i] = va_arg(vl, long);

      va_end(vl);
    }
}

weak_alias (__makecontext, makecontext)
