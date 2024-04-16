/* aarch64-opc.c -- AArch64 opcode support.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>

#include "opintl.h"
#include "libiberty.h"

#include "aarch64-opc.h"

#ifdef DEBUG_AARCH64
int debug_dump = false;
#endif /* DEBUG_AARCH64 */

/* The enumeration strings associated with each value of a 5-bit SVE
   pattern operand.  A null entry indicates a reserved meaning.  */
const char *const aarch64_sve_pattern_array[32] = {
  /* 0-7.  */
  "pow2",
  "vl1",
  "vl2",
  "vl3",
  "vl4",
  "vl5",
  "vl6",
  "vl7",
  /* 8-15.  */
  "vl8",
  "vl16",
  "vl32",
  "vl64",
  "vl128",
  "vl256",
  0,
  0,
  /* 16-23.  */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  /* 24-31.  */
  0,
  0,
  0,
  0,
  0,
  "mul4",
  "mul3",
  "all"
};

/* The enumeration strings associated with each value of a 4-bit SVE
   prefetch operand.  A null entry indicates a reserved meaning.  */
const char *const aarch64_sve_prfop_array[16] = {
  /* 0-7.  */
  "pldl1keep",
  "pldl1strm",
  "pldl2keep",
  "pldl2strm",
  "pldl3keep",
  "pldl3strm",
  0,
  0,
  /* 8-15.  */
  "pstl1keep",
  "pstl1strm",
  "pstl2keep",
  "pstl2strm",
  "pstl3keep",
  "pstl3strm",
  0,
  0
};

/* The enumeration strings associated with each value of a 6-bit RPRFM
   operation.  */
const char *const aarch64_rprfmop_array[64] = {
  "pldkeep",
  "pstkeep",
  0,
  0,
  "pldstrm",
  "pststrm"
};

/* Vector length multiples for a predicate-as-counter operand.  Used in things
   like AARCH64_OPND_SME_VLxN_10.  */
const char *const aarch64_sme_vlxn_array[2] = {
  "vlx2",
  "vlx4"
};

/* Helper functions to determine which operand to be used to encode/decode
   the size:Q fields for AdvSIMD instructions.  */

static inline bool
vector_qualifier_p (enum aarch64_opnd_qualifier qualifier)
{
  return (qualifier >= AARCH64_OPND_QLF_V_8B
	  && qualifier <= AARCH64_OPND_QLF_V_1Q);
}

static inline bool
fp_qualifier_p (enum aarch64_opnd_qualifier qualifier)
{
  return (qualifier >= AARCH64_OPND_QLF_S_B
	  && qualifier <= AARCH64_OPND_QLF_S_Q);
}

enum data_pattern
{
  DP_UNKNOWN,
  DP_VECTOR_3SAME,
  DP_VECTOR_LONG,
  DP_VECTOR_WIDE,
  DP_VECTOR_ACROSS_LANES,
};

static const char significant_operand_index [] =
{
  0,	/* DP_UNKNOWN, by default using operand 0.  */
  0,	/* DP_VECTOR_3SAME */
  1,	/* DP_VECTOR_LONG */
  2,	/* DP_VECTOR_WIDE */
  1,	/* DP_VECTOR_ACROSS_LANES */
};

/* Given a sequence of qualifiers in QUALIFIERS, determine and return
   the data pattern.
   N.B. QUALIFIERS is a possible sequence of qualifiers each of which
   corresponds to one of a sequence of operands.  */

static enum data_pattern
get_data_pattern (const aarch64_opnd_qualifier_seq_t qualifiers)
{
  if (vector_qualifier_p (qualifiers[0]))
    {
      /* e.g. v.4s, v.4s, v.4s
	   or v.4h, v.4h, v.h[3].  */
      if (qualifiers[0] == qualifiers[1]
	  && vector_qualifier_p (qualifiers[2])
	  && (aarch64_get_qualifier_esize (qualifiers[0])
	      == aarch64_get_qualifier_esize (qualifiers[1]))
	  && (aarch64_get_qualifier_esize (qualifiers[0])
	      == aarch64_get_qualifier_esize (qualifiers[2])))
	return DP_VECTOR_3SAME;
      /* e.g. v.8h, v.8b, v.8b.
           or v.4s, v.4h, v.h[2].
	   or v.8h, v.16b.  */
      if (vector_qualifier_p (qualifiers[1])
	  && aarch64_get_qualifier_esize (qualifiers[0]) != 0
	  && (aarch64_get_qualifier_esize (qualifiers[0])
	      == aarch64_get_qualifier_esize (qualifiers[1]) << 1))
	return DP_VECTOR_LONG;
      /* e.g. v.8h, v.8h, v.8b.  */
      if (qualifiers[0] == qualifiers[1]
	  && vector_qualifier_p (qualifiers[2])
	  && aarch64_get_qualifier_esize (qualifiers[0]) != 0
	  && (aarch64_get_qualifier_esize (qualifiers[0])
	      == aarch64_get_qualifier_esize (qualifiers[2]) << 1)
	  && (aarch64_get_qualifier_esize (qualifiers[0])
	      == aarch64_get_qualifier_esize (qualifiers[1])))
	return DP_VECTOR_WIDE;
    }
  else if (fp_qualifier_p (qualifiers[0]))
    {
      /* e.g. SADDLV <V><d>, <Vn>.<T>.  */
      if (vector_qualifier_p (qualifiers[1])
	  && qualifiers[2] == AARCH64_OPND_QLF_NIL)
	return DP_VECTOR_ACROSS_LANES;
    }

  return DP_UNKNOWN;
}

/* Select the operand to do the encoding/decoding of the 'size:Q' fields in
   the AdvSIMD instructions.  */
/* N.B. it is possible to do some optimization that doesn't call
   get_data_pattern each time when we need to select an operand.  We can
   either buffer the caculated the result or statically generate the data,
   however, it is not obvious that the optimization will bring significant
   benefit.  */

int
aarch64_select_operand_for_sizeq_field_coding (const aarch64_opcode *opcode)
{
  return
    significant_operand_index [get_data_pattern (opcode->qualifiers_list[0])];
}

/* Instruction bit-fields.
+   Keep synced with 'enum aarch64_field_kind'.  */
const aarch64_field fields[] =
{
    {  0,  0 },	/* NIL.  */
    {  8,  4 },	/* CRm: in the system instructions.  */
    { 10,  2 }, /* CRm_dsb_nxs: 2-bit imm. encoded in CRm<3:2>.  */
    { 12,  4 },	/* CRn: in the system instructions.  */
    { 10,  8 }, /* CSSC_imm8.  */
    { 11,  1 },	/* H: in advsimd scalar x indexed element instructions.  */
    { 21,  1 },	/* L: in advsimd scalar x indexed element instructions.  */
    {  0,  5 },	/* LSE128_Rt: Shared input+output operand register.  */
    { 16,  5 },	/* LSE128_Rt2: Shared input+output operand register 2.  */
    { 20,  1 },	/* M: in advsimd scalar x indexed element instructions.  */
    { 22,  1 },	/* N: in logical (immediate) instructions.  */
    { 30,  1 },	/* Q: in most AdvSIMD instructions.  */
    { 10,  5 },	/* Ra: in fp instructions.  */
    {  0,  5 },	/* Rd: in many integer instructions.  */
    { 16,  5 },	/* Rm: in ld/st reg offset and some integer inst.  */
    {  5,  5 },	/* Rn: in many integer instructions.  */
    { 16,  5 },	/* Rs: in load/store exclusive instructions.  */
    {  0,  5 },	/* Rt: in load/store instructions.  */
    { 10,  5 },	/* Rt2: in load/store pair instructions.  */
    { 12,  1 },	/* S: in load/store reg offset instructions.  */
    { 12,  2 }, /* SM3_imm2: Indexed element SM3 2 bits index immediate.  */
    {  1,  3 }, /* SME_Pdx2: predicate register, multiple of 2, [3:1].  */
    { 13,  3 }, /* SME_Pm: second source scalable predicate register P0-P7.  */
    {  0,  3 }, /* SME_PNd3: PN0-PN7, bits [2:0].  */
    {  5,  3 }, /* SME_PNn3: PN0-PN7, bits [7:5].  */
    { 16,  1 }, /* SME_Q: Q class bit, bit 16.  */
    { 16,  2 }, /* SME_Rm: index base register W12-W15 [17:16].  */
    { 13,  2 }, /* SME_Rv: vector select register W12-W15, bits [14:13].  */
    { 15,  1 }, /* SME_V: (horizontal / vertical tiles), bit 15.  */
    { 10,  1 }, /* SME_VL_10: VLx2 or VLx4, bit [10].  */
    { 13,  1 }, /* SME_VL_13: VLx2 or VLx4, bit [13].  */
    {  0,  2 }, /* SME_ZAda_2b: tile ZA0-ZA3.  */
    {  0,  3 }, /* SME_ZAda_3b: tile ZA0-ZA7.  */
    {  1,  4 }, /* SME_Zdn2: Z0-Z31, multiple of 2, bits [4:1].  */
    {  2,  3 }, /* SME_Zdn4: Z0-Z31, multiple of 4, bits [4:2].  */
    { 16,  4 }, /* SME_Zm: Z0-Z15, bits [19:16].  */
    { 17,  4 }, /* SME_Zm2: Z0-Z31, multiple of 2, bits [20:17].  */
    { 18,  3 }, /* SME_Zm4: Z0-Z31, multiple of 4, bits [20:18].  */
    {  6,  4 }, /* SME_Zn2: Z0-Z31, multiple of 2, bits [9:6].  */
    {  7,  3 }, /* SME_Zn4: Z0-Z31, multiple of 4, bits [9:7].  */
    {  4,  1 }, /* SME_ZtT: upper bit of Zt, bit [4].  */
    {  0,  3 }, /* SME_Zt3: lower 3 bits of Zt, bits [2:0].  */
    {  0,  2 }, /* SME_Zt2: lower 2 bits of Zt, bits [1:0].  */
    { 23,  1 }, /* SME_i1: immediate field, bit 23.  */
    { 12,  2 }, /* SME_size_12: bits [13:12].  */
    { 22,  2 }, /* SME_size_22: size<1>, size<0> class field, [23:22].  */
    { 23,  1 }, /* SME_sz_23: bit [23].  */
    { 22,  1 }, /* SME_tszh: immediate and qualifier field, bit 22.  */
    { 18,  3 }, /* SME_tszl: immediate and qualifier field, bits [20:18].  */
    { 0,   8 }, /* SME_zero_mask: list of up to 8 tile names separated by commas [7:0].  */
    {  4,  1 }, /* SVE_M_4: Merge/zero select, bit 4.  */
    { 14,  1 }, /* SVE_M_14: Merge/zero select, bit 14.  */
    { 16,  1 }, /* SVE_M_16: Merge/zero select, bit 16.  */
    { 17,  1 }, /* SVE_N: SVE equivalent of N.  */
    {  0,  4 }, /* SVE_Pd: p0-p15, bits [3,0].  */
    { 10,  3 }, /* SVE_Pg3: p0-p7, bits [12,10].  */
    {  5,  4 }, /* SVE_Pg4_5: p0-p15, bits [8,5].  */
    { 10,  4 }, /* SVE_Pg4_10: p0-p15, bits [13,10].  */
    { 16,  4 }, /* SVE_Pg4_16: p0-p15, bits [19,16].  */
    { 16,  4 }, /* SVE_Pm: p0-p15, bits [19,16].  */
    {  5,  4 }, /* SVE_Pn: p0-p15, bits [8,5].  */
    {  0,  4 }, /* SVE_Pt: p0-p15, bits [3,0].  */
    {  5,  5 }, /* SVE_Rm: SVE alternative position for Rm.  */
    { 16,  5 }, /* SVE_Rn: SVE alternative position for Rn.  */
    {  0,  5 }, /* SVE_Vd: Scalar SIMD&FP register, bits [4,0].  */
    {  5,  5 }, /* SVE_Vm: Scalar SIMD&FP register, bits [9,5].  */
    {  5,  5 }, /* SVE_Vn: Scalar SIMD&FP register, bits [9,5].  */
    {  5,  5 }, /* SVE_Za_5: SVE vector register, bits [9,5].  */
    { 16,  5 }, /* SVE_Za_16: SVE vector register, bits [20,16].  */
    {  0,  5 }, /* SVE_Zd: SVE vector register. bits [4,0].  */
    {  5,  5 }, /* SVE_Zm_5: SVE vector register, bits [9,5].  */
    { 16,  5 }, /* SVE_Zm_16: SVE vector register, bits [20,16]. */
    {  5,  5 }, /* SVE_Zn: SVE vector register, bits [9,5].  */
    {  0,  5 }, /* SVE_Zt: SVE vector register, bits [4,0].  */
    {  5,  1 }, /* SVE_i1: single-bit immediate.  */
    { 20,  1 }, /* SVE_i2h: high bit of 2bit immediate, bits.  */
    { 22,  1 }, /* SVE_i3h: high bit of 3-bit immediate.  */
    { 19,  2 }, /* SVE_i3h2: two high bits of 3bit immediate, bits [20,19].  */
    { 11,  1 }, /* SVE_i3l: low bit of 3-bit immediate.  */
    { 16,  3 }, /* SVE_imm3: 3-bit immediate field.  */
    { 16,  4 }, /* SVE_imm4: 4-bit immediate field.  */
    {  5,  5 }, /* SVE_imm5: 5-bit immediate field.  */
    { 16,  5 }, /* SVE_imm5b: secondary 5-bit immediate field.  */
    { 16,  6 }, /* SVE_imm6: 6-bit immediate field.  */
    { 14,  7 }, /* SVE_imm7: 7-bit immediate field.  */
    {  5,  8 }, /* SVE_imm8: 8-bit immediate field.  */
    {  5,  9 }, /* SVE_imm9: 9-bit immediate field.  */
    { 11,  6 }, /* SVE_immr: SVE equivalent of immr.  */
    {  5,  6 }, /* SVE_imms: SVE equivalent of imms.  */
    { 10,  2 }, /* SVE_msz: 2-bit shift amount for ADR.  */
    {  5,  5 }, /* SVE_pattern: vector pattern enumeration.  */
    {  0,  4 }, /* SVE_prfop: prefetch operation for SVE PRF[BHWD].  */
    { 16,  1 }, /* SVE_rot1: 1-bit rotation amount.  */
    { 10,  2 }, /* SVE_rot2: 2-bit rotation amount.  */
    { 10,  1 }, /* SVE_rot3: 1-bit rotation amount at bit 10.  */
    { 17,  2 }, /* SVE_size: 2-bit element size, bits [18,17].  */
    { 22,  1 }, /* SVE_sz: 1-bit element size select.  */
    { 30,  1 }, /* SVE_sz2: 1-bit element size select.  */
    { 16,  4 }, /* SVE_tsz: triangular size select.  */
    { 22,  2 }, /* SVE_tszh: triangular size select high, bits [23,22].  */
    {  8,  2 }, /* SVE_tszl_8: triangular size select low, bits [9,8].  */
    { 19,  2 }, /* SVE_tszl_19: triangular size select low, bits [20,19].  */
    { 14,  1 }, /* SVE_xs_14: UXTW/SXTW select (bit 14).  */
    { 22,  1 }, /* SVE_xs_22: UXTW/SXTW select (bit 22).  */
    { 22,  1 },	/* S_imm10: in LDRAA and LDRAB instructions.  */
    { 16,  3 },	/* abc: a:b:c bits in AdvSIMD modified immediate.  */
    { 13,  3 },	/* asisdlso_opcode: opcode in advsimd ld/st single element.  */
    { 19,  5 },	/* b40: in the test bit and branch instructions.  */
    { 31,  1 },	/* b5: in the test bit and branch instructions.  */
    { 12,  4 },	/* cmode: in advsimd modified immediate instructions.  */
    { 12,  4 },	/* cond: condition flags as a source operand.  */
    {  0,  4 },	/* cond2: condition in truly conditional-executed inst.  */
    {  5,  5 },	/* defgh: d:e:f:g:h bits in AdvSIMD modified immediate.  */
    { 21,  2 },	/* hw: in move wide constant instructions.  */
    {  0,  1 },	/* imm1_0: general immediate in bits [0].  */
    {  2,  1 },	/* imm1_2: general immediate in bits [2].  */
    {  8,  1 },	/* imm1_8: general immediate in bits [8].  */
    { 10,  1 },	/* imm1_10: general immediate in bits [10].  */
    { 15,  1 },	/* imm1_15: general immediate in bits [15].  */
    { 16,  1 },	/* imm1_16: general immediate in bits [16].  */
    {  0,  2 },	/* imm2_0: general immediate in bits [1:0].  */
    {  1,  2 },	/* imm2_1: general immediate in bits [2:1].  */
    {  8,  2 },	/* imm2_8: general immediate in bits [9:8].  */
    { 10,  2 }, /* imm2_10: 2-bit immediate, bits [11:10] */
    { 12,  2 }, /* imm2_12: 2-bit immediate, bits [13:12] */
    { 15,  2 }, /* imm2_15: 2-bit immediate, bits [16:15] */
    { 16,  2 }, /* imm2_16: 2-bit immediate, bits [17:16] */
    { 19,  2 }, /* imm2_19: 2-bit immediate, bits [20:19] */
    {  0,  3 },	/* imm3_0: general immediate in bits [2:0].  */
    {  5,  3 },	/* imm3_5: general immediate in bits [7:5].  */
    { 10,  3 },	/* imm3_10: in add/sub extended reg instructions.  */
    { 12,  3 },	/* imm3_12: general immediate in bits [14:12].  */
    { 14,  3 },	/* imm3_14: general immediate in bits [16:14].  */
    { 15,  3 },	/* imm3_15: general immediate in bits [17:15].  */
    {  0,  4 },	/* imm4_0: in rmif instructions.  */
    {  5,  4 }, /* imm4_5: in SME instructions.  */
    { 10,  4 },	/* imm4_10: in adddg/subg instructions.  */
    { 11,  4 },	/* imm4_11: in advsimd ext and advsimd ins instructions.  */
    { 14,  4 },	/* imm4_14: general immediate in bits [17:14].  */
    { 16,  5 },	/* imm5: in conditional compare (immediate) instructions.  */
    { 10,  6 },	/* imm6_10: in add/sub reg shifted instructions.  */
    { 15,  6 },	/* imm6_15: in rmif instructions.  */
    { 15,  7 },	/* imm7: in load/store pair pre/post index instructions.  */
    { 13,  8 },	/* imm8: in floating-point scalar move immediate inst.  */
    { 12,  9 },	/* imm9: in load/store pre/post index instructions.  */
    { 10, 12 },	/* imm12: in ld/st unsigned imm or add/sub shifted inst.  */
    {  5, 14 },	/* imm14: in test bit and branch instructions.  */
    {  0, 16 },	/* imm16_0: in udf instruction. */
    {  5, 16 },	/* imm16_5: in exception instructions.  */
    {  5, 19 },	/* imm19: e.g. in CBZ.  */
    {  0, 26 },	/* imm26: in unconditional branch instructions.  */
    { 16,  3 },	/* immb: in advsimd shift by immediate instructions.  */
    { 19,  4 },	/* immh: in advsimd shift by immediate instructions.  */
    {  5, 19 },	/* immhi: e.g. in ADRP.  */
    { 29,  2 },	/* immlo: e.g. in ADRP.  */
    { 16,  6 },	/* immr: in bitfield and logical immediate instructions.  */
    { 10,  6 },	/* imms: in bitfield and logical immediate instructions.  */
    { 11,  1 },	/* index: in ld/st inst deciding the pre/post-index.  */
    { 24,  1 },	/* index2: in ld/st pair inst deciding the pre/post-index.  */
    { 30,  2 },	/* ldst_size: size field in ld/st reg offset inst.  */
    { 13,  2 },	/* len: in advsimd tbl/tbx instructions.  */
    { 30,  1 },	/* lse_sz: in LSE extension atomic instructions.  */
    {  0,  4 },	/* nzcv: flag bit specifier, encoded in the "nzcv" field.  */
    { 29,  1 },	/* op: in AdvSIMD modified immediate instructions.  */
    { 19,  2 },	/* op0: in the system instructions.  */
    { 16,  3 },	/* op1: in the system instructions.  */
    {  5,  3 },	/* op2: in the system instructions.  */
    { 22,  2 },	/* opc: in load/store reg offset instructions.  */
    { 23,  1 },	/* opc1: in load/store reg offset instructions.  */
    { 12,  4 },	/* opcode: in advsimd load/store instructions.  */
    { 13,  3 },	/* option: in ld/st reg offset + add/sub extended reg inst.  */
    { 11,  2 }, /* rotate1: FCMLA immediate rotate.  */
    { 13,  2 }, /* rotate2: Indexed element FCMLA immediate rotate.  */
    { 12,  1 }, /* rotate3: FCADD immediate rotate.  */
    { 10,  6 },	/* scale: in the fixed-point scalar to fp converting inst.  */
    { 31,  1 },	/* sf: in integer data processing instructions.  */
    { 22,  2 },	/* shift: in add/sub reg/imm shifted instructions.  */
    { 22,  2 },	/* size: in most AdvSIMD and floating-point instructions.  */
    { 22,  1 }, /* sz: 1-bit element size select.  */
    { 22,  2 },	/* type: floating point type field in fp data inst.  */
    { 10,  2 },	/* vldst_size: size field in the AdvSIMD load/store inst.  */
    {  5,  3 }, /* off3: immediate offset used to calculate slice number in a
		   ZA tile.  */
    {  5,  2 }, /* off2: immediate offset used to calculate slice number in
		   a ZA tile.  */
    {  7,  1 }, /* ZAn_1: name of the 1bit encoded ZA tile.  */
    {  5,  1 }, /* ol: immediate offset used to calculate slice number in a ZA
		   tile.  */
    {  6,  2 }, /* ZAn_2: name of the 2bit encoded ZA tile.  */
    {  5,  3 }, /* ZAn_3: name of the 3bit encoded ZA tile.  */
    {  6,  1 }, /* ZAn: name of the bit encoded ZA tile.  */
    { 12,  4 },	/* opc2: in rcpc3 ld/st inst deciding the pre/post-index.  */
    { 30,  2 },	/* rcpc3_size: in rcpc3 ld/st, field controls Rt/Rt2 width.  */
};

enum aarch64_operand_class
aarch64_get_operand_class (enum aarch64_opnd type)
{
  return aarch64_operands[type].op_class;
}

const char *
aarch64_get_operand_name (enum aarch64_opnd type)
{
  return aarch64_operands[type].name;
}

/* Get operand description string.
   This is usually for the diagnosis purpose.  */
const char *
aarch64_get_operand_desc (enum aarch64_opnd type)
{
  return aarch64_operands[type].desc;
}

/* Table of all conditional affixes.  */
const aarch64_cond aarch64_conds[16] =
{
  {{"eq", "none"}, 0x0},
  {{"ne", "any"}, 0x1},
  {{"cs", "hs", "nlast"}, 0x2},
  {{"cc", "lo", "ul", "last"}, 0x3},
  {{"mi", "first"}, 0x4},
  {{"pl", "nfrst"}, 0x5},
  {{"vs"}, 0x6},
  {{"vc"}, 0x7},
  {{"hi", "pmore"}, 0x8},
  {{"ls", "plast"}, 0x9},
  {{"ge", "tcont"}, 0xa},
  {{"lt", "tstop"}, 0xb},
  {{"gt"}, 0xc},
  {{"le"}, 0xd},
  {{"al"}, 0xe},
  {{"nv"}, 0xf},
};

const aarch64_cond *
get_cond_from_value (aarch64_insn value)
{
  assert (value < 16);
  return &aarch64_conds[(unsigned int) value];
}

const aarch64_cond *
get_inverted_cond (const aarch64_cond *cond)
{
  return &aarch64_conds[cond->value ^ 0x1];
}

/* Table describing the operand extension/shifting operators; indexed by
   enum aarch64_modifier_kind.

   The value column provides the most common values for encoding modifiers,
   which enables table-driven encoding/decoding for the modifiers.  */
const struct aarch64_name_value_pair aarch64_operand_modifiers [] =
{
    {"none", 0x0},
    {"msl",  0x0},
    {"ror",  0x3},
    {"asr",  0x2},
    {"lsr",  0x1},
    {"lsl",  0x0},
    {"uxtb", 0x0},
    {"uxth", 0x1},
    {"uxtw", 0x2},
    {"uxtx", 0x3},
    {"sxtb", 0x4},
    {"sxth", 0x5},
    {"sxtw", 0x6},
    {"sxtx", 0x7},
    {"mul", 0x0},
    {"mul vl", 0x0},
    {NULL, 0},
};

enum aarch64_modifier_kind
aarch64_get_operand_modifier (const struct aarch64_name_value_pair *desc)
{
  return desc - aarch64_operand_modifiers;
}

aarch64_insn
aarch64_get_operand_modifier_value (enum aarch64_modifier_kind kind)
{
  return aarch64_operand_modifiers[kind].value;
}

enum aarch64_modifier_kind
aarch64_get_operand_modifier_from_value (aarch64_insn value,
					 bool extend_p)
{
  if (extend_p)
    return AARCH64_MOD_UXTB + value;
  else
    return AARCH64_MOD_LSL - value;
}

bool
aarch64_extend_operator_p (enum aarch64_modifier_kind kind)
{
  return kind > AARCH64_MOD_LSL && kind <= AARCH64_MOD_SXTX;
}

static inline bool
aarch64_shift_operator_p (enum aarch64_modifier_kind kind)
{
  return kind >= AARCH64_MOD_ROR && kind <= AARCH64_MOD_LSL;
}

const struct aarch64_name_value_pair aarch64_barrier_options[16] =
{
    { "#0x00", 0x0 },
    { "oshld", 0x1 },
    { "oshst", 0x2 },
    { "osh",   0x3 },
    { "#0x04", 0x4 },
    { "nshld", 0x5 },
    { "nshst", 0x6 },
    { "nsh",   0x7 },
    { "#0x08", 0x8 },
    { "ishld", 0x9 },
    { "ishst", 0xa },
    { "ish",   0xb },
    { "#0x0c", 0xc },
    { "ld",    0xd },
    { "st",    0xe },
    { "sy",    0xf },
};

const struct aarch64_name_value_pair aarch64_barrier_dsb_nxs_options[4] =
{                       /*  CRm<3:2>  #imm  */
    { "oshnxs", 16 },    /*    00       16   */
    { "nshnxs", 20 },    /*    01       20   */
    { "ishnxs", 24 },    /*    10       24   */
    { "synxs",  28 },    /*    11       28   */
};

/* Table describing the operands supported by the aliases of the HINT
   instruction.

   The name column is the operand that is accepted for the alias.  The value
   column is the hint number of the alias.  The list of operands is terminated
   by NULL in the name column.  */

