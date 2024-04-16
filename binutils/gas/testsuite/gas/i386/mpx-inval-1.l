.*: Assembler messages:
.*:5: Error: expecting valid branch instruction after `bnd'
.*:6: Error: expecting valid branch instruction after `bnd'
.*:7: Error: expecting valid branch instruction after `bnd'
.*:7: Warning: skipping prefixes on `lcall'
.*:8: Error: expecting valid branch instruction after `bnd'
.*:8: Warning: skipping prefixes on `ljmp'
.*:9: Error: expecting valid branch instruction after `bnd'
.*:10: Error: expecting valid branch instruction after `bnd'
.*:13: Error: expecting valid branch instruction after `bnd'
.*:14: Error: expecting valid branch instruction after `bnd'
.*:15: Error: expecting valid branch instruction after `bnd'
.*:15: Warning: skipping prefixes on `lcall'
.*:16: Error: expecting valid branch instruction after `bnd'
.*:16: Warning: skipping prefixes on `ljmp'
.*:17: Error: expecting valid branch instruction after `bnd'
.*:18: Error: expecting valid branch instruction after `bnd'
GAS LISTING .*


[ 	]*1[ 	]+\# MPX instructions
[ 	]*2[ 	]+\.text
[ 	]*3[ 	]+\.extern xxx
[ 	]*4[ 	]+foo:
[ 	]*5[ 	]+\?\?\?\? F201C3   		bnd add %eax, %ebx  		\# Bad
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*6[ 	]+\?\?\?\? 66F2AB   		bnd stosw \(%edi\)    		\# Bad
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*7[ 	]+\?\?\?\? 9A000000 		bnd lcall \$0x1234,\$xxx
.*  Error: expecting valid branch instruction after `bnd'
.*  Warning: skipping prefixes on `lcall'
[ 	]*7[ 	]+003412
[ 	]*8[ 	]+\?\?\?\? EA000000 		bnd ljmp \$0x1234,\$xxx
.*  Error: expecting valid branch instruction after `bnd'
.*  Warning: skipping prefixes on `ljmp'
[ 	]*8[ 	]+003412
[ 	]*9[ 	]+\?\?\?\? F2E2E9   		bnd loop foo
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*10[ 	]+\?\?\?\? 67F2E3E5 		bnd jcxz foo
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*11[ 	]+
[ 	]*12[ 	]+\.intel_syntax noprefix
[ 	]*13[ 	]+\?\?\?\? F201C3   		bnd add ebx, eax		\# Bad
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*14[ 	]+\?\?\?\? 66F2AB   		bnd stos WORD PTR\[edi]		\# Bad
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*15[ 	]+\?\?\?\? 9A000000 		bnd lcall 0x1234,xxx
.*  Error: expecting valid branch instruction after `bnd'
.*  Warning: skipping prefixes on `lcall'
[ 	]*15[ 	]+003412
[ 	]*16[ 	]+\?\?\?\? EA000000 		bnd ljmp 0x1234,xxx
.*  Error: expecting valid branch instruction after `bnd'
.*  Warning: skipping prefixes on `ljmp'
[ 	]*16[ 	]+003412
[ 	]*17[ 	]+\?\?\?\? F2E2CE   		bnd loop foo
.*  Error: expecting valid branch instruction after `bnd'
[ 	]*18[ 	]+\?\?\?\? 67F2E3CA 		bnd jcxz foo
.*  Error: expecting valid branch instruction after `bnd'
#pass
