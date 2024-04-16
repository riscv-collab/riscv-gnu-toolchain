/* GDB stub for Itanium OpenVMS
   Copyright (C) 2012-2024 Free Software Foundation, Inc.

   Contributed by Tristan Gingold, AdaCore.

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

/* On VMS, the debugger (in our case the stub) is loaded in the process and
   executed (via SYS$IMGSTA) before the main entry point of the executable.
   In UNIX parlance, this is like using LD_PRELOAD and debug via installing
   SIGTRAP, SIGSEGV... handlers.

   This is currently a partial implementation.  In particular, modifying
   registers is currently not implemented, as well as inferior procedure
   calls.

   This is written in very low-level C, in order not to use the C runtime,
   because it may have weird consequences on the program being debugged.
*/

#if __INITIAL_POINTER_SIZE != 64
#error "Must be compiled with 64 bit pointers"
#endif

#define __NEW_STARLET 1
#include <descrip.h>
#include <iledef.h>
#include <efndef.h>
#include <in.h>
#include <inet.h>
#include <iodef.h>
#include <ssdef.h>
#include <starlet.h>
#include <stsdef.h>
#include <tcpip$inetdef.h>

#include <lib$routines.h>
#include <ots$routines.h>
#include <str$routines.h>
#include <libdef.h>
#include <clidef.h>
#include <iosbdef.h>
#include <dvidef.h>
#include <lnmdef.h>
#include <builtins.h>
#include <prtdef.h>
#include <psldef.h>
#include <chfdef.h>

#include <lib_c/imcbdef.h>
#include <lib_c/ldrimgdef.h>
#include <lib_c/intstkdef.h>
#include <lib_c/psrdef.h>
#include <lib_c/ifddef.h>
#include <lib_c/eihddef.h>

#include <stdarg.h>
#include <pthread_debug.h>

#define VMS_PAGE_SIZE 0x2000
#define VMS_PAGE_MASK (VMS_PAGE_SIZE - 1)

/* Declared in lib$ots.  */
extern void ots$fill (void *addr, size_t len, unsigned char b);
extern void ots$move (void *dst, size_t len, const void *src);
extern int ots$strcmp_eql (const void *str1, size_t str1len,
			   const void *str2, size_t str2len);

/* Stub port number.  */
static unsigned int serv_port = 1234;

/* DBGEXT structure.  Not declared in any header.  */
struct dbgext_control_block
{
  unsigned short dbgext$w_function_code;
#define DBGEXT$K_NEXT_TASK	      3
#define DBGEXT$K_STOP_ALL_OTHER_TASKS 31
#define DBGEXT$K_GET_REGS 33
  unsigned short dbgext$w_facility_id;
#define CMA$_FACILITY 64
  unsigned int dbgext$l_status;
  unsigned int dbgext$l_flags;
  unsigned int dbgext$l_print_routine;
  unsigned int dbgext$l_evnt_code;
  unsigned int dbgext$l_evnt_name;
  unsigned int dbgext$l_evnt_entry;
  unsigned int dbgext$l_task_value;
  unsigned int dbgext$l_task_number;
  unsigned int dbgext$l_ada_flags;
  unsigned int dbgext$l_stop_value;
#define dbgext$l_priority   dbgext$l_stop_value;
#define dbgext$l_symb_addr  dbgext$l_stop_value;
#define dbgext$l_time_slice dbgext$l_stop_value;
  unsigned int dbgext$l_active_registers;
};

#pragma pointer_size save
#pragma pointer_size 32

/* Pthread handler.  */
static int (*dbgext_func) (struct dbgext_control_block *blk);

#pragma pointer_size restore

/* Set to 1 if thread-aware.  */
static int has_threads;

/* Current thread.  */
static pthread_t selected_thread;
static pthreadDebugId_t selected_id;

/* Internal debugging flags.  */
struct debug_flag
{
  /* Name of the flag (as a string descriptor).  */
  const struct dsc$descriptor_s name;
  /* Value.  */
  int val;
};

/* Macro to define a debugging flag.  */
#define DEBUG_FLAG_ENTRY(str) \
  { { sizeof (str) - 1, DSC$K_DTYPE_T, DSC$K_CLASS_S, str }, 0}

static struct debug_flag debug_flags[] =
{
  /* Disp packets exchanged with gdb.  */
  DEBUG_FLAG_ENTRY("packets"),
#define trace_pkt (debug_flags[0].val)
  /* Display entry point informations.  */
  DEBUG_FLAG_ENTRY("entry"),
#define trace_entry (debug_flags[1].val)
  /* Be verbose about exceptions.  */
  DEBUG_FLAG_ENTRY("excp"),
#define trace_excp (debug_flags[2].val)
  /* Be verbose about unwinding.  */
  DEBUG_FLAG_ENTRY("unwind"),
#define trace_unwind (debug_flags[3].val)
  /* Display image at startup.  */
  DEBUG_FLAG_ENTRY("images"),
#define trace_images (debug_flags[4].val)
  /* Display pthread_debug info.  */
  DEBUG_FLAG_ENTRY("pthreaddbg")
#define trace_pthreaddbg (debug_flags[5].val)
};

#define NBR_DEBUG_FLAGS (sizeof (debug_flags) / sizeof (debug_flags[0]))

/* Connect inet device I/O channel.  */
static unsigned short conn_channel;

/* Widely used hex digit to ascii.  */
static const char hex[] = "0123456789abcdef";

/* Socket characteristics.  Apparently, there are no declaration for it in
   standard headers.  */
struct sockchar
{
  unsigned short prot;
  unsigned char type;
  unsigned char af;
};

/* Chain of images loaded.  */
extern IMCB* ctl$gl_imglstptr;

/* IA64 integer register representation.  */
union ia64_ireg
{
  unsigned __int64 v;
  unsigned char b[8];
};

/* IA64 register numbers, as defined by ia64-tdep.h.  */
#define IA64_GR0_REGNUM		0
#define IA64_GR32_REGNUM	(IA64_GR0_REGNUM + 32)

/* Floating point registers; 128 82-bit wide registers.  */
#define IA64_FR0_REGNUM		128

/* Predicate registers; There are 64 of these one bit registers.  It'd
   be more convenient (implementation-wise) to use a single 64 bit
   word with all of these register in them.  Note that there's also a
   IA64_PR_REGNUM below which contains all the bits and is used for
   communicating the actual values to the target.  */
#define IA64_PR0_REGNUM		256

/* Branch registers: 8 64-bit registers for holding branch targets.  */
#define IA64_BR0_REGNUM		320

/* Virtual frame pointer; this matches IA64_FRAME_POINTER_REGNUM in
   gcc/config/ia64/ia64.h.  */
#define IA64_VFP_REGNUM		328

/* Virtual return address pointer; this matches
   IA64_RETURN_ADDRESS_POINTER_REGNUM in gcc/config/ia64/ia64.h.  */
#define IA64_VRAP_REGNUM	329

/* Predicate registers: There are 64 of these 1-bit registers.  We
   define a single register which is used to communicate these values
   to/from the target.  We will somehow contrive to make it appear
   that IA64_PR0_REGNUM thru IA64_PR63_REGNUM hold the actual values.  */
#define IA64_PR_REGNUM		330

/* Instruction pointer: 64 bits wide.  */
#define IA64_IP_REGNUM		331

/* Process Status Register.  */
#define IA64_PSR_REGNUM		332

/* Current Frame Marker (raw form may be the cr.ifs).  */
#define IA64_CFM_REGNUM		333

/* Application registers; 128 64-bit wide registers possible, but some
   of them are reserved.  */
#define IA64_AR0_REGNUM		334
#define IA64_KR0_REGNUM		(IA64_AR0_REGNUM + 0)
#define IA64_KR7_REGNUM		(IA64_KR0_REGNUM + 7)

#define IA64_RSC_REGNUM		(IA64_AR0_REGNUM + 16)
#define IA64_BSP_REGNUM		(IA64_AR0_REGNUM + 17)
#define IA64_BSPSTORE_REGNUM	(IA64_AR0_REGNUM + 18)
#define IA64_RNAT_REGNUM	(IA64_AR0_REGNUM + 19)
#define IA64_FCR_REGNUM		(IA64_AR0_REGNUM + 21)
#define IA64_EFLAG_REGNUM	(IA64_AR0_REGNUM + 24)
#define IA64_CSD_REGNUM		(IA64_AR0_REGNUM + 25)
#define IA64_SSD_REGNUM		(IA64_AR0_REGNUM + 26)
#define IA64_CFLG_REGNUM	(IA64_AR0_REGNUM + 27)
#define IA64_FSR_REGNUM		(IA64_AR0_REGNUM + 28)
#define IA64_FIR_REGNUM		(IA64_AR0_REGNUM + 29)
#define IA64_FDR_REGNUM		(IA64_AR0_REGNUM + 30)
#define IA64_CCV_REGNUM		(IA64_AR0_REGNUM + 32)
#define IA64_UNAT_REGNUM	(IA64_AR0_REGNUM + 36)
#define IA64_FPSR_REGNUM	(IA64_AR0_REGNUM + 40)
#define IA64_ITC_REGNUM		(IA64_AR0_REGNUM + 44)
#define IA64_PFS_REGNUM		(IA64_AR0_REGNUM + 64)
#define IA64_LC_REGNUM		(IA64_AR0_REGNUM + 65)
#define IA64_EC_REGNUM		(IA64_AR0_REGNUM + 66)

/* NAT (Not A Thing) Bits for the general registers; there are 128 of
   these.  */
#define IA64_NAT0_REGNUM	462