const struct aarch64_name_value_pair aarch64_hint_options[] =
{
  /* BTI.  This is also the F_DEFAULT entry for AARCH64_OPND_BTI_TARGET.  */
  { " ",	HINT_ENCODE (HINT_OPD_F_NOPRINT, 0x20) },
  { "csync",	HINT_OPD_CSYNC },	/* PSB CSYNC.  */
  { "dsync",	HINT_OPD_DSYNC },	/* GCSB DSYNC.  */
  { "c",	HINT_OPD_C },		/* BTI C.  */
  { "j",	HINT_OPD_J },		/* BTI J.  */
  { "jc",	HINT_OPD_JC },		/* BTI JC.  */
  { NULL,	HINT_OPD_NULL },
};

/* op -> op:       load = 0 instruction = 1 store = 2
   l  -> level:    1-3
   t  -> temporal: temporal (retained) = 0 non-temporal (streaming) = 1   */
#define B(op,l,t) (((op) << 3) | (((l) - 1) << 1) | (t))
const struct aarch64_name_value_pair aarch64_prfops[32] =
{
  { "pldl1keep", B(0, 1, 0) },
  { "pldl1strm", B(0, 1, 1) },
  { "pldl2keep", B(0, 2, 0) },
  { "pldl2strm", B(0, 2, 1) },
  { "pldl3keep", B(0, 3, 0) },
  { "pldl3strm", B(0, 3, 1) },
  { "pldslckeep", B(0, 4, 0) },
  { "pldslcstrm", B(0, 4, 1) },
  { "plil1keep", B(1, 1, 0) },
  { "plil1strm", B(1, 1, 1) },
  { "plil2keep", B(1, 2, 0) },
  { "plil2strm", B(1, 2, 1) },
  { "plil3keep", B(1, 3, 0) },
  { "plil3strm", B(1, 3, 1) },
  { "plislckeep", B(1, 4, 0) },
  { "plislcstrm", B(1, 4, 1) },
  { "pstl1keep", B(2, 1, 0) },
  { "pstl1strm", B(2, 1, 1) },
  { "pstl2keep", B(2, 2, 0) },
  { "pstl2strm", B(2, 2, 1) },
  { "pstl3keep", B(2, 3, 0) },
  { "pstl3strm", B(2, 3, 1) },
  { "pstslckeep", B(2, 4, 0) },
  { "pstslcstrm", B(2, 4, 1) },
  { NULL, 0x18 },
  { NULL, 0x19 },
  { NULL, 0x1a },
  { NULL, 0x1b },
  { NULL, 0x1c },
  { NULL, 0x1d },
  { NULL, 0x1e },
  { NULL, 0x1f },
};
#undef B

/* Utilities on value constraint.  */

static inline int
value_in_range_p (int64_t value, int low, int high)
{
  return (value >= low && value <= high) ? 1 : 0;
}

/* Return true if VALUE is a multiple of ALIGN.  */
static inline int
value_aligned_p (int64_t value, int align)
{
  return (value % align) == 0;
}

/* A signed value fits in a field.  */
static inline int
value_fit_signed_field_p (int64_t value, unsigned width)
{
  assert (width < 32);
  if (width < sizeof (value) * 8)
    {
      int64_t lim = (uint64_t) 1 << (width - 1);
      if (value >= -lim && value < lim)
	return 1;
    }
  return 0;
}

/* An unsigned value fits in a field.  */
static inline int
value_fit_unsigned_field_p (int64_t value, unsigned width)
{
  assert (width < 32);
  if (width < sizeof (value) * 8)
    {
      int64_t lim = (uint64_t) 1 << width;
      if (value >= 0 && value < lim)
	return 1;
    }
  return 0;
}

/* Return 1 if OPERAND is SP or WSP.  */
int
aarch64_stack_pointer_p (const aarch64_opnd_info *operand)
{
  return ((aarch64_get_operand_class (operand->type)
	   == AARCH64_OPND_CLASS_INT_REG)
	  && operand_maybe_stack_pointer (aarch64_operands + operand->type)
	  && operand->reg.regno == 31);
}

/* Return 1 if OPERAND is XZR or WZP.  */
int
aarch64_zero_register_p (const aarch64_opnd_info *operand)
{
  return ((aarch64_get_operand_class (operand->type)
	   == AARCH64_OPND_CLASS_INT_REG)
	  && !operand_maybe_stack_pointer (aarch64_operands + operand->type)
	  && operand->reg.regno == 31);
}

/* Return true if the operand *OPERAND that has the operand code
   OPERAND->TYPE and been qualified by OPERAND->QUALIFIER can be also
   qualified by the qualifier TARGET.  */

static inline int
operand_also_qualified_p (const struct aarch64_opnd_info *operand,
			  aarch64_opnd_qualifier_t target)
{
  switch (operand->qualifier)
    {
    case AARCH64_OPND_QLF_W:
      if (target == AARCH64_OPND_QLF_WSP && aarch64_stack_pointer_p (operand))
	return 1;
      break;
    case AARCH64_OPND_QLF_X:
      if (target == AARCH64_OPND_QLF_SP && aarch64_stack_pointer_p (operand))
	return 1;
      break;
    case AARCH64_OPND_QLF_WSP:
      if (target == AARCH64_OPND_QLF_W
	  && operand_maybe_stack_pointer (aarch64_operands + operand->type))
	return 1;
      break;
    case AARCH64_OPND_QLF_SP:
      if (target == AARCH64_OPND_QLF_X
	  && operand_maybe_stack_pointer (aarch64_operands + operand->type))
	return 1;
      break;
    default:
      break;
    }

  return 0;
}

/* Given qualifier sequence list QSEQ_LIST and the known qualifier KNOWN_QLF
   for operand KNOWN_IDX, return the expected qualifier for operand IDX.

   Return NIL if more than one expected qualifiers are found.  */

aarch64_opnd_qualifier_t
aarch64_get_expected_qualifier (const aarch64_opnd_qualifier_seq_t *qseq_list,
				int idx,
				const aarch64_opnd_qualifier_t known_qlf,
				int known_idx)
{
  int i, saved_i;

  /* Special case.

     When the known qualifier is NIL, we have to assume that there is only
     one qualifier sequence in the *QSEQ_LIST and return the corresponding
     qualifier directly.  One scenario is that for instruction
	PRFM <prfop>, [<Xn|SP>, #:lo12:<symbol>]
     which has only one possible valid qualifier sequence
	NIL, S_D
     the caller may pass NIL in KNOWN_QLF to obtain S_D so that it can
     determine the correct relocation type (i.e. LDST64_LO12) for PRFM.

     Because the qualifier NIL has dual roles in the qualifier sequence:
     it can mean no qualifier for the operand, or the qualifer sequence is
     not in use (when all qualifiers in the sequence are NILs), we have to
     handle this special case here.  */
  if (known_qlf == AARCH64_OPND_NIL)
    {
      assert (qseq_list[0][known_idx] == AARCH64_OPND_NIL);
      return qseq_list[0][idx];
    }

  for (i = 0, saved_i = -1; i < AARCH64_MAX_QLF_SEQ_NUM; ++i)
    {
      if (qseq_list[i][known_idx] == known_qlf)
	{
	  if (saved_i != -1)
	    /* More than one sequences are found to have KNOWN_QLF at
	       KNOWN_IDX.  */
	    return AARCH64_OPND_NIL;
	  saved_i = i;
	}
    }

  return qseq_list[saved_i][idx];
}

enum operand_qualifier_kind
{
  OQK_NIL,
  OQK_OPD_VARIANT,
  OQK_VALUE_IN_RANGE,
  OQK_MISC,
};

/* Operand qualifier description.  */
struct operand_qualifier_data
{
  /* The usage of the three data fields depends on the qualifier kind.  */
  int data0;
  int data1;
  int data2;
  /* Description.  */
  const char *desc;
  /* Kind.  */
  enum operand_qualifier_kind kind;
};

/* Indexed by the operand qualifier enumerators.  */
struct operand_qualifier_data aarch64_opnd_qualifiers[] =
{
  {0, 0, 0, "NIL", OQK_NIL},

  /* Operand variant qualifiers.
     First 3 fields:
     element size, number of elements and common value for encoding.  */

  {4, 1, 0x0, "w", OQK_OPD_VARIANT},
  {8, 1, 0x1, "x", OQK_OPD_VARIANT},
  {4, 1, 0x0, "wsp", OQK_OPD_VARIANT},
  {8, 1, 0x1, "sp", OQK_OPD_VARIANT},

  {1, 1, 0x0, "b", OQK_OPD_VARIANT},
  {2, 1, 0x1, "h", OQK_OPD_VARIANT},
  {4, 1, 0x2, "s", OQK_OPD_VARIANT},
  {8, 1, 0x3, "d", OQK_OPD_VARIANT},
  {16, 1, 0x4, "q", OQK_OPD_VARIANT},
  {4, 1, 0x0, "4b", OQK_OPD_VARIANT},
  {4, 1, 0x0, "2h", OQK_OPD_VARIANT},

  {1, 4, 0x0, "4b", OQK_OPD_VARIANT},
  {1, 8, 0x0, "8b", OQK_OPD_VARIANT},
  {1, 16, 0x1, "16b", OQK_OPD_VARIANT},
  {2, 2, 0x0, "2h", OQK_OPD_VARIANT},
  {2, 4, 0x2, "4h", OQK_OPD_VARIANT},
  {2, 8, 0x3, "8h", OQK_OPD_VARIANT},
  {4, 2, 0x4, "2s", OQK_OPD_VARIANT},
  {4, 4, 0x5, "4s", OQK_OPD_VARIANT},
  {8, 1, 0x6, "1d", OQK_OPD_VARIANT},
  {8, 2, 0x7, "2d", OQK_OPD_VARIANT},
  {16, 1, 0x8, "1q", OQK_OPD_VARIANT},

  {0, 0, 0, "z", OQK_OPD_VARIANT},
  {0, 0, 0, "m", OQK_OPD_VARIANT},

  /* Qualifier for scaled immediate for Tag granule (stg,st2g,etc).  */
  {16, 0, 0, "tag", OQK_OPD_VARIANT},

  /* Qualifiers constraining the value range.
     First 3 fields:
     Lower bound, higher bound, unused.  */

  {0, 15, 0, "CR",       OQK_VALUE_IN_RANGE},
  {0,  7, 0, "imm_0_7" , OQK_VALUE_IN_RANGE},
  {0, 15, 0, "imm_0_15", OQK_VALUE_IN_RANGE},
  {0, 31, 0, "imm_0_31", OQK_VALUE_IN_RANGE},
  {0, 63, 0, "imm_0_63", OQK_VALUE_IN_RANGE},
  {1, 32, 0, "imm_1_32", OQK_VALUE_IN_RANGE},
  {1, 64, 0, "imm_1_64", OQK_VALUE_IN_RANGE},

  /* Qualifiers for miscellaneous purpose.
     First 3 fields:
     unused, unused and unused.  */

  {0, 0, 0, "lsl", 0},
  {0, 0, 0, "msl", 0},

  {0, 0, 0, "retrieving", 0},
};

static inline bool
operand_variant_qualifier_p (aarch64_opnd_qualifier_t qualifier)
{
  return aarch64_opnd_qualifiers[qualifier].kind == OQK_OPD_VARIANT;
}

static inline bool
qualifier_value_in_range_constraint_p (aarch64_opnd_qualifier_t qualifier)
{
  return aarch64_opnd_qualifiers[qualifier].kind == OQK_VALUE_IN_RANGE;
}

const char*
aarch64_get_qualifier_name (aarch64_opnd_qualifier_t qualifier)
{
  return aarch64_opnd_qualifiers[qualifier].desc;
}

/* Given an operand qualifier, return the expected data element size
   of a qualified operand.  */
unsigned char
aarch64_get_qualifier_esize (aarch64_opnd_qualifier_t qualifier)
{
  assert (operand_variant_qualifier_p (qualifier));
  return aarch64_opnd_qualifiers[qualifier].data0;
}

unsigned char
aarch64_get_qualifier_nelem (aarch64_opnd_qualifier_t qualifier)
{
  assert (operand_variant_qualifier_p (qualifier));
  return aarch64_opnd_qualifiers[qualifier].data1;
}

aarch64_insn
aarch64_get_qualifier_standard_value (aarch64_opnd_qualifier_t qualifier)
{
  assert (operand_variant_qualifier_p (qualifier));
  return aarch64_opnd_qualifiers[qualifier].data2;
}

static int
get_lower_bound (aarch64_opnd_qualifier_t qualifier)
{
  assert (qualifier_value_in_range_constraint_p (qualifier));
  return aarch64_opnd_qualifiers[qualifier].data0;
}

static int
get_upper_bound (aarch64_opnd_qualifier_t qualifier)
{
  assert (qualifier_value_in_range_constraint_p (qualifier));
  return aarch64_opnd_qualifiers[qualifier].data1;
}

#ifdef DEBUG_AARCH64
void
aarch64_verbose (const char *str, ...)
{
  va_list ap;
  va_start (ap, str);
  printf ("#### ");
  vprintf (str, ap);
  printf ("\n");
  va_end (ap);
}

static inline void
dump_qualifier_sequence (const aarch64_opnd_qualifier_t *qualifier)
{
  int i;
  printf ("#### \t");
  for (i = 0; i < AARCH64_MAX_OPND_NUM; ++i, ++qualifier)
    printf ("%s,", aarch64_get_qualifier_name (*qualifier));
  printf ("\n");
}

static void
dump_match_qualifiers (const struct aarch64_opnd_info *opnd,
		       const aarch64_opnd_qualifier_t *qualifier)
{
  int i;
  aarch64_opnd_qualifier_t curr[AARCH64_MAX_OPND_NUM];

  aarch64_verbose ("dump_match_qualifiers:");
  for (i = 0; i < AARCH64_MAX_OPND_NUM; ++i)
    curr[i] = opnd[i].qualifier;
  dump_qualifier_sequence (curr);
  aarch64_verbose ("against");
  dump_qualifier_sequence (qualifier);
}
#endif /* DEBUG_AARCH64 */

/* This function checks if the given instruction INSN is a destructive
   instruction based on the usage of the registers.  It does not recognize
   unary destructive instructions.  */
bool
aarch64_is_destructive_by_operands (const aarch64_opcode *opcode)
{
  int i = 0;
  const enum aarch64_opnd *opnds = opcode->operands;

  if (opnds[0] == AARCH64_OPND_NIL)
    return false;

  while (opnds[++i] != AARCH64_OPND_NIL)
    if (opnds[i] == opnds[0])
      return true;

  return false;
}

/* TODO improve this, we can have an extra field at the runtime to
   store the number of operands rather than calculating it every time.  */

int
aarch64_num_of_operands (const aarch64_opcode *opcode)
{
  int i = 0;
  const enum aarch64_opnd *opnds = opcode->operands;
  while (opnds[i++] != AARCH64_OPND_NIL)
    ;
  --i;
  assert (i >= 0 && i <= AARCH64_MAX_OPND_NUM);
  return i;
}

/* Find the best matched qualifier sequence in *QUALIFIERS_LIST for INST.
   If succeeds, fill the found sequence in *RET, return 1; otherwise return 0.

   Store the smallest number of non-matching qualifiers in *INVALID_COUNT.
   This is always 0 if the function succeeds.

   N.B. on the entry, it is very likely that only some operands in *INST
   have had their qualifiers been established.

   If STOP_AT is not -1, the function will only try to match
   the qualifier sequence for operands before and including the operand
   of index STOP_AT; and on success *RET will only be filled with the first
   (STOP_AT+1) qualifiers.

   A couple examples of the matching algorithm:

   X,W,NIL should match
   X,W,NIL

   NIL,NIL should match
   X  ,NIL

   Apart from serving the main encoding routine, this can also be called
   during or after the operand decoding.  */

int
aarch64_find_best_match (const aarch64_inst *inst,
			 const aarch64_opnd_qualifier_seq_t *qualifiers_list,
			 int stop_at, aarch64_opnd_qualifier_t *ret,
			 int *invalid_count)
{
  int i, num_opnds, invalid, min_invalid;
  const aarch64_opnd_qualifier_t *qualifiers;

  num_opnds = aarch64_num_of_operands (inst->opcode);
  if (num_opnds == 0)
    {
      DEBUG_TRACE ("SUCCEED: no operand");
      *invalid_count = 0;
      return 1;
    }

  if (stop_at < 0 || stop_at >= num_opnds)
    stop_at = num_opnds - 1;

  /* For each pattern.  */
  min_invalid = num_opnds;
  for (i = 0; i < AARCH64_MAX_QLF_SEQ_NUM; ++i, ++qualifiers_list)
    {
      int j;
      qualifiers = *qualifiers_list;

      /* Start as positive.  */
      invalid = 0;

      DEBUG_TRACE ("%d", i);
#ifdef DEBUG_AARCH64
      if (debug_dump)
	dump_match_qualifiers (inst->operands, qualifiers);
#endif

      /* The first entry should be taken literally, even if it's an empty
	 qualifier sequence.  (This matters for strict testing.)  In other
	 positions an empty sequence acts as a terminator.  */
      if (i > 0 && empty_qualifier_sequence_p (qualifiers))
	break;

      for (j = 0; j < num_opnds && j <= stop_at; ++j, ++qualifiers)
	{
	  if (inst->operands[j].qualifier == AARCH64_OPND_QLF_NIL
	      && !(inst->opcode->flags & F_STRICT))
	    {
	      /* Either the operand does not have qualifier, or the qualifier
		 for the operand needs to be deduced from the qualifier
		 sequence.
		 In the latter case, any constraint checking related with
		 the obtained qualifier should be done later in
		 operand_general_constraint_met_p.  */
	      continue;
	    }
	  else if (*qualifiers != inst->operands[j].qualifier)
	    {
	      /* Unless the target qualifier can also qualify the operand
		 (which has already had a non-nil qualifier), non-equal
		 qualifiers are generally un-matched.  */
	      if (operand_also_qualified_p (inst->operands + j, *qualifiers))
		continue;
	      else
		invalid += 1;
	    }
	  else
	    continue;	/* Equal qualifiers are certainly matched.  */
	}

      if (min_invalid > invalid)
	min_invalid = invalid;

      /* Qualifiers established.  */
      if (min_invalid == 0)
	break;
    }

  *invalid_count = min_invalid;
  if (min_invalid == 0)
    {
      /* Fill the result in *RET.  */
      int j;
      qualifiers = *qualifiers_list;

      DEBUG_TRACE ("complete qualifiers using list %d", i);
#ifdef DEBUG_AARCH64
      if (debug_dump)
	dump_qualifier_sequence (qualifiers);
#endif

      for (j = 0; j <= stop_at; ++j, ++qualifiers)
	ret[j] = *qualifiers;
      for (; j < AARCH64_MAX_OPND_NUM; ++j)
	ret[j] = AARCH64_OPND_QLF_NIL;

      DEBUG_TRACE ("SUCCESS");
      return 1;
    }

  DEBUG_TRACE ("FAIL");
  return 0;
}

/* Operand qualifier matching and resolving.

   Return 1 if the operand qualifier(s) in *INST match one of the qualifier
   sequences in INST->OPCODE->qualifiers_list; otherwise return 0.

   Store the smallest number of non-matching qualifiers in *INVALID_COUNT.
   This is always 0 if the function succeeds.

   if UPDATE_P, update the qualifier(s) in *INST after the matching
   succeeds.  */

static int
match_operands_qualifier (aarch64_inst *inst, bool update_p,
			  int *invalid_count)
{
  int i;
  aarch64_opnd_qualifier_seq_t qualifiers;

  if (!aarch64_find_best_match (inst, inst->opcode->qualifiers_list, -1,
				qualifiers, invalid_count))
    {
      DEBUG_TRACE ("matching FAIL");
      return 0;
    }

  /* Update the qualifiers.  */
  if (update_p)
    for (i = 0; i < AARCH64_MAX_OPND_NUM; ++i)
      {
	if (inst->opcode->operands[i] == AARCH64_OPND_NIL)
	  break;
	DEBUG_TRACE_IF (inst->operands[i].qualifier != qualifiers[i],
			"update %s with %s for operand %d",
			aarch64_get_qualifier_name (inst->operands[i].qualifier),
			aarch64_get_qualifier_name (qualifiers[i]), i);
	inst->operands[i].qualifier = qualifiers[i];
      }

  DEBUG_TRACE ("matching SUCCESS");
  return 1;
}

/* Return TRUE if VALUE is a wide constant that can be moved into a general
   register by MOVZ.

   IS32 indicates whether value is a 32-bit immediate or not.
   If SHIFT_AMOUNT is not NULL, on the return of TRUE, the logical left shift
   amount will be returned in *SHIFT_AMOUNT.  */

bool
aarch64_wide_constant_p (uint64_t value, int is32, unsigned int *shift_amount)
{
  int amount;

  DEBUG_TRACE ("enter with 0x%" PRIx64 "(%" PRIi64 ")", value, value);

  if (is32)
    {
      /* Allow all zeros or all ones in top 32-bits, so that
	 32-bit constant expressions like ~0x80000000 are
	 permitted.  */
      if (value >> 32 != 0 && value >> 32 != 0xffffffff)
	/* Immediate out of range.  */
	return false;
      value &= 0xffffffff;
    }

  /* first, try movz then movn */
  amount = -1;
  if ((value & ((uint64_t) 0xffff << 0)) == value)
    amount = 0;
  else if ((value & ((uint64_t) 0xffff << 16)) == value)
    amount = 16;
  else if (!is32 && (value & ((uint64_t) 0xffff << 32)) == value)
    amount = 32;
  else if (!is32 && (value & ((uint64_t) 0xffff << 48)) == value)
    amount = 48;

  if (amount == -1)
    {
      DEBUG_TRACE ("exit false with 0x%" PRIx64 "(%" PRIi64 ")", value, value);
      return false;
    }

  if (shift_amount != NULL)
    *shift_amount = amount;

  DEBUG_TRACE ("exit true with amount %d", amount);

  return true;
}

/* Build the accepted values for immediate logical SIMD instructions.

   The standard encodings of the immediate value are:
     N      imms     immr         SIMD size  R             S
     1      ssssss   rrrrrr       64      UInt(rrrrrr)  UInt(ssssss)
     0      0sssss   0rrrrr       32      UInt(rrrrr)   UInt(sssss)
     0      10ssss   00rrrr       16      UInt(rrrr)    UInt(ssss)
     0      110sss   000rrr       8       UInt(rrr)     UInt(sss)
     0      1110ss   0000rr       4       UInt(rr)      UInt(ss)
     0      11110s   00000r       2       UInt(r)       UInt(s)
   where all-ones value of S is reserved.

   Let's call E the SIMD size.

   The immediate value is: S+1 bits '1' rotated to the right by R.

   The total of valid encodings is 64*63 + 32*31 + ... + 2*1 = 5334
   (remember S != E - 1).  */

#define TOTAL_IMM_NB  5334

typedef struct
{
  uint64_t imm;
  aarch64_insn encoding;
} simd_imm_encoding;

static simd_imm_encoding simd_immediates[TOTAL_IMM_NB];

static int
simd_imm_encoding_cmp(const void *i1, const void *i2)
{
  const simd_imm_encoding *imm1 = (const simd_imm_encoding *)i1;
  const simd_imm_encoding *imm2 = (const simd_imm_encoding *)i2;

  if (imm1->imm < imm2->imm)
    return -1;
  if (imm1->imm > imm2->imm)
    return +1;
  return 0;
}

/* immediate bitfield standard encoding
   imm13<12> imm13<5:0> imm13<11:6> SIMD size R      S
   1         ssssss     rrrrrr      64        rrrrrr ssssss
   0         0sssss     0rrrrr      32        rrrrr  sssss
   0         10ssss     00rrrr      16        rrrr   ssss
   0         110sss     000rrr      8         rrr    sss
   0         1110ss     0000rr      4         rr     ss
   0         11110s     00000r      2         r      s  */
static inline int
encode_immediate_bitfield (int is64, uint32_t s, uint32_t r)
{
  return (is64 << 12) | (r << 6) | s;
}

static void
build_immediate_table (void)
{
  uint32_t log_e, e, s, r, s_mask;
  uint64_t mask, imm;
  int nb_imms;
  int is64;

  nb_imms = 0;
  for (log_e = 1; log_e <= 6; log_e++)
    {
      /* Get element size.  */
      e = 1u << log_e;
      if (log_e == 6)
	{
	  is64 = 1;
	  mask = 0xffffffffffffffffull;
	  s_mask = 0;
	}
      else
	{
	  is64 = 0;
	  mask = (1ull << e) - 1;
	  /* log_e  s_mask
	     1     ((1 << 4) - 1) << 2 = 111100
	     2     ((1 << 3) - 1) << 3 = 111000
	     3     ((1 << 2) - 1) << 4 = 110000
	     4     ((1 << 1) - 1) << 5 = 100000
	     5     ((1 << 0) - 1) << 6 = 000000  */
	  s_mask = ((1u << (5 - log_e)) - 1) << (log_e + 1);
	}
      for (s = 0; s < e - 1; s++)
	for (r = 0; r < e; r++)
	  {
	    /* s+1 consecutive bits to 1 (s < 63) */
	    imm = (1ull << (s + 1)) - 1;
	    /* rotate right by r */
	    if (r != 0)
	      imm = (imm >> r) | ((imm << (e - r)) & mask);
	    /* replicate the constant depending on SIMD size */
	    switch (log_e)
	      {
	      case 1: imm = (imm <<  2) | imm;
		/* Fall through.  */
	      case 2: imm = (imm <<  4) | imm;
		/* Fall through.  */
	      case 3: imm = (imm <<  8) | imm;
		/* Fall through.  */
	      case 4: imm = (imm << 16) | imm;
		/* Fall through.  */
	      case 5: imm = (imm << 32) | imm;
		/* Fall through.  */
	      case 6: break;
	      default: abort ();
	      }
	    simd_immediates[nb_imms].imm = imm;
	    simd_immediates[nb_imms].encoding =
	      encode_immediate_bitfield(is64, s | s_mask, r);
	    nb_imms++;
	  }
    }
  assert (nb_imms == TOTAL_IMM_NB);
  qsort(simd_immediates, nb_imms,
	sizeof(simd_immediates[0]), simd_imm_encoding_cmp);
}

/* Return TRUE if VALUE is a valid logical immediate, i.e. bitmask, that can
   be accepted by logical (immediate) instructions
   e.g. ORR <Xd|SP>, <Xn>, #<imm>.

   ESIZE is the number of bytes in the decoded immediate value.
   If ENCODING is not NULL, on the return of TRUE, the standard encoding for
   VALUE will be returned in *ENCODING.  */

