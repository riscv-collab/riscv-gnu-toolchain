        .section .text.start

        la      sp, stack0
        call    main
loop:
        j       main

        .section .data.stack
        .align 16
        .fill 4096, 1, 0xA5
stack0:
