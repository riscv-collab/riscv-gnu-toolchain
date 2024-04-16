/* This must come before any other includes.  */
#include "defs.h"

#include "sim-main.h"
#include "sim-options.h"
#include "v850-sim.h"
#include "sim-assert.h"
#include "itable.h"

#include <stdlib.h>
#include <string.h>

#include "bfd.h"

#include "target-newlib-syscall.h"

static const char * get_insn_name (sim_cpu *, int);

/* For compatibility.  */
SIM_DESC simulator;

/* V850 interrupt model.  */

enum interrupt_type
{
  int_reset,
  int_nmi,
  int_intov1,
  int_intp10,
  int_intp11,
  int_intp12,
  int_intp13,
  int_intcm4,
  num_int_types
};

const char *interrupt_names[] =
{
  "reset",
  "nmi",
  "intov1",
  "intp10",
  "intp11",
  "intp12",
  "intp13",
  "intcm4",
  NULL
};

static void
do_interrupt (SIM_DESC sd, void *data)
{
  sim_cpu *cpu = STATE_CPU (sd, 0);
  struct v850_sim_cpu *v850_cpu = V850_SIM_CPU (cpu);
  const char **interrupt_name = (const char**)data;
  enum interrupt_type inttype;
  inttype = (interrupt_name - STATE_WATCHPOINTS (sd)->interrupt_names);

  /* For a hardware reset, drop everything and jump to the start
     address */
  if (inttype == int_reset)
    {
      PC = 0;
      PSW = 0x20;
      ECR = 0;
      sim_engine_restart (sd, NULL, NULL, NULL_CIA);
    }

  /* Deliver an NMI when allowed */
  if (inttype == int_nmi)
    {
      if (PSW & PSW_NP)
	{
	  /* We're already working on an NMI, so this one must wait
	     around until the previous one is done.  The processor
	     ignores subsequent NMIs, so we don't need to count them.
	     Just keep re-scheduling a single NMI until it manages to
	     be delivered */
	  if (v850_cpu->pending_nmi != NULL)
	    sim_events_deschedule (sd, v850_cpu->pending_nmi);
	  v850_cpu->pending_nmi =
	    sim_events_schedule (sd, 1, do_interrupt, data);
	  return;
	}
      else
	{
	  /* NMI can be delivered.  Do not deschedule pending_nmi as
             that, if still in the event queue, is a second NMI that
             needs to be delivered later. */
	  FEPC = PC;
	  FEPSW = PSW;
	  /* Set the FECC part of the ECR. */
	  ECR &= 0x0000ffff;
	  ECR |= 0x10;
	  PSW |= PSW_NP;
	  PSW &= ~PSW_EP;
	  PSW |= PSW_ID;
	  PC = 0x10;
	  sim_engine_restart (sd, NULL, NULL, NULL_CIA);
	}
    }

  /* deliver maskable interrupt when allowed */
  if (inttype > int_nmi && inttype < num_int_types)
    {
      if ((PSW & PSW_NP) || (PSW & PSW_ID))
	{
	  /* Can't deliver this interrupt, reschedule it for later */
	  sim_events_schedule (sd, 1, do_interrupt, data);
	  return;
	}
      else
	{
	  /* save context */
	  EIPC = PC;
	  EIPSW = PSW;
	  /* Disable further interrupts.  */
	  PSW |= PSW_ID;
	  /* Indicate that we're doing interrupt not exception processing.  */
	  PSW &= ~PSW_EP;
	  /* Clear the EICC part of the ECR, will set below. */
	  ECR &= 0xffff0000;
	  switch (inttype)
	    {
	    case int_intov1:
	      PC = 0x80;
	      ECR |= 0x80;
	      break;
	    case int_intp10:
	      PC = 0x90;
	      ECR |= 0x90;
	      break;
	    case int_intp11:
	      PC = 0xa0;
	      ECR |= 0xa0;
	      break;
	    case int_intp12:
	      PC = 0xb0;
	      ECR |= 0xb0;
	      break;
	    case int_intp13:
	      PC = 0xc0;
	      ECR |= 0xc0;
	      break;
	    case int_intcm4:
	      PC = 0xd0;
	      ECR |= 0xd0;
	      break;
	    default:
	      /* Should never be possible.  */
	      sim_engine_abort (sd, NULL, NULL_CIA,
				"do_interrupt - internal error - bad switch");
	      break;
	    }
	}
      sim_engine_restart (sd, NULL, NULL, NULL_CIA);
    }
  
  /* some other interrupt? */
  sim_engine_abort (sd, NULL, NULL_CIA,
		    "do_interrupt - internal error - interrupt %d unknown",
		    inttype);
}

