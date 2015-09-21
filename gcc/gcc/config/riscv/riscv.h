/* Definition of RISC-V target for GNU compiler.
   Copyright (C) 2011-2014 Free Software Foundation, Inc.
   Contributed by Andrew Waterman (waterman@cs.berkeley.edu) at UC Berkeley.
   Based on MIPS target for GNU compiler.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/* TARGET_HARD_FLOAT and TARGET_SOFT_FLOAT reflect whether the FPU is
   directly accessible, while the command-line options select
   TARGET_HARD_FLOAT_ABI and TARGET_SOFT_FLOAT_ABI to reflect the ABI
   in use.  */
#define TARGET_HARD_FLOAT TARGET_HARD_FLOAT_ABI
#define TARGET_SOFT_FLOAT TARGET_SOFT_FLOAT_ABI

/* Target CPU builtins.  */
#define TARGET_CPU_CPP_BUILTINS()					\
  do									\
    {									\
      builtin_assert ("machine=riscv");					\
									\
      builtin_assert ("cpu=riscv");					\
      builtin_define ("__riscv__");					\
      builtin_define ("__riscv");					\
      builtin_define ("_riscv");					\
      builtin_define ("__riscv");					\
									\
      if (TARGET_64BIT)							\
	{								\
	  builtin_define ("__riscv64");					\
	  builtin_define ("_RISCV_SIM=_ABI64");				\
	}								\
      else								\
	builtin_define ("_RISCV_SIM=_ABI32");				\
									\
      builtin_define ("_ABI32=1");					\
      builtin_define ("_ABI64=3");					\
									\
									\
      builtin_define_with_int_value ("_RISCV_SZINT", INT_TYPE_SIZE);	\
      builtin_define_with_int_value ("_RISCV_SZLONG", LONG_TYPE_SIZE);	\
      builtin_define_with_int_value ("_RISCV_SZPTR", POINTER_SIZE);	\
      builtin_define_with_int_value ("_RISCV_FPSET", 32);		\
									\
      if (TARGET_RVC)							\
	builtin_define ("__riscv_compressed");				\
									\
      if (TARGET_ATOMIC)						\
	builtin_define ("__riscv_atomic");				\
									\
      /* These defines reflect the ABI in use, not whether the  	\
	 FPU is directly accessible.  */				\
      if (TARGET_HARD_FLOAT_ABI) {					\
	builtin_define ("__riscv_hard_float");				\
	if (TARGET_FDIV) {						\
	  builtin_define ("__riscv_fdiv");				\
	  builtin_define ("__riscv_fsqrt");				\
	}								\
      } else								\
	builtin_define ("__riscv_soft_float");				\
									\
      /* The base RISC-V ISA is always little-endian. */		\
      builtin_define_std ("RISCVEL");					\
      builtin_define ("_RISCVEL");					\
									\
      /* Macros dependent on the C dialect.  */				\
      if (preprocessing_asm_p ())					\
	{								\
	  builtin_define_std ("LANGUAGE_ASSEMBLY");			\
	  builtin_define ("_LANGUAGE_ASSEMBLY");			\
	}								\
      else if (c_dialect_cxx ())					\
	{								\
	  builtin_define ("_LANGUAGE_C_PLUS_PLUS");			\
	  builtin_define ("__LANGUAGE_C_PLUS_PLUS");			\
	  builtin_define ("__LANGUAGE_C_PLUS_PLUS__");			\
	}								\
      else								\
	{								\
	  builtin_define_std ("LANGUAGE_C");				\
	  builtin_define ("_LANGUAGE_C");				\
	}								\
      if (c_dialect_objc ())						\
	{								\
	  builtin_define ("_LANGUAGE_OBJECTIVE_C");			\
	  builtin_define ("__LANGUAGE_OBJECTIVE_C");			\
	  /* Bizarre, but needed at least for Irix.  */			\
	  builtin_define_std ("LANGUAGE_C");				\
	  builtin_define ("_LANGUAGE_C");				\
	}								\
      if (riscv_cmodel == CM_MEDANY)					\
	builtin_define ("_RISCV_CMODEL_MEDANY");			\
    }									\
  while (0)

/* Default target_flags if no switches are specified  */

#ifndef TARGET_DEFAULT
#define TARGET_DEFAULT 0
#endif

#ifndef RISCV_ARCH_STRING_DEFAULT
#define RISCV_ARCH_STRING_DEFAULT "IMAFD"
#endif

#ifndef RISCV_TUNE_STRING_DEFAULT
#define RISCV_TUNE_STRING_DEFAULT "rocket"
#endif

#ifndef TARGET_64BIT_DEFAULT
#define TARGET_64BIT_DEFAULT 1
#endif

#if TARGET_64BIT_DEFAULT
# define MULTILIB_ARCH_DEFAULT "m64"
# define OPT_ARCH64 "!m32"
# define OPT_ARCH32 "m32"
#else
# define MULTILIB_ARCH_DEFAULT "m32"
# define OPT_ARCH64 "m64"
# define OPT_ARCH32 "!m64"
#endif

#ifndef MULTILIB_DEFAULTS
#define MULTILIB_DEFAULTS \
    { MULTILIB_ARCH_DEFAULT }
#endif


/* Support for a compile-time default CPU, et cetera.  The rules are:
   --with-arch is ignored if -march is specified.
   --with-tune is ignored if -mtune is specified.
   --with-float is ignored if -mhard-float or -msoft-float are specified. */
#define OPTION_DEFAULT_SPECS \
  {"arch", "%{!march=*:-march=%(VALUE)}"},			   \
  {"arch_32", "%{" OPT_ARCH32 ":%{m32}}" }, \
  {"arch_64", "%{" OPT_ARCH64 ":%{m64}}" }, \
  {"tune", "%{!mtune=*:-mtune=%(VALUE)}" }, \
  {"float", "%{!msoft-float:%{!mhard-float:-m%(VALUE)-float}}" }, \

#define DRIVER_SELF_SPECS ""

