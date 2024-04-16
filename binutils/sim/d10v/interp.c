/* This must come before any other includes.  */
#include "defs.h"

#include <inttypes.h>
#include <signal.h>
#include "bfd.h"
#include "sim/callback.h"
#include "sim/sim.h"

#include "sim-main.h"
#include "sim-options.h"
#include "sim-signal.h"

#include "sim/sim-d10v.h"
#include "gdb/signals.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "d10v-sim.h"

#include "target-newlib-syscall.h"

enum _leftright { LEFT_FIRST, RIGHT_FIRST };

struct _state State;

int d10v_debug;

/* Set this to true to get the previous segment layout. */

int old_segment_mapping;

unsigned long ins_type_counters[ (int)INS_MAX ];

uint16_t OP[4];

static long hash (long insn, int format);
static struct hash_entry *lookup_hash (SIM_DESC, SIM_CPU *, uint32_t ins, int size);
static void get_operands (struct simops *s, uint32_t ins);
static void do_long (SIM_DESC, SIM_CPU *, uint32_t ins);
static void do_2_short (SIM_DESC, SIM_CPU *, uint16_t ins1, uint16_t ins2, enum _leftright leftright);
static void do_parallel (SIM_DESC, SIM_CPU *, uint16_t ins1, uint16_t ins2);
static char *add_commas (char *buf, int sizeof_buf, unsigned long value);
static INLINE uint8_t *map_memory (SIM_DESC, SIM_CPU *, unsigned phys_addr);

#define MAX_HASH  63
struct hash_entry
{
  struct hash_entry *next;
  uint32_t opcode;
  uint32_t mask;
  int size;
  struct simops *ops;
};

struct hash_entry hash_table[MAX_HASH+1];

INLINE static long 
hash (long insn, int format)
{
  if (format & LONG_OPCODE)
    return ((insn & 0x3F000000) >> 24);
  else
    return((insn & 0x7E00) >> 9);
}

INLINE static struct hash_entry *
lookup_hash (SIM_DESC sd, SIM_CPU *cpu, uint32_t ins, int size)
{
  struct hash_entry *h;

  if (size)
    h = &hash_table[(ins & 0x3F000000) >> 24];
  else
    h = &hash_table[(ins & 0x7E00) >> 9];

  while ((ins & h->mask) != h->opcode || h->size != size)
    {
      if (h->next == NULL)
	sim_engine_halt (sd, cpu, NULL, PC, sim_stopped, SIM_SIGILL);
      h = h->next;
    }
  return (h);
}

INLINE static void
get_operands (struct simops *s, uint32_t ins)
{
  int i, shift, bits;
  uint32_t mask;
  for (i=0; i < s->numops; i++)
    {
      shift = s->operands[3*i];
      bits = s->operands[3*i+1];
      /* flags = s->operands[3*i+2]; */
      mask = 0x7FFFFFFF >> (31 - bits);
      OP[i] = (ins >> shift) & mask;
    }
  /* FIXME: for tracing, update values that need to be updated each
     instruction decode cycle */
  State.trace.psw = PSW;
}

static void
do_long (SIM_DESC sd, SIM_CPU *cpu, uint32_t ins)
{
  struct hash_entry *h;
#ifdef DEBUG
  if ((d10v_debug & DEBUG_INSTRUCTION) != 0)
    sim_io_printf (sd, "do_long 0x%x\n", ins);
#endif
  h = lookup_hash (sd, cpu, ins, 1);
  if (h == NULL)
    return;
  get_operands (h->ops, ins);
  State.ins_type = INS_LONG;
  ins_type_counters[ (int)State.ins_type ]++;
  (h->ops->func) (sd, cpu);
}

static void
do_2_short (SIM_DESC sd, SIM_CPU *cpu, uint16_t ins1, uint16_t ins2, enum _leftright leftright)
{
  struct hash_entry *h;
  enum _ins_type first, second;

#ifdef DEBUG
  if ((d10v_debug & DEBUG_INSTRUCTION) != 0)
    sim_io_printf (sd, "do_2_short 0x%x (%s) -> 0x%x\n", ins1,
		   leftright ? "left" : "right", ins2);
#endif

  if (leftright == LEFT_FIRST)
    {
      first = INS_LEFT;
      second = INS_RIGHT;
      ins_type_counters[ (int)INS_LEFTRIGHT ]++;
    }
  else
    {
      first = INS_RIGHT;
      second = INS_LEFT;
      ins_type_counters[ (int)INS_RIGHTLEFT ]++;
    }

  /* Issue the first instruction */
  h = lookup_hash (sd, cpu, ins1, 0);
  if (h == NULL)
    return;
  get_operands (h->ops, ins1);
  State.ins_type = first;
  ins_type_counters[ (int)State.ins_type ]++;
  (h->ops->func) (sd, cpu);

  /* Issue the second instruction (if the PC hasn't changed) */
  if (!State.pc_changed)
    {
      /* finish any existing instructions */
      SLOT_FLUSH ();
      h = lookup_hash (sd, cpu, ins2, 0);
      if (h == NULL)
	return;
      get_operands (h->ops, ins2);
      State.ins_type = second;
      ins_type_counters[ (int)State.ins_type ]++;
      ins_type_counters[ (int)INS_CYCLES ]++;
      (h->ops->func) (sd, cpu);
    }
  else
    ins_type_counters[ (int)INS_COND_JUMP ]++;
}

