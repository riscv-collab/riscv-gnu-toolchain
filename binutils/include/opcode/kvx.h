/* KVX assembler/disassembler support.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

   This file is part of GNU Binutils.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the license, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */


#ifndef OPCODE_KVX_H
#define OPCODE_KVX_H

#define KVXMAXSYLLABLES 3
#define KVXMAXOPERANDS 7
#define KVXMAXBUNDLEISSUE 6
#define KVXMAXBUNDLEWORDS 8
#define KVXNUMCORES 3
#define KVXNUMBUNDLINGS 19


/*
 * The following macros are provided for compatibility with old
 * code.  They should not be used in new code.
 */


/***********************************************/
/*       DATA TYPES                            */
/***********************************************/

/*  Operand definition -- used in building     */
/*  format table                               */

enum kvx_rel {
  /* Absolute relocation. */
  KVX_REL_ABS,
  /* PC relative relocation. */
  KVX_REL_PC,
  /* GP relative relocation. */
  KVX_REL_GP,
  /* TP relative relocation. */
  KVX_REL_TP,
  /* GOT relative relocation. */
  KVX_REL_GOT,
  /* BASE load address relative relocation. */
  KVX_REL_BASE,
};

struct kvx_reloc {
  /* Size in bits. */
  int bitsize;
  /* Type of relative relocation. */
  enum kvx_rel relative;
  /* Number of BFD relocations. */
  int reloc_nb;
  /* List of BFD relocations. */
  unsigned int relocs[];
};

struct kvx_bitfield {
  /* Number of bits.  */
  int size;
  /* Offset in abstract value.  */
  int from_offset;
  /* Offset in encoded value.  */
  int to_offset;
};

struct kvx_operand {
  /* Operand type name.  */
  const char *tname;
  /* Type of operand.  */
  int type;
  /* Width of the operand. */
  int width;
  /* Encoded value shift. */
  int shift;
  /* Encoded value bias.  */
  int bias;
  /* Can be SIGNED|CANEXTEND|BITMASK|WRAPPED.  */
  int flags;
  /* Number of registers.  */
  int reg_nb;
  /* Valid registers for this operand (if no register get null pointer).  */
  int *regs;
  /* Number of relocations.  */
  int reloc_nb;
  /* List of relocations that can be applied to this operand.  */
  struct kvx_reloc **relocs;
  /* Number of given bitfields.  */
  int bitfields;
  /* Bitfields in most to least significant order.  */
  struct kvx_bitfield bfield[];
};

struct kvx_pseudo_relocs
{
  enum
  {
    S32_LO5_UP27,
    S37_LO10_UP27,
    S43_LO10_UP27_EX6,
    S64_LO10_UP27_EX27,
    S16,
    S32,
    S64,
  } reloc_type;

  int bitsize;

  /* Used when pseudo func should expand to different relocations
     based on the 32/64 bits mode.
     Enum values should match the kvx_arch_size var set by -m32
   */
  enum
  {
    PSEUDO_ALL = 0,
    PSEUDO_32_ONLY = 32,
    PSEUDO_64_ONLY = 64,
  } avail_modes;

  /* set to 1 when pseudo func does not take an argument */
  int has_no_arg;

  bfd_reloc_code_real_type reloc_lo5, reloc_lo10, reloc_up27, reloc_ex;
  bfd_reloc_code_real_type single;
  struct kvx_reloc *kreloc;
};

typedef struct symbol symbolS;

struct pseudo_func
{
  const char *name;

  symbolS *sym;
  struct kvx_pseudo_relocs pseudo_relocs;
};

/* some flags for kvx_operand                                 */
/* kvxSIGNED    : is this operand treated as signed ?         */
/* kvxCANEXTEND : can this operand have an extension          */
/* kvxBITMASK   : this operand is a bit mask */
/* kvxWRAPPED   : this operand can accept signed and unsigned integer ranges */


#define kvxSIGNED    1
#define kvxCANEXTEND 2
#define kvxBITMASK   4
#define kvxWRAPPED   8

#define kvxOPCODE_FLAG_UNDEF 0

#define kvxOPCODE_FLAG_IMMX0 1
#define kvxOPCODE_FLAG_IMMX1 2
#define kvxOPCODE_FLAG_BCU 4
#define kvxOPCODE_FLAG_ALU 8
#define kvxOPCODE_FLAG_LSU 16
#define kvxOPCODE_FLAG_MAU 32
#define kvxOPCODE_FLAG_MODE64 64
#define kvxOPCODE_FLAG_MODE32 128
/* Opcode definition.  */

struct kvx_codeword {
  /* The opcode.  */
  unsigned opcode;
  /* Disassembly mask.  */
  unsigned mask;
  /* Target dependent flags.  */
  unsigned flags;
};

struct kvxopc {
  /* asm name */
  const char  *as_op;
  /* 32 bits code words. */
  struct kvx_codeword codewords[KVXMAXSYLLABLES];
  /* Number of words in codewords[].  */
  int wordcount;
  /* coding size in case of variable length.  */
  unsigned coding_size;
  /* Bundling class.  */
  int bundling;
  /* Reservation class.  */
  int reservation;
  /* 0 terminated.  */
  struct kvx_operand *format[KVXMAXOPERANDS + 1];
  /* Resource class.  */
  const char *rclass;
  /* Formating string.  */
  const char *fmtstring;
};

struct kvx_core_info {
  struct kvxopc *optab;
  const char *name;
  const int *resources;
  int elf_core;
  struct pseudo_func *pseudo_funcs;
  int nb_pseudo_funcs;
  int **reservation_table_table;
  int reservation_table_lines;
  int resource_max;
  char **resource_names;
};

struct kvx_Register {
  int id;
  const char *name;
};

extern const int kvx_kv3_v1_reservation_table_lines;
extern const int *kvx_kv3_v1_reservation_table_table[];
extern const char *kvx_kv3_v1_resource_names[];

extern const int kvx_kv3_v1_resources[];
extern struct kvxopc kvx_kv3_v1_optab[];
extern const struct kvx_core_info kvx_kv3_v1_core_info;
extern const int kvx_kv3_v2_reservation_table_lines;
extern const int *kvx_kv3_v2_reservation_table_table[];
extern const char *kvx_kv3_v2_resource_names[];

extern const int kvx_kv3_v2_resources[];
extern struct kvxopc kvx_kv3_v2_optab[];
extern const struct kvx_core_info kvx_kv3_v2_core_info;
extern const int kvx_kv4_v1_reservation_table_lines;
extern const int *kvx_kv4_v1_reservation_table_table[];
extern const char *kvx_kv4_v1_resource_names[];

extern const int kvx_kv4_v1_resources[];
extern struct kvxopc kvx_kv4_v1_optab[];
extern const struct kvx_core_info kvx_kv4_v1_core_info;
extern const struct kvx_core_info *kvx_core_info_table[];
extern const char ***kvx_modifiers_table[];
extern const struct kvx_Register *kvx_registers_table[];
extern const int *kvx_regfiles_table[];

#define KVX_REGFILE_FIRST_GPR 0
#define KVX_REGFILE_LAST_GPR 1
#define KVX_REGFILE_DEC_GPR 2
#define KVX_REGFILE_FIRST_PGR 3
#define KVX_REGFILE_LAST_PGR 4
#define KVX_REGFILE_DEC_PGR 5
#define KVX_REGFILE_FIRST_QGR 6
#define KVX_REGFILE_LAST_QGR 7
#define KVX_REGFILE_DEC_QGR 8
#define KVX_REGFILE_FIRST_SFR 9
#define KVX_REGFILE_LAST_SFR 10
#define KVX_REGFILE_DEC_SFR 11
#define KVX_REGFILE_FIRST_X16R 12
#define KVX_REGFILE_LAST_X16R 13
#define KVX_REGFILE_DEC_X16R 14
#define KVX_REGFILE_FIRST_X2R 15
#define KVX_REGFILE_LAST_X2R 16
#define KVX_REGFILE_DEC_X2R 17
#define KVX_REGFILE_FIRST_X32R 18
#define KVX_REGFILE_LAST_X32R 19
#define KVX_REGFILE_DEC_X32R 20
#define KVX_REGFILE_FIRST_X4R 21
#define KVX_REGFILE_LAST_X4R 22
#define KVX_REGFILE_DEC_X4R 23
#define KVX_REGFILE_FIRST_X64R 24
#define KVX_REGFILE_LAST_X64R 25
#define KVX_REGFILE_DEC_X64R 26
#define KVX_REGFILE_FIRST_X8R 27
#define KVX_REGFILE_LAST_X8R 28
#define KVX_REGFILE_DEC_X8R 29
#define KVX_REGFILE_FIRST_XBR 30
#define KVX_REGFILE_LAST_XBR 31
#define KVX_REGFILE_DEC_XBR 32
#define KVX_REGFILE_FIRST_XCR 33
#define KVX_REGFILE_LAST_XCR 34
#define KVX_REGFILE_DEC_XCR 35
#define KVX_REGFILE_FIRST_XMR 36
#define KVX_REGFILE_LAST_XMR 37
#define KVX_REGFILE_DEC_XMR 38
#define KVX_REGFILE_FIRST_XTR 39
#define KVX_REGFILE_LAST_XTR 40
#define KVX_REGFILE_DEC_XTR 41
#define KVX_REGFILE_FIRST_XVR 42
#define KVX_REGFILE_LAST_XVR 43
#define KVX_REGFILE_DEC_XVR 44
#define KVX_REGFILE_REGISTERS 45
#define KVX_REGFILE_DEC_REGISTERS 46


extern int kvx_kv3_v1_regfiles[];
extern const char **kvx_kv3_v1_modifiers[];
extern struct kvx_Register kvx_kv3_v1_registers[];

extern int kvx_kv3_v1_dec_registers[];