#ifdef IN_LIBGCC2
#undef TARGET_64BIT
/* Make this compile time constant for libgcc2 */
#ifdef __riscv64
#define TARGET_64BIT		1
#else
#define TARGET_64BIT		0
#endif
#endif /* IN_LIBGCC2 */

/* Tell collect what flags to pass to nm.  */
#ifndef NM_FLAGS
#define NM_FLAGS "-Bn"
#endif

#undef ASM_SPEC
#define ASM_SPEC "\
%(subtarget_asm_debugging_spec) \
%{m32} %{m64} %{!m32:%{!m64: %(asm_abi_default_spec)}} \
%{mrvc} %{mno-rvc} \
%{msoft-float} %{mhard-float} \
%{fPIC|fpic|fPIE|fpie:-fpic} \
%{march=*} \
%(subtarget_asm_spec)"

/* Extra switches sometimes passed to the linker.  */

#ifndef LINK_SPEC
#define LINK_SPEC "\
%{!T:-dT riscv.ld} \
%{m64:-melf64lriscv} \
%{m32:-melf32lriscv} \
%{shared}"
#endif  /* LINK_SPEC defined */

/* This macro defines names of additional specifications to put in the specs
   that can be used in various specifications like CC1_SPEC.  Its definition
   is an initializer with a subgrouping for each command option.

   Each subgrouping contains a string constant, that defines the
   specification name, and a string constant that used by the GCC driver
   program.

   Do not define this macro if it does not need to do anything.  */

#define EXTRA_SPECS							\
  { "asm_abi_default_spec", "-" MULTILIB_ARCH_DEFAULT },		\
  SUBTARGET_EXTRA_SPECS

#ifndef SUBTARGET_EXTRA_SPECS
#define SUBTARGET_EXTRA_SPECS
#endif

#define TARGET_DEFAULT_CMODEL CM_MEDLOW

/* By default, turn on GDB extensions.  */
#define DEFAULT_GDB_EXTENSIONS 1

#define LOCAL_LABEL_PREFIX	"."
#define USER_LABEL_PREFIX	""

#define DWARF2_DEBUGGING_INFO 1
#define DWARF2_ASM_LINE_DEBUG_INFO 0

/* The mapping from gcc register number to DWARF 2 CFA column number.  */
#define DWARF_FRAME_REGNUM(REGNO) \
  (GP_REG_P (REGNO) || FP_REG_P (REGNO) ? REGNO : INVALID_REGNUM)

/* The DWARF 2 CFA column which tracks the return address.  */
#define DWARF_FRAME_RETURN_COLUMN RETURN_ADDR_REGNUM

/* Don't emit .cfi_sections, as it does not work */
#undef HAVE_GAS_CFI_SECTIONS_DIRECTIVE
#define HAVE_GAS_CFI_SECTIONS_DIRECTIVE 0

/* Before the prologue, RA lives in r31.  */
#define INCOMING_RETURN_ADDR_RTX gen_rtx_REG (VOIDmode, RETURN_ADDR_REGNUM)

/* Describe how we implement __builtin_eh_return.  */
#define EH_RETURN_DATA_REGNO(N) \
  ((N) < 4 ? (N) + GP_ARG_FIRST : INVALID_REGNUM)

#define EH_RETURN_STACKADJ_RTX  gen_rtx_REG (Pmode, GP_ARG_FIRST + 4)

/* Target machine storage layout */

#define BITS_BIG_ENDIAN 0
#define BYTES_BIG_ENDIAN 0
#define WORDS_BIG_ENDIAN 0

#define MAX_BITS_PER_WORD 64

/* Width of a word, in units (bytes).  */
#define UNITS_PER_WORD (TARGET_64BIT ? 8 : 4)
#ifndef IN_LIBGCC2
#define MIN_UNITS_PER_WORD 4
#endif

/* We currently require both or neither of the `F' and `D' extensions. */
#define UNITS_PER_FPREG 8

/* If FP regs aren't wide enough for a given FP argument, it is passed in
   integer registers. */
#define MIN_FPRS_PER_FMT 1

/* The largest size of value that can be held in floating-point
   registers and moved with a single instruction.  */
#define UNITS_PER_HWFPVALUE \
  (TARGET_SOFT_FLOAT_ABI ? 0 : UNITS_PER_FPREG)

/* The largest size of value that can be held in floating-point
   registers.  */
#define UNITS_PER_FPVALUE			\
  (TARGET_SOFT_FLOAT_ABI ? 0			\
   : LONG_DOUBLE_TYPE_SIZE / BITS_PER_UNIT)

/* The number of bytes in a double.  */
#define UNITS_PER_DOUBLE (TYPE_PRECISION (double_type_node) / BITS_PER_UNIT)

/* Set the sizes of the core types.  */
#define SHORT_TYPE_SIZE 16
#define INT_TYPE_SIZE 32
#define LONG_TYPE_SIZE (TARGET_64BIT ? 64 : 32)
#define LONG_LONG_TYPE_SIZE 64

#define FLOAT_TYPE_SIZE 32
#define DOUBLE_TYPE_SIZE 64
/* XXX The ABI says long doubles are IEEE-754-2008 float128s. */
#define LONG_DOUBLE_TYPE_SIZE 64

#ifdef IN_LIBGCC2
# define LIBGCC2_LONG_DOUBLE_TYPE_SIZE LONG_DOUBLE_TYPE_SIZE
#endif

/* Allocation boundary (in *bits*) for storing arguments in argument list.  */
#define PARM_BOUNDARY BITS_PER_WORD

/* Allocation boundary (in *bits*) for the code of a function.  */
#define FUNCTION_BOUNDARY (TARGET_RVC ? 16 : 32)

/* There is no point aligning anything to a rounder boundary than this.  */
#define BIGGEST_ALIGNMENT 128

/* All accesses must be aligned.  */
#define STRICT_ALIGNMENT 1