/* Process registers when a condition is caught.  */
struct ia64_all_regs
{
  union ia64_ireg gr[32];
  union ia64_ireg br[8];
  union ia64_ireg ip;
  union ia64_ireg psr;
  union ia64_ireg bsp;
  union ia64_ireg cfm;
  union ia64_ireg pfs;
  union ia64_ireg pr;
};

static struct ia64_all_regs excp_regs;
static struct ia64_all_regs sel_regs;
static pthread_t sel_regs_pthread;

/* IO channel for the terminal.  */
static unsigned short term_chan;

/* Output buffer and length.  */
static char term_buf[128];
static int term_buf_len;

/* Buffer for communication with gdb.  */
static unsigned char gdb_buf[sizeof (struct ia64_all_regs) * 2 + 64];
static unsigned int gdb_blen;

/* Previous primary handler.  */
static void *prevhnd;

/* Entry point address and bundle.  */
static unsigned __int64 entry_pc;
static unsigned char entry_saved[16];

/* Write on the terminal.  */

static void
term_raw_write (const char *str, unsigned int len)
{
  unsigned short status;
  struct _iosb iosb;

  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     term_chan,           /* I/O channel.  */
		     IO$_WRITEVBLK,       /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     (char *)str,         /* P1 - buffer address.  */
		     len,                 /* P2 - buffer length.  */
		     0, 0, 0, 0);

  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
}

/* Flush ther term buffer.  */

static void
term_flush (void)
{
  if (term_buf_len != 0)
    {
      term_raw_write (term_buf, term_buf_len);
      term_buf_len = 0;
    }
}

/* Write a single character, without translation.  */

static void
term_raw_putchar (char c)
{
  if (term_buf_len == sizeof (term_buf))
    term_flush ();
  term_buf[term_buf_len++] = c;
}

/* Write character C.  Translate '\n' to '\n\r'.  */

static void
term_putc (char c)
{
  if (c < 32)
    switch (c)
      {
      case '\r':
      case '\n':
	break;
      default:
	c = '.';
	break;
      }
  term_raw_putchar (c);
  if (c == '\n')
    {
      term_raw_putchar ('\r');
      term_flush ();
    }
}

/* Write a C string.  */

static void
term_puts (const char *str)
{
  while (*str)
    term_putc (*str++);
}

/* Write LEN bytes from STR.  */

static void
term_write (const char *str, unsigned int len)
{
  for (; len > 0; len--)
    term_putc (*str++);
}

/* Write using FAO formatting.  */

static void
term_fao (const char *str, unsigned int str_len, ...)
{
  int cnt;
  va_list vargs;
  int i;
  __int64 *args;
  int status;
  struct dsc$descriptor_s dstr =
    { str_len, DSC$K_DTYPE_T, DSC$K_CLASS_S, (__char_ptr32)str };
  char buf[128];
  $DESCRIPTOR (buf_desc, buf);

  va_start (vargs, str_len);
  va_count (cnt);
  args = (__int64 *) __ALLOCA (cnt * sizeof (__int64));
  cnt -= 2;
  for (i = 0; i < cnt; i++)
    args[i] = va_arg (vargs, __int64);

  status = sys$faol_64 (&dstr, &buf_desc.dsc$w_length, &buf_desc, args);
  if (status & 1)
    {
      /* FAO !/ already insert a line feed.  */
      for (i = 0; i < buf_desc.dsc$w_length; i++)
	{
	  term_raw_putchar (buf[i]);
	  if (buf[i] == '\n')
	    term_flush ();
	}
    }
      
  va_end (vargs);
}

#define TERM_FAO(STR, ...) term_fao (STR, sizeof (STR) - 1, __VA_ARGS__)

/* New line.  */

static void
term_putnl (void)
{
  term_putc ('\n');
}

/* Initialize terminal.  */

static void
term_init (void)
{
  unsigned int status,i;
  unsigned short len;
  char resstring[LNM$C_NAMLENGTH];
  static const $DESCRIPTOR (tabdesc, "LNM$FILE_DEV");
  static const $DESCRIPTOR (logdesc, "SYS$OUTPUT");
  $DESCRIPTOR (term_desc, resstring);
  ILE3 item_lst[2];

  item_lst[0].ile3$w_length = LNM$C_NAMLENGTH;
  item_lst[0].ile3$w_code = LNM$_STRING;
  item_lst[0].ile3$ps_bufaddr = resstring;
  item_lst[0].ile3$ps_retlen_addr = &len;
  item_lst[1].ile3$w_length = 0;
  item_lst[1].ile3$w_code = 0;

  /* Translate the logical name.  */
  status = SYS$TRNLNM (0,          	  /* Attr of the logical name.  */
		       (void *) &tabdesc, /* Logical name table.  */
		       (void *) &logdesc, /* Logical name.  */
		       0,          /* Access mode.  */
		       item_lst);  /* Item list.  */
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);

  term_desc.dsc$w_length = len;

  /* Examine 4-byte header.  Skip escape sequence.  */
  if (resstring[0] == 0x1B)
    {
      term_desc.dsc$w_length -= 4;
      term_desc.dsc$a_pointer += 4;
    }

  /* Assign a channel.  */
  status = sys$assign (&term_desc,   /* Device name.  */
		       &term_chan,   /* I/O channel.  */
		       0,            /* Access mode.  */
		       0);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
}

/* Convert from native endianness to network endianness (and vice-versa).  */

static unsigned int
wordswap (unsigned int v)
{
  return ((v & 0xff) << 8) | ((v >> 8) & 0xff);
}

/* Initialize the socket connection, and wait for a client.  */

static void
sock_init (void)
{
  struct _iosb iosb;
  unsigned int status;

  /* Listen channel and characteristics.  */
  unsigned short listen_channel;
  struct sockchar listen_sockchar;

  /* Client address.  */
  unsigned short cli_addrlen;
  struct sockaddr_in cli_addr;
  ILE3 cli_itemlst;

  /* Our address.  */
  struct sockaddr_in serv_addr;
  ILE2 serv_itemlst;

  /* Reuseaddr option value (on).  */
  int optval = 1;
  ILE2 sockopt_itemlst;
  ILE2 reuseaddr_itemlst;

  /* TCP/IP network pseudodevice.  */
  static const $DESCRIPTOR (inet_device, "TCPIP$DEVICE:");

  /* Initialize socket characteristics.  */
  listen_sockchar.prot = TCPIP$C_TCP;
  listen_sockchar.type = TCPIP$C_STREAM;
  listen_sockchar.af   = TCPIP$C_AF_INET;

  /* Assign I/O channels to network device.  */
  status = sys$assign ((void *) &inet_device, &listen_channel, 0, 0);
  if (status & STS$M_SUCCESS)
    status = sys$assign ((void *) &inet_device, &conn_channel, 0, 0);
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to assign I/O channel(s)\n");
      LIB$SIGNAL (status);
    }

  /* Create a listen socket.  */
  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     listen_channel,      /* I/O channel.  */
		     IO$_SETMODE,         /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     &listen_sockchar,    /* P1 - socket characteristics.  */
		     0, 0, 0, 0, 0);
  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to create socket\n");
      LIB$SIGNAL (status);
    }

  /* Set reuse address option.  */
  /* Initialize reuseaddr's item-list element.  */
  reuseaddr_itemlst.ile2$w_length   = sizeof (optval);
  reuseaddr_itemlst.ile2$w_code     = TCPIP$C_REUSEADDR;
  reuseaddr_itemlst.ile2$ps_bufaddr = &optval;

  /* Initialize setsockopt's item-list descriptor.  */
  sockopt_itemlst.ile2$w_length   = sizeof (reuseaddr_itemlst);
  sockopt_itemlst.ile2$w_code     = TCPIP$C_SOCKOPT;
  sockopt_itemlst.ile2$ps_bufaddr = &reuseaddr_itemlst;

  status = sys$qiow (EFN$C_ENF,       /* Event flag.  */
		     listen_channel,  /* I/O channel.  */
		     IO$_SETMODE,     /* I/O function code.  */
		     &iosb,           /* I/O status block.  */
		     0,               /* Ast service routine.  */
		     0,               /* Ast parameter.  */
		     0,               /* P1.  */
		     0,               /* P2.  */
		     0,               /* P3.  */
		     0,               /* P4.  */
		     (__int64) &sockopt_itemlst, /* P5 - socket options.  */
		     0);
  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to set socket option\n");
      LIB$SIGNAL (status);
    }

  /* Bind server's ip address and port number to listen socket.  */
  /* Initialize server's socket address structure.  */
  ots$fill (&serv_addr, sizeof (serv_addr), 0);
  serv_addr.sin_family = TCPIP$C_AF_INET;
  serv_addr.sin_port = wordswap (serv_port);
  serv_addr.sin_addr.s_addr = TCPIP$C_INADDR_ANY;

  /* Initialize server's item-list descriptor.  */
  serv_itemlst.ile2$w_length   = sizeof (serv_addr);
  serv_itemlst.ile2$w_code     = TCPIP$C_SOCK_NAME;
  serv_itemlst.ile2$ps_bufaddr = &serv_addr;

  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     listen_channel,      /* I/O channel.  */
		     IO$_SETMODE,         /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     0,                   /* P1.  */
		     0,                   /* P2.  */
		     (__int64) &serv_itemlst, /* P3 - local socket name.  */
		     0, 0, 0);
  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to bind socket\n");
      LIB$SIGNAL (status);
    }

  /* Set socket as a listen socket.  */
  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     listen_channel,      /* I/O channel.  */
		     IO$_SETMODE,         /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     0,                   /* P1.  */
		     0,                   /* P2.  */
		     0,                   /* P3.  */
		     1,                   /* P4 - connection backlog.  */
		     0, 0);
  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to set socket passive\n");
      LIB$SIGNAL (status);
    }

  /* Accept connection from a client.  */
  TERM_FAO ("Waiting for a client connection on port: !ZW!/",
	    wordswap (serv_addr.sin_port));

  status = sys$qiow (EFN$C_ENF,              /* Event flag.  */
		     listen_channel,         /* I/O channel.  */
		     IO$_ACCESS|IO$M_ACCEPT, /* I/O function code.  */
		     &iosb,                  /* I/O status block.  */
		     0,                      /* Ast service routine.  */
		     0,                      /* Ast parameter.  */
		     0,                      /* P1.  */
		     0,                      /* P2.  */
		     0,                      /* P3.  */
		     (__int64) &conn_channel, /* P4 - I/O channel for conn.  */
		     0, 0);

  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to accept client connection\n");
      LIB$SIGNAL (status);
    }

  /* Log client connection request.  */
  cli_itemlst.ile3$w_length = sizeof (cli_addr);
  cli_itemlst.ile3$w_code = TCPIP$C_SOCK_NAME;
  cli_itemlst.ile3$ps_bufaddr = &cli_addr;
  cli_itemlst.ile3$ps_retlen_addr = &cli_addrlen;
  ots$fill (&cli_addr, sizeof(cli_addr), 0);
  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     conn_channel,        /* I/O channel.  */
		     IO$_SENSEMODE,       /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     0,                   /* P1.  */
		     0,                   /* P2.  */
		     0,                   /* P3.  */
		     (__int64) &cli_itemlst,  /* P4 - peer socket name.  */
		     0, 0);
  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to get client name\n");
      LIB$SIGNAL (status);
    }

  TERM_FAO ("Accepted connection from host: !UB.!UB,!UB.!UB, port: !UW!/",
	    (cli_addr.sin_addr.s_addr >> 0) & 0xff,
	    (cli_addr.sin_addr.s_addr >> 8) & 0xff,
	    (cli_addr.sin_addr.s_addr >> 16) & 0xff,
	    (cli_addr.sin_addr.s_addr >> 24) & 0xff,
	    wordswap (cli_addr.sin_port));
}

