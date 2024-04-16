# mach: crisv3 crisv8 crisv10 crisv32
# ld: -N --oformat binary --section-start=.text=0x10000000
# sim: --cris-naked --memory-mapfile
# xerror:
# output: Usage: run *\n*\n
# progopts: --memory-region 0x10000000,0x1000
 .include "bare3.ms"

; Check that we get an error for wrong usage, not a SEGV for lack of
; bfd when missing the program argument (can't use *only* mapped files).