/* Define this if you wish to imitate the way many other C compilers
   handle alignment of bitfields and the structures that contain
   them.

   The behavior is that the type written for a bit-field (`int',
   `short', or other integer type) imposes an alignment for the
   entire structure, as if the structure really did contain an
   ordinary field of that type.  In addition, the bit-field is placed
   within the structure so that it would fit within such a field,
   not crossing a boundary for it.

   Thus, on most machines, a bit-field whose type is written as `int'
   would not cross a four-byte boundary, and would force four-byte
   alignment for the whole structure.  (The alignment used may not
   be four bytes; it is controlled by the other alignment
   parameters.)

   If the macro is defined, its definition should be a C expression;
   a nonzero value for the expression enables this behavior.  */

#define PCC_BITFIELD_TYPE_MATTERS 1

/* If defined, a C expression to compute the alignment given to a
   constant that is being placed in memory.  CONSTANT is the constant
   and ALIGN is the alignment that the object would ordinarily have.
   The value of this macro is used instead of that alignment to align
   the object.

   If this macro is not defined, then ALIGN is used.

   The typical use of this macro is to increase alignment for string
   constants to be word aligned so that `strcpy' calls that copy
   constants can be done inline.  */

#define CONSTANT_ALIGNMENT(EXP, ALIGN)					\
  ((TREE_CODE (EXP) == STRING_CST  || TREE_CODE (EXP) == CONSTRUCTOR)	\
   && (ALIGN) < BITS_PER_WORD ? BITS_PER_WORD : (ALIGN))

/* If defined, a C expression to compute the alignment for a static
   variable.  TYPE is the data type, and ALIGN is the alignment that
   the object would ordinarily have.  The value of this macro is used
   instead of that alignment to align the object.

   If this macro is not defined, then ALIGN is used.

   One use of this macro is to increase alignment of medium-size
   data to make it all fit in fewer cache lines.  Another is to
   cause character arrays to be word-aligned so that `strcpy' calls
   that copy constants to character arrays can be done inline.  */

#undef DATA_ALIGNMENT
#define DATA_ALIGNMENT(TYPE, ALIGN)					\
  ((((ALIGN) < BITS_PER_WORD)						\
    && (TREE_CODE (TYPE) == ARRAY_TYPE					\
	|| TREE_CODE (TYPE) == UNION_TYPE				\
	|| TREE_CODE (TYPE) == RECORD_TYPE)) ? BITS_PER_WORD : (ALIGN))

/* We need this for the same reason as DATA_ALIGNMENT, namely to cause
   character arrays to be word-aligned so that `strcpy' calls that copy
   constants to character arrays can be done inline, and 'strcmp' can be
   optimised to use word loads. */
#define LOCAL_ALIGNMENT(TYPE, ALIGN) \
  DATA_ALIGNMENT (TYPE, ALIGN)

/* Define if operations between registers always perform the operation
   on the full register even if a narrower mode is specified.  */
#define WORD_REGISTER_OPERATIONS

/* When in 64-bit mode, move insns will sign extend SImode and CCmode
   moves.  All other references are zero extended.  */
#define LOAD_EXTEND_OP(MODE) \
  (TARGET_64BIT && ((MODE) == SImode || (MODE) == CCmode) \
   ? SIGN_EXTEND : ZERO_EXTEND)

/* Define this macro if it is advisable to hold scalars in registers
   in a wider mode than that declared by the program.  In such cases,
   the value is constrained to be within the bounds of the declared
   type, but kept valid in the wider mode.  The signedness of the
   extension may differ from that of the type.  */

#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE)	\
  if (GET_MODE_CLASS (MODE) == MODE_INT		\
      && GET_MODE_SIZE (MODE) < 4)		\
    {						\
      (MODE) = Pmode;				\
    }

/* Pmode is always the same as ptr_mode, but not always the same as word_mode.
   Extensions of pointers to word_mode must be signed.  */
#define POINTERS_EXTEND_UNSIGNED false

/* RV32 double-precision FP <-> integer moves go through memory */
#define SECONDARY_MEMORY_NEEDED(CLASS1,CLASS2,MODE) \
 (!TARGET_64BIT && GET_MODE_SIZE (MODE) == 8 && \
   (((CLASS1) == FP_REGS && (CLASS2) != FP_REGS) \
   || ((CLASS2) == FP_REGS && (CLASS1) != FP_REGS)))

/* Define if loading short immediate values into registers sign extends.  */
#define SHORT_IMMEDIATES_SIGN_EXTEND

/* Standard register usage.  */

/* Number of hardware registers.  We have:

   - 32 integer registers
   - 32 floating point registers
   - 32 vector integer registers
   - 32 vector floating point registers
   - 2 fake registers:
	- ARG_POINTER_REGNUM
	- FRAME_POINTER_REGNUM */

#define FIRST_PSEUDO_REGISTER 66

/* x0, sp, gp, and tp are fixed. */

#define FIXED_REGISTERS							\
{ /* General registers.  */                                             \
  1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			\
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			\
  /* Floating-point registers.  */                                      \
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			\
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			\
  /* Others.  */                                                        \
  1, 1 \
}


/* a0-a7, t0-a6, fa0-fa7, and ft0-ft11 are volatile across calls.
   The call RTLs themselves clobber ra.  */

#define CALL_USED_REGISTERS						\
{ /* General registers.  */                                             \
  1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,			\
  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,			\
  /* Floating-point registers.  */                                      \
  1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,			\
  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,			\
  /* Others.  */                                                        \
  1, 1 \
}

#define CALL_REALLY_USED_REGISTERS                                      \
{ /* General registers.  */                                             \
  1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,			\
  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,			\
  /* Floating-point registers.  */                                      \
  1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1,			\
  1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,			\
  /* Others.  */                                                        \
  1, 1 \
}

/* Internal macros to classify an ISA register's type. */

#define GP_REG_FIRST 0
#define GP_REG_LAST  31
#define GP_REG_NUM   (GP_REG_LAST - GP_REG_FIRST + 1)

#define FP_REG_FIRST 32
#define FP_REG_LAST  63
#define FP_REG_NUM   (FP_REG_LAST - FP_REG_FIRST + 1)

/* The DWARF 2 CFA column which tracks the return address from a
   signal handler context.  This means that to maintain backwards
   compatibility, no hard register can be assigned this column if it
   would need to be handled by the DWARF unwinder.  */