enum Method_kvx_kv3_v1_enum {
  Immediate_kv3_v1_pcrel17 = 1,
  Immediate_kv3_v1_pcrel27 = 2,
  Immediate_kv3_v1_signed10 = 3,
  Immediate_kv3_v1_signed16 = 4,
  Immediate_kv3_v1_signed27 = 5,
  Immediate_kv3_v1_signed37 = 6,
  Immediate_kv3_v1_signed43 = 7,
  Immediate_kv3_v1_signed54 = 8,
  Immediate_kv3_v1_sysnumber = 9,
  Immediate_kv3_v1_unsigned6 = 10,
  Immediate_kv3_v1_wrapped32 = 11,
  Immediate_kv3_v1_wrapped64 = 12,
  Modifier_kv3_v1_column = 13,
  Modifier_kv3_v1_comparison = 14,
  Modifier_kv3_v1_doscale = 15,
  Modifier_kv3_v1_exunum = 16,
  Modifier_kv3_v1_floatcomp = 17,
  Modifier_kv3_v1_qindex = 18,
  Modifier_kv3_v1_rectify = 19,
  Modifier_kv3_v1_rounding = 20,
  Modifier_kv3_v1_roundint = 21,
  Modifier_kv3_v1_saturate = 22,
  Modifier_kv3_v1_scalarcond = 23,
  Modifier_kv3_v1_silent = 24,
  Modifier_kv3_v1_simplecond = 25,
  Modifier_kv3_v1_speculate = 26,
  Modifier_kv3_v1_splat32 = 27,
  Modifier_kv3_v1_variant = 28,
  RegClass_kv3_v1_aloneReg = 29,
  RegClass_kv3_v1_blockReg = 30,
  RegClass_kv3_v1_blockReg0M4 = 31,
  RegClass_kv3_v1_blockReg1M4 = 32,
  RegClass_kv3_v1_blockReg2M4 = 33,
  RegClass_kv3_v1_blockReg3M4 = 34,
  RegClass_kv3_v1_blockRegE = 35,
  RegClass_kv3_v1_blockRegO = 36,
  RegClass_kv3_v1_blockReg_0 = 37,
  RegClass_kv3_v1_blockReg_1 = 38,
  RegClass_kv3_v1_buffer16Reg = 39,
  RegClass_kv3_v1_buffer2Reg = 40,
  RegClass_kv3_v1_buffer32Reg = 41,
  RegClass_kv3_v1_buffer4Reg = 42,
  RegClass_kv3_v1_buffer64Reg = 43,
  RegClass_kv3_v1_buffer8Reg = 44,
  RegClass_kv3_v1_coproReg = 45,
  RegClass_kv3_v1_coproReg0M4 = 46,
  RegClass_kv3_v1_coproReg1M4 = 47,
  RegClass_kv3_v1_coproReg2M4 = 48,
  RegClass_kv3_v1_coproReg3M4 = 49,
  RegClass_kv3_v1_matrixReg = 50,
  RegClass_kv3_v1_matrixReg_0 = 51,
  RegClass_kv3_v1_matrixReg_1 = 52,
  RegClass_kv3_v1_matrixReg_2 = 53,
  RegClass_kv3_v1_matrixReg_3 = 54,
  RegClass_kv3_v1_onlyfxReg = 55,
  RegClass_kv3_v1_onlygetReg = 56,
  RegClass_kv3_v1_onlyraReg = 57,
  RegClass_kv3_v1_onlysetReg = 58,
  RegClass_kv3_v1_onlyswapReg = 59,
  RegClass_kv3_v1_pairedReg = 60,
  RegClass_kv3_v1_pairedReg_0 = 61,
  RegClass_kv3_v1_pairedReg_1 = 62,
  RegClass_kv3_v1_quadReg = 63,
  RegClass_kv3_v1_quadReg_0 = 64,
  RegClass_kv3_v1_quadReg_1 = 65,
  RegClass_kv3_v1_quadReg_2 = 66,
  RegClass_kv3_v1_quadReg_3 = 67,
  RegClass_kv3_v1_singleReg = 68,
  RegClass_kv3_v1_systemReg = 69,
  RegClass_kv3_v1_tileReg = 70,
  RegClass_kv3_v1_tileReg_0 = 71,
  RegClass_kv3_v1_tileReg_1 = 72,
  RegClass_kv3_v1_vectorReg = 73,
  RegClass_kv3_v1_vectorRegE = 74,
  RegClass_kv3_v1_vectorRegO = 75,
  RegClass_kv3_v1_vectorReg_0 = 76,
  RegClass_kv3_v1_vectorReg_1 = 77,
  RegClass_kv3_v1_vectorReg_2 = 78,
  RegClass_kv3_v1_vectorReg_3 = 79,
  Instruction_kv3_v1_abdd = 80,
  Instruction_kv3_v1_abdd_abase = 81,
  Instruction_kv3_v1_abdhq = 82,
  Instruction_kv3_v1_abdw = 83,
  Instruction_kv3_v1_abdwp = 84,
  Instruction_kv3_v1_absd = 85,
  Instruction_kv3_v1_abshq = 86,
  Instruction_kv3_v1_absw = 87,
  Instruction_kv3_v1_abswp = 88,
  Instruction_kv3_v1_acswapd = 89,
  Instruction_kv3_v1_acswapw = 90,
  Instruction_kv3_v1_addcd = 91,
  Instruction_kv3_v1_addcd_i = 92,
  Instruction_kv3_v1_addd = 93,
  Instruction_kv3_v1_addd_abase = 94,
  Instruction_kv3_v1_addhcp_c = 95,
  Instruction_kv3_v1_addhq = 96,
  Instruction_kv3_v1_addsd = 97,
  Instruction_kv3_v1_addshq = 98,
  Instruction_kv3_v1_addsw = 99,
  Instruction_kv3_v1_addswp = 100,
  Instruction_kv3_v1_adduwd = 101,
  Instruction_kv3_v1_addw = 102,
  Instruction_kv3_v1_addwc_c = 103,
  Instruction_kv3_v1_addwd = 104,
  Instruction_kv3_v1_addwp = 105,
  Instruction_kv3_v1_addx16d = 106,
  Instruction_kv3_v1_addx16hq = 107,
  Instruction_kv3_v1_addx16uwd = 108,
  Instruction_kv3_v1_addx16w = 109,
  Instruction_kv3_v1_addx16wd = 110,
  Instruction_kv3_v1_addx16wp = 111,
  Instruction_kv3_v1_addx2d = 112,
  Instruction_kv3_v1_addx2hq = 113,
  Instruction_kv3_v1_addx2uwd = 114,
  Instruction_kv3_v1_addx2w = 115,
  Instruction_kv3_v1_addx2wd = 116,
  Instruction_kv3_v1_addx2wp = 117,
  Instruction_kv3_v1_addx4d = 118,
  Instruction_kv3_v1_addx4hq = 119,
  Instruction_kv3_v1_addx4uwd = 120,
  Instruction_kv3_v1_addx4w = 121,
  Instruction_kv3_v1_addx4wd = 122,
  Instruction_kv3_v1_addx4wp = 123,
  Instruction_kv3_v1_addx8d = 124,
  Instruction_kv3_v1_addx8hq = 125,
  Instruction_kv3_v1_addx8uwd = 126,
  Instruction_kv3_v1_addx8w = 127,
  Instruction_kv3_v1_addx8wd = 128,
  Instruction_kv3_v1_addx8wp = 129,
  Instruction_kv3_v1_aladdd = 130,
  Instruction_kv3_v1_aladdw = 131,
  Instruction_kv3_v1_alclrd = 132,
  Instruction_kv3_v1_alclrw = 133,
  Instruction_kv3_v1_aligno = 134,
  Instruction_kv3_v1_alignv = 135,
  Instruction_kv3_v1_andd = 136,
  Instruction_kv3_v1_andd_abase = 137,
  Instruction_kv3_v1_andnd = 138,
  Instruction_kv3_v1_andnd_abase = 139,
  Instruction_kv3_v1_andnw = 140,
  Instruction_kv3_v1_andw = 141,
  Instruction_kv3_v1_avghq = 142,
  Instruction_kv3_v1_avgrhq = 143,
  Instruction_kv3_v1_avgruhq = 144,
  Instruction_kv3_v1_avgruw = 145,
  Instruction_kv3_v1_avgruwp = 146,
  Instruction_kv3_v1_avgrw = 147,
  Instruction_kv3_v1_avgrwp = 148,
  Instruction_kv3_v1_avguhq = 149,
  Instruction_kv3_v1_avguw = 150,
  Instruction_kv3_v1_avguwp = 151,
  Instruction_kv3_v1_avgw = 152,
  Instruction_kv3_v1_avgwp = 153,
  Instruction_kv3_v1_await = 154,
  Instruction_kv3_v1_barrier = 155,
  Instruction_kv3_v1_call = 156,
  Instruction_kv3_v1_cb = 157,
  Instruction_kv3_v1_cbsd = 158,
  Instruction_kv3_v1_cbsw = 159,
  Instruction_kv3_v1_cbswp = 160,
  Instruction_kv3_v1_clrf = 161,
  Instruction_kv3_v1_clsd = 162,
  Instruction_kv3_v1_clsw = 163,
  Instruction_kv3_v1_clswp = 164,
  Instruction_kv3_v1_clzd = 165,
  Instruction_kv3_v1_clzw = 166,
  Instruction_kv3_v1_clzwp = 167,
  Instruction_kv3_v1_cmoved = 168,
  Instruction_kv3_v1_cmovehq = 169,
  Instruction_kv3_v1_cmovewp = 170,
  Instruction_kv3_v1_cmuldt = 171,
  Instruction_kv3_v1_cmulghxdt = 172,
  Instruction_kv3_v1_cmulglxdt = 173,
  Instruction_kv3_v1_cmulgmxdt = 174,
  Instruction_kv3_v1_cmulxdt = 175,
  Instruction_kv3_v1_compd = 176,
  Instruction_kv3_v1_compnhq = 177,
  Instruction_kv3_v1_compnwp = 178,
  Instruction_kv3_v1_compuwd = 179,
  Instruction_kv3_v1_compw = 180,
  Instruction_kv3_v1_compwd = 181,
  Instruction_kv3_v1_convdhv0 = 182,
  Instruction_kv3_v1_convdhv1 = 183,
  Instruction_kv3_v1_convwbv0 = 184,
  Instruction_kv3_v1_convwbv1 = 185,
  Instruction_kv3_v1_convwbv2 = 186,
  Instruction_kv3_v1_convwbv3 = 187,
  Instruction_kv3_v1_copyd = 188,
  Instruction_kv3_v1_copyo = 189,
  Instruction_kv3_v1_copyq = 190,
  Instruction_kv3_v1_copyw = 191,
  Instruction_kv3_v1_crcbellw = 192,
  Instruction_kv3_v1_crcbelmw = 193,
  Instruction_kv3_v1_crclellw = 194,
  Instruction_kv3_v1_crclelmw = 195,
  Instruction_kv3_v1_ctzd = 196,
  Instruction_kv3_v1_ctzw = 197,
  Instruction_kv3_v1_ctzwp = 198,
  Instruction_kv3_v1_d1inval = 199,
  Instruction_kv3_v1_dinvall = 200,
  Instruction_kv3_v1_dot2suwd = 201,
  Instruction_kv3_v1_dot2suwdp = 202,
  Instruction_kv3_v1_dot2uwd = 203,
  Instruction_kv3_v1_dot2uwdp = 204,
  Instruction_kv3_v1_dot2w = 205,
  Instruction_kv3_v1_dot2wd = 206,
  Instruction_kv3_v1_dot2wdp = 207,
  Instruction_kv3_v1_dot2wzp = 208,
  Instruction_kv3_v1_dtouchl = 209,
  Instruction_kv3_v1_dzerol = 210,
  Instruction_kv3_v1_errop = 211,
  Instruction_kv3_v1_extfs = 212,
  Instruction_kv3_v1_extfz = 213,
  Instruction_kv3_v1_fabsd = 214,
  Instruction_kv3_v1_fabshq = 215,
  Instruction_kv3_v1_fabsw = 216,
  Instruction_kv3_v1_fabswp = 217,
  Instruction_kv3_v1_faddd = 218,
  Instruction_kv3_v1_fadddc = 219,
  Instruction_kv3_v1_fadddc_c = 220,
  Instruction_kv3_v1_fadddp = 221,
  Instruction_kv3_v1_faddhq = 222,
  Instruction_kv3_v1_faddw = 223,
  Instruction_kv3_v1_faddwc = 224,
  Instruction_kv3_v1_faddwc_c = 225,
  Instruction_kv3_v1_faddwcp = 226,
  Instruction_kv3_v1_faddwcp_c = 227,
  Instruction_kv3_v1_faddwp = 228,
  Instruction_kv3_v1_faddwq = 229,
  Instruction_kv3_v1_fcdivd = 230,
  Instruction_kv3_v1_fcdivw = 231,
  Instruction_kv3_v1_fcdivwp = 232,
  Instruction_kv3_v1_fcompd = 233,
  Instruction_kv3_v1_fcompnhq = 234,
  Instruction_kv3_v1_fcompnwp = 235,
  Instruction_kv3_v1_fcompw = 236,
  Instruction_kv3_v1_fdot2w = 237,
  Instruction_kv3_v1_fdot2wd = 238,
  Instruction_kv3_v1_fdot2wdp = 239,
  Instruction_kv3_v1_fdot2wzp = 240,
  Instruction_kv3_v1_fence = 241,
  Instruction_kv3_v1_ffmad = 242,
  Instruction_kv3_v1_ffmahq = 243,
  Instruction_kv3_v1_ffmahw = 244,
  Instruction_kv3_v1_ffmahwq = 245,
  Instruction_kv3_v1_ffmaw = 246,
  Instruction_kv3_v1_ffmawd = 247,
  Instruction_kv3_v1_ffmawdp = 248,
  Instruction_kv3_v1_ffmawp = 249,
  Instruction_kv3_v1_ffmsd = 250,
  Instruction_kv3_v1_ffmshq = 251,
  Instruction_kv3_v1_ffmshw = 252,
  Instruction_kv3_v1_ffmshwq = 253,
  Instruction_kv3_v1_ffmsw = 254,
  Instruction_kv3_v1_ffmswd = 255,
  Instruction_kv3_v1_ffmswdp = 256,
  Instruction_kv3_v1_ffmswp = 257,
  Instruction_kv3_v1_fixedd = 258,
  Instruction_kv3_v1_fixedud = 259,
  Instruction_kv3_v1_fixeduw = 260,
  Instruction_kv3_v1_fixeduwp = 261,
  Instruction_kv3_v1_fixedw = 262,
  Instruction_kv3_v1_fixedwp = 263,
  Instruction_kv3_v1_floatd = 264,
  Instruction_kv3_v1_floatud = 265,
  Instruction_kv3_v1_floatuw = 266,
  Instruction_kv3_v1_floatuwp = 267,
  Instruction_kv3_v1_floatw = 268,
  Instruction_kv3_v1_floatwp = 269,
  Instruction_kv3_v1_fmaxd = 270,
  Instruction_kv3_v1_fmaxhq = 271,
  Instruction_kv3_v1_fmaxw = 272,
  Instruction_kv3_v1_fmaxwp = 273,
  Instruction_kv3_v1_fmind = 274,
  Instruction_kv3_v1_fminhq = 275,
  Instruction_kv3_v1_fminw = 276,
  Instruction_kv3_v1_fminwp = 277,
  Instruction_kv3_v1_fmm212w = 278,
  Instruction_kv3_v1_fmma212w = 279,
  Instruction_kv3_v1_fmma242hw0 = 280,
  Instruction_kv3_v1_fmma242hw1 = 281,
  Instruction_kv3_v1_fmma242hw2 = 282,
  Instruction_kv3_v1_fmma242hw3 = 283,
  Instruction_kv3_v1_fmms212w = 284,
  Instruction_kv3_v1_fmuld = 285,
  Instruction_kv3_v1_fmulhq = 286,
  Instruction_kv3_v1_fmulhw = 287,
  Instruction_kv3_v1_fmulhwq = 288,
  Instruction_kv3_v1_fmulw = 289,
  Instruction_kv3_v1_fmulwc = 290,
  Instruction_kv3_v1_fmulwc_c = 291,
  Instruction_kv3_v1_fmulwd = 292,
  Instruction_kv3_v1_fmulwdc = 293,
  Instruction_kv3_v1_fmulwdc_c = 294,
  Instruction_kv3_v1_fmulwdp = 295,
  Instruction_kv3_v1_fmulwp = 296,
  Instruction_kv3_v1_fmulwq = 297,
  Instruction_kv3_v1_fnarrow44wh = 298,
  Instruction_kv3_v1_fnarrowdw = 299,
  Instruction_kv3_v1_fnarrowdwp = 300,
  Instruction_kv3_v1_fnarrowwh = 301,
  Instruction_kv3_v1_fnarrowwhq = 302,
  Instruction_kv3_v1_fnegd = 303,
  Instruction_kv3_v1_fneghq = 304,
  Instruction_kv3_v1_fnegw = 305,
  Instruction_kv3_v1_fnegwp = 306,
  Instruction_kv3_v1_frecw = 307,
  Instruction_kv3_v1_frsrw = 308,
  Instruction_kv3_v1_fsbfd = 309,
  Instruction_kv3_v1_fsbfdc = 310,
  Instruction_kv3_v1_fsbfdc_c = 311,
  Instruction_kv3_v1_fsbfdp = 312,
  Instruction_kv3_v1_fsbfhq = 313,
  Instruction_kv3_v1_fsbfw = 314,
  Instruction_kv3_v1_fsbfwc = 315,
  Instruction_kv3_v1_fsbfwc_c = 316,
  Instruction_kv3_v1_fsbfwcp = 317,
  Instruction_kv3_v1_fsbfwcp_c = 318,
  Instruction_kv3_v1_fsbfwp = 319,
  Instruction_kv3_v1_fsbfwq = 320,
  Instruction_kv3_v1_fscalewv = 321,
  Instruction_kv3_v1_fsdivd = 322,
  Instruction_kv3_v1_fsdivw = 323,
  Instruction_kv3_v1_fsdivwp = 324,
  Instruction_kv3_v1_fsrecd = 325,
  Instruction_kv3_v1_fsrecw = 326,
  Instruction_kv3_v1_fsrecwp = 327,
  Instruction_kv3_v1_fsrsrd = 328,
  Instruction_kv3_v1_fsrsrw = 329,
  Instruction_kv3_v1_fsrsrwp = 330,
  Instruction_kv3_v1_fwidenlhw = 331,
  Instruction_kv3_v1_fwidenlhwp = 332,
  Instruction_kv3_v1_fwidenlwd = 333,
  Instruction_kv3_v1_fwidenmhw = 334,
  Instruction_kv3_v1_fwidenmhwp = 335,
  Instruction_kv3_v1_fwidenmwd = 336,
  Instruction_kv3_v1_get = 337,
  Instruction_kv3_v1_goto = 338,
  Instruction_kv3_v1_i1inval = 339,
  Instruction_kv3_v1_i1invals = 340,
  Instruction_kv3_v1_icall = 341,
  Instruction_kv3_v1_iget = 342,
  Instruction_kv3_v1_igoto = 343,
  Instruction_kv3_v1_insf = 344,
  Instruction_kv3_v1_landd = 345,
  Instruction_kv3_v1_landhq = 346,
  Instruction_kv3_v1_landw = 347,
  Instruction_kv3_v1_landwp = 348,
  Instruction_kv3_v1_lbs = 349,
  Instruction_kv3_v1_lbz = 350,
  Instruction_kv3_v1_ld = 351,
  Instruction_kv3_v1_lhs = 352,
  Instruction_kv3_v1_lhz = 353,
  Instruction_kv3_v1_lnandd = 354,
  Instruction_kv3_v1_lnandhq = 355,
  Instruction_kv3_v1_lnandw = 356,
  Instruction_kv3_v1_lnandwp = 357,
  Instruction_kv3_v1_lnord = 358,
  Instruction_kv3_v1_lnorhq = 359,
  Instruction_kv3_v1_lnorw = 360,
  Instruction_kv3_v1_lnorwp = 361,
  Instruction_kv3_v1_lo = 362,
  Instruction_kv3_v1_loopdo = 363,
  Instruction_kv3_v1_lord = 364,
  Instruction_kv3_v1_lorhq = 365,
  Instruction_kv3_v1_lorw = 366,
  Instruction_kv3_v1_lorwp = 367,
  Instruction_kv3_v1_lq = 368,
  Instruction_kv3_v1_lws = 369,
  Instruction_kv3_v1_lwz = 370,
  Instruction_kv3_v1_maddd = 371,
  Instruction_kv3_v1_madddt = 372,
  Instruction_kv3_v1_maddhq = 373,
  Instruction_kv3_v1_maddhwq = 374,
  Instruction_kv3_v1_maddsudt = 375,
  Instruction_kv3_v1_maddsuhwq = 376,
  Instruction_kv3_v1_maddsuwd = 377,
  Instruction_kv3_v1_maddsuwdp = 378,
  Instruction_kv3_v1_maddudt = 379,
  Instruction_kv3_v1_madduhwq = 380,
  Instruction_kv3_v1_madduwd = 381,
  Instruction_kv3_v1_madduwdp = 382,
  Instruction_kv3_v1_madduzdt = 383,
  Instruction_kv3_v1_maddw = 384,
  Instruction_kv3_v1_maddwd = 385,
  Instruction_kv3_v1_maddwdp = 386,
  Instruction_kv3_v1_maddwp = 387,
  Instruction_kv3_v1_make = 388,
  Instruction_kv3_v1_maxd = 389,
  Instruction_kv3_v1_maxd_abase = 390,
  Instruction_kv3_v1_maxhq = 391,
  Instruction_kv3_v1_maxud = 392,
  Instruction_kv3_v1_maxud_abase = 393,
  Instruction_kv3_v1_maxuhq = 394,
  Instruction_kv3_v1_maxuw = 395,
  Instruction_kv3_v1_maxuwp = 396,
  Instruction_kv3_v1_maxw = 397,
  Instruction_kv3_v1_maxwp = 398,
  Instruction_kv3_v1_mind = 399,
  Instruction_kv3_v1_mind_abase = 400,
  Instruction_kv3_v1_minhq = 401,
  Instruction_kv3_v1_minud = 402,
  Instruction_kv3_v1_minud_abase = 403,
  Instruction_kv3_v1_minuhq = 404,
  Instruction_kv3_v1_minuw = 405,
  Instruction_kv3_v1_minuwp = 406,
  Instruction_kv3_v1_minw = 407,
  Instruction_kv3_v1_minwp = 408,
  Instruction_kv3_v1_mm212w = 409,
  Instruction_kv3_v1_mma212w = 410,
  Instruction_kv3_v1_mma444hbd0 = 411,
  Instruction_kv3_v1_mma444hbd1 = 412,
  Instruction_kv3_v1_mma444hd = 413,
  Instruction_kv3_v1_mma444suhbd0 = 414,
  Instruction_kv3_v1_mma444suhbd1 = 415,
  Instruction_kv3_v1_mma444suhd = 416,
  Instruction_kv3_v1_mma444uhbd0 = 417,
  Instruction_kv3_v1_mma444uhbd1 = 418,
  Instruction_kv3_v1_mma444uhd = 419,
  Instruction_kv3_v1_mma444ushbd0 = 420,
  Instruction_kv3_v1_mma444ushbd1 = 421,
  Instruction_kv3_v1_mma444ushd = 422,
  Instruction_kv3_v1_mms212w = 423,
  Instruction_kv3_v1_movetq = 424,
  Instruction_kv3_v1_msbfd = 425,
  Instruction_kv3_v1_msbfdt = 426,
  Instruction_kv3_v1_msbfhq = 427,
  Instruction_kv3_v1_msbfhwq = 428,
  Instruction_kv3_v1_msbfsudt = 429,
  Instruction_kv3_v1_msbfsuhwq = 430,
  Instruction_kv3_v1_msbfsuwd = 431,
  Instruction_kv3_v1_msbfsuwdp = 432,
  Instruction_kv3_v1_msbfudt = 433,
  Instruction_kv3_v1_msbfuhwq = 434,
  Instruction_kv3_v1_msbfuwd = 435,
  Instruction_kv3_v1_msbfuwdp = 436,
  Instruction_kv3_v1_msbfuzdt = 437,
  Instruction_kv3_v1_msbfw = 438,
  Instruction_kv3_v1_msbfwd = 439,
  Instruction_kv3_v1_msbfwdp = 440,
  Instruction_kv3_v1_msbfwp = 441,
  Instruction_kv3_v1_muld = 442,
  Instruction_kv3_v1_muldt = 443,
  Instruction_kv3_v1_mulhq = 444,
  Instruction_kv3_v1_mulhwq = 445,
  Instruction_kv3_v1_mulsudt = 446,
  Instruction_kv3_v1_mulsuhwq = 447,
  Instruction_kv3_v1_mulsuwd = 448,
  Instruction_kv3_v1_mulsuwdp = 449,
  Instruction_kv3_v1_muludt = 450,
  Instruction_kv3_v1_muluhwq = 451,
  Instruction_kv3_v1_muluwd = 452,
  Instruction_kv3_v1_muluwdp = 453,
  Instruction_kv3_v1_mulw = 454,
  Instruction_kv3_v1_mulwc = 455,
  Instruction_kv3_v1_mulwc_c = 456,
  Instruction_kv3_v1_mulwd = 457,
  Instruction_kv3_v1_mulwdc = 458,
  Instruction_kv3_v1_mulwdc_c = 459,
  Instruction_kv3_v1_mulwdp = 460,
  Instruction_kv3_v1_mulwp = 461,
  Instruction_kv3_v1_mulwq = 462,
  Instruction_kv3_v1_nandd = 463,
  Instruction_kv3_v1_nandd_abase = 464,
  Instruction_kv3_v1_nandw = 465,
  Instruction_kv3_v1_negd = 466,
  Instruction_kv3_v1_neghq = 467,
  Instruction_kv3_v1_negw = 468,
  Instruction_kv3_v1_negwp = 469,
  Instruction_kv3_v1_nop = 470,
  Instruction_kv3_v1_nord = 471,
  Instruction_kv3_v1_nord_abase = 472,
  Instruction_kv3_v1_norw = 473,
  Instruction_kv3_v1_notd = 474,
  Instruction_kv3_v1_notw = 475,
  Instruction_kv3_v1_nxord = 476,
  Instruction_kv3_v1_nxord_abase = 477,
  Instruction_kv3_v1_nxorw = 478,
  Instruction_kv3_v1_ord = 479,
  Instruction_kv3_v1_ord_abase = 480,
  Instruction_kv3_v1_ornd = 481,
  Instruction_kv3_v1_ornd_abase = 482,
  Instruction_kv3_v1_ornw = 483,
  Instruction_kv3_v1_orw = 484,
  Instruction_kv3_v1_pcrel = 485,
  Instruction_kv3_v1_ret = 486,
  Instruction_kv3_v1_rfe = 487,
  Instruction_kv3_v1_rolw = 488,
  Instruction_kv3_v1_rolwps = 489,
  Instruction_kv3_v1_rorw = 490,
  Instruction_kv3_v1_rorwps = 491,
  Instruction_kv3_v1_rswap = 492,
  Instruction_kv3_v1_satd = 493,
  Instruction_kv3_v1_satdh = 494,
  Instruction_kv3_v1_satdw = 495,
  Instruction_kv3_v1_sb = 496,
  Instruction_kv3_v1_sbfcd = 497,
  Instruction_kv3_v1_sbfcd_i = 498,
  Instruction_kv3_v1_sbfd = 499,
  Instruction_kv3_v1_sbfd_abase = 500,
  Instruction_kv3_v1_sbfhcp_c = 501,
  Instruction_kv3_v1_sbfhq = 502,
  Instruction_kv3_v1_sbfsd = 503,
  Instruction_kv3_v1_sbfshq = 504,
  Instruction_kv3_v1_sbfsw = 505,
  Instruction_kv3_v1_sbfswp = 506,
  Instruction_kv3_v1_sbfuwd = 507,
  Instruction_kv3_v1_sbfw = 508,
  Instruction_kv3_v1_sbfwc_c = 509,
  Instruction_kv3_v1_sbfwd = 510,
  Instruction_kv3_v1_sbfwp = 511,
  Instruction_kv3_v1_sbfx16d = 512,
  Instruction_kv3_v1_sbfx16hq = 513,
  Instruction_kv3_v1_sbfx16uwd = 514,
  Instruction_kv3_v1_sbfx16w = 515,
  Instruction_kv3_v1_sbfx16wd = 516,
  Instruction_kv3_v1_sbfx16wp = 517,
  Instruction_kv3_v1_sbfx2d = 518,
  Instruction_kv3_v1_sbfx2hq = 519,
  Instruction_kv3_v1_sbfx2uwd = 520,
  Instruction_kv3_v1_sbfx2w = 521,
  Instruction_kv3_v1_sbfx2wd = 522,
  Instruction_kv3_v1_sbfx2wp = 523,
  Instruction_kv3_v1_sbfx4d = 524,
  Instruction_kv3_v1_sbfx4hq = 525,
  Instruction_kv3_v1_sbfx4uwd = 526,
  Instruction_kv3_v1_sbfx4w = 527,
  Instruction_kv3_v1_sbfx4wd = 528,
  Instruction_kv3_v1_sbfx4wp = 529,
  Instruction_kv3_v1_sbfx8d = 530,
  Instruction_kv3_v1_sbfx8hq = 531,
  Instruction_kv3_v1_sbfx8uwd = 532,
  Instruction_kv3_v1_sbfx8w = 533,
  Instruction_kv3_v1_sbfx8wd = 534,
  Instruction_kv3_v1_sbfx8wp = 535,
  Instruction_kv3_v1_sbmm8 = 536,
  Instruction_kv3_v1_sbmm8_abase = 537,
  Instruction_kv3_v1_sbmmt8 = 538,
  Instruction_kv3_v1_sbmmt8_abase = 539,
  Instruction_kv3_v1_scall = 540,
  Instruction_kv3_v1_sd = 541,
  Instruction_kv3_v1_set = 542,
  Instruction_kv3_v1_sh = 543,
  Instruction_kv3_v1_sleep = 544,
  Instruction_kv3_v1_slld = 545,
  Instruction_kv3_v1_sllhqs = 546,
  Instruction_kv3_v1_sllw = 547,
  Instruction_kv3_v1_sllwps = 548,
  Instruction_kv3_v1_slsd = 549,
  Instruction_kv3_v1_slshqs = 550,
  Instruction_kv3_v1_slsw = 551,
  Instruction_kv3_v1_slswps = 552,
  Instruction_kv3_v1_so = 553,
  Instruction_kv3_v1_sq = 554,
  Instruction_kv3_v1_srad = 555,
  Instruction_kv3_v1_srahqs = 556,
  Instruction_kv3_v1_sraw = 557,
  Instruction_kv3_v1_srawps = 558,
  Instruction_kv3_v1_srld = 559,
  Instruction_kv3_v1_srlhqs = 560,
  Instruction_kv3_v1_srlw = 561,
  Instruction_kv3_v1_srlwps = 562,
  Instruction_kv3_v1_srsd = 563,
  Instruction_kv3_v1_srshqs = 564,
  Instruction_kv3_v1_srsw = 565,
  Instruction_kv3_v1_srswps = 566,
  Instruction_kv3_v1_stop = 567,
  Instruction_kv3_v1_stsud = 568,
  Instruction_kv3_v1_stsuw = 569,
  Instruction_kv3_v1_sw = 570,
  Instruction_kv3_v1_sxbd = 571,
  Instruction_kv3_v1_sxhd = 572,
  Instruction_kv3_v1_sxlbhq = 573,
  Instruction_kv3_v1_sxlhwp = 574,
  Instruction_kv3_v1_sxmbhq = 575,
  Instruction_kv3_v1_sxmhwp = 576,
  Instruction_kv3_v1_sxwd = 577,
  Instruction_kv3_v1_syncgroup = 578,
  Instruction_kv3_v1_tlbdinval = 579,
  Instruction_kv3_v1_tlbiinval = 580,
  Instruction_kv3_v1_tlbprobe = 581,
  Instruction_kv3_v1_tlbread = 582,
  Instruction_kv3_v1_tlbwrite = 583,
  Instruction_kv3_v1_waitit = 584,
  Instruction_kv3_v1_wfxl = 585,
  Instruction_kv3_v1_wfxm = 586,
  Instruction_kv3_v1_xcopyo = 587,
  Instruction_kv3_v1_xlo = 588,
  Instruction_kv3_v1_xmma484bw = 589,
  Instruction_kv3_v1_xmma484subw = 590,
  Instruction_kv3_v1_xmma484ubw = 591,
  Instruction_kv3_v1_xmma484usbw = 592,
  Instruction_kv3_v1_xmovefo = 593,
  Instruction_kv3_v1_xmovetq = 594,
  Instruction_kv3_v1_xmt44d = 595,
  Instruction_kv3_v1_xord = 596,
  Instruction_kv3_v1_xord_abase = 597,
  Instruction_kv3_v1_xorw = 598,
  Instruction_kv3_v1_xso = 599,
  Instruction_kv3_v1_zxbd = 600,
  Instruction_kv3_v1_zxhd = 601,
  Instruction_kv3_v1_zxwd = 602,
  Separator_kv3_v1_comma = 603,
  Separator_kv3_v1_equal = 604,
  Separator_kv3_v1_qmark = 605,
  Separator_kv3_v1_rsbracket = 606,
  Separator_kv3_v1_lsbracket = 607
};

