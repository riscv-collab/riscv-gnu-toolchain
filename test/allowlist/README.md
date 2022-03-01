This folder contain all allowlist files for testsuite result,
it used for `riscv-gnu-toolchain/scripts/testsuite-filter`,
naming rule of allowlist file as below:

```
<toolname>/common.log
<toolname>/[<lib>.][rv(32|64|128).][<ext>.][<abi>.]log
```

- `toolname` can be `gcc`, `binutils` or `gdb`.

- `<toolname>/common.log`: Every target/library combination for the `<toolname>`
  will use this allowlist file.

- `<toolname>/[<lib>.][rv(32|64|128).][<ext>.][<abi>.]log`: `testsuite-filter`
  will according the target/library combination to match corresponding allowlist
  files.

- For example, rv32im,ilp32/newlib will match following 24 files, and ignored if
  file not exist:
    - common.log
    - newlib.log
    - rv32.log
    - ilp32.log
    - rv32.ilp32.log
    - newlib.rv32.log
    - newlib.ilp32.log
    - newlib.rv32.ilp32.log
    - i.log
    - rv32.i.log
    - i.ilp32.log
    - rv32.i.ilp32.log
    - newlib.i.log
    - newlib.rv32.i.log
    - newlib.i.ilp32.log
    - newlib.rv32.i.ilp32.log
    - m.log
    - rv32.m.log
    - m.ilp32.log
    - rv32.m.ilp32.log
    - newlib.m.log
    - newlib.rv32.m.log
    - newlib.m.ilp32.log
    - newlib.rv32.m.ilp32.log