#define DWARF_ALT_FRAME_RETURN_COLUMN 64

#define GP_REG_P(REGNO)	\
  ((unsigned int) ((int) (REGNO) - GP_REG_FIRST) < GP_REG_NUM)
#define FP_REG_P(REGNO)  \
  ((unsigned int) ((int) (REGNO) - FP_REG_FIRST) < FP_REG_NUM)

#define FP_REG_RTX_P(X) (REG_P (X) && FP_REG_P (REGNO (X)))

/* Return coprocessor number from register number.  */

#define COPNUM_AS_CHAR_FROM_REGNUM(REGNO) 				\
  (COP0_REG_P (REGNO) ? '0' : COP2_REG_P (REGNO) ? '2'			\
   : COP3_REG_P (REGNO) ? '3' : '?')


#define HARD_REGNO_NREGS(REGNO, MODE) riscv_hard_regno_nregs (REGNO, MODE)

#define HARD_REGNO_MODE_OK(REGNO, MODE)					\
  riscv_hard_regno_mode_ok[ (int)(MODE) ][ (REGNO) ]

#define MODES_TIEABLE_P(MODE1, MODE2)					\
  ((MODE1) == (MODE2) || (GET_MODE_CLASS (MODE1) == MODE_INT		\
			  && GET_MODE_CLASS (MODE2) == MODE_INT))

/* Use s0 as the frame pointer if it is so requested. */
#define HARD_FRAME_POINTER_REGNUM 8
#define STACK_POINTER_REGNUM 2
#define THREAD_POINTER_REGNUM 4

/* These two registers don't really exist: they get eliminated to either
   the stack or hard frame pointer.  */
#define ARG_POINTER_REGNUM 64
#define FRAME_POINTER_REGNUM 65

#define HARD_FRAME_POINTER_IS_FRAME_POINTER 0
#define HARD_FRAME_POINTER_IS_ARG_POINTER 0

/* Register in which static-chain is passed to a function.  */
#define STATIC_CHAIN_REGNUM GP_TEMP_FIRST

/* Registers used as temporaries in prologue/epilogue code.

   The prologue registers mustn't conflict with any
   incoming arguments, the static chain pointer, or the frame pointer.
   The epilogue temporary mustn't conflict with the return registers,
   the frame pointer, the EH stack adjustment, or the EH data registers. */

#define RISCV_PROLOGUE_TEMP_REGNUM (GP_TEMP_FIRST + 1)
#define RISCV_EPILOGUE_TEMP_REGNUM RISCV_PROLOGUE_TEMP_REGNUM

#define RISCV_PROLOGUE_TEMP(MODE) gen_rtx_REG (MODE, RISCV_PROLOGUE_TEMP_REGNUM)
#define RISCV_EPILOGUE_TEMP(MODE) gen_rtx_REG (MODE, RISCV_EPILOGUE_TEMP_REGNUM)

#define FUNCTION_PROFILER(STREAM, LABELNO)	\
{						\
    sorry ("profiler support for RISC-V");	\
}

/* Define this macro if it is as good or better to call a constant
   function address than to call an address kept in a register.  */
#define NO_FUNCTION_CSE 1

/* Define the classes of registers for register constraints in the
   machine description.  Also define ranges of constants.

   One of the classes must always be named ALL_REGS and include all hard regs.
   If there is more than one class, another class must be named NO_REGS
   and contain no registers.

   The name GENERAL_REGS must be the name of a class (or an alias for
   another name such as ALL_REGS).  This is the class of registers
   that is allowed by "g" or "r" in a register constraint.
   Also, registers outside this class are allocated only when
   instructions express preferences for them.

   The classes must be numbered in nondecreasing order; that is,
   a larger-numbered class must never be contained completely
   in a smaller-numbered class.

   For any two classes, it is very desirable that there be another
   class that represents their union.  */

enum reg_class
{
  NO_REGS,			/* no registers in set */
  T_REGS,			/* registers used by indirect sibcalls */
  JALR_REGS,			/* registers used by indirect calls */
  GR_REGS,			/* integer registers */
  FP_REGS,			/* floating point registers */
  FRAME_REGS,			/* $arg and $frame */
  ALL_REGS,			/* all registers */
  LIM_REG_CLASSES		/* max value + 1 */
};

#define N_REG_CLASSES (int) LIM_REG_CLASSES

#define GENERAL_REGS GR_REGS

/* An initializer containing the names of the register classes as C
   string constants.  These names are used in writing some of the
   debugging dumps.  */

#define REG_CLASS_NAMES							\
{									\
  "NO_REGS",								\
  "T_REGS",								\
  "JALR_REGS",								\
  "GR_REGS",								\
  "FP_REGS",								\
  "FRAME_REGS",								\
  "ALL_REGS"								\
}

/* An initializer containing the contents of the register classes,
   as integers which are bit masks.  The Nth integer specifies the
   contents of class N.  The way the integer MASK is interpreted is
   that register R is in the class if `MASK & (1 << R)' is 1.

   When the machine has more than 32 registers, an integer does not
   suffice.  Then the integers are replaced by sub-initializers,
   braced groupings containing several integers.  Each
   sub-initializer must be suitable as an initializer for the type
   `HARD_REG_SET' which is defined in `hard-reg-set.h'.  */

#define REG_CLASS_CONTENTS									\
{												\
  { 0x00000000, 0x00000000, 0x00000000 },	/* NO_REGS */		\
  { 0xf0000040, 0x00000000, 0x00000000 },	/* T_REGS */		\
  { 0xffffff40, 0x00000000, 0x00000000 },	/* JALR_REGS */		\
  { 0xffffffff, 0x00000000, 0x00000000 },	/* GR_REGS */		\
  { 0x00000000, 0xffffffff, 0x00000000 },	/* FP_REGS */		\
  { 0x00000000, 0x00000000, 0x00000003 },	/* FRAME_REGS */	\
  { 0xffffffff, 0xffffffff, 0x00000003 }	/* ALL_REGS */		\
}