/* Close the socket.  */

static void
sock_close (void)
{
  struct _iosb iosb;
  unsigned int status;

  /* Close socket.  */
  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     conn_channel,        /* I/O channel.  */
		     IO$_DEACCESS,        /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     0, 0, 0, 0, 0, 0);

  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to close socket\n");
      LIB$SIGNAL (status);
    }

  /* Deassign I/O channel to network device.  */
  status = sys$dassgn (conn_channel);

  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to deassign I/O channel\n");
      LIB$SIGNAL (status);
    }
}

/* Mark a page as R/W.  Return old rights.  */

static unsigned int
page_set_rw (unsigned __int64 startva, unsigned __int64 len,
	     unsigned int *oldprot)
{
  unsigned int status;
  unsigned __int64 retva;
  unsigned __int64 retlen;

  status = SYS$SETPRT_64 ((void *)startva, len, PSL$C_USER, PRT$C_UW,
			  (void *)&retva, &retlen, oldprot);
  return status;
}

/* Restore page rights.  */

static void
page_restore_rw (unsigned __int64 startva, unsigned __int64 len,
		unsigned int prot)
{
  unsigned int status;
  unsigned __int64 retva;
  unsigned __int64 retlen;
  unsigned int oldprot;

  status = SYS$SETPRT_64 ((void *)startva, len, PSL$C_USER, prot,
			  (void *)&retva, &retlen, &oldprot);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
}

/* Get the TEB (thread environment block).  */

static pthread_t
get_teb (void)
{
  return (pthread_t)__getReg (_IA64_REG_TP);
}

/* Enable thread scheduling if VAL is true.  */

static unsigned int
set_thread_scheduling (int val)
{
  struct dbgext_control_block blk;
  unsigned int status;

  if (!dbgext_func)
    return 0;

  blk.dbgext$w_function_code = DBGEXT$K_STOP_ALL_OTHER_TASKS;
  blk.dbgext$w_facility_id = CMA$_FACILITY;
  blk.dbgext$l_stop_value = val;

  status = dbgext_func (&blk);
  if (!(status & STS$M_SUCCESS))
    {
      TERM_FAO ("set_thread_scheduling error, val=!SL, status=!XL!/",
		val, blk.dbgext$l_status);
      lib$signal (status);
    }

  return blk.dbgext$l_stop_value;
}

/* Get next thread (after THR).  Start with 0.  */

static unsigned int
thread_next (unsigned int thr)
{
  struct dbgext_control_block blk;
  unsigned int status;

  if (!dbgext_func)
    return 0;

  blk.dbgext$w_function_code = DBGEXT$K_NEXT_TASK;
  blk.dbgext$w_facility_id = CMA$_FACILITY;
  blk.dbgext$l_ada_flags = 0;
  blk.dbgext$l_task_value = thr;

  status = dbgext_func (&blk);
  if (!(status & STS$M_SUCCESS))
    lib$signal (status);

  return blk.dbgext$l_task_value;
}

/* Pthread Debug callbacks.  */

static int
read_callback (pthreadDebugClient_t context,
	       pthreadDebugTargetAddr_t addr,
	       pthreadDebugAddr_t buf,
	       size_t size)
{
  if (trace_pthreaddbg)
    TERM_FAO ("read_callback (!XH, !XH, !SL)!/", addr, buf, size);
  ots$move (buf, size, addr);
  return 0;
}

static int
write_callback (pthreadDebugClient_t context,
		pthreadDebugTargetAddr_t addr,
		pthreadDebugLongConstAddr_t buf,
		size_t size)
{
  if (trace_pthreaddbg)
    TERM_FAO ("write_callback (!XH, !XH, !SL)!/", addr, buf, size);
  ots$move (addr, size, buf);
  return 0;
}

static int
suspend_callback (pthreadDebugClient_t context)
{
  /* Always suspended.  */
  return 0;
}

static int
resume_callback (pthreadDebugClient_t context)
{
  /* So no need to resume.  */
  return 0;
}

static int
kthdinfo_callback (pthreadDebugClient_t context,
		   pthreadDebugKId_t kid,
		   pthreadDebugKThreadInfo_p thread_info)
{
  if (trace_pthreaddbg)
    term_puts ("kthinfo_callback");
  return ENOSYS;
}

static int
hold_callback (pthreadDebugClient_t context,
	       pthreadDebugKId_t kid)
{
  if (trace_pthreaddbg)
    term_puts ("hold_callback");
  return ENOSYS;
}

static int
unhold_callback (pthreadDebugClient_t context,
		 pthreadDebugKId_t kid)
{
  if (trace_pthreaddbg)
    term_puts ("unhold_callback");
  return ENOSYS;
}

static int
getfreg_callback (pthreadDebugClient_t context,
		  pthreadDebugFregs_t *reg,
		  pthreadDebugKId_t kid)
{
  if (trace_pthreaddbg)
    term_puts ("getfreg_callback");
  return ENOSYS;
}

static int
setfreg_callback (pthreadDebugClient_t context,
		  const pthreadDebugFregs_t *reg,
		  pthreadDebugKId_t kid)
{
  if (trace_pthreaddbg)
    term_puts ("setfreg_callback");
  return ENOSYS;
}

static int
getreg_callback (pthreadDebugClient_t context,
		 pthreadDebugRegs_t *reg,
		 pthreadDebugKId_t kid)
{
  if (trace_pthreaddbg)
    term_puts ("getreg_callback");
  return ENOSYS;
}

static int
setreg_callback (pthreadDebugClient_t context,
		 const pthreadDebugRegs_t *reg,
		 pthreadDebugKId_t kid)
{
  if (trace_pthreaddbg)
    term_puts ("setreg_callback");
  return ENOSYS;
}

static int
output_callback (pthreadDebugClient_t context, 
		 pthreadDebugConstString_t line)
{
  term_puts (line);
  term_putnl ();
  return 0;
}

static int
error_callback (pthreadDebugClient_t context, 
		 pthreadDebugConstString_t line)
{
  term_puts (line);
  term_putnl ();
  return 0;
}

static pthreadDebugAddr_t
malloc_callback (pthreadDebugClient_t caller_context, size_t size)
{
  unsigned int status;
  unsigned int res;
  int len;

  len = size + 16;
  status = lib$get_vm (&len, &res, 0);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
  if (trace_pthreaddbg)
    TERM_FAO ("malloc_callback (!UL) -> !XA!/", size, res);
  *(unsigned int *)res = len;
  return (char *)res + 16;
}

static void
free_callback (pthreadDebugClient_t caller_context, pthreadDebugAddr_t address)
{
  unsigned int status;
  unsigned int res;
  int len;

  res = (unsigned int)address - 16;
  len = *(unsigned int *)res;
  if (trace_pthreaddbg)
    TERM_FAO ("free_callback (!XA)!/", address);
  status = lib$free_vm (&len, &res, 0);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
}

static int
speckthd_callback (pthreadDebugClient_t caller_context,
		   pthreadDebugSpecialType_t type,
		   pthreadDebugKId_t *kernel_tid)
{
  return ENOTSUP;
}

static pthreadDebugCallbacks_t pthread_debug_callbacks = {
  PTHREAD_DEBUG_VERSION,
  read_callback,
  write_callback,
  suspend_callback,
  resume_callback,
  kthdinfo_callback,
  hold_callback,
  unhold_callback,
  getfreg_callback,
  setfreg_callback,
  getreg_callback,
  setreg_callback,
  output_callback,
  error_callback,
  malloc_callback,
  free_callback,
  speckthd_callback
};

/* Name of the pthread shared library.  */
static const $DESCRIPTOR (pthread_rtl_desc, "PTHREAD$RTL");

