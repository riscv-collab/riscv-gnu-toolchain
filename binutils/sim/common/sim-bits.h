/* The common simulator framework for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney and Red Hat.

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


#ifndef SIM_BITS_H
#define SIM_BITS_H


/* Bit manipulation routines:

   Bit numbering: The bits are numbered according to the target ISA's
   convention.  That being controlled by WITH_TARGET_WORD_MSB.  For
   the PowerPC (WITH_TARGET_WORD_MSB == 0) the numbering is 0..31
   while for the MIPS (WITH_TARGET_WORD_MSB == 31) it is 31..0.

   Size convention: Each macro is in three forms - <MACRO>32 which
   operates in 32bit quantity (bits are numbered 0..31); <MACRO>64
   which operates using 64bit quantites (and bits are numbered 0..63);
   and <MACRO> which operates using the bit size of the target
   architecture (bits are still numbered 0..63), with 32bit
   architectures ignoring the first 32bits leaving bit 32 as the most
   significant.

   NB: Use EXTRACTED, MSEXTRACTED and LSEXTRACTED as a guideline for
   naming.  LSMASK and LSMASKED are wrong.

   BIT*(POS): `*' bit constant with just 1 bit set.

   LSBIT*(OFFSET): `*' bit constant with just 1 bit set - LS bit is
   zero.

   MSBIT*(OFFSET): `*' bit constant with just 1 bit set - MS bit is
   zero.

   MASK*(FIRST, LAST): `*' bit constant with bits [FIRST .. LAST]
   set. The <MACRO> (no size) version permits FIRST >= LAST and
   generates a wrapped bit mask vis ([0..LAST] | [FIRST..LSB]).

   LSMASK*(FIRST, LAST): Like MASK - LS bit is zero.

   MSMASK*(FIRST, LAST): Like MASK - LS bit is zero.

   MASKED*(VALUE, FIRST, LAST): Masks out all but bits [FIRST
   .. LAST].

   LSMASKED*(VALUE, FIRST, LAST): Like MASKED - LS bit is zero.

   MSMASKED*(VALUE, FIRST, LAST): Like MASKED - MS bit is zero.

   EXTRACTED*(VALUE, FIRST, LAST): Masks out bits [FIRST .. LAST] but
   also right shifts the masked value so that bit LAST becomes the
   least significant (right most).

   LSEXTRACTED*(VALUE, FIRST, LAST): Same as extracted - LS bit is
   zero.

   MSEXTRACTED*(VALUE, FIRST, LAST): Same as extracted - MS bit is
   zero.

   SHUFFLED**(VALUE, OLD, NEW): Mask then move a single bit from OLD
   new NEW.

   MOVED**(VALUE, OLD_FIRST, OLD_LAST, NEW_FIRST, NEW_LAST): Moves
   things around so that bits OLD_FIRST..OLD_LAST are masked then
   moved to NEW_FIRST..NEW_LAST.

   INSERTED*(VALUE, FIRST, LAST): Takes VALUE and `inserts' the (LAST
   - FIRST + 1) least significant bits into bit positions [ FIRST
   .. LAST ].  This is almost the complement to EXTRACTED.

   IEA_MASKED(SHOULD_MASK, ADDR): Convert the address to the targets
   natural size.  If in 32bit mode, discard the high 32bits.

   EXTEND*(VALUE): Convert the `*' bit value to the targets natural
   word size.  Sign extend the value if needed.

   align_*(VALUE, BYTES): Round the value so that it is aligned to a
   BYTES boundary.

   ROT*(VALUE, NR_BITS): Return the `*' bit VALUE rotated by NR_BITS
   right (positive) or left (negative).

   ROTL*(VALUE, NR_BITS): Return the `*' bit value rotated by NR_BITS
   left.  0 <= NR_BITS <= `*'.

   ROTR*(VALUE, NR_BITS): Return the `*' bit value rotated by NR_BITS
   right.  0 <= NR_BITS <= N.

   SEXT*(VALUE, SIGN_BIT): Treat SIGN_BIT as VALUEs sign, extend it ti
   `*' bits.

   Note: Only the BIT* and MASK* macros return a constant that can be
   used in variable declarations.

   */


