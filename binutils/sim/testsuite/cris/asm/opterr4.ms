# mach: crisv3 crisv8 crisv10 crisv32
# xerror:
# output: Invalid option [`']--cris-start-address=x'\n
# sim: --cris-start-address=x
 .include "nopv32t.ms"

; Check that we recognize wrong usage of the --cris-start-address option.
