# Test -march=
	.text

    {vex} vpdpbusd %xmm2, %xmm4, %xmm2    #AVX_VNNI
    movdiri %eax, (%ecx)            #MOVDIRI
    movdir64b (%ecx), %eax          #MOVDIR64B
    vp2intersectd %zmm1, %zmm2, %k3 #AVX512_VP2INTERSECT
