# Copyright (C) 2014-2024 Free Software Foundation, Inc.
#
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.

# RODATA_PM_OFFSET
#         If empty and not HAVE_FLMAP, .rodata sections will be part of .data.
#         This is for devices where it is not possible to use LD* instructions
#         to read from flash.
#
#         If non-empty, .rodata is not part of .data and the .rodata
#         objects are assigned addresses at an offest of RODATA_PM_OFFSET.
#         This is for devices that feature reading from flash by means of
#         LD* instructions, provided the addresses are offset by
#         __RODATA_PM_OFFSET__ (which defaults to RODATA_PM_OFFSET).

# HAVE_FLMAP
#         The .rodata section is located in program memory. Devices from
#         the AVR64* and AVR128* families (from avrxmega2 and avrxmega4)
#         see a 32k segment of their program memory in their RAM address
#         space.  Which 32k segment is visible is determined by the
#         bit-field NVMCTRL_CTRLB.FLMAP.
#         Output section .rodata is placed in MEMORY region rodata.
#         The LMA of the .rodata section can be set by means of:
#         * __RODATA_FLASH_START__ specifies the byte address of the
#           rodata LMA.
#         * __flmap specifies which 32k block is visible in RAM provided
#           __RODATA_FLASH_START__ is undefined
#         * When __flmap and __RODATA_FLASH_START__ are undefined, then an
#           emulation-specific default is used (the last 32k block).

# MAYBE_FLMAP
#         For devices from avrxmega2 and avrxmega4: The user can chose whether
#         or not .rodata is located in flash (if HAVE_FLMAP) or located in
#         in RAM (if not HAVE_FLMAP by means of -mrodata-in-ram).  This is
#         achieved by new emulations avrxmega2_flmap and avrxmega4_flmap that
#         are selected by compiler option -mno-rodata-in-ram.
#
#         In order to facilitate initialization of NVMCTRL_CTRLB.FLMAP and
#         NVMCTRL_CTRLB.FLMAPLOCK in the startup code irrespective of
#         HAVE_FLMAP, the following symbols are used / defined in order to
#         communicate with the startup code.
#         Notice that the hardware default for FLMAP is the last 32k block,
#         so that explicit initialization of FLMAP is only required when the
#         user wants to deviate from the defaults.
#
#         __flmap = HAVE_FLMAP
#                   ? given by __flmap resp. __RODATA_FLASH_START__ >> 15
#                   : 0;
#
#         __flmap_value = __flmap << __flmap_bpos;
#
#         __flmap_value_with_lock = __flmap__value | __flmap_lock_mask;
#
#         __flmap_init_label = HAVE_FLMAP
#                              ? __flmap_init_start
#                              : __flmap_noinit_start;
#             Supposed to be used as a jump target for RJMP so that the code
#             can initialize FLMAP / skip initialization of FLMAP depending
#             on the chosen emulation, and without the need to support two code
#             versions of crt<mcu>.o for the two possible emulations.
#
#         __flmap_lock is a bool provided by the user when FLMAP should be
#             protected from any further changes.
#
#         __flmap_lock_mask is an 8-bit mask like NVMCTRL_FLMAPLOCK_bm
#             provided by the user which is set in __flmap_value_with_lock
#             when __flmap_lock is on.
#
#         __do_init_flmap = HAVE_FLMAP ? 1 : 0;
#             Whether or not FLMAP is supposed to be initialized according
#             to, and for the purpose of, .rodata in flash.
#
#         Apart from that, the compiler (device-specs actually) defines the
#         following macros:
#
#         __AVR_HAVE_FLMAP__
#             Defined if a device has the NVMCTRL_CTRLB.FLMAP bitfield
#             *AND* if it's unknown at compile-time / assembler-time whether
#             emulation avrxmega* is used or avrxmega*_flmap.

cat <<EOF
/* Copyright (C) 2014-2024 Free Software Foundation, Inc.

   Copying and distribution of this script, with or without modification,
   are permitted in any medium without royalty provided the copyright
   notice and this notice are preserved.  */

OUTPUT_FORMAT("${OUTPUT_FORMAT}","${OUTPUT_FORMAT}","${OUTPUT_FORMAT}")
OUTPUT_ARCH(${ARCH})
EOF