/* List of symbols to extract from pthread debug library.  */
struct pthread_debug_entry
{
  const unsigned int namelen;
  const __char_ptr32 name;
  __void_ptr32 func;
};

#define DEBUG_ENTRY(str) { sizeof(str) - 1, str, 0 }

static struct pthread_debug_entry pthread_debug_entries[] = {
  DEBUG_ENTRY("pthreadDebugContextInit"),
  DEBUG_ENTRY("pthreadDebugThdSeqInit"),
  DEBUG_ENTRY("pthreadDebugThdSeqNext"),
  DEBUG_ENTRY("pthreadDebugThdSeqDestroy"),
  DEBUG_ENTRY("pthreadDebugThdGetInfo"),
  DEBUG_ENTRY("pthreadDebugThdGetInfoAddr"),
  DEBUG_ENTRY("pthreadDebugThdGetReg"),
  DEBUG_ENTRY("pthreadDebugCmd")
};

/* Pthread debug context.  */
static pthreadDebugContext_t debug_context;

/* Wrapper around pthread debug entry points.  */

static int
pthread_debug_thd_seq_init (pthreadDebugId_t *id)
{
  return ((int (*)())pthread_debug_entries[1].func)
    (debug_context, id);
}

static int
pthread_debug_thd_seq_next (pthreadDebugId_t *id)
{
  return ((int (*)())pthread_debug_entries[2].func)
    (debug_context, id);
}

static int
pthread_debug_thd_seq_destroy (void)
{
  return ((int (*)())pthread_debug_entries[3].func)
    (debug_context);
}

static int
pthread_debug_thd_get_info (pthreadDebugId_t id,
			    pthreadDebugThreadInfo_t *info)
{
  return ((int (*)())pthread_debug_entries[4].func)
    (debug_context, id, info);
}

static int
pthread_debug_thd_get_info_addr (pthread_t thr,
				 pthreadDebugThreadInfo_t *info)
{
  return ((int (*)())pthread_debug_entries[5].func)
    (debug_context, thr, info);
}

static int
pthread_debug_thd_get_reg (pthreadDebugId_t thr,
			   pthreadDebugRegs_t *regs)
{
  return ((int (*)())pthread_debug_entries[6].func)
    (debug_context, thr, regs);
}

static int
stub_pthread_debug_cmd (const char *cmd)
{
  return ((int (*)())pthread_debug_entries[7].func)
    (debug_context, cmd);
}

/* Show all the threads.  */

static void
threads_show (void)
{
  pthreadDebugId_t id;
  pthreadDebugThreadInfo_t info;
  int res;

  res = pthread_debug_thd_seq_init (&id);
  if (res != 0)
    {
      TERM_FAO ("seq init failed, res=!SL!/", res);
      return;
    }
  while (1)
    {
      if (pthread_debug_thd_get_info (id, &info) != 0)
	{
	  TERM_FAO ("thd_get_info !SL failed!/", id);
	  break;
	}
      if (pthread_debug_thd_seq_next (&id) != 0)
	break;
    }
  pthread_debug_thd_seq_destroy ();
}

/* Initialize pthread support.  */

static void
threads_init (void)
{
  static const $DESCRIPTOR (dbgext_desc, "PTHREAD$DBGEXT");
  static const $DESCRIPTOR (pthread_debug_desc, "PTHREAD$DBGSHR");
  static const $DESCRIPTOR (dbgsymtable_desc, "PTHREAD_DBG_SYMTABLE");
  int pthread_dbgext;
  int status;
  void *dbg_symtable;
  int i;
  void *caller_context = 0;

  status = lib$find_image_symbol
    ((void *) &pthread_rtl_desc, (void *) &dbgext_desc,
     (int *) &dbgext_func);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
  
  status = lib$find_image_symbol
    ((void *) &pthread_rtl_desc, (void *) &dbgsymtable_desc,
     (int *) &dbg_symtable);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);

  /* Find entry points in pthread_debug.  */
  for (i = 0;
       i < sizeof (pthread_debug_entries) / sizeof (pthread_debug_entries[0]);
       i++)
    {
      struct dsc$descriptor_s sym =
	{ pthread_debug_entries[i].namelen,
	  DSC$K_DTYPE_T, DSC$K_CLASS_S,
	  pthread_debug_entries[i].name };
      status = lib$find_image_symbol
	((void *) &pthread_debug_desc, (void *) &sym,
	 (int *) &pthread_debug_entries[i].func);
      if (!(status & STS$M_SUCCESS))
	lib$signal (status);
    }

  if (trace_pthreaddbg)
    TERM_FAO ("debug symtable: !XH!/", dbg_symtable);
  status = ((int (*)()) pthread_debug_entries[0].func)
    (&caller_context, &pthread_debug_callbacks, dbg_symtable, &debug_context);
  if (status != 0)
    TERM_FAO ("cannot initialize pthread_debug: !UL!/", status);
  TERM_FAO ("pthread debug done!/", 0);
}

/* Convert an hexadecimal character to a nibble.  Return -1 in case of
   error.  */

static int
hex2nibble (unsigned char h)
{
  if (h >= '0' && h <= '9')
    return h - '0';
  if (h >= 'A' && h <= 'F')
    return h - 'A' + 10;
  if (h >= 'a' && h <= 'f')
    return h - 'a' + 10;
  return -1;
}

/* Convert an hexadecimal 2 character string to a byte.  Return -1 in case
   of error.  */

static int
hex2byte (const unsigned char *p)
{
  int h, l;

  h = hex2nibble (p[0]);
  l = hex2nibble (p[1]);
  if (h == -1 || l == -1)
    return -1;
  return (h << 4) | l;
}

/* Convert a byte V to a 2 character strings P.  */

static void
byte2hex (unsigned char *p, unsigned char v)
{
  p[0] = hex[v >> 4];
  p[1] = hex[v & 0xf];
}

/* Convert a quadword V to a 16 character strings P.  */

static void
quad2hex (unsigned char *p, unsigned __int64 v)
{
  int i;
  for (i = 0; i < 16; i++)
    {
      p[i] = hex[v >> 60];
      v <<= 4;
    }
}

static void
long2pkt (unsigned int v)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      gdb_buf[gdb_blen + i] = hex[(v >> 28) & 0x0f];
      v <<= 4;
    }
  gdb_blen += 8;
}

/* Generate an error packet.  */

static void
packet_error (unsigned int err)
{
  gdb_buf[1] = 'E';
  byte2hex (gdb_buf + 2, err);
  gdb_blen = 4;
}

/* Generate an OK packet.  */

static void
packet_ok (void)
{
  gdb_buf[1] = 'O';
  gdb_buf[2] = 'K';
  gdb_blen = 3;
}

/* Append a register to the packet.  */

static void
ireg2pkt (const unsigned char *p)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      byte2hex (gdb_buf + gdb_blen, p[i]);
      gdb_blen += 2;
    }
}

/* Append a C string (ASCIZ) to the packet.  */

static void
str2pkt (const char *str)
{
  while (*str)
    gdb_buf[gdb_blen++] = *str++;
}

/* Extract a number fro the packet.  */

static unsigned __int64
pkt2val (const unsigned char *pkt, unsigned int *pos)
{
  unsigned __int64 res = 0;
  unsigned int i;

  while (1)
    {
      int r = hex2nibble (pkt[*pos]);

      if (r < 0)
	return res;
      res = (res << 4) | r;
      (*pos)++;
    }
}

/* Append LEN bytes from B to the current gdb packet (encode in binary).  */

static void
mem2bin (const unsigned char *b, unsigned int len)
{
  unsigned int i;
  for (i = 0; i < len; i++)
    switch (b[i])
      {
      case '#':
      case '$':
      case '}':
      case '*':
      case 0:
	gdb_buf[gdb_blen++] = '}';
	gdb_buf[gdb_blen++] = b[i] ^ 0x20;
	break;
      default:
	gdb_buf[gdb_blen++] = b[i];
	break;
      }
}

/* Append LEN bytes from B to the current gdb packet (encode in hex).  */

static void
mem2hex (const unsigned char *b, unsigned int len)
{
  unsigned int i;
  for (i = 0; i < len; i++)
    {
      byte2hex (gdb_buf + gdb_blen, b[i]);
      gdb_blen += 2;
    }
}

/* Handle the 'q' packet.  */

