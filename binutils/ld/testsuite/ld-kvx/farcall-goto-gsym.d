#name: kvx-farcall-goto-gsym
#source: farcall-goto-gsym.s
#as:
#ld: -Ttext 0x1000
#error: .*\(.text\+0x0\): relocation truncated to fit: R_KVX_PCREL27 against symbol `bar_gsym'.*