/* compute the number of bits between START and STOP */

#if (WITH_TARGET_WORD_MSB == 0)
#define _MAKE_WIDTH(START, STOP) (STOP - START + 1)
#else
#define _MAKE_WIDTH(START, STOP) (START - STOP + 1)
#endif



/* compute the number shifts required to move a bit between LSB (MSB)
   and POS */

#if (WITH_TARGET_WORD_MSB == 0)
#define _LSB_SHIFT(WIDTH, POS) (WIDTH - 1 - POS)
#else
#define _LSB_SHIFT(WIDTH, POS) (POS)
#endif

#if (WITH_TARGET_WORD_MSB == 0)
#define _MSB_SHIFT(WIDTH, POS) (POS)
#else
#define _MSB_SHIFT(WIDTH, POS) (WIDTH - 1 - POS)
#endif


/* compute the absolute bit position given the OFFSET from the MSB(LSB)
   NB: _MAKE_xxx_POS (WIDTH, _MAKE_xxx_SHIFT (WIDTH, POS)) == POS */

#if (WITH_TARGET_WORD_MSB == 0)
#define _MSB_POS(WIDTH, SHIFT) (SHIFT)
#else
#define _MSB_POS(WIDTH, SHIFT) (WIDTH - 1 - SHIFT)
#endif

#if (WITH_TARGET_WORD_MSB == 0)
#define _LSB_POS(WIDTH, SHIFT) (WIDTH - 1 - SHIFT)
#else
#define _LSB_POS(WIDTH, SHIFT) (SHIFT)
#endif


/* convert a 64 bit position into a corresponding 32bit position. MSB
   pos handles the posibility that the bit lies beyond the 32bit
   boundary */

#if (WITH_TARGET_WORD_MSB == 0)
#define _MSB_32(START, STOP) (START <= STOP \
			      ? (START < 32 ? 0 : START - 32) \
			      : (STOP < 32 ? 0 : STOP - 32))
#define _MSB_16(START, STOP) (START <= STOP \
			      ? (START < 48 ? 0 : START - 48) \
			      : (STOP < 48 ? 0 : STOP - 48))
#else
#define _MSB_32(START, STOP) (START >= STOP \
			      ? (START >= 32 ? 31 : START) \
			      : (STOP >= 32 ? 31 : STOP))
#define _MSB_16(START, STOP) (START >= STOP \
			      ? (START >= 16 ? 15 : START) \
			      : (STOP >= 16 ? 15 : STOP))
#endif

#if (WITH_TARGET_WORD_MSB == 0)
#define _LSB_32(START, STOP) (START <= STOP \
			      ? (STOP < 32 ? 0 : STOP - 32) \
			      : (START < 32 ? 0 : START - 32))
#define _LSB_16(START, STOP) (START <= STOP \
			      ? (STOP < 48 ? 0 : STOP - 48) \
			      : (START < 48 ? 0 : START - 48))
#else
#define _LSB_32(START, STOP) (START >= STOP \
			      ? (STOP >= 32 ? 31 : STOP) \
			      : (START >= 32 ? 31 : START))
#define _LSB_16(START, STOP) (START >= STOP \
			      ? (STOP >= 16 ? 15 : STOP) \
			      : (START >= 16 ? 15 : START))
#endif

#if (WITH_TARGET_WORD_MSB == 0)
#define _MSB(START, STOP) (START <= STOP ? START : STOP)
#else
#define _MSB(START, STOP) (START >= STOP ? START : STOP)
#endif