test -n "${RELOCATING}" && cat <<EOF
__TEXT_REGION_ORIGIN__ = DEFINED(__TEXT_REGION_ORIGIN__) ? __TEXT_REGION_ORIGIN__ : 0;
__TEXT_REGION_LENGTH__ = DEFINED(__TEXT_REGION_LENGTH__) ? __TEXT_REGION_LENGTH__ : $TEXT_LENGTH;
__DATA_REGION_ORIGIN__ = DEFINED(__DATA_REGION_ORIGIN__) ? __DATA_REGION_ORIGIN__ : $DATA_ORIGIN;
__DATA_REGION_LENGTH__ = DEFINED(__DATA_REGION_LENGTH__) ? __DATA_REGION_LENGTH__ : $DATA_LENGTH;

${EEPROM_LENGTH+__EEPROM_REGION_LENGTH__ = DEFINED(__EEPROM_REGION_LENGTH__) ? __EEPROM_REGION_LENGTH__ : $EEPROM_LENGTH;}
__FUSE_REGION_LENGTH__ = DEFINED(__FUSE_REGION_LENGTH__) ? __FUSE_REGION_LENGTH__ : $FUSE_LENGTH;
__LOCK_REGION_LENGTH__ = DEFINED(__LOCK_REGION_LENGTH__) ? __LOCK_REGION_LENGTH__ : $LOCK_LENGTH;
__SIGNATURE_REGION_LENGTH__ = DEFINED(__SIGNATURE_REGION_LENGTH__) ? __SIGNATURE_REGION_LENGTH__ : $SIGNATURE_LENGTH;
${USER_SIGNATURE_LENGTH+__USER_SIGNATURE_REGION_LENGTH__ = DEFINED(__USER_SIGNATURE_REGION_LENGTH__) ? __USER_SIGNATURE_REGION_LENGTH__ : $USER_SIGNATURE_LENGTH;}
${RODATA_PM_OFFSET+__RODATA_PM_OFFSET__ = DEFINED(__RODATA_PM_OFFSET__) ? __RODATA_PM_OFFSET__ : $RODATA_PM_OFFSET;}
${HAVE_FLMAP+__RODATA_VMA__ = ${RODATA_VMA};}
${HAVE_FLMAP+__RODATA_LDS_OFFSET__ = DEFINED(__RODATA_LDS_OFFSET__) ? __RODATA_LDS_OFFSET__ : ${RODATA_LDS_OFFSET};}
${HAVE_FLMAP+__RODATA_REGION_LENGTH__ = DEFINED(__RODATA_REGION_LENGTH__) ? __RODATA_REGION_LENGTH__ : ${RODATA_LENGTH};}
${HAVE_FLMAP+__RODATA_ORIGIN__ = __RODATA_VMA__ + __RODATA_LDS_OFFSET__;}
MEMORY
{
  text   (rx)   : ORIGIN = __TEXT_REGION_ORIGIN__, LENGTH = __TEXT_REGION_LENGTH__
  data   (rw!x) : ORIGIN = __DATA_REGION_ORIGIN__, LENGTH = __DATA_REGION_LENGTH__
${EEPROM_LENGTH+  eeprom (rw!x) : ORIGIN = 0x810000, LENGTH = __EEPROM_REGION_LENGTH__}
  $FUSE_NAME      (rw!x) : ORIGIN = 0x820000, LENGTH = __FUSE_REGION_LENGTH__
  lock      (rw!x) : ORIGIN = 0x830000, LENGTH = __LOCK_REGION_LENGTH__
  signature (rw!x) : ORIGIN = 0x840000, LENGTH = __SIGNATURE_REGION_LENGTH__
${USER_SIGNATURE_LENGTH+  user_signatures (rw!x) : ORIGIN = 0x850000, LENGTH = __USER_SIGNATURE_REGION_LENGTH__}
${HAVE_FLMAP+  rodata (r!x) : ORIGIN = __RODATA_ORIGIN__, LENGTH = __RODATA_REGION_LENGTH__}
}
EOF

