OUTPUT_FORMAT("elf32-m32r", "elf32-m32r",
	      "elf32-m32r")
OUTPUT_ARCH(m32r)
ENTRY(_start)

MEMORY
{
  RAM : ORIGIN = 0x208000, LENGTH = 0x100000
  OVLY_1 : ORIGIN = 0x300000, LENGTH = 0x40000
  OVLY_2 : ORIGIN = 0x340000, LENGTH = 0x40000
  OVLY_3 : ORIGIN = 0x380000, LENGTH = 0x40000
  OVLY_4 : ORIGIN = 0x3c0000, LENGTH = 0x40000
  OVLY_STORAGE : ORIGIN = 0x400000, LENGTH = 0x100000
}

/* Do we need any of these for elf?
   __DYNAMIC = 0;    */
SECTIONS
{
   OVERLAY :
      {
        .ovly0 { */overlays2.o(.text) }
        .ovly1 { */overlays3.o(.text) }
      } >OVLY_1 AT>OVLY_STORAGE
   OVERLAY :
      {
        .ovly2 { */overlays4.o(.text) }
        .ovly3 { */overlays5.o(.text) }
      } >OVLY_3 AT>OVLY_STORAGE
   OVERLAY :
      {
        .data00 { */overlays2.o(.data) }
        .data01 { */overlays3.o(.data) }
      } >OVLY_2 AT>OVLY_STORAGE
   OVERLAY :
      {
        .data02 { */overlays4.o(.data) }
        .data03 { */overlays5.o(.data) }
      } >OVLY_4 AT>OVLY_STORAGE

  /* Read-only sections, merged into text segment: */
  .interp        : { *(.interp) 	}
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .rel.text      : { *(.rel.text)	}
  .rela.text     : { *(.rela.text) 	}
  .rel.data      : { *(.rel.data)	}
  .rela.data     : { *(.rela.data) 	}
  .rel.rodata    : { *(.rel.rodata) 	}
  .rela.rodata   : { *(.rela.rodata) 	}
  .rel.got       : { *(.rel.got)	}
  .rela.got      : { *(.rela.got)	}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.init      : { *(.rel.init)	}
  .rela.init     : { *(.rela.init)	}
  .rel.fini      : { *(.rel.fini)	}
  .rela.fini     : { *(.rela.fini)	}
  .rel.bss       : { *(.rel.bss)	}
  .rela.bss      : { *(.rela.bss)	}
  .rel.plt       : { *(.rel.plt)	}
  .rela.plt      : { *(.rela.plt)	}
  .init          : { *(.init)		} >RAM AT>RAM =0
  .plt           : { *(.plt)		} >RAM AT>RAM

  .text          :
  {
    *(.text)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } >RAM AT>RAM =0
  _etext = .;
  PROVIDE (etext = .);
  .fini          : { *(.fini)		} >RAM AT>RAM =0
  .rodata        : { *(.rodata) *(.gnu.linkonce.r*) } >RAM AT>RAM
  .rodata1       : { *(.rodata1)	} >RAM AT>RAM
  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  . = ALIGN(32) + (ALIGN(8) & (32 - 1));
  .data          :
  {
    *(.data)
    *(.gnu.linkonce.d*)
    _ovly_table = .; 
      _ovly0_entry = .;
	LONG(ABSOLUTE(ADDR(.ovly0)));
	LONG(SIZEOF(.ovly0));
	LONG(LOADADDR(.ovly0));
	LONG(0);
      _ovly1_entry = .;
	LONG(ABSOLUTE(ADDR(.ovly1)));
	LONG(SIZEOF(.ovly1));
	LONG(LOADADDR(.ovly1));
	LONG(0);
      _ovly2_entry = .;
	LONG(ABSOLUTE(ADDR(.ovly2)));
	LONG(SIZEOF(.ovly2));
	LONG(LOADADDR(.ovly2));
	LONG(0);
      _ovly3_entry = .;
	LONG(ABSOLUTE(ADDR(.ovly3)));
	LONG(SIZEOF(.ovly3));
	LONG(LOADADDR(.ovly3));
	LONG(0);
      _data00_entry = .;
	LONG(ABSOLUTE(ADDR(.data00)));
	LONG(SIZEOF(.data00));
	LONG(LOADADDR(.data00));
	LONG(0);
      _data01_entry = .;
	LONG(ABSOLUTE(ADDR(.data01)));
	LONG(SIZEOF(.data01));
	LONG(LOADADDR(.data01));
	LONG(0);
      _data02_entry = .;
	LONG(ABSOLUTE(ADDR(.data02)));
	LONG(SIZEOF(.data02));
	LONG(LOADADDR(.data02));
	LONG(0);
      _data03_entry = .;
	LONG(ABSOLUTE(ADDR(.data03)));
	LONG(SIZEOF(.data03));
	LONG(LOADADDR(.data03));
	LONG(0);
    _novlys = .;
	LONG((_novlys - _ovly_table) / 16);

    CONSTRUCTORS
  } >RAM AT>RAM
  .data1         : { *(.data1)		} >RAM AT>RAM
  .ctors         : { *(.ctors)		} >RAM AT>RAM
  .dtors         : { *(.dtors)		} >RAM AT>RAM
  .got           : { *(.got.plt) *(.got)} >RAM AT>RAM
  .dynamic       : { *(.dynamic)	} >RAM AT>RAM
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata         : { *(.sdata)		} >RAM AT>RAM
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .sbss          : { *(.sbss) *(.scommon) } >RAM AT>RAM
  .bss           : { *(.dynbss) *(.bss) *(COMMON) } >RAM AT>RAM
  _end = . ;
  PROVIDE (end = .);
  /* Stabs debugging sections.  */
  .stab 0        : { *(.stab)		}
  .stabstr 0     : { *(.stabstr)	}
  .stab.excl 0   : { *(.stab.excl)	}
  .stab.exclstr 0 : { *(.stab.exclstr)	}
  .stab.index 0  : { *(.stab.index)	}
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0     : { *(.comment)	}
  /* DWARF debug sections.
     Symbols in the .debug DWARF section are relative to the beginning of the
     section so we begin .debug at 0.  It's not clear yet what needs to happen
     for the others.   */
  .debug          0 : { *(.debug) 	  }
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  .line           0 : { *(.line)	  }
  .stack   0x5ffffc : { _stack = .; *(.stack) }
  /* These must appear regardless of  .  */
}
