#ifndef MN10300_SIM_H
#define MN10300_SIM_H

/* For compatibility, until all functions converted to passing
   SIM_DESC as an argument */
extern SIM_DESC simulator;

typedef struct
{
  uint32_t low, high;
} dword;
typedef uint32_t reg_t;

struct simops 
{
  long opcode;
  long mask;
  void (*func)();
  int length;
  int format;
  int numops;
  int operands[16];
};

/* The current state of the processor; registers, memory, etc.  */

struct _state
{
  reg_t regs[32];		/* registers, d0-d3, a0-a3, sp, pc, mdr, psw,
				   lir, lar, mdrq, plus some room for processor
				   specific regs.  */
  union
  {
    reg_t fs[32]; /* FS0-31 */
    dword fd[16]; /* FD0,2,...,30 */
  } fpregs;

  /* All internal state modified by signal_exception() that may need to be
     rolled back for passing moment-of-exception image back to gdb. */
  reg_t exc_trigger_regs[32];
  reg_t exc_suspend_regs[32];
  int exc_suspended;

#define SIM_CPU_EXCEPTION_TRIGGER(SD,CPU,CIA) mn10300_cpu_exception_trigger(SD,CPU,CIA)
#define SIM_CPU_EXCEPTION_SUSPEND(SD,CPU,EXC) mn10300_cpu_exception_suspend(SD,CPU,EXC)
#define SIM_CPU_EXCEPTION_RESUME(SD,CPU,EXC) mn10300_cpu_exception_resume(SD,CPU,EXC)
};

extern struct _state State;

#define PC	(State.regs[REG_PC])
#define SP	(State.regs[REG_SP])

#define PSW (State.regs[11])
#define PSW_Z 0x1
#define PSW_N 0x2
#define PSW_C 0x4
#define PSW_V 0x8
#define PSW_IE LSBIT (11)
#define PSW_LM LSMASK (10, 8)

#define EXTRACT_PSW_LM LSEXTRACTED16 (PSW, 10, 8)
#define INSERT_PSW_LM(l) LSINSERTED16 ((l), 10, 8)

#define REG_D0 0
#define REG_A0 4
#define REG_SP 8
#define REG_PC 9
#define REG_MDR 10
#define REG_PSW 11
#define REG_LIR 12
#define REG_LAR 13
#define REG_MDRQ 14
#define REG_E0 15
#define REG_SSP 23
#define REG_MSP 24
#define REG_USP 25
#define REG_MCRH 26
#define REG_MCRL 27
#define REG_MCVF 28

#define REG_FPCR 29

#define FPCR (State.regs[REG_FPCR])

#define FCC_MASK LSMASK (21, 18)
#define RM_MASK  LSMASK (17, 16) /* Must always be zero.  */
#define EC_MASK  LSMASK (14, 10)
#define EE_MASK  LSMASK ( 9,  5)
#define EF_MASK  LSMASK ( 4,  0)
#define FPCR_MASK (FCC_MASK | EC_MASK | EE_MASK | EF_MASK)

#define FCC_L LSBIT (21)
#define FCC_G LSBIT (20)
#define FCC_E LSBIT (19)
#define FCC_U LSBIT (18)

#define EC_V LSBIT (14)
#define EC_Z LSBIT (13)
#define EC_O LSBIT (12)
#define EC_U LSBIT (11)
#define EC_I LSBIT (10)

#define EE_V LSBIT (9)
#define EE_Z LSBIT (8)
#define EE_O LSBIT (7)
#define EE_U LSBIT (6)
#define EE_I LSBIT (5)

#define EF_V LSBIT (4)
#define EF_Z LSBIT (3)
#define EF_O LSBIT (2)
#define EF_U LSBIT (1)
#define EF_I LSBIT (0)

#define PSW_FE LSBIT(20)
#define FPU_DISABLED !(PSW & PSW_FE)

#define XS2FS(X,S) State.fpregs.fs[((X<<4)|(S))]
#define AS2FS(A,S) State.fpregs.fs[((A<<2)|(S))]
#define Xf2FD(X,f) State.fpregs.fd[((X<<3)|(f))]

#define FS2FPU(FS,F) sim_fpu_32to (&(F), (FS))
#define FD2FPU(FD,F) sim_fpu_232to (&(F), ((FD).high), ((FD).low))
#define FPU2FS(F,FS) sim_fpu_to32 (&(FS), &(F))
#define FPU2FD(F,FD) sim_fpu_to232 (&((FD).high), &((FD).low), &(F))

#define FETCH32(a,b,c,d) \
 ((a)+((b)<<8)+((c)<<16)+((d)<<24))

#define FETCH24(a,b,c) \
 ((a)+((b)<<8)+((c)<<16))

#define FETCH16(a,b) ((a)+((b)<<8))

#define load_byte(ADDR) \
sim_core_read_unaligned_1 (STATE_CPU (simulator, 0), PC, read_map, (ADDR))