static void
do_parallel (SIM_DESC sd, SIM_CPU *cpu, uint16_t ins1, uint16_t ins2)
{
  struct hash_entry *h1, *h2;
#ifdef DEBUG
  if ((d10v_debug & DEBUG_INSTRUCTION) != 0)
    sim_io_printf (sd, "do_parallel 0x%x || 0x%x\n", ins1, ins2);
#endif
  ins_type_counters[ (int)INS_PARALLEL ]++;
  h1 = lookup_hash (sd, cpu, ins1, 0);
  if (h1 == NULL)
    return;
  h2 = lookup_hash (sd, cpu, ins2, 0);
  if (h2 == NULL)
    return;

  if (h1->ops->exec_type == PARONLY)
    {
      get_operands (h1->ops, ins1);
      State.ins_type = INS_LEFT_COND_TEST;
      ins_type_counters[ (int)State.ins_type ]++;
      (h1->ops->func) (sd, cpu);
      if (State.exe)
	{
	  ins_type_counters[ (int)INS_COND_TRUE ]++;
	  get_operands (h2->ops, ins2);
	  State.ins_type = INS_RIGHT_COND_EXE;
	  ins_type_counters[ (int)State.ins_type ]++;
	  (h2->ops->func) (sd, cpu);
	}
      else
	ins_type_counters[ (int)INS_COND_FALSE ]++;
    }
  else if (h2->ops->exec_type == PARONLY)
    {
      get_operands (h2->ops, ins2);
      State.ins_type = INS_RIGHT_COND_TEST;
      ins_type_counters[ (int)State.ins_type ]++;
      (h2->ops->func) (sd, cpu);
      if (State.exe)
	{
	  ins_type_counters[ (int)INS_COND_TRUE ]++;
	  get_operands (h1->ops, ins1);
	  State.ins_type = INS_LEFT_COND_EXE;
	  ins_type_counters[ (int)State.ins_type ]++;
	  (h1->ops->func) (sd, cpu);
	}
      else
	ins_type_counters[ (int)INS_COND_FALSE ]++;
    }
  else
    {
      get_operands (h1->ops, ins1);
      State.ins_type = INS_LEFT_PARALLEL;
      ins_type_counters[ (int)State.ins_type ]++;
      (h1->ops->func) (sd, cpu);
      get_operands (h2->ops, ins2);
      State.ins_type = INS_RIGHT_PARALLEL;
      ins_type_counters[ (int)State.ins_type ]++;
      (h2->ops->func) (sd, cpu);
    }
}
 
static char *
add_commas (char *buf, int sizeof_buf, unsigned long value)
{
  int comma = 3;
  char *endbuf = buf + sizeof_buf - 1;

  *--endbuf = '\0';
  do {
    if (comma-- == 0)
      {
	*--endbuf = ',';
	comma = 2;
      }

    *--endbuf = (value % 10) + '0';
  } while ((value /= 10) != 0);

  return endbuf;
}

static void
sim_size (int power)
{
  int i;
  for (i = 0; i < IMEM_SEGMENTS; i++)
    {
      if (State.mem.insn[i])
	free (State.mem.insn[i]);
    }
  for (i = 0; i < DMEM_SEGMENTS; i++)
    {
      if (State.mem.data[i])
	free (State.mem.data[i]);
    }
  for (i = 0; i < UMEM_SEGMENTS; i++)
    {
      if (State.mem.unif[i])
	free (State.mem.unif[i]);
    }
  /* Always allocate dmem segment 0.  This contains the IMAP and DMAP
     registers. */
  State.mem.data[0] = calloc (1, SEGMENT_SIZE);
}

/* For tracing - leave info on last access around. */
static char *last_segname = "invalid";
static char *last_from = "invalid";
static char *last_to = "invalid";

enum
  {
    IMAP0_OFFSET = 0xff00,
    DMAP0_OFFSET = 0xff08,
    DMAP2_SHADDOW = 0xff04,
    DMAP2_OFFSET = 0xff0c
  };

static void
set_dmap_register (SIM_DESC sd, int reg_nr, unsigned long value)
{
  uint8_t *raw = map_memory (sd, NULL, SIM_D10V_MEMORY_DATA
			   + DMAP0_OFFSET + 2 * reg_nr);
  WRITE_16 (raw, value);
#ifdef DEBUG
  if ((d10v_debug & DEBUG_MEMORY))
    {
      sim_io_printf (sd, "mem: dmap%d=0x%04lx\n", reg_nr, value);
    }
#endif
}

static unsigned long
dmap_register (SIM_DESC sd, SIM_CPU *cpu, void *regcache, int reg_nr)
{
  uint8_t *raw = map_memory (sd, cpu, SIM_D10V_MEMORY_DATA
			   + DMAP0_OFFSET + 2 * reg_nr);
  return READ_16 (raw);
}

static void
set_imap_register (SIM_DESC sd, int reg_nr, unsigned long value)
{
  uint8_t *raw = map_memory (sd, NULL, SIM_D10V_MEMORY_DATA
			   + IMAP0_OFFSET + 2 * reg_nr);
  WRITE_16 (raw, value);
#ifdef DEBUG
  if ((d10v_debug & DEBUG_MEMORY))
    {
      sim_io_printf (sd, "mem: imap%d=0x%04lx\n", reg_nr, value);
    }
#endif
}

static unsigned long
imap_register (SIM_DESC sd, SIM_CPU *cpu, void *regcache, int reg_nr)
{
  uint8_t *raw = map_memory (sd, cpu, SIM_D10V_MEMORY_DATA
			   + IMAP0_OFFSET + 2 * reg_nr);
  return READ_16 (raw);
}

enum
  {
    HELD_SPI_IDX = 0,
    HELD_SPU_IDX = 1
  };

static unsigned long
spu_register (void)
{
  if (PSW_SM)
    return GPR (SP_IDX);
  else
    return HELD_SP (HELD_SPU_IDX);
}

static unsigned long
spi_register (void)
{
  if (!PSW_SM)
    return GPR (SP_IDX);
  else
    return HELD_SP (HELD_SPI_IDX);
}

