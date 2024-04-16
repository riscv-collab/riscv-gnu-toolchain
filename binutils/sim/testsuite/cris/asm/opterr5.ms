# mach: crisv3 crisv8 crisv10 crisv32
# xerror:
# output: Invalid option [`']--cris-program-offset=x'\n
# sim: --cris-program-offset=x
 .include "nopv32t.ms"

; Check that we recognize wrong usage of the --cris-program-offset option.
