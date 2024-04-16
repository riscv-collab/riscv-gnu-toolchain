#ifndef V850_SIM_H
#define V850_SIM_H

#include <stdint.h>

struct simops
{
  unsigned long   opcode;
  unsigned long   mask;
  int (* func) (void);
  int    numops;
  int    operands[12];
};

#include "simops.h"

typedef uint32_t reg_t;
typedef uint64_t reg64_t;


/* The current state of the processor; registers, memory, etc.  */

typedef struct _v850_regs {
  reg_t regs[32];		/* general-purpose registers */
  reg_t sregs[32];		/* system registers, including psw */
  reg_t pc;
  int dummy_mem;                /* where invalid accesses go */
  reg_t mpu0_sregs[28];         /* mpu0 system registers */
  reg_t mpu1_sregs[28];         /* mpu1 system registers */
  reg_t fpu_sregs[28];          /* fpu system registers */
  reg_t selID_sregs[7][32];	/* system registers, selID 1 thru selID 7 */
  reg64_t vregs[32];		/* vector registers.  */
} v850_regs;

struct v850_sim_cpu {
  v850_regs reg;
  reg_t psw_mask;               /* only allow non-reserved bits to be set */
  sim_event *pending_nmi;
};

#define V850_SIM_CPU(cpu) ((struct v850_sim_cpu *) CPU_ARCH_DATA (cpu))

/* For compatibility, until all functions converted to passing
   SIM_DESC as an argument */
extern SIM_DESC simulator;


#define V850_ROM_SIZE 0x8000
#define V850_LOW_END 0x200000
#define V850_HIGH_START 0xffe000


/* Because we are still using the old semantic table, provide compat
   macro's that store the instruction where the old simops expects
   it. */

extern uint32_t OP[4];
#if 0
OP[0] = inst & 0x1f;           /* RRRRR -> reg1 */
OP[1] = (inst >> 11) & 0x1f;   /* rrrrr -> reg2 */
OP[2] = (inst >> 16) & 0xffff; /* wwwww -> reg3 OR imm16 */
OP[3] = inst;
#endif

#define SAVE_1 \
PC = cia; \
OP[0] = instruction_0 & 0x1f; \
OP[1] = (instruction_0 >> 11) & 0x1f; \
OP[2] = 0; \
OP[3] = instruction_0

#define COMPAT_1(CALL) \
SAVE_1; \
PC += (CALL); \
nia = PC

#define SAVE_2 \
PC = cia; \
OP[0] = instruction_0 & 0x1f; \
OP[1] = (instruction_0 >> 11) & 0x1f; \
OP[2] = instruction_1; \
OP[3] = (instruction_1 << 16) | instruction_0

#define COMPAT_2(CALL) \
SAVE_2; \
PC += (CALL); \
nia = PC


/* new */
#define GR  (V850_SIM_CPU (CPU)->reg.regs)
#define SR  (V850_SIM_CPU (CPU)->reg.sregs)
#define VR  (V850_SIM_CPU (CPU)->reg.vregs)
#define MPU0_SR  (V850_SIM_CPU (CPU)->reg.mpu0_sregs)
#define MPU1_SR  (V850_SIM_CPU (CPU)->reg.mpu1_sregs)
#define FPU_SR   (V850_SIM_CPU (CPU)->reg.fpu_sregs)

/* old */
#define State    (V850_SIM_CPU (STATE_CPU (simulator, 0))->reg)
#define PC	(State.pc)
#define SP_REGNO        3
#define SP      (State.regs[SP_REGNO])
#define EP	(State.regs[30])

#define EIPC  (State.sregs[0])
#define EIPSW (State.sregs[1])
#define FEPC  (State.sregs[2])
#define FEPSW (State.sregs[3])
#define ECR   (State.sregs[4])
#define PSW   (State.sregs[5])
#define PSW_REGNO   5
#define EIIC  (State.sregs[13])
#define FEIC  (State.sregs[14])
#define DBIC  (SR[15])
#define CTPC  (SR[16])
#define CTPSW (SR[17])
#define DBPC  (State.sregs[18])
#define DBPSW (State.sregs[19])
#define CTBP  (State.sregs[20])
#define DIR   (SR[21])
#define EIWR  (SR[28])
#define FEWR  (SR[29])
#define DBWR  (SR[30])
#define BSEL  (SR[31])