static void
set_spi_register (unsigned long value)
{
  if (!PSW_SM)
    SET_GPR (SP_IDX, value);
  SET_HELD_SP (HELD_SPI_IDX, value);
}

static void
set_spu_register  (unsigned long value)
{
  if (PSW_SM)
    SET_GPR (SP_IDX, value);
  SET_HELD_SP (HELD_SPU_IDX, value);
}

/* Given a virtual address in the DMAP address space, translate it
   into a physical address. */

static unsigned long
sim_d10v_translate_dmap_addr (SIM_DESC sd,
			      SIM_CPU *cpu,
			      unsigned long offset,
			      int nr_bytes,
			      unsigned long *phys,
			      void *regcache,
			      unsigned long (*dmap_register) (SIM_DESC,
							      SIM_CPU *,
							      void *regcache,
							      int reg_nr))
{
  short map;
  int regno;
  last_from = "logical-data";
  if (offset >= DMAP_BLOCK_SIZE * SIM_D10V_NR_DMAP_REGS)
    {
      /* Logical address out side of data segments, not supported */
      return 0;
    }
  regno = (offset / DMAP_BLOCK_SIZE);
  offset = (offset % DMAP_BLOCK_SIZE);
  if ((offset % DMAP_BLOCK_SIZE) + nr_bytes > DMAP_BLOCK_SIZE)
    {
      /* Don't cross a BLOCK boundary */
      nr_bytes = DMAP_BLOCK_SIZE - (offset % DMAP_BLOCK_SIZE);
    }
  map = dmap_register (sd, cpu, regcache, regno);
  if (regno == 3)
    {
      /* Always maps to data memory */
      int iospi = (offset / 0x1000) % 4;
      int iosp = (map >> (4 * (3 - iospi))) % 0x10;
      last_to = "io-space";
      *phys = (SIM_D10V_MEMORY_DATA + (iosp * 0x10000) + 0xc000 + offset);
    }
  else
    {
      int sp = ((map & 0x3000) >> 12);
      int segno = (map & 0x3ff);
      switch (sp)
	{
	case 0: /* 00: Unified memory */
	  *phys = SIM_D10V_MEMORY_UNIFIED + (segno * DMAP_BLOCK_SIZE) + offset;
	  last_to = "unified";
	  break;
	case 1: /* 01: Instruction Memory */
	  *phys = SIM_D10V_MEMORY_INSN + (segno * DMAP_BLOCK_SIZE) + offset;
	  last_to = "chip-insn";
	  break;
	case 2: /* 10: Internal data memory */
	  *phys = SIM_D10V_MEMORY_DATA + (segno << 16) + (regno * DMAP_BLOCK_SIZE) + offset;
	  last_to = "chip-data";
	  break;
	case 3: /* 11: Reserved */
	  return 0;
	}
    }
  return nr_bytes;
}

/* Given a virtual address in the IMAP address space, translate it
   into a physical address. */

static unsigned long
sim_d10v_translate_imap_addr (SIM_DESC sd,
			      SIM_CPU *cpu,
			      unsigned long offset,
			      int nr_bytes,
			      unsigned long *phys,
			      void *regcache,
			      unsigned long (*imap_register) (SIM_DESC,
							      SIM_CPU *,
							      void *regcache,
							      int reg_nr))
{
  short map;
  int regno;
  int sp;
  int segno;
  last_from = "logical-insn";
  if (offset >= (IMAP_BLOCK_SIZE * SIM_D10V_NR_IMAP_REGS))
    {
      /* Logical address outside of IMAP segments, not supported */
      return 0;
    }
  regno = (offset / IMAP_BLOCK_SIZE);
  offset = (offset % IMAP_BLOCK_SIZE);
  if (offset + nr_bytes > IMAP_BLOCK_SIZE)
    {
      /* Don't cross a BLOCK boundary */
      nr_bytes = IMAP_BLOCK_SIZE - offset;
    }
  map = imap_register (sd, cpu, regcache, regno);
  sp = (map & 0x3000) >> 12;
  segno = (map & 0x007f);
  switch (sp)
    {
    case 0: /* 00: unified memory */
      *phys = SIM_D10V_MEMORY_UNIFIED + (segno << 17) + offset;
      last_to = "unified";
      break;
    case 1: /* 01: instruction memory */
      *phys = SIM_D10V_MEMORY_INSN + (IMAP_BLOCK_SIZE * regno) + offset;
      last_to = "chip-insn";
      break;
    case 2: /*10*/
      /* Reserved. */
      return 0;
    case 3: /* 11: for testing  - instruction memory */
      offset = (offset % 0x800);
      *phys = SIM_D10V_MEMORY_INSN + offset;
      if (offset + nr_bytes > 0x800)
	/* don't cross VM boundary */
	nr_bytes = 0x800 - offset;
      last_to = "test-insn";
      break;
    }
  return nr_bytes;
}

