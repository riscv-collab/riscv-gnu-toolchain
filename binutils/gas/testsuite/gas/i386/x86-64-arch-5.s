# Test -march=
	.text

    {vex} vpdpbusd %xmm12, %xmm4, %xmm2     #AVX_VNNI
    movdiri %rax, (%rcx)                    #MOVDIRI
    movdir64b (%rcx), %rax                  #MOVDIR64B
    vp2intersectd %zmm1, %zmm2, %k3         #AVX512_VP2INTERSECT
    prefetchit0 0x12345678(%rip)            #prefetchi