static void
handle_q_packet (const unsigned char *pkt, unsigned int pktlen)
{
  /* For qfThreadInfo and qsThreadInfo.  */
  static unsigned int first_thread;
  static unsigned int last_thread;

  static const char xfer_uib[] = "qXfer:uib:read:";
#define XFER_UIB_LEN (sizeof (xfer_uib) - 1)
  static const char qfthreadinfo[] = "qfThreadInfo";
#define QFTHREADINFO_LEN (sizeof (qfthreadinfo) - 1)
  static const char qsthreadinfo[] = "qsThreadInfo";
#define QSTHREADINFO_LEN (sizeof (qsthreadinfo) - 1)
  static const char qthreadextrainfo[] = "qThreadExtraInfo,";
#define QTHREADEXTRAINFO_LEN (sizeof (qthreadextrainfo) - 1)
  static const char qsupported[] = "qSupported:";
#define QSUPPORTED_LEN (sizeof (qsupported) - 1)

  if (pktlen == 2 && pkt[1] == 'C')
    {
      /* Current thread.  */
      gdb_buf[0] = '$';
      gdb_buf[1] = 'Q';
      gdb_buf[2] = 'C';
      gdb_blen = 3;
      if (has_threads)
	long2pkt ((unsigned long) get_teb ());
      return;
    }
  else if (pktlen > XFER_UIB_LEN
      && ots$strcmp_eql (pkt, XFER_UIB_LEN, xfer_uib, XFER_UIB_LEN))
    {
      /* Get unwind information block.  */
      unsigned __int64 pc;
      unsigned int pos = XFER_UIB_LEN;
      unsigned int off;
      unsigned int len;
      union
      {
	unsigned char bytes[32];
	struct
	{
	  unsigned __int64 code_start_va;
	  unsigned __int64 code_end_va;
	  unsigned __int64 uib_start_va;
	  unsigned __int64 gp_value;
	} data;
      } uei;
      int res;
      int i;

      packet_error (0);

      pc = pkt2val (pkt, &pos);
      if (pkt[pos] != ':')
	return;
      pos++;
      off = pkt2val (pkt, &pos);
      if (pkt[pos] != ',' || off != 0)
	return;
      pos++;
      len = pkt2val (pkt, &pos);
      if (pkt[pos] != '#' || len != 0x20)
	return;

      res = SYS$GET_UNWIND_ENTRY_INFO (pc, &uei.data, 0);
      if (res == SS$_NODATA || res != SS$_NORMAL)
	ots$fill (uei.bytes, sizeof (uei.bytes), 0);

      if (trace_unwind)
	{
	  TERM_FAO ("Unwind request for !XH, status=!XL, uib=!XQ, GP=!XQ!/",
		    pc, res, uei.data.uib_start_va, uei.data.gp_value);
	}

      gdb_buf[0] = '$';
      gdb_buf[1] = 'l';
      gdb_blen = 2;
      mem2bin (uei.bytes, sizeof (uei.bytes));
    }
  else if (pktlen == QFTHREADINFO_LEN
	   && ots$strcmp_eql (pkt, QFTHREADINFO_LEN,
			      qfthreadinfo, QFTHREADINFO_LEN))
    {
      /* Get first thread(s).  */
      gdb_buf[0] = '$';
      gdb_buf[1] = 'm';
      gdb_blen = 2;

      if (!has_threads)
	{
	  gdb_buf[1] = 'l';
	  return;
	}
      first_thread = thread_next (0);
      last_thread = first_thread;
      long2pkt (first_thread);
    }
  else if (pktlen == QSTHREADINFO_LEN
	   && ots$strcmp_eql (pkt, QSTHREADINFO_LEN,
			      qsthreadinfo, QSTHREADINFO_LEN))
    {
      /* Get subsequent threads.  */
      gdb_buf[0] = '$';
      gdb_buf[1] = 'm';
      gdb_blen = 2;
      while (dbgext_func)
	{
	  unsigned int res;
	  res = thread_next (last_thread);
	  if (res == first_thread)
	    break;
	  if (gdb_blen > 2)
	    gdb_buf[gdb_blen++] = ',';
	  long2pkt (res);
	  last_thread = res;
	  if (gdb_blen > sizeof (gdb_buf) - 16)
	    break;
	}

      if (gdb_blen == 2)
	gdb_buf[1] = 'l';
    }
  else if (pktlen > QTHREADEXTRAINFO_LEN
	   && ots$strcmp_eql (pkt, QTHREADEXTRAINFO_LEN,
			      qthreadextrainfo, QTHREADEXTRAINFO_LEN))
    {
      /* Get extra info about a thread.  */
      pthread_t thr;
      unsigned int pos = QTHREADEXTRAINFO_LEN;
      pthreadDebugThreadInfo_t info;
      int res;

      packet_error (0);
      if (!has_threads)
	return;

      thr = (pthread_t) pkt2val (pkt, &pos);
      if (pkt[pos] != '#')
	return;
      res = pthread_debug_thd_get_info_addr (thr, &info);
      if (res != 0)
	{
	  TERM_FAO ("qThreadExtraInfo (!XH) failed: !SL!/", thr, res);
	  return;
	}
      gdb_buf[0] = '$';
      gdb_blen = 1;
      mem2hex ((const unsigned char *)"VMS-thread", 11);
    }
  else if (pktlen > QSUPPORTED_LEN
	   && ots$strcmp_eql (pkt, QSUPPORTED_LEN,
			      qsupported, QSUPPORTED_LEN))
    {
      /* Get supported features.  */
      pthread_t thr;
      unsigned int pos = QSUPPORTED_LEN;
      pthreadDebugThreadInfo_t info;
      int res;
      
      /* Ignore gdb features.  */
      gdb_buf[0] = '$';
      gdb_blen = 1;

      str2pkt ("qXfer:uib:read+");
      return;
    }
  else
    {
      if (trace_pkt)
	{
	  term_puts ("unknown <: ");
	  term_write ((char *)pkt, pktlen);
	  term_putnl ();
	}
      return;
    }
}

/* Handle the 'v' packet.  */

static int
handle_v_packet (const unsigned char *pkt, unsigned int pktlen)
{
  static const char vcontq[] = "vCont?";
#define VCONTQ_LEN (sizeof (vcontq) - 1)

  if (pktlen == VCONTQ_LEN
      && ots$strcmp_eql (pkt, VCONTQ_LEN, vcontq, VCONTQ_LEN))
    {
      gdb_buf[0] = '$';
      gdb_blen = 1;

      str2pkt ("vCont;c;s");
      return 0;
    }
  else
    {
      if (trace_pkt)
	{
	  term_puts ("unknown <: ");
	  term_write ((char *)pkt, pktlen);
	  term_putnl ();
	}
      return 0;
    }
}

/* Get regs for the selected thread.  */

static struct ia64_all_regs *
get_selected_regs (void)
{
  pthreadDebugRegs_t regs;
  int res;

  if (selected_thread == 0 || selected_thread == get_teb ())
    return &excp_regs;

  if (selected_thread == sel_regs_pthread)
    return &sel_regs;

  /* Read registers.  */
  res = pthread_debug_thd_get_reg (selected_id, &regs);
  if (res != 0)
    {
      /* FIXME: return NULL ?  */
      return &excp_regs;
    }
  sel_regs_pthread = selected_thread;
  sel_regs.gr[1].v = regs.gp;
  sel_regs.gr[4].v = regs.r4;
  sel_regs.gr[5].v = regs.r5;
  sel_regs.gr[6].v = regs.r6;
  sel_regs.gr[7].v = regs.r7;
  sel_regs.gr[12].v = regs.sp;
  sel_regs.br[0].v = regs.rp;
  sel_regs.br[1].v = regs.b1;
  sel_regs.br[2].v = regs.b2;
  sel_regs.br[3].v = regs.b3;
  sel_regs.br[4].v = regs.b4;
  sel_regs.br[5].v = regs.b5;
  sel_regs.ip.v = regs.ip;
  sel_regs.bsp.v = regs.bspstore; /* FIXME: it is correct ?  */
  sel_regs.pfs.v = regs.pfs;
  sel_regs.pr.v = regs.pr;
  return &sel_regs;
}

/* Create a status packet.  */

static void
packet_status (void)
{
  gdb_blen = 0;
  if (has_threads)
    {
      str2pkt ("$T05thread:");
      long2pkt ((unsigned long) get_teb ());
      gdb_buf[gdb_blen++] = ';';
    }
  else
    str2pkt ("$S05");
}

/* Return 1 to continue.  */