static unsigned long
sim_d10v_translate_addr (SIM_DESC sd,
			 SIM_CPU *cpu,
			 unsigned long memaddr,
			 int nr_bytes,
			 unsigned long *targ_addr,
			 void *regcache,
			 unsigned long (*dmap_register) (SIM_DESC,
							 SIM_CPU *,
							 void *regcache,
							 int reg_nr),
			 unsigned long (*imap_register) (SIM_DESC,
							 SIM_CPU *,
							 void *regcache,
							 int reg_nr))
{
  unsigned long phys;
  unsigned long seg;
  unsigned long off;

  last_from = "unknown";
  last_to = "unknown";

  seg = (memaddr >> 24);
  off = (memaddr & 0xffffffL);

  /* However, if we've asked to use the previous generation of segment
     mapping, rearrange the segments as follows. */

  if (old_segment_mapping)
    {
      switch (seg)
	{
	case 0x00: /* DMAP translated memory */
	  seg = 0x10;
	  break;
	case 0x01: /* IMAP translated memory */
	  seg = 0x11;
	  break;
	case 0x10: /* On-chip data memory */
	  seg = 0x02;
	  break;
	case 0x11: /* On-chip insn memory */
	  seg = 0x01;
	  break;
	case 0x12: /* Unified memory */
	  seg = 0x00;
	  break;
	}
    }

  switch (seg)
    {
    case 0x00:			/* Physical unified memory */
      last_from = "phys-unified";
      last_to = "unified";
      phys = SIM_D10V_MEMORY_UNIFIED + off;
      if ((off % SEGMENT_SIZE) + nr_bytes > SEGMENT_SIZE)
	nr_bytes = SEGMENT_SIZE - (off % SEGMENT_SIZE);
      break;

    case 0x01:			/* Physical instruction memory */
      last_from = "phys-insn";
      last_to = "chip-insn";
      phys = SIM_D10V_MEMORY_INSN + off;
      if ((off % SEGMENT_SIZE) + nr_bytes > SEGMENT_SIZE)
	nr_bytes = SEGMENT_SIZE - (off % SEGMENT_SIZE);
      break;

    case 0x02:			/* Physical data memory segment */
      last_from = "phys-data";
      last_to = "chip-data";
      phys = SIM_D10V_MEMORY_DATA + off;
      if ((off % SEGMENT_SIZE) + nr_bytes > SEGMENT_SIZE)
	nr_bytes = SEGMENT_SIZE - (off % SEGMENT_SIZE);
      break;

    case 0x10:			/* in logical data address segment */
      nr_bytes = sim_d10v_translate_dmap_addr (sd, cpu, off, nr_bytes, &phys,
					       regcache, dmap_register);
      break;

    case 0x11:			/* in logical instruction address segment */
      nr_bytes = sim_d10v_translate_imap_addr (sd, cpu, off, nr_bytes, &phys,
					       regcache, imap_register);
      break;

    default:
      return 0;
    }

  *targ_addr = phys;
  return nr_bytes;
}

/* Return a pointer into the raw buffer designated by phys_addr.  It
   is assumed that the client has already ensured that the access
   isn't going to cross a segment boundary. */

uint8_t *
map_memory (SIM_DESC sd, SIM_CPU *cpu, unsigned phys_addr)
{
  uint8_t **memory;
  uint8_t *raw;
  unsigned offset;
  int segment = ((phys_addr >> 24) & 0xff);
  
  switch (segment)
    {
      
    case 0x00: /* Unified memory */
      {
	memory = &State.mem.unif[(phys_addr / SEGMENT_SIZE) % UMEM_SEGMENTS];
	last_segname = "umem";
	break;
      }
    
    case 0x01: /* On-chip insn memory */
      {
	memory = &State.mem.insn[(phys_addr / SEGMENT_SIZE) % IMEM_SEGMENTS];
	last_segname = "imem";
	break;
      }
    
    case 0x02: /* On-chip data memory */
      {
	if ((phys_addr & 0xff00) == 0xff00)
	  {
	    phys_addr = (phys_addr & 0xffff);
	    if (phys_addr == DMAP2_SHADDOW)
	      {
		phys_addr = DMAP2_OFFSET;
		last_segname = "dmap";
	      }
	    else
	      last_segname = "reg";
	  }
	else
	  last_segname = "dmem";
	memory = &State.mem.data[(phys_addr / SEGMENT_SIZE) % DMEM_SEGMENTS];
	break;
      }
    
    default:
      /* OOPS! */
      last_segname = "scrap";
      sim_engine_halt (sd, cpu, NULL, PC, sim_stopped, SIM_SIGBUS);
    }
  
  if (*memory == NULL)
    *memory = xcalloc (1, SEGMENT_SIZE);
  
  offset = (phys_addr % SEGMENT_SIZE);
  raw = *memory + offset;
  return raw;
}
  
/* Transfer data to/from simulated memory.  Since a bug in either the
   simulated program or in gdb or the simulator itself may cause a
   bogus address to be passed in, we need to do some sanity checking
   on addresses to make sure they are within bounds.  When an address
   fails the bounds check, treat it as a zero length read/write rather
   than aborting the entire run. */

static int
xfer_mem (SIM_DESC sd,
	  address_word virt,
	  unsigned char *buffer,
	  uint64_t size,
	  int write_p)
{
  uint8_t *memory;
  unsigned long phys;
  int phys_size;
  phys_size = sim_d10v_translate_addr (sd, NULL, virt, size, &phys, NULL,
				       dmap_register, imap_register);
  if (phys_size == 0)
    return 0;

  memory = map_memory (sd, NULL, phys);

#ifdef DEBUG
  if ((d10v_debug & DEBUG_INSTRUCTION) != 0)
    {
      sim_io_printf
	(sd,
	 "sim_%s %d bytes: 0x%08" PRIxTA " (%s) -> 0x%08lx (%s) -> %p (%s)\n",
	 write_p ? "write" : "read",
	 phys_size, virt, last_from,
	 phys, last_to,
	 memory, last_segname);
    }
#endif

  if (write_p)
    {
      memcpy (memory, buffer, phys_size);
    }
  else
    {
      memcpy (buffer, memory, phys_size);
    }
  
  return phys_size;
}


uint64_t
sim_write (SIM_DESC sd, uint64_t addr, const void *buffer, uint64_t size)
{
  /* FIXME: this should be performing a virtual transfer */
  /* FIXME: We cast the const away, but it's safe because xfer_mem only reads
     when write_p==1.  This is still ugly.  */
  return xfer_mem (sd, addr, (void *) buffer, size, 1);
}

