#source: pr30791a.s
#source: pr30791b.s
#ld: -r
#readelf: -S --wide
#xfail: [is_generic] fr30-*-* frv-*-elf ft32-*-* iq2000-*-* mn10200-*-*
#xfail: msp430-* mt-*-* z80-*-*
# Generic linker targets don't comply with all orhpan section merging
# rules.  z80 fails since a, b, c, d are registers for z80.

#...
Section Headers:
#...
  \[[ 0-9]+\] __patchable_function_entries[ \t]+PROGBITS[ \t0-9a-f]+WAL.*
#...
  \[[ 0-9]+\] __patchable_function_entries[ \t]+PROGBITS[ \t0-9a-f]+WAL.*
#...
  \[[ 0-9]+\] __patchable_function_entries[ \t]+PROGBITS[ \t0-9a-f]+WAL.*
#...
  \[[ 0-9]+\] __patchable_function_entries[ \t]+PROGBITS[ \t0-9a-f]+WAL.*
#pass
