/* Debug stub for Z80.

   Copyright (C) 2021-2024 Free Software Foundation, Inc.

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

/* Usage:
  1. Copy this file to project directory
  2. Configure it commenting/uncommenting macros below or define DBG_CONFIGURED
     and all required macros and then include this file to one of your C-source
     files.
  3. Implement getDebugChar() and putDebugChar(), functions must not return
     until data received or sent.
  4. Implement all optional functions used to toggle breakpoints/watchpoints,
     if supported. Do not write fuctions to toggle software breakpoints if
     you unsure (GDB will do itself).
  5. Implement serial port initialization routine called at program start.
  6. Add necessary debugger entry points to your program, for example:
	.org 0x08	;RST 8 handler
	jp _debug_swbreak
	...
	.org	0x66	;NMI handler
	jp	_debug_nmi
	...
	main_loop:
	halt
	call	isDbgInterrupt
	jr	z,101$
	ld	hl, 2	;EX_SIGINT
	push	hl
	call	_debug_exception
	101$:
	...
  7. Compile file using SDCC (supported ports are: z80, z180, z80n, gbz80 and
     ez80_z80), do not use --peep-asm option. For example:
	$ sdcc -mz80 --opt-code-size --max-allocs-per-node 50000 z80-stub.c
*/
/******************************************************************************\
			     Configuration
\******************************************************************************/
#ifndef DBG_CONFIGURED
/* Uncomment this line, if stub size is critical for you */
//#define DBG_MIN_SIZE

/* Comment this line out if software breakpoints are unsupported.
   If you have special function to toggle software breakpoints, then provide
   here name of these function. Expected prototype:
       int toggle_swbreak(int set, void *addr);
   function must return 0 on success. */
//#define DBG_SWBREAK toggle_swbreak
#define DBG_SWBREAK

/* Define if one of standard RST handlers is used as software
   breakpoint entry point */
//#define DBG_SWBREAK_RST 0x08

/* if platform supports hardware breakpoints then define following macro
   by name of function. Fuction must have next prototype:
     int toggle_hwbreak(int set, void *addr);
   function must return 0 on success. */
//#define DBG_HWBREAK toggle_hwbreak

/* if platform supports hardware watchpoints then define all or some of
   following macros by names of functions. Fuctions prototypes:
     int toggle_watch(int set, void *addr, size_t size);  // memory write watch
     int toggle_rwatch(int set, void *addr, size_t size); // memory read watch
     int toggle_awatch(int set, void *addr, size_t size); // memory access watch
   function must return 0 on success. */
//#define DBG_WWATCH toggle_watch
//#define DBG_RWATCH toggle_rwatch
//#define DBG_AWATCH toggle_awatch

/* Size of hardware breakpoint. Required to correct PC. */
#define DBG_HWBREAK_SIZE 0

/* Define following macro if you need custom memory read/write routine.
   Function should return non-zero on success, and zero on failure
   (for example, write to ROM area).
   Useful with overlays (bank switching).
   Do not forget to define:
   _ovly_table - overlay table
   _novlys - number of items in _ovly_table
   or
   _ovly_region_table - overlay regions table
   _novly_regions - number of items in _ovly_region_table

   _ovly_debug_prepare - function is called before overlay mapping
   _ovly_debug_event - function is called after overlay mapping
 */
//#define DBG_MEMCPY memcpy

/* define dedicated stack size if required */
//#define DBG_STACK_SIZE 256

/* max GDB packet size
   should be much less that DBG_STACK_SIZE because it will be allocated on stack
*/
#define DBG_PACKET_SIZE 150

/* Uncomment if required to use trampoline when resuming operation.
   Useful with dedicated stack when stack pointer do not point to the stack or
   stack is not writable */
//#define DBG_USE_TRAMPOLINE

/* Uncomment following macro to enable debug printing to debugger console */
//#define DBG_PRINT

#define DBG_NMI_EX EX_HWBREAK
#define DBG_INT_EX EX_SIGINT

/* Define following macro to statement, which will be executed after entering to
   stub_main function. Statement should include semicolon. */
//#define DBG_ENTER debug_enter();

/* Define following macro to instruction(s), which will be execute before return
   control to the program. It is useful when gdb-stub is placed in one of overlays.
   This procedure must not change any register. On top of stack before invocation
   will be return address of the program. */
//#define DBG_RESUME jp _restore_bank

/* Define following macro to the string containing memory map definition XML.
   GDB will use it to select proper breakpoint type (HW or SW). */