#if (WITH_TARGET_WORD_MSB == 0)
#define _LSB(START, STOP) (START <= STOP ? STOP : START)
#else
#define _LSB(START, STOP) (START >= STOP ? STOP : START)
#endif


/* LS/MS Bit operations */

#define LSBIT8(POS)  ((uint8_t) 1 << (POS))
#define LSBIT16(POS) ((uint16_t)1 << (POS))
#define LSBIT32(POS) ((uint32_t)1 << (POS))
#define LSBIT64(POS) ((uint64_t)1 << (POS))

#if (WITH_TARGET_WORD_BITSIZE == 64)
#define LSBIT(POS) LSBIT64 (POS)
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
#define LSBIT(POS) ((uint32_t)((POS) >= 32 \
		                 ? 0 \
			         : (1 << ((POS) >= 32 ? 0 : (POS)))))
#endif
#if (WITH_TARGET_WORD_BITSIZE == 16)
#define LSBIT(POS) ((uint16_t)((POS) >= 16 \
		                 ? 0 \
			         : (1 << ((POS) >= 16 ? 0 : (POS)))))
#endif


#define MSBIT8(POS)  ((uint8_t) 1 << ( 8 - 1 - (POS)))
#define MSBIT16(POS) ((uint16_t)1 << (16 - 1 - (POS)))
#define MSBIT32(POS) ((uint32_t)1 << (32 - 1 - (POS)))
#define MSBIT64(POS) ((uint64_t)1 << (64 - 1 - (POS)))

#if (WITH_TARGET_WORD_BITSIZE == 64)
#define MSBIT(POS) MSBIT64 (POS)
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
#define MSBIT(POS) ((uint32_t)((POS) < 32 \
		                 ? 0 \
		                 : (1 << ((POS) < 32 ? 0 : (64 - 1) - (POS)))))
#endif
#if (WITH_TARGET_WORD_BITSIZE == 16)
#define MSBIT(POS) ((uint16_t)((POS) < 48 \
		                 ? 0 \
		                 : (1 << ((POS) < 48 ? 0 : (64 - 1) - (POS)))))
#endif


/* Bit operations */

#define BIT4(POS)  (1 << _LSB_SHIFT (4, (POS)))
#define BIT5(POS)  (1 << _LSB_SHIFT (5, (POS)))
#define BIT10(POS) (1 << _LSB_SHIFT (10, (POS)))

#if (WITH_TARGET_WORD_MSB == 0)
#define BIT8  MSBIT8
#define BIT16 MSBIT16
#define BIT32 MSBIT32
#define BIT64 MSBIT64
#define BIT   MSBIT
#else
#define BIT8  LSBIT8
#define BIT16 LSBIT16
#define BIT32 LSBIT32
#define BIT64 LSBIT64
#define BIT   LSBIT
#endif



/* multi bit mask */

/* 111111 -> mmll11 -> mm11ll */
#define _MASKn(WIDTH, START, STOP) (((uint##WIDTH##_t)(-1) \
				     >> (_MSB_SHIFT (WIDTH, START) \
					 + _LSB_SHIFT (WIDTH, STOP))) \
				    << _LSB_SHIFT (WIDTH, STOP))

#if (WITH_TARGET_WORD_MSB == 0)
#define _POS_LE(START, STOP) (START <= STOP)
#else
#define _POS_LE(START, STOP) (STOP <= START)
#endif