enum Modifier_kv3_v1_exunum_enum {
  Modifier_kv3_v1_exunum_ALU0=0,
  Modifier_kv3_v1_exunum_ALU1=1,
  Modifier_kv3_v1_exunum_MAU=2,
  Modifier_kv3_v1_exunum_LSU=3,
};

extern const char *mod_kv3_v1_exunum[];
extern const char *mod_kv3_v1_scalarcond[];
extern const char *mod_kv3_v1_simplecond[];
extern const char *mod_kv3_v1_comparison[];
extern const char *mod_kv3_v1_floatcomp[];
extern const char *mod_kv3_v1_rounding[];
extern const char *mod_kv3_v1_silent[];
extern const char *mod_kv3_v1_roundint[];
extern const char *mod_kv3_v1_saturate[];
extern const char *mod_kv3_v1_rectify[];
extern const char *mod_kv3_v1_variant[];
extern const char *mod_kv3_v1_speculate[];
extern const char *mod_kv3_v1_column[];
extern const char *mod_kv3_v1_doscale[];
extern const char *mod_kv3_v1_qindex[];
extern const char *mod_kv3_v1_splat32[];
typedef enum {
  Bundling_kv3_v1_ALL,
  Bundling_kv3_v1_BCU,
  Bundling_kv3_v1_TCA,
  Bundling_kv3_v1_FULL,
  Bundling_kv3_v1_FULL_X,
  Bundling_kv3_v1_FULL_Y,
  Bundling_kv3_v1_LITE,
  Bundling_kv3_v1_LITE_X,
  Bundling_kv3_v1_LITE_Y,
  Bundling_kv3_v1_MAU,
  Bundling_kv3_v1_MAU_X,
  Bundling_kv3_v1_MAU_Y,
  Bundling_kv3_v1_LSU,
  Bundling_kv3_v1_LSU_X,
  Bundling_kv3_v1_LSU_Y,
  Bundling_kv3_v1_TINY,
  Bundling_kv3_v1_TINY_X,
  Bundling_kv3_v1_TINY_Y,
  Bundling_kv3_v1_NOP,
} Bundling_kv3_v1;


static const char *bundling_kv3_v1_names(Bundling_kv3_v1 bundling) __attribute__((unused));
static const char *bundling_kv3_v1_names(Bundling_kv3_v1 bundling) {
  switch(bundling) {
  case Bundling_kv3_v1_ALL: return "Bundling_kv3_v1_ALL";
  case Bundling_kv3_v1_BCU: return "Bundling_kv3_v1_BCU";
  case Bundling_kv3_v1_TCA: return "Bundling_kv3_v1_TCA";
  case Bundling_kv3_v1_FULL: return "Bundling_kv3_v1_FULL";
  case Bundling_kv3_v1_FULL_X: return "Bundling_kv3_v1_FULL_X";
  case Bundling_kv3_v1_FULL_Y: return "Bundling_kv3_v1_FULL_Y";
  case Bundling_kv3_v1_LITE: return "Bundling_kv3_v1_LITE";
  case Bundling_kv3_v1_LITE_X: return "Bundling_kv3_v1_LITE_X";
  case Bundling_kv3_v1_LITE_Y: return "Bundling_kv3_v1_LITE_Y";
  case Bundling_kv3_v1_MAU: return "Bundling_kv3_v1_MAU";
  case Bundling_kv3_v1_MAU_X: return "Bundling_kv3_v1_MAU_X";
  case Bundling_kv3_v1_MAU_Y: return "Bundling_kv3_v1_MAU_Y";
  case Bundling_kv3_v1_LSU: return "Bundling_kv3_v1_LSU";
  case Bundling_kv3_v1_LSU_X: return "Bundling_kv3_v1_LSU_X";
  case Bundling_kv3_v1_LSU_Y: return "Bundling_kv3_v1_LSU_Y";
  case Bundling_kv3_v1_TINY: return "Bundling_kv3_v1_TINY";
  case Bundling_kv3_v1_TINY_X: return "Bundling_kv3_v1_TINY_X";
  case Bundling_kv3_v1_TINY_Y: return "Bundling_kv3_v1_TINY_Y";
  case Bundling_kv3_v1_NOP: return "Bundling_kv3_v1_NOP";
  };
  return "unknown bundling";
};

/* Resources list */
#define Resource_kv3_v1_ISSUE 0
#define Resource_kv3_v1_TINY 1
#define Resource_kv3_v1_LITE 2
#define Resource_kv3_v1_FULL 3
#define Resource_kv3_v1_LSU 4
#define Resource_kv3_v1_MAU 5
#define Resource_kv3_v1_BCU 6
#define Resource_kv3_v1_TCA 7
#define Resource_kv3_v1_AUXR 8
#define Resource_kv3_v1_AUXW 9
#define Resource_kv3_v1_CRRP 10
#define Resource_kv3_v1_CRWL 11
#define Resource_kv3_v1_CRWH 12
#define Resource_kv3_v1_NOP 13
#define kvx_kv3_v1_RESOURCE_MAX 14


/* Reservations list */
#define Reservation_kv3_v1_ALL 0
#define Reservation_kv3_v1_ALU_NOP 1
#define Reservation_kv3_v1_ALU_TINY 2
#define Reservation_kv3_v1_ALU_TINY_X 3
#define Reservation_kv3_v1_ALU_TINY_Y 4
#define Reservation_kv3_v1_ALU_LITE 5
#define Reservation_kv3_v1_ALU_LITE_X 6
#define Reservation_kv3_v1_ALU_LITE_Y 7
#define Reservation_kv3_v1_ALU_LITE_CRWL 8
#define Reservation_kv3_v1_ALU_LITE_CRWH 9
#define Reservation_kv3_v1_ALU_FULL 10
#define Reservation_kv3_v1_ALU_FULL_X 11
#define Reservation_kv3_v1_ALU_FULL_Y 12
#define Reservation_kv3_v1_BCU 13
#define Reservation_kv3_v1_BCU_CRRP_CRWL_CRWH 14
#define Reservation_kv3_v1_BCU_TINY_AUXW_CRRP 15
#define Reservation_kv3_v1_BCU_TINY_TINY_MAU_XNOP 16
#define Reservation_kv3_v1_TCA 17
#define Reservation_kv3_v1_LSU 18
#define Reservation_kv3_v1_LSU_X 19
#define Reservation_kv3_v1_LSU_Y 20
#define Reservation_kv3_v1_LSU_CRRP 21
#define Reservation_kv3_v1_LSU_CRRP_X 22
#define Reservation_kv3_v1_LSU_CRRP_Y 23
#define Reservation_kv3_v1_LSU_AUXR 24
#define Reservation_kv3_v1_LSU_AUXR_X 25
#define Reservation_kv3_v1_LSU_AUXR_Y 26
#define Reservation_kv3_v1_LSU_AUXW 27
#define Reservation_kv3_v1_LSU_AUXW_X 28
#define Reservation_kv3_v1_LSU_AUXW_Y 29
#define Reservation_kv3_v1_LSU_AUXR_AUXW 30
#define Reservation_kv3_v1_LSU_AUXR_AUXW_X 31
#define Reservation_kv3_v1_LSU_AUXR_AUXW_Y 32
#define Reservation_kv3_v1_MAU 33
#define Reservation_kv3_v1_MAU_X 34
#define Reservation_kv3_v1_MAU_Y 35
#define Reservation_kv3_v1_MAU_AUXR 36
#define Reservation_kv3_v1_MAU_AUXR_X 37
#define Reservation_kv3_v1_MAU_AUXR_Y 38


extern struct kvx_reloc kv3_v1_rel16_reloc;
extern struct kvx_reloc kv3_v1_rel32_reloc;
extern struct kvx_reloc kv3_v1_rel64_reloc;
extern struct kvx_reloc kv3_v1_pcrel_signed16_reloc;
extern struct kvx_reloc kv3_v1_pcrel17_reloc;
extern struct kvx_reloc kv3_v1_pcrel27_reloc;
extern struct kvx_reloc kv3_v1_pcrel32_reloc;
extern struct kvx_reloc kv3_v1_pcrel_signed37_reloc;
extern struct kvx_reloc kv3_v1_pcrel_signed43_reloc;
extern struct kvx_reloc kv3_v1_pcrel_signed64_reloc;
extern struct kvx_reloc kv3_v1_pcrel64_reloc;
extern struct kvx_reloc kv3_v1_signed16_reloc;
extern struct kvx_reloc kv3_v1_signed32_reloc;
extern struct kvx_reloc kv3_v1_signed37_reloc;
extern struct kvx_reloc kv3_v1_gotoff_signed37_reloc;
extern struct kvx_reloc kv3_v1_gotoff_signed43_reloc;
extern struct kvx_reloc kv3_v1_gotoff_32_reloc;
extern struct kvx_reloc kv3_v1_gotoff_64_reloc;
extern struct kvx_reloc kv3_v1_got_32_reloc;
extern struct kvx_reloc kv3_v1_got_signed37_reloc;
extern struct kvx_reloc kv3_v1_got_signed43_reloc;
extern struct kvx_reloc kv3_v1_got_64_reloc;
extern struct kvx_reloc kv3_v1_glob_dat_reloc;
extern struct kvx_reloc kv3_v1_copy_reloc;
extern struct kvx_reloc kv3_v1_jump_slot_reloc;
extern struct kvx_reloc kv3_v1_relative_reloc;
extern struct kvx_reloc kv3_v1_signed43_reloc;
extern struct kvx_reloc kv3_v1_signed64_reloc;
extern struct kvx_reloc kv3_v1_gotaddr_signed37_reloc;
extern struct kvx_reloc kv3_v1_gotaddr_signed43_reloc;
extern struct kvx_reloc kv3_v1_gotaddr_signed64_reloc;
extern struct kvx_reloc kv3_v1_dtpmod64_reloc;
extern struct kvx_reloc kv3_v1_dtpoff64_reloc;
extern struct kvx_reloc kv3_v1_dtpoff_signed37_reloc;
extern struct kvx_reloc kv3_v1_dtpoff_signed43_reloc;
extern struct kvx_reloc kv3_v1_tlsgd_signed37_reloc;
extern struct kvx_reloc kv3_v1_tlsgd_signed43_reloc;
extern struct kvx_reloc kv3_v1_tlsld_signed37_reloc;
extern struct kvx_reloc kv3_v1_tlsld_signed43_reloc;
extern struct kvx_reloc kv3_v1_tpoff64_reloc;
extern struct kvx_reloc kv3_v1_tlsie_signed37_reloc;
extern struct kvx_reloc kv3_v1_tlsie_signed43_reloc;
extern struct kvx_reloc kv3_v1_tlsle_signed37_reloc;
extern struct kvx_reloc kv3_v1_tlsle_signed43_reloc;
extern struct kvx_reloc kv3_v1_rel8_reloc;

#define KVX_REGFILE_FIRST_GPR 0
#define KVX_REGFILE_LAST_GPR 1
#define KVX_REGFILE_DEC_GPR 2
#define KVX_REGFILE_FIRST_PGR 3
#define KVX_REGFILE_LAST_PGR 4
#define KVX_REGFILE_DEC_PGR 5
#define KVX_REGFILE_FIRST_QGR 6
#define KVX_REGFILE_LAST_QGR 7
#define KVX_REGFILE_DEC_QGR 8
#define KVX_REGFILE_FIRST_SFR 9
#define KVX_REGFILE_LAST_SFR 10
#define KVX_REGFILE_DEC_SFR 11
#define KVX_REGFILE_FIRST_X16R 12
#define KVX_REGFILE_LAST_X16R 13
#define KVX_REGFILE_DEC_X16R 14
#define KVX_REGFILE_FIRST_X2R 15
#define KVX_REGFILE_LAST_X2R 16
#define KVX_REGFILE_DEC_X2R 17
#define KVX_REGFILE_FIRST_X32R 18
#define KVX_REGFILE_LAST_X32R 19
#define KVX_REGFILE_DEC_X32R 20
#define KVX_REGFILE_FIRST_X4R 21
#define KVX_REGFILE_LAST_X4R 22
#define KVX_REGFILE_DEC_X4R 23
#define KVX_REGFILE_FIRST_X64R 24
#define KVX_REGFILE_LAST_X64R 25
#define KVX_REGFILE_DEC_X64R 26
#define KVX_REGFILE_FIRST_X8R 27
#define KVX_REGFILE_LAST_X8R 28
#define KVX_REGFILE_DEC_X8R 29
#define KVX_REGFILE_FIRST_XBR 30
#define KVX_REGFILE_LAST_XBR 31
#define KVX_REGFILE_DEC_XBR 32
#define KVX_REGFILE_FIRST_XCR 33
#define KVX_REGFILE_LAST_XCR 34
#define KVX_REGFILE_DEC_XCR 35
#define KVX_REGFILE_FIRST_XMR 36
#define KVX_REGFILE_LAST_XMR 37
#define KVX_REGFILE_DEC_XMR 38
#define KVX_REGFILE_FIRST_XTR 39
#define KVX_REGFILE_LAST_XTR 40
#define KVX_REGFILE_DEC_XTR 41
#define KVX_REGFILE_FIRST_XVR 42
#define KVX_REGFILE_LAST_XVR 43
#define KVX_REGFILE_DEC_XVR 44
#define KVX_REGFILE_REGISTERS 45
#define KVX_REGFILE_DEC_REGISTERS 46


extern int kvx_kv3_v2_regfiles[];
extern const char **kvx_kv3_v2_modifiers[];
extern struct kvx_Register kvx_kv3_v2_registers[];

extern int kvx_kv3_v2_dec_registers[];

