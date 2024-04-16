        call -2147483648
        call -2147483649        /* This overflows.  */
        call 4294967295
        call 4294967296         /* This overflows.  */