static int
handle_packet (unsigned char *pkt, unsigned int len)
{
  unsigned int pos;

  /* By default, reply unsupported.  */
  gdb_buf[0] = '$';
  gdb_blen = 1;

  pos = 1;
  switch (pkt[0])
    {
    case '?':
      if (len == 1)
	{
	  packet_status ();
	  return 0;
	}
      break;
    case 'c':
      if (len == 1)
	{
	  /* Clear psr.ss.  */
	  excp_regs.psr.v &= ~(unsigned __int64)PSR$M_SS;
	  return 1;
	}
      else
	packet_error (0);
      break;
    case 'g':
      if (len == 1)
	{
	  unsigned int i;
	  struct ia64_all_regs *regs = get_selected_regs ();
	  unsigned char *p = regs->gr[0].b;

	  for (i = 0; i < 8 * 32; i++)
	    byte2hex (gdb_buf + 1 + 2 * i, p[i]);
	  gdb_blen += 2 * 8 * 32;
	  return 0;
	}
      break;
    case 'H':
      if (pkt[1] == 'g')
	{
	  int res;
	  unsigned __int64 val;
	  pthreadDebugThreadInfo_t info;
	  
	  pos++;
	  val = pkt2val (pkt, &pos);
	  if (pos != len)
	    {
	      packet_error (0);
	      return 0;
	    }
	  if (val == 0)
	    {
	      /* Default one.  */
	      selected_thread = get_teb ();
	      selected_id = 0;
	    }
	  else if (!has_threads)
	    {
	      packet_error (0);
	      return 0;
	    }
	  else
	    {
	      res = pthread_debug_thd_get_info_addr ((pthread_t) val, &info);
	      if (res != 0)
		{
		  TERM_FAO ("qThreadExtraInfo (!XH) failed: !SL!/", val, res);
		  packet_error (0);
		  return 0;
		}
	      selected_thread = info.teb;
	      selected_id = info.sequence;
	    }
	  packet_ok ();
	  break;
	}
      else if (pkt[1] == 'c'
	       && ((pkt[2] == '-' && pkt[3] == '1' && len == 4)
		   || (pkt[2] == '0' && len == 3)))
	{
	  /* Silently accept 'Hc0' and 'Hc-1'.  */
	  packet_ok ();
	  break;
	}
      else
	{
	  packet_error (0);
	  return 0;
	}
    case 'k':
      SYS$EXIT (SS$_NORMAL);
      break;
    case 'm':
      {
	unsigned __int64 addr;
	unsigned __int64 paddr;
	unsigned int l;
	unsigned int i;

	addr = pkt2val (pkt, &pos);
	if (pkt[pos] != ',')
	  {
	    packet_error (0);
	    return 0;
	  }
	pos++;
	l = pkt2val (pkt, &pos);
	if (pkt[pos] != '#')
	  {
	    packet_error (0);
	    return 0;
	  }

	/* Check access.  */
	i = l + (addr & VMS_PAGE_MASK);
	paddr = addr & ~VMS_PAGE_MASK;
	while (1)
	  {
	    if (__prober (paddr, 0) != 1)
	      {
		packet_error (2);
		return 0;
	      }
	    if (i < VMS_PAGE_SIZE)
	      break;
	    i -= VMS_PAGE_SIZE;
	    paddr += VMS_PAGE_SIZE;
	  }

	/* Transfer.  */
	for (i = 0; i < l; i++)
	  byte2hex (gdb_buf + 1 + 2 * i, ((unsigned char *)addr)[i]);
	gdb_blen += 2 * l;
      }
      break;
    case 'M':
      {
	unsigned __int64 addr;
	unsigned __int64 paddr;
	unsigned int l;
	unsigned int i;
	unsigned int oldprot;

	addr = pkt2val (pkt, &pos);
	if (pkt[pos] != ',')
	  {
	    packet_error (0);
	    return 0;
	  }
	pos++;
	l = pkt2val (pkt, &pos);
	if (pkt[pos] != ':')
	  {
	    packet_error (0);
	    return 0;
	  }
	pos++;
	page_set_rw (addr, l, &oldprot);

	/* Check access.  */
	i = l + (addr & VMS_PAGE_MASK);
	paddr = addr & ~VMS_PAGE_MASK;
	while (1)
	  {
	    if (__probew (paddr, 0) != 1)
	      {
		page_restore_rw (addr, l, oldprot);
		return 0;
	      }
	    if (i < VMS_PAGE_SIZE)
	      break;
	    i -= VMS_PAGE_SIZE;
	    paddr += VMS_PAGE_SIZE;
	  }

	/* Write.  */
	for (i = 0; i < l; i++)
	  {
	    int v = hex2byte (pkt + pos);
	    pos += 2;
	    ((unsigned char *)addr)[i] = v;
	  }

	/* Sync caches.  */
	for (i = 0; i < l; i += 15)
	  __fc (addr + i);
	__fc (addr + l);

	page_restore_rw (addr, l, oldprot);
	packet_ok ();
      }
      break;
    case 'p':
      {
	unsigned int num = 0;
	unsigned int i;
	struct ia64_all_regs *regs = get_selected_regs ();

	num = pkt2val (pkt, &pos);
	if (pos != len)
	  {
	    packet_error (0);
	    return 0;
	  }

	switch (num)
	  {
	  case IA64_IP_REGNUM:
	    ireg2pkt (regs->ip.b);
	    break;
	  case IA64_BR0_REGNUM:
	    ireg2pkt (regs->br[0].b);
	    break;
	  case IA64_PSR_REGNUM:
	    ireg2pkt (regs->psr.b);
	    break;
	  case IA64_BSP_REGNUM:
	    ireg2pkt (regs->bsp.b);
	    break;
	  case IA64_CFM_REGNUM:
	    ireg2pkt (regs->cfm.b);
	    break;
	  case IA64_PFS_REGNUM:
	    ireg2pkt (regs->pfs.b);
	    break;
	  case IA64_PR_REGNUM:
	    ireg2pkt (regs->pr.b);
	    break;
	  default:
	    TERM_FAO ("gdbserv: unhandled reg !UW!/", num);
	    packet_error (0);
	    return 0;
	  }
      }
      break;
    case 'q':
      handle_q_packet (pkt, len);
      break;
    case 's':
      if (len == 1)
	{
	  /* Set psr.ss.  */
	  excp_regs.psr.v |= (unsigned __int64)PSR$M_SS;
	  return 1;
	}
      else
	packet_error (0);
      break;
    case 'T':
      /* Thread status.  */
      if (!has_threads)
	{
	  packet_ok ();
	  break;
	}
      else
	{
	  int res;
	  unsigned __int64 val;
	  unsigned int fthr, thr;
	  
	  val = pkt2val (pkt, &pos);
	  /* Default is error (but only after parsing is complete).  */
	  packet_error (0);
	  if (pos != len)
	    break;

	  /* Follow the list.  This makes a O(n2) algorithm, but we don't really
	     have the choice.  Note that pthread_debug_thd_get_info_addr
	     doesn't look reliable.  */
	  fthr = thread_next (0);
	  thr = fthr;
	  do
	    {
	      if (val == thr)
		{
		  packet_ok ();
		  break;
		}
	      thr = thread_next (thr);
	    }
	  while (thr != fthr);
	}
      break;
    case 'v':
      return handle_v_packet (pkt, len);
      break;
    case 'V':
      if (len > 3 && pkt[1] == 'M' && pkt[2] == 'S' && pkt[3] == ' ')
	{
	  /* Temporary extension.  */
	  if (has_threads)
	    {
	      pkt[len] = 0;
	      stub_pthread_debug_cmd ((char *)pkt + 4);
	      packet_ok ();
	    }
	  else
	    packet_error (0);
	}
      break;
    default:
      if (trace_pkt)
	{
	  term_puts ("unknown <: ");
	  term_write ((char *)pkt, len);
	  term_putnl ();
	}
      break;
    }
  return 0;
}

/* Raw write to gdb.  */

static void
sock_write (const unsigned char *buf, int len)
{
  struct _iosb iosb;
  unsigned int status;

  /* Write data to connection.  */
  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
		     conn_channel,        /* I/O channel.  */
		     IO$_WRITEVBLK,       /* I/O function code.  */
		     &iosb,               /* I/O status block.  */
		     0,                   /* Ast service routine.  */
		     0,                   /* Ast parameter.  */
		     (char *)buf,         /* P1 - buffer address.  */
		     len,                 /* P2 - buffer length.  */
		     0, 0, 0, 0);
  if (status & STS$M_SUCCESS)
    status = iosb.iosb$w_status;
  if (!(status & STS$M_SUCCESS))
    {
      term_puts ("Failed to write data to gdb\n");
      LIB$SIGNAL (status);
    }
}

/* Compute the checksum and send the packet.  */

static void
send_pkt (void)
{
  unsigned char chksum = 0;
  unsigned int i;

  for (i = 1; i < gdb_blen; i++)
    chksum += gdb_buf[i];

  gdb_buf[gdb_blen] = '#';
  byte2hex (gdb_buf + gdb_blen + 1, chksum);

  sock_write (gdb_buf, gdb_blen + 3);

  if (trace_pkt > 1)
    {
      term_puts (">: ");
      term_write ((char *)gdb_buf, gdb_blen + 3);
      term_putnl ();
    }
}

/* Read and handle one command.  Return 1 is execution must resume.  */

static int
one_command (void)
{
  struct _iosb iosb;
  unsigned int status;
  unsigned int off;
  unsigned int dollar_off = 0;
  unsigned int sharp_off = 0;
  unsigned int cmd_off;
  unsigned int cmd_len;

  /* Wait for a packet.  */
  while (1)
    {
      off = 0;
      while (1)
	{
	  /* Read data from connection.  */
	  status = sys$qiow (EFN$C_ENF,           /* Event flag.  */
			     conn_channel,        /* I/O channel.  */
			     IO$_READVBLK,        /* I/O function code.  */
			     &iosb,               /* I/O status block.  */
			     0,                   /* Ast service routine.  */
			     0,                   /* Ast parameter.  */
			     gdb_buf + off,       /* P1 - buffer address.  */
			     sizeof (gdb_buf) - off, /* P2 - buffer leng.  */
			     0, 0, 0, 0);
	  if (status & STS$M_SUCCESS)
	    status = iosb.iosb$w_status;
	  if (!(status & STS$M_SUCCESS))
	    {
	      term_puts ("Failed to read data from connection\n" );
	      LIB$SIGNAL (status);
	    }

#ifdef RAW_DUMP
	  term_puts ("{: ");
	  term_write ((char *)gdb_buf + off, iosb.iosb$w_bcnt);
	  term_putnl ();
#endif

	  gdb_blen = off + iosb.iosb$w_bcnt;

	  if (off == 0)
	    {
	      /* Search for '$'.  */
	      for (dollar_off = 0; dollar_off < gdb_blen; dollar_off++)
		if (gdb_buf[dollar_off] == '$')
		  break;
	      if (dollar_off >= gdb_blen)
		{
		  /* Not found, discard the data.  */
		  off = 0;
		  continue;
		}
	      /* Search for '#'.  */
	      for (sharp_off = dollar_off + 1;
		   sharp_off < gdb_blen;
		   sharp_off++)
		if (gdb_buf[sharp_off] == '#')
		  break;
	    }
	  else if (sharp_off >= off)
	    {
	      /* Search for '#'.  */
	      for (; sharp_off < gdb_blen; sharp_off++)
		if (gdb_buf[sharp_off] == '#')
		  break;
	    }

	  /* Got packet with checksum.  */
	  if (sharp_off + 2 <= gdb_blen)
	    break;

	  off = gdb_blen;
	  if (gdb_blen == sizeof (gdb_buf))
	    {
	      /* Packet too large, discard.  */
	      off = 0;
	    }
	}

      /* Validate and acknowledge a packet.  */
      {
	unsigned char chksum = 0;
	unsigned int i;
	int v;

	for (i = dollar_off + 1; i < sharp_off; i++)
	  chksum += gdb_buf[i];
	v = hex2byte (gdb_buf + sharp_off + 1);
	if (v != chksum)
	  {
	    term_puts ("Discard bad checksum packet\n");
	    continue;
	  }
	else
	  {
	    sock_write ((const unsigned char *)"+", 1);
	    break;
	  }
      }
    }

  if (trace_pkt > 1)
    {
      term_puts ("<: ");
      term_write ((char *)gdb_buf + dollar_off, sharp_off - dollar_off + 1);
      term_putnl ();
    }

  cmd_off = dollar_off + 1;
  cmd_len = sharp_off - dollar_off - 1;

  if (handle_packet (gdb_buf + dollar_off + 1, sharp_off - dollar_off - 1) == 1)
    return 1;

  send_pkt ();
  return 0;
}