bool
aarch64_logical_immediate_p (uint64_t value, int esize, aarch64_insn *encoding)
{
  simd_imm_encoding imm_enc;
  const simd_imm_encoding *imm_encoding;
  static bool initialized = false;
  uint64_t upper;
  int i;

  DEBUG_TRACE ("enter with 0x%" PRIx64 "(%" PRIi64 "), esize: %d", value,
	       value, esize);

  if (!initialized)
    {
      build_immediate_table ();
      initialized = true;
    }

  /* Allow all zeros or all ones in top bits, so that
     constant expressions like ~1 are permitted.  */
  upper = (uint64_t) -1 << (esize * 4) << (esize * 4);
  if ((value & ~upper) != value && (value | upper) != value)
    return false;

  /* Replicate to a full 64-bit value.  */
  value &= ~upper;
  for (i = esize * 8; i < 64; i *= 2)
    value |= (value << i);

  imm_enc.imm = value;
  imm_encoding = (const simd_imm_encoding *)
    bsearch(&imm_enc, simd_immediates, TOTAL_IMM_NB,
            sizeof(simd_immediates[0]), simd_imm_encoding_cmp);
  if (imm_encoding == NULL)
    {
      DEBUG_TRACE ("exit with false");
      return false;
    }
  if (encoding != NULL)
    *encoding = imm_encoding->encoding;
  DEBUG_TRACE ("exit with true");
  return true;
}

/* If 64-bit immediate IMM is in the format of
   "aaaaaaaabbbbbbbbccccccccddddddddeeeeeeeeffffffffgggggggghhhhhhhh",
   where a, b, c, d, e, f, g and h are independently 0 or 1, return an integer
   of value "abcdefgh".  Otherwise return -1.  */
int
aarch64_shrink_expanded_imm8 (uint64_t imm)
{
  int i, ret;
  uint32_t byte;

  ret = 0;
  for (i = 0; i < 8; i++)
    {
      byte = (imm >> (8 * i)) & 0xff;
      if (byte == 0xff)
	ret |= 1 << i;
      else if (byte != 0x00)
	return -1;
    }
  return ret;
}

/* Utility inline functions for operand_general_constraint_met_p.  */

static inline void
set_error (aarch64_operand_error *mismatch_detail,
	   enum aarch64_operand_error_kind kind, int idx,
	   const char* error)
{
  if (mismatch_detail == NULL)
    return;
  mismatch_detail->kind = kind;
  mismatch_detail->index = idx;
  mismatch_detail->error = error;
}

static inline void
set_syntax_error (aarch64_operand_error *mismatch_detail, int idx,
		  const char* error)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_SYNTAX_ERROR, idx, error);
}

static inline void
set_invalid_regno_error (aarch64_operand_error *mismatch_detail, int idx,
			 const char *prefix, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_INVALID_REGNO, idx, NULL);
  mismatch_detail->data[0].s = prefix;
  mismatch_detail->data[1].i = lower_bound;
  mismatch_detail->data[2].i = upper_bound;
}

static inline void
set_out_of_range_error (aarch64_operand_error *mismatch_detail,
			int idx, int lower_bound, int upper_bound,
			const char* error)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_OUT_OF_RANGE, idx, error);
  mismatch_detail->data[0].i = lower_bound;
  mismatch_detail->data[1].i = upper_bound;
}

static inline void
set_imm_out_of_range_error (aarch64_operand_error *mismatch_detail,
			    int idx, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_out_of_range_error (mismatch_detail, idx, lower_bound, upper_bound,
			  _("immediate value"));
}

static inline void
set_offset_out_of_range_error (aarch64_operand_error *mismatch_detail,
			       int idx, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_out_of_range_error (mismatch_detail, idx, lower_bound, upper_bound,
			  _("immediate offset"));
}

static inline void
set_regno_out_of_range_error (aarch64_operand_error *mismatch_detail,
			      int idx, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_out_of_range_error (mismatch_detail, idx, lower_bound, upper_bound,
			  _("register number"));
}

static inline void
set_elem_idx_out_of_range_error (aarch64_operand_error *mismatch_detail,
				 int idx, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_out_of_range_error (mismatch_detail, idx, lower_bound, upper_bound,
			  _("register element index"));
}

static inline void
set_sft_amount_out_of_range_error (aarch64_operand_error *mismatch_detail,
				   int idx, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_out_of_range_error (mismatch_detail, idx, lower_bound, upper_bound,
			  _("shift amount"));
}

/* Report that the MUL modifier in operand IDX should be in the range
   [LOWER_BOUND, UPPER_BOUND].  */
static inline void
set_multiplier_out_of_range_error (aarch64_operand_error *mismatch_detail,
				   int idx, int lower_bound, int upper_bound)
{
  if (mismatch_detail == NULL)
    return;
  set_out_of_range_error (mismatch_detail, idx, lower_bound, upper_bound,
			  _("multiplier"));
}

static inline void
set_unaligned_error (aarch64_operand_error *mismatch_detail, int idx,
		     int alignment)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_UNALIGNED, idx, NULL);
  mismatch_detail->data[0].i = alignment;
}

static inline void
set_reg_list_length_error (aarch64_operand_error *mismatch_detail, int idx,
			   int expected_num)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_REG_LIST_LENGTH, idx, NULL);
  mismatch_detail->data[0].i = 1 << expected_num;
}

static inline void
set_reg_list_stride_error (aarch64_operand_error *mismatch_detail, int idx,
			   int expected_num)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_REG_LIST_STRIDE, idx, NULL);
  mismatch_detail->data[0].i = 1 << expected_num;
}

static inline void
set_invalid_vg_size (aarch64_operand_error *mismatch_detail,
		     int idx, int expected)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_INVALID_VG_SIZE, idx, NULL);
  mismatch_detail->data[0].i = expected;
}

static inline void
set_other_error (aarch64_operand_error *mismatch_detail, int idx,
		 const char* error)
{
  if (mismatch_detail == NULL)
    return;
  set_error (mismatch_detail, AARCH64_OPDE_OTHER_ERROR, idx, error);
}

/* Check that indexed register operand OPND has a register in the range
   [MIN_REGNO, MAX_REGNO] and an index in the range [MIN_INDEX, MAX_INDEX].
   PREFIX is the register prefix, such as "z" for SVE vector registers.  */

static bool
check_reglane (const aarch64_opnd_info *opnd,
	       aarch64_operand_error *mismatch_detail, int idx,
	       const char *prefix, int min_regno, int max_regno,
	       int min_index, int max_index)
{
  if (!value_in_range_p (opnd->reglane.regno, min_regno, max_regno))
    {
      set_invalid_regno_error (mismatch_detail, idx, prefix, min_regno,
			       max_regno);
      return false;
    }
  if (!value_in_range_p (opnd->reglane.index, min_index, max_index))
    {
      set_elem_idx_out_of_range_error (mismatch_detail, idx, min_index,
				       max_index);
      return false;
    }
  return true;
}

/* Check that register list operand OPND has NUM_REGS registers and a
   register stride of STRIDE.  */

static bool
check_reglist (const aarch64_opnd_info *opnd,
	       aarch64_operand_error *mismatch_detail, int idx,
	       int num_regs, int stride)
{
  if (opnd->reglist.num_regs != num_regs)
    {
      set_reg_list_length_error (mismatch_detail, idx, num_regs);
      return false;
    }
  if (opnd->reglist.stride != stride)
    {
      set_reg_list_stride_error (mismatch_detail, idx, stride);
      return false;
    }
  return true;
}

/* Check that indexed ZA operand OPND has:

   - a selection register in the range [MIN_WREG, MIN_WREG + 3]

   - RANGE_SIZE consecutive immediate offsets.

   - an initial immediate offset that is a multiple of RANGE_SIZE
     in the range [0, MAX_VALUE * RANGE_SIZE]

   - a vector group size of GROUP_SIZE.  */

static bool
check_za_access (const aarch64_opnd_info *opnd,
		 aarch64_operand_error *mismatch_detail, int idx,
		 int min_wreg, int max_value, unsigned int range_size,
		 int group_size)
{
  if (!value_in_range_p (opnd->indexed_za.index.regno, min_wreg, min_wreg + 3))
    {
      if (min_wreg == 12)
	set_other_error (mismatch_detail, idx,
			 _("expected a selection register in the"
			   " range w12-w15"));
      else if (min_wreg == 8)
	set_other_error (mismatch_detail, idx,
			 _("expected a selection register in the"
			   " range w8-w11"));
      else
	abort ();
      return false;
    }

  int max_index = max_value * range_size;
  if (!value_in_range_p (opnd->indexed_za.index.imm, 0, max_index))
    {
      set_offset_out_of_range_error (mismatch_detail, idx, 0, max_index);
      return false;
    }

  if ((opnd->indexed_za.index.imm % range_size) != 0)
    {
      assert (range_size == 2 || range_size == 4);
      set_other_error (mismatch_detail, idx,
		       range_size == 2
		       ? _("starting offset is not a multiple of 2")
		       : _("starting offset is not a multiple of 4"));
      return false;
    }

  if (opnd->indexed_za.index.countm1 != range_size - 1)
    {
      if (range_size == 1)
	set_other_error (mismatch_detail, idx,
			 _("expected a single offset rather than"
			   " a range"));
      else if (range_size == 2)
	set_other_error (mismatch_detail, idx,
			 _("expected a range of two offsets"));
      else if (range_size == 4)
	set_other_error (mismatch_detail, idx,
			 _("expected a range of four offsets"));
      else
	abort ();
      return false;
    }

  /* The vector group specifier is optional in assembly code.  */
  if (opnd->indexed_za.group_size != 0
      && opnd->indexed_za.group_size != group_size)
    {
      set_invalid_vg_size (mismatch_detail, idx, group_size);
      return false;
    }

  return true;
}

/* Given a load/store operation, calculate the size of transferred data via a
   cumulative sum of qualifier sizes preceding the address operand in the
   OPNDS operand list argument.  */
int
calc_ldst_datasize (const aarch64_opnd_info *opnds)
{
  unsigned num_bytes = 0; /* total number of bytes transferred.  */
  enum aarch64_operand_class opnd_class;
  enum aarch64_opnd type;

  for (int i = 0; i < AARCH64_MAX_OPND_NUM; i++)
    {
      type = opnds[i].type;
      opnd_class = aarch64_operands[type].op_class;
      if (opnd_class == AARCH64_OPND_CLASS_ADDRESS)
	break;
      num_bytes += aarch64_get_qualifier_esize (opnds[i].qualifier);
    }
  return num_bytes;
}


/* General constraint checking based on operand code.

   Return 1 if OPNDS[IDX] meets the general constraint of operand code TYPE
   as the IDXth operand of opcode OPCODE.  Otherwise return 0.

   This function has to be called after the qualifiers for all operands
   have been resolved.

   Mismatching error message is returned in *MISMATCH_DETAIL upon request,
   i.e. when MISMATCH_DETAIL is non-NULL.  This avoids the generation
   of error message during the disassembling where error message is not
   wanted.  We avoid the dynamic construction of strings of error messages
   here (i.e. in libopcodes), as it is costly and complicated; instead, we
   use a combination of error code, static string and some integer data to
   represent an error.  */