uint64_t
sim_read (SIM_DESC sd, uint64_t addr, void *buffer, uint64_t size)
{
  /* FIXME: this should be performing a virtual transfer */
  return xfer_mem (sd, addr, buffer, size, 0);
}

static sim_cia
d10v_pc_get (sim_cpu *cpu)
{
  return PC;
}

static void
d10v_pc_set (sim_cpu *cpu, sim_cia pc)
{
  SIM_DESC sd = CPU_STATE (cpu);
  SET_PC (pc);
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

static int d10v_reg_fetch (SIM_CPU *, int, void *, int);
static int d10v_reg_store (SIM_CPU *, int, const void *, int);

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  struct simops *s;
  struct hash_entry *h;
  static int init_p = 0;
  char * const *p;
  int i;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_alignment = STRICT_ALIGNMENT;
  cb->syscall_map = cb_d10v_syscall_map;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 0) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = d10v_reg_fetch;
      CPU_REG_STORE (cpu) = d10v_reg_store;
      CPU_PC_FETCH (cpu) = d10v_pc_get;
      CPU_PC_STORE (cpu) = d10v_pc_set;
    }

  old_segment_mapping = 0;

  /* NOTE: This argument parsing is only effective when this function
     is called by GDB. Standalone argument parsing is handled by
     sim/common/run.c. */
  for (p = argv + 1; *p; ++p)
    {
      if (strcmp (*p, "-oldseg") == 0)
	old_segment_mapping = 1;
#ifdef DEBUG
      else if (strcmp (*p, "-t") == 0)
	d10v_debug = DEBUG;
      else if (strncmp (*p, "-t", 2) == 0)
	d10v_debug = atoi (*p + 2);
#endif
    }
  
  /* put all the opcodes in the hash table */
  if (!init_p++)
    {
      for (s = Simops; s->func; s++)
	{
	  h = &hash_table[hash(s->opcode,s->format)];
      
	  /* go to the last entry in the chain */
	  while (h->next)
	    h = h->next;

	  if (h->ops)
	    {
	      h->next = (struct hash_entry *) calloc(1,sizeof(struct hash_entry));
	      if (!h->next)
		perror ("malloc failure");

	      h = h->next;
	    }
	  h->ops = s;
	  h->mask = s->mask;
	  h->opcode = s->opcode;
	  h->size = s->is_long;
	}
    }

  /* reset the processor state */
  if (!State.mem.data[0])
    sim_size (1);

  return sd;
}

uint8_t *
dmem_addr (SIM_DESC sd, SIM_CPU *cpu, uint16_t offset)
{
  unsigned long phys;
  uint8_t *mem;
  int phys_size;

  /* Note: DMEM address range is 0..0x10000. Calling code can compute
     things like ``0xfffe + 0x0e60 == 0x10e5d''.  Since offset's type
     is uint16_t this is modulo'ed onto 0x0e5d. */

  phys_size = sim_d10v_translate_dmap_addr (sd, cpu, offset, 1, &phys, NULL,
					    dmap_register);
  if (phys_size == 0)
    sim_engine_halt (sd, cpu, NULL, PC, sim_stopped, SIM_SIGBUS);
  mem = map_memory (sd, cpu, phys);
#ifdef DEBUG
  if ((d10v_debug & DEBUG_MEMORY))
    {
      sim_io_printf
	(sd,
	 "mem: 0x%08x (%s) -> 0x%08lx %d (%s) -> %p (%s)\n",
	 offset, last_from,
	 phys, phys_size, last_to,
	 mem, last_segname);
    }
#endif
  return mem;
}

uint8_t *
imem_addr (SIM_DESC sd, SIM_CPU *cpu, uint32_t offset)
{
  unsigned long phys;
  uint8_t *mem;
  int phys_size = sim_d10v_translate_imap_addr (sd, cpu, offset, 1, &phys, NULL,
						imap_register);
  if (phys_size == 0)
    sim_engine_halt (sd, cpu, NULL, PC, sim_stopped, SIM_SIGBUS);
  mem = map_memory (sd, cpu, phys);
#ifdef DEBUG
  if ((d10v_debug & DEBUG_MEMORY))
    {
      sim_io_printf
	(sd,
	 "mem: 0x%08x (%s) -> 0x%08lx %d (%s) -> %p (%s)\n",
	 offset, last_from,
	 phys, phys_size, last_to,
	 mem, last_segname);
    }
#endif
  return mem;
}

