/* Process record and replay target code for GNU/Linux.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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
#include "target.h"
#include "gdbtypes.h"
#include "regcache.h"
#include "record.h"
#include "record-full.h"
#include "linux-record.h"
#include "gdbarch.h"

/* These macros are the values of the first argument of system call
   "sys_ptrace".  The values of these macros were obtained from Linux
   Kernel source.  */

#define RECORD_PTRACE_PEEKTEXT	1
#define RECORD_PTRACE_PEEKDATA	2
#define RECORD_PTRACE_PEEKUSR	3

/* These macros are the values of the first argument of system call
   "sys_socketcall".  The values of these macros were obtained from
   Linux Kernel source.  */

#define RECORD_SYS_SOCKET	1
#define RECORD_SYS_BIND		2
#define RECORD_SYS_CONNECT	3
#define RECORD_SYS_LISTEN	4
#define RECORD_SYS_ACCEPT	5
#define RECORD_SYS_GETSOCKNAME	6
#define RECORD_SYS_GETPEERNAME	7
#define RECORD_SYS_SOCKETPAIR	8
#define RECORD_SYS_SEND		9
#define RECORD_SYS_RECV		10
#define RECORD_SYS_SENDTO	11
#define RECORD_SYS_RECVFROM	12
#define RECORD_SYS_SHUTDOWN	13
#define RECORD_SYS_SETSOCKOPT	14
#define RECORD_SYS_GETSOCKOPT	15
#define RECORD_SYS_SENDMSG	16
#define RECORD_SYS_RECVMSG	17

/* These macros are the values of the first argument of system call
   "sys_ipc".  The values of these macros were obtained from Linux
   Kernel source.  */

#define RECORD_SEMOP		1
#define RECORD_SEMGET		2
#define RECORD_SEMCTL		3
#define RECORD_SEMTIMEDOP	4
#define RECORD_MSGSND		11
#define RECORD_MSGRCV		12
#define RECORD_MSGGET		13
#define RECORD_MSGCTL		14
#define RECORD_SHMAT		21
#define RECORD_SHMDT		22
#define RECORD_SHMGET		23
#define RECORD_SHMCTL		24

/* These macros are the values of the first argument of system call
   "sys_quotactl".  The values of these macros were obtained from Linux
   Kernel source.  */

#define RECORD_Q_GETFMT		0x800004
#define RECORD_Q_GETINFO	0x800005
#define RECORD_Q_GETQUOTA	0x800007
#define RECORD_Q_XGETQSTAT	(('5' << 8) + 5)
#define RECORD_Q_XGETQUOTA	(('3' << 8) + 3)

#define OUTPUT_REG(val, num)      phex_nz ((val), \
    gdbarch_register_type (regcache->arch (), (num))->length ())

/* Record a memory area of length LEN pointed to by register
   REGNUM.  */

static int
record_mem_at_reg (struct regcache *regcache, int regnum, int len)
{
  ULONGEST addr;

  regcache_raw_read_unsigned (regcache, regnum, &addr);
  return record_full_arch_list_add_mem ((CORE_ADDR) addr, len);
}

static int
record_linux_sockaddr (struct regcache *regcache,
		       struct linux_record_tdep *tdep, ULONGEST addr,
		       ULONGEST len)
{
  gdb_byte *a;
  int addrlen;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  if (!addr)
    return 0;

  a = (gdb_byte *) alloca (tdep->size_int);

  if (record_full_arch_list_add_mem ((CORE_ADDR) len, tdep->size_int))
    return -1;

  /* Get the addrlen.  */
  if (target_read_memory ((CORE_ADDR) len, a, tdep->size_int))
    {
      if (record_debug)
	gdb_printf (gdb_stdlog,
		    "Process record: error reading "
		    "memory at addr = 0x%s len = %d.\n",
		    phex_nz (len, tdep->size_pointer),
		    tdep->size_int);
      return -1;
    }
  addrlen = (int) extract_unsigned_integer (a, tdep->size_int, byte_order);
  if (addrlen <= 0 || addrlen > tdep->size_sockaddr)
    addrlen = tdep->size_sockaddr;

  if (record_full_arch_list_add_mem ((CORE_ADDR) addr, addrlen))
    return -1;

  return 0;
}

static int
record_linux_msghdr (struct regcache *regcache,
		     struct linux_record_tdep *tdep, ULONGEST addr)
{
  gdb_byte *a;
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  CORE_ADDR tmpaddr;
  int tmpint;

  if (!addr)
    return 0;

  if (record_full_arch_list_add_mem ((CORE_ADDR) addr, tdep->size_msghdr))
    return -1;

  a = (gdb_byte *) alloca (tdep->size_msghdr);
  if (target_read_memory ((CORE_ADDR) addr, a, tdep->size_msghdr))
    {
      if (record_debug)
	gdb_printf (gdb_stdlog,
		    "Process record: error reading "
		    "memory at addr = 0x%s "
		    "len = %d.\n",
		    phex_nz (addr, tdep->size_pointer),
		    tdep->size_msghdr);
      return -1;
    }

  /* msg_name msg_namelen */
  addr = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
  a += tdep->size_pointer;
  if (record_full_arch_list_add_mem
      ((CORE_ADDR) addr,
       (int) extract_unsigned_integer (a,
				       tdep->size_int,
				       byte_order)))
    return -1;
  /* We have read an int, but skip size_pointer bytes to account for alignment
     of the next field on 64-bit targets. */
  a += tdep->size_pointer;

  /* msg_iov msg_iovlen */
  addr = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
  a += tdep->size_pointer;
  if (addr)
    {
      ULONGEST i;
      ULONGEST len = extract_unsigned_integer (a, tdep->size_size_t,
					       byte_order);
      gdb_byte *iov = (gdb_byte *) alloca (tdep->size_iovec);

      for (i = 0; i < len; i++)
	{
	  if (target_read_memory ((CORE_ADDR) addr, iov, tdep->size_iovec))
	    {
	      if (record_debug)
		gdb_printf (gdb_stdlog,
			    "Process record: error "
			    "reading memory at "
			    "addr = 0x%s "
			    "len = %d.\n",
			    phex_nz (addr,tdep->size_pointer),
			    tdep->size_iovec);
	      return -1;
	    }
	  tmpaddr = (CORE_ADDR) extract_unsigned_integer (iov,
							  tdep->size_pointer,
							  byte_order);
	  tmpint = (int) extract_unsigned_integer (iov + tdep->size_pointer,
						   tdep->size_size_t,
						   byte_order);
	  if (record_full_arch_list_add_mem (tmpaddr, tmpint))
	    return -1;
	  addr += tdep->size_iovec;
	}
    }
  a += tdep->size_size_t;

  /* msg_control msg_controllen */
  addr = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
  a += tdep->size_pointer;
  tmpint = (int) extract_unsigned_integer (a, tdep->size_size_t, byte_order);
  if (record_full_arch_list_add_mem ((CORE_ADDR) addr, tmpint))
    return -1;

  return 0;
}