cat <<EOF
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  ${TEXT_DYNAMIC+${DYNAMIC}}
  .hash        ${RELOCATING-0} : { *(.hash)		}
  .dynsym      ${RELOCATING-0} : { *(.dynsym)		}
  .dynstr      ${RELOCATING-0} : { *(.dynstr)		}
  .gnu.version ${RELOCATING-0} : { *(.gnu.version)	}
  .gnu.version_d ${RELOCATING-0} : { *(.gnu.version_d)	}
  .gnu.version_r ${RELOCATING-0} : { *(.gnu.version_r)	}

  .rel.init    ${RELOCATING-0} : { *(.rel.init)		}
  .rela.init   ${RELOCATING-0} : { *(.rela.init)	}
  .rel.text    ${RELOCATING-0} :
    {
      *(.rel.text)
      ${RELOCATING+*(.rel.text.*)}
      ${RELOCATING+*(.rel.gnu.linkonce.t*)}
    }
  .rela.text   ${RELOCATING-0} :
    {
      *(.rela.text)
      ${RELOCATING+*(.rela.text.*)}
      ${RELOCATING+*(.rela.gnu.linkonce.t*)}
    }
  .rel.fini    ${RELOCATING-0} : { *(.rel.fini)		}
  .rela.fini   ${RELOCATING-0} : { *(.rela.fini)	}
  .rel.rodata  ${RELOCATING-0} :
    {
      *(.rel.rodata)
      ${RELOCATING+*(.rel.rodata.*)}
      ${RELOCATING+*(.rel.gnu.linkonce.r*)}
    }
  .rela.rodata ${RELOCATING-0} :
    {
      *(.rela.rodata)
      ${RELOCATING+*(.rela.rodata.*)}
      ${RELOCATING+*(.rela.gnu.linkonce.r*)}
    }
  .rel.data    ${RELOCATING-0} :
    {
      *(.rel.data)
      ${RELOCATING+*(.rel.data.*)}
      ${RELOCATING+*(.rel.gnu.linkonce.d*)}
    }
  .rela.data   ${RELOCATING-0} :
    {
      *(.rela.data)
      ${RELOCATING+*(.rela.data.*)}
      ${RELOCATING+*(.rela.gnu.linkonce.d*)}
    }
  .rel.ctors   ${RELOCATING-0} : { *(.rel.ctors)	}
  .rela.ctors  ${RELOCATING-0} : { *(.rela.ctors)	}
  .rel.dtors   ${RELOCATING-0} : { *(.rel.dtors)	}
  .rela.dtors  ${RELOCATING-0} : { *(.rela.dtors)	}
  .rel.got     ${RELOCATING-0} : { *(.rel.got)		}
  .rela.got    ${RELOCATING-0} : { *(.rela.got)		}
  .rel.bss     ${RELOCATING-0} : { *(.rel.bss)		}
  .rela.bss    ${RELOCATING-0} : { *(.rela.bss)		}
  .rel.plt     ${RELOCATING-0} : { *(.rel.plt)		}
  .rela.plt    ${RELOCATING-0} : { *(.rela.plt)		}

  /* Internal text space or external memory.  */
  .text ${RELOCATING-0} :
  {
    ${RELOCATING+*(.vectors)
    KEEP(*(.vectors))

    /* For data that needs to reside in the lower 64k of progmem.  */
    *(.progmem.gcc*)

    /* PR 13812: Placing the trampolines here gives a better chance
       that they will be in range of the code that uses them.  */
    . = ALIGN(2);
    __trampolines_start = . ;
    /* The jump trampolines for the 16-bit limited relocs will reside here.  */
    *(.trampolines)
    *(.trampolines*)
    __trampolines_end = . ;

    /* avr-libc expects these data to reside in lower 64K. */
    *libprintf_flt.a:*(.progmem.data)
    *libc.a:*(.progmem.data)

    *(.progmem.*)

    . = ALIGN(2);

    /* For code that needs to reside in the lower 128k progmem.  */
    *(.lowtext)
    *(.lowtext*)}

    ${CONSTRUCTING+ __ctors_start = . ; }
    ${CONSTRUCTING+ *(.ctors) }
    ${CONSTRUCTING+ __ctors_end = . ; }
    ${CONSTRUCTING+ __dtors_start = . ; }
    ${CONSTRUCTING+ *(.dtors) }
    ${CONSTRUCTING+ __dtors_end = . ; }
    ${RELOCATING+KEEP(SORT(*)(.ctors))
    KEEP(SORT(*)(.dtors))

    /* From this point on, we do not bother about whether the insns are
       below or above the 16 bits boundary.  */
    *(.init0)  /* Start here after reset.  */
    KEEP (*(.init0))
    *(.init1)
    KEEP (*(.init1))
    *(.init2)  /* Clear __zero_reg__, set up stack pointer.  */
    KEEP (*(.init2))
    *(.init3)
    KEEP (*(.init3))
    *(.init4)  /* Initialize data and BSS.  */
    KEEP (*(.init4))
    *(.init5)
    KEEP (*(.init5))
    *(.init6)  /* C++ constructors.  */
    KEEP (*(.init6))
    *(.init7)
    KEEP (*(.init7))
    *(.init8)
    KEEP (*(.init8))
    *(.init9)  /* Call main().  */
    KEEP (*(.init9))}
    *(.text)
    ${RELOCATING+. = ALIGN(2);
    *(.text.*)
    . = ALIGN(2);
    *(.fini9)  /* _exit() starts here.  */
    KEEP (*(.fini9))
    *(.fini8)
    KEEP (*(.fini8))
    *(.fini7)
    KEEP (*(.fini7))
    *(.fini6)  /* C++ destructors.  */
    KEEP (*(.fini6))
    *(.fini5)
    KEEP (*(.fini5))
    *(.fini4)
    KEEP (*(.fini4))
    *(.fini3)
    KEEP (*(.fini3))
    *(.fini2)
    KEEP (*(.fini2))
    *(.fini1)
    KEEP (*(.fini1))
    *(.fini0)  /* Infinite loop after program termination.  */
    KEEP (*(.fini0))

    /* For code that needs not to reside in the lower progmem.  */
    *(.hightext)
    *(.hightext*)

    *(.progmemx.*)

    . = ALIGN(2);

    /* For tablejump instruction arrays.  We do not relax
       JMP / CALL instructions within these sections.  */
    *(.jumptables)
    *(.jumptables*)

    _etext = . ;}
  } ${RELOCATING+ > text}