enum Method_kvx_kv3_v2_enum {
  Immediate_kv3_v2_brknumber = 1,
  Immediate_kv3_v2_pcrel17 = 2,
  Immediate_kv3_v2_pcrel27 = 3,
  Immediate_kv3_v2_signed10 = 4,
  Immediate_kv3_v2_signed16 = 5,
  Immediate_kv3_v2_signed27 = 6,
  Immediate_kv3_v2_signed37 = 7,
  Immediate_kv3_v2_signed43 = 8,
  Immediate_kv3_v2_signed54 = 9,
  Immediate_kv3_v2_sysnumber = 10,
  Immediate_kv3_v2_unsigned6 = 11,
  Immediate_kv3_v2_wrapped32 = 12,
  Immediate_kv3_v2_wrapped64 = 13,
  Immediate_kv3_v2_wrapped8 = 14,
  Modifier_kv3_v2_accesses = 15,
  Modifier_kv3_v2_boolcas = 16,
  Modifier_kv3_v2_cachelev = 17,
  Modifier_kv3_v2_channel = 18,
  Modifier_kv3_v2_coherency = 19,
  Modifier_kv3_v2_comparison = 20,
  Modifier_kv3_v2_conjugate = 21,
  Modifier_kv3_v2_doscale = 22,
  Modifier_kv3_v2_exunum = 23,
  Modifier_kv3_v2_floatcomp = 24,
  Modifier_kv3_v2_hindex = 25,
  Modifier_kv3_v2_lsomask = 26,
  Modifier_kv3_v2_lsumask = 27,
  Modifier_kv3_v2_lsupack = 28,
  Modifier_kv3_v2_qindex = 29,
  Modifier_kv3_v2_rounding = 30,
  Modifier_kv3_v2_scalarcond = 31,
  Modifier_kv3_v2_shuffleV = 32,
  Modifier_kv3_v2_shuffleX = 33,
  Modifier_kv3_v2_silent = 34,
  Modifier_kv3_v2_simplecond = 35,
  Modifier_kv3_v2_speculate = 36,
  Modifier_kv3_v2_splat32 = 37,
  Modifier_kv3_v2_transpose = 38,
  Modifier_kv3_v2_variant = 39,
  RegClass_kv3_v2_aloneReg = 40,
  RegClass_kv3_v2_blockReg = 41,
  RegClass_kv3_v2_blockRegE = 42,
  RegClass_kv3_v2_blockRegO = 43,
  RegClass_kv3_v2_blockReg_0 = 44,
  RegClass_kv3_v2_blockReg_1 = 45,
  RegClass_kv3_v2_buffer16Reg = 46,
  RegClass_kv3_v2_buffer2Reg = 47,
  RegClass_kv3_v2_buffer32Reg = 48,
  RegClass_kv3_v2_buffer4Reg = 49,
  RegClass_kv3_v2_buffer64Reg = 50,
  RegClass_kv3_v2_buffer8Reg = 51,
  RegClass_kv3_v2_coproReg = 52,
  RegClass_kv3_v2_coproReg0M4 = 53,
  RegClass_kv3_v2_coproReg1M4 = 54,
  RegClass_kv3_v2_coproReg2M4 = 55,
  RegClass_kv3_v2_coproReg3M4 = 56,
  RegClass_kv3_v2_matrixReg = 57,
  RegClass_kv3_v2_matrixReg_0 = 58,
  RegClass_kv3_v2_matrixReg_1 = 59,
  RegClass_kv3_v2_matrixReg_2 = 60,
  RegClass_kv3_v2_matrixReg_3 = 61,
  RegClass_kv3_v2_onlyfxReg = 62,
  RegClass_kv3_v2_onlygetReg = 63,
  RegClass_kv3_v2_onlyraReg = 64,
  RegClass_kv3_v2_onlysetReg = 65,
  RegClass_kv3_v2_onlyswapReg = 66,
  RegClass_kv3_v2_pairedReg = 67,
  RegClass_kv3_v2_pairedReg_0 = 68,
  RegClass_kv3_v2_pairedReg_1 = 69,
  RegClass_kv3_v2_quadReg = 70,
  RegClass_kv3_v2_quadReg_0 = 71,
  RegClass_kv3_v2_quadReg_1 = 72,
  RegClass_kv3_v2_quadReg_2 = 73,
  RegClass_kv3_v2_quadReg_3 = 74,
  RegClass_kv3_v2_singleReg = 75,
  RegClass_kv3_v2_systemReg = 76,
  RegClass_kv3_v2_tileReg = 77,
  RegClass_kv3_v2_tileReg_0 = 78,
  RegClass_kv3_v2_tileReg_1 = 79,
  RegClass_kv3_v2_vectorReg = 80,
  RegClass_kv3_v2_vectorReg_0 = 81,
  RegClass_kv3_v2_vectorReg_1 = 82,
  RegClass_kv3_v2_vectorReg_2 = 83,
  RegClass_kv3_v2_vectorReg_3 = 84,
  Instruction_kv3_v2_abdbo = 85,
  Instruction_kv3_v2_abdd = 86,
  Instruction_kv3_v2_abdd_abase = 87,
  Instruction_kv3_v2_abdhq = 88,
  Instruction_kv3_v2_abdsbo = 89,
  Instruction_kv3_v2_abdsd = 90,
  Instruction_kv3_v2_abdshq = 91,
  Instruction_kv3_v2_abdsw = 92,
  Instruction_kv3_v2_abdswp = 93,
  Instruction_kv3_v2_abdubo = 94,
  Instruction_kv3_v2_abdud = 95,
  Instruction_kv3_v2_abduhq = 96,
  Instruction_kv3_v2_abduw = 97,
  Instruction_kv3_v2_abduwp = 98,
  Instruction_kv3_v2_abdw = 99,
  Instruction_kv3_v2_abdwp = 100,
  Instruction_kv3_v2_absbo = 101,
  Instruction_kv3_v2_absd = 102,
  Instruction_kv3_v2_abshq = 103,
  Instruction_kv3_v2_abssbo = 104,
  Instruction_kv3_v2_abssd = 105,
  Instruction_kv3_v2_absshq = 106,
  Instruction_kv3_v2_abssw = 107,
  Instruction_kv3_v2_absswp = 108,
  Instruction_kv3_v2_absw = 109,
  Instruction_kv3_v2_abswp = 110,
  Instruction_kv3_v2_acswapd = 111,
  Instruction_kv3_v2_acswapq = 112,
  Instruction_kv3_v2_acswapw = 113,
  Instruction_kv3_v2_addbo = 114,
  Instruction_kv3_v2_addcd = 115,
  Instruction_kv3_v2_addcd_i = 116,
  Instruction_kv3_v2_addd = 117,
  Instruction_kv3_v2_addd_abase = 118,
  Instruction_kv3_v2_addhq = 119,
  Instruction_kv3_v2_addrbod = 120,
  Instruction_kv3_v2_addrhqd = 121,
  Instruction_kv3_v2_addrwpd = 122,
  Instruction_kv3_v2_addsbo = 123,
  Instruction_kv3_v2_addsd = 124,
  Instruction_kv3_v2_addshq = 125,
  Instruction_kv3_v2_addsw = 126,
  Instruction_kv3_v2_addswp = 127,
  Instruction_kv3_v2_addurbod = 128,
  Instruction_kv3_v2_addurhqd = 129,
  Instruction_kv3_v2_addurwpd = 130,
  Instruction_kv3_v2_addusbo = 131,
  Instruction_kv3_v2_addusd = 132,
  Instruction_kv3_v2_addushq = 133,
  Instruction_kv3_v2_addusw = 134,
  Instruction_kv3_v2_adduswp = 135,
  Instruction_kv3_v2_adduwd = 136,
  Instruction_kv3_v2_addw = 137,
  Instruction_kv3_v2_addwd = 138,
  Instruction_kv3_v2_addwp = 139,
  Instruction_kv3_v2_addx16bo = 140,
  Instruction_kv3_v2_addx16d = 141,
  Instruction_kv3_v2_addx16hq = 142,
  Instruction_kv3_v2_addx16uwd = 143,
  Instruction_kv3_v2_addx16w = 144,
  Instruction_kv3_v2_addx16wd = 145,
  Instruction_kv3_v2_addx16wp = 146,
  Instruction_kv3_v2_addx2bo = 147,
  Instruction_kv3_v2_addx2d = 148,
  Instruction_kv3_v2_addx2hq = 149,
  Instruction_kv3_v2_addx2uwd = 150,
  Instruction_kv3_v2_addx2w = 151,
  Instruction_kv3_v2_addx2wd = 152,
  Instruction_kv3_v2_addx2wp = 153,
  Instruction_kv3_v2_addx32d = 154,
  Instruction_kv3_v2_addx32uwd = 155,
  Instruction_kv3_v2_addx32w = 156,
  Instruction_kv3_v2_addx32wd = 157,
  Instruction_kv3_v2_addx4bo = 158,
  Instruction_kv3_v2_addx4d = 159,
  Instruction_kv3_v2_addx4hq = 160,
  Instruction_kv3_v2_addx4uwd = 161,
  Instruction_kv3_v2_addx4w = 162,
  Instruction_kv3_v2_addx4wd = 163,
  Instruction_kv3_v2_addx4wp = 164,
  Instruction_kv3_v2_addx64d = 165,
  Instruction_kv3_v2_addx64uwd = 166,
  Instruction_kv3_v2_addx64w = 167,
  Instruction_kv3_v2_addx64wd = 168,
  Instruction_kv3_v2_addx8bo = 169,
  Instruction_kv3_v2_addx8d = 170,
  Instruction_kv3_v2_addx8hq = 171,
  Instruction_kv3_v2_addx8uwd = 172,
  Instruction_kv3_v2_addx8w = 173,
  Instruction_kv3_v2_addx8wd = 174,
  Instruction_kv3_v2_addx8wp = 175,
  Instruction_kv3_v2_aladdd = 176,
  Instruction_kv3_v2_aladdw = 177,
  Instruction_kv3_v2_alclrd = 178,
  Instruction_kv3_v2_alclrw = 179,
  Instruction_kv3_v2_ald = 180,
  Instruction_kv3_v2_alw = 181,
  Instruction_kv3_v2_andd = 182,
  Instruction_kv3_v2_andd_abase = 183,
  Instruction_kv3_v2_andnd = 184,
  Instruction_kv3_v2_andnd_abase = 185,
  Instruction_kv3_v2_andnw = 186,
  Instruction_kv3_v2_andrbod = 187,
  Instruction_kv3_v2_andrhqd = 188,
  Instruction_kv3_v2_andrwpd = 189,
  Instruction_kv3_v2_andw = 190,
  Instruction_kv3_v2_asd = 191,
  Instruction_kv3_v2_asw = 192,
  Instruction_kv3_v2_avgbo = 193,
  Instruction_kv3_v2_avghq = 194,
  Instruction_kv3_v2_avgrbo = 195,
  Instruction_kv3_v2_avgrhq = 196,
  Instruction_kv3_v2_avgrubo = 197,
  Instruction_kv3_v2_avgruhq = 198,
  Instruction_kv3_v2_avgruw = 199,
  Instruction_kv3_v2_avgruwp = 200,
  Instruction_kv3_v2_avgrw = 201,
  Instruction_kv3_v2_avgrwp = 202,
  Instruction_kv3_v2_avgubo = 203,
  Instruction_kv3_v2_avguhq = 204,
  Instruction_kv3_v2_avguw = 205,
  Instruction_kv3_v2_avguwp = 206,
  Instruction_kv3_v2_avgw = 207,
  Instruction_kv3_v2_avgwp = 208,
  Instruction_kv3_v2_await = 209,
  Instruction_kv3_v2_barrier = 210,
  Instruction_kv3_v2_break = 211,
  Instruction_kv3_v2_call = 212,
  Instruction_kv3_v2_cb = 213,
  Instruction_kv3_v2_cbsd = 214,
  Instruction_kv3_v2_cbsw = 215,
  Instruction_kv3_v2_cbswp = 216,
  Instruction_kv3_v2_clrf = 217,
  Instruction_kv3_v2_clsd = 218,
  Instruction_kv3_v2_clsw = 219,
  Instruction_kv3_v2_clswp = 220,
  Instruction_kv3_v2_clzd = 221,
  Instruction_kv3_v2_clzw = 222,
  Instruction_kv3_v2_clzwp = 223,
  Instruction_kv3_v2_cmovebo = 224,
  Instruction_kv3_v2_cmoved = 225,
  Instruction_kv3_v2_cmovehq = 226,
  Instruction_kv3_v2_cmovewp = 227,
  Instruction_kv3_v2_cmuldt = 228,
  Instruction_kv3_v2_cmulghxdt = 229,
  Instruction_kv3_v2_cmulglxdt = 230,
  Instruction_kv3_v2_cmulgmxdt = 231,
  Instruction_kv3_v2_cmulxdt = 232,
  Instruction_kv3_v2_compd = 233,
  Instruction_kv3_v2_compnbo = 234,
  Instruction_kv3_v2_compnd = 235,
  Instruction_kv3_v2_compnhq = 236,
  Instruction_kv3_v2_compnw = 237,
  Instruction_kv3_v2_compnwp = 238,
  Instruction_kv3_v2_compuwd = 239,
  Instruction_kv3_v2_compw = 240,
  Instruction_kv3_v2_compwd = 241,
  Instruction_kv3_v2_copyd = 242,
  Instruction_kv3_v2_copyo = 243,
  Instruction_kv3_v2_copyq = 244,
  Instruction_kv3_v2_copyw = 245,
  Instruction_kv3_v2_crcbellw = 246,
  Instruction_kv3_v2_crcbelmw = 247,
  Instruction_kv3_v2_crclellw = 248,
  Instruction_kv3_v2_crclelmw = 249,
  Instruction_kv3_v2_ctzd = 250,
  Instruction_kv3_v2_ctzw = 251,
  Instruction_kv3_v2_ctzwp = 252,
  Instruction_kv3_v2_d1inval = 253,
  Instruction_kv3_v2_dflushl = 254,
  Instruction_kv3_v2_dflushsw = 255,
  Instruction_kv3_v2_dinvall = 256,
  Instruction_kv3_v2_dinvalsw = 257,
  Instruction_kv3_v2_dot2suwd = 258,
  Instruction_kv3_v2_dot2suwdp = 259,
  Instruction_kv3_v2_dot2uwd = 260,
  Instruction_kv3_v2_dot2uwdp = 261,
  Instruction_kv3_v2_dot2w = 262,
  Instruction_kv3_v2_dot2wd = 263,
  Instruction_kv3_v2_dot2wdp = 264,
  Instruction_kv3_v2_dot2wzp = 265,
  Instruction_kv3_v2_dpurgel = 266,
  Instruction_kv3_v2_dpurgesw = 267,
  Instruction_kv3_v2_dtouchl = 268,
  Instruction_kv3_v2_errop = 269,
  Instruction_kv3_v2_extfs = 270,
  Instruction_kv3_v2_extfz = 271,
  Instruction_kv3_v2_fabsd = 272,
  Instruction_kv3_v2_fabshq = 273,
  Instruction_kv3_v2_fabsw = 274,
  Instruction_kv3_v2_fabswp = 275,
  Instruction_kv3_v2_faddd = 276,
  Instruction_kv3_v2_fadddc = 277,
  Instruction_kv3_v2_fadddc_c = 278,
  Instruction_kv3_v2_fadddp = 279,
  Instruction_kv3_v2_faddho = 280,
  Instruction_kv3_v2_faddhq = 281,
  Instruction_kv3_v2_faddw = 282,
  Instruction_kv3_v2_faddwc = 283,
  Instruction_kv3_v2_faddwc_c = 284,
  Instruction_kv3_v2_faddwcp = 285,
  Instruction_kv3_v2_faddwcp_c = 286,
  Instruction_kv3_v2_faddwp = 287,
  Instruction_kv3_v2_faddwq = 288,
  Instruction_kv3_v2_fcdivd = 289,
  Instruction_kv3_v2_fcdivw = 290,
  Instruction_kv3_v2_fcdivwp = 291,
  Instruction_kv3_v2_fcompd = 292,
  Instruction_kv3_v2_fcompnd = 293,
  Instruction_kv3_v2_fcompnhq = 294,
  Instruction_kv3_v2_fcompnw = 295,
  Instruction_kv3_v2_fcompnwp = 296,
  Instruction_kv3_v2_fcompw = 297,
  Instruction_kv3_v2_fdot2w = 298,
  Instruction_kv3_v2_fdot2wd = 299,
  Instruction_kv3_v2_fdot2wdp = 300,
  Instruction_kv3_v2_fdot2wzp = 301,
  Instruction_kv3_v2_fence = 302,
  Instruction_kv3_v2_ffdmasw = 303,
  Instruction_kv3_v2_ffdmaswp = 304,
  Instruction_kv3_v2_ffdmaswq = 305,
  Instruction_kv3_v2_ffdmaw = 306,
  Instruction_kv3_v2_ffdmawp = 307,
  Instruction_kv3_v2_ffdmawq = 308,
  Instruction_kv3_v2_ffdmdaw = 309,
  Instruction_kv3_v2_ffdmdawp = 310,
  Instruction_kv3_v2_ffdmdawq = 311,
  Instruction_kv3_v2_ffdmdsw = 312,
  Instruction_kv3_v2_ffdmdswp = 313,
  Instruction_kv3_v2_ffdmdswq = 314,
  Instruction_kv3_v2_ffdmsaw = 315,
  Instruction_kv3_v2_ffdmsawp = 316,
  Instruction_kv3_v2_ffdmsawq = 317,
  Instruction_kv3_v2_ffdmsw = 318,
  Instruction_kv3_v2_ffdmswp = 319,
  Instruction_kv3_v2_ffdmswq = 320,
  Instruction_kv3_v2_ffmad = 321,
  Instruction_kv3_v2_ffmaho = 322,
  Instruction_kv3_v2_ffmahq = 323,
  Instruction_kv3_v2_ffmahw = 324,
  Instruction_kv3_v2_ffmahwq = 325,
  Instruction_kv3_v2_ffmaw = 326,
  Instruction_kv3_v2_ffmawc = 327,
  Instruction_kv3_v2_ffmawcp = 328,
  Instruction_kv3_v2_ffmawd = 329,
  Instruction_kv3_v2_ffmawdp = 330,
  Instruction_kv3_v2_ffmawp = 331,
  Instruction_kv3_v2_ffmawq = 332,
  Instruction_kv3_v2_ffmsd = 333,
  Instruction_kv3_v2_ffmsho = 334,
  Instruction_kv3_v2_ffmshq = 335,
  Instruction_kv3_v2_ffmshw = 336,
  Instruction_kv3_v2_ffmshwq = 337,
  Instruction_kv3_v2_ffmsw = 338,
  Instruction_kv3_v2_ffmswc = 339,
  Instruction_kv3_v2_ffmswcp = 340,
  Instruction_kv3_v2_ffmswd = 341,
  Instruction_kv3_v2_ffmswdp = 342,
  Instruction_kv3_v2_ffmswp = 343,
  Instruction_kv3_v2_ffmswq = 344,
  Instruction_kv3_v2_fixedd = 345,
  Instruction_kv3_v2_fixedud = 346,
  Instruction_kv3_v2_fixeduw = 347,
  Instruction_kv3_v2_fixeduwp = 348,
  Instruction_kv3_v2_fixedw = 349,
  Instruction_kv3_v2_fixedwp = 350,
  Instruction_kv3_v2_floatd = 351,
  Instruction_kv3_v2_floatud = 352,
  Instruction_kv3_v2_floatuw = 353,
  Instruction_kv3_v2_floatuwp = 354,
  Instruction_kv3_v2_floatw = 355,
  Instruction_kv3_v2_floatwp = 356,
  Instruction_kv3_v2_fmaxd = 357,
  Instruction_kv3_v2_fmaxhq = 358,
  Instruction_kv3_v2_fmaxw = 359,
  Instruction_kv3_v2_fmaxwp = 360,
  Instruction_kv3_v2_fmind = 361,
  Instruction_kv3_v2_fminhq = 362,
  Instruction_kv3_v2_fminw = 363,
  Instruction_kv3_v2_fminwp = 364,
  Instruction_kv3_v2_fmm212w = 365,
  Instruction_kv3_v2_fmm222w = 366,
  Instruction_kv3_v2_fmma212w = 367,
  Instruction_kv3_v2_fmma222w = 368,
  Instruction_kv3_v2_fmms212w = 369,
  Instruction_kv3_v2_fmms222w = 370,
  Instruction_kv3_v2_fmuld = 371,
  Instruction_kv3_v2_fmulho = 372,
  Instruction_kv3_v2_fmulhq = 373,
  Instruction_kv3_v2_fmulhw = 374,
  Instruction_kv3_v2_fmulhwq = 375,
  Instruction_kv3_v2_fmulw = 376,
  Instruction_kv3_v2_fmulwc = 377,
  Instruction_kv3_v2_fmulwcp = 378,
  Instruction_kv3_v2_fmulwd = 379,
  Instruction_kv3_v2_fmulwdp = 380,
  Instruction_kv3_v2_fmulwp = 381,
  Instruction_kv3_v2_fmulwq = 382,
  Instruction_kv3_v2_fnarrowdw = 383,
  Instruction_kv3_v2_fnarrowdwp = 384,
  Instruction_kv3_v2_fnarrowwh = 385,
  Instruction_kv3_v2_fnarrowwhq = 386,
  Instruction_kv3_v2_fnegd = 387,
  Instruction_kv3_v2_fneghq = 388,
  Instruction_kv3_v2_fnegw = 389,
  Instruction_kv3_v2_fnegwp = 390,
  Instruction_kv3_v2_frecw = 391,
  Instruction_kv3_v2_frsrw = 392,
  Instruction_kv3_v2_fsbfd = 393,
  Instruction_kv3_v2_fsbfdc = 394,
  Instruction_kv3_v2_fsbfdc_c = 395,
  Instruction_kv3_v2_fsbfdp = 396,
  Instruction_kv3_v2_fsbfho = 397,
  Instruction_kv3_v2_fsbfhq = 398,
  Instruction_kv3_v2_fsbfw = 399,
  Instruction_kv3_v2_fsbfwc = 400,
  Instruction_kv3_v2_fsbfwc_c = 401,
  Instruction_kv3_v2_fsbfwcp = 402,
  Instruction_kv3_v2_fsbfwcp_c = 403,
  Instruction_kv3_v2_fsbfwp = 404,
  Instruction_kv3_v2_fsbfwq = 405,
  Instruction_kv3_v2_fsdivd = 406,
  Instruction_kv3_v2_fsdivw = 407,
  Instruction_kv3_v2_fsdivwp = 408,
  Instruction_kv3_v2_fsrecd = 409,
  Instruction_kv3_v2_fsrecw = 410,
  Instruction_kv3_v2_fsrecwp = 411,
  Instruction_kv3_v2_fsrsrd = 412,
  Instruction_kv3_v2_fsrsrw = 413,
  Instruction_kv3_v2_fsrsrwp = 414,
  Instruction_kv3_v2_fwidenlhw = 415,
  Instruction_kv3_v2_fwidenlhwp = 416,
  Instruction_kv3_v2_fwidenlwd = 417,
  Instruction_kv3_v2_fwidenmhw = 418,
  Instruction_kv3_v2_fwidenmhwp = 419,
  Instruction_kv3_v2_fwidenmwd = 420,
  Instruction_kv3_v2_get = 421,
  Instruction_kv3_v2_goto = 422,
  Instruction_kv3_v2_i1inval = 423,
  Instruction_kv3_v2_i1invals = 424,
  Instruction_kv3_v2_icall = 425,
  Instruction_kv3_v2_iget = 426,
  Instruction_kv3_v2_igoto = 427,
  Instruction_kv3_v2_insf = 428,
  Instruction_kv3_v2_landd = 429,
  Instruction_kv3_v2_landw = 430,
  Instruction_kv3_v2_lbs = 431,
  Instruction_kv3_v2_lbz = 432,
  Instruction_kv3_v2_ld = 433,
  Instruction_kv3_v2_lhs = 434,
  Instruction_kv3_v2_lhz = 435,
  Instruction_kv3_v2_lnandd = 436,
  Instruction_kv3_v2_lnandw = 437,
  Instruction_kv3_v2_lnord = 438,
  Instruction_kv3_v2_lnorw = 439,
  Instruction_kv3_v2_lo = 440,
  Instruction_kv3_v2_loopdo = 441,
  Instruction_kv3_v2_lord = 442,
  Instruction_kv3_v2_lorw = 443,
  Instruction_kv3_v2_lq = 444,
  Instruction_kv3_v2_lws = 445,
  Instruction_kv3_v2_lwz = 446,
  Instruction_kv3_v2_maddd = 447,
  Instruction_kv3_v2_madddt = 448,
  Instruction_kv3_v2_maddhq = 449,
  Instruction_kv3_v2_maddhwq = 450,
  Instruction_kv3_v2_maddmwq = 451,
  Instruction_kv3_v2_maddsudt = 452,
  Instruction_kv3_v2_maddsuhwq = 453,
  Instruction_kv3_v2_maddsumwq = 454,
  Instruction_kv3_v2_maddsuwd = 455,
  Instruction_kv3_v2_maddsuwdp = 456,
  Instruction_kv3_v2_maddudt = 457,
  Instruction_kv3_v2_madduhwq = 458,
  Instruction_kv3_v2_maddumwq = 459,
  Instruction_kv3_v2_madduwd = 460,
  Instruction_kv3_v2_madduwdp = 461,
  Instruction_kv3_v2_madduzdt = 462,
  Instruction_kv3_v2_maddw = 463,
  Instruction_kv3_v2_maddwd = 464,
  Instruction_kv3_v2_maddwdp = 465,
  Instruction_kv3_v2_maddwp = 466,
  Instruction_kv3_v2_maddwq = 467,
  Instruction_kv3_v2_make = 468,
  Instruction_kv3_v2_maxbo = 469,
  Instruction_kv3_v2_maxd = 470,
  Instruction_kv3_v2_maxd_abase = 471,
  Instruction_kv3_v2_maxhq = 472,
  Instruction_kv3_v2_maxrbod = 473,
  Instruction_kv3_v2_maxrhqd = 474,
  Instruction_kv3_v2_maxrwpd = 475,
  Instruction_kv3_v2_maxubo = 476,
  Instruction_kv3_v2_maxud = 477,
  Instruction_kv3_v2_maxud_abase = 478,
  Instruction_kv3_v2_maxuhq = 479,
  Instruction_kv3_v2_maxurbod = 480,
  Instruction_kv3_v2_maxurhqd = 481,
  Instruction_kv3_v2_maxurwpd = 482,
  Instruction_kv3_v2_maxuw = 483,
  Instruction_kv3_v2_maxuwp = 484,
  Instruction_kv3_v2_maxw = 485,
  Instruction_kv3_v2_maxwp = 486,
  Instruction_kv3_v2_minbo = 487,
  Instruction_kv3_v2_mind = 488,
  Instruction_kv3_v2_mind_abase = 489,
  Instruction_kv3_v2_minhq = 490,
  Instruction_kv3_v2_minrbod = 491,
  Instruction_kv3_v2_minrhqd = 492,
  Instruction_kv3_v2_minrwpd = 493,
  Instruction_kv3_v2_minubo = 494,
  Instruction_kv3_v2_minud = 495,
  Instruction_kv3_v2_minud_abase = 496,
  Instruction_kv3_v2_minuhq = 497,
  Instruction_kv3_v2_minurbod = 498,
  Instruction_kv3_v2_minurhqd = 499,
  Instruction_kv3_v2_minurwpd = 500,
  Instruction_kv3_v2_minuw = 501,
  Instruction_kv3_v2_minuwp = 502,
  Instruction_kv3_v2_minw = 503,
  Instruction_kv3_v2_minwp = 504,
  Instruction_kv3_v2_mm212w = 505,
  Instruction_kv3_v2_mma212w = 506,
  Instruction_kv3_v2_mms212w = 507,
  Instruction_kv3_v2_msbfd = 508,
  Instruction_kv3_v2_msbfdt = 509,
  Instruction_kv3_v2_msbfhq = 510,
  Instruction_kv3_v2_msbfhwq = 511,
  Instruction_kv3_v2_msbfmwq = 512,
  Instruction_kv3_v2_msbfsudt = 513,
  Instruction_kv3_v2_msbfsuhwq = 514,
  Instruction_kv3_v2_msbfsumwq = 515,
  Instruction_kv3_v2_msbfsuwd = 516,
  Instruction_kv3_v2_msbfsuwdp = 517,
  Instruction_kv3_v2_msbfudt = 518,
  Instruction_kv3_v2_msbfuhwq = 519,
  Instruction_kv3_v2_msbfumwq = 520,
  Instruction_kv3_v2_msbfuwd = 521,
  Instruction_kv3_v2_msbfuwdp = 522,
  Instruction_kv3_v2_msbfuzdt = 523,
  Instruction_kv3_v2_msbfw = 524,
  Instruction_kv3_v2_msbfwd = 525,
  Instruction_kv3_v2_msbfwdp = 526,
  Instruction_kv3_v2_msbfwp = 527,
  Instruction_kv3_v2_msbfwq = 528,
  Instruction_kv3_v2_muld = 529,
  Instruction_kv3_v2_muldt = 530,
  Instruction_kv3_v2_mulhq = 531,
  Instruction_kv3_v2_mulhwq = 532,
  Instruction_kv3_v2_mulmwq = 533,
  Instruction_kv3_v2_mulsudt = 534,
  Instruction_kv3_v2_mulsuhwq = 535,
  Instruction_kv3_v2_mulsumwq = 536,
  Instruction_kv3_v2_mulsuwd = 537,
  Instruction_kv3_v2_mulsuwdp = 538,
  Instruction_kv3_v2_muludt = 539,
  Instruction_kv3_v2_muluhwq = 540,
  Instruction_kv3_v2_mulumwq = 541,
  Instruction_kv3_v2_muluwd = 542,
  Instruction_kv3_v2_muluwdp = 543,
  Instruction_kv3_v2_mulw = 544,
  Instruction_kv3_v2_mulwd = 545,
  Instruction_kv3_v2_mulwdp = 546,
  Instruction_kv3_v2_mulwp = 547,
  Instruction_kv3_v2_mulwq = 548,
  Instruction_kv3_v2_nandd = 549,
  Instruction_kv3_v2_nandd_abase = 550,
  Instruction_kv3_v2_nandw = 551,
  Instruction_kv3_v2_negbo = 552,
  Instruction_kv3_v2_negd = 553,
  Instruction_kv3_v2_neghq = 554,
  Instruction_kv3_v2_negsbo = 555,
  Instruction_kv3_v2_negsd = 556,
  Instruction_kv3_v2_negshq = 557,
  Instruction_kv3_v2_negsw = 558,
  Instruction_kv3_v2_negswp = 559,
  Instruction_kv3_v2_negw = 560,
  Instruction_kv3_v2_negwp = 561,
  Instruction_kv3_v2_nop = 562,
  Instruction_kv3_v2_nord = 563,
  Instruction_kv3_v2_nord_abase = 564,
  Instruction_kv3_v2_norw = 565,
  Instruction_kv3_v2_notd = 566,
  Instruction_kv3_v2_notw = 567,
  Instruction_kv3_v2_nxord = 568,
  Instruction_kv3_v2_nxord_abase = 569,
  Instruction_kv3_v2_nxorw = 570,
  Instruction_kv3_v2_ord = 571,
  Instruction_kv3_v2_ord_abase = 572,
  Instruction_kv3_v2_ornd = 573,
  Instruction_kv3_v2_ornd_abase = 574,
  Instruction_kv3_v2_ornw = 575,
  Instruction_kv3_v2_orrbod = 576,
  Instruction_kv3_v2_orrhqd = 577,
  Instruction_kv3_v2_orrwpd = 578,
  Instruction_kv3_v2_orw = 579,
  Instruction_kv3_v2_pcrel = 580,
  Instruction_kv3_v2_ret = 581,
  Instruction_kv3_v2_rfe = 582,
  Instruction_kv3_v2_rolw = 583,
  Instruction_kv3_v2_rolwps = 584,
  Instruction_kv3_v2_rorw = 585,
  Instruction_kv3_v2_rorwps = 586,
  Instruction_kv3_v2_rswap = 587,
  Instruction_kv3_v2_sb = 588,
  Instruction_kv3_v2_sbfbo = 589,
  Instruction_kv3_v2_sbfcd = 590,
  Instruction_kv3_v2_sbfcd_i = 591,
  Instruction_kv3_v2_sbfd = 592,
  Instruction_kv3_v2_sbfd_abase = 593,
  Instruction_kv3_v2_sbfhq = 594,
  Instruction_kv3_v2_sbfsbo = 595,
  Instruction_kv3_v2_sbfsd = 596,
  Instruction_kv3_v2_sbfshq = 597,
  Instruction_kv3_v2_sbfsw = 598,
  Instruction_kv3_v2_sbfswp = 599,
  Instruction_kv3_v2_sbfusbo = 600,
  Instruction_kv3_v2_sbfusd = 601,
  Instruction_kv3_v2_sbfushq = 602,
  Instruction_kv3_v2_sbfusw = 603,
  Instruction_kv3_v2_sbfuswp = 604,
  Instruction_kv3_v2_sbfuwd = 605,
  Instruction_kv3_v2_sbfw = 606,
  Instruction_kv3_v2_sbfwd = 607,
  Instruction_kv3_v2_sbfwp = 608,
  Instruction_kv3_v2_sbfx16bo = 609,
  Instruction_kv3_v2_sbfx16d = 610,
  Instruction_kv3_v2_sbfx16hq = 611,
  Instruction_kv3_v2_sbfx16uwd = 612,
  Instruction_kv3_v2_sbfx16w = 613,
  Instruction_kv3_v2_sbfx16wd = 614,
  Instruction_kv3_v2_sbfx16wp = 615,
  Instruction_kv3_v2_sbfx2bo = 616,
  Instruction_kv3_v2_sbfx2d = 617,
  Instruction_kv3_v2_sbfx2hq = 618,
  Instruction_kv3_v2_sbfx2uwd = 619,
  Instruction_kv3_v2_sbfx2w = 620,
  Instruction_kv3_v2_sbfx2wd = 621,
  Instruction_kv3_v2_sbfx2wp = 622,
  Instruction_kv3_v2_sbfx32d = 623,
  Instruction_kv3_v2_sbfx32uwd = 624,
  Instruction_kv3_v2_sbfx32w = 625,
  Instruction_kv3_v2_sbfx32wd = 626,
  Instruction_kv3_v2_sbfx4bo = 627,
  Instruction_kv3_v2_sbfx4d = 628,
  Instruction_kv3_v2_sbfx4hq = 629,
  Instruction_kv3_v2_sbfx4uwd = 630,
  Instruction_kv3_v2_sbfx4w = 631,
  Instruction_kv3_v2_sbfx4wd = 632,
  Instruction_kv3_v2_sbfx4wp = 633,
  Instruction_kv3_v2_sbfx64d = 634,
  Instruction_kv3_v2_sbfx64uwd = 635,
  Instruction_kv3_v2_sbfx64w = 636,
  Instruction_kv3_v2_sbfx64wd = 637,
  Instruction_kv3_v2_sbfx8bo = 638,
  Instruction_kv3_v2_sbfx8d = 639,
  Instruction_kv3_v2_sbfx8hq = 640,
  Instruction_kv3_v2_sbfx8uwd = 641,
  Instruction_kv3_v2_sbfx8w = 642,
  Instruction_kv3_v2_sbfx8wd = 643,
  Instruction_kv3_v2_sbfx8wp = 644,
  Instruction_kv3_v2_sbmm8 = 645,
  Instruction_kv3_v2_sbmm8_abase = 646,
  Instruction_kv3_v2_sbmmt8 = 647,
  Instruction_kv3_v2_sbmmt8_abase = 648,
  Instruction_kv3_v2_scall = 649,
  Instruction_kv3_v2_sd = 650,
  Instruction_kv3_v2_set = 651,
  Instruction_kv3_v2_sh = 652,
  Instruction_kv3_v2_sleep = 653,
  Instruction_kv3_v2_sllbos = 654,
  Instruction_kv3_v2_slld = 655,
  Instruction_kv3_v2_sllhqs = 656,
  Instruction_kv3_v2_sllw = 657,
  Instruction_kv3_v2_sllwps = 658,
  Instruction_kv3_v2_slsbos = 659,
  Instruction_kv3_v2_slsd = 660,
  Instruction_kv3_v2_slshqs = 661,
  Instruction_kv3_v2_slsw = 662,
  Instruction_kv3_v2_slswps = 663,
  Instruction_kv3_v2_slusbos = 664,
  Instruction_kv3_v2_slusd = 665,
  Instruction_kv3_v2_slushqs = 666,
  Instruction_kv3_v2_slusw = 667,
  Instruction_kv3_v2_sluswps = 668,
  Instruction_kv3_v2_so = 669,
  Instruction_kv3_v2_sq = 670,
  Instruction_kv3_v2_srabos = 671,
  Instruction_kv3_v2_srad = 672,
  Instruction_kv3_v2_srahqs = 673,
  Instruction_kv3_v2_sraw = 674,
  Instruction_kv3_v2_srawps = 675,
  Instruction_kv3_v2_srlbos = 676,
  Instruction_kv3_v2_srld = 677,
  Instruction_kv3_v2_srlhqs = 678,
  Instruction_kv3_v2_srlw = 679,
  Instruction_kv3_v2_srlwps = 680,
  Instruction_kv3_v2_srsbos = 681,
  Instruction_kv3_v2_srsd = 682,
  Instruction_kv3_v2_srshqs = 683,
  Instruction_kv3_v2_srsw = 684,
  Instruction_kv3_v2_srswps = 685,
  Instruction_kv3_v2_stop = 686,
  Instruction_kv3_v2_stsud = 687,
  Instruction_kv3_v2_stsuhq = 688,
  Instruction_kv3_v2_stsuw = 689,
  Instruction_kv3_v2_stsuwp = 690,
  Instruction_kv3_v2_sw = 691,
  Instruction_kv3_v2_sxbd = 692,
  Instruction_kv3_v2_sxhd = 693,
  Instruction_kv3_v2_sxlbhq = 694,
  Instruction_kv3_v2_sxlhwp = 695,
  Instruction_kv3_v2_sxmbhq = 696,
  Instruction_kv3_v2_sxmhwp = 697,
  Instruction_kv3_v2_sxwd = 698,
  Instruction_kv3_v2_syncgroup = 699,
  Instruction_kv3_v2_tlbdinval = 700,
  Instruction_kv3_v2_tlbiinval = 701,
  Instruction_kv3_v2_tlbprobe = 702,
  Instruction_kv3_v2_tlbread = 703,
  Instruction_kv3_v2_tlbwrite = 704,
  Instruction_kv3_v2_waitit = 705,
  Instruction_kv3_v2_wfxl = 706,
  Instruction_kv3_v2_wfxm = 707,
  Instruction_kv3_v2_xaccesso = 708,
  Instruction_kv3_v2_xaligno = 709,
  Instruction_kv3_v2_xandno = 710,
  Instruction_kv3_v2_xando = 711,
  Instruction_kv3_v2_xclampwo = 712,
  Instruction_kv3_v2_xcopyo = 713,
  Instruction_kv3_v2_xcopyv = 714,
  Instruction_kv3_v2_xcopyx = 715,
  Instruction_kv3_v2_xffma44hw = 716,
  Instruction_kv3_v2_xfmaxhx = 717,
  Instruction_kv3_v2_xfminhx = 718,
  Instruction_kv3_v2_xfmma484hw = 719,
  Instruction_kv3_v2_xfnarrow44wh = 720,
  Instruction_kv3_v2_xfscalewo = 721,
  Instruction_kv3_v2_xlo = 722,
  Instruction_kv3_v2_xmadd44bw0 = 723,
  Instruction_kv3_v2_xmadd44bw1 = 724,
  Instruction_kv3_v2_xmaddifwo = 725,
  Instruction_kv3_v2_xmaddsu44bw0 = 726,
  Instruction_kv3_v2_xmaddsu44bw1 = 727,
  Instruction_kv3_v2_xmaddu44bw0 = 728,
  Instruction_kv3_v2_xmaddu44bw1 = 729,
  Instruction_kv3_v2_xmma4164bw = 730,
  Instruction_kv3_v2_xmma484bw = 731,
  Instruction_kv3_v2_xmmasu4164bw = 732,
  Instruction_kv3_v2_xmmasu484bw = 733,
  Instruction_kv3_v2_xmmau4164bw = 734,
  Instruction_kv3_v2_xmmau484bw = 735,
  Instruction_kv3_v2_xmmaus4164bw = 736,
  Instruction_kv3_v2_xmmaus484bw = 737,
  Instruction_kv3_v2_xmovefd = 738,
  Instruction_kv3_v2_xmovefo = 739,
  Instruction_kv3_v2_xmovefq = 740,
  Instruction_kv3_v2_xmovetd = 741,
  Instruction_kv3_v2_xmovetq = 742,
  Instruction_kv3_v2_xmsbfifwo = 743,
  Instruction_kv3_v2_xmt44d = 744,
  Instruction_kv3_v2_xnando = 745,
  Instruction_kv3_v2_xnoro = 746,
  Instruction_kv3_v2_xnxoro = 747,
  Instruction_kv3_v2_xord = 748,
  Instruction_kv3_v2_xord_abase = 749,
  Instruction_kv3_v2_xorno = 750,
  Instruction_kv3_v2_xoro = 751,
  Instruction_kv3_v2_xorrbod = 752,
  Instruction_kv3_v2_xorrhqd = 753,
  Instruction_kv3_v2_xorrwpd = 754,
  Instruction_kv3_v2_xorw = 755,
  Instruction_kv3_v2_xrecvo = 756,
  Instruction_kv3_v2_xsbmm8dq = 757,
  Instruction_kv3_v2_xsbmmt8dq = 758,
  Instruction_kv3_v2_xsendo = 759,
  Instruction_kv3_v2_xsendrecvo = 760,
  Instruction_kv3_v2_xso = 761,
  Instruction_kv3_v2_xsplatdo = 762,
  Instruction_kv3_v2_xsplatov = 763,
  Instruction_kv3_v2_xsplatox = 764,
  Instruction_kv3_v2_xsx48bw = 765,
  Instruction_kv3_v2_xtrunc48wb = 766,
  Instruction_kv3_v2_xxoro = 767,
  Instruction_kv3_v2_xzx48bw = 768,
  Instruction_kv3_v2_zxbd = 769,
  Instruction_kv3_v2_zxhd = 770,
  Instruction_kv3_v2_zxlbhq = 771,
  Instruction_kv3_v2_zxlhwp = 772,
  Instruction_kv3_v2_zxmbhq = 773,
  Instruction_kv3_v2_zxmhwp = 774,
  Instruction_kv3_v2_zxwd = 775,
  Separator_kv3_v2_comma = 776,
  Separator_kv3_v2_equal = 777,
  Separator_kv3_v2_qmark = 778,
  Separator_kv3_v2_rsbracket = 779,
  Separator_kv3_v2_lsbracket = 780
};

