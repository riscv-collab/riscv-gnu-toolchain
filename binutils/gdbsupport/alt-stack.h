/* Temporarily install an alternate signal stack

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef GDBSUPPORT_ALT_STACK_H
#define GDBSUPPORT_ALT_STACK_H

#include <signal.h>

namespace gdb
{

/* Try to set up an alternate signal stack for SIGSEGV handlers.
   This allows us to handle SIGSEGV signals generated when the
   normal process stack is exhausted.  If this stack is not set
   up (sigaltstack is unavailable or fails) and a SIGSEGV is
   generated when the normal stack is exhausted then the program
   will behave as though no SIGSEGV handler was installed.  */
class alternate_signal_stack
{
public:
  alternate_signal_stack ()
  {
#ifdef HAVE_SIGALTSTACK
    m_stack.reset ((char *) xmalloc (SIGSTKSZ));

    stack_t stack;
    stack.ss_sp = m_stack.get ();
    stack.ss_size = SIGSTKSZ;
    stack.ss_flags = 0;

    sigaltstack (&stack, &m_old_stack);
#endif
  }

  ~alternate_signal_stack ()
  {
#ifdef HAVE_SIGALTSTACK
    sigaltstack (&m_old_stack, nullptr);
#endif
  }

  DISABLE_COPY_AND_ASSIGN (alternate_signal_stack);

private:

#ifdef HAVE_SIGALTSTACK
  gdb::unique_xmalloc_ptr<char> m_stack;
  stack_t m_old_stack;
#endif
};

}

#endif /* GDBSUPPORT_ALT_STACK_H */
