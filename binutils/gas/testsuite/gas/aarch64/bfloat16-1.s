bfadd z0.h, p0/m, z0.h, z16.h
bfadd z1.h, p1/m, z1.h, z8.h
bfadd z2.h, p2/m, z2.h, z4.h
bfadd z4.h, p4/m, z4.h, z2.h
bfadd z8.h, p6/m, z8.h, z1.h
bfadd z16.h, p7/m, z16.h, z0.h

bfmax z0.h, p0/m, z0.h, z16.h
bfmax z1.h, p1/m, z1.h, z8.h
bfmax z2.h, p2/m, z2.h, z4.h
bfmax z4.h, p4/m, z4.h, z2.h
bfmax z8.h, p6/m, z8.h, z1.h
bfmax z16.h, p7/m, z16.h, z0.h

bfmaxnm z0.h, p0/m, z0.h, z16.h
bfmaxnm z1.h, p1/m, z1.h, z8.h
bfmaxnm z2.h, p2/m, z2.h, z4.h
bfmaxnm z4.h, p4/m, z4.h, z2.h
bfmaxnm z8.h, p6/m, z8.h, z1.h
bfmaxnm z16.h, p7/m, z16.h, z0.h

bfmin z0.h, p0/m, z0.h, z16.h
bfmin z1.h, p1/m, z1.h, z8.h
bfmin z2.h, p2/m, z2.h, z4.h
bfmin z4.h, p4/m, z4.h, z2.h
bfmin z8.h, p6/m, z8.h, z1.h
bfmin z16.h, p7/m, z16.h, z0.h

bfminnm z0.h, p0/m, z0.h, z16.h
bfminnm z1.h, p1/m, z1.h, z8.h
bfminnm z2.h, p2/m, z2.h, z4.h
bfminnm z4.h, p4/m, z4.h, z2.h
bfminnm z8.h, p6/m, z8.h, z1.h
bfminnm z16.h, p7/m, z16.h, z0.h

bfadd z0.h, z4.h, z16.h
bfadd z1.h, z8.h, z8.h
bfadd z2.h, z12.h, z4.h
bfadd z4.h, z16.h, z2.h
bfadd z8.h, z20.h, z1.h
bfadd z16.h, z24.h, z0.h

bfclamp z0.h, z4.h, z16.h
bfclamp z1.h, z8.h, z8.h
bfclamp z2.h, z12.h, z4.h
bfclamp z4.h, z16.h, z2.h
bfclamp z8.h, z20.h, z1.h
bfclamp z16.h, z24.h, z0.h
bfmla z0.h, p0/m, z0.h, z16.h
bfmla z1.h, p1/m, z1.h, z8.h
bfmla z2.h, p2/m, z2.h, z4.h
bfmla z4.h, p4/m, z4.h, z2.h
bfmla z8.h, p6/m, z8.h, z1.h
bfmla z16.h, p7/m, z16.h, z0.h

bfmla z0.h, z16.h, z6.h[7]
bfmla z1.h, z8.h, z5.h[6]
bfmla z2.h, z14.h, z4.h[4]
bfmla z4.h, z21.h, z2.h[2]
bfmla z8.h, z12.h, z1.h[1]
bfmla z16.h, z10.h, z0.h[0]

bfmls z0.h, p0/m, z0.h, z16.h
bfmls z1.h, p1/m, z1.h, z8.h
bfmls z2.h, p2/m, z2.h, z4.h
bfmls z4.h, p4/m, z4.h, z2.h
bfmls z8.h, p6/m, z8.h, z1.h
bfmls z16.h, p7/m, z16.h, z0.h

bfmls z0.h, z16.h, z6.h[7]
bfmls z1.h, z8.h, z5.h[6]
bfmls z2.h, z14.h, z4.h[4]
bfmls z4.h, z21.h, z2.h[2]
bfmls z8.h, z12.h, z1.h[1]
bfmls z16.h, z10.h, z0.h[0]

bfmul z0.h, p0/m, z0.h, z16.h
bfmul z1.h, p1/m, z1.h, z8.h
bfmul z2.h, p2/m, z2.h, z4.h
bfmul z4.h, p4/m, z4.h, z2.h
bfmul z8.h, p6/m, z8.h, z1.h
bfmul z16.h, p7/m, z16.h, z0.h

bfmul z0.h, z4.h, z16.h
bfmul z1.h, z8.h, z8.h
bfmul z2.h, z12.h, z4.h
bfmul z4.h, z16.h, z2.h
bfmul z8.h, z20.h, z1.h
bfmul z16.h, z24.h, z0.h

bfmul z0.h, z16.h, z6.h[7]
bfmul z1.h, z8.h, z5.h[6]
bfmul z2.h, z14.h, z4.h[4]
bfmul z4.h, z21.h, z2.h[2]
bfmul z8.h, z12.h, z1.h[1]
bfmul z16.h, z10.h, z0.h[0]

bfsub z0.h, p0/m, z0.h, z16.h
bfsub z1.h, p1/m, z1.h, z8.h
bfsub z2.h, p2/m, z2.h, z4.h
bfsub z4.h, p4/m, z4.h, z2.h
bfsub z8.h, p6/m, z8.h, z1.h
bfsub z16.h, p7/m, z16.h, z0.h

bfsub z0.h, z4.h, z16.h
bfsub z1.h, z8.h, z8.h
bfsub z2.h, z12.h, z4.h
bfsub z4.h, z16.h, z2.h
bfsub z8.h, z20.h, z1.h
bfsub z16.h, z24.h, z0.h