/* Return name of an insn, used by insn profiling.  */

static const char *
get_insn_name (sim_cpu *cpu, int i)
{
  return itable[i].name;
}

/* These default values correspond to expected usage for the chip.  */

uint32_t OP[4];

static sim_cia
v850_pc_get (sim_cpu *cpu)
{
  return PC;
}

static void
v850_pc_set (sim_cpu *cpu, sim_cia pc)
{
  PC = pc;
}

static int v850_reg_fetch (SIM_CPU *, int, void *, int);
static int v850_reg_store (SIM_CPU *, int, const void *, int);

SIM_DESC
sim_open (SIM_OPEN_KIND    kind,
	  host_callback *  cb,
	  struct bfd *     abfd,
	  char * const *   argv)
{
  int i;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  int mach;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_target_byte_order = BFD_ENDIAN_LITTLE;
  cb->syscall_map = cb_v850_syscall_map;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct v850_sim_cpu))
      != SIM_RC_OK)
    return 0;

  /* for compatibility */
  simulator = sd;

  /* FIXME: should be better way of setting up interrupts */
  STATE_WATCHPOINTS (sd)->interrupt_handler = do_interrupt;
  STATE_WATCHPOINTS (sd)->interrupt_names = interrupt_names;

  /* Initialize the mechanism for doing insn profiling.  */
  CPU_INSN_NAME (STATE_CPU (sd, 0)) = get_insn_name;
  CPU_MAX_INSNS (STATE_CPU (sd, 0)) = nr_itable_entries;

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    return 0;

  /* Allocate core managed memory */

  /* "Mirror" the ROM addresses below 1MB. */
  sim_do_commandf (sd, "memory region 0,0x100000,0x%x", V850_ROM_SIZE);
  /* Chunk of ram adjacent to rom */
  sim_do_commandf (sd, "memory region 0x100000,0x%x", V850_LOW_END-0x100000);
  /* peripheral I/O region - mirror 1K across 4k (0x1000) */
  sim_do_command (sd, "memory region 0xfff000,0x1000,1024");
  /* similarly if in the internal RAM region */
  sim_do_command (sd, "memory region 0xffe000,0x1000,1024");

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* check for/establish the a reference program image */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  /* establish any remaining configuration options */
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


  /* determine the machine type */
  if (STATE_ARCHITECTURE (sd) != NULL
      && (STATE_ARCHITECTURE (sd)->arch == bfd_arch_v850
	  || STATE_ARCHITECTURE (sd)->arch == bfd_arch_v850_rh850))
    mach = STATE_ARCHITECTURE (sd)->mach;
  else
    mach = bfd_mach_v850; /* default */

  /* set machine specific configuration */
  switch (mach)
    {
    case bfd_mach_v850:
    case bfd_mach_v850e:
    case bfd_mach_v850e1:
    case bfd_mach_v850e2:
    case bfd_mach_v850e2v3:
    case bfd_mach_v850e3v5:
      V850_SIM_CPU (STATE_CPU (sd, 0))->psw_mask =
	(PSW_NP | PSW_EP | PSW_ID | PSW_SAT | PSW_CY | PSW_OV | PSW_S | PSW_Z);
      break;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = v850_reg_fetch;
      CPU_REG_STORE (cpu) = v850_reg_store;
      CPU_PC_FETCH (cpu) = v850_pc_get;
      CPU_PC_STORE (cpu) = v850_pc_set;
    }

  return sd;
}

SIM_RC
sim_create_inferior (SIM_DESC      sd,
		     struct bfd *  prog_bfd,
		     char * const *argv,
		     char * const *env)
{
  memset (&State, 0, sizeof (State));
  if (prog_bfd != NULL)
    PC = bfd_get_start_address (prog_bfd);
  return SIM_RC_OK;
}

static int
v850_reg_fetch (SIM_CPU *cpu, int rn, void *memory, int length)
{
  *(uint32_t*)memory = H2T_4 (State.regs[rn]);
  return -1;
}

static int
v850_reg_store (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  State.regs[rn] = T2H_4 (*(uint32_t *) memory);
  return length;
}