/* A C expression whose value is a register class containing hard
   register REGNO.  In general there is more that one such class;
   choose a class which is "minimal", meaning that no smaller class
   also contains the register.  */

#define REGNO_REG_CLASS(REGNO) riscv_regno_to_class[ (REGNO) ]

/* A macro whose definition is the name of the class to which a
   valid base register must belong.  A base register is one used in
   an address which is the register value plus a displacement.  */

#define BASE_REG_CLASS GR_REGS

/* A macro whose definition is the name of the class to which a
   valid index register must belong.  An index register is one used
   in an address where its value is either multiplied by a scale
   factor or added to another register (as well as added to a
   displacement).  */

#define INDEX_REG_CLASS NO_REGS

/* We generally want to put call-clobbered registers ahead of
   call-saved ones.  (IRA expects this.)  */

#define REG_ALLOC_ORDER							\
{ \
  /* Call-clobbered GPRs.  */						\
  15, 14, 13, 12, 11, 10, 16, 17, 5, 6, 7, 28, 29, 30, 31, 1,		\
  /* Call-saved GPRs.  */						\
  8, 9, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,	       			\
  /* GPRs that can never be exposed to the register allocator.  */	\
  0, 2, 3, 4,								\
  /* Call-clobbered FPRs.  */						\
  47, 46, 45, 44, 43, 42, 32, 33, 34, 35, 36, 37, 38, 39, 48, 49,	\
  60, 61, 62, 63,							\
  /* Call-saved FPRs.  */						\
  40, 41, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,			\
  /* None of the remaining classes have defined call-saved		\
     registers.  */							\
  64, 65								\
}

/* True if VALUE is a signed 16-bit number.  */

#define SMALL_OPERAND(VALUE) \
  ((unsigned HOST_WIDE_INT) (VALUE) + IMM_REACH/2 < IMM_REACH)

/* True if VALUE can be loaded into a register using LUI.  */

#define LUI_OPERAND(VALUE)						\
  (((VALUE) | ((1UL<<31) - IMM_REACH)) == ((1UL<<31) - IMM_REACH)	\
   || ((VALUE) | ((1UL<<31) - IMM_REACH)) + IMM_REACH == 0)

/* Return a value X with the low 16 bits clear, and such that
   VALUE - X is a signed 16-bit value.  */

#define SMALL_INT(X) SMALL_OPERAND (INTVAL (X))
#define LUI_INT(X) LUI_OPERAND (INTVAL (X))

/* The HI and LO registers can only be reloaded via the general
   registers.  Condition code registers can only be loaded to the
   general registers, and from the floating point registers.  */

#define SECONDARY_INPUT_RELOAD_CLASS(CLASS, MODE, X)			\
  riscv_secondary_reload_class (CLASS, MODE, X, true)
#define SECONDARY_OUTPUT_RELOAD_CLASS(CLASS, MODE, X)			\
  riscv_secondary_reload_class (CLASS, MODE, X, false)

/* Return the maximum number of consecutive registers
   needed to represent mode MODE in a register of class CLASS.  */

#define CLASS_MAX_NREGS(CLASS, MODE) riscv_class_max_nregs (CLASS, MODE)

/* It is undefined to interpret an FP register in a different format than
   that which it was created to be. */

#define CANNOT_CHANGE_MODE_CLASS(FROM, TO, CLASS) \
  reg_classes_intersect_p (FP_REGS, CLASS)

/* Stack layout; function entry, exit and calling.  */

#define STACK_GROWS_DOWNWARD

#define FRAME_GROWS_DOWNWARD 1

#define STARTING_FRAME_OFFSET 0

#define RETURN_ADDR_RTX riscv_return_addr

#define ELIMINABLE_REGS							\
{{ ARG_POINTER_REGNUM,   STACK_POINTER_REGNUM},				\
 { ARG_POINTER_REGNUM,   HARD_FRAME_POINTER_REGNUM},			\
 { FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM},				\
 { FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM}}				\

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET) \
  (OFFSET) = riscv_initial_elimination_offset (FROM, TO)

/* Allocate stack space for arguments at the beginning of each function.  */
#define ACCUMULATE_OUTGOING_ARGS 1

/* The argument pointer always points to the first argument.  */
#define FIRST_PARM_OFFSET(FNDECL) 0

#define REG_PARM_STACK_SPACE(FNDECL) 0

/* Define this if it is the responsibility of the caller to
   allocate the area reserved for arguments passed in registers.
   If `ACCUMULATE_OUTGOING_ARGS' is also defined, the only effect
   of this macro is to determine whether the space is included in
   `crtl->outgoing_args_size'.  */
#define OUTGOING_REG_PARM_STACK_SPACE(FNTYPE) 1

#define STACK_BOUNDARY 128

/* Symbolic macros for the registers used to return integer and floating
   point values.  */

#define GP_RETURN GP_ARG_FIRST
#define FP_RETURN ((TARGET_SOFT_FLOAT) ? GP_RETURN : FP_ARG_FIRST)

#define MAX_ARGS_IN_REGISTERS 8

/* Symbolic macros for the first/last argument registers.  */

#define GP_ARG_FIRST (GP_REG_FIRST + 10)
#define GP_ARG_LAST  (GP_ARG_FIRST + MAX_ARGS_IN_REGISTERS - 1)
#define GP_TEMP_FIRST (GP_REG_FIRST + 5)
#define FP_ARG_FIRST (FP_REG_FIRST + 10)
#define FP_ARG_LAST  (FP_ARG_FIRST + MAX_ARGS_IN_REGISTERS - 1)

#define CALLEE_SAVED_REG_NUMBER(REGNO)			\
  ((REGNO) >= 8 && (REGNO) <= 9 ? (REGNO) - 8 :		\
   (REGNO) >= 18 && (REGNO) <= 27 ? (REGNO) - 16 : -1)

#define LIBCALL_VALUE(MODE) \
  riscv_function_value (NULL_TREE, NULL_TREE, MODE)

#define FUNCTION_VALUE(VALTYPE, FUNC) \
  riscv_function_value (VALTYPE, FUNC, VOIDmode)

