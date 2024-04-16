addqv v0.16b, p0, z16.b
addqv v1.8h, p1, z8.h
addqv v2.4s, p2, z4.s
addqv v4.2d, p3, z2.d
addqv v8.2d, p4, z1.d
addqv v16.4s, p7, z0.s

andqv v0.16b, p0, z16.b
andqv v1.8h, p1, z8.h
andqv v2.4s, p2, z4.s
andqv v4.2d, p3, z2.d
andqv v8.2d, p4, z1.d
andqv v16.4s, p7, z0.s

smaxqv v0.16b, p0, z16.b
smaxqv v1.8h, p1, z8.h
smaxqv v2.4s, p2, z4.s
smaxqv v4.2d, p3, z2.d
smaxqv v8.2d, p4, z1.d
smaxqv v16.4s, p7, z0.s

umaxqv v0.16b, p0, z16.b
umaxqv v1.8h, p1, z8.h
umaxqv v2.4s, p2, z4.s
umaxqv v4.2d, p3, z2.d
umaxqv v8.2d, p4, z1.d
umaxqv v16.4s, p7, z0.s

sminqv v0.16b, p0, z16.b
sminqv v1.8h, p1, z8.h
sminqv v2.4s, p2, z4.s
sminqv v4.2d, p3, z2.d
sminqv v8.2d, p4, z1.d
sminqv v16.4s, p7, z0.s

uminqv v0.16b, p0, z16.b
uminqv v1.8h, p1, z8.h
uminqv v2.4s, p2, z4.s
uminqv v4.2d, p3, z2.d
uminqv v8.2d, p4, z1.d
uminqv v16.4s, p7, z0.s
dupq z10.b, z20.b[0]
dupq z10.b, z20.b[15]
dupq z10.h, z20.h[0]
dupq z10.h, z20.h[7]
dupq z10.s, z20.s[0]
dupq z10.s, z20.s[3]
dupq z10.d, z20.d[0]
dupq z10.d, z20.d[1]

eorqv v0.16b, p0, z16.b
eorqv v1.8h, p1, z8.h
eorqv v2.4s, p2, z4.s
eorqv v4.2d, p3, z2.d
eorqv v8.2d, p4, z1.d
eorqv v16.4s, p7, z0.s

extq z0.b, z0.b, z10.b[15]
extq z1.b, z1.b, z15.b[7]
extq z2.b, z2.b, z5.b[3]
extq z4.b, z4.b, z12.b[1]
extq z8.b, z8.b, z7.b[4]
extq z16.b, z16.b, z1.b[8]
faddqv v1.8h, p1, z8.h
faddqv v2.4s, p2, z4.s
faddqv v4.2d, p3, z2.d
faddqv v8.2d, p4, z1.d
faddqv v16.4s, p7, z0.s

fmaxnmqv v1.8h, p1, z8.h
fmaxnmqv v2.4s, p2, z4.s
fmaxnmqv v4.2d, p3, z2.d
fmaxnmqv v8.2d, p4, z1.d
fmaxnmqv v16.4s, p7, z0.s

fmaxqv v1.8h, p1, z8.h
fmaxqv v2.4s, p2, z4.s
fmaxqv v4.2d, p3, z2.d
fmaxqv v8.2d, p4, z1.d
fmaxqv v16.4s, p7, z0.s

fminnmqv v1.8h, p1, z8.h
fminnmqv v2.4s, p2, z4.s
fminnmqv v4.2d, p3, z2.d
fminnmqv v8.2d, p4, z1.d
fminnmqv v16.4s, p7, z0.s

fminqv v1.8h, p1, z8.h
fminqv v2.4s, p2, z4.s
fminqv v4.2d, p3, z2.d
fminqv v8.2d, p4, z1.d
fminqv v16.4s, p7, z0.s
ld1q Z0.Q, p4/Z, [Z16.D, x0]
ld2q {Z0.Q, Z1.Q}, p4/Z, [x0,  #-4, MUL VL]
ld3q {Z0.Q, Z1.Q, Z2.Q}, p4/Z, [x0,  #-4, MUL VL]
ld4q {Z0.Q, Z1.Q, Z2.Q, Z3.Q}, p4/Z, [x0,  #-4, MUL VL]
ld2q {Z0.Q, Z1.Q}, p4/Z, [x0, x2, lsl  #4]
ld3q {Z0.Q, Z1.Q, Z2.Q}, p4/Z, [x0, x4, lsl  #4]
ld4q {Z0.Q, Z1.Q, Z2.Q, Z3.Q}, p4/Z, [x0, x6, lsl  #4]

st1q Z0.Q, p4, [Z16.D, x0]
st2q {Z0.Q, Z1.Q}, p4, [x0,  #-4, MUL VL]
st3q {Z0.Q, Z1.Q, Z2.Q}, p4, [x0,  #-4, MUL VL]
st4q {Z0.Q, Z1.Q, Z2.Q, Z3.Q}, p4, [x0,  #-4, MUL VL]
st2q {Z0.Q, Z1.Q}, p4, [x0, x2, lsl  #4]
st3q {Z0.Q, Z1.Q, Z2.Q}, p4, [x0, x4, lsl  #4]
st4q {Z0.Q, Z1.Q, Z2.Q, Z3.Q}, p4, [x0, x6, lsl  #4]