/*#define DBG_MEMORY_MAP "\
<memory-map>\
	<memory type=\"rom\" start=\"0x0000\" length=\"0x4000\"/>\
<!--	<memory type=\"flash\" start=\"0x4000\" length=\"0x4000\">\
		<property name=\"blocksize\">128</property>\
	</memory> -->\
	<memory type=\"ram\" start=\"0x8000\" length=\"0x8000\"/>\
</memory-map>\
"
*/
#endif /* DBG_CONFIGURED */
/******************************************************************************\
			     Public Interface
\******************************************************************************/

/* Enter to debug mode from software or hardware breakpoint.
   Assume address of next instruction after breakpoint call is on top of stack.
   Do JP _debug_swbreak or JP _debug_hwbreak from RST handler, for example.
 */
void debug_swbreak (void);
void debug_hwbreak (void);

/* Jump to this function from NMI handler. Just replace RETN instruction by
   JP _debug_nmi
   Use if NMI detects request to enter to debug mode.
 */
void debug_nmi (void);

/* Jump to this function from INT handler. Just replace EI+RETI instructions by
   JP _debug_int
   Use if INT detects request to enter to debug mode.
 */
void debug_int (void);

#define EX_SWBREAK	0	/* sw breakpoint */
#define EX_HWBREAK	-1	/* hw breakpoint */
#define EX_WWATCH	-2	/* memory write watch */
#define EX_RWATCH	-3	/* memory read watch */
#define EX_AWATCH	-4	/* memory access watch */
#define EX_SIGINT	2
#define EX_SIGTRAP	5
#define EX_SIGABRT	6
#define EX_SIGBUS	10
#define EX_SIGSEGV	11
/* or any standard *nix signal value */

/* Enter to debug mode (after receiving BREAK from GDB, for example)
 * Assume:
 *   program PC in (SP+0)
 *   caught signal in (SP+2)
 *   program SP is SP+4
 */
void debug_exception (int ex);

/* Prints to debugger console. */
void debug_print(const char *str);
/******************************************************************************\
			      Required functions
\******************************************************************************/

extern int getDebugChar (void);
extern void putDebugChar (int ch);

#ifdef DBG_SWBREAK
#define DO_EXPAND(VAL)  VAL ## 123456
#define EXPAND(VAL)     DO_EXPAND(VAL)

#if EXPAND(DBG_SWBREAK) != 123456
#define DBG_SWBREAK_PROC DBG_SWBREAK
extern int DBG_SWBREAK(int set, void *addr);
#endif

#undef EXPAND
#undef DO_EXPAND
#endif /* DBG_SWBREAK */

#ifdef DBG_HWBREAK
extern int DBG_HWBREAK(int set, void *addr);
#endif

#ifdef DBG_MEMCPY
extern void* DBG_MEMCPY (void *dest, const void *src, unsigned n);
#endif

#ifdef DBG_WWATCH
extern int DBG_WWATCH(int set, void *addr, unsigned size);
#endif

#ifdef DBG_RWATCH
extern int DBG_RWATCH(int set, void *addr, unsigned size);
#endif

#ifdef DBG_AWATCH
extern int DBG_AWATCH(int set, void *addr, unsigned size);
#endif

/******************************************************************************\
			       IMPLEMENTATION
\******************************************************************************/

#include <string.h>

#ifndef NULL
# define NULL (void*)0
#endif

typedef unsigned char byte;
typedef unsigned short word;

/* CPU state */
#ifdef __SDCC_ez80_adl
# define REG_SIZE 3
#else
# define REG_SIZE 2
#endif /* __SDCC_ez80_adl */

#define R_AF    (0*REG_SIZE)
#define R_BC    (1*REG_SIZE)
#define R_DE    (2*REG_SIZE)
#define R_HL    (3*REG_SIZE)
#define R_SP    (4*REG_SIZE)
#define R_PC    (5*REG_SIZE)

#ifndef __SDCC_gbz80
#define R_IX    (6*REG_SIZE)
#define R_IY    (7*REG_SIZE)
#define R_AF_   (8*REG_SIZE)
#define R_BC_   (9*REG_SIZE)
#define R_DE_   (10*REG_SIZE)
#define R_HL_   (11*REG_SIZE)
#define R_IR    (12*REG_SIZE)

#ifdef __SDCC_ez80_adl
#define R_SPS   (13*REG_SIZE)
#define NUMREGBYTES (14*REG_SIZE)
#else
#define NUMREGBYTES (13*REG_SIZE)
#endif /* __SDCC_ez80_adl */
#else
#define NUMREGBYTES (6*REG_SIZE)
#define FASTCALL
#endif /*__SDCC_gbz80 */
static byte state[NUMREGBYTES];

#if DBG_PACKET_SIZE < (NUMREGBYTES*2+5)
#error "Too small DBG_PACKET_SIZE"
#endif

