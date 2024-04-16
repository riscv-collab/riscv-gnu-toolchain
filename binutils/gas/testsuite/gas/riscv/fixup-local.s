.global foo
.global bar
foo:
       la a0, .LL0
.LL0:
       la a0, bar
       la a0, foo
.LL1:
       auipc a0, %pcrel_hi(.LL2)
       lw    a0, %pcrel_lo(.LL1)(a0)

.LL2:
       ret