static int
operand_general_constraint_met_p (const aarch64_opnd_info *opnds, int idx,
				  enum aarch64_opnd type,
				  const aarch64_opcode *opcode,
				  aarch64_operand_error *mismatch_detail)
{
  unsigned num, modifiers, shift;
  unsigned char size;
  int64_t imm, min_value, max_value;
  uint64_t uvalue, mask;
  const aarch64_opnd_info *opnd = opnds + idx;
  aarch64_opnd_qualifier_t qualifier = opnd->qualifier;
  int i;

  assert (opcode->operands[idx] == opnd->type && opnd->type == type);

  switch (aarch64_operands[type].op_class)
    {
    case AARCH64_OPND_CLASS_INT_REG:
      /* Check for pair of xzr registers.  */
      if (type == AARCH64_OPND_PAIRREG_OR_XZR
	  && opnds[idx - 1].reg.regno == 0x1f)
	{
	  if (opnds[idx].reg.regno != 0x1f)
	    {
	      set_syntax_error (mismatch_detail, idx - 1,
				_("second reg in pair should be xzr if first is"
				  " xzr"));
	      return 0;
	    }
	}
      /* Check pair reg constraints for instructions taking a pair of
	 consecutively-numbered general-purpose registers.  */
      else if (type == AARCH64_OPND_PAIRREG
	       || type == AARCH64_OPND_PAIRREG_OR_XZR)
	{
	  assert (idx == 1 || idx == 2 || idx == 3 || idx == 5);
	  if (opnds[idx - 1].reg.regno % 2 != 0)
	    {
	      set_syntax_error (mismatch_detail, idx - 1,
				_("reg pair must start from even reg"));
	      return 0;
	    }
	  if (opnds[idx].reg.regno != opnds[idx - 1].reg.regno + 1)
	    {
	      set_syntax_error (mismatch_detail, idx,
				_("reg pair must be contiguous"));
	      return 0;
	    }
	  break;
	}

      /* <Xt> may be optional in some IC and TLBI instructions.  */
      if (type == AARCH64_OPND_Rt_SYS)
	{
	  assert (idx == 1 && (aarch64_get_operand_class (opnds[0].type)
			       == AARCH64_OPND_CLASS_SYSTEM));
	  if (opnds[1].present
	      && !aarch64_sys_ins_reg_has_xt (opnds[0].sysins_op))
	    {
	      set_other_error (mismatch_detail, idx, _("extraneous register"));
	      return 0;
	    }
	  if (!opnds[1].present
	      && aarch64_sys_ins_reg_has_xt (opnds[0].sysins_op))
	    {
	      set_other_error (mismatch_detail, idx, _("missing register"));
	      return 0;
	    }
	}
      switch (qualifier)
	{
	case AARCH64_OPND_QLF_WSP:
	case AARCH64_OPND_QLF_SP:
	  if (!aarch64_stack_pointer_p (opnd))
	    {
	      set_other_error (mismatch_detail, idx,
		       _("stack pointer register expected"));
	      return 0;
	    }
	  break;
	default:
	  break;
	}
      break;

    case AARCH64_OPND_CLASS_SVE_REG:
      switch (type)
	{
	case AARCH64_OPND_SVE_Zm3_INDEX:
	case AARCH64_OPND_SVE_Zm3_22_INDEX:
	case AARCH64_OPND_SVE_Zm3_19_INDEX:
	case AARCH64_OPND_SVE_Zm3_11_INDEX:
	case AARCH64_OPND_SVE_Zm4_11_INDEX:
	case AARCH64_OPND_SVE_Zm4_INDEX:
	  size = get_operand_fields_width (get_operand_from_code (type));
	  shift = get_operand_specific_data (&aarch64_operands[type]);
	  if (!check_reglane (opnd, mismatch_detail, idx,
			      "z", 0, (1 << shift) - 1,
			      0, (1u << (size - shift)) - 1))
	    return 0;
	  break;

	case AARCH64_OPND_SVE_Zn_INDEX:
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  if (!check_reglane (opnd, mismatch_detail, idx, "z", 0, 31,
			      0, 64 / size - 1))
	    return 0;
	  break;

	case AARCH64_OPND_SVE_Zm_imm4:
	  if (!check_reglane (opnd, mismatch_detail, idx, "z", 0, 31, 0, 15))
	    return 0;
	  break;

	case AARCH64_OPND_SVE_Zn_5_INDEX:
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  if (!check_reglane (opnd, mismatch_detail, idx, "z", 0, 31,
			      0, 16 / size - 1))
	    return 0;
	  break;

	case AARCH64_OPND_SME_PNn3_INDEX1:
	case AARCH64_OPND_SME_PNn3_INDEX2:
	  size = get_operand_field_width (get_operand_from_code (type), 1);
	  if (!check_reglane (opnd, mismatch_detail, idx, "pn", 8, 15,
			      0, (1 << size) - 1))
	    return 0;
	  break;

	case AARCH64_OPND_SME_Zn_INDEX1_16:
	case AARCH64_OPND_SME_Zn_INDEX2_15:
	case AARCH64_OPND_SME_Zn_INDEX2_16:
	case AARCH64_OPND_SME_Zn_INDEX3_14:
	case AARCH64_OPND_SME_Zn_INDEX3_15:
	case AARCH64_OPND_SME_Zn_INDEX4_14:
	  size = get_operand_fields_width (get_operand_from_code (type)) - 5;
	  if (!check_reglane (opnd, mismatch_detail, idx, "z", 0, 31,
			      0, (1 << size) - 1))
	    return 0;
	  break;

	case AARCH64_OPND_SME_Zm_INDEX1:
	case AARCH64_OPND_SME_Zm_INDEX2:
	case AARCH64_OPND_SME_Zm_INDEX3_1:
	case AARCH64_OPND_SME_Zm_INDEX3_2:
	case AARCH64_OPND_SME_Zm_INDEX3_10:
	case AARCH64_OPND_SME_Zm_INDEX4_1:
	case AARCH64_OPND_SME_Zm_INDEX4_10:
	  size = get_operand_fields_width (get_operand_from_code (type)) - 4;
	  if (!check_reglane (opnd, mismatch_detail, idx, "z", 0, 15,
			      0, (1 << size) - 1))
	    return 0;
	  break;

	case AARCH64_OPND_SME_Zm:
	  if (opnd->reg.regno > 15)
	    {
	      set_invalid_regno_error (mismatch_detail, idx, "z", 0, 15);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_PnT_Wm_imm:
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  max_value = 16 / size - 1;
	  if (!check_za_access (opnd, mismatch_detail, idx,
				12, max_value, 1, 0))
	    return 0;
	  break;

	default:
	  break;
	}
      break;

    case AARCH64_OPND_CLASS_SVE_REGLIST:
      switch (type)
	{
	case AARCH64_OPND_SME_Pdx2:
	case AARCH64_OPND_SME_Zdnx2:
	case AARCH64_OPND_SME_Zdnx4:
	case AARCH64_OPND_SME_Zmx2:
	case AARCH64_OPND_SME_Zmx4:
	case AARCH64_OPND_SME_Znx2:
	case AARCH64_OPND_SME_Znx4:
	case AARCH64_OPND_SME_Zt2:
	case AARCH64_OPND_SME_Zt3:
	case AARCH64_OPND_SME_Zt4:
	  num = get_operand_specific_data (&aarch64_operands[type]);
	  if (!check_reglist (opnd, mismatch_detail, idx, num, 1))
	    return 0;
	  if ((opnd->reglist.first_regno % num) != 0)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("start register out of range"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_Ztx2_STRIDED:
	case AARCH64_OPND_SME_Ztx4_STRIDED:
	  /* 2-register lists have a stride of 8 and 4-register lists
	     have a stride of 4.  */
	  num = get_operand_specific_data (&aarch64_operands[type]);
	  if (!check_reglist (opnd, mismatch_detail, idx, num, 16 / num))
	    return 0;
	  num = 16 | (opnd->reglist.stride - 1);
	  if ((opnd->reglist.first_regno & ~num) != 0)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("start register out of range"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_PdxN:
	case AARCH64_OPND_SVE_ZnxN:
	case AARCH64_OPND_SVE_ZtxN:
	  num = get_opcode_dependent_value (opcode);
	  if (!check_reglist (opnd, mismatch_detail, idx, num, 1))
	    return 0;
	  break;

	default:
	  abort ();
	}
      break;

    case AARCH64_OPND_CLASS_ZA_ACCESS:
      switch (type)
	{
	case AARCH64_OPND_SME_ZA_HV_idx_src:
	case AARCH64_OPND_SME_ZA_HV_idx_dest:
	case AARCH64_OPND_SME_ZA_HV_idx_ldstr:
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  max_value = 16 / size - 1;
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, max_value, 1,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_off4:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 15, 1,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_off3_0:
	case AARCH64_OPND_SME_ZA_array_off3_5:
	  if (!check_za_access (opnd, mismatch_detail, idx, 8, 7, 1,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_off1x4:
	  if (!check_za_access (opnd, mismatch_detail, idx, 8, 1, 4,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_off2x2:
	  if (!check_za_access (opnd, mismatch_detail, idx, 8, 3, 2,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_off2x4:
	  if (!check_za_access (opnd, mismatch_detail, idx, 8, 3, 4,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_off3x2:
	  if (!check_za_access (opnd, mismatch_detail, idx, 8, 7, 2,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrsb_1:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 7, 2,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrsh_1:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 3, 2,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrss_1:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 1, 2,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrsd_1:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 0, 2,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrsb_2:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 3, 4,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrsh_2:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 1, 4,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_array_vrss_2:
	case AARCH64_OPND_SME_ZA_array_vrsd_2:
	  if (!check_za_access (opnd, mismatch_detail, idx, 12, 0, 4,
				get_opcode_dependent_value (opcode)))
	    return 0;
	  break;

	case AARCH64_OPND_SME_ZA_HV_idx_srcxN:
	case AARCH64_OPND_SME_ZA_HV_idx_destxN:
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  num = get_opcode_dependent_value (opcode);
	  max_value = 16 / num / size;
	  if (max_value > 0)
	    max_value -= 1;
	  if (!check_za_access (opnd, mismatch_detail, idx,
				12, max_value, num, 0))
	    return 0;
	  break;

	default:
	  abort ();
	}
      break;

    case AARCH64_OPND_CLASS_PRED_REG:
      switch (type)
	{
	case AARCH64_OPND_SME_PNd3:
	case AARCH64_OPND_SME_PNg3:
	  if (opnd->reg.regno < 8)
	    {
	      set_invalid_regno_error (mismatch_detail, idx, "pn", 8, 15);
	      return 0;
	    }
	  break;

	default:
	  if (opnd->reg.regno >= 8
	      && get_operand_fields_width (get_operand_from_code (type)) == 3)
	    {
	      set_invalid_regno_error (mismatch_detail, idx, "p", 0, 7);
	      return 0;
	    }
	  break;
	}
      break;

    case AARCH64_OPND_CLASS_COND:
      if (type == AARCH64_OPND_COND1
	  && (opnds[idx].cond->value & 0xe) == 0xe)
	{
	  /* Not allow AL or NV.  */
	  set_syntax_error (mismatch_detail, idx, NULL);
	}
      break;

    case AARCH64_OPND_CLASS_ADDRESS:
      /* Check writeback.  */
      switch (opcode->iclass)
	{
	case ldst_pos:
	case ldst_unscaled:
	case ldstnapair_offs:
	case ldstpair_off:
	case ldst_unpriv:
	  if (opnd->addr.writeback == 1)
	    {
	      set_syntax_error (mismatch_detail, idx,
				_("unexpected address writeback"));
	      return 0;
	    }
	  break;
	case ldst_imm10:
	  if (opnd->addr.writeback == 1 && opnd->addr.preind != 1)
	    {
	      set_syntax_error (mismatch_detail, idx,
				_("unexpected address writeback"));
	      return 0;
	    }
	  break;
	case ldst_imm9:
	case ldstpair_indexed:
	case asisdlsep:
	case asisdlsop:
	  if (opnd->addr.writeback == 0)
	    {
	      set_syntax_error (mismatch_detail, idx,
				_("address writeback expected"));
	      return 0;
	    }
	  break;
	case rcpc3:
	  if (opnd->addr.writeback)
	    if ((type == AARCH64_OPND_RCPC3_ADDR_PREIND_WB
		 && !opnd->addr.preind)
		|| (type == AARCH64_OPND_RCPC3_ADDR_POSTIND
		    && !opnd->addr.postind))
	      {
		set_syntax_error (mismatch_detail, idx,
				  _("unexpected address writeback"));
		return 0;
	      }

	  break;
	default:
	  assert (opnd->addr.writeback == 0);
	  break;
	}
      switch (type)
	{
	case AARCH64_OPND_ADDR_SIMM7:
	  /* Scaled signed 7 bits immediate offset.  */
	  /* Get the size of the data element that is accessed, which may be
	     different from that of the source register size,
	     e.g. in strb/ldrb.  */
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  if (!value_in_range_p (opnd->addr.offset.imm, -64 * size, 63 * size))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx,
					     -64 * size, 63 * size);
	      return 0;
	    }
	  if (!value_aligned_p (opnd->addr.offset.imm, size))
	    {
	      set_unaligned_error (mismatch_detail, idx, size);
	      return 0;
	    }
	  break;
	case AARCH64_OPND_ADDR_OFFSET:
	case AARCH64_OPND_ADDR_SIMM9:
	  /* Unscaled signed 9 bits immediate offset.  */
	  if (!value_in_range_p (opnd->addr.offset.imm, -256, 255))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx, -256, 255);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_ADDR_SIMM9_2:
	  /* Unscaled signed 9 bits immediate offset, which has to be negative
	     or unaligned.  */
	  size = aarch64_get_qualifier_esize (qualifier);
	  if ((value_in_range_p (opnd->addr.offset.imm, 0, 255)
	       && !value_aligned_p (opnd->addr.offset.imm, size))
	      || value_in_range_p (opnd->addr.offset.imm, -256, -1))
	    return 1;
	  set_other_error (mismatch_detail, idx,
			   _("negative or unaligned offset expected"));
	  return 0;

	case AARCH64_OPND_ADDR_SIMM10:
	  /* Scaled signed 10 bits immediate offset.  */
	  if (!value_in_range_p (opnd->addr.offset.imm, -4096, 4088))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx, -4096, 4088);
	      return 0;
	    }
	  if (!value_aligned_p (opnd->addr.offset.imm, 8))
	    {
	      set_unaligned_error (mismatch_detail, idx, 8);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_ADDR_SIMM11:
	  /* Signed 11 bits immediate offset (multiple of 16).  */
	  if (!value_in_range_p (opnd->addr.offset.imm, -1024, 1008))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx, -1024, 1008);
	      return 0;
	    }

	  if (!value_aligned_p (opnd->addr.offset.imm, 16))
	    {
	      set_unaligned_error (mismatch_detail, idx, 16);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_ADDR_SIMM13:
	  /* Signed 13 bits immediate offset (multiple of 16).  */
	  if (!value_in_range_p (opnd->addr.offset.imm, -4096, 4080))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx, -4096, 4080);
	      return 0;
	    }

	  if (!value_aligned_p (opnd->addr.offset.imm, 16))
	    {
	      set_unaligned_error (mismatch_detail, idx, 16);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SIMD_ADDR_POST:
	  /* AdvSIMD load/store multiple structures, post-index.  */
	  assert (idx == 1);
	  if (opnd->addr.offset.is_reg)
	    {
	      if (value_in_range_p (opnd->addr.offset.regno, 0, 30))
		return 1;
	      else
		{
		  set_other_error (mismatch_detail, idx,
				   _("invalid register offset"));
		  return 0;
		}
	    }
	  else
	    {
	      const aarch64_opnd_info *prev = &opnds[idx-1];
	      unsigned num_bytes; /* total number of bytes transferred.  */
	      /* The opcode dependent area stores the number of elements in
		 each structure to be loaded/stored.  */
	      int is_ld1r = get_opcode_dependent_value (opcode) == 1;
	      if (opcode->operands[0] == AARCH64_OPND_LVt_AL)
		/* Special handling of loading single structure to all lane.  */
		num_bytes = (is_ld1r ? 1 : prev->reglist.num_regs)
		  * aarch64_get_qualifier_esize (prev->qualifier);
	      else
		num_bytes = prev->reglist.num_regs
		  * aarch64_get_qualifier_esize (prev->qualifier)
		  * aarch64_get_qualifier_nelem (prev->qualifier);
	      if ((int) num_bytes != opnd->addr.offset.imm)
		{
		  set_other_error (mismatch_detail, idx,
				   _("invalid post-increment amount"));
		  return 0;
		}
	    }
	  break;

	case AARCH64_OPND_ADDR_REGOFF:
	  /* Get the size of the data element that is accessed, which may be
	     different from that of the source register size,
	     e.g. in strb/ldrb.  */
	  size = aarch64_get_qualifier_esize (opnd->qualifier);
	  /* It is either no shift or shift by the binary logarithm of SIZE.  */
	  if (opnd->shifter.amount != 0
	      && opnd->shifter.amount != (int)get_logsz (size))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid shift amount"));
	      return 0;
	    }
	  /* Only UXTW, LSL, SXTW and SXTX are the accepted extending
	     operators.  */
	  switch (opnd->shifter.kind)
	    {
	    case AARCH64_MOD_UXTW:
	    case AARCH64_MOD_LSL:
	    case AARCH64_MOD_SXTW:
	    case AARCH64_MOD_SXTX: break;
	    default:
	      set_other_error (mismatch_detail, idx,
			       _("invalid extend/shift operator"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_ADDR_UIMM12:
	  imm = opnd->addr.offset.imm;
	  /* Get the size of the data element that is accessed, which may be
	     different from that of the source register size,
	     e.g. in strb/ldrb.  */
	  size = aarch64_get_qualifier_esize (qualifier);
	  if (!value_in_range_p (opnd->addr.offset.imm, 0, 4095 * size))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx,
					     0, 4095 * size);
	      return 0;
	    }
	  if (!value_aligned_p (opnd->addr.offset.imm, size))
	    {
	      set_unaligned_error (mismatch_detail, idx, size);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_ADDR_PCREL14:
	case AARCH64_OPND_ADDR_PCREL19:
	case AARCH64_OPND_ADDR_PCREL21:
	case AARCH64_OPND_ADDR_PCREL26:
	  imm = opnd->imm.value;
	  if (operand_need_shift_by_two (get_operand_from_code (type)))
	    {
	      /* The offset value in a PC-relative branch instruction is alway
		 4-byte aligned and is encoded without the lowest 2 bits.  */
	      if (!value_aligned_p (imm, 4))
		{
		  set_unaligned_error (mismatch_detail, idx, 4);
		  return 0;
		}
	      /* Right shift by 2 so that we can carry out the following check
		 canonically.  */
	      imm >>= 2;
	    }
	  size = get_operand_fields_width (get_operand_from_code (type));
	  if (!value_fit_signed_field_p (imm, size))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("immediate out of range"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_ADDR_RI_U4xVL:
	  if (!value_in_range_p (opnd->addr.offset.imm, 0, 15))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx, 0, 15);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_ADDR_RI_S4xVL:
	case AARCH64_OPND_SVE_ADDR_RI_S4x2xVL:
	case AARCH64_OPND_SVE_ADDR_RI_S4x3xVL:
	case AARCH64_OPND_SVE_ADDR_RI_S4x4xVL:
	  min_value = -8;
	  max_value = 7;
	sve_imm_offset_vl:
	  assert (!opnd->addr.offset.is_reg);
	  assert (opnd->addr.preind);
	  num = 1 + get_operand_specific_data (&aarch64_operands[type]);
	  min_value *= num;
	  max_value *= num;
	  if ((opnd->addr.offset.imm != 0 && !opnd->shifter.operator_present)
	      || (opnd->shifter.operator_present
		  && opnd->shifter.kind != AARCH64_MOD_MUL_VL))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid addressing mode"));
	      return 0;
	    }
	  if (!value_in_range_p (opnd->addr.offset.imm, min_value, max_value))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx,
					     min_value, max_value);
	      return 0;
	    }
	  if (!value_aligned_p (opnd->addr.offset.imm, num))
	    {
	      set_unaligned_error (mismatch_detail, idx, num);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_ADDR_RI_S6xVL:
	  min_value = -32;
	  max_value = 31;
	  goto sve_imm_offset_vl;

	case AARCH64_OPND_SVE_ADDR_RI_S9xVL:
	  min_value = -256;
	  max_value = 255;
	  goto sve_imm_offset_vl;

	case AARCH64_OPND_SVE_ADDR_RI_U6:
	case AARCH64_OPND_SVE_ADDR_RI_U6x2:
	case AARCH64_OPND_SVE_ADDR_RI_U6x4:
	case AARCH64_OPND_SVE_ADDR_RI_U6x8:
	  min_value = 0;
	  max_value = 63;
	sve_imm_offset:
	  assert (!opnd->addr.offset.is_reg);
	  assert (opnd->addr.preind);
	  num = 1 << get_operand_specific_data (&aarch64_operands[type]);
	  min_value *= num;
	  max_value *= num;
	  if (opnd->shifter.operator_present
	      || opnd->shifter.amount_present)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid addressing mode"));
	      return 0;
	    }
	  if (!value_in_range_p (opnd->addr.offset.imm, min_value, max_value))
	    {
	      set_offset_out_of_range_error (mismatch_detail, idx,
					     min_value, max_value);
	      return 0;
	    }
	  if (!value_aligned_p (opnd->addr.offset.imm, num))
	    {
	      set_unaligned_error (mismatch_detail, idx, num);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_ADDR_RI_S4x16:
	case AARCH64_OPND_SVE_ADDR_RI_S4x32:
	  min_value = -8;
	  max_value = 7;
	  goto sve_imm_offset;

	case AARCH64_OPND_SVE_ADDR_ZX:
	  /* Everything is already ensured by parse_operands or
	     aarch64_ext_sve_addr_rr_lsl (because this is a very specific
	     argument type).  */
	  assert (opnd->addr.offset.is_reg);
	  assert (opnd->addr.preind);
	  assert ((aarch64_operands[type].flags & OPD_F_NO_ZR) == 0);
	  assert (opnd->shifter.kind == AARCH64_MOD_LSL);
	  assert (opnd->shifter.operator_present == 0);
	  break;

	case AARCH64_OPND_SVE_ADDR_R:
	case AARCH64_OPND_SVE_ADDR_RR:
	case AARCH64_OPND_SVE_ADDR_RR_LSL1:
	case AARCH64_OPND_SVE_ADDR_RR_LSL2:
	case AARCH64_OPND_SVE_ADDR_RR_LSL3:
	case AARCH64_OPND_SVE_ADDR_RR_LSL4:
	case AARCH64_OPND_SVE_ADDR_RX:
	case AARCH64_OPND_SVE_ADDR_RX_LSL1:
	case AARCH64_OPND_SVE_ADDR_RX_LSL2:
	case AARCH64_OPND_SVE_ADDR_RX_LSL3:
	case AARCH64_OPND_SVE_ADDR_RZ:
	case AARCH64_OPND_SVE_ADDR_RZ_LSL1:
	case AARCH64_OPND_SVE_ADDR_RZ_LSL2:
	case AARCH64_OPND_SVE_ADDR_RZ_LSL3:
	  modifiers = 1 << AARCH64_MOD_LSL;
	sve_rr_operand:
	  assert (opnd->addr.offset.is_reg);
	  assert (opnd->addr.preind);
	  if ((aarch64_operands[type].flags & OPD_F_NO_ZR) != 0
	      && opnd->addr.offset.regno == 31)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("index register xzr is not allowed"));
	      return 0;
	    }
	  if (((1 << opnd->shifter.kind) & modifiers) == 0
	      || (opnd->shifter.amount
		  != get_operand_specific_data (&aarch64_operands[type])))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid addressing mode"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_ADDR_RZ_XTW_14:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW_22:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW1_14:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW1_22:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW2_14:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW2_22:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW3_14:
	case AARCH64_OPND_SVE_ADDR_RZ_XTW3_22:
	  modifiers = (1 << AARCH64_MOD_SXTW) | (1 << AARCH64_MOD_UXTW);
	  goto sve_rr_operand;

	case AARCH64_OPND_SVE_ADDR_ZI_U5:
	case AARCH64_OPND_SVE_ADDR_ZI_U5x2:
	case AARCH64_OPND_SVE_ADDR_ZI_U5x4:
	case AARCH64_OPND_SVE_ADDR_ZI_U5x8:
	  min_value = 0;
	  max_value = 31;
	  goto sve_imm_offset;

	case AARCH64_OPND_SVE_ADDR_ZZ_LSL:
	  modifiers = 1 << AARCH64_MOD_LSL;
	sve_zz_operand:
	  assert (opnd->addr.offset.is_reg);
	  assert (opnd->addr.preind);
	  if (((1 << opnd->shifter.kind) & modifiers) == 0
	      || opnd->shifter.amount < 0
	      || opnd->shifter.amount > 3)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid addressing mode"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_ADDR_ZZ_SXTW:
	  modifiers = (1 << AARCH64_MOD_SXTW);
	  goto sve_zz_operand;

	case AARCH64_OPND_SVE_ADDR_ZZ_UXTW:
	  modifiers = 1 << AARCH64_MOD_UXTW;
	  goto sve_zz_operand;

	case AARCH64_OPND_RCPC3_ADDR_OPT_PREIND_WB:
	case AARCH64_OPND_RCPC3_ADDR_OPT_POSTIND:
	case AARCH64_OPND_RCPC3_ADDR_PREIND_WB:
	case AARCH64_OPND_RCPC3_ADDR_POSTIND:
	  {
	    int num_bytes = calc_ldst_datasize (opnds);
	    int abs_offset = (type == AARCH64_OPND_RCPC3_ADDR_OPT_PREIND_WB
			      || type == AARCH64_OPND_RCPC3_ADDR_PREIND_WB)
	      ? opnd->addr.offset.imm * -1
	      : opnd->addr.offset.imm;
	    if ((int) num_bytes != abs_offset
		&& opnd->addr.offset.imm != 0)
	      {
		set_other_error (mismatch_detail, idx,
				 _("invalid increment amount"));
		return 0;
	      }
	  }
	  break;

	case AARCH64_OPND_RCPC3_ADDR_OFFSET:
	  if (!value_in_range_p (opnd->addr.offset.imm, -256, 255))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, -256, 255);
	      return 0;
	    }

	default:
	  break;
	}
      break;

    case AARCH64_OPND_CLASS_SIMD_REGLIST:
      if (type == AARCH64_OPND_LEt)
	{
	  /* Get the upper bound for the element index.  */
	  num = 16 / aarch64_get_qualifier_esize (qualifier) - 1;
	  if (!value_in_range_p (opnd->reglist.index, 0, num))
	    {
	      set_elem_idx_out_of_range_error (mismatch_detail, idx, 0, num);
	      return 0;
	    }
	}
      /* The opcode dependent area stores the number of elements in
	 each structure to be loaded/stored.  */
      num = get_opcode_dependent_value (opcode);
      switch (type)
	{
	case AARCH64_OPND_LVt:
	  assert (num >= 1 && num <= 4);
	  /* Unless LD1/ST1, the number of registers should be equal to that
	     of the structure elements.  */
	  if (num != 1 && !check_reglist (opnd, mismatch_detail, idx, num, 1))
	    return 0;
	  break;
	case AARCH64_OPND_LVt_AL:
	case AARCH64_OPND_LEt:
	  assert (num >= 1 && num <= 4);
	  /* The number of registers should be equal to that of the structure
	     elements.  */
	  if (!check_reglist (opnd, mismatch_detail, idx, num, 1))
	    return 0;
	  break;
	default:
	  break;
	}
      if (opnd->reglist.stride != 1)
	{
	  set_reg_list_stride_error (mismatch_detail, idx, 1);
	  return 0;
	}
      break;

    case AARCH64_OPND_CLASS_IMMEDIATE:
      /* Constraint check on immediate operand.  */
      imm = opnd->imm.value;
      /* E.g. imm_0_31 constrains value to be 0..31.  */
      if (qualifier_value_in_range_constraint_p (qualifier)
	  && !value_in_range_p (imm, get_lower_bound (qualifier),
				get_upper_bound (qualifier)))
	{
	  set_imm_out_of_range_error (mismatch_detail, idx,
				      get_lower_bound (qualifier),
				      get_upper_bound (qualifier));
	  return 0;
	}

      switch (type)
	{
	case AARCH64_OPND_AIMM:
	  if (opnd->shifter.kind != AARCH64_MOD_LSL)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid shift operator"));
	      return 0;
	    }
	  if (opnd->shifter.amount != 0 && opnd->shifter.amount != 12)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("shift amount must be 0 or 12"));
	      return 0;
	    }
	  if (!value_fit_unsigned_field_p (opnd->imm.value, 12))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("immediate out of range"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_HALF:
	  assert (idx == 1 && opnds[0].type == AARCH64_OPND_Rd);
	  if (opnd->shifter.kind != AARCH64_MOD_LSL)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid shift operator"));
	      return 0;
	    }
	  size = aarch64_get_qualifier_esize (opnds[0].qualifier);
	  if (!value_aligned_p (opnd->shifter.amount, 16))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("shift amount must be a multiple of 16"));
	      return 0;
	    }
	  if (!value_in_range_p (opnd->shifter.amount, 0, size * 8 - 16))
	    {
	      set_sft_amount_out_of_range_error (mismatch_detail, idx,
						 0, size * 8 - 16);
	      return 0;
	    }
	  if (opnd->imm.value < 0)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("negative immediate value not allowed"));
	      return 0;
	    }
	  if (!value_fit_unsigned_field_p (opnd->imm.value, 16))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("immediate out of range"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_IMM_MOV:
	    {
	      int esize = aarch64_get_qualifier_esize (opnds[0].qualifier);
	      imm = opnd->imm.value;
	      assert (idx == 1);
	      switch (opcode->op)
		{
		case OP_MOV_IMM_WIDEN:
		  imm = ~imm;
		  /* Fall through.  */
		case OP_MOV_IMM_WIDE:
		  if (!aarch64_wide_constant_p (imm, esize == 4, NULL))
		    {
		      set_other_error (mismatch_detail, idx,
				       _("immediate out of range"));
		      return 0;
		    }
		  break;
		case OP_MOV_IMM_LOG:
		  if (!aarch64_logical_immediate_p (imm, esize, NULL))
		    {
		      set_other_error (mismatch_detail, idx,
				       _("immediate out of range"));
		      return 0;
		    }
		  break;
		default:
		  assert (0);
		  return 0;
		}
	    }
	  break;

	case AARCH64_OPND_NZCV:
	case AARCH64_OPND_CCMP_IMM:
	case AARCH64_OPND_EXCEPTION:
	case AARCH64_OPND_UNDEFINED:
	case AARCH64_OPND_TME_UIMM16:
	case AARCH64_OPND_UIMM4:
	case AARCH64_OPND_UIMM4_ADDG:
	case AARCH64_OPND_UIMM7:
	case AARCH64_OPND_UIMM3_OP1:
	case AARCH64_OPND_UIMM3_OP2:
	case AARCH64_OPND_SVE_UIMM3:
	case AARCH64_OPND_SVE_UIMM7:
	case AARCH64_OPND_SVE_UIMM8:
	case AARCH64_OPND_SVE_UIMM8_53:
	case AARCH64_OPND_CSSC_UIMM8:
	  size = get_operand_fields_width (get_operand_from_code (type));
	  assert (size < 32);
	  if (!value_fit_unsigned_field_p (opnd->imm.value, size))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 0,
					  (1u << size) - 1);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_UIMM10:
	  /* Scaled unsigned 10 bits immediate offset.  */
	  if (!value_in_range_p (opnd->imm.value, 0, 1008))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 0, 1008);
	      return 0;
	    }

	  if (!value_aligned_p (opnd->imm.value, 16))
	    {
	      set_unaligned_error (mismatch_detail, idx, 16);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SIMM5:
	case AARCH64_OPND_SVE_SIMM5:
	case AARCH64_OPND_SVE_SIMM5B:
	case AARCH64_OPND_SVE_SIMM6:
	case AARCH64_OPND_SVE_SIMM8:
	case AARCH64_OPND_CSSC_SIMM8:
	  size = get_operand_fields_width (get_operand_from_code (type));
	  assert (size < 32);
	  if (!value_fit_signed_field_p (opnd->imm.value, size))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx,
					  -(1 << (size - 1)),
					  (1 << (size - 1)) - 1);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_WIDTH:
	  assert (idx > 1 && opnds[idx-1].type == AARCH64_OPND_IMM
		  && opnds[0].type == AARCH64_OPND_Rd);
	  size = get_upper_bound (qualifier);
	  if (opnd->imm.value + opnds[idx-1].imm.value > size)
	    /* lsb+width <= reg.size  */
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 1,
					  size - opnds[idx-1].imm.value);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_LIMM:
	case AARCH64_OPND_SVE_LIMM:
	  {
	    int esize = aarch64_get_qualifier_esize (opnds[0].qualifier);
	    uint64_t uimm = opnd->imm.value;
	    if (opcode->op == OP_BIC)
	      uimm = ~uimm;
	    if (!aarch64_logical_immediate_p (uimm, esize, NULL))
	      {
		set_other_error (mismatch_detail, idx,
				 _("immediate out of range"));
		return 0;
	      }
	  }
	  break;

	case AARCH64_OPND_IMM0:
	case AARCH64_OPND_FPIMM0:
	  if (opnd->imm.value != 0)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("immediate zero expected"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_IMM_ROT1:
	case AARCH64_OPND_IMM_ROT2:
	case AARCH64_OPND_SVE_IMM_ROT2:
	  if (opnd->imm.value != 0
	      && opnd->imm.value != 90
	      && opnd->imm.value != 180
	      && opnd->imm.value != 270)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("rotate expected to be 0, 90, 180 or 270"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_IMM_ROT3:
	case AARCH64_OPND_SVE_IMM_ROT1:
	case AARCH64_OPND_SVE_IMM_ROT3:
	  if (opnd->imm.value != 90 && opnd->imm.value != 270)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("rotate expected to be 90 or 270"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SHLL_IMM:
	  assert (idx == 2);
	  size = 8 * aarch64_get_qualifier_esize (opnds[idx - 1].qualifier);
	  if (opnd->imm.value != size)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid shift amount"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_IMM_VLSL:
	  size = aarch64_get_qualifier_esize (qualifier);
	  if (!value_in_range_p (opnd->imm.value, 0, size * 8 - 1))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 0,
					  size * 8 - 1);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_IMM_VLSR:
	  size = aarch64_get_qualifier_esize (qualifier);
	  if (!value_in_range_p (opnd->imm.value, 1, size * 8))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 1, size * 8);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SIMD_IMM:
	case AARCH64_OPND_SIMD_IMM_SFT:
	  /* Qualifier check.  */
	  switch (qualifier)
	    {
	    case AARCH64_OPND_QLF_LSL:
	      if (opnd->shifter.kind != AARCH64_MOD_LSL)
		{
		  set_other_error (mismatch_detail, idx,
				   _("invalid shift operator"));
		  return 0;
		}
	      break;
	    case AARCH64_OPND_QLF_MSL:
	      if (opnd->shifter.kind != AARCH64_MOD_MSL)
		{
		  set_other_error (mismatch_detail, idx,
				   _("invalid shift operator"));
		  return 0;
		}
	      break;
	    case AARCH64_OPND_QLF_NIL:
	      if (opnd->shifter.kind != AARCH64_MOD_NONE)
		{
		  set_other_error (mismatch_detail, idx,
				   _("shift is not permitted"));
		  return 0;
		}
	      break;
	    default:
	      assert (0);
	      return 0;
	    }
	  /* Is the immediate valid?  */
	  assert (idx == 1);
	  if (aarch64_get_qualifier_esize (opnds[0].qualifier) != 8)
	    {
	      /* uimm8 or simm8 */
	      if (!value_in_range_p (opnd->imm.value, -128, 255))
		{
		  set_imm_out_of_range_error (mismatch_detail, idx, -128, 255);
		  return 0;
		}
	    }
	  else if (aarch64_shrink_expanded_imm8 (opnd->imm.value) < 0)
	    {
	      /* uimm64 is not
		 'aaaaaaaabbbbbbbbccccccccddddddddeeeeeeee
		 ffffffffgggggggghhhhhhhh'.  */
	      set_other_error (mismatch_detail, idx,
			       _("invalid value for immediate"));
	      return 0;
	    }
	  /* Is the shift amount valid?  */
	  switch (opnd->shifter.kind)
	    {
	    case AARCH64_MOD_LSL:
	      size = aarch64_get_qualifier_esize (opnds[0].qualifier);
	      if (!value_in_range_p (opnd->shifter.amount, 0, (size - 1) * 8))
		{
		  set_sft_amount_out_of_range_error (mismatch_detail, idx, 0,
						     (size - 1) * 8);
		  return 0;
		}
	      if (!value_aligned_p (opnd->shifter.amount, 8))
		{
		  set_unaligned_error (mismatch_detail, idx, 8);
		  return 0;
		}
	      break;
	    case AARCH64_MOD_MSL:
	      /* Only 8 and 16 are valid shift amount.  */
	      if (opnd->shifter.amount != 8 && opnd->shifter.amount != 16)
		{
		  set_other_error (mismatch_detail, idx,
				   _("shift amount must be 0 or 16"));
		  return 0;
		}
	      break;
	    default:
	      if (opnd->shifter.kind != AARCH64_MOD_NONE)
		{
		  set_other_error (mismatch_detail, idx,
				   _("invalid shift operator"));
		  return 0;
		}
	      break;
	    }
	  break;

	case AARCH64_OPND_FPIMM:
	case AARCH64_OPND_SIMD_FPIMM:
	case AARCH64_OPND_SVE_FPIMM8:
	  if (opnd->imm.is_fp == 0)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("floating-point immediate expected"));
	      return 0;
	    }
	  /* The value is expected to be an 8-bit floating-point constant with
	     sign, 3-bit exponent and normalized 4 bits of precision, encoded
	     in "a:b:c:d:e:f:g:h" or FLD_imm8 (depending on the type of the
	     instruction).  */
	  if (!value_in_range_p (opnd->imm.value, 0, 255))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("immediate out of range"));
	      return 0;
	    }
	  if (opnd->shifter.kind != AARCH64_MOD_NONE)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid shift operator"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_AIMM:
	  min_value = 0;
	sve_aimm:
	  assert (opnd->shifter.kind == AARCH64_MOD_LSL);
	  size = aarch64_get_qualifier_esize (opnds[0].qualifier);
	  mask = ~((uint64_t) -1 << (size * 4) << (size * 4));
	  uvalue = opnd->imm.value;
	  shift = opnd->shifter.amount;
	  if (size == 1)
	    {
	      if (shift != 0)
		{
		  set_other_error (mismatch_detail, idx,
				   _("no shift amount allowed for"
				     " 8-bit constants"));
		  return 0;
		}
	    }
	  else
	    {
	      if (shift != 0 && shift != 8)
		{
		  set_other_error (mismatch_detail, idx,
				   _("shift amount must be 0 or 8"));
		  return 0;
		}
	      if (shift == 0 && (uvalue & 0xff) == 0)
		{
		  shift = 8;
		  uvalue = (int64_t) uvalue / 256;
		}
	    }
	  mask >>= shift;
	  if ((uvalue & mask) != uvalue && (uvalue | ~mask) != uvalue)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("immediate too big for element size"));
	      return 0;
	    }
	  uvalue = (uvalue - min_value) & mask;
	  if (uvalue > 0xff)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("invalid arithmetic immediate"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_ASIMM:
	  min_value = -128;
	  goto sve_aimm;

	case AARCH64_OPND_SVE_I1_HALF_ONE:
	  assert (opnd->imm.is_fp);
	  if (opnd->imm.value != 0x3f000000 && opnd->imm.value != 0x3f800000)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("floating-point value must be 0.5 or 1.0"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_I1_HALF_TWO:
	  assert (opnd->imm.is_fp);
	  if (opnd->imm.value != 0x3f000000 && opnd->imm.value != 0x40000000)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("floating-point value must be 0.5 or 2.0"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_I1_ZERO_ONE:
	  assert (opnd->imm.is_fp);
	  if (opnd->imm.value != 0 && opnd->imm.value != 0x3f800000)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("floating-point value must be 0.0 or 1.0"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_INV_LIMM:
	  {
	    int esize = aarch64_get_qualifier_esize (opnds[0].qualifier);
	    uint64_t uimm = ~opnd->imm.value;
	    if (!aarch64_logical_immediate_p (uimm, esize, NULL))
	      {
		set_other_error (mismatch_detail, idx,
				 _("immediate out of range"));
		return 0;
	      }
	  }
	  break;

	case AARCH64_OPND_SVE_LIMM_MOV:
	  {
	    int esize = aarch64_get_qualifier_esize (opnds[0].qualifier);
	    uint64_t uimm = opnd->imm.value;
	    if (!aarch64_logical_immediate_p (uimm, esize, NULL))
	      {
		set_other_error (mismatch_detail, idx,
				 _("immediate out of range"));
		return 0;
	      }
	    if (!aarch64_sve_dupm_mov_immediate_p (uimm, esize))
	      {
		set_other_error (mismatch_detail, idx,
				 _("invalid replicated MOV immediate"));
		return 0;
	      }
	  }
	  break;

	case AARCH64_OPND_SVE_PATTERN_SCALED:
	  assert (opnd->shifter.kind == AARCH64_MOD_MUL);
	  if (!value_in_range_p (opnd->shifter.amount, 1, 16))
	    {
	      set_multiplier_out_of_range_error (mismatch_detail, idx, 1, 16);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SVE_SHLIMM_PRED:
	case AARCH64_OPND_SVE_SHLIMM_UNPRED:
	case AARCH64_OPND_SVE_SHLIMM_UNPRED_22:
	  size = aarch64_get_qualifier_esize (opnds[idx - 1].qualifier);
	  if (!value_in_range_p (opnd->imm.value, 0, 8 * size - 1))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx,
					  0, 8 * size - 1);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_SHRIMM4:
	  size = 1 << get_operand_fields_width (get_operand_from_code (type));
	  if (!value_in_range_p (opnd->imm.value, 1, size))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 1, size);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_SHRIMM5:
	case AARCH64_OPND_SVE_SHRIMM_PRED:
	case AARCH64_OPND_SVE_SHRIMM_UNPRED:
	case AARCH64_OPND_SVE_SHRIMM_UNPRED_22:
	  num = (type == AARCH64_OPND_SVE_SHRIMM_UNPRED_22) ? 2 : 1;
	  size = aarch64_get_qualifier_esize (opnds[idx - num].qualifier);
	  if (!value_in_range_p (opnd->imm.value, 1, 8 * size))
	    {
	      set_imm_out_of_range_error (mismatch_detail, idx, 1, 8*size);
	      return 0;
	    }
	  break;

	case AARCH64_OPND_SME_ZT0_INDEX:
	  if (!value_in_range_p (opnd->imm.value, 0, 56))
	    {
	      set_elem_idx_out_of_range_error (mismatch_detail, idx, 0, 56);
	      return 0;
	    }
	  if (opnd->imm.value % 8 != 0)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("byte index must be a multiple of 8"));
	      return 0;
	    }
	  break;

	default:
	  break;
	}
      break;

    case AARCH64_OPND_CLASS_SYSTEM:
      switch (type)
	{
	case AARCH64_OPND_PSTATEFIELD:
	  for (i = 0; aarch64_pstatefields[i].name; ++i)
	    if (aarch64_pstatefields[i].value == opnd->pstatefield)
	      break;
	  assert (aarch64_pstatefields[i].name);
	  assert (idx == 0 && opnds[1].type == AARCH64_OPND_UIMM4);
	  max_value = F_GET_REG_MAX_VALUE (aarch64_pstatefields[i].flags);
	  if (opnds[1].imm.value < 0 || opnds[1].imm.value > max_value)
	    {
	      set_imm_out_of_range_error (mismatch_detail, 1, 0, max_value);
	      return 0;
	    }
	  break;
	case AARCH64_OPND_PRFOP:
	  if (opcode->iclass == ldst_regoff && opnd->prfop->value >= 24)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("the register-index form of PRFM does"
				 " not accept opcodes in the range 24-31"));
	      return 0;
	    }
	  break;
	default:
	  break;
	}
      break;

    case AARCH64_OPND_CLASS_SIMD_ELEMENT:
      /* Get the upper bound for the element index.  */
      if (opcode->op == OP_FCMLA_ELEM)
	/* FCMLA index range depends on the vector size of other operands
	   and is halfed because complex numbers take two elements.  */
	num = aarch64_get_qualifier_nelem (opnds[0].qualifier)
	      * aarch64_get_qualifier_esize (opnds[0].qualifier) / 2;
      else
	num = 16;
      num = num / aarch64_get_qualifier_esize (qualifier) - 1;
      assert (aarch64_get_qualifier_nelem (qualifier) == 1);

      /* Index out-of-range.  */
      if (!value_in_range_p (opnd->reglane.index, 0, num))
	{
	  set_elem_idx_out_of_range_error (mismatch_detail, idx, 0, num);
	  return 0;
	}
      /* SMLAL<Q> <Vd>.<Ta>, <Vn>.<Tb>, <Vm>.<Ts>[<index>].
	 <Vm>	Is the vector register (V0-V31) or (V0-V15), whose
	 number is encoded in "size:M:Rm":
	 size	<Vm>
	 00		RESERVED
	 01		0:Rm
	 10		M:Rm
	 11		RESERVED  */
      if (type == AARCH64_OPND_Em16 && qualifier == AARCH64_OPND_QLF_S_H
	  && !value_in_range_p (opnd->reglane.regno, 0, 15))
	{
	  set_regno_out_of_range_error (mismatch_detail, idx, 0, 15);
	  return 0;
	}
      break;

    case AARCH64_OPND_CLASS_MODIFIED_REG:
      assert (idx == 1 || idx == 2);
      switch (type)
	{
	case AARCH64_OPND_Rm_EXT:
	  if (!aarch64_extend_operator_p (opnd->shifter.kind)
	      && opnd->shifter.kind != AARCH64_MOD_LSL)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("extend operator expected"));
	      return 0;
	    }
	  /* It is not optional unless at least one of "Rd" or "Rn" is '11111'
	     (i.e. SP), in which case it defaults to LSL. The LSL alias is
	     only valid when "Rd" or "Rn" is '11111', and is preferred in that
	     case.  */
	  if (!aarch64_stack_pointer_p (opnds + 0)
	      && (idx != 2 || !aarch64_stack_pointer_p (opnds + 1)))
	    {
	      if (!opnd->shifter.operator_present)
		{
		  set_other_error (mismatch_detail, idx,
				   _("missing extend operator"));
		  return 0;
		}
	      else if (opnd->shifter.kind == AARCH64_MOD_LSL)
		{
		  set_other_error (mismatch_detail, idx,
				   _("'LSL' operator not allowed"));
		  return 0;
		}
	    }
	  assert (opnd->shifter.operator_present	/* Default to LSL.  */
		  || opnd->shifter.kind == AARCH64_MOD_LSL);
	  if (!value_in_range_p (opnd->shifter.amount, 0, 4))
	    {
	      set_sft_amount_out_of_range_error (mismatch_detail, idx, 0, 4);
	      return 0;
	    }
	  /* In the 64-bit form, the final register operand is written as Wm
	     for all but the (possibly omitted) UXTX/LSL and SXTX
	     operators.
	     N.B. GAS allows X register to be used with any operator as a
	     programming convenience.  */
	  if (qualifier == AARCH64_OPND_QLF_X
	      && opnd->shifter.kind != AARCH64_MOD_LSL
	      && opnd->shifter.kind != AARCH64_MOD_UXTX
	      && opnd->shifter.kind != AARCH64_MOD_SXTX)
	    {
	      set_other_error (mismatch_detail, idx, _("W register expected"));
	      return 0;
	    }
	  break;

	case AARCH64_OPND_Rm_SFT:
	  /* ROR is not available to the shifted register operand in
	     arithmetic instructions.  */
	  if (!aarch64_shift_operator_p (opnd->shifter.kind))
	    {
	      set_other_error (mismatch_detail, idx,
			       _("shift operator expected"));
	      return 0;
	    }
	  if (opnd->shifter.kind == AARCH64_MOD_ROR
	      && opcode->iclass != log_shift)
	    {
	      set_other_error (mismatch_detail, idx,
			       _("'ROR' operator not allowed"));
	      return 0;
	    }
	  num = qualifier == AARCH64_OPND_QLF_W ? 31 : 63;
	  if (!value_in_range_p (opnd->shifter.amount, 0, num))
	    {
	      set_sft_amount_out_of_range_error (mismatch_detail, idx, 0, num);
	      return 0;
	    }
	  break;

	default:
	  break;
	}
      break;

    default:
      break;
    }

  return 1;
}

