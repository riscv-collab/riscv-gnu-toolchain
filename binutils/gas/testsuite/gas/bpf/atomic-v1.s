
        # Test for eBPF XADDW and XADDDW instructions.
    .text
        xadddw	[%r1+0x1eef], %r2
        xaddw	[%r1+0x1eef], %r2