static void
step_once (SIM_DESC sd, SIM_CPU *cpu)
{
  uint32_t inst;
  uint8_t *iaddr;

  /* TODO: Unindent this block.  */
    {
      iaddr = imem_addr (sd, cpu, (uint32_t)PC << 2);
 
      inst = get_longword( iaddr ); 
 
      State.pc_changed = 0;
      ins_type_counters[ (int)INS_CYCLES ]++;
      
      switch (inst & 0xC0000000)
	{
	case 0xC0000000:
	  /* long instruction */
	  do_long (sd, cpu, inst & 0x3FFFFFFF);
	  break;
	case 0x80000000:
	  /* R -> L */
	  do_2_short (sd, cpu, inst & 0x7FFF, (inst & 0x3FFF8000) >> 15, RIGHT_FIRST);
	  break;
	case 0x40000000:
	  /* L -> R */
	  do_2_short (sd, cpu, (inst & 0x3FFF8000) >> 15, inst & 0x7FFF, LEFT_FIRST);
	  break;
	case 0:
	  do_parallel (sd, cpu, (inst & 0x3FFF8000) >> 15, inst & 0x7FFF);
	  break;
	}
      
      /* If the PC of the current instruction matches RPT_E then
	 schedule a branch to the loop start.  If one of those
	 instructions happens to be a branch, than that instruction
	 will be ignored */
      if (!State.pc_changed)
	{
	  if (PSW_RP && PC == RPT_E)
	    {
	      /* Note: The behavour of a branch instruction at RPT_E
		 is implementation dependant, this simulator takes the
		 branch.  Branching to RPT_E is valid, the instruction
		 must be executed before the loop is taken.  */
	      if (RPT_C == 1)
		{
		  SET_PSW_RP (0);
		  SET_RPT_C (0);
		  SET_PC (PC + 1);
		}
	      else
		{
		  SET_RPT_C (RPT_C - 1);
		  SET_PC (RPT_S);
		}
	    }
	  else
	    SET_PC (PC + 1);
	}	  
      
      /* Check for a breakpoint trap on this instruction.  This
	 overrides any pending branches or loops */
      if (PSW_DB && PC == IBA)
	{
	  SET_BPC (PC);
	  SET_BPSW (PSW);
	  SET_PSW (PSW & PSW_SM_BIT);
	  SET_PC (SDBT_VECTOR_START);
	}

      /* Writeback all the DATA / PC changes */
      SLOT_FLUSH ();
    }
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr,  /* ignore  */
		int nr_cpus,      /* ignore  */
		int siggnal)
{
  sim_cpu *cpu;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  cpu = STATE_CPU (sd, 0);

  switch (siggnal)
    {
    case 0:
      break;
    case GDB_SIGNAL_BUS:
      SET_BPC (PC);
      SET_BPSW (PSW);
      SET_HW_PSW ((PSW & (PSW_F0_BIT | PSW_F1_BIT | PSW_C_BIT)));
      JMP (AE_VECTOR_START);
      SLOT_FLUSH ();
      break;
    case GDB_SIGNAL_ILL:
      SET_BPC (PC);
      SET_BPSW (PSW);
      SET_HW_PSW ((PSW & (PSW_F0_BIT | PSW_F1_BIT | PSW_C_BIT)));
      JMP (RIE_VECTOR_START);
      SLOT_FLUSH ();
      break;
    default:
      /* just ignore it */
      break;
    }

  while (1)
    {
      step_once (sd, cpu);
      if (sim_events_tick (sd))
	sim_events_process (sd);
    }
}