#if (WITH_TARGET_WORD_BITSIZE == 64)
#define MASK(START, STOP) \
     (_POS_LE ((START), (STOP)) \
      ? _MASKn(64, \
	       _MSB ((START), (STOP)), \
	       _LSB ((START), (STOP)) ) \
      : (_MASKn(64, _MSB_POS (64, 0), (STOP)) \
	 | _MASKn(64, (START), _LSB_POS (64, 0))))
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
#define MASK(START, STOP) \
     (_POS_LE ((START), (STOP)) \
      ? (_POS_LE ((STOP), _MSB_POS (64, 31)) \
	 ? 0 \
	 : _MASKn (32, \
		   _MSB_32 ((START), (STOP)), \
		   _LSB_32 ((START), (STOP)))) \
      : (_MASKn (32, \
		 _LSB_32 ((START), (STOP)), \
		 _LSB_POS (32, 0)) \
	 | (_POS_LE ((STOP), _MSB_POS (64, 31)) \
	    ? 0 \
	    : _MASKn (32, \
		      _MSB_POS (32, 0), \
		      _MSB_32 ((START), (STOP))))))
#endif
#if (WITH_TARGET_WORD_BITSIZE == 16)
#define MASK(START, STOP) \
     (_POS_LE ((START), (STOP)) \
      ? (_POS_LE ((STOP), _MSB_POS (64, 15)) \
	 ? 0 \
	 : _MASKn (16, \
		   _MSB_16 ((START), (STOP)), \
		   _LSB_16 ((START), (STOP)))) \
      : (_MASKn (16, \
		 _LSB_16 ((START), (STOP)), \
		 _LSB_POS (16, 0)) \
	 | (_POS_LE ((STOP), _MSB_POS (64, 15)) \
	    ? 0 \
	    : _MASKn (16, \
		      _MSB_POS (16, 0), \
		      _MSB_16 ((START), (STOP))))))
#endif
#if !defined (MASK)
#error "MASK never undefined"
#endif


/* Multi-bit mask on least significant bits */

#define _LSMASKn(WIDTH, FIRST, LAST) _MASKn (WIDTH, \
					     _LSB_POS (WIDTH, FIRST), \
					     _LSB_POS (WIDTH, LAST))

#define LSMASK8(FIRST, LAST)   _LSMASKn ( 8, (FIRST), (LAST))
#define LSMASK16(FIRST, LAST)  _LSMASKn (16, (FIRST), (LAST))
#define LSMASK32(FIRST, LAST)  _LSMASKn (32, (FIRST), (LAST))
#define LSMASK64(FIRST, LAST)  _LSMASKn (64, (FIRST), (LAST))

#define LSMASK(FIRST, LAST) (MASK (_LSB_POS (64, FIRST), _LSB_POS (64, LAST)))


/* Multi-bit mask on most significant bits */

#define _MSMASKn(WIDTH, FIRST, LAST) _MASKn (WIDTH, \
					     _MSB_POS (WIDTH, FIRST), \
					     _MSB_POS (WIDTH, LAST))

#define MSMASK8(FIRST, LAST)  _MSMASKn ( 8, (FIRST), (LAST))
#define MSMASK16(FIRST, LAST) _MSMASKn (16, (FIRST), (LAST))
#define MSMASK32(FIRST, LAST) _MSMASKn (32, (FIRST), (LAST))
#define MSMASK64(FIRST, LAST) _MSMASKn (64, (FIRST), (LAST))

#define MSMASK(FIRST, LAST) (MASK (_MSB_POS (64, FIRST), _MSB_POS (64, LAST)))



#if (WITH_TARGET_WORD_MSB == 0)
#define MASK8  MSMASK8
#define MASK16 MSMASK16
#define MASK32 MSMASK32
#define MASK64 MSMASK64
#else
#define MASK8  LSMASK8
#define MASK16 LSMASK16
#define MASK32 LSMASK32
#define MASK64 LSMASK64
#endif



/* mask the required bits, leaving them in place */

INLINE_SIM_BITS(uint8_t)  LSMASKED8  (uint8_t  word, int first, int last);
INLINE_SIM_BITS(uint16_t) LSMASKED16 (uint16_t word, int first, int last);
INLINE_SIM_BITS(uint32_t) LSMASKED32 (uint32_t word, int first, int last);
INLINE_SIM_BITS(uint64_t) LSMASKED64 (uint64_t word, int first, int last);

