.L1:
# A UND symbol but no more than the range of pcaddi,
# do pcalau12i + addi => pcaddi
la.tls.desc $a0,var
add.d $t0,$a0,$tp