/* Display the condition given by SIG64.  */

static void
display_excp (struct chf64$signal_array *sig64, struct chf$mech_array *mech)
{
  unsigned int status;
  char msg[160];
  unsigned short msglen;
  $DESCRIPTOR (msg_desc, msg);
  unsigned char outadr[4];

  status = SYS$GETMSG (sig64->chf64$q_sig_name, &msglen, &msg_desc, 0, outadr);
  if (status & STS$M_SUCCESS)
    {
      char msg2[160];
      unsigned short msg2len;
      struct dsc$descriptor_s msg2_desc =
	{ sizeof (msg2), DSC$K_DTYPE_T, DSC$K_CLASS_S, msg2};
      msg_desc.dsc$w_length = msglen;
      status = SYS$FAOL_64 (&msg_desc, &msg2len, &msg2_desc,
			    &sig64->chf64$q_sig_arg1);
      if (status & STS$M_SUCCESS)
	term_write (msg2, msg2len);
    }
  else
    term_puts ("no message");
  term_putnl ();

  if (trace_excp > 1)
    {
      TERM_FAO (" Frame: !XH, Depth: !4SL, Esf: !XH!/",
		mech->chf$q_mch_frame, mech->chf$q_mch_depth,
		mech->chf$q_mch_esf_addr);
    }
}

/* Get all registers from current thread.  */

static void
read_all_registers (struct chf$mech_array *mech)
{
  struct _intstk *intstk =
    (struct _intstk *)mech->chf$q_mch_esf_addr;
  struct chf64$signal_array *sig64 =
    (struct chf64$signal_array *)mech->chf$ph_mch_sig64_addr;
  unsigned int cnt = sig64->chf64$w_sig_arg_count;
  unsigned __int64 pc = (&sig64->chf64$q_sig_name)[cnt - 2];

  excp_regs.ip.v = pc;
  excp_regs.psr.v = intstk->intstk$q_ipsr;
  /* GDB and linux expects bsp to point after the current register frame.
     Adjust.  */
  {
    unsigned __int64 bsp = intstk->intstk$q_bsp;
    unsigned int sof = intstk->intstk$q_ifs & 0x7f;
    unsigned int delta = ((bsp >> 3) & 0x3f) + sof;
    excp_regs.bsp.v = bsp + ((sof + delta / 0x3f) << 3);
  }
  excp_regs.cfm.v = intstk->intstk$q_ifs & 0x3fffffffff;
  excp_regs.pfs.v = intstk->intstk$q_pfs;
  excp_regs.pr.v = intstk->intstk$q_preds;
  excp_regs.gr[0].v = 0;
  excp_regs.gr[1].v = intstk->intstk$q_gp;
  excp_regs.gr[2].v = intstk->intstk$q_r2;
  excp_regs.gr[3].v = intstk->intstk$q_r3;
  excp_regs.gr[4].v = intstk->intstk$q_r4;
  excp_regs.gr[5].v = intstk->intstk$q_r5;
  excp_regs.gr[6].v = intstk->intstk$q_r6;
  excp_regs.gr[7].v = intstk->intstk$q_r7;
  excp_regs.gr[8].v = intstk->intstk$q_r8;
  excp_regs.gr[9].v = intstk->intstk$q_r9;
  excp_regs.gr[10].v = intstk->intstk$q_r10;
  excp_regs.gr[11].v = intstk->intstk$q_r11;
  excp_regs.gr[12].v = (unsigned __int64)intstk + intstk->intstk$l_stkalign;
  excp_regs.gr[13].v = intstk->intstk$q_r13;
  excp_regs.gr[14].v = intstk->intstk$q_r14;
  excp_regs.gr[15].v = intstk->intstk$q_r15;
  excp_regs.gr[16].v = intstk->intstk$q_r16;
  excp_regs.gr[17].v = intstk->intstk$q_r17;
  excp_regs.gr[18].v = intstk->intstk$q_r18;
  excp_regs.gr[19].v = intstk->intstk$q_r19;
  excp_regs.gr[20].v = intstk->intstk$q_r20;
  excp_regs.gr[21].v = intstk->intstk$q_r21;
  excp_regs.gr[22].v = intstk->intstk$q_r22;
  excp_regs.gr[23].v = intstk->intstk$q_r23;
  excp_regs.gr[24].v = intstk->intstk$q_r24;
  excp_regs.gr[25].v = intstk->intstk$q_r25;
  excp_regs.gr[26].v = intstk->intstk$q_r26;
  excp_regs.gr[27].v = intstk->intstk$q_r27;
  excp_regs.gr[28].v = intstk->intstk$q_r28;
  excp_regs.gr[29].v = intstk->intstk$q_r29;
  excp_regs.gr[30].v = intstk->intstk$q_r30;
  excp_regs.gr[31].v = intstk->intstk$q_r31;
  excp_regs.br[0].v = intstk->intstk$q_b0;
  excp_regs.br[1].v = intstk->intstk$q_b1;
  excp_regs.br[2].v = intstk->intstk$q_b2;
  excp_regs.br[3].v = intstk->intstk$q_b3;
  excp_regs.br[4].v = intstk->intstk$q_b4;
  excp_regs.br[5].v = intstk->intstk$q_b5;
  excp_regs.br[6].v = intstk->intstk$q_b6;
  excp_regs.br[7].v = intstk->intstk$q_b7;
}

/* Write all registers to current thread.  FIXME: not yet complete.  */

static void
write_all_registers (struct chf$mech_array *mech)
{
  struct _intstk *intstk =
    (struct _intstk *)mech->chf$q_mch_esf_addr;

  intstk->intstk$q_ipsr = excp_regs.psr.v;
}

/* Do debugging.  Report status to gdb and execute commands.  */

static void
do_debug (struct chf$mech_array *mech)
{
  struct _intstk *intstk =
    (struct _intstk *)mech->chf$q_mch_esf_addr;
  unsigned int old_ast;
  unsigned int old_sch;
  unsigned int status;

  /* Disable ast.  */
  status = sys$setast (0);
  switch (status)
    {
    case SS$_WASCLR:
      old_ast = 0;
      break;
    case SS$_WASSET:
      old_ast = 1;
      break;
    default:
      /* Should never happen!  */
      lib$signal (status);
    }

  /* Disable thread scheduling.  */
  if (has_threads)
    old_sch = set_thread_scheduling (0);

  read_all_registers (mech);

  /* Send stop reply packet.  */
  packet_status ();
  send_pkt ();

  while (one_command () == 0)
    ;

  write_all_registers (mech);

  /* Re-enable scheduling.  */
  if (has_threads)
    set_thread_scheduling (old_sch);

  /* Re-enable AST.  */
  status = sys$setast (old_ast);
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);
}

/* The condition handler.  That's the core of the stub.  */

static int
excp_handler (struct chf$signal_array *sig,
	      struct chf$mech_array *mech)
{
  struct chf64$signal_array *sig64 =
    (struct chf64$signal_array *)mech->chf$ph_mch_sig64_addr;
  unsigned int code = sig->chf$l_sig_name & STS$M_COND_ID;
  unsigned int cnt = sig64->chf64$w_sig_arg_count;
  unsigned __int64 pc;
  unsigned int ret;
  /* Self protection.  FIXME: Should be per thread ?  */
  static int in_handler = 0;

  /* Completely ignore some conditions (signaled indirectly by this stub).  */
  switch (code)
    {
    case LIB$_KEYNOTFOU & STS$M_COND_ID:
      return SS$_RESIGNAL_64;
    default:
      break;
    }

  /* Protect against recursion.  */
  in_handler++;
  if (in_handler > 1)
    {
      if (in_handler == 2)
	TERM_FAO ("gdbstub: exception in handler (pc=!XH)!!!/",
		  (&sig64->chf64$q_sig_name)[cnt - 2]);
      sys$exit (sig->chf$l_sig_name);
    }

  pc = (&sig64->chf64$q_sig_name)[cnt - 2];
  if (trace_excp)
    TERM_FAO ("excp_handler: code: !XL, pc=!XH!/", code, pc);

  /* If break on the entry point, restore the bundle.  */
  if (code == (SS$_BREAK & STS$M_COND_ID)
      && pc == entry_pc
      && entry_pc != 0)
    {
      static unsigned int entry_prot;

      if (trace_entry)
	term_puts ("initial entry breakpoint\n");
      page_set_rw (entry_pc, 16, &entry_prot);

      ots$move ((void *)entry_pc, 16, entry_saved);
      __fc (entry_pc);
      page_restore_rw (entry_pc, 16, entry_prot);
    }

  switch (code)
    {
    case SS$_ACCVIO & STS$M_COND_ID:
      if (trace_excp <= 1)
	display_excp (sig64, mech);
      /* Fall through.  */
    case SS$_BREAK  & STS$M_COND_ID:
    case SS$_OPCDEC & STS$M_COND_ID:
    case SS$_TBIT   & STS$M_COND_ID:
    case SS$_DEBUG  & STS$M_COND_ID:
      if (trace_excp > 1)
	{
	  int i;
	  struct _intstk *intstk =
	    (struct _intstk *)mech->chf$q_mch_esf_addr;

	  display_excp (sig64, mech);

	  TERM_FAO (" intstk: !XH!/", intstk);
	  for (i = 0; i < cnt + 1; i++)
	    TERM_FAO ("   !XH!/", ((unsigned __int64 *)sig64)[i]);
	}
      do_debug (mech);
      ret = SS$_CONTINUE_64;
      break;

    default:
      display_excp (sig64, mech);
      ret = SS$_RESIGNAL_64;
      break;
    }

