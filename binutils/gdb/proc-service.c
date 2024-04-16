/* <proc_service.h> implementation.

   Copyright (C) 1999-2024 Free Software Foundation, Inc.

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

#include "defs.h"

#include "gdbcore.h"
#include "inferior.h"
#include "gdbthread.h"
#include "symtab.h"
#include "target.h"
#include "regcache.h"
#include "objfiles.h"

#include "gdb_proc_service.h"

#include <sys/procfs.h>

/* Prototypes for supply_gregset etc.  */
#include "gregset.h"


/* Helper functions.  */

/* Convert a psaddr_t to a CORE_ADDR.  */

static CORE_ADDR
ps_addr_to_core_addr (psaddr_t addr)
{
  if (current_program_space->exec_bfd ()
      && bfd_get_sign_extend_vma (current_program_space->exec_bfd ()))
    return (intptr_t) addr;
  else
    return (uintptr_t) addr;
}

/* Convert a CORE_ADDR to a psaddr_t.  */

static psaddr_t
core_addr_to_ps_addr (CORE_ADDR addr)
{
  if (current_program_space->exec_bfd ()
      && bfd_get_sign_extend_vma (current_program_space->exec_bfd ()))
    return (psaddr_t) (intptr_t) addr;
  else
    return (psaddr_t) (uintptr_t) addr;
}

/* Transfer LEN bytes of memory between BUF and address ADDR in the
   process specified by PH.  If WRITE, transfer them to the process,
   else transfer them from the process.  Returns PS_OK for success,
   PS_ERR on failure.

   This is a helper function for ps_pdread and ps_pdwrite.  */

static ps_err_e
ps_xfer_memory (const struct ps_prochandle *ph, psaddr_t addr,
		gdb_byte *buf, size_t len, int write)
{
  scoped_restore_current_inferior_for_memory save_inferior (ph->thread->inf);

  CORE_ADDR core_addr = ps_addr_to_core_addr (addr);

  int ret;
  if (write)
    ret = target_write_memory (core_addr, buf, len);
  else
    ret = target_read_memory (core_addr, buf, len);
  return (ret == 0 ? PS_OK : PS_ERR);
}


/* Search for the symbol named NAME within the object named OBJ within
   the target process PH.  If the symbol is found the address of the
   symbol is stored in SYM_ADDR.  */

ps_err_e
ps_pglobal_lookup (struct ps_prochandle *ph, const char *obj,
		   const char *name, psaddr_t *sym_addr)
{
  inferior *inf = ph->thread->inf;

  scoped_restore_current_program_space restore_pspace;

  set_current_program_space (inf->pspace);

  /* FIXME: kettenis/2000-09-03: What should we do with OBJ?  */
  bound_minimal_symbol ms = lookup_minimal_symbol (name, NULL, NULL);
  if (ms.minsym == NULL)
    return PS_NOSYM;

  *sym_addr = core_addr_to_ps_addr (ms.value_address ());
  return PS_OK;
}

/* Read SIZE bytes from the target process PH at address ADDR and copy
   them into BUF.  */

ps_err_e
ps_pdread (struct ps_prochandle *ph, psaddr_t addr, void *buf, size_t size)
{
  return ps_xfer_memory (ph, addr, (gdb_byte *) buf, size, 0);
}

/* Write SIZE bytes from BUF into the target process PH at address ADDR.  */

ps_err_e
ps_pdwrite (struct ps_prochandle *ph, psaddr_t addr,
	    const void *buf, size_t size)
{
  return ps_xfer_memory (ph, addr, (gdb_byte *) buf, size, 1);
}

/* Get a regcache for LWPID using its inferior's "main" architecture,
   which is the register set libthread_db expects to be using.  In
   multi-arch debugging scenarios, the thread's architecture may
   differ from the inferior's "main" architecture.  */

static struct regcache *
get_ps_regcache (struct ps_prochandle *ph, lwpid_t lwpid)
{
  inferior *inf = ph->thread->inf;
  return get_thread_arch_regcache (inf, ptid_t (inf->pid, lwpid),
				   inf->arch ());
}

/* Get the general registers of LWP LWPID within the target process PH
   and store them in GREGSET.  */

ps_err_e
ps_lgetregs (struct ps_prochandle *ph, lwpid_t lwpid, prgregset_t gregset)
{
  struct regcache *regcache = get_ps_regcache (ph, lwpid);

  target_fetch_registers (regcache, -1);
  fill_gregset (regcache, (gdb_gregset_t *) gregset, -1);

  return PS_OK;
}

/* Set the general registers of LWP LWPID within the target process PH
   from GREGSET.  */

ps_err_e
ps_lsetregs (struct ps_prochandle *ph, lwpid_t lwpid, const prgregset_t gregset)
{
  struct regcache *regcache = get_ps_regcache (ph, lwpid);

  supply_gregset (regcache, (const gdb_gregset_t *) gregset);
  target_store_registers (regcache, -1);

  return PS_OK;
}

/* Get the floating-point registers of LWP LWPID within the target
   process PH and store them in FPREGSET.  */

ps_err_e
ps_lgetfpregs (struct ps_prochandle *ph, lwpid_t lwpid,
	       prfpregset_t *fpregset)
{
  struct regcache *regcache = get_ps_regcache (ph, lwpid);

  target_fetch_registers (regcache, -1);
  fill_fpregset (regcache, (gdb_fpregset_t *) fpregset, -1);

  return PS_OK;
}

/* Set the floating-point registers of LWP LWPID within the target
   process PH from FPREGSET.  */

ps_err_e
ps_lsetfpregs (struct ps_prochandle *ph, lwpid_t lwpid,
	       const prfpregset_t *fpregset)
{
  struct regcache *regcache = get_ps_regcache (ph, lwpid);

  supply_fpregset (regcache, (const gdb_fpregset_t *) fpregset);
  target_store_registers (regcache, -1);

  return PS_OK;
}

/* Return overall process id of the target PH.  Special for GNU/Linux
   -- not used on Solaris.  */

pid_t
ps_getpid (struct ps_prochandle *ph)
{
  return ph->thread->ptid.pid ();
}

void _initialize_proc_service ();
void
_initialize_proc_service ()
{
  /* This function solely exists to make sure this module is linked
     into the final binary.  */
}