#define load_half(ADDR) \
sim_core_read_unaligned_2 (STATE_CPU (simulator, 0), PC, read_map, (ADDR))

#define load_word(ADDR) \
sim_core_read_unaligned_4 (STATE_CPU (simulator, 0), PC, read_map, (ADDR))

#define load_dword(ADDR) \
u642dw (sim_core_read_unaligned_8 (STATE_CPU (simulator, 0), \
				   PC, read_map, (ADDR)))

static INLINE2 dword
u642dw (uint64_t dw)
{
  dword r;

  r.low = (uint32_t)dw;
  r.high = (uint32_t)(dw >> 32);
  return r;
}

#define store_byte(ADDR, DATA) \
sim_core_write_unaligned_1 (STATE_CPU (simulator, 0), \
			    PC, write_map, (ADDR), (DATA))


#define store_half(ADDR, DATA) \
sim_core_write_unaligned_2 (STATE_CPU (simulator, 0), \
			    PC, write_map, (ADDR), (DATA))


#define store_word(ADDR, DATA) \
sim_core_write_unaligned_4 (STATE_CPU (simulator, 0), \
			    PC, write_map, (ADDR), (DATA))
#define store_dword(ADDR, DATA) \
sim_core_write_unaligned_8 (STATE_CPU (simulator, 0), \
			    PC, write_map, (ADDR), dw2u64 (DATA))

static INLINE2 uint64_t
dw2u64 (dword data)
{
  return data.low | (((uint64_t)data.high) << 32);
}

/* Bring data in from the cold */

#define IMEM8(EA) \
(sim_core_read_aligned_1(STATE_CPU (SD, 0), EA, exec_map, (EA)))

#define IMEM8_IMMED(EA, N) \
(sim_core_read_aligned_1(STATE_CPU (SD, 0), EA, exec_map, (EA) + (N)))

/* Function declarations.  */

INLINE_SIM_MAIN (void) genericAdd (uint32_t source, uint32_t destReg);
INLINE_SIM_MAIN (void) genericSub (uint32_t source, uint32_t destReg);
INLINE_SIM_MAIN (void) genericCmp (uint32_t leftOpnd, uint32_t rightOpnd);
INLINE_SIM_MAIN (void) genericOr (uint32_t source, uint32_t destReg);
INLINE_SIM_MAIN (void) genericXor (uint32_t source, uint32_t destReg);
INLINE_SIM_MAIN (void) genericBtst (uint32_t leftOpnd, uint32_t rightOpnd);
INLINE_SIM_MAIN (void) do_syscall (SIM_DESC sd);
void program_interrupt (SIM_DESC sd, sim_cpu *cpu, sim_cia cia, SIM_SIGNAL sig)
  ATTRIBUTE_NORETURN;

void mn10300_cpu_exception_trigger(SIM_DESC sd, sim_cpu* cpu, address_word pc);
void mn10300_cpu_exception_suspend(SIM_DESC sd, sim_cpu* cpu, int exception);
void mn10300_cpu_exception_resume(SIM_DESC sd, sim_cpu* cpu, int exception);

void fpu_disabled_exception     (SIM_DESC, sim_cpu *, address_word);
void fpu_unimp_exception        (SIM_DESC, sim_cpu *, address_word);
void fpu_check_signal_exception (SIM_DESC, sim_cpu *, address_word);

extern const struct fp_prec_t
{
  void (* reg2val) (const void *, sim_fpu *);
  int (*  round)   (sim_fpu *);
  void (* val2reg) (const sim_fpu *, void *);
} fp_single_prec, fp_double_prec;

#define FP_SINGLE (&fp_single_prec)
#define FP_DOUBLE (&fp_double_prec)

void fpu_rsqrt  (SIM_DESC, sim_cpu *, address_word, const void *, void *, const struct fp_prec_t *);
void fpu_sqrt   (SIM_DESC, sim_cpu *, address_word, const void *, void *, const struct fp_prec_t *);
void fpu_cmp    (SIM_DESC, sim_cpu *, address_word, const void *, const void *, const struct fp_prec_t *);
void fpu_add    (SIM_DESC, sim_cpu *, address_word, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_sub    (SIM_DESC, sim_cpu *, address_word, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_mul    (SIM_DESC, sim_cpu *, address_word, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_div    (SIM_DESC, sim_cpu *, address_word, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_fmadd  (SIM_DESC, sim_cpu *, address_word, const void *, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_fmsub  (SIM_DESC, sim_cpu *, address_word, const void *, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_fnmadd (SIM_DESC, sim_cpu *, address_word, const void *, const void *, const void *, void *, const struct fp_prec_t *);
void fpu_fnmsub (SIM_DESC, sim_cpu *, address_word, const void *, const void *, const void *, void *, const struct fp_prec_t *);

#endif