#define FUNCTION_VALUE_REGNO_P(N) ((N) == GP_RETURN || (N) == FP_RETURN)

/* 1 if N is a possible register number for function argument passing.
   We have no FP argument registers when soft-float.  When FP registers
   are 32 bits, we can't directly reference the odd numbered ones.  */

/* Accept arguments in a0-a7 and/or fa0-fa7. */
#define FUNCTION_ARG_REGNO_P(N)					\
  (IN_RANGE((N), GP_ARG_FIRST, GP_ARG_LAST)			\
   || IN_RANGE((N), FP_ARG_FIRST, FP_ARG_LAST))

/* The ABI views the arguments as a structure, of which the first 8
   words go in registers and the rest go on the stack.  If I < 8, N, the Ith
   word might go in the Ith integer argument register or the Ith
   floating-point argument register. */

typedef struct {
  /* Number of integer registers used so far, up to MAX_ARGS_IN_REGISTERS. */
  unsigned int num_gprs;

  /* Number of words passed on the stack.  */
  unsigned int stack_words;
} CUMULATIVE_ARGS;

/* Initialize a variable CUM of type CUMULATIVE_ARGS
   for a call to a function whose data type is FNTYPE.
   For a library call, FNTYPE is 0.  */

#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, INDIRECT, N_NAMED_ARGS) \
  memset (&(CUM), 0, sizeof (CUM))

#define EPILOGUE_USES(REGNO)	((REGNO) == RETURN_ADDR_REGNUM)

/* ABI requires 16-byte alignment, even on ven on RV32. */
#define RISCV_STACK_ALIGN(LOC) (((LOC) + 15) & -16)

#define NO_PROFILE_COUNTERS 1

/* Define this macro if the code for function profiling should come
   before the function prologue.  Normally, the profiling code comes
   after.  */

/* #define PROFILE_BEFORE_PROLOGUE */

/* EXIT_IGNORE_STACK should be nonzero if, when returning from a function,
   the stack pointer does not matter.  The value is tested only in
   functions that have frame pointers.
   No definition is equivalent to always zero.  */

#define EXIT_IGNORE_STACK 1


/* Trampolines are a block of code followed by two pointers.  */

#define TRAMPOLINE_CODE_SIZE 16
#define TRAMPOLINE_SIZE (TRAMPOLINE_CODE_SIZE + POINTER_SIZE * 2)
#define TRAMPOLINE_ALIGNMENT POINTER_SIZE

/* Addressing modes, and classification of registers for them.  */

#define REGNO_OK_FOR_INDEX_P(REGNO) 0
#define REGNO_MODE_OK_FOR_BASE_P(REGNO, MODE) \
  riscv_regno_mode_ok_for_base_p (REGNO, MODE, 1)

/* The macros REG_OK_FOR..._P assume that the arg is a REG rtx
   and check its validity for a certain class.
   We have two alternate definitions for each of them.
   The usual definition accepts all pseudo regs; the other rejects them all.
   The symbol REG_OK_STRICT causes the latter definition to be used.

   Most source files want to accept pseudo regs in the hope that
   they will get allocated to the class that the insn wants them to be in.
   Some source files that are used after register allocation
   need to be strict.  */

#ifndef REG_OK_STRICT
#define REG_MODE_OK_FOR_BASE_P(X, MODE) \
  riscv_regno_mode_ok_for_base_p (REGNO (X), MODE, 0)
#else
#define REG_MODE_OK_FOR_BASE_P(X, MODE) \
  riscv_regno_mode_ok_for_base_p (REGNO (X), MODE, 1)
#endif

#define REG_OK_FOR_INDEX_P(X) 0


/* Maximum number of registers that can appear in a valid memory address.  */

#define MAX_REGS_PER_ADDRESS 1

#define CONSTANT_ADDRESS_P(X) \
  (CONSTANT_P (X) && memory_address_p (SImode, X))

/* This handles the magic '..CURRENT_FUNCTION' symbol, which means
   'the start of the function that this code is output in'.  */

#define ASM_OUTPUT_LABELREF(FILE,NAME)  \
  if (strcmp (NAME, "..CURRENT_FUNCTION") == 0)				\
    asm_fprintf ((FILE), "%U%s",					\
		 XSTR (XEXP (DECL_RTL (current_function_decl), 0), 0));	\
  else									\
    asm_fprintf ((FILE), "%U%s", (NAME))

/* This flag marks functions that cannot be lazily bound.  */
#define SYMBOL_FLAG_BIND_NOW (SYMBOL_FLAG_MACH_DEP << 1)
#define SYMBOL_REF_BIND_NOW_P(RTX) \
  ((SYMBOL_REF_FLAGS (RTX) & SYMBOL_FLAG_BIND_NOW) != 0)

#define JUMP_TABLES_IN_TEXT_SECTION 0
#define CASE_VECTOR_MODE SImode
#define CASE_VECTOR_PC_RELATIVE (riscv_cmodel != CM_MEDLOW)

/* Define this as 1 if `char' should by default be signed; else as 0.  */
#define DEFAULT_SIGNED_CHAR 0

/* Consider using fld/fsd to move 8 bytes at a time for RV32IFD. */
#define MOVE_MAX UNITS_PER_WORD
#define MAX_MOVE_MAX 8

#define SLOW_BYTE_ACCESS 0

#define SHIFT_COUNT_TRUNCATED 1

/* Value is 1 if truncating an integer of INPREC bits to OUTPREC bits
   is done just by pretending it is already truncated.  */
#define TRULY_NOOP_TRUNCATION(OUTPREC, INPREC) \
  (TARGET_64BIT ? ((INPREC) <= 32 || (OUTPREC) < 32) : 1)

/* Specify the machine mode that pointers have.
   After generation of rtl, the compiler makes no further distinction
   between pointers and any other objects of this machine mode.  */

#ifndef Pmode
#define Pmode (TARGET_64BIT ? DImode : SImode)
#endif

/* Give call MEMs SImode since it is the "most permissive" mode
   for both 32-bit and 64-bit targets.  */