enum Modifier_kv3_v2_exunum_enum {
  Modifier_kv3_v2_exunum_ALU0=0,
  Modifier_kv3_v2_exunum_ALU1=1,
  Modifier_kv3_v2_exunum_MAU=2,
  Modifier_kv3_v2_exunum_LSU=3,
};

extern const char *mod_kv3_v2_exunum[];
extern const char *mod_kv3_v2_scalarcond[];
extern const char *mod_kv3_v2_lsomask[];
extern const char *mod_kv3_v2_lsumask[];
extern const char *mod_kv3_v2_lsupack[];
extern const char *mod_kv3_v2_simplecond[];
extern const char *mod_kv3_v2_comparison[];
extern const char *mod_kv3_v2_floatcomp[];
extern const char *mod_kv3_v2_rounding[];
extern const char *mod_kv3_v2_silent[];
extern const char *mod_kv3_v2_variant[];
extern const char *mod_kv3_v2_speculate[];
extern const char *mod_kv3_v2_doscale[];
extern const char *mod_kv3_v2_qindex[];
extern const char *mod_kv3_v2_hindex[];
extern const char *mod_kv3_v2_cachelev[];
extern const char *mod_kv3_v2_coherency[];
extern const char *mod_kv3_v2_boolcas[];
extern const char *mod_kv3_v2_accesses[];
extern const char *mod_kv3_v2_channel[];
extern const char *mod_kv3_v2_conjugate[];
extern const char *mod_kv3_v2_transpose[];
extern const char *mod_kv3_v2_shuffleV[];
extern const char *mod_kv3_v2_shuffleX[];
extern const char *mod_kv3_v2_splat32[];
typedef enum {
  Bundling_kv3_v2_ALL,
  Bundling_kv3_v2_BCU,
  Bundling_kv3_v2_TCA,
  Bundling_kv3_v2_FULL,
  Bundling_kv3_v2_FULL_X,
  Bundling_kv3_v2_FULL_Y,
  Bundling_kv3_v2_LITE,
  Bundling_kv3_v2_LITE_X,
  Bundling_kv3_v2_LITE_Y,
  Bundling_kv3_v2_MAU,
  Bundling_kv3_v2_MAU_X,
  Bundling_kv3_v2_MAU_Y,
  Bundling_kv3_v2_LSU,
  Bundling_kv3_v2_LSU_X,
  Bundling_kv3_v2_LSU_Y,
  Bundling_kv3_v2_TINY,
  Bundling_kv3_v2_TINY_X,
  Bundling_kv3_v2_TINY_Y,
  Bundling_kv3_v2_NOP,
} Bundling_kv3_v2;


static const char *bundling_kv3_v2_names(Bundling_kv3_v2 bundling) __attribute__((unused));
static const char *bundling_kv3_v2_names(Bundling_kv3_v2 bundling) {
  switch(bundling) {
  case Bundling_kv3_v2_ALL: return "Bundling_kv3_v2_ALL";
  case Bundling_kv3_v2_BCU: return "Bundling_kv3_v2_BCU";
  case Bundling_kv3_v2_TCA: return "Bundling_kv3_v2_TCA";
  case Bundling_kv3_v2_FULL: return "Bundling_kv3_v2_FULL";
  case Bundling_kv3_v2_FULL_X: return "Bundling_kv3_v2_FULL_X";
  case Bundling_kv3_v2_FULL_Y: return "Bundling_kv3_v2_FULL_Y";
  case Bundling_kv3_v2_LITE: return "Bundling_kv3_v2_LITE";
  case Bundling_kv3_v2_LITE_X: return "Bundling_kv3_v2_LITE_X";
  case Bundling_kv3_v2_LITE_Y: return "Bundling_kv3_v2_LITE_Y";
  case Bundling_kv3_v2_MAU: return "Bundling_kv3_v2_MAU";
  case Bundling_kv3_v2_MAU_X: return "Bundling_kv3_v2_MAU_X";
  case Bundling_kv3_v2_MAU_Y: return "Bundling_kv3_v2_MAU_Y";
  case Bundling_kv3_v2_LSU: return "Bundling_kv3_v2_LSU";
  case Bundling_kv3_v2_LSU_X: return "Bundling_kv3_v2_LSU_X";
  case Bundling_kv3_v2_LSU_Y: return "Bundling_kv3_v2_LSU_Y";
  case Bundling_kv3_v2_TINY: return "Bundling_kv3_v2_TINY";
  case Bundling_kv3_v2_TINY_X: return "Bundling_kv3_v2_TINY_X";
  case Bundling_kv3_v2_TINY_Y: return "Bundling_kv3_v2_TINY_Y";
  case Bundling_kv3_v2_NOP: return "Bundling_kv3_v2_NOP";
  };
  return "unknown bundling";
};

/* Resources list */
#define Resource_kv3_v2_ISSUE 0
#define Resource_kv3_v2_TINY 1
#define Resource_kv3_v2_LITE 2
#define Resource_kv3_v2_FULL 3
#define Resource_kv3_v2_LSU 4
#define Resource_kv3_v2_MAU 5
#define Resource_kv3_v2_BCU 6
#define Resource_kv3_v2_TCA 7
#define Resource_kv3_v2_AUXR 8
#define Resource_kv3_v2_AUXW 9
#define Resource_kv3_v2_CRRP 10
#define Resource_kv3_v2_CRWL 11
#define Resource_kv3_v2_CRWH 12
#define Resource_kv3_v2_NOP 13
#define kvx_kv3_v2_RESOURCE_MAX 14