void
sim_info (SIM_DESC sd, bool verbose)
{
  char buf1[40];
  char buf2[40];
  char buf3[40];
  char buf4[40];
  char buf5[40];
  unsigned long left		= ins_type_counters[ (int)INS_LEFT ] + ins_type_counters[ (int)INS_LEFT_COND_EXE ];
  unsigned long left_nops	= ins_type_counters[ (int)INS_LEFT_NOPS ];
  unsigned long left_parallel	= ins_type_counters[ (int)INS_LEFT_PARALLEL ];
  unsigned long left_cond	= ins_type_counters[ (int)INS_LEFT_COND_TEST ];
  unsigned long left_total	= left + left_parallel + left_cond + left_nops;

  unsigned long right		= ins_type_counters[ (int)INS_RIGHT ] + ins_type_counters[ (int)INS_RIGHT_COND_EXE ];
  unsigned long right_nops	= ins_type_counters[ (int)INS_RIGHT_NOPS ];
  unsigned long right_parallel	= ins_type_counters[ (int)INS_RIGHT_PARALLEL ];
  unsigned long right_cond	= ins_type_counters[ (int)INS_RIGHT_COND_TEST ];
  unsigned long right_total	= right + right_parallel + right_cond + right_nops;

  unsigned long unknown		= ins_type_counters[ (int)INS_UNKNOWN ];
  unsigned long ins_long	= ins_type_counters[ (int)INS_LONG ];
  unsigned long parallel	= ins_type_counters[ (int)INS_PARALLEL ];
  unsigned long leftright	= ins_type_counters[ (int)INS_LEFTRIGHT ];
  unsigned long rightleft	= ins_type_counters[ (int)INS_RIGHTLEFT ];
  unsigned long cond_true	= ins_type_counters[ (int)INS_COND_TRUE ];
  unsigned long cond_false	= ins_type_counters[ (int)INS_COND_FALSE ];
  unsigned long cond_jump	= ins_type_counters[ (int)INS_COND_JUMP ];
  unsigned long cycles		= ins_type_counters[ (int)INS_CYCLES ];
  unsigned long total		= (unknown + left_total + right_total + ins_long);

  int size			= strlen (add_commas (buf1, sizeof (buf1), total));
  int parallel_size		= strlen (add_commas (buf1, sizeof (buf1),
						      (left_parallel > right_parallel) ? left_parallel : right_parallel));
  int cond_size			= strlen (add_commas (buf1, sizeof (buf1), (left_cond > right_cond) ? left_cond : right_cond));
  int nop_size			= strlen (add_commas (buf1, sizeof (buf1), (left_nops > right_nops) ? left_nops : right_nops));
  int normal_size		= strlen (add_commas (buf1, sizeof (buf1), (left > right) ? left : right));

  sim_io_printf (sd,
		 "executed %*s left  instruction(s), %*s normal, %*s parallel, %*s EXExxx, %*s nops\n",
		 size, add_commas (buf1, sizeof (buf1), left_total),
		 normal_size, add_commas (buf2, sizeof (buf2), left),
		 parallel_size, add_commas (buf3, sizeof (buf3), left_parallel),
		 cond_size, add_commas (buf4, sizeof (buf4), left_cond),
		 nop_size, add_commas (buf5, sizeof (buf5), left_nops));

  sim_io_printf (sd,
		 "executed %*s right instruction(s), %*s normal, %*s parallel, %*s EXExxx, %*s nops\n",
		 size, add_commas (buf1, sizeof (buf1), right_total),
		 normal_size, add_commas (buf2, sizeof (buf2), right),
		 parallel_size, add_commas (buf3, sizeof (buf3), right_parallel),
		 cond_size, add_commas (buf4, sizeof (buf4), right_cond),
		 nop_size, add_commas (buf5, sizeof (buf5), right_nops));

  if (ins_long)
    sim_io_printf (sd,
		   "executed %*s long instruction(s)\n",
		   size, add_commas (buf1, sizeof (buf1), ins_long));

  if (parallel)
    sim_io_printf (sd,
		   "executed %*s parallel instruction(s)\n",
		   size, add_commas (buf1, sizeof (buf1), parallel));

  if (leftright)
    sim_io_printf (sd,
		   "executed %*s instruction(s) encoded L->R\n",
		   size, add_commas (buf1, sizeof (buf1), leftright));

  if (rightleft)
    sim_io_printf (sd,
		   "executed %*s instruction(s) encoded R->L\n",
		   size, add_commas (buf1, sizeof (buf1), rightleft));

  if (unknown)
    sim_io_printf (sd,
		   "executed %*s unknown instruction(s)\n",
		   size, add_commas (buf1, sizeof (buf1), unknown));

  if (cond_true)
    sim_io_printf (sd,
		   "executed %*s instruction(s) due to EXExxx condition being true\n",
		   size, add_commas (buf1, sizeof (buf1), cond_true));

  if (cond_false)
    sim_io_printf (sd,
		   "skipped  %*s instruction(s) due to EXExxx condition being false\n",
		   size, add_commas (buf1, sizeof (buf1), cond_false));

  if (cond_jump)
    sim_io_printf (sd,
		   "skipped  %*s instruction(s) due to conditional branch succeeding\n",
		   size, add_commas (buf1, sizeof (buf1), cond_jump));

  sim_io_printf (sd,
		 "executed %*s cycle(s)\n",
		 size, add_commas (buf1, sizeof (buf1), cycles));

  sim_io_printf (sd,
		 "executed %*s total instructions\n",
		 size, add_commas (buf1, sizeof (buf1), total));
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  bfd_vma start_address;

  /* Make sure we have the right structure for the following memset.  */
  static_assert (offsetof (struct _state, regs) == 0,
		 "State.regs is not at offset 0");

  /* Reset state from the regs field until the mem field.  */
  memset (&State, 0, (uintptr_t) &State.mem - (uintptr_t) &State.regs);

  /* There was a hack here to copy the values of argc and argv into r0
     and r1.  The values were also saved into some high memory that
     won't be overwritten by the stack (0x7C00).  The reason for doing
     this was to allow the 'run' program to accept arguments.  Without
     the hack, this is not possible anymore.  If the simulator is run
     from the debugger, arguments cannot be passed in, so this makes
     no difference.  */

  /* set PC */
  if (abfd != NULL)
    start_address = bfd_get_start_address (abfd);
  else
    start_address = 0xffc0 << 2;
#ifdef DEBUG
  if (d10v_debug)
    sim_io_printf (sd, "sim_create_inferior:  PC=0x%" PRIx64 "\n",
		   (uint64_t) start_address);
#endif
  {
    SIM_CPU *cpu = STATE_CPU (sd, 0);
    SET_CREG (PC_CR, start_address >> 2);
  }

  /* cpu resets imap0 to 0 and imap1 to 0x7f, but D10V-EVA board
     initializes imap0 and imap1 to 0x1000 as part of its ROM
     initialization. */
  if (old_segment_mapping)
    {
      /* External memory startup.  This is the HARD reset state. */
      set_imap_register (sd, 0, 0x0000);
      set_imap_register (sd, 1, 0x007f);
      set_dmap_register (sd, 0, 0x2000);
      set_dmap_register (sd, 1, 0x2000);
      set_dmap_register (sd, 2, 0x0000); /* Old DMAP */
      set_dmap_register (sd, 3, 0x0000);
    }
  else
    {
      /* Internal memory startup. This is the ROM intialized state. */
      set_imap_register (sd, 0, 0x1000);
      set_imap_register (sd, 1, 0x1000);
      set_dmap_register (sd, 0, 0x2000);
      set_dmap_register (sd, 1, 0x2000);
      set_dmap_register (sd, 2, 0x2000); /* DMAP2 initial internal value is
					    0x2000 on the new board. */
      set_dmap_register (sd, 3, 0x0000);
    }

  SLOT_FLUSH ();
  return SIM_RC_OK;
}