#define FUNCTION_MODE SImode

/* A C expression for the cost of a branch instruction.  A value of 2
   seems to minimize code size.  */

#define BRANCH_COST(speed_p, predictable_p) \
  ((!(speed_p) || (predictable_p)) ? 2 : riscv_branch_cost)

#define LOGICAL_OP_NON_SHORT_CIRCUIT 0

/* Control the assembler format that we output.  */

/* Output to assembler file text saying following lines
   may contain character constants, extra white space, comments, etc.  */

#ifndef ASM_APP_ON
#define ASM_APP_ON " #APP\n"
#endif

/* Output to assembler file text saying following lines
   no longer contain unusual constructs.  */

#ifndef ASM_APP_OFF
#define ASM_APP_OFF " #NO_APP\n"
#endif

#define REGISTER_NAMES						\
{ "zero","ra",  "sp",  "gp",  "tp",  "t0",  "t1",  "t2",	\
  "s0",  "s1",  "a0",  "a1",  "a2",  "a3",  "a4",  "a5",	\
  "a6",  "a7",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",	\
  "s8",  "s9",  "s10", "s11", "t3",  "t4",  "t5",  "t6",	\
  "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7",	\
  "fs0", "fs1", "fa0", "fa1", "fa2", "fa3", "fa4", "fa5",	\
  "fa6", "fa7", "fs2", "fs3", "fs4", "fs5", "fs6", "fs7",	\
  "fs8", "fs9", "fs10","fs11","ft8", "ft9", "ft10","ft11",	\
  "arg", "frame", }

#define ADDITIONAL_REGISTER_NAMES					\
{									\
  { "x0",	 0 + GP_REG_FIRST },					\
  { "x1",	 1 + GP_REG_FIRST },					\
  { "x2",	 2 + GP_REG_FIRST },					\
  { "x3",	 3 + GP_REG_FIRST },					\
  { "x4",	 4 + GP_REG_FIRST },					\
  { "x5",	 5 + GP_REG_FIRST },					\
  { "x6",	 6 + GP_REG_FIRST },					\
  { "x7",	 7 + GP_REG_FIRST },					\
  { "x8",	 8 + GP_REG_FIRST },					\
  { "x9",	 9 + GP_REG_FIRST },					\
  { "x10",	10 + GP_REG_FIRST },					\
  { "x11",	11 + GP_REG_FIRST },					\
  { "x12",	12 + GP_REG_FIRST },					\
  { "x13",	13 + GP_REG_FIRST },					\
  { "x14",	14 + GP_REG_FIRST },					\
  { "x15",	15 + GP_REG_FIRST },					\
  { "x16",	16 + GP_REG_FIRST },					\
  { "x17",	17 + GP_REG_FIRST },					\
  { "x18",	18 + GP_REG_FIRST },					\
  { "x19",	19 + GP_REG_FIRST },					\
  { "x20",	20 + GP_REG_FIRST },					\
  { "x21",	21 + GP_REG_FIRST },					\
  { "x22",	22 + GP_REG_FIRST },					\
  { "x23",	23 + GP_REG_FIRST },					\
  { "x24",	24 + GP_REG_FIRST },					\
  { "x25",	25 + GP_REG_FIRST },					\
  { "x26",	26 + GP_REG_FIRST },					\
  { "x27",	27 + GP_REG_FIRST },					\
  { "x28",	28 + GP_REG_FIRST },					\
  { "x29",	29 + GP_REG_FIRST },					\
  { "x30",	30 + GP_REG_FIRST },					\
  { "x31",	31 + GP_REG_FIRST },					\
  { "f0",	 0 + FP_REG_FIRST },					\
  { "f1",	 1 + FP_REG_FIRST },					\
  { "f2",	 2 + FP_REG_FIRST },					\
  { "f3",	 3 + FP_REG_FIRST },					\
  { "f4",	 4 + FP_REG_FIRST },					\
  { "f5",	 5 + FP_REG_FIRST },					\
  { "f6",	 6 + FP_REG_FIRST },					\
  { "f7",	 7 + FP_REG_FIRST },					\
  { "f8",	 8 + FP_REG_FIRST },					\
  { "f9",	 9 + FP_REG_FIRST },					\
  { "f10",	10 + FP_REG_FIRST },					\
  { "f11",	11 + FP_REG_FIRST },					\
  { "f12",	12 + FP_REG_FIRST },					\
  { "f13",	13 + FP_REG_FIRST },					\
  { "f14",	14 + FP_REG_FIRST },					\
  { "f15",	15 + FP_REG_FIRST },					\
  { "f16",	16 + FP_REG_FIRST },					\
  { "f17",	17 + FP_REG_FIRST },					\
  { "f18",	18 + FP_REG_FIRST },					\
  { "f19",	19 + FP_REG_FIRST },					\
  { "f20",	20 + FP_REG_FIRST },					\
  { "f21",	21 + FP_REG_FIRST },					\
  { "f22",	22 + FP_REG_FIRST },					\
  { "f23",	23 + FP_REG_FIRST },					\
  { "f24",	24 + FP_REG_FIRST },					\
  { "f25",	25 + FP_REG_FIRST },					\
  { "f26",	26 + FP_REG_FIRST },					\
  { "f27",	27 + FP_REG_FIRST },					\
  { "f28",	28 + FP_REG_FIRST },					\
  { "f29",	29 + FP_REG_FIRST },					\
  { "f30",	30 + FP_REG_FIRST },					\
  { "f31",	31 + FP_REG_FIRST },					\
}

/* Globalizing directive for a label.  */
#define GLOBAL_ASM_OP "\t.globl\t"

/* This is how to store into the string LABEL
   the symbol_ref name of an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.
   This is suitable for output with `assemble_name'.  */

#undef ASM_GENERATE_INTERNAL_LABEL
#define ASM_GENERATE_INTERNAL_LABEL(LABEL,PREFIX,NUM)			\
  sprintf ((LABEL), "*%s%s%ld", (LOCAL_LABEL_PREFIX), (PREFIX), (long)(NUM))

/* This is how to output an element of a case-vector that is absolute.  */

#define ASM_OUTPUT_ADDR_VEC_ELT(STREAM, VALUE)				\
  fprintf (STREAM, "\t.word\t%sL%d\n", LOCAL_LABEL_PREFIX, VALUE)

/* This is how to output an element of a PIC case-vector. */

#define ASM_OUTPUT_ADDR_DIFF_ELT(STREAM, BODY, VALUE, REL)		\
  fprintf (STREAM, "\t.word\t%sL%d-%sL%d\n",				\
	   LOCAL_LABEL_PREFIX, VALUE, LOCAL_LABEL_PREFIX, REL)

/* This is how to output an assembler line
   that says to advance the location counter
   to a multiple of 2**LOG bytes.  */

#define ASM_OUTPUT_ALIGN(STREAM,LOG)					\
  fprintf (STREAM, "\t.align\t%d\n", (LOG))

/* Define the strings to put out for each section in the object file.  */
#define TEXT_SECTION_ASM_OP	"\t.text"	/* instructions */
#define DATA_SECTION_ASM_OP	"\t.data"	/* large data */
#define READONLY_DATA_SECTION_ASM_OP	"\t.section\t.rodata"
#define BSS_SECTION_ASM_OP	"\t.bss"
#define SBSS_SECTION_ASM_OP	"\t.section\t.sbss,\"aw\",@nobits"
#define SDATA_SECTION_ASM_OP	"\t.section\t.sdata,\"aw\",@progbits"

#define ASM_OUTPUT_REG_PUSH(STREAM,REGNO)				\
do									\
  {									\
    fprintf (STREAM, "\taddi\t%s,%s,-8\n\t%s\t%s,0(%s)\n",		\
	     reg_names[STACK_POINTER_REGNUM],				\
	     reg_names[STACK_POINTER_REGNUM],				\
	     TARGET_64BIT ? "sd" : "sw",				\
	     reg_names[REGNO],						\
	     reg_names[STACK_POINTER_REGNUM]);				\
  }									\
while (0)

#define ASM_OUTPUT_REG_POP(STREAM,REGNO)				\
do									\
  {									\
    fprintf (STREAM, "\t%s\t%s,0(%s)\n\taddi\t%s,%s,8\n",		\
	     TARGET_64BIT ? "ld" : "lw",				\
	     reg_names[REGNO],						\
	     reg_names[STACK_POINTER_REGNUM],				\
	     reg_names[STACK_POINTER_REGNUM],				\
	     reg_names[STACK_POINTER_REGNUM]);				\
  }									\
while (0)

#define ASM_COMMENT_START "#"

#undef SIZE_TYPE
#define SIZE_TYPE (POINTER_SIZE == 64 ? "long unsigned int" : "unsigned int")

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE (POINTER_SIZE == 64 ? "long int" : "int")

/* The maximum number of bytes that can be copied by one iteration of
   a movmemsi loop; see riscv_block_move_loop.  */
#define RISCV_MAX_MOVE_BYTES_PER_LOOP_ITER (UNITS_PER_WORD * 4)

/* The maximum number of bytes that can be copied by a straight-line
   implementation of movmemsi; see riscv_block_move_straight.  We want
   to make sure that any loop-based implementation will iterate at
   least twice.  */
#define RISCV_MAX_MOVE_BYTES_STRAIGHT (RISCV_MAX_MOVE_BYTES_PER_LOOP_ITER * 2)

/* The base cost of a memcpy call, for MOVE_RATIO and friends. */

#define RISCV_CALL_RATIO 6

/* Any loop-based implementation of movmemsi will have at least
   RISCV_MAX_MOVE_BYTES_STRAIGHT / UNITS_PER_WORD memory-to-memory
   moves, so allow individual copies of fewer elements.

   When movmemsi is not available, use a value approximating
   the length of a memcpy call sequence, so that move_by_pieces
   will generate inline code if it is shorter than a function call.
   Since move_by_pieces_ninsns counts memory-to-memory moves, but
   we'll have to generate a load/store pair for each, halve the
   value of RISCV_CALL_RATIO to take that into account.  */

#define MOVE_RATIO(speed)				\
  (HAVE_movmemsi					\
   ? RISCV_MAX_MOVE_BYTES_STRAIGHT / MOVE_MAX		\
   : RISCV_CALL_RATIO / 2)

/* For CLEAR_RATIO, when optimizing for size, give a better estimate
   of the length of a memset call, but use the default otherwise.  */

#define CLEAR_RATIO(speed)\
  ((speed) ? 15 : RISCV_CALL_RATIO)

/* This is similar to CLEAR_RATIO, but for a non-zero constant, so when
   optimizing for size adjust the ratio to account for the overhead of
   loading the constant and replicating it across the word.  */

#define SET_RATIO(speed) \
  ((speed) ? 15 : RISCV_CALL_RATIO - 2)

#ifndef HAVE_AS_TLS
#define HAVE_AS_TLS 0
#endif

#ifndef USED_FOR_TARGET

extern const enum reg_class riscv_regno_to_class[];
extern bool riscv_hard_regno_mode_ok[][FIRST_PSEUDO_REGISTER];
extern const char* riscv_hi_relocs[];
#endif

#define ASM_PREFERRED_EH_DATA_FORMAT(CODE,GLOBAL) \
  (((GLOBAL) ? DW_EH_PE_indirect : 0) | DW_EH_PE_pcrel | DW_EH_PE_sdata4)

/* ISA constants needed for code generation.  */
#define OPCODE_LW    0x2003
#define OPCODE_LD    0x3003
#define OPCODE_AUIPC 0x17
#define OPCODE_JALR  0x67
#define SHIFT_RD  7
#define SHIFT_RS1 15
#define SHIFT_IMM 20
#define IMM_BITS 12

#define IMM_REACH (1LL << IMM_BITS)
#define CONST_HIGH_PART(VALUE) (((VALUE) + (IMM_REACH/2)) & ~(IMM_REACH-1))
#define CONST_LOW_PART(VALUE) ((VALUE) - CONST_HIGH_PART (VALUE))