/* Main entrypoint for the operand constraint checking.

   Return 1 if operands of *INST meet the constraint applied by the operand
   codes and operand qualifiers; otherwise return 0 and if MISMATCH_DETAIL is
   not NULL, return the detail of the error in *MISMATCH_DETAIL.  N.B. when
   adding more constraint checking, make sure MISMATCH_DETAIL->KIND is set
   with a proper error kind rather than AARCH64_OPDE_NIL (GAS asserts non-NIL
   error kind when it is notified that an instruction does not pass the check).

   Un-determined operand qualifiers may get established during the process.  */

int
aarch64_match_operands_constraint (aarch64_inst *inst,
				   aarch64_operand_error *mismatch_detail)
{
  int i;

  DEBUG_TRACE ("enter");

  i = inst->opcode->tied_operand;

  if (i > 0)
    {
      /* Check for tied_operands with specific opcode iclass.  */
      switch (inst->opcode->iclass)
        {
        /* For SME LDR and STR instructions #imm must have the same numerical
           value for both operands.
        */
        case sme_ldr:
        case sme_str:
          assert (inst->operands[0].type == AARCH64_OPND_SME_ZA_array_off4);
          assert (inst->operands[1].type == AARCH64_OPND_SME_ADDR_RI_U4xVL);
          if (inst->operands[0].indexed_za.index.imm
              != inst->operands[1].addr.offset.imm)
            {
              if (mismatch_detail)
                {
                  mismatch_detail->kind = AARCH64_OPDE_UNTIED_IMMS;
                  mismatch_detail->index = i;
                }
              return 0;
            }
          break;

        default:
	  {
	    /* Check for cases where a source register needs to be the
	       same as the destination register.  Do this before
	       matching qualifiers since if an instruction has both
	       invalid tying and invalid qualifiers, the error about
	       qualifiers would suggest several alternative instructions
	       that also have invalid tying.  */
	    enum aarch64_operand_class op_class
	       = aarch64_get_operand_class (inst->operands[0].type);
	    assert (aarch64_get_operand_class (inst->operands[i].type)
		    == op_class);
	    if (op_class == AARCH64_OPND_CLASS_SVE_REGLIST
		? ((inst->operands[0].reglist.first_regno
		    != inst->operands[i].reglist.first_regno)
		   || (inst->operands[0].reglist.num_regs
		       != inst->operands[i].reglist.num_regs)
		   || (inst->operands[0].reglist.stride
		       != inst->operands[i].reglist.stride))
		: (inst->operands[0].reg.regno
		   != inst->operands[i].reg.regno))
	      {
		if (mismatch_detail)
		  {
		    mismatch_detail->kind = AARCH64_OPDE_UNTIED_OPERAND;
		    mismatch_detail->index = i;
		    mismatch_detail->error = NULL;
		  }
		return 0;
	      }
	    break;
	  }
        }
    }

  /* Match operands' qualifier.
     *INST has already had qualifier establish for some, if not all, of
     its operands; we need to find out whether these established
     qualifiers match one of the qualifier sequence in
     INST->OPCODE->QUALIFIERS_LIST.  If yes, we will assign each operand
     with the corresponding qualifier in such a sequence.
     Only basic operand constraint checking is done here; the more thorough
     constraint checking will carried out by operand_general_constraint_met_p,
     which has be to called after this in order to get all of the operands'
     qualifiers established.  */
  int invalid_count;
  if (match_operands_qualifier (inst, true /* update_p */,
				&invalid_count) == 0)
    {
      DEBUG_TRACE ("FAIL on operand qualifier matching");
      if (mismatch_detail)
	{
	  /* Return an error type to indicate that it is the qualifier
	     matching failure; we don't care about which operand as there
	     are enough information in the opcode table to reproduce it.  */
	  mismatch_detail->kind = AARCH64_OPDE_INVALID_VARIANT;
	  mismatch_detail->index = -1;
	  mismatch_detail->error = NULL;
	  mismatch_detail->data[0].i = invalid_count;
	}
      return 0;
    }

  /* Match operands' constraint.  */
  for (i = 0; i < AARCH64_MAX_OPND_NUM; ++i)
    {
      enum aarch64_opnd type = inst->opcode->operands[i];
      if (type == AARCH64_OPND_NIL)
	break;
      if (inst->operands[i].skip)
	{
	  DEBUG_TRACE ("skip the incomplete operand %d", i);
	  continue;
	}
      if (operand_general_constraint_met_p (inst->operands, i, type,
					    inst->opcode, mismatch_detail) == 0)
	{
	  DEBUG_TRACE ("FAIL on operand %d", i);
	  return 0;
	}
    }

  DEBUG_TRACE ("PASS");

  return 1;
}

/* Replace INST->OPCODE with OPCODE and return the replaced OPCODE.
   Also updates the TYPE of each INST->OPERANDS with the corresponding
   value of OPCODE->OPERANDS.

   Note that some operand qualifiers may need to be manually cleared by
   the caller before it further calls the aarch64_opcode_encode; by
   doing this, it helps the qualifier matching facilities work
   properly.  */

const aarch64_opcode*
aarch64_replace_opcode (aarch64_inst *inst, const aarch64_opcode *opcode)
{
  int i;
  const aarch64_opcode *old = inst->opcode;

  inst->opcode = opcode;

  /* Update the operand types.  */
  for (i = 0; i < AARCH64_MAX_OPND_NUM; ++i)
    {
      inst->operands[i].type = opcode->operands[i];
      if (opcode->operands[i] == AARCH64_OPND_NIL)
	break;
    }

  DEBUG_TRACE ("replace %s with %s", old->name, opcode->name);

  return old;
}

int
aarch64_operand_index (const enum aarch64_opnd *operands, enum aarch64_opnd operand)
{
  int i;
  for (i = 0; i < AARCH64_MAX_OPND_NUM; ++i)
    if (operands[i] == operand)
      return i;
    else if (operands[i] == AARCH64_OPND_NIL)
      break;
  return -1;
}

/* R0...R30, followed by FOR31.  */
#define BANK(R, FOR31) \
  { R  (0), R  (1), R  (2), R  (3), R  (4), R  (5), R  (6), R  (7), \
    R  (8), R  (9), R (10), R (11), R (12), R (13), R (14), R (15), \
    R (16), R (17), R (18), R (19), R (20), R (21), R (22), R (23), \
    R (24), R (25), R (26), R (27), R (28), R (29), R (30),  FOR31 }
/* [0][0]  32-bit integer regs with sp   Wn
   [0][1]  64-bit integer regs with sp   Xn  sf=1
   [1][0]  32-bit integer regs with #0   Wn
   [1][1]  64-bit integer regs with #0   Xn  sf=1 */
static const char *int_reg[2][2][32] = {
#define R32(X) "w" #X
#define R64(X) "x" #X
  { BANK (R32, "wsp"), BANK (R64, "sp") },
  { BANK (R32, "wzr"), BANK (R64, "xzr") }
#undef R64
#undef R32
};

/* Names of the SVE vector registers, first with .S suffixes,
   then with .D suffixes.  */

static const char *sve_reg[2][32] = {
#define ZS(X) "z" #X ".s"
#define ZD(X) "z" #X ".d"
  BANK (ZS, ZS (31)), BANK (ZD, ZD (31))
#undef ZD
#undef ZS
};
#undef BANK

/* Return the integer register name.
   if SP_REG_P is not 0, R31 is an SP reg, other R31 is the zero reg.  */

static inline const char *
get_int_reg_name (int regno, aarch64_opnd_qualifier_t qualifier, int sp_reg_p)
{
  const int has_zr = sp_reg_p ? 0 : 1;
  const int is_64 = aarch64_get_qualifier_esize (qualifier) == 4 ? 0 : 1;
  return int_reg[has_zr][is_64][regno];
}

/* Like get_int_reg_name, but IS_64 is always 1.  */

static inline const char *
get_64bit_int_reg_name (int regno, int sp_reg_p)
{
  const int has_zr = sp_reg_p ? 0 : 1;
  return int_reg[has_zr][1][regno];
}

/* Get the name of the integer offset register in OPND, using the shift type
   to decide whether it's a word or doubleword.  */

static inline const char *
get_offset_int_reg_name (const aarch64_opnd_info *opnd)
{
  switch (opnd->shifter.kind)
    {
    case AARCH64_MOD_UXTW:
    case AARCH64_MOD_SXTW:
      return get_int_reg_name (opnd->addr.offset.regno, AARCH64_OPND_QLF_W, 0);

    case AARCH64_MOD_LSL:
    case AARCH64_MOD_SXTX:
      return get_int_reg_name (opnd->addr.offset.regno, AARCH64_OPND_QLF_X, 0);

    default:
      abort ();
    }
}

/* Get the name of the SVE vector offset register in OPND, using the operand
   qualifier to decide whether the suffix should be .S or .D.  */

static inline const char *
get_addr_sve_reg_name (int regno, aarch64_opnd_qualifier_t qualifier)
{
  assert (qualifier == AARCH64_OPND_QLF_S_S
	  || qualifier == AARCH64_OPND_QLF_S_D);
  return sve_reg[qualifier == AARCH64_OPND_QLF_S_D][regno];
}

/* Types for expanding an encoded 8-bit value to a floating-point value.  */

typedef union
{
  uint64_t i;
  double   d;
} double_conv_t;

typedef union
{
  uint32_t i;
  float    f;
} single_conv_t;

typedef union
{
  uint32_t i;
  float    f;
} half_conv_t;

/* IMM8 is an 8-bit floating-point constant with sign, 3-bit exponent and
   normalized 4 bits of precision, encoded in "a:b:c:d:e:f:g:h" or FLD_imm8
   (depending on the type of the instruction).  IMM8 will be expanded to a
   single-precision floating-point value (SIZE == 4) or a double-precision
   floating-point value (SIZE == 8).  A half-precision floating-point value
   (SIZE == 2) is expanded to a single-precision floating-point value.  The
   expanded value is returned.  */

static uint64_t
expand_fp_imm (int size, uint32_t imm8)
{
  uint64_t imm = 0;
  uint32_t imm8_7, imm8_6_0, imm8_6, imm8_6_repl4;

  imm8_7 = (imm8 >> 7) & 0x01;	/* imm8<7>   */
  imm8_6_0 = imm8 & 0x7f;	/* imm8<6:0> */
  imm8_6 = imm8_6_0 >> 6;	/* imm8<6>   */
  imm8_6_repl4 = (imm8_6 << 3) | (imm8_6 << 2)
    | (imm8_6 << 1) | imm8_6;	/* Replicate(imm8<6>,4) */
  if (size == 8)
    {
      imm = (imm8_7 << (63-32))		/* imm8<7>  */
	| ((imm8_6 ^ 1) << (62-32))	/* NOT(imm8<6)	*/
	| (imm8_6_repl4 << (58-32)) | (imm8_6 << (57-32))
	| (imm8_6 << (56-32)) | (imm8_6 << (55-32)) /* Replicate(imm8<6>,7) */
	| (imm8_6_0 << (48-32));	/* imm8<6>:imm8<5:0>    */
      imm <<= 32;
    }
  else if (size == 4 || size == 2)
    {
      imm = (imm8_7 << 31)	/* imm8<7>              */
	| ((imm8_6 ^ 1) << 30)	/* NOT(imm8<6>)         */
	| (imm8_6_repl4 << 26)	/* Replicate(imm8<6>,4) */
	| (imm8_6_0 << 19);	/* imm8<6>:imm8<5:0>    */
    }
  else
    {
      /* An unsupported size.  */
      assert (0);
    }

  return imm;
}

/* Return a string based on FMT with the register style applied.  */

static const char *
style_reg (struct aarch64_styler *styler, const char *fmt, ...)
{
  const char *txt;
  va_list ap;

  va_start (ap, fmt);
  txt = styler->apply_style (styler, dis_style_register, fmt, ap);
  va_end (ap);

  return txt;
}

/* Return a string based on FMT with the immediate style applied.  */

static const char *
style_imm (struct aarch64_styler *styler, const char *fmt, ...)
{
  const char *txt;
  va_list ap;

  va_start (ap, fmt);
  txt = styler->apply_style (styler, dis_style_immediate, fmt, ap);
  va_end (ap);

  return txt;
}

/* Return a string based on FMT with the sub-mnemonic style applied.  */

static const char *
style_sub_mnem (struct aarch64_styler *styler, const char *fmt, ...)
{
  const char *txt;
  va_list ap;

  va_start (ap, fmt);
  txt = styler->apply_style (styler, dis_style_sub_mnemonic, fmt, ap);
  va_end (ap);

  return txt;
}

/* Return a string based on FMT with the address style applied.  */

static const char *
style_addr (struct aarch64_styler *styler, const char *fmt, ...)
{
  const char *txt;
  va_list ap;

  va_start (ap, fmt);
  txt = styler->apply_style (styler, dis_style_address, fmt, ap);
  va_end (ap);

  return txt;
}

/* Produce the string representation of the register list operand *OPND
   in the buffer pointed by BUF of size SIZE.  PREFIX is the part of
   the register name that comes before the register number, such as "v".  */
static void
print_register_list (char *buf, size_t size, const aarch64_opnd_info *opnd,
		     const char *prefix, struct aarch64_styler *styler)
{
  const int mask = (prefix[0] == 'p' ? 15 : 31);
  const int num_regs = opnd->reglist.num_regs;
  const int stride = opnd->reglist.stride;
  const int first_reg = opnd->reglist.first_regno;
  const int last_reg = (first_reg + (num_regs - 1) * stride) & mask;
  const char *qlf_name = aarch64_get_qualifier_name (opnd->qualifier);
  char tb[16];	/* Temporary buffer.  */

  assert (opnd->type != AARCH64_OPND_LEt || opnd->reglist.has_index);
  assert (num_regs >= 1 && num_regs <= 4);

  /* Prepare the index if any.  */
  if (opnd->reglist.has_index)
    /* PR 21096: The %100 is to silence a warning about possible truncation.  */
    snprintf (tb, sizeof (tb), "[%s]",
	      style_imm (styler, "%" PRIi64, (opnd->reglist.index % 100)));
  else
    tb[0] = '\0';

  /* The hyphenated form is preferred for disassembly if there are
     more than two registers in the list, and the register numbers
     are monotonically increasing in increments of one.  */
  if (stride == 1 && num_regs > 1
      && ((opnd->type != AARCH64_OPND_SME_Zt2)
	  && (opnd->type != AARCH64_OPND_SME_Zt3)
	  && (opnd->type != AARCH64_OPND_SME_Zt4)))
    snprintf (buf, size, "{%s-%s}%s",
	      style_reg (styler, "%s%d.%s", prefix, first_reg, qlf_name),
	      style_reg (styler, "%s%d.%s", prefix, last_reg, qlf_name), tb);
  else
    {
      const int reg0 = first_reg;
      const int reg1 = (first_reg + stride) & mask;
      const int reg2 = (first_reg + stride * 2) & mask;
      const int reg3 = (first_reg + stride * 3) & mask;

      switch (num_regs)
	{
	case 1:
	  snprintf (buf, size, "{%s}%s",
		    style_reg (styler, "%s%d.%s", prefix, reg0, qlf_name),
		    tb);
	  break;
	case 2:
	  snprintf (buf, size, "{%s, %s}%s",
		    style_reg (styler, "%s%d.%s", prefix, reg0, qlf_name),
		    style_reg (styler, "%s%d.%s", prefix, reg1, qlf_name),
		    tb);
	  break;
	case 3:
	  snprintf (buf, size, "{%s, %s, %s}%s",
		    style_reg (styler, "%s%d.%s", prefix, reg0, qlf_name),
		    style_reg (styler, "%s%d.%s", prefix, reg1, qlf_name),
		    style_reg (styler, "%s%d.%s", prefix, reg2, qlf_name),
		    tb);
	  break;
	case 4:
	  snprintf (buf, size, "{%s, %s, %s, %s}%s",
		    style_reg (styler, "%s%d.%s", prefix, reg0, qlf_name),
		    style_reg (styler, "%s%d.%s", prefix, reg1, qlf_name),
		    style_reg (styler, "%s%d.%s", prefix, reg2, qlf_name),
		    style_reg (styler, "%s%d.%s", prefix, reg3, qlf_name),
		    tb);
	  break;
	}
    }
}

/* Print the register+immediate address in OPND to BUF, which has SIZE
   characters.  BASE is the name of the base register.  */

static void
print_immediate_offset_address (char *buf, size_t size,
				const aarch64_opnd_info *opnd,
				const char *base,
				struct aarch64_styler *styler)
{
  if (opnd->addr.writeback)
    {
      if (opnd->addr.preind)
        {
	  if (opnd->type == AARCH64_OPND_ADDR_SIMM10 && !opnd->addr.offset.imm)
	    snprintf (buf, size, "[%s]!", style_reg (styler, base));
          else
	    snprintf (buf, size, "[%s, %s]!",
		      style_reg (styler, base),
		      style_imm (styler, "#%d", opnd->addr.offset.imm));
        }
      else
	snprintf (buf, size, "[%s], %s",
		  style_reg (styler, base),
		  style_imm (styler, "#%d", opnd->addr.offset.imm));
    }
  else
    {
      if (opnd->shifter.operator_present)
	{
	  assert (opnd->shifter.kind == AARCH64_MOD_MUL_VL);
	  snprintf (buf, size, "[%s, %s, %s]",
		    style_reg (styler, base),
		    style_imm (styler, "#%d", opnd->addr.offset.imm),
		    style_sub_mnem (styler, "mul vl"));
	}
      else if (opnd->addr.offset.imm)
	snprintf (buf, size, "[%s, %s]",
		  style_reg (styler, base),
		  style_imm (styler, "#%d", opnd->addr.offset.imm));
      else
	snprintf (buf, size, "[%s]", style_reg (styler, base));
    }
}

/* Produce the string representation of the register offset address operand
   *OPND in the buffer pointed by BUF of size SIZE.  BASE and OFFSET are
   the names of the base and offset registers.  */
static void
print_register_offset_address (char *buf, size_t size,
			       const aarch64_opnd_info *opnd,
			       const char *base, const char *offset,
			       struct aarch64_styler *styler)
{
  char tb[32];			/* Temporary buffer.  */
  bool print_extend_p = true;
  bool print_amount_p = true;
  const char *shift_name = aarch64_operand_modifiers[opnd->shifter.kind].name;

  if (!opnd->shifter.amount && (opnd->qualifier != AARCH64_OPND_QLF_S_B
				|| !opnd->shifter.amount_present))
    {
      /* Not print the shift/extend amount when the amount is zero and
         when it is not the special case of 8-bit load/store instruction.  */
      print_amount_p = false;
      /* Likewise, no need to print the shift operator LSL in such a
	 situation.  */
      if (opnd->shifter.kind == AARCH64_MOD_LSL)
	print_extend_p = false;
    }

  /* Prepare for the extend/shift.  */
  if (print_extend_p)
    {
      if (print_amount_p)
	snprintf (tb, sizeof (tb), ", %s %s",
		  style_sub_mnem (styler, shift_name),
		  style_imm (styler, "#%" PRIi64,
  /* PR 21096: The %100 is to silence a warning about possible truncation.  */
			     (opnd->shifter.amount % 100)));
      else
	snprintf (tb, sizeof (tb), ", %s",
		  style_sub_mnem (styler, shift_name));
    }
  else
    tb[0] = '\0';

  snprintf (buf, size, "[%s, %s%s]", style_reg (styler, base),
	    style_reg (styler, offset), tb);
}

/* Print ZA tiles from imm8 in ZERO instruction.

   The preferred disassembly of this instruction uses the shortest list of tile
   names that represent the encoded immediate mask.

   For example:
    * An all-ones immediate is disassembled as {ZA}.
    * An all-zeros immediate is disassembled as an empty list { }.
*/
static void
print_sme_za_list (char *buf, size_t size, int mask,
		   struct aarch64_styler *styler)
{
  const char* zan[] = { "za",    "za0.h", "za1.h", "za0.s",
                        "za1.s", "za2.s", "za3.s", "za0.d",
                        "za1.d", "za2.d", "za3.d", "za4.d",
                        "za5.d", "za6.d", "za7.d", " " };
  const int zan_v[] = { 0xff, 0x55, 0xaa, 0x11,
                        0x22, 0x44, 0x88, 0x01,
                        0x02, 0x04, 0x08, 0x10,
                        0x20, 0x40, 0x80, 0x00 };
  int i, k;
  const int ZAN_SIZE = sizeof(zan) / sizeof(zan[0]);

  k = snprintf (buf, size, "{");
  for (i = 0; i < ZAN_SIZE; i++)
    {
      if ((mask & zan_v[i]) == zan_v[i])
        {
          mask &= ~zan_v[i];
          if (k > 1)
	    k += snprintf (buf + k, size - k, ", ");

	  k += snprintf (buf + k, size - k, "%s", style_reg (styler, zan[i]));
        }
      if (mask == 0)
        break;
    }
  snprintf (buf + k, size - k, "}");
}

/* Generate the string representation of the operand OPNDS[IDX] for OPCODE
   in *BUF.  The caller should pass in the maximum size of *BUF in SIZE.
   PC, PCREL_P and ADDRESS are used to pass in and return information about
   the PC-relative address calculation, where the PC value is passed in
   PC.  If the operand is pc-relative related, *PCREL_P (if PCREL_P non-NULL)
   will return 1 and *ADDRESS (if ADDRESS non-NULL) will return the
   calculated address; otherwise, *PCREL_P (if PCREL_P non-NULL) returns 0.

   The function serves both the disassembler and the assembler diagnostics
   issuer, which is the reason why it lives in this file.  */

