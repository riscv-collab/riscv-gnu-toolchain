<<<<<<< HEAD
=======
/* Based on linker script from the xv6-riscv project. */

>>>>>>> release/mp1
OUTPUT_ARCH("riscv")

PHDRS {
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

SECTIONS {
  /* ensure that _start is at 0x80000000 where qemu's -kernel jumps. */

  . = 0x80000000;

  .text (READONLY) : {
<<<<<<< HEAD
    PROVIDE(__kimg_text_start = .);
    start.o (.text.start)
    *(.text .text.*)
    . = ALIGN(0x1000);
    PROVIDE(__kimg_text_end = .);
  } :text

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  } :data

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  } :data

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  } :data

  PROVIDE(__kimg_end_all = .);
=======
    PROVIDE(_kimg_text_start = .);
    start.o (.text.start)
    *(.text .text.*)
    . = ALIGN(16);
    PROVIDE(_kimg_text_end = .);
  } :text

  .rodata : {
    *(.srodata .srodata.*)
    . = ALIGN(16);
    *(.rodata .rodata.*)
    . = ALIGN(16);
  } :data

  .data : {
    *(.sdata .sdata.*)
    . = ALIGN(16);
    *(.data .data.*)
    . = ALIGN(16);
  } :data

  .bss : {
    PROVIDE(_kimg_bss_start = .);
    *(.sbss .sbss.*)
    . = ALIGN(16);
    *(.bss .bss.*)
    . = ALIGN(16);
    PROVIDE(_kimg_bss_end = .);
    . = ALIGN(16);
  } :data
  
  PROVIDE(_kimg_end = .);
>>>>>>> release/mp1
}