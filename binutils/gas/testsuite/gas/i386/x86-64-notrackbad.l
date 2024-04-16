.*: Assembler messages:
.*:5: Error: expecting indirect branch instruction after `notrack'
.*:5: Warning: skipping prefixes on `call'
.*:6: Error: expecting indirect branch instruction after `notrack'
.*:8: Error: same type of prefix used twice
.*:9: Error: same type of prefix used twice
.*:12: Error: same type of prefix used twice
.*:13: Error: same type of prefix used twice
GAS LISTING .*


[ 	]*1[ 	]+\# Check 64bit unsupported NOTRACK prefix
[ 	]*2[ 	]+
[ 	]*3[ 	]+\.text
[ 	]*4[ 	]+_start:
[ 	]*5[ 	]+\?\?\?\? [0-9A-F]* 		notrack call foo
\*\*\*\*  Error: expecting indirect branch instruction after `notrack'
\*\*\*\*  Warning: skipping prefixes on `call'
[ 	]*5[ 	]+[0-9A-F]*
[ 	]*6[ 	]+\?\?\?\? [0-9A-F]* 		notrack jmp foo
\*\*\*\*  Error: expecting indirect branch instruction after `notrack'
[ 	]*6[ 	]+[0-9A-F]*
[ 	]*7[ 	]+
[ 	]*8[ 	]+fs notrack call \*%rax
[ 	]*9[ 	]+notrack fs call \*%rax
[ 	]*10[ 	]+
[ 	]*11[ 	]+\.intel_syntax noprefix
[ 	]*12[ 	]+fs notrack call rax
[ 	]*13[ 	]+notrack fs call rax
#pass