void
aarch64_print_operand (char *buf, size_t size, bfd_vma pc,
		       const aarch64_opcode *opcode,
		       const aarch64_opnd_info *opnds, int idx, int *pcrel_p,
		       bfd_vma *address, char** notes,
		       char *comment, size_t comment_size,
		       aarch64_feature_set features,
		       struct aarch64_styler *styler)
{
  unsigned int i, num_conds;
  const char *name = NULL;
  const aarch64_opnd_info *opnd = opnds + idx;
  enum aarch64_modifier_kind kind;
  uint64_t addr, enum_value;

  if (comment != NULL)
    {
      assert (comment_size > 0);
      comment[0] = '\0';
    }
  else
    assert (comment_size == 0);

  buf[0] = '\0';
  if (pcrel_p)
    *pcrel_p = 0;

  switch (opnd->type)
    {
    case AARCH64_OPND_Rd:
    case AARCH64_OPND_Rn:
    case AARCH64_OPND_Rm:
    case AARCH64_OPND_Rt:
    case AARCH64_OPND_Rt2:
    case AARCH64_OPND_Rs:
    case AARCH64_OPND_Ra:
    case AARCH64_OPND_Rt_LS64:
    case AARCH64_OPND_Rt_SYS:
    case AARCH64_OPND_PAIRREG:
    case AARCH64_OPND_PAIRREG_OR_XZR:
    case AARCH64_OPND_SVE_Rm:
    case AARCH64_OPND_LSE128_Rt:
    case AARCH64_OPND_LSE128_Rt2:
      /* The optional-ness of <Xt> in e.g. IC <ic_op>{, <Xt>} is determined by
	 the <ic_op>, therefore we use opnd->present to override the
	 generic optional-ness information.  */
      if (opnd->type == AARCH64_OPND_Rt_SYS)
	{
	  if (!opnd->present)
	    break;
	}
      /* Omit the operand, e.g. RET.  */
      else if (optional_operand_p (opcode, idx)
	       && (opnd->reg.regno
		   == get_optional_operand_default_value (opcode)))
	break;
      assert (opnd->qualifier == AARCH64_OPND_QLF_W
	      || opnd->qualifier == AARCH64_OPND_QLF_X);
      snprintf (buf, size, "%s",
		style_reg (styler, get_int_reg_name (opnd->reg.regno,
						     opnd->qualifier, 0)));
      break;

    case AARCH64_OPND_Rd_SP:
    case AARCH64_OPND_Rn_SP:
    case AARCH64_OPND_Rt_SP:
    case AARCH64_OPND_SVE_Rn_SP:
    case AARCH64_OPND_Rm_SP:
      assert (opnd->qualifier == AARCH64_OPND_QLF_W
	      || opnd->qualifier == AARCH64_OPND_QLF_WSP
	      || opnd->qualifier == AARCH64_OPND_QLF_X
	      || opnd->qualifier == AARCH64_OPND_QLF_SP);
      snprintf (buf, size, "%s",
		style_reg (styler, get_int_reg_name (opnd->reg.regno,
						     opnd->qualifier, 1)));
      break;

    case AARCH64_OPND_Rm_EXT:
      kind = opnd->shifter.kind;
      assert (idx == 1 || idx == 2);
      if ((aarch64_stack_pointer_p (opnds)
	   || (idx == 2 && aarch64_stack_pointer_p (opnds + 1)))
	  && ((opnd->qualifier == AARCH64_OPND_QLF_W
	       && opnds[0].qualifier == AARCH64_OPND_QLF_W
	       && kind == AARCH64_MOD_UXTW)
	      || (opnd->qualifier == AARCH64_OPND_QLF_X
		  && kind == AARCH64_MOD_UXTX)))
	{
	  /* 'LSL' is the preferred form in this case.  */
	  kind = AARCH64_MOD_LSL;
	  if (opnd->shifter.amount == 0)
	    {
	      /* Shifter omitted.  */
	      snprintf (buf, size, "%s",
			style_reg (styler,
				   get_int_reg_name (opnd->reg.regno,
						     opnd->qualifier, 0)));
	      break;
	    }
	}
      if (opnd->shifter.amount)
	snprintf (buf, size, "%s, %s %s",
		  style_reg (styler, get_int_reg_name (opnd->reg.regno, opnd->qualifier, 0)),
		  style_sub_mnem (styler, aarch64_operand_modifiers[kind].name),
		  style_imm (styler, "#%" PRIi64, opnd->shifter.amount));
      else
	snprintf (buf, size, "%s, %s",
		  style_reg (styler, get_int_reg_name (opnd->reg.regno, opnd->qualifier, 0)),
		  style_sub_mnem (styler, aarch64_operand_modifiers[kind].name));
      break;

    case AARCH64_OPND_Rm_SFT:
      assert (opnd->qualifier == AARCH64_OPND_QLF_W
	      || opnd->qualifier == AARCH64_OPND_QLF_X);
      if (opnd->shifter.amount == 0 && opnd->shifter.kind == AARCH64_MOD_LSL)
	snprintf (buf, size, "%s",
		  style_reg (styler, get_int_reg_name (opnd->reg.regno,
						       opnd->qualifier, 0)));
      else
	snprintf (buf, size, "%s, %s %s",
		  style_reg (styler, get_int_reg_name (opnd->reg.regno, opnd->qualifier, 0)),
		  style_sub_mnem (styler, aarch64_operand_modifiers[opnd->shifter.kind].name),
		  style_imm (styler, "#%" PRIi64, opnd->shifter.amount));
      break;

    case AARCH64_OPND_Fd:
    case AARCH64_OPND_Fn:
    case AARCH64_OPND_Fm:
    case AARCH64_OPND_Fa:
    case AARCH64_OPND_Ft:
    case AARCH64_OPND_Ft2:
    case AARCH64_OPND_Sd:
    case AARCH64_OPND_Sn:
    case AARCH64_OPND_Sm:
    case AARCH64_OPND_SVE_VZn:
    case AARCH64_OPND_SVE_Vd:
    case AARCH64_OPND_SVE_Vm:
    case AARCH64_OPND_SVE_Vn:
      snprintf (buf, size, "%s",
		style_reg (styler, "%s%d",
			   aarch64_get_qualifier_name (opnd->qualifier),
			   opnd->reg.regno));
      break;

    case AARCH64_OPND_Va:
    case AARCH64_OPND_Vd:
    case AARCH64_OPND_Vn:
    case AARCH64_OPND_Vm:
      snprintf (buf, size, "%s",
		style_reg (styler, "v%d.%s", opnd->reg.regno,
			   aarch64_get_qualifier_name (opnd->qualifier)));
      break;

    case AARCH64_OPND_Ed:
    case AARCH64_OPND_En:
    case AARCH64_OPND_Em:
    case AARCH64_OPND_Em16:
    case AARCH64_OPND_SM3_IMM2:
      snprintf (buf, size, "%s[%s]",
		style_reg (styler, "v%d.%s", opnd->reglane.regno,
			   aarch64_get_qualifier_name (opnd->qualifier)),
		style_imm (styler, "%" PRIi64, opnd->reglane.index));
      break;

    case AARCH64_OPND_VdD1:
    case AARCH64_OPND_VnD1:
      snprintf (buf, size, "%s[%s]",
		style_reg (styler, "v%d.d", opnd->reg.regno),
		style_imm (styler, "1"));
      break;

    case AARCH64_OPND_LVn:
    case AARCH64_OPND_LVt:
    case AARCH64_OPND_LVt_AL:
    case AARCH64_OPND_LEt:
      print_register_list (buf, size, opnd, "v", styler);
      break;

    case AARCH64_OPND_SVE_Pd:
    case AARCH64_OPND_SVE_Pg3:
    case AARCH64_OPND_SVE_Pg4_5:
    case AARCH64_OPND_SVE_Pg4_10:
    case AARCH64_OPND_SVE_Pg4_16:
    case AARCH64_OPND_SVE_Pm:
    case AARCH64_OPND_SVE_Pn:
    case AARCH64_OPND_SVE_Pt:
    case AARCH64_OPND_SME_Pm:
      if (opnd->qualifier == AARCH64_OPND_QLF_NIL)
	snprintf (buf, size, "%s",
		  style_reg (styler, "p%d", opnd->reg.regno));
      else if (opnd->qualifier == AARCH64_OPND_QLF_P_Z
	       || opnd->qualifier == AARCH64_OPND_QLF_P_M)
	snprintf (buf, size, "%s",
		  style_reg (styler, "p%d/%s", opnd->reg.regno,
			     aarch64_get_qualifier_name (opnd->qualifier)));
      else
	snprintf (buf, size, "%s",
		  style_reg (styler, "p%d.%s", opnd->reg.regno,
			     aarch64_get_qualifier_name (opnd->qualifier)));
      break;

    case AARCH64_OPND_SVE_PNd:
    case AARCH64_OPND_SVE_PNg4_10:
    case AARCH64_OPND_SVE_PNn:
    case AARCH64_OPND_SVE_PNt:
    case AARCH64_OPND_SME_PNd3:
    case AARCH64_OPND_SME_PNg3:
    case AARCH64_OPND_SME_PNn:
      if (opnd->qualifier == AARCH64_OPND_QLF_NIL)
	snprintf (buf, size, "%s",
		  style_reg (styler, "pn%d", opnd->reg.regno));
      else if (opnd->qualifier == AARCH64_OPND_QLF_P_Z
	       || opnd->qualifier == AARCH64_OPND_QLF_P_M)
	snprintf (buf, size, "%s",
		  style_reg (styler, "pn%d/%s", opnd->reg.regno,
			     aarch64_get_qualifier_name (opnd->qualifier)));
      else
	snprintf (buf, size, "%s",
		  style_reg (styler, "pn%d.%s", opnd->reg.regno,
			     aarch64_get_qualifier_name (opnd->qualifier)));
      break;

    case AARCH64_OPND_SME_Pdx2:
    case AARCH64_OPND_SME_PdxN:
      print_register_list (buf, size, opnd, "p", styler);
      break;

    case AARCH64_OPND_SME_PNn3_INDEX1:
    case AARCH64_OPND_SME_PNn3_INDEX2:
      snprintf (buf, size, "%s[%s]",
		style_reg (styler, "pn%d", opnd->reglane.regno),
		style_imm (styler, "%" PRIi64, opnd->reglane.index));
      break;

    case AARCH64_OPND_SVE_Za_5:
    case AARCH64_OPND_SVE_Za_16:
    case AARCH64_OPND_SVE_Zd:
    case AARCH64_OPND_SVE_Zm_5:
    case AARCH64_OPND_SVE_Zm_16:
    case AARCH64_OPND_SVE_Zn:
    case AARCH64_OPND_SVE_Zt:
    case AARCH64_OPND_SME_Zm:
      if (opnd->qualifier == AARCH64_OPND_QLF_NIL)
	snprintf (buf, size, "%s", style_reg (styler, "z%d", opnd->reg.regno));
      else
	snprintf (buf, size, "%s",
		  style_reg (styler, "z%d.%s", opnd->reg.regno,
			     aarch64_get_qualifier_name (opnd->qualifier)));
      break;

    case AARCH64_OPND_SVE_ZnxN:
    case AARCH64_OPND_SVE_ZtxN:
    case AARCH64_OPND_SME_Zdnx2:
    case AARCH64_OPND_SME_Zdnx4:
    case AARCH64_OPND_SME_Zmx2:
    case AARCH64_OPND_SME_Zmx4:
    case AARCH64_OPND_SME_Znx2:
    case AARCH64_OPND_SME_Znx4:
    case AARCH64_OPND_SME_Ztx2_STRIDED:
    case AARCH64_OPND_SME_Ztx4_STRIDED:
    case AARCH64_OPND_SME_Zt2:
    case AARCH64_OPND_SME_Zt3:
    case AARCH64_OPND_SME_Zt4:
      print_register_list (buf, size, opnd, "z", styler);
      break;

    case AARCH64_OPND_SVE_Zm3_INDEX:
    case AARCH64_OPND_SVE_Zm3_22_INDEX:
    case AARCH64_OPND_SVE_Zm3_19_INDEX:
    case AARCH64_OPND_SVE_Zm3_11_INDEX:
    case AARCH64_OPND_SVE_Zm4_11_INDEX:
    case AARCH64_OPND_SVE_Zm4_INDEX:
    case AARCH64_OPND_SVE_Zn_INDEX:
    case AARCH64_OPND_SME_Zm_INDEX1:
    case AARCH64_OPND_SME_Zm_INDEX2:
    case AARCH64_OPND_SME_Zm_INDEX3_1:
    case AARCH64_OPND_SME_Zm_INDEX3_2:
    case AARCH64_OPND_SME_Zm_INDEX3_10:
    case AARCH64_OPND_SVE_Zn_5_INDEX:
    case AARCH64_OPND_SME_Zm_INDEX4_1:
    case AARCH64_OPND_SME_Zm_INDEX4_10:
    case AARCH64_OPND_SME_Zn_INDEX1_16:
    case AARCH64_OPND_SME_Zn_INDEX2_15:
    case AARCH64_OPND_SME_Zn_INDEX2_16:
    case AARCH64_OPND_SME_Zn_INDEX3_14:
    case AARCH64_OPND_SME_Zn_INDEX3_15:
    case AARCH64_OPND_SME_Zn_INDEX4_14:
    case AARCH64_OPND_SVE_Zm_imm4:
      snprintf (buf, size, "%s[%s]",
		(opnd->qualifier == AARCH64_OPND_QLF_NIL
		 ? style_reg (styler, "z%d", opnd->reglane.regno)
		 : style_reg (styler, "z%d.%s", opnd->reglane.regno,
			      aarch64_get_qualifier_name (opnd->qualifier))),
		style_imm (styler, "%" PRIi64, opnd->reglane.index));
      break;

    case AARCH64_OPND_SME_ZAda_2b:
    case AARCH64_OPND_SME_ZAda_3b:
      snprintf (buf, size, "%s",
		style_reg (styler, "za%d.%s", opnd->reg.regno,
			   aarch64_get_qualifier_name (opnd->qualifier)));
      break;

    case AARCH64_OPND_SME_ZA_HV_idx_src:
    case AARCH64_OPND_SME_ZA_HV_idx_srcxN:
    case AARCH64_OPND_SME_ZA_HV_idx_dest:
    case AARCH64_OPND_SME_ZA_HV_idx_destxN:
    case AARCH64_OPND_SME_ZA_HV_idx_ldstr:
      snprintf (buf, size, "%s%s[%s, %s%s%s%s%s]%s",
		opnd->type == AARCH64_OPND_SME_ZA_HV_idx_ldstr ? "{" : "",
		style_reg (styler, "za%d%c.%s",
			   opnd->indexed_za.regno,
			   opnd->indexed_za.v == 1 ? 'v' : 'h',
			   aarch64_get_qualifier_name (opnd->qualifier)),
		style_reg (styler, "w%d", opnd->indexed_za.index.regno),
		style_imm (styler, "%" PRIi64, opnd->indexed_za.index.imm),
		opnd->indexed_za.index.countm1 ? ":" : "",
		(opnd->indexed_za.index.countm1
		 ? style_imm (styler, "%d",
			      opnd->indexed_za.index.imm
			      + opnd->indexed_za.index.countm1)
		 : ""),
		opnd->indexed_za.group_size ? ", " : "",
		opnd->indexed_za.group_size == 2
		? style_sub_mnem (styler, "vgx2")
		: opnd->indexed_za.group_size == 4
		? style_sub_mnem (styler, "vgx4") : "",
		opnd->type == AARCH64_OPND_SME_ZA_HV_idx_ldstr ? "}" : "");
      break;

    case AARCH64_OPND_SME_list_of_64bit_tiles:
      print_sme_za_list (buf, size, opnd->reg.regno, styler);
      break;

    case AARCH64_OPND_SME_ZA_array_off1x4:
    case AARCH64_OPND_SME_ZA_array_off2x2:
    case AARCH64_OPND_SME_ZA_array_off2x4:
    case AARCH64_OPND_SME_ZA_array_off3_0:
    case AARCH64_OPND_SME_ZA_array_off3_5:
    case AARCH64_OPND_SME_ZA_array_off3x2:
    case AARCH64_OPND_SME_ZA_array_off4:
      snprintf (buf, size, "%s[%s, %s%s%s%s%s]",
		style_reg (styler, "za%s%s",
			   opnd->qualifier == AARCH64_OPND_QLF_NIL ? "" : ".",
			   (opnd->qualifier == AARCH64_OPND_QLF_NIL
			    ? ""
			    : aarch64_get_qualifier_name (opnd->qualifier))),
		style_reg (styler, "w%d", opnd->indexed_za.index.regno),
		style_imm (styler, "%" PRIi64, opnd->indexed_za.index.imm),
		opnd->indexed_za.index.countm1 ? ":" : "",
		(opnd->indexed_za.index.countm1
		 ? style_imm (styler, "%d",
			      opnd->indexed_za.index.imm
			      + opnd->indexed_za.index.countm1)
		 : ""),
		opnd->indexed_za.group_size ? ", " : "",
		opnd->indexed_za.group_size == 2
		? style_sub_mnem (styler, "vgx2")
		: opnd->indexed_za.group_size == 4
		? style_sub_mnem (styler, "vgx4") : "");
      break;

    case AARCH64_OPND_SME_ZA_array_vrsb_1:
    case AARCH64_OPND_SME_ZA_array_vrsh_1:
    case AARCH64_OPND_SME_ZA_array_vrss_1:
    case AARCH64_OPND_SME_ZA_array_vrsd_1:
    case AARCH64_OPND_SME_ZA_array_vrsb_2:
    case AARCH64_OPND_SME_ZA_array_vrsh_2:
    case AARCH64_OPND_SME_ZA_array_vrss_2:
    case AARCH64_OPND_SME_ZA_array_vrsd_2:
      snprintf (buf, size, "%s [%s, %s%s%s]",
		style_reg (styler, "za%d%c%s%s",
			   opnd->indexed_za.regno,
			   opnd->indexed_za.v ? 'v': 'h',
			   opnd->qualifier == AARCH64_OPND_QLF_NIL ? "" : ".",
			   (opnd->qualifier == AARCH64_OPND_QLF_NIL
			    ? ""
			    : aarch64_get_qualifier_name (opnd->qualifier))),
		style_reg (styler, "w%d", opnd->indexed_za.index.regno),
		style_imm (styler, "%" PRIi64, opnd->indexed_za.index.imm),
		opnd->indexed_za.index.countm1 ? ":" : "",
		opnd->indexed_za.index.countm1  ? style_imm (styler, "%d",
		opnd->indexed_za.index.imm
		+ opnd->indexed_za.index.countm1):"");
      break;

    case AARCH64_OPND_SME_SM_ZA:
      snprintf (buf, size, "%s",
		style_reg (styler, opnd->reg.regno == 's' ? "sm" : "za"));
      break;

    case AARCH64_OPND_SME_PnT_Wm_imm:
      snprintf (buf, size, "%s[%s, %s]",
		style_reg (styler, "p%d.%s", opnd->indexed_za.regno,
			   aarch64_get_qualifier_name (opnd->qualifier)),
		style_reg (styler, "w%d", opnd->indexed_za.index.regno),
		style_imm (styler, "%" PRIi64, opnd->indexed_za.index.imm));
      break;

    case AARCH64_OPND_SME_VLxN_10:
    case AARCH64_OPND_SME_VLxN_13:
      enum_value = opnd->imm.value;
      assert (enum_value < ARRAY_SIZE (aarch64_sme_vlxn_array));
      snprintf (buf, size, "%s",
		style_sub_mnem (styler, aarch64_sme_vlxn_array[enum_value]));
      break;

    case AARCH64_OPND_CRn:
    case AARCH64_OPND_CRm:
      snprintf (buf, size, "%s",
		style_reg (styler, "C%" PRIi64, opnd->imm.value));
      break;

    case AARCH64_OPND_IDX:
    case AARCH64_OPND_MASK:
    case AARCH64_OPND_IMM:
    case AARCH64_OPND_IMM_2:
    case AARCH64_OPND_WIDTH:
    case AARCH64_OPND_UIMM3_OP1:
    case AARCH64_OPND_UIMM3_OP2:
    case AARCH64_OPND_BIT_NUM:
    case AARCH64_OPND_IMM_VLSL:
    case AARCH64_OPND_IMM_VLSR:
    case AARCH64_OPND_SHLL_IMM:
    case AARCH64_OPND_IMM0:
    case AARCH64_OPND_IMMR:
    case AARCH64_OPND_IMMS:
    case AARCH64_OPND_UNDEFINED:
    case AARCH64_OPND_FBITS:
    case AARCH64_OPND_TME_UIMM16:
    case AARCH64_OPND_SIMM5:
    case AARCH64_OPND_SME_SHRIMM4:
    case AARCH64_OPND_SME_SHRIMM5:
    case AARCH64_OPND_SVE_SHLIMM_PRED:
    case AARCH64_OPND_SVE_SHLIMM_UNPRED:
    case AARCH64_OPND_SVE_SHLIMM_UNPRED_22:
    case AARCH64_OPND_SVE_SHRIMM_PRED:
    case AARCH64_OPND_SVE_SHRIMM_UNPRED:
    case AARCH64_OPND_SVE_SHRIMM_UNPRED_22:
    case AARCH64_OPND_SVE_SIMM5:
    case AARCH64_OPND_SVE_SIMM5B:
    case AARCH64_OPND_SVE_SIMM6:
    case AARCH64_OPND_SVE_SIMM8:
    case AARCH64_OPND_SVE_UIMM3:
    case AARCH64_OPND_SVE_UIMM7:
    case AARCH64_OPND_SVE_UIMM8:
    case AARCH64_OPND_SVE_UIMM8_53:
    case AARCH64_OPND_IMM_ROT1:
    case AARCH64_OPND_IMM_ROT2:
    case AARCH64_OPND_IMM_ROT3:
    case AARCH64_OPND_SVE_IMM_ROT1:
    case AARCH64_OPND_SVE_IMM_ROT2:
    case AARCH64_OPND_SVE_IMM_ROT3:
    case AARCH64_OPND_CSSC_SIMM8:
    case AARCH64_OPND_CSSC_UIMM8:
      snprintf (buf, size, "%s",
		style_imm (styler, "#%" PRIi64, opnd->imm.value));
      break;

    case AARCH64_OPND_SVE_I1_HALF_ONE:
    case AARCH64_OPND_SVE_I1_HALF_TWO:
    case AARCH64_OPND_SVE_I1_ZERO_ONE:
      {
	single_conv_t c;
	c.i = opnd->imm.value;
	snprintf (buf, size, "%s", style_imm (styler, "#%.1f", c.f));
	break;
      }

    case AARCH64_OPND_SVE_PATTERN:
      if (optional_operand_p (opcode, idx)
	  && opnd->imm.value == get_optional_operand_default_value (opcode))
	break;
      enum_value = opnd->imm.value;
      assert (enum_value < ARRAY_SIZE (aarch64_sve_pattern_array));
      if (aarch64_sve_pattern_array[enum_value])
	snprintf (buf, size, "%s",
		  style_reg (styler, aarch64_sve_pattern_array[enum_value]));
      else
	snprintf (buf, size, "%s",
		  style_imm (styler, "#%" PRIi64, opnd->imm.value));
      break;

    case AARCH64_OPND_SVE_PATTERN_SCALED:
      if (optional_operand_p (opcode, idx)
	  && !opnd->shifter.operator_present
	  && opnd->imm.value == get_optional_operand_default_value (opcode))
	break;
      enum_value = opnd->imm.value;
      assert (enum_value < ARRAY_SIZE (aarch64_sve_pattern_array));
      if (aarch64_sve_pattern_array[opnd->imm.value])
	snprintf (buf, size, "%s",
		  style_reg (styler,
			     aarch64_sve_pattern_array[opnd->imm.value]));
      else
	snprintf (buf, size, "%s",
		  style_imm (styler, "#%" PRIi64, opnd->imm.value));
      if (opnd->shifter.operator_present)
	{
	  size_t len = strlen (buf);
	  const char *shift_name
	    = aarch64_operand_modifiers[opnd->shifter.kind].name;
	  snprintf (buf + len, size - len, ", %s %s",
		    style_sub_mnem (styler, shift_name),
		    style_imm (styler, "#%" PRIi64, opnd->shifter.amount));
	}
      break;

    case AARCH64_OPND_SVE_PRFOP:
      enum_value = opnd->imm.value;
      assert (enum_value < ARRAY_SIZE (aarch64_sve_prfop_array));
      if (aarch64_sve_prfop_array[enum_value])
	snprintf (buf, size, "%s",
		  style_reg (styler, aarch64_sve_prfop_array[enum_value]));
      else
	snprintf (buf, size, "%s",
		  style_imm (styler, "#%" PRIi64, opnd->imm.value));
      break;

    case AARCH64_OPND_IMM_MOV:
      switch (aarch64_get_qualifier_esize (opnds[0].qualifier))
	{
	case 4:	/* e.g. MOV Wd, #<imm32>.  */
	    {
	      int imm32 = opnd->imm.value;
	      snprintf (buf, size, "%s",
			style_imm (styler, "#0x%-20x", imm32));
	      snprintf (comment, comment_size, "#%d", imm32);
	    }
	  break;
	case 8:	/* e.g. MOV Xd, #<imm64>.  */
	  snprintf (buf, size, "%s", style_imm (styler, "#0x%-20" PRIx64,
						opnd->imm.value));
	  snprintf (comment, comment_size, "#%" PRIi64, opnd->imm.value);
	  break;
	default:
	  snprintf (buf, size, "<invalid>");
	  break;
	}
      break;

    case AARCH64_OPND_FPIMM0:
      snprintf (buf, size, "%s", style_imm (styler, "#0.0"));
      break;

    case AARCH64_OPND_LIMM:
    case AARCH64_OPND_AIMM:
    case AARCH64_OPND_HALF:
    case AARCH64_OPND_SVE_INV_LIMM:
    case AARCH64_OPND_SVE_LIMM:
    case AARCH64_OPND_SVE_LIMM_MOV:
      if (opnd->shifter.amount)
	snprintf (buf, size, "%s, %s %s",
		  style_imm (styler, "#0x%" PRIx64, opnd->imm.value),
		  style_sub_mnem (styler, "lsl"),
		  style_imm (styler, "#%" PRIi64, opnd->shifter.amount));
      else
	snprintf (buf, size, "%s",
		  style_imm (styler, "#0x%" PRIx64, opnd->imm.value));
      break;

    case AARCH64_OPND_SIMD_IMM:
    case AARCH64_OPND_SIMD_IMM_SFT:
      if ((! opnd->shifter.amount && opnd->shifter.kind == AARCH64_MOD_LSL)
	  || opnd->shifter.kind == AARCH64_MOD_NONE)
	snprintf (buf, size, "%s",
		  style_imm (styler, "#0x%" PRIx64, opnd->imm.value));
      else
	snprintf (buf, size, "%s, %s %s",
		  style_imm (styler, "#0x%" PRIx64, opnd->imm.value),
		  style_sub_mnem (styler, aarch64_operand_modifiers[opnd->shifter.kind].name),
		  style_imm (styler, "#%" PRIi64, opnd->shifter.amount));
      break;

    case AARCH64_OPND_SVE_AIMM:
    case AARCH64_OPND_SVE_ASIMM:
      if (opnd->shifter.amount)
	snprintf (buf, size, "%s, %s %s",
		  style_imm (styler, "#%" PRIi64, opnd->imm.value),
		  style_sub_mnem (styler, "lsl"),
		  style_imm (styler, "#%" PRIi64, opnd->shifter.amount));
      else
	snprintf (buf, size, "%s",
		  style_imm (styler, "#%" PRIi64, opnd->imm.value));
      break;

    case AARCH64_OPND_FPIMM:
    case AARCH64_OPND_SIMD_FPIMM:
    case AARCH64_OPND_SVE_FPIMM8:
      switch (aarch64_get_qualifier_esize (opnds[0].qualifier))
	{
	case 2:	/* e.g. FMOV <Hd>, #<imm>.  */
	    {
	      half_conv_t c;
	      c.i = expand_fp_imm (2, opnd->imm.value);
	      snprintf (buf, size, "%s", style_imm (styler, "#%.18e", c.f));
	    }
	  break;
	case 4:	/* e.g. FMOV <Vd>.4S, #<imm>.  */
	    {
	      single_conv_t c;
	      c.i = expand_fp_imm (4, opnd->imm.value);
	      snprintf (buf, size, "%s", style_imm (styler, "#%.18e", c.f));
	    }
	  break;
	case 8:	/* e.g. FMOV <Sd>, #<imm>.  */
	    {
	      double_conv_t c;
	      c.i = expand_fp_imm (8, opnd->imm.value);
	      snprintf (buf, size, "%s", style_imm (styler, "#%.18e", c.d));
	    }
	  break;
	default:
	  snprintf (buf, size, "<invalid>");
	  break;
	}
      break;

    case AARCH64_OPND_CCMP_IMM:
    case AARCH64_OPND_NZCV:
    case AARCH64_OPND_EXCEPTION:
    case AARCH64_OPND_UIMM4:
    case AARCH64_OPND_UIMM4_ADDG:
    case AARCH64_OPND_UIMM7:
    case AARCH64_OPND_UIMM10:
      if (optional_operand_p (opcode, idx)
	  && (opnd->imm.value ==
	      (int64_t) get_optional_operand_default_value (opcode)))
	/* Omit the operand, e.g. DCPS1.  */
	break;
      snprintf (buf, size, "%s",
		style_imm (styler, "#0x%x", (unsigned int) opnd->imm.value));
      break;

    case AARCH64_OPND_COND:
    case AARCH64_OPND_COND1:
      snprintf (buf, size, "%s",
		style_sub_mnem (styler, opnd->cond->names[0]));
      num_conds = ARRAY_SIZE (opnd->cond->names);
      for (i = 1; i < num_conds && opnd->cond->names[i]; ++i)
	{
	  size_t len = comment != NULL ? strlen (comment) : 0;
	  if (i == 1)
	    snprintf (comment + len, comment_size - len, "%s = %s",
		      opnd->cond->names[0], opnd->cond->names[i]);
	  else
	    snprintf (comment + len, comment_size - len, ", %s",
		      opnd->cond->names[i]);
	}
      break;

    case AARCH64_OPND_ADDR_ADRP:
      addr = ((pc + AARCH64_PCREL_OFFSET) & ~(uint64_t)0xfff)
	+ opnd->imm.value;
      if (pcrel_p)
	*pcrel_p = 1;
      if (address)
	*address = addr;
      /* This is not necessary during the disassembling, as print_address_func
	 in the disassemble_info will take care of the printing.  But some
	 other callers may be still interested in getting the string in *STR,
	 so here we do snprintf regardless.  */
      snprintf (buf, size, "%s", style_addr (styler, "#0x%" PRIx64 , addr));
      break;

    case AARCH64_OPND_ADDR_PCREL14:
    case AARCH64_OPND_ADDR_PCREL19:
    case AARCH64_OPND_ADDR_PCREL21:
    case AARCH64_OPND_ADDR_PCREL26:
      addr = pc + AARCH64_PCREL_OFFSET + opnd->imm.value;
      if (pcrel_p)
	*pcrel_p = 1;
      if (address)
	*address = addr;
      /* This is not necessary during the disassembling, as print_address_func
	 in the disassemble_info will take care of the printing.  But some
	 other callers may be still interested in getting the string in *STR,
	 so here we do snprintf regardless.  */
      snprintf (buf, size, "%s", style_addr (styler, "#0x%" PRIx64, addr));
      break;

    case AARCH64_OPND_ADDR_SIMPLE:
    case AARCH64_OPND_SIMD_ADDR_SIMPLE:
    case AARCH64_OPND_SIMD_ADDR_POST:
      name = get_64bit_int_reg_name (opnd->addr.base_regno, 1);
      if (opnd->type == AARCH64_OPND_SIMD_ADDR_POST)
	{
	  if (opnd->addr.offset.is_reg)
	    snprintf (buf, size, "[%s], %s",
		      style_reg (styler, name),
		      style_reg (styler, "x%d", opnd->addr.offset.regno));
	  else
	    snprintf (buf, size, "[%s], %s",
		      style_reg (styler, name),
		      style_imm (styler, "#%d", opnd->addr.offset.imm));
	}
      else
	snprintf (buf, size, "[%s]", style_reg (styler, name));
      break;

    case AARCH64_OPND_ADDR_REGOFF:
    case AARCH64_OPND_SVE_ADDR_R:
    case AARCH64_OPND_SVE_ADDR_RR:
    case AARCH64_OPND_SVE_ADDR_RR_LSL1:
    case AARCH64_OPND_SVE_ADDR_RR_LSL2:
    case AARCH64_OPND_SVE_ADDR_RR_LSL3:
    case AARCH64_OPND_SVE_ADDR_RR_LSL4:
    case AARCH64_OPND_SVE_ADDR_RX:
    case AARCH64_OPND_SVE_ADDR_RX_LSL1:
    case AARCH64_OPND_SVE_ADDR_RX_LSL2:
    case AARCH64_OPND_SVE_ADDR_RX_LSL3:
      print_register_offset_address
	(buf, size, opnd, get_64bit_int_reg_name (opnd->addr.base_regno, 1),
	 get_offset_int_reg_name (opnd), styler);
      break;

    case AARCH64_OPND_SVE_ADDR_ZX:
      print_register_offset_address
	(buf, size, opnd,
	 get_addr_sve_reg_name (opnd->addr.base_regno, opnd->qualifier),
	 get_64bit_int_reg_name (opnd->addr.offset.regno, 0), styler);
      break;

    case AARCH64_OPND_SVE_ADDR_RZ:
    case AARCH64_OPND_SVE_ADDR_RZ_LSL1:
    case AARCH64_OPND_SVE_ADDR_RZ_LSL2:
    case AARCH64_OPND_SVE_ADDR_RZ_LSL3:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW_14:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW_22:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW1_14:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW1_22:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW2_14:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW2_22:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW3_14:
    case AARCH64_OPND_SVE_ADDR_RZ_XTW3_22:
      print_register_offset_address
	(buf, size, opnd, get_64bit_int_reg_name (opnd->addr.base_regno, 1),
	 get_addr_sve_reg_name (opnd->addr.offset.regno, opnd->qualifier),
	 styler);
      break;

    case AARCH64_OPND_ADDR_SIMM7:
    case AARCH64_OPND_ADDR_SIMM9:
    case AARCH64_OPND_ADDR_SIMM9_2:
    case AARCH64_OPND_ADDR_SIMM10:
    case AARCH64_OPND_ADDR_SIMM11:
    case AARCH64_OPND_ADDR_SIMM13:
    case AARCH64_OPND_RCPC3_ADDR_OFFSET:
    case AARCH64_OPND_ADDR_OFFSET:
    case AARCH64_OPND_RCPC3_ADDR_OPT_POSTIND:
    case AARCH64_OPND_RCPC3_ADDR_OPT_PREIND_WB:
    case AARCH64_OPND_RCPC3_ADDR_POSTIND:
    case AARCH64_OPND_RCPC3_ADDR_PREIND_WB:
    case AARCH64_OPND_SME_ADDR_RI_U4xVL:
    case AARCH64_OPND_SVE_ADDR_RI_S4x16:
    case AARCH64_OPND_SVE_ADDR_RI_S4x32:
    case AARCH64_OPND_SVE_ADDR_RI_S4xVL:
    case AARCH64_OPND_SVE_ADDR_RI_S4x2xVL:
    case AARCH64_OPND_SVE_ADDR_RI_S4x3xVL:
    case AARCH64_OPND_SVE_ADDR_RI_S4x4xVL:
    case AARCH64_OPND_SVE_ADDR_RI_S6xVL:
    case AARCH64_OPND_SVE_ADDR_RI_S9xVL:
    case AARCH64_OPND_SVE_ADDR_RI_U6:
    case AARCH64_OPND_SVE_ADDR_RI_U6x2:
    case AARCH64_OPND_SVE_ADDR_RI_U6x4:
    case AARCH64_OPND_SVE_ADDR_RI_U6x8:
      print_immediate_offset_address
	(buf, size, opnd, get_64bit_int_reg_name (opnd->addr.base_regno, 1),
	 styler);
      break;

    case AARCH64_OPND_SVE_ADDR_ZI_U5:
    case AARCH64_OPND_SVE_ADDR_ZI_U5x2:
    case AARCH64_OPND_SVE_ADDR_ZI_U5x4:
    case AARCH64_OPND_SVE_ADDR_ZI_U5x8:
      print_immediate_offset_address
	(buf, size, opnd,
	 get_addr_sve_reg_name (opnd->addr.base_regno, opnd->qualifier),
	 styler);
      break;

    case AARCH64_OPND_SVE_ADDR_ZZ_LSL:
    case AARCH64_OPND_SVE_ADDR_ZZ_SXTW:
    case AARCH64_OPND_SVE_ADDR_ZZ_UXTW:
      print_register_offset_address
	(buf, size, opnd,
	 get_addr_sve_reg_name (opnd->addr.base_regno, opnd->qualifier),
	 get_addr_sve_reg_name (opnd->addr.offset.regno, opnd->qualifier),
	 styler);
      break;

    case AARCH64_OPND_ADDR_UIMM12:
      name = get_64bit_int_reg_name (opnd->addr.base_regno, 1);
      if (opnd->addr.offset.imm)
	snprintf (buf, size, "[%s, %s]",
		  style_reg (styler, name),
		  style_imm (styler, "#%d", opnd->addr.offset.imm));
      else
	snprintf (buf, size, "[%s]", style_reg (styler, name));
      break;

    case AARCH64_OPND_SYSREG:
    case AARCH64_OPND_SYSREG128:
      for (i = 0; aarch64_sys_regs[i].name; ++i)
	{
	  const aarch64_sys_reg *sr = aarch64_sys_regs + i;

	  bool exact_match
	    = (!(sr->flags & (F_REG_READ | F_REG_WRITE))
	    || (sr->flags & opnd->sysreg.flags) == opnd->sysreg.flags)
	    && AARCH64_CPU_HAS_ALL_FEATURES (features, sr->features);

	  /* Try and find an exact match, But if that fails, return the first
	     partial match that was found.  */
	  if (aarch64_sys_regs[i].value == opnd->sysreg.value
	      && ! aarch64_sys_reg_deprecated_p (aarch64_sys_regs[i].flags)
	      && ! aarch64_sys_reg_alias_p (aarch64_sys_regs[i].flags)
	      && (name == NULL || exact_match))
	    {
	      name = aarch64_sys_regs[i].name;
	      if (exact_match)
		{
		  if (notes)
		    *notes = NULL;
		  break;
		}

	      /* If we didn't match exactly, that means the presense of a flag
		 indicates what we didn't want for this instruction.  e.g. If
		 F_REG_READ is there, that means we were looking for a write
		 register.  See aarch64_ext_sysreg.  */
	      if (aarch64_sys_regs[i].flags & F_REG_WRITE)
		*notes = _("reading from a write-only register");
	      else if (aarch64_sys_regs[i].flags & F_REG_READ)
		*notes = _("writing to a read-only register");
	    }
	}

      if (name)
	snprintf (buf, size, "%s", style_reg (styler, name));
      else
	{
	  /* Implementation defined system register.  */
	  unsigned int value = opnd->sysreg.value;
	  snprintf (buf, size, "%s",
		    style_reg (styler, "s%u_%u_c%u_c%u_%u",
			       (value >> 14) & 0x3, (value >> 11) & 0x7,
			       (value >> 7) & 0xf, (value >> 3) & 0xf,
			       value & 0x7));
	}
      break;

    case AARCH64_OPND_PSTATEFIELD:
      for (i = 0; aarch64_pstatefields[i].name; ++i)
        if (aarch64_pstatefields[i].value == opnd->pstatefield)
          {
            /* PSTATEFIELD name is encoded partially in CRm[3:1] for SVCRSM,
               SVCRZA and SVCRSMZA.  */
            uint32_t flags = aarch64_pstatefields[i].flags;
            if (flags & F_REG_IN_CRM
                && (PSTATE_DECODE_CRM (opnd->sysreg.flags)
                    != PSTATE_DECODE_CRM (flags)))
              continue;
            break;
          }
      assert (aarch64_pstatefields[i].name);
      snprintf (buf, size, "%s",
		style_reg (styler, aarch64_pstatefields[i].name));
      break;

    case AARCH64_OPND_SYSREG_AT:
    case AARCH64_OPND_SYSREG_DC:
    case AARCH64_OPND_SYSREG_IC:
    case AARCH64_OPND_SYSREG_TLBI:
    case AARCH64_OPND_SYSREG_TLBIP:
    case AARCH64_OPND_SYSREG_SR:
      snprintf (buf, size, "%s", style_reg (styler, opnd->sysins_op->name));
      break;

    case AARCH64_OPND_BARRIER:
    case AARCH64_OPND_BARRIER_DSB_NXS:
      {
	if (opnd->barrier->name[0] == '#')
	  snprintf (buf, size, "%s", style_imm (styler, opnd->barrier->name));
	else
	  snprintf (buf, size, "%s",
		    style_sub_mnem (styler, opnd->barrier->name));
      }
      break;

    case AARCH64_OPND_BARRIER_ISB:
      /* Operand can be omitted, e.g. in DCPS1.  */
      if (! optional_operand_p (opcode, idx)
	  || (opnd->barrier->value
	      != get_optional_operand_default_value (opcode)))
	snprintf (buf, size, "%s",
		  style_imm (styler, "#0x%x", opnd->barrier->value));
      break;

    case AARCH64_OPND_PRFOP:
      if (opnd->prfop->name != NULL)
	snprintf (buf, size, "%s", style_sub_mnem (styler, opnd->prfop->name));
      else
	snprintf (buf, size, "%s", style_imm (styler, "#0x%02x",
					      opnd->prfop->value));
      break;

    case AARCH64_OPND_RPRFMOP:
      enum_value = opnd->imm.value;
      if (enum_value < ARRAY_SIZE (aarch64_rprfmop_array)
	  && aarch64_rprfmop_array[enum_value])
	snprintf (buf, size, "%s",
		  style_reg (styler, aarch64_rprfmop_array[enum_value]));
      else
	snprintf (buf, size, "%s",
		  style_imm (styler, "#%" PRIi64, opnd->imm.value));
      break;

    case AARCH64_OPND_BARRIER_PSB:
      snprintf (buf, size, "%s", style_sub_mnem (styler, "csync"));
      break;

    case AARCH64_OPND_X16:
      snprintf (buf, size, "%s", style_reg (styler, "x16"));
      break;

    case AARCH64_OPND_SME_ZT0:
      snprintf (buf, size, "%s", style_reg (styler, "zt0"));
      break;

    case AARCH64_OPND_SME_ZT0_INDEX:
      snprintf (buf, size, "%s[%s]", style_reg (styler, "zt0"),
		style_imm (styler, "%d", (int) opnd->imm.value));
      break;

    case AARCH64_OPND_SME_ZT0_LIST:
      snprintf (buf, size, "{%s}", style_reg (styler, "zt0"));
      break;

    case AARCH64_OPND_BARRIER_GCSB:
      snprintf (buf, size, "%s", style_sub_mnem (styler, "dsync"));
      break;

    case AARCH64_OPND_BTI_TARGET:
      if ((HINT_FLAG (opnd->hint_option->value) & HINT_OPD_F_NOPRINT) == 0)
	snprintf (buf, size, "%s",
		  style_sub_mnem (styler, opnd->hint_option->name));
      break;

    case AARCH64_OPND_MOPS_ADDR_Rd:
    case AARCH64_OPND_MOPS_ADDR_Rs:
      snprintf (buf, size, "[%s]!",
		style_reg (styler,
			   get_int_reg_name (opnd->reg.regno,
					     AARCH64_OPND_QLF_X, 0)));
      break;

    case AARCH64_OPND_MOPS_WB_Rn:
      snprintf (buf, size, "%s!",
		style_reg (styler, get_int_reg_name (opnd->reg.regno,
						     AARCH64_OPND_QLF_X, 0)));
      break;

    default:
      snprintf (buf, size, "<invalid>");
      break;
    }
}

