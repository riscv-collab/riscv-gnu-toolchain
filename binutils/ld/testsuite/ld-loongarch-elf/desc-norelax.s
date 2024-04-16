.L1:
# The got address of the symbol exceeds the
# range of pcaddi, do not relax.
la.tls.desc $a0,var
add.d $t0,$a0,$tp