/* Reservations list */
#define Reservation_kv3_v2_ALL 0
#define Reservation_kv3_v2_ALU_NOP 1
#define Reservation_kv3_v2_ALU_TINY 2
#define Reservation_kv3_v2_ALU_TINY_X 3
#define Reservation_kv3_v2_ALU_TINY_Y 4
#define Reservation_kv3_v2_ALU_TINY_CRRP 5
#define Reservation_kv3_v2_ALU_TINY_CRWL_CRWH 6
#define Reservation_kv3_v2_ALU_TINY_CRWL_CRWH_X 7
#define Reservation_kv3_v2_ALU_TINY_CRWL_CRWH_Y 8
#define Reservation_kv3_v2_ALU_TINY_CRRP_CRWL_CRWH 9
#define Reservation_kv3_v2_ALU_TINY_CRWL 10
#define Reservation_kv3_v2_ALU_TINY_CRWH 11
#define Reservation_kv3_v2_ALU_LITE 12
#define Reservation_kv3_v2_ALU_LITE_X 13
#define Reservation_kv3_v2_ALU_LITE_Y 14
#define Reservation_kv3_v2_ALU_LITE_CRWL 15
#define Reservation_kv3_v2_ALU_LITE_CRWH 16
#define Reservation_kv3_v2_ALU_FULL 17
#define Reservation_kv3_v2_ALU_FULL_X 18
#define Reservation_kv3_v2_ALU_FULL_Y 19
#define Reservation_kv3_v2_BCU 20
#define Reservation_kv3_v2_BCU_CRRP_CRWL_CRWH 21
#define Reservation_kv3_v2_BCU_TINY_AUXW_CRRP 22
#define Reservation_kv3_v2_BCU_TINY_TINY_MAU_XNOP 23
#define Reservation_kv3_v2_TCA 24
#define Reservation_kv3_v2_LSU 25
#define Reservation_kv3_v2_LSU_X 26
#define Reservation_kv3_v2_LSU_Y 27
#define Reservation_kv3_v2_LSU_CRRP 28
#define Reservation_kv3_v2_LSU_CRRP_X 29
#define Reservation_kv3_v2_LSU_CRRP_Y 30
#define Reservation_kv3_v2_LSU_AUXR 31
#define Reservation_kv3_v2_LSU_AUXR_X 32
#define Reservation_kv3_v2_LSU_AUXR_Y 33
#define Reservation_kv3_v2_LSU_AUXW 34
#define Reservation_kv3_v2_LSU_AUXW_X 35
#define Reservation_kv3_v2_LSU_AUXW_Y 36
#define Reservation_kv3_v2_LSU_AUXR_AUXW 37
#define Reservation_kv3_v2_LSU_AUXR_AUXW_X 38
#define Reservation_kv3_v2_LSU_AUXR_AUXW_Y 39
#define Reservation_kv3_v2_MAU 40
#define Reservation_kv3_v2_MAU_X 41
#define Reservation_kv3_v2_MAU_Y 42
#define Reservation_kv3_v2_MAU_AUXR 43
#define Reservation_kv3_v2_MAU_AUXR_X 44
#define Reservation_kv3_v2_MAU_AUXR_Y 45


extern struct kvx_reloc kv3_v2_rel16_reloc;
extern struct kvx_reloc kv3_v2_rel32_reloc;
extern struct kvx_reloc kv3_v2_rel64_reloc;
extern struct kvx_reloc kv3_v2_pcrel_signed16_reloc;
extern struct kvx_reloc kv3_v2_pcrel17_reloc;
extern struct kvx_reloc kv3_v2_pcrel27_reloc;
extern struct kvx_reloc kv3_v2_pcrel32_reloc;
extern struct kvx_reloc kv3_v2_pcrel_signed37_reloc;
extern struct kvx_reloc kv3_v2_pcrel_signed43_reloc;
extern struct kvx_reloc kv3_v2_pcrel_signed64_reloc;
extern struct kvx_reloc kv3_v2_pcrel64_reloc;
extern struct kvx_reloc kv3_v2_signed16_reloc;
extern struct kvx_reloc kv3_v2_signed32_reloc;
extern struct kvx_reloc kv3_v2_signed37_reloc;
extern struct kvx_reloc kv3_v2_gotoff_signed37_reloc;
extern struct kvx_reloc kv3_v2_gotoff_signed43_reloc;
extern struct kvx_reloc kv3_v2_gotoff_32_reloc;
extern struct kvx_reloc kv3_v2_gotoff_64_reloc;
extern struct kvx_reloc kv3_v2_got_32_reloc;
extern struct kvx_reloc kv3_v2_got_signed37_reloc;
extern struct kvx_reloc kv3_v2_got_signed43_reloc;
extern struct kvx_reloc kv3_v2_got_64_reloc;
extern struct kvx_reloc kv3_v2_glob_dat_reloc;
extern struct kvx_reloc kv3_v2_copy_reloc;
extern struct kvx_reloc kv3_v2_jump_slot_reloc;
extern struct kvx_reloc kv3_v2_relative_reloc;
extern struct kvx_reloc kv3_v2_signed43_reloc;
extern struct kvx_reloc kv3_v2_signed64_reloc;
extern struct kvx_reloc kv3_v2_gotaddr_signed37_reloc;
extern struct kvx_reloc kv3_v2_gotaddr_signed43_reloc;
extern struct kvx_reloc kv3_v2_gotaddr_signed64_reloc;
extern struct kvx_reloc kv3_v2_dtpmod64_reloc;
extern struct kvx_reloc kv3_v2_dtpoff64_reloc;
extern struct kvx_reloc kv3_v2_dtpoff_signed37_reloc;
extern struct kvx_reloc kv3_v2_dtpoff_signed43_reloc;
extern struct kvx_reloc kv3_v2_tlsgd_signed37_reloc;
extern struct kvx_reloc kv3_v2_tlsgd_signed43_reloc;
extern struct kvx_reloc kv3_v2_tlsld_signed37_reloc;
extern struct kvx_reloc kv3_v2_tlsld_signed43_reloc;
extern struct kvx_reloc kv3_v2_tpoff64_reloc;
extern struct kvx_reloc kv3_v2_tlsie_signed37_reloc;
extern struct kvx_reloc kv3_v2_tlsie_signed43_reloc;
extern struct kvx_reloc kv3_v2_tlsle_signed37_reloc;
extern struct kvx_reloc kv3_v2_tlsle_signed43_reloc;
extern struct kvx_reloc kv3_v2_rel8_reloc;

#define KVX_REGFILE_FIRST_GPR 0
#define KVX_REGFILE_LAST_GPR 1
#define KVX_REGFILE_DEC_GPR 2
#define KVX_REGFILE_FIRST_PGR 3
#define KVX_REGFILE_LAST_PGR 4
#define KVX_REGFILE_DEC_PGR 5
#define KVX_REGFILE_FIRST_QGR 6
#define KVX_REGFILE_LAST_QGR 7
#define KVX_REGFILE_DEC_QGR 8
#define KVX_REGFILE_FIRST_SFR 9
#define KVX_REGFILE_LAST_SFR 10
#define KVX_REGFILE_DEC_SFR 11
#define KVX_REGFILE_FIRST_X16R 12
#define KVX_REGFILE_LAST_X16R 13
#define KVX_REGFILE_DEC_X16R 14
#define KVX_REGFILE_FIRST_X2R 15
#define KVX_REGFILE_LAST_X2R 16
#define KVX_REGFILE_DEC_X2R 17
#define KVX_REGFILE_FIRST_X32R 18
#define KVX_REGFILE_LAST_X32R 19
#define KVX_REGFILE_DEC_X32R 20
#define KVX_REGFILE_FIRST_X4R 21
#define KVX_REGFILE_LAST_X4R 22
#define KVX_REGFILE_DEC_X4R 23
#define KVX_REGFILE_FIRST_X64R 24
#define KVX_REGFILE_LAST_X64R 25
#define KVX_REGFILE_DEC_X64R 26
#define KVX_REGFILE_FIRST_X8R 27
#define KVX_REGFILE_LAST_X8R 28
#define KVX_REGFILE_DEC_X8R 29
#define KVX_REGFILE_FIRST_XBR 30
#define KVX_REGFILE_LAST_XBR 31
#define KVX_REGFILE_DEC_XBR 32
#define KVX_REGFILE_FIRST_XCR 33
#define KVX_REGFILE_LAST_XCR 34
#define KVX_REGFILE_DEC_XCR 35
#define KVX_REGFILE_FIRST_XMR 36
#define KVX_REGFILE_LAST_XMR 37
#define KVX_REGFILE_DEC_XMR 38
#define KVX_REGFILE_FIRST_XTR 39
#define KVX_REGFILE_LAST_XTR 40
#define KVX_REGFILE_DEC_XTR 41
#define KVX_REGFILE_FIRST_XVR 42
#define KVX_REGFILE_LAST_XVR 43
#define KVX_REGFILE_DEC_XVR 44
#define KVX_REGFILE_REGISTERS 45
#define KVX_REGFILE_DEC_REGISTERS 46


extern int kvx_kv4_v1_regfiles[];
extern const char **kvx_kv4_v1_modifiers[];
extern struct kvx_Register kvx_kv4_v1_registers[];

extern int kvx_kv4_v1_dec_registers[];

