	.text
pseudos:
	{vex} vmovaps %xmm0, %xmm30
	{vex3} vmovaps %xmm30, %xmm0
	{rex} vmovaps %xmm7,%xmm2
	{rex} vmovaps %xmm17,%xmm2
	{rex} rorx $7,%eax,%ebx
	{rex2} vmovaps %xmm7,%xmm2
	{rex2} xsave (%rax)
	{rex2} xsaves (%ecx)
	{rex2} xsaves64 (%ecx)
	{rex2} xsavec (%ecx)
	{rex2} xrstors (%ecx)
	{rex2} xrstors64 (%ecx)

	#All opcodes in the row 0xA* (map0) prefixed REX2 are illegal.
	#{rex2} test (0xa8) is a special case, it will remap to test (0xf6)
	{rex2} mov    0x90909090,%al
	{rex2} movabs 0x1,%al
	{rex2} cmpsb  %es:(%edi),%ds:(%esi)
	{rex2} lodsb
	{rex2} lods   %ds:(%esi),%al
	{rex2} lodsb   (%esi)
	{rex2} movs
	{rex2} movs   (%esi), (%edi)
	{rex2} scasl
	{rex2} scas   %es:(%edi),%eax
	{rex2} scasb   (%edi)
	{rex2} stosb
	{rex2} stosb   (%edi)
	{rex2} stos   %eax,%es:(%edi)

	#All opcodes in the row 0x7* (map0) and 0x8* (map1) prefixed REX2 are illegal.
	{rex2} jo     .+2-0x70
	{rex2} jno    .+2-0x70
	{rex2} jb     .+2-0x70
	{rex2} jae    .+2-0x70
	{rex2} je     .+2-0x70
	{rex2} jne    .+2-0x70
	{rex2} jbe    .+2-0x70
	{rex2} ja     .+2-0x70
	{rex2} js     .+2-0x70
	{rex2} jns    .+2-0x70
	{rex2} jp     .+2-0x70
	{rex2} jnp    .+2-0x70
	{rex2} jl     .+2-0x70
	{rex2} jge    .+2-0x70
	{rex2} jle    .+2-0x70
	{rex2} jg     .+2-0x70
	{rex2} jo     .+6+0x90909090
	{rex2} jno    .+6+0x90909090
	{rex2} jb     .+6+0x90909090
	{rex2} jae    .+6+0x90909090
	{rex2} je     .+6+0x90909090
	{rex2} jne    .+6+0x90909090
	{rex2} jbe    .+6+0x90909090
	{rex2} ja     .+6+0x90909090
	{rex2} js     .+6+0x90909090
	{rex2} jns    .+6+0x90909090
	{rex2} jp     .+6+0x90909090
	{rex2} jnp    .+6+0x90909090
	{rex2} jl     .+6+0x90909090
	{rex2} jge    .+6+0x90909090
	{rex2} jle    .+6+0x90909090
	{rex2} jg     .+6+0x90909090

	#All opcodes in the row 0xE* (map0) prefixed REX2 are illegal.
	{rex2} in $0x90,%al
	{rex2} in $0x90
	{rex2} out $0x90,%al
	{rex2} out $0x90
	{rex2} jmp  *%eax
	{rex2} loop foo

	#All opcodes in the row 0x3* (map1) prefixed REX2 are illegal.
	{rex2} wrmsr
	{rex2} rdtsc
	{rex2} rdmsr
	{rex2} sysenter
	{rex2} sysexitl
	{rex2} rdpmc