#define PSW_US BIT32 (8)
#define PSW_NP 0x80
#define PSW_EP 0x40
#define PSW_ID 0x20
#define PSW_SAT 0x10
#define PSW_CY 0x8
#define PSW_OV 0x4
#define PSW_S 0x2
#define PSW_Z 0x1

#define PSW_NPV	(1<<18)
#define PSW_DMP	(1<<17)
#define PSW_IMP	(1<<16)

#define ECR_EICC 0x0000ffff
#define ECR_FECC 0xffff0000

/* FPU */

#define FPSR  (FPU_SR[6])
#define FPSR_REGNO 6
#define FPEPC (FPU_SR[7])
#define FPST  (FPU_SR[8])
#define FPST_REGNO 8
#define FPCC  (FPU_SR[9])
#define FPCFG (FPU_SR[10])
#define FPCFG_REGNO 10

#define FPSR_DEM  0x00200000
#define FPSR_SEM  0x00100000
#define FPSR_RM   0x000c0000
#define FPSR_RN   0x00000000
#define FPSR_FS   0x00020000
#define FPSR_PR   0x00010000

#define FPSR_XC   0x0000fc00
#define FPSR_XCE  0x00008000
#define FPSR_XCV  0x00004000
#define FPSR_XCZ  0x00002000
#define FPSR_XCO  0x00001000
#define FPSR_XCU  0x00000800
#define FPSR_XCI  0x00000400

#define FPSR_XE   0x000003e0
#define FPSR_XEV  0x00000200
#define FPSR_XEZ  0x00000100
#define FPSR_XEO  0x00000080
#define FPSR_XEU  0x00000040
#define FPSR_XEI  0x00000020

#define FPSR_XP   0x0000001f
#define FPSR_XPV  0x00000010
#define FPSR_XPZ  0x00000008
#define FPSR_XPO  0x00000004
#define FPSR_XPU  0x00000002
#define FPSR_XPI  0x00000001

#define FPST_PR   0x00008000
#define FPST_XCE  0x00002000
#define FPST_XCV  0x00001000
#define FPST_XCZ  0x00000800
#define FPST_XCO  0x00000400
#define FPST_XCU  0x00000200
#define FPST_XCI  0x00000100

#define FPST_XPV  0x00000010
#define FPST_XPZ  0x00000008
#define FPST_XPO  0x00000004
#define FPST_XPU  0x00000002
#define FPST_XPI  0x00000001

#define FPCFG_RM   0x00000180
#define FPCFG_XEV  0x00000010
#define FPCFG_XEZ  0x00000008
#define FPCFG_XEO  0x00000004
#define FPCFG_XEU  0x00000002
#define FPCFG_XEI  0x00000001

#define GET_FPCC()\
 ((FPSR >> 24) &0xf)

#define CLEAR_FPCC(bbb)\
  (FPSR &= ~(1 << (bbb+24)))

#define SET_FPCC(bbb)\
 (FPSR |= 1 << (bbb+24))

#define TEST_FPCC(bbb)\
  ((FPSR & (1 << (bbb+24))) != 0)

#define FPSR_GET_ROUND()					\
  (((FPSR & FPSR_RM) == FPSR_RN) ? sim_fpu_round_near		\
   : ((FPSR & FPSR_RM) == 0x00040000) ? sim_fpu_round_up	\
   : ((FPSR & FPSR_RM) == 0x00080000) ? sim_fpu_round_down	\
   : sim_fpu_round_zero)


enum FPU_COMPARE {
  FPU_CMP_F = 0,
  FPU_CMP_UN,
  FPU_CMP_EQ,
  FPU_CMP_UEQ,
  FPU_CMP_OLT,
  FPU_CMP_ULT,
  FPU_CMP_OLE,
  FPU_CMP_ULE,
  FPU_CMP_SF,
  FPU_CMP_NGLE,
  FPU_CMP_SEQ,
  FPU_CMP_NGL,
  FPU_CMP_LT,
  FPU_CMP_NGE,
  FPU_CMP_LE,
  FPU_CMP_NGT
};


