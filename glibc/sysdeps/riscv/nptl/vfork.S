/* Copyright (C) 2005, 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <tls.h>
#include "tcb-offsets.h"

/* Save the PID value.  */
#define SAVE_PID \
	lw	t0, PID_OFFSET(tp);		/* Load the saved PID.  */	\
	neg	t0, t0;				/* Negate it.  */		\
	bnez	t0, 1f;				/* If it was zero... */		\
	li	t0, 0x80000000;			/* use 0x80000000 instead.  */	\
1:	sw	t0, PID_OFFSET(tp);		/* Store the temporary PID.  */

/* Restore the old PID value in the parent.  */
#define RESTORE_PID \
	beqz	a0, 1f;				/* If we are the parent... */	\
	lw	t0, PID_OFFSET(tp);		/* Load the saved PID.  */	\
	neg	t0, t0;			 	/* Re-negate it.  */		\
	li	t1, 0x80000000;			/* Load 0x80000000... */	\
	bne	t0, t1, 2f;			/* ... compare against it... */	\
	li	t0, 0;				/* ... use 0 instead.  */	\
2:	sw	t0, PID_OFFSET(tp);		/* Restore the PID.  */		\
1:

#include <sysdeps/unix/sysv/linux/riscv/vfork.S>