INLINE_SIM_BITS(unsigned_word) LSMASKED (unsigned_word word, int first, int last);

INLINE_SIM_BITS(uint8_t)  MSMASKED8  (uint8_t  word, int first, int last);
INLINE_SIM_BITS(uint16_t) MSMASKED16 (uint16_t word, int first, int last);
INLINE_SIM_BITS(uint32_t) MSMASKED32 (uint32_t word, int first, int last);
INLINE_SIM_BITS(uint64_t) MSMASKED64 (uint64_t word, int first, int last);

INLINE_SIM_BITS(unsigned_word) MSMASKED (unsigned_word word, int first, int last);

#if (WITH_TARGET_WORD_MSB == 0)
#define MASKED8  MSMASKED8
#define MASKED16 MSMASKED16
#define MASKED32 MSMASKED32
#define MASKED64 MSMASKED64
#define MASKED   MSMASKED
#else
#define MASKED8  LSMASKED8
#define MASKED16 LSMASKED16
#define MASKED32 LSMASKED32
#define MASKED64 LSMASKED64
#define MASKED LSMASKED
#endif



/* extract the required bits aligning them with the lsb */

INLINE_SIM_BITS(uint8_t)  LSEXTRACTED8  (uint8_t  val, int start, int stop);
INLINE_SIM_BITS(uint16_t) LSEXTRACTED16 (uint16_t val, int start, int stop);
INLINE_SIM_BITS(uint32_t) LSEXTRACTED32 (uint32_t val, int start, int stop);
INLINE_SIM_BITS(uint64_t) LSEXTRACTED64 (uint64_t val, int start, int stop);

INLINE_SIM_BITS(unsigned_word) LSEXTRACTED (unsigned_word val, int start, int stop);

INLINE_SIM_BITS(uint8_t)  MSEXTRACTED8  (uint8_t  val, int start, int stop);
INLINE_SIM_BITS(uint16_t) MSEXTRACTED16 (uint16_t val, int start, int stop);
INLINE_SIM_BITS(uint32_t) MSEXTRACTED32 (uint32_t val, int start, int stop);
INLINE_SIM_BITS(uint64_t) MSEXTRACTED64 (uint64_t val, int start, int stop);

INLINE_SIM_BITS(unsigned_word) MSEXTRACTED (unsigned_word val, int start, int stop);

#if (WITH_TARGET_WORD_MSB == 0)
#define EXTRACTED8  MSEXTRACTED8
#define EXTRACTED16 MSEXTRACTED16
#define EXTRACTED32 MSEXTRACTED32
#define EXTRACTED64 MSEXTRACTED64
#define EXTRACTED   MSEXTRACTED
#else
#define EXTRACTED8  LSEXTRACTED8
#define EXTRACTED16 LSEXTRACTED16
#define EXTRACTED32 LSEXTRACTED32
#define EXTRACTED64 LSEXTRACTED64
#define EXTRACTED   LSEXTRACTED
#endif