/* MPU */
#define MPM	(MPU1_SR[0])
#define MPC	(MPU1_SR[1])
#define MPC_REGNO 1
#define TID	(MPU1_SR[2])
#define PPA	(MPU1_SR[3])
#define PPM	(MPU1_SR[4])
#define PPC	(MPU1_SR[5])
#define DCC	(MPU1_SR[6])
#define DCV0	(MPU1_SR[7])
#define DCV1	(MPU1_SR[8])
#define SPAL	(MPU1_SR[10])
#define SPAU	(MPU1_SR[11])
#define IPA0L	(MPU1_SR[12])
#define IPA0U	(MPU1_SR[13])
#define IPA1L	(MPU1_SR[14])
#define IPA1U	(MPU1_SR[15])
#define IPA2L	(MPU1_SR[16])
#define IPA2U	(MPU1_SR[17])
#define IPA3L	(MPU1_SR[18])
#define IPA3U	(MPU1_SR[19])
#define DPA0L	(MPU1_SR[20])
#define DPA0U	(MPU1_SR[21])
#define DPA1L	(MPU1_SR[22])
#define DPA1U	(MPU1_SR[23])
#define DPA2L	(MPU1_SR[24])
#define DPA2U	(MPU1_SR[25])
#define DPA3L	(MPU1_SR[26])
#define DPA3U	(MPU1_SR[27])

#define PPC_PPE 0x1
#define SPAL_SPE 0x1
#define SPAL_SPS 0x10

#define VIP	(MPU0_SR[0])
#define VMECR	(MPU0_SR[4])
#define VMTID	(MPU0_SR[5])
#define VMADR	(MPU0_SR[6])
#define VPECR	(MPU0_SR[8])
#define VPTID	(MPU0_SR[9])
#define VPADR	(MPU0_SR[10])
#define VDECR	(MPU0_SR[12])
#define VDTID	(MPU0_SR[13])

#define MPM_AUE	0x2
#define MPM_MPE	0x1

#define VMECR_VMX   0x2
#define VMECR_VMR   0x4
#define VMECR_VMW   0x8
#define VMECR_VMS   0x10
#define VMECR_VMRMW 0x20
#define VMECR_VMMS  0x40

#define IPA2ADDR(IPA)	((IPA) & 0x1fffff80)
#define IPA_IPE	0x1
#define IPA_IPX	0x2
#define IPA_IPR	0x4
#define IPE0	(IPA0L & IPA_IPE)
#define IPE1	(IPA1L & IPA_IPE)
#define IPE2	(IPA2L & IPA_IPE)
#define IPE3	(IPA3L & IPA_IPE)
#define IPX0	(IPA0L & IPA_IPX)
#define IPX1	(IPA1L & IPA_IPX)
#define IPX2	(IPA2L & IPA_IPX)
#define IPX3	(IPA3L & IPA_IPX)
#define IPR0	(IPA0L & IPA_IPR)
#define IPR1	(IPA1L & IPA_IPR)
#define IPR2	(IPA2L & IPA_IPR)
#define IPR3	(IPA3L & IPA_IPR)

#define DPA2ADDR(DPA)	((DPA) & 0x1fffff80)
#define DPA_DPE 0x1
#define DPA_DPR 0x4
#define DPA_DPW 0x8
#define DPE0	(DPA0L & DPA_DPE)
#define DPE1	(DPA1L & DPA_DPE)
#define DPE2	(DPA2L & DPA_DPE)
#define DPE3	(DPA3L & DPA_DPE)
#define DPR0	(DPA0L & DPA_DPR)
#define DPR1	(DPA1L & DPA_DPR)
#define DPR2	(DPA2L & DPA_DPR)
#define DPR3	(DPA3L & DPA_DPR)
#define DPW0	(DPA0L & DPA_DPW)
#define DPW1	(DPA1L & DPA_DPW)
#define DPW2	(DPA2L & DPA_DPW)
#define DPW3	(DPA3L & DPA_DPW)

#define DCC_DCE0 0x1
#define DCC_DCE1 0x10000

#define PPA2ADDR(PPA)	((PPA) & 0x1fffff80)
#define PPC_PPC 0xfffffffe
#define PPC_PPE 0x1
#define PPC_PPM 0x0000fff8


#define SEXT3(x)	((((x)&0x7)^(~0x3))+0x4)

/* sign-extend a 4-bit number */
#define SEXT4(x)	((((x)&0xf)^(~0x7))+0x8)

/* sign-extend a 5-bit number */
#define SEXT5(x)	((((x)&0x1f)^(~0xf))+0x10)