EOF

# Devices like ATtiny816 allow to read from flash memory by means of LD*
# instructions provided we add an offset of __RODATA_PM_OFFSET__ to the
# flash addresses.

if test -n "$RODATA_PM_OFFSET"; then
    cat <<EOF
  .rodata ${RELOCATING+ ADDR(.text) + SIZEOF (.text) + __RODATA_PM_OFFSET__ } ${RELOCATING-0} :
  {
    *(.rodata)
    ${RELOCATING+ *(.rodata*)
    *(.gnu.linkonce.r*)}
  } ${RELOCATING+AT> text}
EOF
fi

cat <<EOF
  .data        ${RELOCATING-0} :
  {
    ${RELOCATING+ PROVIDE (__data_start = .) ; }
    *(.data)
    ${RELOCATING+ *(.data*)
    *(.gnu.linkonce.d*)}
EOF

# Classical devices that don't show flash memory in the SRAM address space
# need .rodata to be part of .data because the compiler will use LD*
# instructions and LD* cannot access flash.

if test -z "$RODATA_PM_OFFSET" && test -z "${HAVE_FLMAP}" && test -n "${RELOCATING}"; then
    cat <<EOF
    *(.rodata)  /* We need to include .rodata here if gcc is used */
    *(.rodata*) /* with -fdata-sections.  */
    *(.gnu.linkonce.r*)
EOF
fi

cat <<EOF
    ${RELOCATING+. = ALIGN(2);}
    ${RELOCATING+ _edata = . ; }
    ${RELOCATING+ PROVIDE (__data_end = .) ; }
  } ${RELOCATING+ > data ${RELOCATING+AT> text}}

  .bss ${RELOCATING+ ADDR(.data) + SIZEOF (.data)} ${RELOCATING-0} :${RELOCATING+ AT (ADDR (.bss))}
  {
    ${RELOCATING+ PROVIDE (__bss_start = .) ; }
    *(.bss)
    ${RELOCATING+ *(.bss*)}
    ${RELOCATING+ *(COMMON)}
    ${RELOCATING+ PROVIDE (__bss_end = .) ; }
  } ${RELOCATING+ > data}

  ${RELOCATING+ __data_load_start = LOADADDR(.data); }
  ${RELOCATING+ __data_load_end = __data_load_start + SIZEOF(.data); }

  /* Global data not cleared after reset.  */
  .noinit ${RELOCATING+ ADDR(.bss) + SIZEOF (.bss)} ${RELOCATING-0}: ${RELOCATING+ AT (ADDR (.noinit))}
  {
    ${RELOCATING+ PROVIDE (__noinit_start = .) ; }
    *(.noinit${RELOCATING+ .noinit.* .gnu.linkonce.n.*})
    ${RELOCATING+ PROVIDE (__noinit_end = .) ; }
    ${RELOCATING+ _end = . ;  }
    ${RELOCATING+ PROVIDE (__heap_start = .) ; }
  } ${RELOCATING+ > data}