#define CPENC(op0,op1,crn,crm,op2) \
  ((((op0) << 19) | ((op1) << 16) | ((crn) << 12) | ((crm) << 8) | ((op2) << 5)) >> 5)
  /* for 3.9.3 Instructions for Accessing Special Purpose Registers */
#define CPEN_(op1,crm,op2) CPENC(3,(op1),4,(crm),(op2))
  /* for 3.9.10 System Instructions */
#define CPENS(op1,crn,crm,op2) CPENC(1,(op1),(crn),(crm),(op2))

#define C0  0
#define C1  1
#define C2  2
#define C3  3
#define C4  4
#define C5  5
#define C6  6
#define C7  7
#define C8  8
#define C9  9
#define C10 10
#define C11 11
#define C12 12
#define C13 13
#define C14 14
#define C15 15

/* TODO there is one more issues need to be resolved
   1. handle cpu-implementation-defined system registers.

   Note that the F_REG_{READ,WRITE} flags mean read-only and write-only
   respectively.  If neither of these are set then the register is read-write.  */
const aarch64_sys_reg aarch64_sys_regs [] =
{
  #define SYSREG(name, encoding, flags, features) \
    { name, encoding, flags, features },
  #include "aarch64-sys-regs.def"
  { 0, CPENC (0,0,0,0,0), 0, AARCH64_NO_FEATURES }
  #undef SYSREG
};

bool
aarch64_sys_reg_deprecated_p (const uint32_t reg_flags)
{
  return (reg_flags & F_DEPRECATED) != 0;
}

bool
aarch64_sys_reg_128bit_p (const uint32_t reg_flags)
{
  return (reg_flags & F_REG_128) != 0;
}

bool
aarch64_sys_reg_alias_p (const uint32_t reg_flags)
{
  return (reg_flags & F_REG_ALIAS) != 0;
}

/* The CPENC below is fairly misleading, the fields
   here are not in CPENC form. They are in op2op1 form. The fields are encoded
   by ins_pstatefield, which just shifts the value by the width of the fields
   in a loop. So if you CPENC them only the first value will be set, the rest
   are masked out to 0. As an example. op2 = 3, op1=2. CPENC would produce a
   value of 0b110000000001000000 (0x30040) while what you want is
   0b011010 (0x1a).  */