/* sign-extend a 9-bit number */
#define SEXT9(x)	((((x)&0x1ff)^(~0xff))+0x100)

/* sign-extend a 22-bit number */
#define SEXT22(x)	((((x)&0x3fffff)^(~0x1fffff))+0x200000)

/* sign extend a 40 bit number */
#define SEXT40(x)	((((x) & UNSIGNED64 (0xffffffffff)) \
			  ^ (~UNSIGNED64 (0x7fffffffff))) \
			 + UNSIGNED64 (0x8000000000))

/* sign extend a 44 bit number */
#define SEXT44(x)	((((x) & UNSIGNED64 (0xfffffffffff)) \
			  ^ (~ UNSIGNED64 (0x7ffffffffff))) \
			 + UNSIGNED64 (0x80000000000))

/* sign extend a 60 bit number */
#define SEXT60(x)	((((x) & UNSIGNED64 (0xfffffffffffffff)) \
			  ^ (~ UNSIGNED64 (0x7ffffffffffffff))) \
			 + UNSIGNED64 (0x800000000000000))

/* No sign extension */
#define NOP(x)		(x)

#define INC_ADDR(x,i)	x = ((State.MD && x == MOD_E) ? MOD_S : (x)+(i))

#define RLW(x) load_mem (x, 4)

/* Function declarations.  */

#define IMEM16(EA) \
sim_core_read_aligned_2 (CPU, PC, exec_map, (EA))

#define IMEM16_IMMED(EA,N) \
sim_core_read_aligned_2 (STATE_CPU (SD, 0), \
			 PC, exec_map, (EA) + (N) * 2)

#define load_mem(ADDR,LEN) \
sim_core_read_unaligned_##LEN (STATE_CPU (simulator, 0), \
			       PC, read_map, (ADDR))

#define store_mem(ADDR,LEN,DATA) \
sim_core_write_unaligned_##LEN (STATE_CPU (simulator, 0), \
				PC, write_map, (ADDR), (DATA))


/* compare cccc field against PSW */
int condition_met (unsigned code);


/* Debug/tracing calls */

enum op_types
{
  OP_UNKNOWN,
  OP_NONE,
  OP_TRAP,
  OP_REG,
  OP_REG_REG,
  OP_REG_REG_CMP,
  OP_REG_REG_MOVE,
  OP_IMM_REG,
  OP_IMM_REG_CMP,
  OP_IMM_REG_MOVE,
  OP_COND_BR,
  OP_LOAD16,
  OP_STORE16,
  OP_LOAD32,
  OP_STORE32,
  OP_JUMP,
  OP_IMM_REG_REG,
  OP_UIMM_REG_REG,
  OP_IMM16_REG_REG,
  OP_UIMM16_REG_REG,
  OP_BIT,
  OP_EX1,
  OP_EX2,
  OP_LDSR,
  OP_STSR,
  OP_BIT_CHANGE,
  OP_REG_REG_REG,
  OP_REG_REG3,
  OP_IMM_REG_REG_REG,
  OP_PUSHPOP1,
  OP_PUSHPOP2,
  OP_PUSHPOP3,
};

#if WITH_TRACE_ANY_P
void trace_input (char *name, enum op_types type, int size);
void trace_output (enum op_types result);
void trace_result (int has_result, uint32_t result);

extern int trace_num_values;
extern uint32_t trace_values[];
extern uint32_t trace_pc;
extern const char *trace_name;
extern int trace_module;

#define TRACE_BRANCH0() \
do { \
  if (TRACE_BRANCH_P (CPU)) { \
    trace_module = TRACE_BRANCH_IDX; \
    trace_pc = cia; \
    trace_name = itable[MY_INDEX].name; \
    trace_num_values = 0; \
    trace_result (1, (nia)); \
  } \
} while (0)

#define TRACE_BRANCH1(IN1) \
do { \
  if (TRACE_BRANCH_P (CPU)) { \
    trace_module = TRACE_BRANCH_IDX; \
    trace_pc = cia; \
    trace_name = itable[MY_INDEX].name; \
    trace_values[0] = (IN1); \
    trace_num_values = 1; \
    trace_result (1, (nia)); \
  } \
} while (0)