EOF

# Devices like AVR128DA32 and AVR64DA32 see a 32 KiB block of their program
# memory at 0x8000 (RODATA_LDS_OFFSET).  Which portion will be determined by
# bitfield NVMCTRL_CTRLB.FLMAP.

if test -z "${HAVE_FLMAP}" && test -n "${RELOCATING}"; then
    cat <<EOF

PROVIDE (__flmap_init_label = DEFINED(__flmap_noinit_start) ? __flmap_noinit_start : 0) ;
PROVIDE (__flmap = DEFINED(__flmap) ? __flmap : 0) ;

EOF
fi

if test -n "${HAVE_FLMAP}"; then
    cat <<EOF

${RELOCATING+
PROVIDE (__flmap_init_label = DEFINED(__flmap_init_start) ? __flmap_init_start : 0) ;
/* User can specify position of .rodata in flash (LMA) by supplying
   __RODATA_FLASH_START__ or __flmap, where the former takes precedence. */
__RODATA_FLASH_START__ = DEFINED(__RODATA_FLASH_START__)
   ? __RODATA_FLASH_START__
   : DEFINED(__flmap) ? __flmap * 32K : ${RODATA_FLASH_START};
ASSERT (__RODATA_FLASH_START__ % 32K == 0, \"__RODATA_FLASH_START__ must be a multiple of 32 KiB\")
__flmap = ${FLMAP_MASK} & (__RODATA_FLASH_START__ >> 15);
__RODATA_FLASH_START__ = __flmap << 15;
__rodata_load_start = MAX (__data_load_end, __RODATA_FLASH_START__);
__rodata_start = __RODATA_ORIGIN__ + __rodata_load_start - __RODATA_FLASH_START__;}

  .rodata ${RELOCATING+ __rodata_start} ${RELOCATING-0} : ${RELOCATING+ AT (__rodata_load_start)}
  {
    *(.rodata)
    ${RELOCATING+ *(.rodata*)}
    ${RELOCATING+ *(.gnu.linkonce.r*)}
    ${RELOCATING+ __rodata_end = ABSOLUTE(.) ;}
  } ${RELOCATING+ > rodata}

${RELOCATING+ __rodata_load_end = __rodata_load_start + __rodata_end - __rodata_start;}

EOF
fi

if test -n "${MAYBE_FLMAP}" && test -n "${RELOCATING}"; then
    cat <<EOF

__do_init_flmap = ${HAVE_FLMAP-0};
__flmap_value = __flmap << (DEFINED(__flmap_bpos) ? __flmap_bpos : 4);
__flmap_value_with_lock =
   __flmap_value | (DEFINED(__flmap_lock) && DEFINED(__flmap_lock_mask)
                    ? (__flmap_lock && __do_init_flmap ? __flmap_lock_mask : 0)
                    : 0);

EOF
fi

if test -n "${EEPROM_LENGTH}"; then
cat <<EOF

  .eeprom ${RELOCATING-0}:
  {
    /* See .data above...  */
    KEEP(*(.eeprom*))
    ${RELOCATING+ __eeprom_end = . ; }
  } ${RELOCATING+ > eeprom}
EOF
fi

if test "$FUSE_NAME" = "fuse" ; then
cat <<EOF

  .fuse ${RELOCATING-0}:
  {
    KEEP(*(.fuse))
    ${RELOCATING+KEEP(*(.lfuse))
    KEEP(*(.hfuse))
    KEEP(*(.efuse))}
  } ${RELOCATING+ > fuse}
EOF
fi

cat <<EOF

  .lock ${RELOCATING-0}:
  {
    KEEP(*(.lock*))
  } ${RELOCATING+ > lock}

  .signature ${RELOCATING-0}:
  {
    KEEP(*(.signature*))
  } ${RELOCATING+ > signature}
EOF

if test "$FUSE_NAME" = "config" ; then
cat <<EOF

  .config ${RELOCATING-0}:
  {
    KEEP(*(.config*))
  } ${RELOCATING+ > config}
EOF
fi

source_sh $srcdir/scripttempl/misc-sections.sc

cat <<EOF
  .note.gnu.build-id ${RELOCATING-0} : { *(.note.gnu.build-id) }
EOF

source_sh $srcdir/scripttempl/DWARF.sc

cat <<EOF
}
EOF
