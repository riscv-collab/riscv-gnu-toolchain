#source: large-sections.s
#PROG: objcopy
#as: --64
#objcopy: -O elf32-i386 --set-section-flags .data=alloc,large
#target: x86_64-*-linux*
#error: \A[^[]*\[.data\]: 'large' flag is ELF x86-64 specific