#define TRACE_BRANCH2(IN1, IN2) \
do { \
  if (TRACE_BRANCH_P (CPU)) { \
    trace_module = TRACE_BRANCH_IDX; \
    trace_pc = cia; \
    trace_name = itable[MY_INDEX].name; \
    trace_values[0] = (IN1); \
    trace_values[1] = (IN2); \
    trace_num_values = 2; \
    trace_result (1, (nia)); \
  } \
} while (0)

#define TRACE_BRANCH3(IN1, IN2, IN3) \
do { \
  if (TRACE_BRANCH_P (CPU)) { \
    trace_module = TRACE_BRANCH_IDX; \
    trace_pc = cia; \
    trace_name = itable[MY_INDEX].name; \
    trace_values[0] = (IN1); \
    trace_values[1] = (IN2); \
    trace_values[2] = (IN3); \
    trace_num_values = 3; \
    trace_result (1, (nia)); \
  } \
} while (0)

#define TRACE_LD(ADDR,RESULT) \
do { \
  if (TRACE_MEMORY_P (CPU)) { \
    trace_module = TRACE_MEMORY_IDX; \
    trace_pc = cia; \
    trace_name = itable[MY_INDEX].name; \
    trace_values[0] = (ADDR); \
    trace_num_values = 1; \
    trace_result (1, (RESULT)); \
  } \
} while (0)

#define TRACE_LD_NAME(NAME, ADDR,RESULT) \
do { \
  if (TRACE_MEMORY_P (CPU)) { \
    trace_module = TRACE_MEMORY_IDX; \
    trace_pc = cia; \
    trace_name = (NAME); \
    trace_values[0] = (ADDR); \
    trace_num_values = 1; \
    trace_result (1, (RESULT)); \
  } \
} while (0)

#define TRACE_ST(ADDR,RESULT) \
do { \
  if (TRACE_MEMORY_P (CPU)) { \
    trace_module = TRACE_MEMORY_IDX; \
    trace_pc = cia; \
    trace_name = itable[MY_INDEX].name; \
    trace_values[0] = (ADDR); \
    trace_num_values = 1; \
    trace_result (1, (RESULT)); \
  } \
} while (0)

#define TRACE_FP_INPUT_FPU1(V0)	\
do { \
  if (TRACE_FPU_P (CPU)) \
    { \
      uint64_t f0; \
      sim_fpu_to64 (&f0, (V0)); \
      trace_input_fp1 (SD, CPU, TRACE_FPU_IDX, f0); \
    } \
} while (0)

#define TRACE_FP_INPUT_FPU2(V0, V1) \
do { \
  if (TRACE_FPU_P (CPU)) \
    { \
      uint64_t f0, f1; \
      sim_fpu_to64 (&f0, (V0)); \
      sim_fpu_to64 (&f1, (V1)); \
      trace_input_fp2 (SD, CPU, TRACE_FPU_IDX, f0, f1);	\
    } \
} while (0)

#define TRACE_FP_INPUT_FPU3(V0, V1, V2) \
do { \
  if (TRACE_FPU_P (CPU)) \
    { \
      uint64_t f0, f1, f2; \
      sim_fpu_to64 (&f0, (V0)); \
      sim_fpu_to64 (&f1, (V1)); \
      sim_fpu_to64 (&f2, (V2)); \
      trace_input_fp3 (SD, CPU, TRACE_FPU_IDX, f0, f1, f2); \
    } \
} while (0)

#define TRACE_FP_INPUT_BOOL1_FPU2(V0, V1, V2) \
do { \
  if (TRACE_FPU_P (CPU)) \
    { \
      int d0 = (V0); \
      uint64_t f1, f2; \
      TRACE_DATA *data = CPU_TRACE_DATA (CPU); \
      TRACE_IDX (data) = TRACE_FPU_IDX;	\
      sim_fpu_to64 (&f1, (V1)); \
      sim_fpu_to64 (&f2, (V2)); \
      save_data (SD, data, trace_fmt_bool, sizeof (d0), &d0); \
      save_data (SD, data, trace_fmt_fp, sizeof (fp_word), &f1); \
      save_data (SD, data, trace_fmt_fp, sizeof (fp_word), &f2); \
    } \
} while (0)

#define TRACE_FP_INPUT_WORD2(V0, V1) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_input_word2 (SD, CPU, TRACE_FPU_IDX, (V0), (V1)); \
} while (0)