const aarch64_sys_reg aarch64_pstatefields [] =
{
  { "spsel",	0x05, F_REG_MAX_VALUE (1), AARCH64_NO_FEATURES },
  { "daifset",	0x1e, F_REG_MAX_VALUE (15), AARCH64_NO_FEATURES },
  { "daifclr",	0x1f, F_REG_MAX_VALUE (15), AARCH64_NO_FEATURES },
  { "pan",	0x04, F_REG_MAX_VALUE (1) | F_ARCHEXT, AARCH64_FEATURE (PAN) },
  { "uao",	0x03, F_REG_MAX_VALUE (1) | F_ARCHEXT, AARCH64_FEATURE (V8_2A) },
  { "ssbs",	0x19, F_REG_MAX_VALUE (1) | F_ARCHEXT, AARCH64_FEATURE (SSBS) },
  { "dit",	0x1a, F_REG_MAX_VALUE (1) | F_ARCHEXT, AARCH64_FEATURE (V8_4A) },
  { "tco",	0x1c, F_REG_MAX_VALUE (1) | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
  { "svcrsm",	0x1b, PSTATE_ENCODE_CRM_AND_IMM (0x2,0x1) | F_REG_MAX_VALUE (1)
		      | F_ARCHEXT, AARCH64_FEATURE (SME) },
  { "svcrza",	0x1b, PSTATE_ENCODE_CRM_AND_IMM (0x4,0x1) | F_REG_MAX_VALUE (1)
		      | F_ARCHEXT, AARCH64_FEATURE (SME) },
  { "svcrsmza",	0x1b, PSTATE_ENCODE_CRM_AND_IMM (0x6,0x1) | F_REG_MAX_VALUE (1)
		      | F_ARCHEXT, AARCH64_FEATURE (SME) },
  { "allint",	0x08, F_REG_MAX_VALUE (1) | F_ARCHEXT, AARCH64_FEATURE (V8_8A) },
  { 0,	CPENC (0,0,0,0,0), 0, AARCH64_NO_FEATURES },
};

bool
aarch64_pstatefield_supported_p (const aarch64_feature_set features,
				 const aarch64_sys_reg *reg)
{
  if (!(reg->flags & F_ARCHEXT))
    return true;

  return AARCH64_CPU_HAS_ALL_FEATURES (features, reg->features);
}

const aarch64_sys_ins_reg aarch64_sys_regs_ic[] =
{
    { "ialluis", CPENS(0,C7,C1,0), 0, AARCH64_NO_FEATURES },
    { "iallu",   CPENS(0,C7,C5,0), 0, AARCH64_NO_FEATURES },
    { "ivau",    CPENS (3, C7, C5, 1), F_HASXT, AARCH64_NO_FEATURES },
    { 0, CPENS(0,0,0,0), 0, AARCH64_NO_FEATURES }
};

const aarch64_sys_ins_reg aarch64_sys_regs_dc[] =
{
    { "zva",	    CPENS (3, C7, C4, 1),  F_HASXT, AARCH64_NO_FEATURES },
    { "gva",	    CPENS (3, C7, C4, 3),  F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "gzva",	    CPENS (3, C7, C4, 4),  F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "ivac",       CPENS (0, C7, C6, 1),  F_HASXT, AARCH64_NO_FEATURES },
    { "igvac",      CPENS (0, C7, C6, 3),  F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "igsw",       CPENS (0, C7, C6, 4),  F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "isw",	    CPENS (0, C7, C6, 2),  F_HASXT, AARCH64_NO_FEATURES },
    { "igdvac",	    CPENS (0, C7, C6, 5),  F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "igdsw",	    CPENS (0, C7, C6, 6),  F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cvac",       CPENS (3, C7, C10, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "cgvac",      CPENS (3, C7, C10, 3), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cgdvac",     CPENS (3, C7, C10, 5), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "csw",	    CPENS (0, C7, C10, 2), F_HASXT, AARCH64_NO_FEATURES },
    { "cgsw",       CPENS (0, C7, C10, 4), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cgdsw",	    CPENS (0, C7, C10, 6), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cvau",       CPENS (3, C7, C11, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "cvap",       CPENS (3, C7, C12, 1), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (V8_2A) },
    { "cgvap",      CPENS (3, C7, C12, 3), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cgdvap",     CPENS (3, C7, C12, 5), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cvadp",      CPENS (3, C7, C13, 1), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (CVADP) },
    { "cgvadp",     CPENS (3, C7, C13, 3), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cgdvadp",    CPENS (3, C7, C13, 5), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "civac",      CPENS (3, C7, C14, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "cigvac",     CPENS (3, C7, C14, 3), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cigdvac",    CPENS (3, C7, C14, 5), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cisw",       CPENS (0, C7, C14, 2), F_HASXT, AARCH64_NO_FEATURES },
    { "cigsw",      CPENS (0, C7, C14, 4), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cigdsw",     CPENS (0, C7, C14, 6), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (MEMTAG) },
    { "cipapa",     CPENS (6, C7, C14, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "cigdpapa",   CPENS (6, C7, C14, 5), F_HASXT, AARCH64_NO_FEATURES },
    { 0,       CPENS(0,0,0,0), 0, AARCH64_NO_FEATURES }
};

const aarch64_sys_ins_reg aarch64_sys_regs_at[] =
{
    { "s1e1r",      CPENS (0, C7, C8, 0), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e1w",      CPENS (0, C7, C8, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e0r",      CPENS (0, C7, C8, 2), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e0w",      CPENS (0, C7, C8, 3), F_HASXT, AARCH64_NO_FEATURES },
    { "s12e1r",     CPENS (4, C7, C8, 4), F_HASXT, AARCH64_NO_FEATURES },
    { "s12e1w",     CPENS (4, C7, C8, 5), F_HASXT, AARCH64_NO_FEATURES },
    { "s12e0r",     CPENS (4, C7, C8, 6), F_HASXT, AARCH64_NO_FEATURES },
    { "s12e0w",     CPENS (4, C7, C8, 7), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e2r",      CPENS (4, C7, C8, 0), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e2w",      CPENS (4, C7, C8, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e3r",      CPENS (6, C7, C8, 0), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e3w",      CPENS (6, C7, C8, 1), F_HASXT, AARCH64_NO_FEATURES },
    { "s1e1rp",     CPENS (0, C7, C9, 0), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (V8_2A) },
    { "s1e1wp",     CPENS (0, C7, C9, 1), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (V8_2A) },
    { "s1e1a",      CPENS (0, C7, C9, 2), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (ATS1A) },
    { "s1e2a",      CPENS (4, C7, C9, 2), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (ATS1A) },
    { "s1e3a",      CPENS (6, C7, C9, 2), F_HASXT | F_ARCHEXT, AARCH64_FEATURE (ATS1A) },
    { 0,       CPENS(0,0,0,0), 0, AARCH64_NO_FEATURES }
};

const aarch64_sys_ins_reg aarch64_sys_regs_tlbi[] =
{
    { "rpaos",      CPENS (6, C8, C4, 3), F_HASXT, AARCH64_NO_FEATURES },
    { "rpalos",     CPENS (6, C8, C4, 7), F_HASXT, AARCH64_NO_FEATURES },
    { "paallos",    CPENS (6, C8, C1, 4), 0, AARCH64_NO_FEATURES },
    { "paall",      CPENS (6, C8, C7, 4), 0, AARCH64_NO_FEATURES },

#define TLBI_XS_OP(OP, CODE, FLAGS) \
    { OP, CODE, FLAGS, AARCH64_NO_FEATURES }, \
    { OP "nxs", CODE | CPENS (0, C9, 0, 0), FLAGS | F_ARCHEXT, AARCH64_FEATURE (XS) },

    TLBI_XS_OP ( "vmalle1",   CPENS (0, C8, C7, 0), 0)
    TLBI_XS_OP ( "vae1",      CPENS (0, C8, C7, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "aside1",    CPENS (0, C8, C7, 2), F_HASXT )
    TLBI_XS_OP ( "vaae1",     CPENS (0, C8, C7, 3), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vmalle1is", CPENS (0, C8, C3, 0), 0)
    TLBI_XS_OP ( "vae1is",    CPENS (0, C8, C3, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "aside1is",  CPENS (0, C8, C3, 2), F_HASXT )
    TLBI_XS_OP ( "vaae1is",   CPENS (0, C8, C3, 3), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "ipas2e1is", CPENS (4, C8, C0, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "ipas2le1is",CPENS (4, C8, C0, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "ipas2e1",   CPENS (4, C8, C4, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "ipas2le1",  CPENS (4, C8, C4, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vae2",      CPENS (4, C8, C7, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vae2is",    CPENS (4, C8, C3, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vmalls12e1",CPENS (4, C8, C7, 6), 0)
    TLBI_XS_OP ( "vmalls12e1is",CPENS(4,C8, C3, 6), 0)
    TLBI_XS_OP ( "vae3",      CPENS (6, C8, C7, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vae3is",    CPENS (6, C8, C3, 1), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "alle2",     CPENS (4, C8, C7, 0), 0)
    TLBI_XS_OP ( "alle2is",   CPENS (4, C8, C3, 0), 0)
    TLBI_XS_OP ( "alle1",     CPENS (4, C8, C7, 4), 0)
    TLBI_XS_OP ( "alle1is",   CPENS (4, C8, C3, 4), 0)
    TLBI_XS_OP ( "alle3",     CPENS (6, C8, C7, 0), 0)
    TLBI_XS_OP ( "alle3is",   CPENS (6, C8, C3, 0), 0)
    TLBI_XS_OP ( "vale1is",   CPENS (0, C8, C3, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vale2is",   CPENS (4, C8, C3, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vale3is",   CPENS (6, C8, C3, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vaale1is",  CPENS (0, C8, C3, 7), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vale1",     CPENS (0, C8, C7, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vale2",     CPENS (4, C8, C7, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vale3",     CPENS (6, C8, C7, 5), F_HASXT | F_REG_128)
    TLBI_XS_OP ( "vaale1",    CPENS (0, C8, C7, 7), F_HASXT | F_REG_128)

#undef TLBI_XS_OP
#define TLBI_XS_OP(OP, CODE, FLAGS) \
    { OP, CODE, FLAGS | F_ARCHEXT, AARCH64_FEATURE (V8_4A) }, \
    { OP "nxs", CODE | CPENS (0, C9, 0, 0), FLAGS | F_ARCHEXT, AARCH64_FEATURE (XS) },

    TLBI_XS_OP ( "vmalle1os",    CPENS (0, C8, C1, 0), 0 )
    TLBI_XS_OP ( "vae1os",       CPENS (0, C8, C1, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "aside1os",     CPENS (0, C8, C1, 2), F_HASXT )
    TLBI_XS_OP ( "vaae1os",      CPENS (0, C8, C1, 3), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "vale1os",      CPENS (0, C8, C1, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "vaale1os",     CPENS (0, C8, C1, 7), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ipas2e1os",    CPENS (4, C8, C4, 0), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ipas2le1os",   CPENS (4, C8, C4, 4), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "vae2os",       CPENS (4, C8, C1, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "vale2os",      CPENS (4, C8, C1, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "vmalls12e1os", CPENS (4, C8, C1, 6), 0 )
    TLBI_XS_OP ( "vae3os",       CPENS (6, C8, C1, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "vale3os",      CPENS (6, C8, C1, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "alle2os",      CPENS (4, C8, C1, 0), 0 )
    TLBI_XS_OP ( "alle1os",      CPENS (4, C8, C1, 4), 0 )
    TLBI_XS_OP ( "alle3os",      CPENS (6, C8, C1, 0), 0 )

    TLBI_XS_OP ( "rvae1",      CPENS (0, C8, C6, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvaae1",     CPENS (0, C8, C6, 3), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale1",     CPENS (0, C8, C6, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvaale1",    CPENS (0, C8, C6, 7), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae1is",    CPENS (0, C8, C2, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvaae1is",   CPENS (0, C8, C2, 3), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale1is",   CPENS (0, C8, C2, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvaale1is",  CPENS (0, C8, C2, 7), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae1os",    CPENS (0, C8, C5, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvaae1os",   CPENS (0, C8, C5, 3), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale1os",   CPENS (0, C8, C5, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvaale1os",  CPENS (0, C8, C5, 7), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ripas2e1is", CPENS (4, C8, C0, 2), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ripas2le1is",CPENS (4, C8, C0, 6), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ripas2e1",   CPENS (4, C8, C4, 2), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ripas2le1",  CPENS (4, C8, C4, 6), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ripas2e1os", CPENS (4, C8, C4, 3), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "ripas2le1os",CPENS (4, C8, C4, 7), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae2",      CPENS (4, C8, C6, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale2",     CPENS (4, C8, C6, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae2is",    CPENS (4, C8, C2, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale2is",   CPENS (4, C8, C2, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae2os",    CPENS (4, C8, C5, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale2os",   CPENS (4, C8, C5, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae3",      CPENS (6, C8, C6, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale3",     CPENS (6, C8, C6, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae3is",    CPENS (6, C8, C2, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale3is",   CPENS (6, C8, C2, 5), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvae3os",    CPENS (6, C8, C5, 1), F_HASXT | F_REG_128 )
    TLBI_XS_OP ( "rvale3os",   CPENS (6, C8, C5, 5), F_HASXT | F_REG_128 )

#undef TLBI_XS_OP

    { 0,       CPENS(0,0,0,0), 0, AARCH64_NO_FEATURES }
};

const aarch64_sys_ins_reg aarch64_sys_regs_sr[] =
{
    /* RCTX is somewhat unique in a way that it has different values
       (op2) based on the instruction in which it is used (cfp/dvp/cpp).
       Thus op2 is masked out and instead encoded directly in the
       aarch64_opcode_table entries for the respective instructions.  */
    { "rctx",   CPENS(3,C7,C3,0), F_HASXT | F_ARCHEXT | F_REG_WRITE, AARCH64_FEATURE (PREDRES) }, /* WO */
    { 0,       CPENS(0,0,0,0), 0, AARCH64_NO_FEATURES }
};

bool
aarch64_sys_ins_reg_has_xt (const aarch64_sys_ins_reg *sys_ins_reg)
{
  return (sys_ins_reg->flags & F_HASXT) != 0;
}

extern bool
aarch64_sys_ins_reg_supported_p (const aarch64_feature_set features,
				 const char *reg_name,
				 uint32_t reg_flags,
				 const aarch64_feature_set *reg_features)
{
  /* Armv8-R has no EL3.  */
  if (AARCH64_CPU_HAS_FEATURE (features, V8R))
    {
      const char *suffix = strrchr (reg_name, '_');
      if (suffix && !strcmp (suffix, "_el3"))
	return false;
    }

  if (!(reg_flags & F_ARCHEXT))
    return true;

  return AARCH64_CPU_HAS_ALL_FEATURES (features, *reg_features);
}

#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
#undef C8
#undef C9
#undef C10
#undef C11
#undef C12
#undef C13
#undef C14
#undef C15

#define BIT(INSN,BT)     (((INSN) >> (BT)) & 1)
#define BITS(INSN,HI,LO) (((INSN) >> (LO)) & ((1 << (((HI) - (LO)) + 1)) - 1))

static enum err_type
verify_ldpsw (const struct aarch64_inst *inst ATTRIBUTE_UNUSED,
	      const aarch64_insn insn, bfd_vma pc ATTRIBUTE_UNUSED,
	      bool encoding ATTRIBUTE_UNUSED,
	      aarch64_operand_error *mismatch_detail ATTRIBUTE_UNUSED,
	      aarch64_instr_sequence *insn_sequence ATTRIBUTE_UNUSED)
{
  int t  = BITS (insn, 4, 0);
  int n  = BITS (insn, 9, 5);
  int t2 = BITS (insn, 14, 10);

  if (BIT (insn, 23))
    {
      /* Write back enabled.  */
      if ((t == n || t2 == n) && n != 31)
	return ERR_UND;
    }

  if (BIT (insn, 22))
    {
      /* Load */
      if (t == t2)
	return ERR_UND;
    }

  return ERR_OK;
}

/* Verifier for vector by element 3 operands functions where the
   conditions `if sz:L == 11 then UNDEFINED` holds.  */

static enum err_type
verify_elem_sd (const struct aarch64_inst *inst, const aarch64_insn insn,
		bfd_vma pc ATTRIBUTE_UNUSED, bool encoding,
		aarch64_operand_error *mismatch_detail ATTRIBUTE_UNUSED,
		aarch64_instr_sequence *insn_sequence ATTRIBUTE_UNUSED)
{
  const aarch64_insn undef_pattern = 0x3;
  aarch64_insn value;

  assert (inst->opcode);
  assert (inst->opcode->operands[2] == AARCH64_OPND_Em);
  value = encoding ? inst->value : insn;
  assert (value);

  if (undef_pattern == extract_fields (value, 0, 2, FLD_sz, FLD_L))
    return ERR_UND;

  return ERR_OK;
}

/* Check an instruction that takes three register operands and that
   requires the register numbers to be distinct from one another.  */

static enum err_type
verify_three_different_regs (const struct aarch64_inst *inst,
			     const aarch64_insn insn ATTRIBUTE_UNUSED,
			     bfd_vma pc ATTRIBUTE_UNUSED,
			     bool encoding ATTRIBUTE_UNUSED,
			     aarch64_operand_error *mismatch_detail
			       ATTRIBUTE_UNUSED,
			     aarch64_instr_sequence *insn_sequence
			       ATTRIBUTE_UNUSED)
{
  int rd, rs, rn;

  rd = inst->operands[0].reg.regno;
  rs = inst->operands[1].reg.regno;
  rn = inst->operands[2].reg.regno;
  if (rd == rs || rd == rn || rs == rn)
    {
      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
      mismatch_detail->error
	= _("the three register operands must be distinct from one another");
      mismatch_detail->index = -1;
      return ERR_UND;
    }

  return ERR_OK;
}

/* Add INST to the end of INSN_SEQUENCE.  */

static void
add_insn_to_sequence (const struct aarch64_inst *inst,
		      aarch64_instr_sequence *insn_sequence)
{
  insn_sequence->instr[insn_sequence->num_added_insns++] = *inst;
}

/* Initialize an instruction sequence insn_sequence with the instruction INST.
   If INST is NULL the given insn_sequence is cleared and the sequence is left
   uninitialized.  */

void
init_insn_sequence (const struct aarch64_inst *inst,
		    aarch64_instr_sequence *insn_sequence)
{
  int num_req_entries = 0;

  if (insn_sequence->instr)
    {
      XDELETE (insn_sequence->instr);
      insn_sequence->instr = NULL;
    }

  /* Handle all the cases here.  May need to think of something smarter than
     a giant if/else chain if this grows.  At that time, a lookup table may be
     best.  */
  if (inst && inst->opcode->constraints & C_SCAN_MOVPRFX)
    num_req_entries = 1;
  if (inst && (inst->opcode->constraints & C_SCAN_MOPS_PME) == C_SCAN_MOPS_P)
    num_req_entries = 2;

  insn_sequence->num_added_insns = 0;
  insn_sequence->num_allocated_insns = num_req_entries;

  if (num_req_entries != 0)
    {
      insn_sequence->instr = XCNEWVEC (aarch64_inst, num_req_entries);
      add_insn_to_sequence (inst, insn_sequence);
    }
}

/* Subroutine of verify_constraints.  Check whether the instruction
   is part of a MOPS P/M/E sequence and, if so, whether sequencing
   expectations are met.  Return true if the check passes, otherwise
   describe the problem in MISMATCH_DETAIL.

   IS_NEW_SECTION is true if INST is assumed to start a new section.
   The other arguments are as for verify_constraints.  */

static bool
verify_mops_pme_sequence (const struct aarch64_inst *inst,
			  bool is_new_section,
			  aarch64_operand_error *mismatch_detail,
			  aarch64_instr_sequence *insn_sequence)
{
  const struct aarch64_opcode *opcode;
  const struct aarch64_inst *prev_insn;
  int i;

  opcode = inst->opcode;
  if (insn_sequence->instr)
    prev_insn = insn_sequence->instr + (insn_sequence->num_added_insns - 1);
  else
    prev_insn = NULL;

  if (prev_insn
      && (prev_insn->opcode->constraints & C_SCAN_MOPS_PME)
      && prev_insn->opcode != opcode - 1)
    {
      mismatch_detail->kind = AARCH64_OPDE_EXPECTED_A_AFTER_B;
      mismatch_detail->error = NULL;
      mismatch_detail->index = -1;
      mismatch_detail->data[0].s = prev_insn->opcode[1].name;
      mismatch_detail->data[1].s = prev_insn->opcode->name;
      mismatch_detail->non_fatal = true;
      return false;
    }

  if (opcode->constraints & C_SCAN_MOPS_PME)
    {
      if (is_new_section || !prev_insn || prev_insn->opcode != opcode - 1)
	{
	  mismatch_detail->kind = AARCH64_OPDE_A_SHOULD_FOLLOW_B;
	  mismatch_detail->error = NULL;
	  mismatch_detail->index = -1;
	  mismatch_detail->data[0].s = opcode->name;
	  mismatch_detail->data[1].s = opcode[-1].name;
	  mismatch_detail->non_fatal = true;
	  return false;
	}

      for (i = 0; i < 3; ++i)
	/* There's no specific requirement for the data register to be
	   the same between consecutive SET* instructions.  */
	if ((opcode->operands[i] == AARCH64_OPND_MOPS_ADDR_Rd
	     || opcode->operands[i] == AARCH64_OPND_MOPS_ADDR_Rs
	     || opcode->operands[i] == AARCH64_OPND_MOPS_WB_Rn)
	    && prev_insn->operands[i].reg.regno != inst->operands[i].reg.regno)
	  {
	    mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	    if (opcode->operands[i] == AARCH64_OPND_MOPS_ADDR_Rd)
	      mismatch_detail->error = _("destination register differs from "
					 "preceding instruction");
	    else if (opcode->operands[i] == AARCH64_OPND_MOPS_ADDR_Rs)
	      mismatch_detail->error = _("source register differs from "
					 "preceding instruction");
	    else
	      mismatch_detail->error = _("size register differs from "
					 "preceding instruction");
	    mismatch_detail->index = i;
	    mismatch_detail->non_fatal = true;
	    return false;
	  }
    }

  return true;
}

/*  This function verifies that the instruction INST adheres to its specified
    constraints.  If it does then ERR_OK is returned, if not then ERR_VFI is
    returned and MISMATCH_DETAIL contains the reason why verification failed.

    The function is called both during assembly and disassembly.  If assembling
    then ENCODING will be TRUE, else FALSE.  If dissassembling PC will be set
    and will contain the PC of the current instruction w.r.t to the section.

    If ENCODING and PC=0 then you are at a start of a section.  The constraints
    are verified against the given state insn_sequence which is updated as it
    transitions through the verification.  */

enum err_type
verify_constraints (const struct aarch64_inst *inst,
		    const aarch64_insn insn ATTRIBUTE_UNUSED,
		    bfd_vma pc,
		    bool encoding,
		    aarch64_operand_error *mismatch_detail,
		    aarch64_instr_sequence *insn_sequence)
{
  assert (inst);
  assert (inst->opcode);

  const struct aarch64_opcode *opcode = inst->opcode;
  if (!opcode->constraints && !insn_sequence->instr)
    return ERR_OK;

  assert (insn_sequence);

  enum err_type res = ERR_OK;

  /* This instruction puts a constraint on the insn_sequence.  */
  if (opcode->flags & F_SCAN)
    {
      if (insn_sequence->instr)
	{
	  mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	  mismatch_detail->error = _("instruction opens new dependency "
				     "sequence without ending previous one");
	  mismatch_detail->index = -1;
	  mismatch_detail->non_fatal = true;
	  res = ERR_VFI;
	}

      init_insn_sequence (inst, insn_sequence);
      return res;
    }

  bool is_new_section = (!encoding && pc == 0);
  if (!verify_mops_pme_sequence (inst, is_new_section, mismatch_detail,
				 insn_sequence))
    {
      res = ERR_VFI;
      if ((opcode->constraints & C_SCAN_MOPS_PME) != C_SCAN_MOPS_M)
	init_insn_sequence (NULL, insn_sequence);
    }

  /* Verify constraints on an existing sequence.  */
  if (insn_sequence->instr)
    {
      const struct aarch64_opcode* inst_opcode = insn_sequence->instr->opcode;
      /* If we're decoding and we hit PC=0 with an open sequence then we haven't
	 closed a previous one that we should have.  */
      if (is_new_section && res == ERR_OK)
	{
	  mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	  mismatch_detail->error = _("previous `movprfx' sequence not closed");
	  mismatch_detail->index = -1;
	  mismatch_detail->non_fatal = true;
	  res = ERR_VFI;
	  /* Reset the sequence.  */
	  init_insn_sequence (NULL, insn_sequence);
	  return res;
	}

      /* Validate C_SCAN_MOVPRFX constraints.  Move this to a lookup table.  */
      if (inst_opcode->constraints & C_SCAN_MOVPRFX)
	{
	  /* Check to see if the MOVPRFX SVE instruction is followed by an SVE
	     instruction for better error messages.  */
	  if (!opcode->avariant
	      || (!AARCH64_CPU_HAS_FEATURE (*opcode->avariant, SVE)
		  && !AARCH64_CPU_HAS_FEATURE (*opcode->avariant, SVE2)))
	    {
	      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	      mismatch_detail->error = _("SVE instruction expected after "
					 "`movprfx'");
	      mismatch_detail->index = -1;
	      mismatch_detail->non_fatal = true;
	      res = ERR_VFI;
	      goto done;
	    }

	  /* Check to see if the MOVPRFX SVE instruction is followed by an SVE
	     instruction that is allowed to be used with a MOVPRFX.  */
	  if (!(opcode->constraints & C_SCAN_MOVPRFX))
	    {
	      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	      mismatch_detail->error = _("SVE `movprfx' compatible instruction "
					 "expected");
	      mismatch_detail->index = -1;
	      mismatch_detail->non_fatal = true;
	      res = ERR_VFI;
	      goto done;
	    }

	  /* Next check for usage of the predicate register.  */
	  aarch64_opnd_info blk_dest = insn_sequence->instr->operands[0];
	  aarch64_opnd_info blk_pred, inst_pred;
	  memset (&blk_pred, 0, sizeof (aarch64_opnd_info));
	  memset (&inst_pred, 0, sizeof (aarch64_opnd_info));
	  bool predicated = false;
	  assert (blk_dest.type == AARCH64_OPND_SVE_Zd);

	  /* Determine if the movprfx instruction used is predicated or not.  */
	  if (insn_sequence->instr->operands[1].type == AARCH64_OPND_SVE_Pg3)
	    {
	      predicated = true;
	      blk_pred = insn_sequence->instr->operands[1];
	    }

	  unsigned char max_elem_size = 0;
	  unsigned char current_elem_size;
	  int num_op_used = 0, last_op_usage = 0;
	  int i, inst_pred_idx = -1;
	  int num_ops = aarch64_num_of_operands (opcode);
	  for (i = 0; i < num_ops; i++)
	    {
	      aarch64_opnd_info inst_op = inst->operands[i];
	      switch (inst_op.type)
		{
		  case AARCH64_OPND_SVE_Zd:
		  case AARCH64_OPND_SVE_Zm_5:
		  case AARCH64_OPND_SVE_Zm_16:
		  case AARCH64_OPND_SVE_Zn:
		  case AARCH64_OPND_SVE_Zt:
		  case AARCH64_OPND_SVE_Vm:
		  case AARCH64_OPND_SVE_Vn:
		  case AARCH64_OPND_Va:
		  case AARCH64_OPND_Vn:
		  case AARCH64_OPND_Vm:
		  case AARCH64_OPND_Sn:
		  case AARCH64_OPND_Sm:
		    if (inst_op.reg.regno == blk_dest.reg.regno)
		      {
			num_op_used++;
			last_op_usage = i;
		      }
		    current_elem_size
		      = aarch64_get_qualifier_esize (inst_op.qualifier);
		    if (current_elem_size > max_elem_size)
		      max_elem_size = current_elem_size;
		    break;
		  case AARCH64_OPND_SVE_Pd:
		  case AARCH64_OPND_SVE_Pg3:
		  case AARCH64_OPND_SVE_Pg4_5:
		  case AARCH64_OPND_SVE_Pg4_10:
		  case AARCH64_OPND_SVE_Pg4_16:
		  case AARCH64_OPND_SVE_Pm:
		  case AARCH64_OPND_SVE_Pn:
		  case AARCH64_OPND_SVE_Pt:
		  case AARCH64_OPND_SME_Pm:
		    inst_pred = inst_op;
		    inst_pred_idx = i;
		    break;
		  default:
		    break;
		}
	    }

	   assert (max_elem_size != 0);
	   aarch64_opnd_info inst_dest = inst->operands[0];
	   /* Determine the size that should be used to compare against the
	      movprfx size.  */
	   current_elem_size
	     = opcode->constraints & C_MAX_ELEM
	       ? max_elem_size
	       : aarch64_get_qualifier_esize (inst_dest.qualifier);

	  /* If movprfx is predicated do some extra checks.  */
	  if (predicated)
	    {
	      /* The instruction must be predicated.  */
	      if (inst_pred_idx < 0)
		{
		  mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
		  mismatch_detail->error = _("predicated instruction expected "
					     "after `movprfx'");
		  mismatch_detail->index = -1;
		  mismatch_detail->non_fatal = true;
		  res = ERR_VFI;
		  goto done;
		}

	      /* The instruction must have a merging predicate.  */
	      if (inst_pred.qualifier != AARCH64_OPND_QLF_P_M)
		{
		  mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
		  mismatch_detail->error = _("merging predicate expected due "
					     "to preceding `movprfx'");
		  mismatch_detail->index = inst_pred_idx;
		  mismatch_detail->non_fatal = true;
		  res = ERR_VFI;
		  goto done;
		}

	      /* The same register must be used in instruction.  */
	      if (blk_pred.reg.regno != inst_pred.reg.regno)
		{
		  mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
		  mismatch_detail->error = _("predicate register differs "
					     "from that in preceding "
					     "`movprfx'");
		  mismatch_detail->index = inst_pred_idx;
		  mismatch_detail->non_fatal = true;
		  res = ERR_VFI;
		  goto done;
		}
	    }

	  /* Destructive operations by definition must allow one usage of the
	     same register.  */
	  int allowed_usage
	    = aarch64_is_destructive_by_operands (opcode) ? 2 : 1;

	  /* Operand is not used at all.  */
	  if (num_op_used == 0)
	    {
	      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	      mismatch_detail->error = _("output register of preceding "
					 "`movprfx' not used in current "
					 "instruction");
	      mismatch_detail->index = 0;
	      mismatch_detail->non_fatal = true;
	      res = ERR_VFI;
	      goto done;
	    }

	  /* We now know it's used, now determine exactly where it's used.  */
	  if (blk_dest.reg.regno != inst_dest.reg.regno)
	    {
	      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	      mismatch_detail->error = _("output register of preceding "
					 "`movprfx' expected as output");
	      mismatch_detail->index = 0;
	      mismatch_detail->non_fatal = true;
	      res = ERR_VFI;
	      goto done;
	    }

	  /* Operand used more than allowed for the specific opcode type.  */
	  if (num_op_used > allowed_usage)
	    {
	      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	      mismatch_detail->error = _("output register of preceding "
					 "`movprfx' used as input");
	      mismatch_detail->index = last_op_usage;
	      mismatch_detail->non_fatal = true;
	      res = ERR_VFI;
	      goto done;
	    }

	  /* Now the only thing left is the qualifiers checks.  The register
	     must have the same maximum element size.  */
	  if (inst_dest.qualifier
	      && blk_dest.qualifier
	      && current_elem_size
		 != aarch64_get_qualifier_esize (blk_dest.qualifier))
	    {
	      mismatch_detail->kind = AARCH64_OPDE_SYNTAX_ERROR;
	      mismatch_detail->error = _("register size not compatible with "
					 "previous `movprfx'");
	      mismatch_detail->index = 0;
	      mismatch_detail->non_fatal = true;
	      res = ERR_VFI;
	      goto done;
	    }
	}

    done:
      if (insn_sequence->num_added_insns == insn_sequence->num_allocated_insns)
	/* We've checked the last instruction in the sequence and so
	   don't need the sequence any more.  */
	init_insn_sequence (NULL, insn_sequence);
      else
	add_insn_to_sequence (inst, insn_sequence);
    }

  return res;
}


/* Return true if VALUE cannot be moved into an SVE register using DUP
   (with any element size, not just ESIZE) and if using DUPM would
   therefore be OK.  ESIZE is the number of bytes in the immediate.  */

bool
aarch64_sve_dupm_mov_immediate_p (uint64_t uvalue, int esize)
{
  int64_t svalue = uvalue;
  uint64_t upper = (uint64_t) -1 << (esize * 4) << (esize * 4);

  if ((uvalue & ~upper) != uvalue && (uvalue | upper) != uvalue)
    return false;
  if (esize <= 4 || (uint32_t) uvalue == (uint32_t) (uvalue >> 32))
    {
      svalue = (int32_t) uvalue;
      if (esize <= 2 || (uint16_t) uvalue == (uint16_t) (uvalue >> 16))
	{
	  svalue = (int16_t) uvalue;
	  if (esize == 1 || (uint8_t) uvalue == (uint8_t) (uvalue >> 8))
	    return false;
	}
    }
  if ((svalue & 0xff) == 0)
    svalue /= 256;
  return svalue < -128 || svalue >= 128;
}

/* Return true if a CPU with the AARCH64_FEATURE_* bits in CPU_VARIANT
   supports the instruction described by INST.  */

bool
aarch64_cpu_supports_inst_p (aarch64_feature_set cpu_variant,
			     aarch64_inst *inst)
{
  if (!inst->opcode->avariant
      || !AARCH64_CPU_HAS_ALL_FEATURES (cpu_variant, *inst->opcode->avariant))
    return false;

  if (inst->opcode->iclass == sme_fp_sd
      && inst->operands[0].qualifier == AARCH64_OPND_QLF_S_D
      && !AARCH64_CPU_HAS_FEATURE (cpu_variant, SME_F64F64))
    return false;

  if (inst->opcode->iclass == sme_int_sd
      && inst->operands[0].qualifier == AARCH64_OPND_QLF_S_D
      && !AARCH64_CPU_HAS_FEATURE (cpu_variant, SME_I16I64))
    return false;

  return true;
}

/* Include the opcode description table as well as the operand description
   table.  */
#define VERIFIER(x) verify_##x
#include "aarch64-tbl.h"