enum Method_kvx_kv4_v1_enum {
  Immediate_kv4_v1_brknumber = 1,
  Immediate_kv4_v1_pcrel17 = 2,
  Immediate_kv4_v1_pcrel27 = 3,
  Immediate_kv4_v1_signed10 = 4,
  Immediate_kv4_v1_signed16 = 5,
  Immediate_kv4_v1_signed27 = 6,
  Immediate_kv4_v1_signed37 = 7,
  Immediate_kv4_v1_signed43 = 8,
  Immediate_kv4_v1_signed54 = 9,
  Immediate_kv4_v1_sysnumber = 10,
  Immediate_kv4_v1_unsigned6 = 11,
  Immediate_kv4_v1_wrapped32 = 12,
  Immediate_kv4_v1_wrapped64 = 13,
  Immediate_kv4_v1_wrapped8 = 14,
  Modifier_kv4_v1_accesses = 15,
  Modifier_kv4_v1_boolcas = 16,
  Modifier_kv4_v1_cachelev = 17,
  Modifier_kv4_v1_channel = 18,
  Modifier_kv4_v1_coherency = 19,
  Modifier_kv4_v1_comparison = 20,
  Modifier_kv4_v1_conjugate = 21,
  Modifier_kv4_v1_doscale = 22,
  Modifier_kv4_v1_exunum = 23,
  Modifier_kv4_v1_floatcomp = 24,
  Modifier_kv4_v1_hindex = 25,
  Modifier_kv4_v1_lsomask = 26,
  Modifier_kv4_v1_lsumask = 27,
  Modifier_kv4_v1_lsupack = 28,
  Modifier_kv4_v1_qindex = 29,
  Modifier_kv4_v1_rounding = 30,
  Modifier_kv4_v1_scalarcond = 31,
  Modifier_kv4_v1_shuffleV = 32,
  Modifier_kv4_v1_shuffleX = 33,
  Modifier_kv4_v1_silent = 34,
  Modifier_kv4_v1_simplecond = 35,
  Modifier_kv4_v1_speculate = 36,
  Modifier_kv4_v1_splat32 = 37,
  Modifier_kv4_v1_transpose = 38,
  Modifier_kv4_v1_variant = 39,
  RegClass_kv4_v1_aloneReg = 40,
  RegClass_kv4_v1_blockReg = 41,
  RegClass_kv4_v1_blockRegE = 42,
  RegClass_kv4_v1_blockRegO = 43,
  RegClass_kv4_v1_blockReg_0 = 44,
  RegClass_kv4_v1_blockReg_1 = 45,
  RegClass_kv4_v1_buffer16Reg = 46,
  RegClass_kv4_v1_buffer2Reg = 47,
  RegClass_kv4_v1_buffer32Reg = 48,
  RegClass_kv4_v1_buffer4Reg = 49,
  RegClass_kv4_v1_buffer64Reg = 50,
  RegClass_kv4_v1_buffer8Reg = 51,
  RegClass_kv4_v1_coproReg = 52,
  RegClass_kv4_v1_coproReg0M4 = 53,
  RegClass_kv4_v1_coproReg1M4 = 54,
  RegClass_kv4_v1_coproReg2M4 = 55,
  RegClass_kv4_v1_coproReg3M4 = 56,
  RegClass_kv4_v1_matrixReg = 57,
  RegClass_kv4_v1_matrixReg_0 = 58,
  RegClass_kv4_v1_matrixReg_1 = 59,
  RegClass_kv4_v1_matrixReg_2 = 60,
  RegClass_kv4_v1_matrixReg_3 = 61,
  RegClass_kv4_v1_onlyfxReg = 62,
  RegClass_kv4_v1_onlygetReg = 63,
  RegClass_kv4_v1_onlyraReg = 64,
  RegClass_kv4_v1_onlysetReg = 65,
  RegClass_kv4_v1_onlyswapReg = 66,
  RegClass_kv4_v1_pairedReg = 67,
  RegClass_kv4_v1_pairedReg_0 = 68,
  RegClass_kv4_v1_pairedReg_1 = 69,
  RegClass_kv4_v1_quadReg = 70,
  RegClass_kv4_v1_quadReg_0 = 71,
  RegClass_kv4_v1_quadReg_1 = 72,
  RegClass_kv4_v1_quadReg_2 = 73,
  RegClass_kv4_v1_quadReg_3 = 74,
  RegClass_kv4_v1_singleReg = 75,
  RegClass_kv4_v1_systemReg = 76,
  RegClass_kv4_v1_tileReg = 77,
  RegClass_kv4_v1_tileReg_0 = 78,
  RegClass_kv4_v1_tileReg_1 = 79,
  RegClass_kv4_v1_vectorReg = 80,
  RegClass_kv4_v1_vectorReg_0 = 81,
  RegClass_kv4_v1_vectorReg_1 = 82,
  RegClass_kv4_v1_vectorReg_2 = 83,
  RegClass_kv4_v1_vectorReg_3 = 84,
  Instruction_kv4_v1_abdbo = 85,
  Instruction_kv4_v1_abdd = 86,
  Instruction_kv4_v1_abdd_abase = 87,
  Instruction_kv4_v1_abdhq = 88,
  Instruction_kv4_v1_abdsbo = 89,
  Instruction_kv4_v1_abdsd = 90,
  Instruction_kv4_v1_abdshq = 91,
  Instruction_kv4_v1_abdsw = 92,
  Instruction_kv4_v1_abdswp = 93,
  Instruction_kv4_v1_abdubo = 94,
  Instruction_kv4_v1_abdud = 95,
  Instruction_kv4_v1_abduhq = 96,
  Instruction_kv4_v1_abduw = 97,
  Instruction_kv4_v1_abduwp = 98,
  Instruction_kv4_v1_abdw = 99,
  Instruction_kv4_v1_abdwp = 100,
  Instruction_kv4_v1_absbo = 101,
  Instruction_kv4_v1_absd = 102,
  Instruction_kv4_v1_abshq = 103,
  Instruction_kv4_v1_abssbo = 104,
  Instruction_kv4_v1_abssd = 105,
  Instruction_kv4_v1_absshq = 106,
  Instruction_kv4_v1_abssw = 107,
  Instruction_kv4_v1_absswp = 108,
  Instruction_kv4_v1_absw = 109,
  Instruction_kv4_v1_abswp = 110,
  Instruction_kv4_v1_acswapd = 111,
  Instruction_kv4_v1_acswapq = 112,
  Instruction_kv4_v1_acswapw = 113,
  Instruction_kv4_v1_addbo = 114,
  Instruction_kv4_v1_addcd = 115,
  Instruction_kv4_v1_addcd_i = 116,
  Instruction_kv4_v1_addd = 117,
  Instruction_kv4_v1_addd_abase = 118,
  Instruction_kv4_v1_addhq = 119,
  Instruction_kv4_v1_addrbod = 120,
  Instruction_kv4_v1_addrhqd = 121,
  Instruction_kv4_v1_addrwpd = 122,
  Instruction_kv4_v1_addsbo = 123,
  Instruction_kv4_v1_addsd = 124,
  Instruction_kv4_v1_addshq = 125,
  Instruction_kv4_v1_addsw = 126,
  Instruction_kv4_v1_addswp = 127,
  Instruction_kv4_v1_addurbod = 128,
  Instruction_kv4_v1_addurhqd = 129,
  Instruction_kv4_v1_addurwpd = 130,
  Instruction_kv4_v1_addusbo = 131,
  Instruction_kv4_v1_addusd = 132,
  Instruction_kv4_v1_addushq = 133,
  Instruction_kv4_v1_addusw = 134,
  Instruction_kv4_v1_adduswp = 135,
  Instruction_kv4_v1_adduwd = 136,
  Instruction_kv4_v1_addw = 137,
  Instruction_kv4_v1_addwd = 138,
  Instruction_kv4_v1_addwp = 139,
  Instruction_kv4_v1_addx16bo = 140,
  Instruction_kv4_v1_addx16d = 141,
  Instruction_kv4_v1_addx16hq = 142,
  Instruction_kv4_v1_addx16uwd = 143,
  Instruction_kv4_v1_addx16w = 144,
  Instruction_kv4_v1_addx16wd = 145,
  Instruction_kv4_v1_addx16wp = 146,
  Instruction_kv4_v1_addx2bo = 147,
  Instruction_kv4_v1_addx2d = 148,
  Instruction_kv4_v1_addx2hq = 149,
  Instruction_kv4_v1_addx2uwd = 150,
  Instruction_kv4_v1_addx2w = 151,
  Instruction_kv4_v1_addx2wd = 152,
  Instruction_kv4_v1_addx2wp = 153,
  Instruction_kv4_v1_addx32d = 154,
  Instruction_kv4_v1_addx32uwd = 155,
  Instruction_kv4_v1_addx32w = 156,
  Instruction_kv4_v1_addx32wd = 157,
  Instruction_kv4_v1_addx4bo = 158,
  Instruction_kv4_v1_addx4d = 159,
  Instruction_kv4_v1_addx4hq = 160,
  Instruction_kv4_v1_addx4uwd = 161,
  Instruction_kv4_v1_addx4w = 162,
  Instruction_kv4_v1_addx4wd = 163,
  Instruction_kv4_v1_addx4wp = 164,
  Instruction_kv4_v1_addx64d = 165,
  Instruction_kv4_v1_addx64uwd = 166,
  Instruction_kv4_v1_addx64w = 167,
  Instruction_kv4_v1_addx64wd = 168,
  Instruction_kv4_v1_addx8bo = 169,
  Instruction_kv4_v1_addx8d = 170,
  Instruction_kv4_v1_addx8hq = 171,
  Instruction_kv4_v1_addx8uwd = 172,
  Instruction_kv4_v1_addx8w = 173,
  Instruction_kv4_v1_addx8wd = 174,
  Instruction_kv4_v1_addx8wp = 175,
  Instruction_kv4_v1_aladdd = 176,
  Instruction_kv4_v1_aladdw = 177,
  Instruction_kv4_v1_alclrd = 178,
  Instruction_kv4_v1_alclrw = 179,
  Instruction_kv4_v1_ald = 180,
  Instruction_kv4_v1_alw = 181,
  Instruction_kv4_v1_andd = 182,
  Instruction_kv4_v1_andd_abase = 183,
  Instruction_kv4_v1_andnd = 184,
  Instruction_kv4_v1_andnd_abase = 185,
  Instruction_kv4_v1_andnw = 186,
  Instruction_kv4_v1_andrbod = 187,
  Instruction_kv4_v1_andrhqd = 188,
  Instruction_kv4_v1_andrwpd = 189,
  Instruction_kv4_v1_andw = 190,
  Instruction_kv4_v1_asd = 191,
  Instruction_kv4_v1_asw = 192,
  Instruction_kv4_v1_avgbo = 193,
  Instruction_kv4_v1_avghq = 194,
  Instruction_kv4_v1_avgrbo = 195,
  Instruction_kv4_v1_avgrhq = 196,
  Instruction_kv4_v1_avgrubo = 197,
  Instruction_kv4_v1_avgruhq = 198,
  Instruction_kv4_v1_avgruw = 199,
  Instruction_kv4_v1_avgruwp = 200,
  Instruction_kv4_v1_avgrw = 201,
  Instruction_kv4_v1_avgrwp = 202,
  Instruction_kv4_v1_avgubo = 203,
  Instruction_kv4_v1_avguhq = 204,
  Instruction_kv4_v1_avguw = 205,
  Instruction_kv4_v1_avguwp = 206,
  Instruction_kv4_v1_avgw = 207,
  Instruction_kv4_v1_avgwp = 208,
  Instruction_kv4_v1_await = 209,
  Instruction_kv4_v1_barrier = 210,
  Instruction_kv4_v1_break = 211,
  Instruction_kv4_v1_call = 212,
  Instruction_kv4_v1_cb = 213,
  Instruction_kv4_v1_cbsd = 214,
  Instruction_kv4_v1_cbsw = 215,
  Instruction_kv4_v1_cbswp = 216,
  Instruction_kv4_v1_clrf = 217,
  Instruction_kv4_v1_clsd = 218,
  Instruction_kv4_v1_clsw = 219,
  Instruction_kv4_v1_clswp = 220,
  Instruction_kv4_v1_clzd = 221,
  Instruction_kv4_v1_clzw = 222,
  Instruction_kv4_v1_clzwp = 223,
  Instruction_kv4_v1_cmovebo = 224,
  Instruction_kv4_v1_cmoved = 225,
  Instruction_kv4_v1_cmovehq = 226,
  Instruction_kv4_v1_cmovewp = 227,
  Instruction_kv4_v1_cmuldt = 228,
  Instruction_kv4_v1_cmulghxdt = 229,
  Instruction_kv4_v1_cmulglxdt = 230,
  Instruction_kv4_v1_cmulgmxdt = 231,
  Instruction_kv4_v1_cmulxdt = 232,
  Instruction_kv4_v1_compd = 233,
  Instruction_kv4_v1_compnbo = 234,
  Instruction_kv4_v1_compnd = 235,
  Instruction_kv4_v1_compnhq = 236,
  Instruction_kv4_v1_compnw = 237,
  Instruction_kv4_v1_compnwp = 238,
  Instruction_kv4_v1_compuwd = 239,
  Instruction_kv4_v1_compw = 240,
  Instruction_kv4_v1_compwd = 241,
  Instruction_kv4_v1_copyd = 242,
  Instruction_kv4_v1_copyo = 243,
  Instruction_kv4_v1_copyq = 244,
  Instruction_kv4_v1_copyw = 245,
  Instruction_kv4_v1_crcbellw = 246,
  Instruction_kv4_v1_crcbelmw = 247,
  Instruction_kv4_v1_crclellw = 248,
  Instruction_kv4_v1_crclelmw = 249,
  Instruction_kv4_v1_ctzd = 250,
  Instruction_kv4_v1_ctzw = 251,
  Instruction_kv4_v1_ctzwp = 252,
  Instruction_kv4_v1_d1inval = 253,
  Instruction_kv4_v1_dflushl = 254,
  Instruction_kv4_v1_dflushsw = 255,
  Instruction_kv4_v1_dinvall = 256,
  Instruction_kv4_v1_dinvalsw = 257,
  Instruction_kv4_v1_dot2suwd = 258,
  Instruction_kv4_v1_dot2suwdp = 259,
  Instruction_kv4_v1_dot2uwd = 260,
  Instruction_kv4_v1_dot2uwdp = 261,
  Instruction_kv4_v1_dot2w = 262,
  Instruction_kv4_v1_dot2wd = 263,
  Instruction_kv4_v1_dot2wdp = 264,
  Instruction_kv4_v1_dot2wzp = 265,
  Instruction_kv4_v1_dpurgel = 266,
  Instruction_kv4_v1_dpurgesw = 267,
  Instruction_kv4_v1_dtouchl = 268,
  Instruction_kv4_v1_errop = 269,
  Instruction_kv4_v1_extfs = 270,
  Instruction_kv4_v1_extfz = 271,
  Instruction_kv4_v1_fabsd = 272,
  Instruction_kv4_v1_fabshq = 273,
  Instruction_kv4_v1_fabsw = 274,
  Instruction_kv4_v1_fabswp = 275,
  Instruction_kv4_v1_faddd = 276,
  Instruction_kv4_v1_fadddc = 277,
  Instruction_kv4_v1_fadddc_c = 278,
  Instruction_kv4_v1_fadddp = 279,
  Instruction_kv4_v1_faddho = 280,
  Instruction_kv4_v1_faddhq = 281,
  Instruction_kv4_v1_faddw = 282,
  Instruction_kv4_v1_faddwc = 283,
  Instruction_kv4_v1_faddwc_c = 284,
  Instruction_kv4_v1_faddwcp = 285,
  Instruction_kv4_v1_faddwcp_c = 286,
  Instruction_kv4_v1_faddwp = 287,
  Instruction_kv4_v1_faddwq = 288,
  Instruction_kv4_v1_fcdivd = 289,
  Instruction_kv4_v1_fcdivw = 290,
  Instruction_kv4_v1_fcdivwp = 291,
  Instruction_kv4_v1_fcompd = 292,
  Instruction_kv4_v1_fcompnd = 293,
  Instruction_kv4_v1_fcompnhq = 294,
  Instruction_kv4_v1_fcompnw = 295,
  Instruction_kv4_v1_fcompnwp = 296,
  Instruction_kv4_v1_fcompw = 297,
  Instruction_kv4_v1_fdot2w = 298,
  Instruction_kv4_v1_fdot2wd = 299,
  Instruction_kv4_v1_fdot2wdp = 300,
  Instruction_kv4_v1_fdot2wzp = 301,
  Instruction_kv4_v1_fence = 302,
  Instruction_kv4_v1_ffdmasw = 303,
  Instruction_kv4_v1_ffdmaswp = 304,
  Instruction_kv4_v1_ffdmaswq = 305,
  Instruction_kv4_v1_ffdmaw = 306,
  Instruction_kv4_v1_ffdmawp = 307,
  Instruction_kv4_v1_ffdmawq = 308,
  Instruction_kv4_v1_ffdmdaw = 309,
  Instruction_kv4_v1_ffdmdawp = 310,
  Instruction_kv4_v1_ffdmdawq = 311,
  Instruction_kv4_v1_ffdmdsw = 312,
  Instruction_kv4_v1_ffdmdswp = 313,
  Instruction_kv4_v1_ffdmdswq = 314,
  Instruction_kv4_v1_ffdmsaw = 315,
  Instruction_kv4_v1_ffdmsawp = 316,
  Instruction_kv4_v1_ffdmsawq = 317,
  Instruction_kv4_v1_ffdmsw = 318,
  Instruction_kv4_v1_ffdmswp = 319,
  Instruction_kv4_v1_ffdmswq = 320,
  Instruction_kv4_v1_ffmad = 321,
  Instruction_kv4_v1_ffmaho = 322,
  Instruction_kv4_v1_ffmahq = 323,
  Instruction_kv4_v1_ffmahw = 324,
  Instruction_kv4_v1_ffmahwq = 325,
  Instruction_kv4_v1_ffmaw = 326,
  Instruction_kv4_v1_ffmawc = 327,
  Instruction_kv4_v1_ffmawcp = 328,
  Instruction_kv4_v1_ffmawd = 329,
  Instruction_kv4_v1_ffmawdp = 330,
  Instruction_kv4_v1_ffmawp = 331,
  Instruction_kv4_v1_ffmawq = 332,
  Instruction_kv4_v1_ffmsd = 333,
  Instruction_kv4_v1_ffmsho = 334,
  Instruction_kv4_v1_ffmshq = 335,
  Instruction_kv4_v1_ffmshw = 336,
  Instruction_kv4_v1_ffmshwq = 337,
  Instruction_kv4_v1_ffmsw = 338,
  Instruction_kv4_v1_ffmswc = 339,
  Instruction_kv4_v1_ffmswcp = 340,
  Instruction_kv4_v1_ffmswd = 341,
  Instruction_kv4_v1_ffmswdp = 342,
  Instruction_kv4_v1_ffmswp = 343,
  Instruction_kv4_v1_ffmswq = 344,
  Instruction_kv4_v1_fixedd = 345,
  Instruction_kv4_v1_fixedud = 346,
  Instruction_kv4_v1_fixeduw = 347,
  Instruction_kv4_v1_fixeduwp = 348,
  Instruction_kv4_v1_fixedw = 349,
  Instruction_kv4_v1_fixedwp = 350,
  Instruction_kv4_v1_floatd = 351,
  Instruction_kv4_v1_floatud = 352,
  Instruction_kv4_v1_floatuw = 353,
  Instruction_kv4_v1_floatuwp = 354,
  Instruction_kv4_v1_floatw = 355,
  Instruction_kv4_v1_floatwp = 356,
  Instruction_kv4_v1_fmaxd = 357,
  Instruction_kv4_v1_fmaxhq = 358,
  Instruction_kv4_v1_fmaxw = 359,
  Instruction_kv4_v1_fmaxwp = 360,
  Instruction_kv4_v1_fmind = 361,
  Instruction_kv4_v1_fminhq = 362,
  Instruction_kv4_v1_fminw = 363,
  Instruction_kv4_v1_fminwp = 364,
  Instruction_kv4_v1_fmm212w = 365,
  Instruction_kv4_v1_fmm222w = 366,
  Instruction_kv4_v1_fmma212w = 367,
  Instruction_kv4_v1_fmma222w = 368,
  Instruction_kv4_v1_fmms212w = 369,
  Instruction_kv4_v1_fmms222w = 370,
  Instruction_kv4_v1_fmuld = 371,
  Instruction_kv4_v1_fmulho = 372,
  Instruction_kv4_v1_fmulhq = 373,
  Instruction_kv4_v1_fmulhw = 374,
  Instruction_kv4_v1_fmulhwq = 375,
  Instruction_kv4_v1_fmulw = 376,
  Instruction_kv4_v1_fmulwc = 377,
  Instruction_kv4_v1_fmulwcp = 378,
  Instruction_kv4_v1_fmulwd = 379,
  Instruction_kv4_v1_fmulwdp = 380,
  Instruction_kv4_v1_fmulwp = 381,
  Instruction_kv4_v1_fmulwq = 382,
  Instruction_kv4_v1_fnarrowdw = 383,
  Instruction_kv4_v1_fnarrowdwp = 384,
  Instruction_kv4_v1_fnarrowwh = 385,
  Instruction_kv4_v1_fnarrowwhq = 386,
  Instruction_kv4_v1_fnegd = 387,
  Instruction_kv4_v1_fneghq = 388,
  Instruction_kv4_v1_fnegw = 389,
  Instruction_kv4_v1_fnegwp = 390,
  Instruction_kv4_v1_frecw = 391,
  Instruction_kv4_v1_frsrw = 392,
  Instruction_kv4_v1_fsbfd = 393,
  Instruction_kv4_v1_fsbfdc = 394,
  Instruction_kv4_v1_fsbfdc_c = 395,
  Instruction_kv4_v1_fsbfdp = 396,
  Instruction_kv4_v1_fsbfho = 397,
  Instruction_kv4_v1_fsbfhq = 398,
  Instruction_kv4_v1_fsbfw = 399,
  Instruction_kv4_v1_fsbfwc = 400,
  Instruction_kv4_v1_fsbfwc_c = 401,
  Instruction_kv4_v1_fsbfwcp = 402,
  Instruction_kv4_v1_fsbfwcp_c = 403,
  Instruction_kv4_v1_fsbfwp = 404,
  Instruction_kv4_v1_fsbfwq = 405,
  Instruction_kv4_v1_fsdivd = 406,
  Instruction_kv4_v1_fsdivw = 407,
  Instruction_kv4_v1_fsdivwp = 408,
  Instruction_kv4_v1_fsrecd = 409,
  Instruction_kv4_v1_fsrecw = 410,
  Instruction_kv4_v1_fsrecwp = 411,
  Instruction_kv4_v1_fsrsrd = 412,
  Instruction_kv4_v1_fsrsrw = 413,
  Instruction_kv4_v1_fsrsrwp = 414,
  Instruction_kv4_v1_fwidenlhw = 415,
  Instruction_kv4_v1_fwidenlhwp = 416,
  Instruction_kv4_v1_fwidenlwd = 417,
  Instruction_kv4_v1_fwidenmhw = 418,
  Instruction_kv4_v1_fwidenmhwp = 419,
  Instruction_kv4_v1_fwidenmwd = 420,
  Instruction_kv4_v1_get = 421,
  Instruction_kv4_v1_goto = 422,
  Instruction_kv4_v1_i1inval = 423,
  Instruction_kv4_v1_i1invals = 424,
  Instruction_kv4_v1_icall = 425,
  Instruction_kv4_v1_iget = 426,
  Instruction_kv4_v1_igoto = 427,
  Instruction_kv4_v1_insf = 428,
  Instruction_kv4_v1_landd = 429,
  Instruction_kv4_v1_landw = 430,
  Instruction_kv4_v1_lbs = 431,
  Instruction_kv4_v1_lbz = 432,
  Instruction_kv4_v1_ld = 433,
  Instruction_kv4_v1_lhs = 434,
  Instruction_kv4_v1_lhz = 435,
  Instruction_kv4_v1_lnandd = 436,
  Instruction_kv4_v1_lnandw = 437,
  Instruction_kv4_v1_lnord = 438,
  Instruction_kv4_v1_lnorw = 439,
  Instruction_kv4_v1_lo = 440,
  Instruction_kv4_v1_loopdo = 441,
  Instruction_kv4_v1_lord = 442,
  Instruction_kv4_v1_lorw = 443,
  Instruction_kv4_v1_lq = 444,
  Instruction_kv4_v1_lws = 445,
  Instruction_kv4_v1_lwz = 446,
  Instruction_kv4_v1_maddd = 447,
  Instruction_kv4_v1_madddt = 448,
  Instruction_kv4_v1_maddhq = 449,
  Instruction_kv4_v1_maddhwq = 450,
  Instruction_kv4_v1_maddmwq = 451,
  Instruction_kv4_v1_maddsudt = 452,
  Instruction_kv4_v1_maddsuhwq = 453,
  Instruction_kv4_v1_maddsumwq = 454,
  Instruction_kv4_v1_maddsuwd = 455,
  Instruction_kv4_v1_maddsuwdp = 456,
  Instruction_kv4_v1_maddudt = 457,
  Instruction_kv4_v1_madduhwq = 458,
  Instruction_kv4_v1_maddumwq = 459,
  Instruction_kv4_v1_madduwd = 460,
  Instruction_kv4_v1_madduwdp = 461,
  Instruction_kv4_v1_madduzdt = 462,
  Instruction_kv4_v1_maddw = 463,
  Instruction_kv4_v1_maddwd = 464,
  Instruction_kv4_v1_maddwdp = 465,
  Instruction_kv4_v1_maddwp = 466,
  Instruction_kv4_v1_maddwq = 467,
  Instruction_kv4_v1_make = 468,
  Instruction_kv4_v1_maxbo = 469,
  Instruction_kv4_v1_maxd = 470,
  Instruction_kv4_v1_maxd_abase = 471,
  Instruction_kv4_v1_maxhq = 472,
  Instruction_kv4_v1_maxrbod = 473,
  Instruction_kv4_v1_maxrhqd = 474,
  Instruction_kv4_v1_maxrwpd = 475,
  Instruction_kv4_v1_maxubo = 476,
  Instruction_kv4_v1_maxud = 477,
  Instruction_kv4_v1_maxud_abase = 478,
  Instruction_kv4_v1_maxuhq = 479,
  Instruction_kv4_v1_maxurbod = 480,
  Instruction_kv4_v1_maxurhqd = 481,
  Instruction_kv4_v1_maxurwpd = 482,
  Instruction_kv4_v1_maxuw = 483,
  Instruction_kv4_v1_maxuwp = 484,
  Instruction_kv4_v1_maxw = 485,
  Instruction_kv4_v1_maxwp = 486,
  Instruction_kv4_v1_minbo = 487,
  Instruction_kv4_v1_mind = 488,
  Instruction_kv4_v1_mind_abase = 489,
  Instruction_kv4_v1_minhq = 490,
  Instruction_kv4_v1_minrbod = 491,
  Instruction_kv4_v1_minrhqd = 492,
  Instruction_kv4_v1_minrwpd = 493,
  Instruction_kv4_v1_minubo = 494,
  Instruction_kv4_v1_minud = 495,
  Instruction_kv4_v1_minud_abase = 496,
  Instruction_kv4_v1_minuhq = 497,
  Instruction_kv4_v1_minurbod = 498,
  Instruction_kv4_v1_minurhqd = 499,
  Instruction_kv4_v1_minurwpd = 500,
  Instruction_kv4_v1_minuw = 501,
  Instruction_kv4_v1_minuwp = 502,
  Instruction_kv4_v1_minw = 503,
  Instruction_kv4_v1_minwp = 504,
  Instruction_kv4_v1_mm212w = 505,
  Instruction_kv4_v1_mma212w = 506,
  Instruction_kv4_v1_mms212w = 507,
  Instruction_kv4_v1_msbfd = 508,
  Instruction_kv4_v1_msbfdt = 509,
  Instruction_kv4_v1_msbfhq = 510,
  Instruction_kv4_v1_msbfhwq = 511,
  Instruction_kv4_v1_msbfmwq = 512,
  Instruction_kv4_v1_msbfsudt = 513,
  Instruction_kv4_v1_msbfsuhwq = 514,
  Instruction_kv4_v1_msbfsumwq = 515,
  Instruction_kv4_v1_msbfsuwd = 516,
  Instruction_kv4_v1_msbfsuwdp = 517,
  Instruction_kv4_v1_msbfudt = 518,
  Instruction_kv4_v1_msbfuhwq = 519,
  Instruction_kv4_v1_msbfumwq = 520,
  Instruction_kv4_v1_msbfuwd = 521,
  Instruction_kv4_v1_msbfuwdp = 522,
  Instruction_kv4_v1_msbfuzdt = 523,
  Instruction_kv4_v1_msbfw = 524,
  Instruction_kv4_v1_msbfwd = 525,
  Instruction_kv4_v1_msbfwdp = 526,
  Instruction_kv4_v1_msbfwp = 527,
  Instruction_kv4_v1_msbfwq = 528,
  Instruction_kv4_v1_muld = 529,
  Instruction_kv4_v1_muldt = 530,
  Instruction_kv4_v1_mulhq = 531,
  Instruction_kv4_v1_mulhwq = 532,
  Instruction_kv4_v1_mulmwq = 533,
  Instruction_kv4_v1_mulsudt = 534,
  Instruction_kv4_v1_mulsuhwq = 535,
  Instruction_kv4_v1_mulsumwq = 536,
  Instruction_kv4_v1_mulsuwd = 537,
  Instruction_kv4_v1_mulsuwdp = 538,
  Instruction_kv4_v1_muludt = 539,
  Instruction_kv4_v1_muluhwq = 540,
  Instruction_kv4_v1_mulumwq = 541,
  Instruction_kv4_v1_muluwd = 542,
  Instruction_kv4_v1_muluwdp = 543,
  Instruction_kv4_v1_mulw = 544,
  Instruction_kv4_v1_mulwd = 545,
  Instruction_kv4_v1_mulwdp = 546,
  Instruction_kv4_v1_mulwp = 547,
  Instruction_kv4_v1_mulwq = 548,
  Instruction_kv4_v1_nandd = 549,
  Instruction_kv4_v1_nandd_abase = 550,
  Instruction_kv4_v1_nandw = 551,
  Instruction_kv4_v1_negbo = 552,
  Instruction_kv4_v1_negd = 553,
  Instruction_kv4_v1_neghq = 554,
  Instruction_kv4_v1_negsbo = 555,
  Instruction_kv4_v1_negsd = 556,
  Instruction_kv4_v1_negshq = 557,
  Instruction_kv4_v1_negsw = 558,
  Instruction_kv4_v1_negswp = 559,
  Instruction_kv4_v1_negw = 560,
  Instruction_kv4_v1_negwp = 561,
  Instruction_kv4_v1_nop = 562,
  Instruction_kv4_v1_nord = 563,
  Instruction_kv4_v1_nord_abase = 564,
  Instruction_kv4_v1_norw = 565,
  Instruction_kv4_v1_notd = 566,
  Instruction_kv4_v1_notw = 567,
  Instruction_kv4_v1_nxord = 568,
  Instruction_kv4_v1_nxord_abase = 569,
  Instruction_kv4_v1_nxorw = 570,
  Instruction_kv4_v1_ord = 571,
  Instruction_kv4_v1_ord_abase = 572,
  Instruction_kv4_v1_ornd = 573,
  Instruction_kv4_v1_ornd_abase = 574,
  Instruction_kv4_v1_ornw = 575,
  Instruction_kv4_v1_orrbod = 576,
  Instruction_kv4_v1_orrhqd = 577,
  Instruction_kv4_v1_orrwpd = 578,
  Instruction_kv4_v1_orw = 579,
  Instruction_kv4_v1_pcrel = 580,
  Instruction_kv4_v1_ret = 581,
  Instruction_kv4_v1_rfe = 582,
  Instruction_kv4_v1_rolw = 583,
  Instruction_kv4_v1_rolwps = 584,
  Instruction_kv4_v1_rorw = 585,
  Instruction_kv4_v1_rorwps = 586,
  Instruction_kv4_v1_rswap = 587,
  Instruction_kv4_v1_sb = 588,
  Instruction_kv4_v1_sbfbo = 589,
  Instruction_kv4_v1_sbfcd = 590,
  Instruction_kv4_v1_sbfcd_i = 591,
  Instruction_kv4_v1_sbfd = 592,
  Instruction_kv4_v1_sbfd_abase = 593,
  Instruction_kv4_v1_sbfhq = 594,
  Instruction_kv4_v1_sbfsbo = 595,
  Instruction_kv4_v1_sbfsd = 596,
  Instruction_kv4_v1_sbfshq = 597,
  Instruction_kv4_v1_sbfsw = 598,
  Instruction_kv4_v1_sbfswp = 599,
  Instruction_kv4_v1_sbfusbo = 600,
  Instruction_kv4_v1_sbfusd = 601,
  Instruction_kv4_v1_sbfushq = 602,
  Instruction_kv4_v1_sbfusw = 603,
  Instruction_kv4_v1_sbfuswp = 604,
  Instruction_kv4_v1_sbfuwd = 605,
  Instruction_kv4_v1_sbfw = 606,
  Instruction_kv4_v1_sbfwd = 607,
  Instruction_kv4_v1_sbfwp = 608,
  Instruction_kv4_v1_sbfx16bo = 609,
  Instruction_kv4_v1_sbfx16d = 610,
  Instruction_kv4_v1_sbfx16hq = 611,
  Instruction_kv4_v1_sbfx16uwd = 612,
  Instruction_kv4_v1_sbfx16w = 613,
  Instruction_kv4_v1_sbfx16wd = 614,
  Instruction_kv4_v1_sbfx16wp = 615,
  Instruction_kv4_v1_sbfx2bo = 616,
  Instruction_kv4_v1_sbfx2d = 617,
  Instruction_kv4_v1_sbfx2hq = 618,
  Instruction_kv4_v1_sbfx2uwd = 619,
  Instruction_kv4_v1_sbfx2w = 620,
  Instruction_kv4_v1_sbfx2wd = 621,
  Instruction_kv4_v1_sbfx2wp = 622,
  Instruction_kv4_v1_sbfx32d = 623,
  Instruction_kv4_v1_sbfx32uwd = 624,
  Instruction_kv4_v1_sbfx32w = 625,
  Instruction_kv4_v1_sbfx32wd = 626,
  Instruction_kv4_v1_sbfx4bo = 627,
  Instruction_kv4_v1_sbfx4d = 628,
  Instruction_kv4_v1_sbfx4hq = 629,
  Instruction_kv4_v1_sbfx4uwd = 630,
  Instruction_kv4_v1_sbfx4w = 631,
  Instruction_kv4_v1_sbfx4wd = 632,
  Instruction_kv4_v1_sbfx4wp = 633,
  Instruction_kv4_v1_sbfx64d = 634,
  Instruction_kv4_v1_sbfx64uwd = 635,
  Instruction_kv4_v1_sbfx64w = 636,
  Instruction_kv4_v1_sbfx64wd = 637,
  Instruction_kv4_v1_sbfx8bo = 638,
  Instruction_kv4_v1_sbfx8d = 639,
  Instruction_kv4_v1_sbfx8hq = 640,
  Instruction_kv4_v1_sbfx8uwd = 641,
  Instruction_kv4_v1_sbfx8w = 642,
  Instruction_kv4_v1_sbfx8wd = 643,
  Instruction_kv4_v1_sbfx8wp = 644,
  Instruction_kv4_v1_sbmm8 = 645,
  Instruction_kv4_v1_sbmm8_abase = 646,
  Instruction_kv4_v1_sbmmt8 = 647,
  Instruction_kv4_v1_sbmmt8_abase = 648,
  Instruction_kv4_v1_scall = 649,
  Instruction_kv4_v1_sd = 650,
  Instruction_kv4_v1_set = 651,
  Instruction_kv4_v1_sh = 652,
  Instruction_kv4_v1_sleep = 653,
  Instruction_kv4_v1_sllbos = 654,
  Instruction_kv4_v1_slld = 655,
  Instruction_kv4_v1_sllhqs = 656,
  Instruction_kv4_v1_sllw = 657,
  Instruction_kv4_v1_sllwps = 658,
  Instruction_kv4_v1_slsbos = 659,
  Instruction_kv4_v1_slsd = 660,
  Instruction_kv4_v1_slshqs = 661,
  Instruction_kv4_v1_slsw = 662,
  Instruction_kv4_v1_slswps = 663,
  Instruction_kv4_v1_slusbos = 664,
  Instruction_kv4_v1_slusd = 665,
  Instruction_kv4_v1_slushqs = 666,
  Instruction_kv4_v1_slusw = 667,
  Instruction_kv4_v1_sluswps = 668,
  Instruction_kv4_v1_so = 669,
  Instruction_kv4_v1_sq = 670,
  Instruction_kv4_v1_srabos = 671,
  Instruction_kv4_v1_srad = 672,
  Instruction_kv4_v1_srahqs = 673,
  Instruction_kv4_v1_sraw = 674,
  Instruction_kv4_v1_srawps = 675,
  Instruction_kv4_v1_srlbos = 676,
  Instruction_kv4_v1_srld = 677,
  Instruction_kv4_v1_srlhqs = 678,
  Instruction_kv4_v1_srlw = 679,
  Instruction_kv4_v1_srlwps = 680,
  Instruction_kv4_v1_srsbos = 681,
  Instruction_kv4_v1_srsd = 682,
  Instruction_kv4_v1_srshqs = 683,
  Instruction_kv4_v1_srsw = 684,
  Instruction_kv4_v1_srswps = 685,
  Instruction_kv4_v1_stop = 686,
  Instruction_kv4_v1_stsud = 687,
  Instruction_kv4_v1_stsuhq = 688,
  Instruction_kv4_v1_stsuw = 689,
  Instruction_kv4_v1_stsuwp = 690,
  Instruction_kv4_v1_sw = 691,
  Instruction_kv4_v1_sxbd = 692,
  Instruction_kv4_v1_sxhd = 693,
  Instruction_kv4_v1_sxlbhq = 694,
  Instruction_kv4_v1_sxlhwp = 695,
  Instruction_kv4_v1_sxmbhq = 696,
  Instruction_kv4_v1_sxmhwp = 697,
  Instruction_kv4_v1_sxwd = 698,
  Instruction_kv4_v1_syncgroup = 699,
  Instruction_kv4_v1_tlbdinval = 700,
  Instruction_kv4_v1_tlbiinval = 701,
  Instruction_kv4_v1_tlbprobe = 702,
  Instruction_kv4_v1_tlbread = 703,
  Instruction_kv4_v1_tlbwrite = 704,
  Instruction_kv4_v1_waitit = 705,
  Instruction_kv4_v1_wfxl = 706,
  Instruction_kv4_v1_wfxm = 707,
  Instruction_kv4_v1_xaccesso = 708,
  Instruction_kv4_v1_xaligno = 709,
  Instruction_kv4_v1_xandno = 710,
  Instruction_kv4_v1_xando = 711,
  Instruction_kv4_v1_xclampwo = 712,
  Instruction_kv4_v1_xcopyo = 713,
  Instruction_kv4_v1_xcopyv = 714,
  Instruction_kv4_v1_xcopyx = 715,
  Instruction_kv4_v1_xffma44hw = 716,
  Instruction_kv4_v1_xfmaxhx = 717,
  Instruction_kv4_v1_xfminhx = 718,
  Instruction_kv4_v1_xfmma484hw = 719,
  Instruction_kv4_v1_xfnarrow44wh = 720,
  Instruction_kv4_v1_xfscalewo = 721,
  Instruction_kv4_v1_xlo = 722,
  Instruction_kv4_v1_xmadd44bw0 = 723,
  Instruction_kv4_v1_xmadd44bw1 = 724,
  Instruction_kv4_v1_xmaddifwo = 725,
  Instruction_kv4_v1_xmaddsu44bw0 = 726,
  Instruction_kv4_v1_xmaddsu44bw1 = 727,
  Instruction_kv4_v1_xmaddu44bw0 = 728,
  Instruction_kv4_v1_xmaddu44bw1 = 729,
  Instruction_kv4_v1_xmma4164bw = 730,
  Instruction_kv4_v1_xmma484bw = 731,
  Instruction_kv4_v1_xmmasu4164bw = 732,
  Instruction_kv4_v1_xmmasu484bw = 733,
  Instruction_kv4_v1_xmmau4164bw = 734,
  Instruction_kv4_v1_xmmau484bw = 735,
  Instruction_kv4_v1_xmmaus4164bw = 736,
  Instruction_kv4_v1_xmmaus484bw = 737,
  Instruction_kv4_v1_xmovefd = 738,
  Instruction_kv4_v1_xmovefo = 739,
  Instruction_kv4_v1_xmovefq = 740,
  Instruction_kv4_v1_xmovetd = 741,
  Instruction_kv4_v1_xmovetq = 742,
  Instruction_kv4_v1_xmsbfifwo = 743,
  Instruction_kv4_v1_xmt44d = 744,
  Instruction_kv4_v1_xnando = 745,
  Instruction_kv4_v1_xnoro = 746,
  Instruction_kv4_v1_xnxoro = 747,
  Instruction_kv4_v1_xord = 748,
  Instruction_kv4_v1_xord_abase = 749,
  Instruction_kv4_v1_xorno = 750,
  Instruction_kv4_v1_xoro = 751,
  Instruction_kv4_v1_xorrbod = 752,
  Instruction_kv4_v1_xorrhqd = 753,
  Instruction_kv4_v1_xorrwpd = 754,
  Instruction_kv4_v1_xorw = 755,
  Instruction_kv4_v1_xrecvo = 756,
  Instruction_kv4_v1_xsbmm8dq = 757,
  Instruction_kv4_v1_xsbmmt8dq = 758,
  Instruction_kv4_v1_xsendo = 759,
  Instruction_kv4_v1_xsendrecvo = 760,
  Instruction_kv4_v1_xso = 761,
  Instruction_kv4_v1_xsplatdo = 762,
  Instruction_kv4_v1_xsplatov = 763,
  Instruction_kv4_v1_xsplatox = 764,
  Instruction_kv4_v1_xsx48bw = 765,
  Instruction_kv4_v1_xtrunc48wb = 766,
  Instruction_kv4_v1_xxoro = 767,
  Instruction_kv4_v1_xzx48bw = 768,
  Instruction_kv4_v1_zxbd = 769,
  Instruction_kv4_v1_zxhd = 770,
  Instruction_kv4_v1_zxlbhq = 771,
  Instruction_kv4_v1_zxlhwp = 772,
  Instruction_kv4_v1_zxmbhq = 773,
  Instruction_kv4_v1_zxmhwp = 774,
  Instruction_kv4_v1_zxwd = 775,
  Separator_kv4_v1_comma = 776,
  Separator_kv4_v1_equal = 777,
  Separator_kv4_v1_qmark = 778,
  Separator_kv4_v1_rsbracket = 779,
  Separator_kv4_v1_lsbracket = 780
};