/* When the architecture process record get a Linux syscall
   instruction, it will get a Linux syscall number of this
   architecture and convert it to the Linux syscall number "num" which
   is internal to GDB.  Most Linux syscalls across architectures in
   Linux would be similar and mostly differ by sizes of types and
   structures.  This sizes are put to "tdep".

   Record the values of the registers and memory that will be changed
   in current system call.

   Return -1 if something wrong.  */

int
record_linux_system_call (enum gdb_syscall syscall,
			  struct regcache *regcache,
			  struct linux_record_tdep *tdep)
{
  struct gdbarch *gdbarch = regcache->arch ();
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  ULONGEST tmpulongest;
  CORE_ADDR tmpaddr;
  int tmpint;

  switch (syscall)
    {
    case gdb_sys_restart_syscall:
      break;

    case gdb_sys_exit:
      if (yquery (_("The next instruction is syscall exit.  "
		    "It will make the program exit.  "
		    "Do you want to stop the program?")))
	return 1;
      break;

    case gdb_sys_fork:
      break;

    case gdb_sys_read:
    case gdb_sys_readlink:
    case gdb_sys_recv:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (record_mem_at_reg (regcache, tdep->arg2, (int) tmpulongest))
	return -1;
      break;

    case gdb_sys_write:
    case gdb_sys_open:
    case gdb_sys_close:
      break;

    case gdb_sys_waitpid:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					   tdep->size_int))
	  return -1;
      break;

    case gdb_sys_creat:
    case gdb_sys_link:
    case gdb_sys_unlink:
    case gdb_sys_execve:
    case gdb_sys_chdir:
      break;

    case gdb_sys_time:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest)
	if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					   tdep->size_time_t))
	  return -1;
      break;

    case gdb_sys_mknod:
    case gdb_sys_chmod:
    case gdb_sys_lchown16:
    case gdb_sys_ni_syscall17:
      break;

    case gdb_sys_stat:
    case gdb_sys_fstat:
    case gdb_sys_lstat:
      if (record_mem_at_reg (regcache, tdep->arg2,
			     tdep->size__old_kernel_stat))
	return -1;
      break;

    case gdb_sys_lseek:
    case gdb_sys_getpid:
    case gdb_sys_mount:
    case gdb_sys_oldumount:
    case gdb_sys_setuid16:
    case gdb_sys_getuid16:
    case gdb_sys_stime:
      break;

    case gdb_sys_ptrace:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest == RECORD_PTRACE_PEEKTEXT
	  || tmpulongest == RECORD_PTRACE_PEEKDATA
	  || tmpulongest == RECORD_PTRACE_PEEKUSR)
	{
	  if (record_mem_at_reg (regcache, tdep->arg4, 4))
	    return -1;
	}
      break;

    case gdb_sys_alarm:
    case gdb_sys_pause:
    case gdb_sys_utime:
    case gdb_sys_ni_syscall31:
    case gdb_sys_ni_syscall32:
    case gdb_sys_access:
    case gdb_sys_nice:
    case gdb_sys_ni_syscall35:
    case gdb_sys_sync:
    case gdb_sys_kill:
    case gdb_sys_rename:
    case gdb_sys_mkdir:
    case gdb_sys_rmdir:
    case gdb_sys_dup:
      break;

    case gdb_sys_pipe:
    case gdb_sys_pipe2:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_int * 2))
	return -1;
      break;

    case gdb_sys_getrandom:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (record_mem_at_reg (regcache, tdep->arg1, tmpulongest))
	return -1;
      break;

    case gdb_sys_times:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_tms))
	return -1;
      break;

    case gdb_sys_ni_syscall44:
    case gdb_sys_brk:
    case gdb_sys_setgid16:
    case gdb_sys_getgid16:
    case gdb_sys_signal:
    case gdb_sys_geteuid16:
    case gdb_sys_getegid16:
    case gdb_sys_acct:
    case gdb_sys_umount:
    case gdb_sys_ni_syscall53:
      break;

    case gdb_sys_ioctl:
      /* XXX Need to add a lot of support of other ioctl requests.  */
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest == tdep->ioctl_FIOCLEX
	  || tmpulongest == tdep->ioctl_FIONCLEX
	  || tmpulongest == tdep->ioctl_FIONBIO
	  || tmpulongest == tdep->ioctl_FIOASYNC
	  || tmpulongest == tdep->ioctl_TCSETS
	  || tmpulongest == tdep->ioctl_TCSETSW
	  || tmpulongest == tdep->ioctl_TCSETSF
	  || tmpulongest == tdep->ioctl_TCSETA
	  || tmpulongest == tdep->ioctl_TCSETAW
	  || tmpulongest == tdep->ioctl_TCSETAF
	  || tmpulongest == tdep->ioctl_TCSBRK
	  || tmpulongest == tdep->ioctl_TCXONC
	  || tmpulongest == tdep->ioctl_TCFLSH
	  || tmpulongest == tdep->ioctl_TIOCEXCL
	  || tmpulongest == tdep->ioctl_TIOCNXCL
	  || tmpulongest == tdep->ioctl_TIOCSCTTY
	  || tmpulongest == tdep->ioctl_TIOCSPGRP
	  || tmpulongest == tdep->ioctl_TIOCSTI
	  || tmpulongest == tdep->ioctl_TIOCSWINSZ
	  || tmpulongest == tdep->ioctl_TIOCMBIS
	  || tmpulongest == tdep->ioctl_TIOCMBIC
	  || tmpulongest == tdep->ioctl_TIOCMSET
	  || tmpulongest == tdep->ioctl_TIOCSSOFTCAR
	  || tmpulongest == tdep->ioctl_TIOCCONS
	  || tmpulongest == tdep->ioctl_TIOCSSERIAL
	  || tmpulongest == tdep->ioctl_TIOCPKT
	  || tmpulongest == tdep->ioctl_TIOCNOTTY
	  || tmpulongest == tdep->ioctl_TIOCSETD
	  || tmpulongest == tdep->ioctl_TCSBRKP
	  || tmpulongest == tdep->ioctl_TIOCTTYGSTRUCT
	  || tmpulongest == tdep->ioctl_TIOCSBRK
	  || tmpulongest == tdep->ioctl_TIOCCBRK
	  || tmpulongest == tdep->ioctl_TCSETS2
	  || tmpulongest == tdep->ioctl_TCSETSW2
	  || tmpulongest == tdep->ioctl_TCSETSF2
	  || tmpulongest == tdep->ioctl_TIOCSPTLCK
	  || tmpulongest == tdep->ioctl_TIOCSERCONFIG
	  || tmpulongest == tdep->ioctl_TIOCSERGWILD
	  || tmpulongest == tdep->ioctl_TIOCSERSWILD
	  || tmpulongest == tdep->ioctl_TIOCSLCKTRMIOS
	  || tmpulongest == tdep->ioctl_TIOCSERGETMULTI
	  || tmpulongest == tdep->ioctl_TIOCSERSETMULTI
	  || tmpulongest == tdep->ioctl_TIOCMIWAIT
	  || tmpulongest == tdep->ioctl_TIOCSHAYESESP)
	{
	  /* Nothing to do.  */
	}
      else if (tmpulongest == tdep->ioctl_TCGETS
	       || tmpulongest == tdep->ioctl_TCGETA
	       || tmpulongest == tdep->ioctl_TIOCGLCKTRMIOS)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_termios))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCGPGRP
	       || tmpulongest == tdep->ioctl_TIOCGSID)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_pid_t))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCOUTQ
	       || tmpulongest == tdep->ioctl_TIOCMGET
	       || tmpulongest == tdep->ioctl_TIOCGSOFTCAR
	       || tmpulongest == tdep->ioctl_FIONREAD
	       || tmpulongest == tdep->ioctl_TIOCINQ
	       || tmpulongest == tdep->ioctl_TIOCGETD
	       || tmpulongest == tdep->ioctl_TIOCGPTN
	       || tmpulongest == tdep->ioctl_TIOCSERGETLSR)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_int))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCGWINSZ)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_winsize))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCLINUX)
	{
	  /* This syscall affects a char-size memory.  */
	  if (record_mem_at_reg (regcache, tdep->arg3, 1))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCGSERIAL)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_serial_struct))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TCGETS2)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_termios2))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_FIOQSIZE)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_loff_t))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCGICOUNT)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_serial_icounter_struct))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCGHAYESESP)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_hayes_esp_config))
	    return -1;
	}
      else if (tmpulongest == tdep->ioctl_TIOCSERGSTRUCT)
	{
	  gdb_printf (gdb_stderr,
		      _("Process record and replay target doesn't "
			"support ioctl request TIOCSERGSTRUCT\n"));
	  return 1;
	}
      else
	{
	  gdb_printf (gdb_stderr,
		      _("Process record and replay target doesn't "
			"support ioctl request 0x%s.\n"),
		      OUTPUT_REG (tmpulongest, tdep->arg2));
	  return 1;
	}
      break;

    case gdb_sys_fcntl:
      /* XXX */
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
    sys_fcntl:
      if (tmpulongest == tdep->fcntl_F_GETLK)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_flock))
	    return -1;
	}
      break;

    case gdb_sys_ni_syscall56:
    case gdb_sys_setpgid:
    case gdb_sys_ni_syscall58:
      break;

    case gdb_sys_olduname:
      if (record_mem_at_reg (regcache, tdep->arg1,
			     tdep->size_oldold_utsname))
	return -1;
      break;

    case gdb_sys_umask:
    case gdb_sys_chroot:
      break;

    case gdb_sys_ustat:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_ustat))
	return -1;
      break;

    case gdb_sys_dup2:
    case gdb_sys_getppid:
    case gdb_sys_getpgrp:
    case gdb_sys_setsid:
      break;

    case gdb_sys_sigaction:
      if (record_mem_at_reg (regcache, tdep->arg3,
			     tdep->size_old_sigaction))
	return -1;
      break;

    case gdb_sys_sgetmask:
    case gdb_sys_ssetmask:
    case gdb_sys_setreuid16:
    case gdb_sys_setregid16:
    case gdb_sys_sigsuspend:
      break;

    case gdb_sys_sigpending:
      if (record_mem_at_reg (regcache, tdep->arg1,
			     tdep->size_old_sigset_t))
	return -1;
      break;

    case gdb_sys_sethostname:
    case gdb_sys_setrlimit:
      break;

    case gdb_sys_old_getrlimit:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_rlimit))
	return -1;
      break;

    case gdb_sys_getrusage:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_rusage))
	return -1;
      break;

    case gdb_sys_gettimeofday:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_timeval)
	  || record_mem_at_reg (regcache, tdep->arg2, tdep->size_timezone))
	return -1;
      break;

    case gdb_sys_settimeofday:
      break;

    case gdb_sys_getgroups16:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST gidsetsize;

	  regcache_raw_read_unsigned (regcache, tdep->arg1,
				      &gidsetsize);
	  tmpint = tdep->size_old_gid_t * (int) gidsetsize;
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest, tmpint))
	    return -1;
	}
      break;

    case gdb_sys_setgroups16:
      break;

    case gdb_old_select:
      {
	unsigned long sz_sel_arg = tdep->size_long + tdep->size_pointer * 4;
	gdb_byte *a = (gdb_byte *) alloca (sz_sel_arg);
	CORE_ADDR inp, outp, exp, tvp;

	regcache_raw_read_unsigned (regcache, tdep->arg1,
				    &tmpulongest);
	if (tmpulongest)
	  {
	    if (target_read_memory (tmpulongest, a, sz_sel_arg))
	      {
		if (record_debug)
		  gdb_printf (gdb_stdlog,
			      "Process record: error reading memory "
			      "at addr = 0x%s len = %lu.\n",
			      OUTPUT_REG (tmpulongest, tdep->arg1),
			      sz_sel_arg);
		return -1;
	      }
	    /* Skip n. */
	    a += tdep->size_long;
	    inp = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
	    a += tdep->size_pointer;
	    outp = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
	    a += tdep->size_pointer;
	    exp = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
	    a += tdep->size_pointer;
	    tvp = extract_unsigned_integer (a, tdep->size_pointer, byte_order);
	    if (inp)
	      if (record_full_arch_list_add_mem (inp, tdep->size_fd_set))
		return -1;
	    if (outp)
	      if (record_full_arch_list_add_mem (outp, tdep->size_fd_set))
		return -1;
	    if (exp)
	      if (record_full_arch_list_add_mem (exp, tdep->size_fd_set))
		return -1;
	    if (tvp)
	      if (record_full_arch_list_add_mem (tvp, tdep->size_timeval))
		return -1;
	  }
      }
      break;

    case gdb_sys_symlink:
      break;

    case gdb_sys_uselib:
    case gdb_sys_swapon:
      break;

    case gdb_sys_reboot:
      if (yquery (_("The next instruction is syscall reboot.  "
		    "It will restart the computer.  "
		    "Do you want to stop the program?")))
	return 1;
      break;

    case gdb_old_readdir:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_old_dirent))
	return -1;
      break;

    case gdb_old_mmap:
      break;

    case gdb_sys_munmap:
      {
	ULONGEST len;

	regcache_raw_read_unsigned (regcache, tdep->arg1,
				    &tmpulongest);
	regcache_raw_read_unsigned (regcache, tdep->arg2, &len);
	if (record_full_memory_query)
	  {
	    if (yquery (_("\
The next instruction is syscall munmap.\n\
It will free the memory addr = 0x%s len = %u.\n\
It will make record target cannot record some memory change.\n\
Do you want to stop the program?"),
			OUTPUT_REG (tmpulongest, tdep->arg1), (int) len))
	      return 1;
	  }
      }
      break;

    case gdb_sys_truncate:
    case gdb_sys_ftruncate:
    case gdb_sys_fchmod:
    case gdb_sys_fchown16:
    case gdb_sys_getpriority:
    case gdb_sys_setpriority:
    case gdb_sys_ni_syscall98:
      break;

    case gdb_sys_statfs:
    case gdb_sys_fstatfs:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_statfs))
	return -1;
      break;

    case gdb_sys_ioperm:
      break;

    case gdb_sys_socket:
    case gdb_sys_sendto:
    case gdb_sys_sendmsg:
    case gdb_sys_shutdown:
    case gdb_sys_bind:
    case gdb_sys_connect:
    case gdb_sys_listen:
    case gdb_sys_setsockopt:
      break;

    case gdb_sys_accept:
    case gdb_sys_getsockname:
    case gdb_sys_getpeername:
      {
	ULONGEST len;

	regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
	regcache_raw_read_unsigned (regcache, tdep->arg3, &len);
	if (record_linux_sockaddr (regcache, tdep, tmpulongest, len))
	  return -1;
      }
      break;

    case gdb_sys_recvfrom:
      {
	ULONGEST len;

	regcache_raw_read_unsigned (regcache, tdep->arg4, &tmpulongest);
	regcache_raw_read_unsigned (regcache, tdep->arg5, &len);
	if (record_linux_sockaddr (regcache, tdep, tmpulongest, len))
	  return -1;
      }
      break;

    case gdb_sys_recvmsg:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (record_linux_msghdr (regcache, tdep, tmpulongest))
	return -1;
      break;

    case gdb_sys_socketpair:
      if (record_mem_at_reg (regcache, tdep->arg4, tdep->size_int))
	return -1;
      break;

    case gdb_sys_getsockopt:
      regcache_raw_read_unsigned (regcache, tdep->arg5, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST optvalp;
	  gdb_byte *optlenp = (gdb_byte *) alloca (tdep->size_int);

	  if (target_read_memory ((CORE_ADDR) tmpulongest, optlenp,
				  tdep->size_int))
	    {
	      if (record_debug)
		gdb_printf (gdb_stdlog,
			    "Process record: error reading "
			    "memory at addr = 0x%s "
			    "len = %d.\n",
			    OUTPUT_REG (tmpulongest, tdep->arg5),
			    tdep->size_int);
	      return -1;
	    }
	  regcache_raw_read_unsigned (regcache, tdep->arg4, &optvalp);
	  tmpint = (int) extract_signed_integer (optlenp, tdep->size_int,
						 byte_order);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) optvalp, tmpint))
	    return -1;
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     tdep->size_int))
	    return -1;
	}
      break;

    case gdb_sys_socketcall:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      switch (tmpulongest)
	{
	case RECORD_SYS_SOCKET:
	case RECORD_SYS_BIND:
	case RECORD_SYS_CONNECT:
	case RECORD_SYS_LISTEN:
	  break;
	case RECORD_SYS_ACCEPT:
	case RECORD_SYS_GETSOCKNAME:
	case RECORD_SYS_GETPEERNAME:
	  {
	    regcache_raw_read_unsigned (regcache, tdep->arg2,
					&tmpulongest);
	    if (tmpulongest)
	      {
		gdb_byte *a = (gdb_byte *) alloca (tdep->size_ulong * 2);
		ULONGEST len;

		tmpulongest += tdep->size_ulong;
		if (target_read_memory ((CORE_ADDR) tmpulongest, a,
					tdep->size_ulong * 2))
		  {
		    if (record_debug)
		      gdb_printf (gdb_stdlog,
				  "Process record: error reading "
				  "memory at addr = 0x%s len = %d.\n",
				  OUTPUT_REG (tmpulongest, tdep->arg2),
				  tdep->size_ulong * 2);
		    return -1;
		  }
		tmpulongest = extract_unsigned_integer (a,
							tdep->size_ulong,
							byte_order);
		len = extract_unsigned_integer (a + tdep->size_ulong,
						tdep->size_ulong, byte_order);
		if (record_linux_sockaddr (regcache, tdep, tmpulongest, len))
		  return -1;
	      }
	  }
	  break;

	case RECORD_SYS_SOCKETPAIR:
	  {
	    gdb_byte *a = (gdb_byte *) alloca (tdep->size_ulong);

	    regcache_raw_read_unsigned (regcache, tdep->arg2,
					&tmpulongest);
	    if (tmpulongest)
	      {
		tmpulongest += tdep->size_ulong * 3;
		if (target_read_memory ((CORE_ADDR) tmpulongest, a,
					tdep->size_ulong))
		  {
		    if (record_debug)
		      gdb_printf (gdb_stdlog,
				  "Process record: error reading "
				  "memory at addr = 0x%s len = %d.\n",
				  OUTPUT_REG (tmpulongest, tdep->arg2),
				  tdep->size_ulong);
		    return -1;
		  }
		tmpaddr
		  = (CORE_ADDR) extract_unsigned_integer (a, tdep->size_ulong,
							  byte_order);
		if (record_full_arch_list_add_mem (tmpaddr, tdep->size_int))
		  return -1;
	      }
	  }
	  break;
	case RECORD_SYS_SEND:
	case RECORD_SYS_SENDTO:
	  break;
	case RECORD_SYS_RECVFROM:
	  regcache_raw_read_unsigned (regcache, tdep->arg2,
				      &tmpulongest);
	  if (tmpulongest)
	    {
	      gdb_byte *a = (gdb_byte *) alloca (tdep->size_ulong * 2);
	      ULONGEST len;

	      tmpulongest += tdep->size_ulong * 4;
	      if (target_read_memory ((CORE_ADDR) tmpulongest, a,
				      tdep->size_ulong * 2))
		{
		  if (record_debug)
		    gdb_printf (gdb_stdlog,
				"Process record: error reading "
				"memory at addr = 0x%s len = %d.\n",
				OUTPUT_REG (tmpulongest, tdep->arg2),
				tdep->size_ulong * 2);
		  return -1;
		}
	      tmpulongest = extract_unsigned_integer (a, tdep->size_ulong,
						      byte_order);
	      len = extract_unsigned_integer (a + tdep->size_ulong,
					      tdep->size_ulong, byte_order);
	      if (record_linux_sockaddr (regcache, tdep, tmpulongest, len))
		return -1;
	    }
	  break;
	case RECORD_SYS_RECV:
	  regcache_raw_read_unsigned (regcache, tdep->arg2,
				      &tmpulongest);
	  if (tmpulongest)
	    {
	      gdb_byte *a = (gdb_byte *) alloca (tdep->size_ulong * 2);

	      tmpulongest += tdep->size_ulong;
	      if (target_read_memory ((CORE_ADDR) tmpulongest, a,
				      tdep->size_ulong))
		{
		  if (record_debug)
		    gdb_printf (gdb_stdlog,
				"Process record: error reading "
				"memory at addr = 0x%s len = %d.\n",
				OUTPUT_REG (tmpulongest, tdep->arg2),
				tdep->size_ulong);
		  return -1;
		}
	      tmpulongest = extract_unsigned_integer (a, tdep->size_ulong,
						      byte_order);
	      if (tmpulongest)
		{
		  a += tdep->size_ulong;
		  tmpint = (int) extract_unsigned_integer (a, tdep->size_ulong,
							   byte_order);
		  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
						     tmpint))
		    return -1;
		}
	    }
	  break;
	case RECORD_SYS_SHUTDOWN:
	case RECORD_SYS_SETSOCKOPT:
	  break;
	case RECORD_SYS_GETSOCKOPT:
	  {
	    gdb_byte *a = (gdb_byte *) alloca (tdep->size_ulong * 2);
	    gdb_byte *av = (gdb_byte *) alloca (tdep->size_int);

	    regcache_raw_read_unsigned (regcache, tdep->arg2,
					&tmpulongest);
	    if (tmpulongest)
	      {
		tmpulongest += tdep->size_ulong * 3;
		if (target_read_memory ((CORE_ADDR) tmpulongest, a,
					tdep->size_ulong * 2))
		  {
		    if (record_debug)
		      gdb_printf (gdb_stdlog,
				  "Process record: error reading "
				  "memory at addr = 0x%s len = %d.\n",
				  OUTPUT_REG (tmpulongest, tdep->arg2),
				  tdep->size_ulong * 2);
		    return -1;
		  }
		tmpulongest = extract_unsigned_integer (a + tdep->size_ulong,
							tdep->size_ulong,
							byte_order);
		if (tmpulongest)
		  {
		    if (target_read_memory ((CORE_ADDR) tmpulongest, av,
					    tdep->size_int))
		      {
			if (record_debug)
			  gdb_printf (gdb_stdlog,
				      "Process record: error reading "
				      "memory at addr = 0x%s "
				      "len = %d.\n",
				      phex_nz (tmpulongest,
					       tdep->size_ulong),
				      tdep->size_int);
			return -1;
		      }
		    tmpaddr
		      = (CORE_ADDR) extract_unsigned_integer (a,
							      tdep->size_ulong,
							      byte_order);
		    tmpint = (int) extract_unsigned_integer (av,
							     tdep->size_int,
							     byte_order);
		    if (record_full_arch_list_add_mem (tmpaddr, tmpint))
		      return -1;
		    a += tdep->size_ulong;
		    tmpaddr
		      = (CORE_ADDR) extract_unsigned_integer (a,
							      tdep->size_ulong,
							      byte_order);
		    if (record_full_arch_list_add_mem (tmpaddr,
						       tdep->size_int))
		      return -1;
		  }
	      }
	  }
	  break;
	case RECORD_SYS_SENDMSG:
	  break;
	case RECORD_SYS_RECVMSG:
	  {
	    gdb_byte *a = (gdb_byte *) alloca (tdep->size_ulong);

	    regcache_raw_read_unsigned (regcache, tdep->arg2,
					&tmpulongest);
	    if (tmpulongest)
	      {
		tmpulongest += tdep->size_ulong;
		if (target_read_memory ((CORE_ADDR) tmpulongest, a,
					tdep->size_ulong))
		  {
		    if (record_debug)
		      gdb_printf (gdb_stdlog,
				  "Process record: error reading "
				  "memory at addr = 0x%s len = %d.\n",
				  OUTPUT_REG (tmpulongest, tdep->arg2),
				  tdep->size_ulong);
		    return -1;
		  }
		tmpulongest = extract_unsigned_integer (a, tdep->size_ulong,
							byte_order);
		if (record_linux_msghdr (regcache, tdep, tmpulongest))
		  return -1;
	      }
	  }
	  break;
	default:
	  gdb_printf (gdb_stderr,
		      _("Process record and replay target "
			"doesn't support socketcall call 0x%s\n"),
		      OUTPUT_REG (tmpulongest, tdep->arg1));
	  return -1;
	  break;
	}
      break;

    case gdb_sys_syslog:
      break;

    case gdb_sys_setitimer:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_itimerval))
	return -1;
      break;

    case gdb_sys_getitimer:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_itimerval))
	return -1;
      break;

    case gdb_sys_newstat:
    case gdb_sys_newlstat:
    case gdb_sys_newfstat:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_stat))
	return -1;
      break;

    case gdb_sys_newfstatat:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					 tdep->size_stat))
	return -1;
      break;

    case gdb_sys_statx:
      regcache_raw_read_unsigned (regcache, tdep->arg5, &tmpulongest);
      if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest, 256))
	return -1;
      break;

    case gdb_sys_uname:
      if (record_mem_at_reg (regcache, tdep->arg1,
			     tdep->size_old_utsname))
	return -1;
      break;

    case gdb_sys_iopl:
    case gdb_sys_vhangup:
    case gdb_sys_ni_syscall112:
    case gdb_sys_vm86old:
      break;

    case gdb_sys_wait4:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_int)
	  || record_mem_at_reg (regcache, tdep->arg4, tdep->size_rusage))
	return -1;
      break;

    case gdb_sys_swapoff:
      break;

    case gdb_sys_sysinfo:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_sysinfo))
	return -1;
      break;

    case gdb_sys_shmget:
    case gdb_sys_semget:
    case gdb_sys_semop:
    case gdb_sys_msgget:
      /* XXX maybe need do some record works with sys_shmdt.  */
    case gdb_sys_shmdt:
    case gdb_sys_msgsnd:
    case gdb_sys_semtimedop:
      break;

    case gdb_sys_shmat:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_ulong))
	return -1;
      break;

    case gdb_sys_shmctl:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_shmid_ds))
	return -1;
      break;

      /* XXX sys_semctl 525 still not supported.  */
      /* sys_semctl */

    case gdb_sys_msgrcv:
      {
	LONGEST l;

	regcache_raw_read_signed (regcache, tdep->arg3, &l);
	tmpint = l + tdep->size_long;
	if (record_mem_at_reg (regcache, tdep->arg2, tmpint))
	  return -1;
      }
      break;

    case gdb_sys_msgctl:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_msqid_ds))
	return -1;
      break;

    case gdb_sys_ipc:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      tmpulongest &= 0xffff;
      switch (tmpulongest)
	{
	case RECORD_SEMOP:
	case RECORD_SEMGET:
	case RECORD_SEMTIMEDOP:
	case RECORD_MSGSND:
	case RECORD_MSGGET:
	  /* XXX maybe need do some record works with RECORD_SHMDT.  */
	case RECORD_SHMDT:
	case RECORD_SHMGET:
	  break;
	case RECORD_MSGRCV:
	  {
	    LONGEST second;

	    regcache_raw_read_signed (regcache, tdep->arg3, &second);
	    tmpint = (int) second + tdep->size_long;
	    if (record_mem_at_reg (regcache, tdep->arg5, tmpint))
	      return -1;
	  }
	  break;
	case RECORD_MSGCTL:
	  if (record_mem_at_reg (regcache, tdep->arg5,
				 tdep->size_msqid_ds))
	    return -1;
	  break;
	case RECORD_SHMAT:
	  if (record_mem_at_reg (regcache, tdep->arg4, tdep->size_ulong))
	    return -1;
	  break;
	case RECORD_SHMCTL:
	  if (record_mem_at_reg (regcache, tdep->arg5,
				 tdep->size_shmid_ds))
	    return -1;
	  break;
	default:
	  /* XXX RECORD_SEMCTL still not supported.  */
	  gdb_printf (gdb_stderr,
		      _("Process record and replay target doesn't "
			"support ipc number %s\n"),
		      pulongest (tmpulongest));
	  break;
	}
      break;

    case gdb_sys_fsync:
    case gdb_sys_sigreturn:
    case gdb_sys_clone:
    case gdb_sys_setdomainname:
      break;

    case gdb_sys_newuname:
      if (record_mem_at_reg (regcache, tdep->arg1,
			     tdep->size_new_utsname))
	return -1;
      break;

    case gdb_sys_modify_ldt:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest == 0 || tmpulongest == 2)
	{
	  ULONGEST bytecount;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &bytecount);
	  if (record_mem_at_reg (regcache, tdep->arg2, (int) bytecount))
	    return -1;
	}
      break;

    case gdb_sys_adjtimex:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_timex))
	return -1;
      break;

    case gdb_sys_mprotect:
      break;

    case gdb_sys_sigprocmask:
      if (record_mem_at_reg (regcache, tdep->arg3,
			     tdep->size_old_sigset_t))
	return -1;
      break;

    case gdb_sys_ni_syscall127:
    case gdb_sys_init_module:
    case gdb_sys_delete_module:
    case gdb_sys_ni_syscall130:
      break;

    case gdb_sys_quotactl:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      switch (tmpulongest)
	{
	case RECORD_Q_GETFMT:
	  /* __u32 */
	  if (record_mem_at_reg (regcache, tdep->arg4, 4))
	    return -1;
	  break;
	case RECORD_Q_GETINFO:
	  if (record_mem_at_reg (regcache, tdep->arg4,
				 tdep->size_mem_dqinfo))
	    return -1;
	  break;
	case RECORD_Q_GETQUOTA:
	  if (record_mem_at_reg (regcache, tdep->arg4,
				 tdep->size_if_dqblk))
	    return -1;
	  break;
	case RECORD_Q_XGETQSTAT:
	case RECORD_Q_XGETQUOTA:
	  if (record_mem_at_reg (regcache, tdep->arg4,
				 tdep->size_fs_quota_stat))
	    return -1;
	  break;
	}
      break;

    case gdb_sys_getpgid:
    case gdb_sys_fchdir:
    case gdb_sys_bdflush:
      break;

    case gdb_sys_sysfs:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest == 2)
	{
	  /*XXX the size of memory is not very clear.  */
	  if (record_mem_at_reg (regcache, tdep->arg3, 10))
	    return -1;
	}
      break;

    case gdb_sys_personality:
    case gdb_sys_ni_syscall137:
    case gdb_sys_setfsuid16:
    case gdb_sys_setfsgid16:
      break;

    case gdb_sys_llseek:
      if (record_mem_at_reg (regcache, tdep->arg4, tdep->size_loff_t))
	return -1;
      break;

    case gdb_sys_getdents:
    case gdb_sys_getdents64:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (record_mem_at_reg (regcache, tdep->arg2, tmpulongest))
	return -1;
      break;

    case gdb_sys_select:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_fd_set)
	  || record_mem_at_reg (regcache, tdep->arg3, tdep->size_fd_set)
	  || record_mem_at_reg (regcache, tdep->arg4, tdep->size_fd_set)
	  || record_mem_at_reg (regcache, tdep->arg5, tdep->size_timeval))
	return -1;
      break;

    case gdb_sys_flock:
    case gdb_sys_msync:
      break;

    case gdb_sys_readv:
      {
	ULONGEST vec, vlen;

	regcache_raw_read_unsigned (regcache, tdep->arg2, &vec);
	if (vec)
	  {
	    gdb_byte *iov = (gdb_byte *) alloca (tdep->size_iovec);

	    regcache_raw_read_unsigned (regcache, tdep->arg3, &vlen);
	    for (tmpulongest = 0; tmpulongest < vlen; tmpulongest++)
	      {
		if (target_read_memory ((CORE_ADDR) vec, iov,
					tdep->size_iovec))
		  {
		    if (record_debug)
		      gdb_printf (gdb_stdlog,
				  "Process record: error reading "
				  "memory at addr = 0x%s len = %d.\n",
				  OUTPUT_REG (vec, tdep->arg2),
				  tdep->size_iovec);
		    return -1;
		  }
		tmpaddr
		  = (CORE_ADDR) extract_unsigned_integer (iov,
							  tdep->size_pointer,
							  byte_order);
		tmpint
		  = (int) extract_unsigned_integer (iov + tdep->size_pointer,
						    tdep->size_size_t,
						    byte_order);
		if (record_full_arch_list_add_mem (tmpaddr, tmpint))
		  return -1;
		vec += tdep->size_iovec;
	      }
	  }
      }
      break;

    case gdb_sys_writev:
    case gdb_sys_getsid:
    case gdb_sys_fdatasync:
    case gdb_sys_sysctl:
    case gdb_sys_mlock:
    case gdb_sys_munlock:
    case gdb_sys_mlockall:
    case gdb_sys_munlockall:
    case gdb_sys_sched_setparam:
      break;

    case gdb_sys_sched_getparam:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_int))
	return -1;
      break;

    case gdb_sys_sched_setscheduler:
    case gdb_sys_sched_getscheduler:
    case gdb_sys_sched_yield:
    case gdb_sys_sched_get_priority_max:
    case gdb_sys_sched_get_priority_min:
      break;

    case gdb_sys_sched_rr_get_interval:
    case gdb_sys_nanosleep:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_timespec))
	return -1;
      break;

    case gdb_sys_mremap:
    case gdb_sys_setresuid16:
      break;

    case gdb_sys_getresuid16:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_old_uid_t)
	  || record_mem_at_reg (regcache, tdep->arg2,
				tdep->size_old_uid_t)
	  || record_mem_at_reg (regcache, tdep->arg3,
				tdep->size_old_uid_t))
	return -1;
      break;

    case gdb_sys_vm86:
    case gdb_sys_ni_syscall167:
      break;

    case gdb_sys_poll:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST nfds;

	  regcache_raw_read_unsigned (regcache, tdep->arg2, &nfds);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     tdep->size_pollfd * nfds))
	    return -1;
	}
      break;

    case gdb_sys_nfsservctl:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest == 7 || tmpulongest == 8)
	{
	  int rsize;

	  if (tmpulongest == 7)
	    rsize = tdep->size_NFS_FHSIZE;
	  else
	    rsize = tdep->size_knfsd_fh;
	  if (record_mem_at_reg (regcache, tdep->arg3, rsize))
	    return -1;
	}
      break;

    case gdb_sys_setresgid16:
      break;

    case gdb_sys_getresgid16:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_old_gid_t)
	  || record_mem_at_reg (regcache, tdep->arg2,
				tdep->size_old_gid_t)
	  || record_mem_at_reg (regcache, tdep->arg3,
				tdep->size_old_gid_t))
	return -1;
      break;

    case gdb_sys_prctl:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      switch (tmpulongest)
	{
	case 2:
	  if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_int))
	    return -1;
	  break;
	case 16:
	  if (record_mem_at_reg (regcache, tdep->arg2,
				 tdep->size_TASK_COMM_LEN))
	    return -1;
	  break;
	}
      break;

    case gdb_sys_rt_sigreturn:
      break;

    case gdb_sys_rt_sigaction:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_sigaction))
	return -1;
      break;

    case gdb_sys_rt_sigprocmask:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_sigset_t))
	return -1;
      break;

    case gdb_sys_rt_sigpending:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST sigsetsize;

	  regcache_raw_read_unsigned (regcache, tdep->arg2,&sigsetsize);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) sigsetsize))
	    return -1;
	}
      break;

    case gdb_sys_rt_sigtimedwait:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_siginfo_t))
	return -1;
      break;

    case gdb_sys_rt_sigqueueinfo:
    case gdb_sys_rt_sigsuspend:
      break;

    case gdb_sys_pread64:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST count;

	  regcache_raw_read_unsigned (regcache, tdep->arg3,&count);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) count))
	    return -1;
	}
      break;

    case gdb_sys_pwrite64:
    case gdb_sys_chown16:
      break;

    case gdb_sys_getcwd:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST size;

	  regcache_raw_read_unsigned (regcache, tdep->arg2, &size);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) size))
	    return -1;
	}
      break;

    case gdb_sys_capget:
      if (record_mem_at_reg (regcache, tdep->arg2,
			     tdep->size_cap_user_data_t))
	return -1;
      break;

    case gdb_sys_capset:
      break;

    case gdb_sys_sigaltstack:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_stack_t))
	return -1;
      break;

    case gdb_sys_sendfile:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_off_t))
	return -1;
      break;

    case gdb_sys_ni_syscall188:
    case gdb_sys_ni_syscall189:
    case gdb_sys_vfork:
      break;

    case gdb_sys_getrlimit:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_rlimit))
	return -1;
      break;

    case gdb_sys_mmap2:
      break;

    case gdb_sys_truncate64:
    case gdb_sys_ftruncate64:
      break;

    case gdb_sys_stat64:
    case gdb_sys_lstat64:
    case gdb_sys_fstat64:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_stat64))
	return -1;
      break;

    case gdb_sys_lchown:
    case gdb_sys_getuid:
    case gdb_sys_getgid:
    case gdb_sys_geteuid:
    case gdb_sys_getegid:
    case gdb_sys_setreuid:
    case gdb_sys_setregid:
      break;

    case gdb_sys_getgroups:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST gidsetsize;

	  regcache_raw_read_unsigned (regcache, tdep->arg1,
				      &gidsetsize);
	  tmpint = tdep->size_gid_t * (int) gidsetsize;
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest, tmpint))
	    return -1;
	}
      break;

    case gdb_sys_setgroups:
    case gdb_sys_fchown:
    case gdb_sys_setresuid:
      break;

    case gdb_sys_getresuid:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_uid_t)
	  || record_mem_at_reg (regcache, tdep->arg2, tdep->size_uid_t)
	  || record_mem_at_reg (regcache, tdep->arg3, tdep->size_uid_t))
	return -1;
      break;

    case gdb_sys_setresgid:
      break;

    case gdb_sys_getresgid:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_gid_t)
	  || record_mem_at_reg (regcache, tdep->arg2, tdep->size_gid_t)
	  || record_mem_at_reg (regcache, tdep->arg3, tdep->size_gid_t))
	return -1;
      break;

    case gdb_sys_chown:
    case gdb_sys_setuid:
    case gdb_sys_setgid:
    case gdb_sys_setfsuid:
    case gdb_sys_setfsgid:
    case gdb_sys_pivot_root:
      break;

    case gdb_sys_mincore:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_PAGE_SIZE))
	return -1;
      break;

    case gdb_sys_madvise:
      break;

    case gdb_sys_fcntl64:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest == tdep->fcntl_F_GETLK64)
	{
	  if (record_mem_at_reg (regcache, tdep->arg3,
				 tdep->size_flock64))
	    return -1;
	}
      else if (tmpulongest != tdep->fcntl_F_SETLK64
	       && tmpulongest != tdep->fcntl_F_SETLKW64)
	{
	  goto sys_fcntl;
	}
      break;

    case gdb_sys_ni_syscall222:
    case gdb_sys_ni_syscall223:
    case gdb_sys_gettid:
    case gdb_sys_readahead:
    case gdb_sys_setxattr:
    case gdb_sys_lsetxattr:
    case gdb_sys_fsetxattr:
      break;

    case gdb_sys_getxattr:
    case gdb_sys_lgetxattr:
    case gdb_sys_fgetxattr:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST size;

	  regcache_raw_read_unsigned (regcache, tdep->arg4, &size);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) size))
	    return -1;
	}
      break;

    case gdb_sys_listxattr:
    case gdb_sys_llistxattr:
    case gdb_sys_flistxattr:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST size;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &size);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) size))
	    return -1;
	}
      break;

    case gdb_sys_removexattr:
    case gdb_sys_lremovexattr:
    case gdb_sys_fremovexattr:
    case gdb_sys_tkill:
      break;

    case gdb_sys_sendfile64:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_loff_t))
	return -1;
      break;

    case gdb_sys_futex:
    case gdb_sys_sched_setaffinity:
      break;

    case gdb_sys_sched_getaffinity:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST len;

	  regcache_raw_read_unsigned (regcache, tdep->arg2, &len);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) len))
	    return -1;
	}
      break;

    case gdb_sys_set_thread_area:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_int))
	return -1;
      break;

    case gdb_sys_get_thread_area:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_user_desc))
	return -1;
      break;

    case gdb_sys_io_setup:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_long))
	return -1;
      break;

    case gdb_sys_io_destroy:
      break;

    case gdb_sys_io_getevents:
      regcache_raw_read_unsigned (regcache, tdep->arg4, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST nr;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &nr);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     nr * tdep->size_io_event))
	    return -1;
	}
      break;

    case gdb_sys_io_submit:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST nr, i;
	  gdb_byte *iocbp;

	  regcache_raw_read_unsigned (regcache, tdep->arg2, &nr);
	  iocbp = (gdb_byte *) alloca (nr * tdep->size_pointer);
	  if (target_read_memory ((CORE_ADDR) tmpulongest, iocbp,
				  nr * tdep->size_pointer))
	    {
	      if (record_debug)
		gdb_printf (gdb_stdlog,
			    "Process record: error reading memory "
			    "at addr = 0x%s len = %u.\n",
			    OUTPUT_REG (tmpulongest, tdep->arg2),
			    (int) (nr * tdep->size_pointer));
	      return -1;
	    }
	  for (i = 0; i < nr; i++)
	    {
	      tmpaddr
		= (CORE_ADDR) extract_unsigned_integer (iocbp,
							tdep->size_pointer,
							byte_order);
	      if (record_full_arch_list_add_mem (tmpaddr, tdep->size_iocb))
		return -1;
	      iocbp += tdep->size_pointer;
	    }
	}
      break;

    case gdb_sys_io_cancel:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_io_event))
	return -1;
      break;

    case gdb_sys_fadvise64:
    case gdb_sys_ni_syscall251:
      break;

    case gdb_sys_exit_group:
      if (yquery (_("The next instruction is syscall exit_group.  "
		    "It will make the program exit.  "
		    "Do you want to stop the program?")))
	return 1;
      break;

    case gdb_sys_lookup_dcookie:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST len;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &len);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) len))
	    return -1;
	}
      break;

    case gdb_sys_epoll_create:
    case gdb_sys_epoll_ctl:
      break;

    case gdb_sys_epoll_wait:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST maxevents;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &maxevents);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (maxevents
					      * tdep->size_epoll_event)))
	    return -1;
	}
      break;

    case gdb_sys_remap_file_pages:
    case gdb_sys_set_tid_address:
      break;

    case gdb_sys_timer_create:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_int))
	return -1;
      break;

    case gdb_sys_timer_settime:
      if (record_mem_at_reg (regcache, tdep->arg4, tdep->size_itimerspec))
	return -1;
      break;

    case gdb_sys_timer_gettime:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_itimerspec))
	return -1;
      break;

    case gdb_sys_timer_getoverrun:
    case gdb_sys_timer_delete:
    case gdb_sys_clock_settime:
      break;

    case gdb_sys_clock_gettime:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_timespec))
	return -1;
      break;

    case gdb_sys_clock_getres:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_timespec))
	return -1;
      break;

    case gdb_sys_clock_nanosleep:
      if (record_mem_at_reg (regcache, tdep->arg4, tdep->size_timespec))
	return -1;
      break;

    case gdb_sys_statfs64:
    case gdb_sys_fstatfs64:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_statfs64))
	return -1;
      break;

    case gdb_sys_tgkill:
    case gdb_sys_utimes:
    case gdb_sys_fadvise64_64:
    case gdb_sys_ni_syscall273:
    case gdb_sys_mbind:
      break;

    case gdb_sys_get_mempolicy:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_int))
	return -1;
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST maxnode;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &maxnode);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     maxnode * tdep->size_long))
	    return -1;
	}
      break;

    case gdb_sys_set_mempolicy:
    case gdb_sys_mq_open:
    case gdb_sys_mq_unlink:
    case gdb_sys_mq_timedsend:
      break;

    case gdb_sys_mq_timedreceive:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST msg_len;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &msg_len);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) msg_len))
	    return -1;
	}
      if (record_mem_at_reg (regcache, tdep->arg4, tdep->size_int))
	return -1;
      break;

    case gdb_sys_mq_notify:
      break;

    case gdb_sys_mq_getsetattr:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_mq_attr))
	return -1;
      break;

    case gdb_sys_kexec_load:
      break;

    case gdb_sys_waitid:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_siginfo_t)
	  || record_mem_at_reg (regcache, tdep->arg5, tdep->size_rusage))
	return -1;
      break;

    case gdb_sys_ni_syscall285:
    case gdb_sys_add_key:
    case gdb_sys_request_key:
      break;

    case gdb_sys_keyctl:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest == 6 || tmpulongest == 11)
	{
	  regcache_raw_read_unsigned (regcache, tdep->arg3,
				      &tmpulongest);
	  if (tmpulongest)
	    {
	      ULONGEST buflen;

	      regcache_raw_read_unsigned (regcache, tdep->arg4, &buflen);
	      if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
						 (int) buflen))
		return -1;
	    }
	}
      break;

    case gdb_sys_ioprio_set:
    case gdb_sys_ioprio_get:
    case gdb_sys_inotify_init:
    case gdb_sys_inotify_add_watch:
    case gdb_sys_inotify_rm_watch:
    case gdb_sys_migrate_pages:
    case gdb_sys_openat:
    case gdb_sys_mkdirat:
    case gdb_sys_mknodat:
    case gdb_sys_fchownat:
    case gdb_sys_futimesat:
      break;

    case gdb_sys_fstatat64:
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_stat64))
	return -1;
      break;

    case gdb_sys_unlinkat:
    case gdb_sys_renameat:
    case gdb_sys_linkat:
    case gdb_sys_symlinkat:
      break;

    case gdb_sys_readlinkat:
      regcache_raw_read_unsigned (regcache, tdep->arg3, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST bufsiz;

	  regcache_raw_read_unsigned (regcache, tdep->arg4, &bufsiz);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     (int) bufsiz))
	    return -1;
	}
      break;

    case gdb_sys_fchmodat:
    case gdb_sys_faccessat:
      break;

    case gdb_sys_pselect6:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_fd_set)
	  || record_mem_at_reg (regcache, tdep->arg3, tdep->size_fd_set)
	  || record_mem_at_reg (regcache, tdep->arg4, tdep->size_fd_set)
	  || record_mem_at_reg (regcache, tdep->arg5, tdep->size_timespec))
	return -1;
      break;

    case gdb_sys_ppoll:
      regcache_raw_read_unsigned (regcache, tdep->arg1, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST nfds;

	  regcache_raw_read_unsigned (regcache, tdep->arg2, &nfds);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     tdep->size_pollfd * nfds))
	    return -1;
	}
      if (record_mem_at_reg (regcache, tdep->arg3, tdep->size_timespec))
	return -1;
      break;

    case gdb_sys_unshare:
    case gdb_sys_set_robust_list:
      break;

    case gdb_sys_get_robust_list:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_int)
	  || record_mem_at_reg (regcache, tdep->arg3, tdep->size_int))
	return -1;
      break;

    case gdb_sys_splice:
      if (record_mem_at_reg (regcache, tdep->arg2, tdep->size_loff_t)
	  || record_mem_at_reg (regcache, tdep->arg4, tdep->size_loff_t))
	return -1;
      break;

    case gdb_sys_sync_file_range:
    case gdb_sys_tee:
    case gdb_sys_vmsplice:
      break;

    case gdb_sys_move_pages:
      regcache_raw_read_unsigned (regcache, tdep->arg5, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST nr_pages;

	  regcache_raw_read_unsigned (regcache, tdep->arg2, &nr_pages);
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest,
					     nr_pages * tdep->size_int))
	    return -1;
	}
      break;

    case gdb_sys_getcpu:
      if (record_mem_at_reg (regcache, tdep->arg1, tdep->size_int)
	  || record_mem_at_reg (regcache, tdep->arg2, tdep->size_int)
	  || record_mem_at_reg (regcache, tdep->arg3,
				tdep->size_ulong * 2))
	return -1;
      break;

    case gdb_sys_epoll_pwait:
      regcache_raw_read_unsigned (regcache, tdep->arg2, &tmpulongest);
      if (tmpulongest)
	{
	  ULONGEST maxevents;

	  regcache_raw_read_unsigned (regcache, tdep->arg3, &maxevents);
	  tmpint = (int) maxevents * tdep->size_epoll_event;
	  if (record_full_arch_list_add_mem ((CORE_ADDR) tmpulongest, tmpint))
	    return -1;
	}
      break;

    case gdb_sys_fallocate:
    case gdb_sys_eventfd2:
    case gdb_sys_epoll_create1:
    case gdb_sys_dup3:
      break;

    case gdb_sys_inotify_init1:
      break;

    default:
      gdb_printf (gdb_stderr,
		  _("Process record and replay target doesn't "
		    "support syscall number %d\n"), syscall);
      return -1;
      break;
    }

  return 0;
}