#define TRACE_FP_RESULT_FPU1(R0) \
do { \
  if (TRACE_FPU_P (CPU)) \
    { \
      uint64_t f0; \
      sim_fpu_to64 (&f0, (R0));	\
      trace_result_fp1 (SD, CPU, TRACE_FPU_IDX, f0); \
    } \
} while (0)

#define TRACE_FP_RESULT_WORD1(R0) TRACE_FP_RESULT_WORD(R0)

#define TRACE_FP_RESULT_WORD2(R0, R1) \
do { \
  if (TRACE_FPU_P (CPU)) \
    trace_result_word2 (SD, CPU, TRACE_FPU_IDX, (R0), (R1)); \
} while (0)

#else
#define trace_input(NAME, IN1, IN2)
#define trace_output(RESULT)
#define trace_result(HAS_RESULT, RESULT)

#define TRACE_BRANCH0()
#define TRACE_BRANCH1(IN1)
#define TRACE_BRANCH2(IN1, IN2)
#define TRACE_BRANCH3(IN1, IN2, IN3)

#define TRACE_LD(ADDR,RESULT)
#define TRACE_ST(ADDR,RESULT)

#endif

#define GPR_SET(N, VAL) (State.regs[(N)] = (VAL))
#define GPR_CLEAR(N)    (State.regs[(N)] = 0)

extern void divun ( unsigned int       N,
		    unsigned long int  als,
		    unsigned long int  sfi,
		    uint32_t /*unsigned long int*/ *  quotient_ptr,
		    uint32_t /*unsigned long int*/ *  remainder_ptr,
		    int *overflow_ptr
		    );
extern void divn ( unsigned int       N,
		   unsigned long int  als,
		   unsigned long int  sfi,
		   int32_t /*signed long int*/ *  quotient_ptr,
		   int32_t /*signed long int*/ *  remainder_ptr,
		   int *overflow_ptr
		   );
extern int type1_regs[];
extern int type2_regs[];
extern int type3_regs[];

#define SESR_OV   (1 << 0)
#define SESR_SOV  (1 << 1)

#define SESR      (State.sregs[12])

#define ROUND_Q62_Q31(X) ((((X) + (1 << 30)) >> 31) & 0xffffffff)
#define ROUND_Q62_Q15(X) ((((X) + (1 << 30)) >> 47) & 0xffff)
#define ROUND_Q31_Q15(X) ((((X) + (1 << 15)) >> 15) & 0xffff)
#define ROUND_Q30_Q15(X) ((((X) + (1 << 14)) >> 15) & 0xffff)

#define SAT16(X)			\
  do					\
    {					\
      int64_t z = (X);			\
      if (z > 0x7fff)			\
	{				\
	  SESR |= SESR_OV | SESR_SOV;	\
	  z = 0x7fff;			\
	}				\
      else if (z < -0x8000)		\
	{				\
	  SESR |= SESR_OV | SESR_SOV;	\
	  z = - 0x8000;			\
	}				\
      (X) = z;				\
    }					\
  while (0)

#define SAT32(X)			\
  do					\
    {					\
      int64_t z = (X);			\
      if (z > 0x7fffffff)		\
	{				\
	  SESR |= SESR_OV | SESR_SOV;	\
	  z = 0x7fffffff;		\
	}				\
      else if (z < -0x80000000)		\
	{				\
	  SESR |= SESR_OV | SESR_SOV;	\
	  z = - 0x80000000;		\
	}				\
      (X) = z;				\
    }					\
  while (0)

#define ABS16(X)			\
  do					\
    {					\
      int64_t z = (X) & 0xffff;	\
      if (z == 0x8000)			\
	{				\
	  SESR |= SESR_OV | SESR_SOV;	\
	  z = 0x7fff;			\
	}				\
      else if (z & 0x8000)		\
	{				\
	  z = (- z) & 0xffff;		\
	}				\
      (X) = z;				\
    }					\
  while (0)

#define ABS32(X)			\
  do					\
    {					\
      int64_t z = (X) & 0xffffffff;	\
      if (z == 0x80000000)		\
	{				\
	  SESR |= SESR_OV | SESR_SOV;	\
	  z = 0x7fffffff;		\
	}				\
      else if (z & 0x80000000)		\
	{				\
	  z = (- z) & 0xffffffff;	\
	}				\
      (X) = z;				\
    }					\
  while (0)

#endif