enum Modifier_kv4_v1_exunum_enum {
  Modifier_kv4_v1_exunum_ALU0=0,
  Modifier_kv4_v1_exunum_ALU1=1,
  Modifier_kv4_v1_exunum_MAU=2,
  Modifier_kv4_v1_exunum_LSU=3,
};

extern const char *mod_kv4_v1_exunum[];
extern const char *mod_kv4_v1_scalarcond[];
extern const char *mod_kv4_v1_lsomask[];
extern const char *mod_kv4_v1_lsumask[];
extern const char *mod_kv4_v1_lsupack[];
extern const char *mod_kv4_v1_simplecond[];
extern const char *mod_kv4_v1_comparison[];
extern const char *mod_kv4_v1_floatcomp[];
extern const char *mod_kv4_v1_rounding[];
extern const char *mod_kv4_v1_silent[];
extern const char *mod_kv4_v1_variant[];
extern const char *mod_kv4_v1_speculate[];
extern const char *mod_kv4_v1_doscale[];
extern const char *mod_kv4_v1_qindex[];
extern const char *mod_kv4_v1_hindex[];
extern const char *mod_kv4_v1_cachelev[];
extern const char *mod_kv4_v1_coherency[];
extern const char *mod_kv4_v1_boolcas[];
extern const char *mod_kv4_v1_accesses[];
extern const char *mod_kv4_v1_channel[];
extern const char *mod_kv4_v1_conjugate[];
extern const char *mod_kv4_v1_transpose[];
extern const char *mod_kv4_v1_shuffleV[];
extern const char *mod_kv4_v1_shuffleX[];
extern const char *mod_kv4_v1_splat32[];
typedef enum {
  Bundling_kv4_v1_ALL,
  Bundling_kv4_v1_BCU,
  Bundling_kv4_v1_TCA,
  Bundling_kv4_v1_FULL,
  Bundling_kv4_v1_FULL_X,
  Bundling_kv4_v1_FULL_Y,
  Bundling_kv4_v1_LITE,
  Bundling_kv4_v1_LITE_X,
  Bundling_kv4_v1_LITE_Y,
  Bundling_kv4_v1_MAU,
  Bundling_kv4_v1_MAU_X,
  Bundling_kv4_v1_MAU_Y,
  Bundling_kv4_v1_LSU,
  Bundling_kv4_v1_LSU_X,
  Bundling_kv4_v1_LSU_Y,
  Bundling_kv4_v1_TINY,
  Bundling_kv4_v1_TINY_X,
  Bundling_kv4_v1_TINY_Y,
  Bundling_kv4_v1_NOP,
} Bundling_kv4_v1;


static const char *bundling_kv4_v1_names(Bundling_kv4_v1 bundling) __attribute__((unused));
static const char *bundling_kv4_v1_names(Bundling_kv4_v1 bundling) {
  switch(bundling) {
  case Bundling_kv4_v1_ALL: return "Bundling_kv4_v1_ALL";
  case Bundling_kv4_v1_BCU: return "Bundling_kv4_v1_BCU";
  case Bundling_kv4_v1_TCA: return "Bundling_kv4_v1_TCA";
  case Bundling_kv4_v1_FULL: return "Bundling_kv4_v1_FULL";
  case Bundling_kv4_v1_FULL_X: return "Bundling_kv4_v1_FULL_X";
  case Bundling_kv4_v1_FULL_Y: return "Bundling_kv4_v1_FULL_Y";
  case Bundling_kv4_v1_LITE: return "Bundling_kv4_v1_LITE";
  case Bundling_kv4_v1_LITE_X: return "Bundling_kv4_v1_LITE_X";
  case Bundling_kv4_v1_LITE_Y: return "Bundling_kv4_v1_LITE_Y";
  case Bundling_kv4_v1_MAU: return "Bundling_kv4_v1_MAU";
  case Bundling_kv4_v1_MAU_X: return "Bundling_kv4_v1_MAU_X";
  case Bundling_kv4_v1_MAU_Y: return "Bundling_kv4_v1_MAU_Y";
  case Bundling_kv4_v1_LSU: return "Bundling_kv4_v1_LSU";
  case Bundling_kv4_v1_LSU_X: return "Bundling_kv4_v1_LSU_X";
  case Bundling_kv4_v1_LSU_Y: return "Bundling_kv4_v1_LSU_Y";
  case Bundling_kv4_v1_TINY: return "Bundling_kv4_v1_TINY";
  case Bundling_kv4_v1_TINY_X: return "Bundling_kv4_v1_TINY_X";
  case Bundling_kv4_v1_TINY_Y: return "Bundling_kv4_v1_TINY_Y";
  case Bundling_kv4_v1_NOP: return "Bundling_kv4_v1_NOP";
  };
  return "unknown bundling";
};

/* Resources list */
#define Resource_kv4_v1_ISSUE 0
#define Resource_kv4_v1_TINY 1
#define Resource_kv4_v1_LITE 2
#define Resource_kv4_v1_FULL 3
#define Resource_kv4_v1_LSU 4
#define Resource_kv4_v1_MAU 5
#define Resource_kv4_v1_BCU 6
#define Resource_kv4_v1_TCA 7
#define Resource_kv4_v1_AUXR 8
#define Resource_kv4_v1_AUXW 9
#define Resource_kv4_v1_CRRP 10
#define Resource_kv4_v1_CRWL 11
#define Resource_kv4_v1_CRWH 12
#define Resource_kv4_v1_NOP 13
#define kvx_kv4_v1_RESOURCE_MAX 14


/* Reservations list */
#define Reservation_kv4_v1_ALL 0
#define Reservation_kv4_v1_ALU_NOP 1
#define Reservation_kv4_v1_ALU_TINY 2
#define Reservation_kv4_v1_ALU_TINY_X 3
#define Reservation_kv4_v1_ALU_TINY_Y 4
#define Reservation_kv4_v1_ALU_TINY_CRRP 5
#define Reservation_kv4_v1_ALU_TINY_CRWL_CRWH 6
#define Reservation_kv4_v1_ALU_TINY_CRWL_CRWH_X 7
#define Reservation_kv4_v1_ALU_TINY_CRWL_CRWH_Y 8
#define Reservation_kv4_v1_ALU_TINY_CRRP_CRWL_CRWH 9
#define Reservation_kv4_v1_ALU_TINY_CRWL 10
#define Reservation_kv4_v1_ALU_TINY_CRWH 11
#define Reservation_kv4_v1_ALU_LITE 12
#define Reservation_kv4_v1_ALU_LITE_X 13
#define Reservation_kv4_v1_ALU_LITE_Y 14
#define Reservation_kv4_v1_ALU_LITE_CRWL 15
#define Reservation_kv4_v1_ALU_LITE_CRWH 16
#define Reservation_kv4_v1_ALU_FULL 17
#define Reservation_kv4_v1_ALU_FULL_X 18
#define Reservation_kv4_v1_ALU_FULL_Y 19
#define Reservation_kv4_v1_BCU 20
#define Reservation_kv4_v1_BCU_CRRP_CRWL_CRWH 21
#define Reservation_kv4_v1_BCU_TINY_AUXW_CRRP 22
#define Reservation_kv4_v1_BCU_TINY_TINY_MAU_XNOP 23
#define Reservation_kv4_v1_TCA 24
#define Reservation_kv4_v1_LSU 25
#define Reservation_kv4_v1_LSU_X 26
#define Reservation_kv4_v1_LSU_Y 27
#define Reservation_kv4_v1_LSU_CRRP 28
#define Reservation_kv4_v1_LSU_CRRP_X 29
#define Reservation_kv4_v1_LSU_CRRP_Y 30
#define Reservation_kv4_v1_LSU_AUXR 31
#define Reservation_kv4_v1_LSU_AUXR_X 32
#define Reservation_kv4_v1_LSU_AUXR_Y 33
#define Reservation_kv4_v1_LSU_AUXW 34
#define Reservation_kv4_v1_LSU_AUXW_X 35
#define Reservation_kv4_v1_LSU_AUXW_Y 36
#define Reservation_kv4_v1_LSU_AUXR_AUXW 37
#define Reservation_kv4_v1_LSU_AUXR_AUXW_X 38
#define Reservation_kv4_v1_LSU_AUXR_AUXW_Y 39
#define Reservation_kv4_v1_MAU 40
#define Reservation_kv4_v1_MAU_X 41
#define Reservation_kv4_v1_MAU_Y 42
#define Reservation_kv4_v1_MAU_AUXR 43
#define Reservation_kv4_v1_MAU_AUXR_X 44
#define Reservation_kv4_v1_MAU_AUXR_Y 45


extern struct kvx_reloc kv4_v1_rel16_reloc;
extern struct kvx_reloc kv4_v1_rel32_reloc;
extern struct kvx_reloc kv4_v1_rel64_reloc;
extern struct kvx_reloc kv4_v1_pcrel_signed16_reloc;
extern struct kvx_reloc kv4_v1_pcrel17_reloc;
extern struct kvx_reloc kv4_v1_pcrel27_reloc;
extern struct kvx_reloc kv4_v1_pcrel32_reloc;
extern struct kvx_reloc kv4_v1_pcrel_signed37_reloc;
extern struct kvx_reloc kv4_v1_pcrel_signed43_reloc;
extern struct kvx_reloc kv4_v1_pcrel_signed64_reloc;
extern struct kvx_reloc kv4_v1_pcrel64_reloc;
extern struct kvx_reloc kv4_v1_signed16_reloc;
extern struct kvx_reloc kv4_v1_signed32_reloc;
extern struct kvx_reloc kv4_v1_signed37_reloc;
extern struct kvx_reloc kv4_v1_gotoff_signed37_reloc;
extern struct kvx_reloc kv4_v1_gotoff_signed43_reloc;
extern struct kvx_reloc kv4_v1_gotoff_32_reloc;
extern struct kvx_reloc kv4_v1_gotoff_64_reloc;
extern struct kvx_reloc kv4_v1_got_32_reloc;
extern struct kvx_reloc kv4_v1_got_signed37_reloc;
extern struct kvx_reloc kv4_v1_got_signed43_reloc;
extern struct kvx_reloc kv4_v1_got_64_reloc;
extern struct kvx_reloc kv4_v1_glob_dat_reloc;
extern struct kvx_reloc kv4_v1_copy_reloc;
extern struct kvx_reloc kv4_v1_jump_slot_reloc;
extern struct kvx_reloc kv4_v1_relative_reloc;
extern struct kvx_reloc kv4_v1_signed43_reloc;
extern struct kvx_reloc kv4_v1_signed64_reloc;
extern struct kvx_reloc kv4_v1_gotaddr_signed37_reloc;
extern struct kvx_reloc kv4_v1_gotaddr_signed43_reloc;
extern struct kvx_reloc kv4_v1_gotaddr_signed64_reloc;
extern struct kvx_reloc kv4_v1_dtpmod64_reloc;
extern struct kvx_reloc kv4_v1_dtpoff64_reloc;
extern struct kvx_reloc kv4_v1_dtpoff_signed37_reloc;
extern struct kvx_reloc kv4_v1_dtpoff_signed43_reloc;
extern struct kvx_reloc kv4_v1_tlsgd_signed37_reloc;
extern struct kvx_reloc kv4_v1_tlsgd_signed43_reloc;
extern struct kvx_reloc kv4_v1_tlsld_signed37_reloc;
extern struct kvx_reloc kv4_v1_tlsld_signed43_reloc;
extern struct kvx_reloc kv4_v1_tpoff64_reloc;
extern struct kvx_reloc kv4_v1_tlsie_signed37_reloc;
extern struct kvx_reloc kv4_v1_tlsie_signed43_reloc;
extern struct kvx_reloc kv4_v1_tlsle_signed37_reloc;
extern struct kvx_reloc kv4_v1_tlsle_signed43_reloc;
extern struct kvx_reloc kv4_v1_rel8_reloc;


#endif /* OPCODE_KVX_H */