  in_handler--;
  /* Discard selected thread registers.  */
  sel_regs_pthread = 0;
  return ret;
}

/* Setup internal trace flags according to GDBSTUB$TRACE logical.  */

static void
trace_init (void)
{
  unsigned int status, i, start;
  unsigned short len;
  char resstring[LNM$C_NAMLENGTH];
  static const $DESCRIPTOR (tabdesc, "LNM$DCL_LOGICAL");
  static const $DESCRIPTOR (logdesc, "GDBSTUB$TRACE");
  $DESCRIPTOR (sub_desc, resstring);
  ILE3 item_lst[2];

  item_lst[0].ile3$w_length = LNM$C_NAMLENGTH;
  item_lst[0].ile3$w_code = LNM$_STRING;
  item_lst[0].ile3$ps_bufaddr = resstring;
  item_lst[0].ile3$ps_retlen_addr = &len;
  item_lst[1].ile3$w_length = 0;
  item_lst[1].ile3$w_code = 0;

  /* Translate the logical name.  */
  status = SYS$TRNLNM (0,   		/* Attributes of the logical name.  */
		       (void *)&tabdesc,       /* Logical name table.  */
		       (void *)&logdesc,       /* Logical name.  */
		       0,              	       /* Access mode.  */
		       &item_lst);             /* Item list.  */
  if (status == SS$_NOLOGNAM)
    return;
  if (!(status & STS$M_SUCCESS))
    LIB$SIGNAL (status);

  start = 0;
  for (i = 0; i <= len; i++)
    {
      if ((i == len || resstring[i] == ',' || resstring[i] == ';')
	  && i != start)
	{
	  int j;

	  sub_desc.dsc$a_pointer = resstring + start;
	  sub_desc.dsc$w_length = i - start;

	  for (j = 0; j < NBR_DEBUG_FLAGS; j++)
	    if (str$case_blind_compare (&sub_desc, 
					(void *)&debug_flags[j].name) == 0)
	      {
		debug_flags[j].val++;
		break;
	      }
	  if (j == NBR_DEBUG_FLAGS)
	    TERM_FAO ("GDBSTUB$TRACE: unknown directive !AS!/", &sub_desc);

	  start = i + 1;
	}
    }

  TERM_FAO ("GDBSTUB$TRACE=!AD ->", len, resstring);
  for (i = 0; i < NBR_DEBUG_FLAGS; i++)
    if (debug_flags[i].val > 0)
      TERM_FAO (" !AS=!ZL", &debug_flags[i].name, debug_flags[i].val);
  term_putnl ();
}


/* Entry point.  */

static int
stub_start (unsigned __int64 *progxfer, void *cli_util,
	    EIHD *imghdr, IFD *imgfile,
	    unsigned int linkflag, unsigned int cliflag)
{
  static int initialized;
  int i;
  int cnt;
  int is_attached;
  IMCB *imcb;
  if (initialized)
    term_puts ("gdbstub: re-entry\n");
  else
    initialized = 1;

  /* When attached (through SS$_DEBUG condition), the number of arguments
     is 4 and PROGXFER is the PC at interruption.  */
  va_count (cnt);
  is_attached = cnt == 4;

  term_init ();

  /* Hello banner.  */
  term_puts ("Hello from gdb stub\n");

  trace_init ();

  if (trace_entry && !is_attached)
    {
      TERM_FAO ("xfer: !XH, imghdr: !XH, ifd: !XH!/",
		progxfer, imghdr, imgfile);
      for (i = -2; i < 8; i++)
	TERM_FAO ("  at !2SW: !XH!/", i, progxfer[i]);
    }

  /* Search for entry point.  */
  if (!is_attached)
    {
      entry_pc = 0;
      for (i = 0; progxfer[i]; i++)
	entry_pc = progxfer[i];

      if (trace_entry)
	{
	  if (entry_pc == 0)
	    {
	      term_puts ("No entry point\n");
	      return 0;
	    }
	  else
	    TERM_FAO ("Entry: !XH!/",entry_pc);
	}
    }
  else
    entry_pc = progxfer[0];

  has_threads = 0;
  for (imcb = ctl$gl_imglstptr->imcb$l_flink;
       imcb != ctl$gl_imglstptr;
       imcb = imcb->imcb$l_flink)
    {
      if (ots$strcmp_eql (pthread_rtl_desc.dsc$a_pointer,
			  pthread_rtl_desc.dsc$w_length,
			  imcb->imcb$t_log_image_name + 1,
			  imcb->imcb$t_log_image_name[0]))
	has_threads = 1;
			  
      if (trace_images)
	{
	  unsigned int j;
	  LDRIMG *ldrimg = imcb->imcb$l_ldrimg;
	  LDRISD *ldrisd;

	  TERM_FAO ("!XA-!XA ",
		    imcb->imcb$l_starting_address,
		    imcb->imcb$l_end_address);

	  switch (imcb->imcb$b_act_code)
	    {
	    case IMCB$K_MAIN_PROGRAM:
	      term_puts ("prog");
	      break;
	    case IMCB$K_MERGED_IMAGE:
	      term_puts ("mrge");
	      break;
	    case IMCB$K_GLOBAL_IMAGE_SECTION:
	      term_puts ("glob");
	      break;
	    default:
	      term_puts ("????");
	    }
	  TERM_FAO (" !AD !40AC!/",
		    1, "KESU" + (imcb->imcb$b_access_mode & 3),
		    imcb->imcb$t_log_image_name);

	  if ((long) ldrimg < 0 || trace_images < 2)
	    continue;
	  ldrisd = ldrimg->ldrimg$l_segments;
	  for (j = 0; j < ldrimg->ldrimg$l_segcount; j++)
	    {
	      unsigned int flags = ldrisd[j].ldrisd$i_flags;
	      term_puts ("   ");
	      term_putc (flags & 0x04 ? 'R' : '-');
	      term_putc (flags & 0x02 ? 'W' : '-');
	      term_putc (flags & 0x01 ? 'X' : '-');
	      term_puts (flags & 0x01000000 ? " Prot" : "     ");
	      term_puts (flags & 0x04000000 ? " Shrt" : "     ");
	      term_puts (flags & 0x08000000 ? " Shrd" : "     ");
	      TERM_FAO (" !XA-!XA!/",
			ldrisd[j].ldrisd$p_base,
			(unsigned __int64) ldrisd[j].ldrisd$p_base 
			+ ldrisd[j].ldrisd$i_len - 1);
	    }
	  ldrisd = ldrimg->ldrimg$l_dyn_seg;
	  if (ldrisd)
	    TERM_FAO ("   dynamic            !XA-!XA!/",
		      ldrisd->ldrisd$p_base,
		      (unsigned __int64) ldrisd->ldrisd$p_base 
		      + ldrisd->ldrisd$i_len - 1);
	}
    }

  if (has_threads)
    threads_init ();

  /* Wait for connection.  */
  sock_init ();

  /* Set primary exception vector.  */
  {
    unsigned int status;
    status = sys$setexv (0, excp_handler, PSL$C_USER, (__void_ptr32) &prevhnd);
    if (!(status & STS$M_SUCCESS))
      LIB$SIGNAL (status);
  }

  if (is_attached)
    {
      return excp_handler ((struct chf$signal_array *) progxfer[2],
			   (struct chf$mech_array *) progxfer[3]);
    }

  /* Change first instruction to set a breakpoint.  */
  {
    /*
      01 08 00 40 00 00 	[MII]       break.m 0x80001
      00 00 00 02 00 00 	            nop.i 0x0
      00 00 04 00       	            nop.i 0x0;;
    */
    static const unsigned char initbp[16] =
      { 0x01, 0x08, 0x00, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x02, 0x00, 0x00,
	0x00, 0x00, 0x04, 0x00 };
    unsigned int entry_prot;
    unsigned int status;
    
    status = page_set_rw (entry_pc, 16, &entry_prot);

    if (!(status & STS$M_SUCCESS))
      {
	if ((status & STS$M_COND_ID) == (SS$_NOT_PROCESS_VA & STS$M_COND_ID))
	  {
	    /* Cannot write here.  This can happen when pthreads are
	       used.  */
	    entry_pc = 0;
	    term_puts ("gdbstub: cannot set breakpoint on entry\n");
	  }
	else
	  LIB$SIGNAL (status);
      }
    
    if (entry_pc != 0)
      {
	ots$move (entry_saved, 16, (void *)entry_pc);
	ots$move ((void *)entry_pc, 16, (void *)initbp);
	__fc (entry_pc);
	page_restore_rw (entry_pc, 16, entry_prot);
      }
  }

  /* If it wasn't possible to set a breakpoint on the entry point,
     accept gdb commands now.  Note that registers are not updated.  */
  if (entry_pc == 0)
    {
      while (one_command () == 0)
	;
    }

  /* We will see!  */
  return SS$_CONTINUE;
}

/* Declare the entry point of this relocatable module.  */

struct xfer_vector
{
  __int64 impure_start;
  __int64 impure_end;
  int (*entry) ();
};

#pragma __extern_model save
#pragma __extern_model strict_refdef "XFER_PSECT"
struct xfer_vector xfer_vector = {0, 0, stub_start};
#pragma __extern_model restore