#ifndef FASTCALL
#define FASTCALL __z88dk_fastcall
#endif

/* dedicated stack */
#ifdef DBG_STACK_SIZE

#define LOAD_SP	ld	sp, #_stack + DBG_STACK_SIZE

static char stack[DBG_STACK_SIZE];

#else

#undef DBG_USE_TRAMPOLINE
#define LOAD_SP

#endif

#ifndef DBG_ENTER
#define DBG_ENTER
#endif

#ifndef DBG_RESUME
#define DBG_RESUME ret
#endif

static signed char sigval;

static void stub_main (int sigval, int pc_adj);
static char high_hex (byte v) FASTCALL;
static char low_hex (byte v) FASTCALL;
static char put_packet_info (const char *buffer) FASTCALL;
static void save_cpu_state (void);
static void rest_cpu_state (void);

/******************************************************************************/
#ifdef DBG_SWBREAK
#ifdef DBG_SWBREAK_RST
#define DBG_SWBREAK_SIZE 1
#else
#define DBG_SWBREAK_SIZE 3
#endif
void
debug_swbreak (void) __naked
{
  __asm
	ld	(#_state + R_SP), sp
	LOAD_SP
	call	_save_cpu_state
	ld	hl, #-DBG_SWBREAK_SIZE
	push	hl
	ld	hl, #EX_SWBREAK
	push	hl
	call	_stub_main
	.globl	_break_handler
#ifdef DBG_SWBREAK_RST
_break_handler = DBG_SWBREAK_RST
#else
_break_handler = _debug_swbreak
#endif
  __endasm;
}
#endif /* DBG_SWBREAK */
/******************************************************************************/
#ifdef DBG_HWBREAK
#ifndef DBG_HWBREAK_SIZE
#define DBG_HWBREAK_SIZE 0
#endif /* DBG_HWBREAK_SIZE */
void
debug_hwbreak (void) __naked
{
  __asm
	ld	(#_state + R_SP), sp
	LOAD_SP
	call	_save_cpu_state
	ld	hl, #-DBG_HWBREAK_SIZE
	push	hl
	ld	hl, #EX_HWBREAK
	push	hl
	call	_stub_main
  __endasm;
}
#endif /* DBG_HWBREAK_SET */
/******************************************************************************/
void
debug_exception (int ex) __naked
{
  __asm
	ld	(#_state + R_SP), sp
	LOAD_SP
	call	_save_cpu_state
	ld	hl, #0
	push	hl
#ifdef __SDCC_gbz80
	ld	hl, #_state + R_SP
	ld	a, (hl+)
	ld	h, (hl)
	ld	l, a
#else
	ld	hl, (#_state + R_SP)
#endif
	inc	hl
	inc	hl
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	push	de
	call	_stub_main
  __endasm;
  (void)ex;
}
/******************************************************************************/
#ifndef __SDCC_gbz80
void
debug_nmi(void) __naked
{
  __asm
	ld	(#_state + R_SP), sp
	LOAD_SP
	call	_save_cpu_state
	ld	hl, #0	;pc_adj
	push	hl
	ld	hl, #DBG_NMI_EX
	push	hl
	ld	hl, #_stub_main
	push	hl
	push	hl
	retn
  __endasm;
}
#endif
/******************************************************************************/
void
debug_int(void) __naked
{
  __asm
	ld	(#_state + R_SP), sp
	LOAD_SP
	call	_save_cpu_state
	ld	hl, #0	;pc_adj
	push	hl
	ld	hl, #DBG_INT_EX
	push	hl
	ld	hl, #_stub_main
	push	hl
	push	hl
	ei
	reti
  __endasm;
}
/******************************************************************************/
#ifdef DBG_PRINT
void
debug_print(const char *str)
{
  putDebugChar ('$');
  putDebugChar ('O');
  char csum = 'O';
  for (; *str != '\0'; )
    {
      char c = high_hex (*str);
      csum += c;
      putDebugChar (c);
      c = low_hex (*str++);
      csum += c;
      putDebugChar (c);
    }
  putDebugChar ('#');
  putDebugChar (high_hex (csum));
  putDebugChar (low_hex (csum));
}
#endif /* DBG_PRINT */
/******************************************************************************/
static void store_pc_sp (int pc_adj) FASTCALL;
#define get_reg_value(mem) (*(void* const*)(mem))
#define set_reg_value(mem,val) do { (*(void**)(mem) = (val)); } while (0)
static char* byte2hex(char *buf, byte val);
static int hex2int (const char **buf) FASTCALL;
static char* int2hex (char *buf, int v);
static void get_packet (char *buffer);
static void put_packet (const char *buffer);
static char process (char *buffer) FASTCALL;
static void rest_cpu_state (void);

static void
stub_main (int ex, int pc_adj)
{
  char buffer[DBG_PACKET_SIZE+1];
  sigval = (signed char)ex;
  store_pc_sp (pc_adj);

  DBG_ENTER

  /* after starting gdb_stub must always return stop reason */
  *buffer = '?';
  for (; process (buffer);)
    {
      put_packet (buffer);
      get_packet (buffer);
    }
  put_packet (buffer);
  rest_cpu_state ();
}

static void
get_packet (char *buffer)
{
  byte csum;
  char ch;
  char *p;
  byte esc;
#if DBG_PACKET_SIZE <= 256
  byte count; /* it is OK to use up to 256 here */
#else
  unsigned count;
#endif
  for (;; putDebugChar ('-'))
    {
      /* wait for packet start character */
      while (getDebugChar () != '$');
retry:
      csum = 0;
      esc = 0;
      p = buffer;
      count = DBG_PACKET_SIZE;
      do
	{
	  ch = getDebugChar ();
	  switch (ch)
	    {
	    case '$':
	      goto retry;
	    case '#':
	      goto finish;
	    case '}':
	      esc = 0x20;
	      break;
	    default:
	      *p++ = ch ^ esc;
	      esc = 0;
	      --count;
	    }
	  csum += ch;
	}
      while (count != 0);
finish:
      *p = '\0';
      if (ch != '#') /* packet is too large */
	continue;
      ch = getDebugChar ();
      if (ch != high_hex (csum))
	continue;
      ch = getDebugChar ();
      if (ch != low_hex (csum))
	continue;
      break;
    }
  putDebugChar ('+');
}

static void
put_packet (const char *buffer)
{
  /*  $<packet info>#<checksum>. */
  for (;;)
    {
      putDebugChar ('$');
      char checksum = put_packet_info (buffer);
      putDebugChar ('#');
      putDebugChar (high_hex(checksum));
      putDebugChar (low_hex(checksum));
      for (;;)
	{
	  char c = getDebugChar ();
	  switch (c)
	    {
	    case '+': return;
	    case '-': break;
	    default:
	      putDebugChar (c);
	      continue;
	    }
	  break;
	}
    }
}

static char
put_packet_info (const char *src) FASTCALL
{
  char ch;
  char checksum = 0;
  for (;;)
    {
      ch = *src++;
      if (ch == '\0')
	break;
      if (ch == '}' || ch == '*' || ch == '#' || ch == '$')
	{
	  /* escape special characters */
	  putDebugChar ('}');
	  checksum += '}';
	  ch ^= 0x20;
	}
      putDebugChar (ch);
      checksum += ch;
    }
  return checksum;
}

static void
store_pc_sp (int pc_adj) FASTCALL
{
  byte *sp = get_reg_value (&state[R_SP]);
  byte *pc = get_reg_value (sp);
  pc += pc_adj;
  set_reg_value (&state[R_PC], pc);
  set_reg_value (&state[R_SP], sp + REG_SIZE);
}

static char *mem2hex (char *buf, const byte *mem, unsigned bytes);
static char *hex2mem (byte *mem, const char *buf, unsigned bytes);

/* Command processors. Takes pointer to buffer (begins from command symbol),
   modifies buffer, returns: -1 - empty response (ignore), 0 - success,
   positive: error code. */

#ifdef DBG_MIN_SIZE
static signed char
process_question (char *p) FASTCALL
{
  signed char sig;
  *p++ = 'S';
  sig = sigval;
  if (sig <= 0)
    sig = EX_SIGTRAP;
  p = byte2hex (p, (byte)sig);
  *p = '\0';
  return 0;
}
#else /* DBG_MIN_SIZE */
static char *format_reg_value (char *p, unsigned reg_num, const byte *value);

static signed char
process_question (char *p) FASTCALL
{
  signed char sig;
  *p++ = 'T';
  sig = sigval;
  if (sig <= 0)
    sig = EX_SIGTRAP;
  p = byte2hex (p, (byte)sig);
  p = format_reg_value(p, R_AF/REG_SIZE, &state[R_AF]);
  p = format_reg_value(p, R_SP/REG_SIZE, &state[R_SP]);
  p = format_reg_value(p, R_PC/REG_SIZE, &state[R_PC]);
#if defined(DBG_SWBREAK_PROC) || defined(DBG_HWBREAK) || defined(DBG_WWATCH) || defined(DBG_RWATCH) || defined(DBG_AWATCH)
  const char *reason;
  unsigned addr = 0;
  switch (sigval)
    {
#ifdef DBG_SWBREAK_PROC
    case EX_SWBREAK:
      reason = "swbreak";
      break;
#endif
#ifdef DBG_HWBREAK
    case EX_HWBREAK:
      reason = "hwbreak";
      break;
#endif
#ifdef DBG_WWATCH
    case EX_WWATCH:
      reason = "watch";
      addr = 1;
      break;
#endif
#ifdef DBG_RWATCH
    case EX_RWATCH:
      reason = "rwatch";
      addr = 1;
      break;
#endif
#ifdef DBG_AWATCH
    case EX_AWATCH:
      reason = "awatch";
      addr = 1;
      break;
#endif
    default:
      goto finish;
    }
  while ((*p++ = *reason++))
    ;
  --p;
  *p++ = ':';
  if (addr != 0)
    p = int2hex(p, addr);
  *p++ = ';';
finish:
#endif /* DBG_HWBREAK, DBG_WWATCH, DBG_RWATCH, DBG_AWATCH */
  *p++ = '\0';
  return 0;
}
#endif /* DBG_MINSIZE */

#define STRING2(x) #x
#define STRING1(x) STRING2(x)
#define STRING(x) STRING1(x)
#ifdef DBG_MEMORY_MAP
static void read_memory_map (char *buffer, unsigned offset, unsigned length);
#endif

static signed char
process_q (char *buffer) FASTCALL
{
  char *p;
  if (memcmp (buffer + 1, "Supported", 9) == 0)
    {
      memcpy (buffer, "PacketSize=", 11);
      p = int2hex (&buffer[11], DBG_PACKET_SIZE);
#ifndef DBG_MIN_SIZE
#ifdef DBG_SWBREAK_PROC
      memcpy (p, ";swbreak+", 9);
      p += 9;
#endif
#ifdef DBG_HWBREAK
      memcpy (p, ";hwbreak+", 9);
      p += 9;
#endif
#endif /* DBG_MIN_SIZE */

#ifdef DBG_MEMORY_MAP
      memcpy (p, ";qXfer:memory-map:read+", 23);
      p += 23;
#endif
      *p = '\0';
      return 0;
    }
#ifdef DBG_MEMORY_MAP
  if (memcmp (buffer + 1, "Xfer:memory-map:read:", 21) == 0)
    {
      p = strchr (buffer + 1 + 21, ':');
      if (p == NULL)
	return 1;
      ++p;
      unsigned offset = hex2int (&p);
      if (*p++ != ',')
	return 2;
      unsigned length = hex2int (&p);
      if (length == 0)
	return 3;
      if (length > DBG_PACKET_SIZE)
	return 4;
      read_memory_map (buffer, offset, length);
      return 0;
    }
#endif
#ifndef DBG_MIN_SIZE
  if (memcmp (&buffer[1], "Attached", 9) == 0)
    {
      /* Just report that GDB attached to existing process
	 if it is not applicable for you, then send patches */
      memcpy(buffer, "1", 2);
      return 0;
    }
#endif /* DBG_MIN_SIZE */
  *buffer = '\0';
  return -1;
}

static signed char
process_g (char *buffer) FASTCALL
{
  mem2hex (buffer, state, NUMREGBYTES);
  return 0;
}

static signed char
process_G (char *buffer) FASTCALL
{
  hex2mem (state, &buffer[1], NUMREGBYTES);
  /* OK response */
  *buffer = '\0';
  return 0;
}

static signed char
process_m (char *buffer) FASTCALL
{/* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
  char *p = &buffer[1];
  byte *addr = (void*)hex2int(&p);
  if (*p++ != ',')
    return 1;
  unsigned len = (unsigned)hex2int(&p);
  if (len == 0)
    return 2;
  if (len > DBG_PACKET_SIZE/2)
    return 3;
  p = buffer;
#ifdef DBG_MEMCPY
  do
    {
      byte tmp[16];
      unsigned tlen = sizeof(tmp);
      if (tlen > len)
	tlen = len;
      if (!DBG_MEMCPY(tmp, addr, tlen))
	return 4;
      p = mem2hex (p, tmp, tlen);
      addr += tlen;
      len -= tlen;
    }
  while (len);
#else
  p = mem2hex (p, addr, len);
#endif
  return 0;
}

static signed char
process_M (char *buffer) FASTCALL
{/* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
  char *p = &buffer[1];
  byte *addr = (void*)hex2int(&p);
  if (*p != ',')
    return 1;
  ++p;
  unsigned len = (unsigned)hex2int(&p);
  if (*p++ != ':')
    return 2;
  if (len == 0)
    goto end;
  if (len*2 + (p - buffer) > DBG_PACKET_SIZE)
    return 3;
#ifdef DBG_MEMCPY
  do
    {
      byte tmp[16];
      unsigned tlen = sizeof(tmp);
      if (tlen > len)
	tlen = len;
      p = hex2mem (tmp, p, tlen);
      if (!DBG_MEMCPY(addr, tmp, tlen))
	return 4;
      addr += tlen;
	len -= tlen;
    }
  while (len);
#else
  hex2mem (addr, p, len);
#endif
end:
  /* OK response */
  *buffer = '\0';
  return 0;
}

#ifndef DBG_MIN_SIZE
static signed char
process_X (char *buffer) FASTCALL
{/* XAA..AA,LLLL: Write LLLL binary bytes at address AA.AA return OK */
  char *p = &buffer[1];
  byte *addr = (void*)hex2int(&p);
  if (*p != ',')
    return 1;
  ++p;
  unsigned len = (unsigned)hex2int(&p);
  if (*p++ != ':')
    return 2;
  if (len == 0)
    goto end;
  if (len + (p - buffer) > DBG_PACKET_SIZE)
    return 3;
#ifdef DBG_MEMCPY
  if (!DBG_MEMCPY(addr, p, len))
    return 4;
#else
  memcpy (addr, p, len);
#endif
end:
  /* OK response */
  *buffer = '\0';
  return 0;
}
#else /* DBG_MIN_SIZE */
static signed char
process_X (char *buffer) FASTCALL
{
  (void)buffer;
  return -1;
}
#endif /* DBG_MIN_SIZE */

static signed char
process_c (char *buffer) FASTCALL
{/* 'cAAAA' - Continue at address AAAA(optional) */
  const char *p = &buffer[1];
  if (*p != '\0')
    {
      void *addr = (void*)hex2int(&p);
      set_reg_value (&state[R_PC], addr);
    }
  rest_cpu_state ();
  return 0;
}

static signed char
process_D (char *buffer) FASTCALL
{/* 'D' - detach the program: continue execution */
  *buffer = '\0';
  return -2;
}

static signed char
process_k (char *buffer) FASTCALL
{/* 'k' - Kill the program */
  set_reg_value (&state[R_PC], 0);
  rest_cpu_state ();
  (void)buffer;
  return 0;
}

static signed char
process_v (char *buffer) FASTCALL
{
#ifndef DBG_MIN_SIZE
  if (memcmp (&buffer[1], "Cont", 4) == 0)
    {
      if (buffer[5] == '?')
	{
	  /* result response will be "vCont;c;C"; C action must be
	     supported too, because GDB requires at lease both of them */
	  memcpy (&buffer[5], ";c;C", 5);
	  return 0;
	}
      buffer[0] = '\0';
      if (buffer[5] == ';' && (buffer[6] == 'c' || buffer[6] == 'C'))
	return -2; /* resume execution */
      return 1;
  }
#endif /* DBG_MIN_SIZE */
  return -1;
}

static signed char
process_zZ (char *buffer) FASTCALL
{ /* insert/remove breakpoint */
#if defined(DBG_SWBREAK_PROC) || defined(DBG_HWBREAK) || \
    defined(DBG_WWATCH) || defined(DBG_RWATCH) || defined(DBG_AWATCH)
  const byte set = (*buffer == 'Z');
  const char *p = &buffer[3];
  void *addr = (void*)hex2int(&p);
  if (*p != ',')
    return 1;
  p++;
  int kind = hex2int(&p);
  *buffer = '\0';
  switch (buffer[1])
    {
#ifdef DBG_SWBREAK_PROC
    case '0': /* sw break */
      return DBG_SWBREAK_PROC(set, addr);
#endif
#ifdef DBG_HWBREAK
    case '1': /* hw break */
      return DBG_HWBREAK(set, addr);
#endif
#ifdef DBG_WWATCH
    case '2': /* write watch */
      return DBG_WWATCH(set, addr, kind);
#endif
#ifdef DBG_RWATCH
    case '3': /* read watch */
      return DBG_RWATCH(set, addr, kind);
#endif
#ifdef DBG_AWATCH
    case '4': /* access watch */
      return DBG_AWATCH(set, addr, kind);
#endif
    default:; /* not supported */
    }
#endif
  (void)buffer;
  return -1;
}

static signed char
do_process (char *buffer) FASTCALL
{
  switch (*buffer)
    {
    case '?': return process_question (buffer);
    case 'G': return process_G (buffer);
    case 'k': return process_k (buffer);
    case 'M': return process_M (buffer);
    case 'X': return process_X (buffer);
    case 'Z': return process_zZ (buffer);
    case 'c': return process_c (buffer);
    case 'D': return process_D (buffer);
    case 'g': return process_g (buffer);
    case 'm': return process_m (buffer);
    case 'q': return process_q (buffer);
    case 'v': return process_v (buffer);
    case 'z': return process_zZ (buffer);
    default:  return -1; /* empty response */
    }
}

static char
process (char *buffer) FASTCALL
{
  signed char err = do_process (buffer);
  char *p = buffer;
  char ret = 1;
  if (err == -2)
    {
      ret = 0;
      err = 0;
    }
  if (err > 0)
    {
      *p++ = 'E';
      p = byte2hex (p, err);
      *p = '\0';
    }
  else if (err < 0)
    {
      *p = '\0';
    }
  else if (*p == '\0')
    memcpy(p, "OK", 3);
  return ret;
}

static char *
byte2hex (char *p, byte v)
{
  *p++ = high_hex (v);
  *p++ = low_hex (v);
  return p;
}

static signed char
hex2val (unsigned char hex) FASTCALL
{
  if (hex <= '9')
    return hex - '0';
  hex &= 0xdf; /* make uppercase */
  hex -= 'A' - 10;
  return (hex >= 10 && hex < 16) ? hex : -1;
}

static int
hex2byte (const char *p) FASTCALL
{
  signed char h = hex2val (p[0]);
  signed char l = hex2val (p[1]);
  if (h < 0 || l < 0)
    return -1;
  return (byte)((byte)h << 4) | (byte)l;
}

static int
hex2int (const char **buf) FASTCALL
{
  word r = 0;
  for (;; (*buf)++)
    {
      signed char a = hex2val(**buf);
      if (a < 0)
	break;
      r <<= 4;
      r += (byte)a;
    }
  return (int)r;
}

static char *
int2hex (char *buf, int v)
{
  buf = byte2hex(buf, (word)v >> 8);
  return byte2hex(buf, (byte)v);
}

static char
high_hex (byte v) FASTCALL
{
  return low_hex(v >> 4);
}

static char
low_hex (byte v) FASTCALL
{
/*
  __asm
	ld	a, l
	and	a, #0x0f
	add	a, #0x90
	daa
	adc	a, #0x40
	daa
	ld	l, a
  __endasm;
  (void)v;
*/
  v &= 0x0f;
  v += '0';
  if (v < '9'+1)
    return v;
  return v + 'a' - '0' - 10;
}

/* convert the memory, pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static char *
mem2hex (char *buf, const byte *mem, unsigned bytes)
{
  char *d = buf;
  if (bytes != 0)
    {
      do
	{
	  d = byte2hex (d, *mem++);
	}
      while (--bytes);
    }
  *d = 0;
  return d;
}

/* convert the hex array pointed to by buf into binary, to be placed in mem
   return a pointer to the character after the last byte written */

static const char *
hex2mem (byte *mem, const char *buf, unsigned bytes)
{
  if (bytes != 0)
    {
      do
	{
	  *mem++ = hex2byte (buf);
	  buf += 2;
	}
      while (--bytes);
    }
  return buf;
}

#ifdef DBG_MEMORY_MAP
static void
read_memory_map (char *buffer, unsigned offset, unsigned length)
{
  const char *map = DBG_MEMORY_MAP;
  const unsigned map_sz = strlen(map);
  if (offset >= map_sz)
    {
      buffer[0] = 'l';
      buffer[1] = '\0';
      return;
    }
  if (offset + length > map_sz)
    length = map_sz - offset;
  buffer[0] = 'm';
  memcpy (&buffer[1], &map[offset], length);
  buffer[1+length] = '\0';
}
#endif

/* write string like " nn:0123" and return pointer after it */
#ifndef DBG_MIN_SIZE
static char *
format_reg_value (char *p, unsigned reg_num, const byte *value)
{
  char *d = p;
  unsigned char i;
  d = byte2hex(d, reg_num);
  *d++ = ':';
  value += REG_SIZE;
  i = REG_SIZE;
  do
    {
      d = byte2hex(d, *--value);
    }
  while (--i != 0);
  *d++ = ';';
  return d;
}
#endif /* DBG_MIN_SIZE */

#ifdef __SDCC_gbz80
/* saves all state.except PC and SP */
static void
save_cpu_state() __naked
{
  __asm
	push	af
	ld	a, l
	ld	(#_state + R_HL + 0), a
	ld	a, h
	ld	(#_state + R_HL + 1), a
	ld	hl, #_state + R_HL - 1
	ld	(hl), d
	dec	hl
	ld	(hl), e
	dec	hl
	ld	(hl), b
	dec	hl
	ld	(hl), c
	dec	hl
	pop	bc
	ld	(hl), b
	dec	hl
	ld	(hl), c
	ret
  __endasm;
}

/* restore CPU state and continue execution */
static void
rest_cpu_state() __naked
{
  __asm
;restore SP
	ld	a, (#_state + R_SP + 0)
	ld	l,a
	ld	a, (#_state + R_SP + 1)
	ld	h,a
	ld	sp, hl
;push PC value as return address
	ld	a, (#_state + R_PC + 0)
	ld	l, a
	ld	a, (#_state + R_PC + 1)
	ld	h, a
	push	hl
;restore registers
	ld	hl, #_state + R_AF
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	inc	hl
	push	bc
	ld	c, (hl)
	inc	hl
	ld	b, (hl)
	inc	hl
	ld	e, (hl)
	inc	hl
	ld	d, (hl)
	inc	hl
	ld	a, (hl)
	inc	hl
	ld	h, (hl)
	ld	l, a
	pop	af
	ret
  __endasm;
}
#else
/* saves all state.except PC and SP */
static void
save_cpu_state() __naked
{
  __asm
	ld	(#_state + R_HL), hl
	ld	(#_state + R_DE), de
	ld	(#_state + R_BC), bc
	push	af
	pop	hl
	ld	(#_state + R_AF), hl
	ld	a, r	;R is increased by 7 or by 8 if called via RST
	ld	l, a
	sub	a, #7
	xor	a, l
	and	a, #0x7f
	xor	a, l
#ifdef __SDCC_ez80_adl
	ld	hl, i
	ex	de, hl
	ld	hl, #_state + R_IR
	ld	(hl), a
	inc	hl
	ld	(hl), e
	inc	hl
	ld	(hl), d
	ld	a, MB
	ld	(#_state + R_AF+2), a
#else
	ld	l, a
	ld	a, i
	ld	h, a
	ld	(#_state + R_IR), hl
#endif /* __SDCC_ez80_adl */
	ld	(#_state + R_IX), ix
	ld	(#_state + R_IY), iy
	ex	af, af'	;'
	exx
	ld	(#_state + R_HL_), hl
	ld	(#_state + R_DE_), de
	ld	(#_state + R_BC_), bc
	push	af
	pop	hl
	ld	(#_state + R_AF_), hl
	ret
  __endasm;
}

/* restore CPU state and continue execution */
static void
rest_cpu_state() __naked
{
  __asm
#ifdef DBG_USE_TRAMPOLINE
	ld	sp, _stack + DBG_STACK_SIZE
	ld	hl, (#_state + R_PC)
	push	hl	/* resume address */
#ifdef __SDCC_ez80_adl
	ld	hl, 0xc30000 ; use 0xc34000 for jp.s
#else
	ld	hl, 0xc300
#endif
	push	hl	/* JP opcode */
#endif /* DBG_USE_TRAMPOLINE */
	ld	hl, (#_state + R_AF_)
	push	hl
	pop	af
	ld	bc, (#_state + R_BC_)
	ld	de, (#_state + R_DE_)
	ld	hl, (#_state + R_HL_)
	exx
	ex	af, af'	;'
	ld	iy, (#_state + R_IY)
	ld	ix, (#_state + R_IX)
#ifdef __SDCC_ez80_adl
	ld	a, (#_state + R_AF + 2)
	ld	MB, a
	ld	hl, (#_state + R_IR + 1) ;I register
	ld	i, hl
	ld	a, (#_state + R_IR + 0) ; R register
	ld	l, a
#else
	ld	hl, (#_state + R_IR)
	ld	a, h
	ld	i, a
	ld	a, l
#endif /* __SDCC_ez80_adl */
	sub	a, #10	;number of M1 cycles after ld r,a
	xor	a, l
	and	a, #0x7f
	xor	a, l
	ld	r, a
	ld	de, (#_state + R_DE)
	ld	bc, (#_state + R_BC)
	ld	hl, (#_state + R_AF)
	push	hl
	pop	af
	ld	sp, (#_state + R_SP)
#ifndef DBG_USE_TRAMPOLINE
	ld	hl, (#_state + R_PC)
	push	hl
	ld	hl, (#_state + R_HL)
	DBG_RESUME
#else
	ld	hl, (#_state + R_HL)
#ifdef __SDCC_ez80_adl
	jp	#_stack + DBG_STACK_SIZE - 4
#else
	jp	#_stack + DBG_STACK_SIZE - 3
#endif
#endif /* DBG_USE_TRAMPOLINE */
  __endasm;
}
#endif /* __SDCC_gbz80 */