/* move a single bit around */
/* NB: the wierdness (N>O?N-O:0) is to stop a warning from GCC */
#define _SHUFFLEDn(N, WORD, OLD, NEW) \
((OLD) < (NEW) \
 ? (((uint##N##_t)(WORD) \
     >> (((NEW) > (OLD)) ? ((NEW) - (OLD)) : 0)) \
    & MASK32((NEW), (NEW))) \
 : (((uint##N##_t)(WORD) \
     << (((OLD) > (NEW)) ? ((OLD) - (NEW)) : 0)) \
    & MASK32((NEW), (NEW))))

#define SHUFFLED32(WORD, OLD, NEW) _SHUFFLEDn (32, WORD, OLD, NEW)
#define SHUFFLED64(WORD, OLD, NEW) _SHUFFLEDn (64, WORD, OLD, NEW)

#define SHUFFLED(WORD, OLD, NEW) _SHUFFLEDn (_word, WORD, OLD, NEW)


/* Insert a group of bits into a bit position */

INLINE_SIM_BITS(uint8_t)  LSINSERTED8  (uint8_t  val, int start, int stop);
INLINE_SIM_BITS(uint16_t) LSINSERTED16 (uint16_t val, int start, int stop);
INLINE_SIM_BITS(uint32_t) LSINSERTED32 (uint32_t val, int start, int stop);
INLINE_SIM_BITS(uint64_t) LSINSERTED64 (uint64_t val, int start, int stop);
INLINE_SIM_BITS(unsigned_word) LSINSERTED (unsigned_word val, int start, int stop);

INLINE_SIM_BITS(uint8_t)  MSINSERTED8  (uint8_t  val, int start, int stop);
INLINE_SIM_BITS(uint16_t) MSINSERTED16 (uint16_t val, int start, int stop);
INLINE_SIM_BITS(uint32_t) MSINSERTED32 (uint32_t val, int start, int stop);
INLINE_SIM_BITS(uint64_t) MSINSERTED64 (uint64_t val, int start, int stop);
INLINE_SIM_BITS(unsigned_word) MSINSERTED (unsigned_word val, int start, int stop);

#if (WITH_TARGET_WORD_MSB == 0)
#define INSERTED8  MSINSERTED8
#define INSERTED16 MSINSERTED16
#define INSERTED32 MSINSERTED32
#define INSERTED64 MSINSERTED64
#define INSERTED   MSINSERTED
#else
#define INSERTED8  LSINSERTED8
#define INSERTED16 LSINSERTED16
#define INSERTED32 LSINSERTED32
#define INSERTED64 LSINSERTED64
#define INSERTED   LSINSERTED
#endif



/* MOVE bits from one loc to another (combination of extract/insert) */

#define MOVED8(VAL,OH,OL,NH,NL)  INSERTED8 (EXTRACTED8 ((VAL), OH, OL), NH, NL)
#define MOVED16(VAL,OH,OL,NH,NL) INSERTED16(EXTRACTED16((VAL), OH, OL), NH, NL)
#define MOVED32(VAL,OH,OL,NH,NL) INSERTED32(EXTRACTED32((VAL), OH, OL), NH, NL)
#define MOVED64(VAL,OH,OL,NH,NL) INSERTED64(EXTRACTED64((VAL), OH, OL), NH, NL)
#define MOVED(VAL,OH,OL,NH,NL)   INSERTED  (EXTRACTED  ((VAL), OH, OL), NH, NL)



/* Sign extend the quantity to the targets natural word size */

#define EXTEND4(X)  (LSSEXT ((X), 3))
#define EXTEND5(X)  (LSSEXT ((X), 4))
#define EXTEND6(X)  (LSSEXT ((X), 5))
#define EXTEND8(X)  ((signed_word)(int8_t)(X))
#define EXTEND9(X)  (LSSEXT ((X), 8))
#define EXTEND11(X)  (LSSEXT ((X), 10))
#define EXTEND12(X)  (LSSEXT ((X), 11))
#define EXTEND15(X)  (LSSEXT ((X), 14))
#define EXTEND16(X) ((signed_word)(int16_t)(X))
#define EXTEND18(X)  (LSSEXT ((X), 17))
#define EXTEND19(X)  (LSSEXT ((X), 18))
#define EXTEND21(X)  (LSSEXT ((X), 20))
#define EXTEND24(X)  (LSSEXT ((X), 23))
#define EXTEND25(X)  (LSSEXT ((X), 24))
#define EXTEND26(X)  (LSSEXT ((X), 25))
#define EXTEND32(X) ((signed_word)(int32_t)(X))
#define EXTEND64(X) ((signed_word)(int64_t)(X))

/* depending on MODE return a 64bit or 32bit (sign extended) value */
#if (WITH_TARGET_WORD_BITSIZE == 64)
#define EXTENDED(X)     ((int64_t)(int32_t)(X))
#endif
#if (WITH_TARGET_WORD_BITSIZE == 32)
#define EXTENDED(X)     (X)
#endif
#if (WITH_TARGET_WORD_BITSIZE == 16)
#define EXTENDED(X)     (X)
#endif


/* memory alignment macro's */
#define align_up(v, n)		(((v) + (n) - 1) & -(n))
#define align_down(v, n)	((v) & -(n))


/* bit bliting macro's */
#define BLIT32(V, POS, BIT) \
do { \
  if (BIT) \
    V |= BIT32 (POS); \
  else \
    V &= ~BIT32 (POS); \
} while (0)
#define MBLIT32(V, LO, HI, VAL) \
do { \
  (V) = (((V) & ~MASK32 ((LO), (HI))) \
	 | INSERTED32 (VAL, LO, HI)); \
} while (0)



/* some rotate functions.  The generic macro's ROT, ROTL, ROTR are
   intentionally omited. */


INLINE_SIM_BITS(uint8_t)  ROT8  (uint8_t  val, int shift);
INLINE_SIM_BITS(uint16_t) ROT16 (uint16_t val, int shift);
INLINE_SIM_BITS(uint32_t) ROT32 (uint32_t val, int shift);
INLINE_SIM_BITS(uint64_t) ROT64 (uint64_t val, int shift);


INLINE_SIM_BITS(uint8_t)  ROTL8  (uint8_t  val, int shift);
INLINE_SIM_BITS(uint16_t) ROTL16 (uint16_t val, int shift);
INLINE_SIM_BITS(uint32_t) ROTL32 (uint32_t val, int shift);
INLINE_SIM_BITS(uint64_t) ROTL64 (uint64_t val, int shift);


INLINE_SIM_BITS(uint8_t)  ROTR8  (uint8_t  val, int shift);
INLINE_SIM_BITS(uint16_t) ROTR16 (uint16_t val, int shift);
INLINE_SIM_BITS(uint32_t) ROTR32 (uint32_t val, int shift);
INLINE_SIM_BITS(uint64_t) ROTR64 (uint64_t val, int shift);



/* Sign extension operations */

INLINE_SIM_BITS(uint8_t)  LSSEXT8  (int8_t  val, int sign_bit);
INLINE_SIM_BITS(uint16_t) LSSEXT16 (int16_t val, int sign_bit);
INLINE_SIM_BITS(uint32_t) LSSEXT32 (int32_t val, int sign_bit);
INLINE_SIM_BITS(uint64_t) LSSEXT64 (int64_t val, int sign_bit);
INLINE_SIM_BITS(unsigned_word) LSSEXT (signed_word val, int sign_bit);

INLINE_SIM_BITS(uint8_t)  MSSEXT8  (int8_t  val, int sign_bit);
INLINE_SIM_BITS(uint16_t) MSSEXT16 (int16_t val, int sign_bit);
INLINE_SIM_BITS(uint32_t) MSSEXT32 (int32_t val, int sign_bit);
INLINE_SIM_BITS(uint64_t) MSSEXT64 (int64_t val, int sign_bit);
INLINE_SIM_BITS(unsigned_word) MSSEXT (signed_word val, int sign_bit);

#if (WITH_TARGET_WORD_MSB == 0)
#define SEXT8  MSSEXT8
#define SEXT16 MSSEXT16
#define SEXT32 MSSEXT32
#define SEXT64 MSSEXT64
#define SEXT   MSSEXT
#else
#define SEXT8  LSSEXT8
#define SEXT16 LSSEXT16
#define SEXT32 LSSEXT32
#define SEXT64 LSSEXT64
#define SEXT   LSSEXT
#endif



#if H_REVEALS_MODULE_P (SIM_BITS_INLINE)
#include "sim-bits.c"
#endif

#endif /* SIM_BITS_H */