static int
d10v_reg_fetch (SIM_CPU *cpu, int rn, void *memory, int length)
{
  SIM_DESC sd = CPU_STATE (cpu);
  int size;
  switch ((enum sim_d10v_regs) rn)
    {
    case SIM_D10V_R0_REGNUM:
    case SIM_D10V_R1_REGNUM:
    case SIM_D10V_R2_REGNUM:
    case SIM_D10V_R3_REGNUM:
    case SIM_D10V_R4_REGNUM:
    case SIM_D10V_R5_REGNUM:
    case SIM_D10V_R6_REGNUM:
    case SIM_D10V_R7_REGNUM:
    case SIM_D10V_R8_REGNUM:
    case SIM_D10V_R9_REGNUM:
    case SIM_D10V_R10_REGNUM:
    case SIM_D10V_R11_REGNUM:
    case SIM_D10V_R12_REGNUM:
    case SIM_D10V_R13_REGNUM:
    case SIM_D10V_R14_REGNUM:
    case SIM_D10V_R15_REGNUM:
      WRITE_16 (memory, GPR (rn - SIM_D10V_R0_REGNUM));
      size = 2;
      break;
    case SIM_D10V_CR0_REGNUM:
    case SIM_D10V_CR1_REGNUM:
    case SIM_D10V_CR2_REGNUM:
    case SIM_D10V_CR3_REGNUM:
    case SIM_D10V_CR4_REGNUM:
    case SIM_D10V_CR5_REGNUM:
    case SIM_D10V_CR6_REGNUM:
    case SIM_D10V_CR7_REGNUM:
    case SIM_D10V_CR8_REGNUM:
    case SIM_D10V_CR9_REGNUM:
    case SIM_D10V_CR10_REGNUM:
    case SIM_D10V_CR11_REGNUM:
    case SIM_D10V_CR12_REGNUM:
    case SIM_D10V_CR13_REGNUM:
    case SIM_D10V_CR14_REGNUM:
    case SIM_D10V_CR15_REGNUM:
      WRITE_16 (memory, CREG (rn - SIM_D10V_CR0_REGNUM));
      size = 2;
      break;
    case SIM_D10V_A0_REGNUM:
    case SIM_D10V_A1_REGNUM:
      WRITE_64 (memory, ACC (rn - SIM_D10V_A0_REGNUM));
      size = 8;
      break;
    case SIM_D10V_SPI_REGNUM:
      /* PSW_SM indicates that the current SP is the USER
         stack-pointer. */
      WRITE_16 (memory, spi_register ());
      size = 2;
      break;
    case SIM_D10V_SPU_REGNUM:
      /* PSW_SM indicates that the current SP is the USER
         stack-pointer. */
      WRITE_16 (memory, spu_register ());
      size = 2;
      break;
    case SIM_D10V_IMAP0_REGNUM:
    case SIM_D10V_IMAP1_REGNUM:
      WRITE_16 (memory, imap_register (sd, cpu, NULL, rn - SIM_D10V_IMAP0_REGNUM));
      size = 2;
      break;
    case SIM_D10V_DMAP0_REGNUM:
    case SIM_D10V_DMAP1_REGNUM:
    case SIM_D10V_DMAP2_REGNUM:
    case SIM_D10V_DMAP3_REGNUM:
      WRITE_16 (memory, dmap_register (sd, cpu, NULL, rn - SIM_D10V_DMAP0_REGNUM));
      size = 2;
      break;
    case SIM_D10V_TS2_DMAP_REGNUM:
      size = 0;
      break;
    default:
      size = 0;
      break;
    }
  return size;
}
 
static int
d10v_reg_store (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  SIM_DESC sd = CPU_STATE (cpu);
  int size;
  switch ((enum sim_d10v_regs) rn)
    {
    case SIM_D10V_R0_REGNUM:
    case SIM_D10V_R1_REGNUM:
    case SIM_D10V_R2_REGNUM:
    case SIM_D10V_R3_REGNUM:
    case SIM_D10V_R4_REGNUM:
    case SIM_D10V_R5_REGNUM:
    case SIM_D10V_R6_REGNUM:
    case SIM_D10V_R7_REGNUM:
    case SIM_D10V_R8_REGNUM:
    case SIM_D10V_R9_REGNUM:
    case SIM_D10V_R10_REGNUM:
    case SIM_D10V_R11_REGNUM:
    case SIM_D10V_R12_REGNUM:
    case SIM_D10V_R13_REGNUM:
    case SIM_D10V_R14_REGNUM:
    case SIM_D10V_R15_REGNUM:
      SET_GPR (rn - SIM_D10V_R0_REGNUM, READ_16 (memory));
      size = 2;
      break;
    case SIM_D10V_CR0_REGNUM:
    case SIM_D10V_CR1_REGNUM:
    case SIM_D10V_CR2_REGNUM:
    case SIM_D10V_CR3_REGNUM:
    case SIM_D10V_CR4_REGNUM:
    case SIM_D10V_CR5_REGNUM:
    case SIM_D10V_CR6_REGNUM:
    case SIM_D10V_CR7_REGNUM:
    case SIM_D10V_CR8_REGNUM:
    case SIM_D10V_CR9_REGNUM:
    case SIM_D10V_CR10_REGNUM:
    case SIM_D10V_CR11_REGNUM:
    case SIM_D10V_CR12_REGNUM:
    case SIM_D10V_CR13_REGNUM:
    case SIM_D10V_CR14_REGNUM:
    case SIM_D10V_CR15_REGNUM:
      SET_CREG (rn - SIM_D10V_CR0_REGNUM, READ_16 (memory));
      size = 2;
      break;
    case SIM_D10V_A0_REGNUM:
    case SIM_D10V_A1_REGNUM:
      SET_ACC (rn - SIM_D10V_A0_REGNUM, READ_64 (memory) & MASK40);
      size = 8;
      break;
    case SIM_D10V_SPI_REGNUM:
      /* PSW_SM indicates that the current SP is the USER
         stack-pointer. */
      set_spi_register (READ_16 (memory));
      size = 2;
      break;
    case SIM_D10V_SPU_REGNUM:
      set_spu_register (READ_16 (memory));
      size = 2;
      break;
    case SIM_D10V_IMAP0_REGNUM:
    case SIM_D10V_IMAP1_REGNUM:
      set_imap_register (sd, rn - SIM_D10V_IMAP0_REGNUM, READ_16(memory));
      size = 2;
      break;
    case SIM_D10V_DMAP0_REGNUM:
    case SIM_D10V_DMAP1_REGNUM:
    case SIM_D10V_DMAP2_REGNUM:
    case SIM_D10V_DMAP3_REGNUM:
      set_dmap_register (sd, rn - SIM_D10V_DMAP0_REGNUM, READ_16(memory));
      size = 2;
      break;
    case SIM_D10V_TS2_DMAP_REGNUM:
      size = 0;
      break;
    default:
      size = 0;
      break;
    }
  SLOT_FLUSH ();
  return size;
}
