# MacOS Build Common Errors

## fdopen macro redefinition error

During `make check-binutils`, you may see an error like this:
```bash
In file included from /Volumes/case-sensitive/riscv-gnu-toolchain/binutils/zlib/zlib/zutil.c:10:
In file included from /Volumes/case-sensitive/riscv-gnu-toolchain/binutils/zlib/zlib/gzguts.h:21:
In file included from /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/stdio.h:61:
/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/_stdio.h:322:7: error: expected identifier or '('
  322 | FILE    *fdopen(int, const char *) __DARWIN_ALIAS_STARTING(__MAC_10_6, __IPHONE_2_0, __DARWIN_ALIAS(fdopen));
      |          ^
/Volumes/case-sensitive/riscv-gnu-toolchain/binutils/zlib/zlib/zutil.h:147:33: note: expanded from macro 'fdopen'
  147 | #        define fdopen(fd,mode) NULL /* No fdopen() */
      |                                 ^
/Library/Developer/CommandLineTools/usr/lib/clang/17/include/__stddef_null.h:26:16: note: expanded from macro 'NULL'
   26 | #define NULL ((void*)0)
      |                ^
```

Then, upate the `binutils` repository to the latest commit on the `gdb-17-branch`. The easiest way to do this is (in the `riscv-gnu-toolchain` folder):

```bash
rm -rf binutils
git clone https://github.com/bminor/binutils-gdb binutils
cd binutils
git checkout gdb-17-branch
cd ..
```
(The gdb-17-branch was last tested to be working on MacOS Tahoe 26.2 (25C56) on an M2 Max machine model A2779)


## Bison/YACC related errors
There may be missing files errors like this during `make`:
```bash
clang: error: no such file or directory: '/Volumes/case-sensitive/riscv-gnu-toolchain/binutils/binutils/sysinfo.c'
clang: error: no input files
gmake[3]: *** [Makefile:1897: sysinfo.o] Error 1
gmake[3]: Leaving directory '/Volumes/case-sensitive/riscv-gnu-toolchain/build-binutils-newlib/binutils'
```
To fix this, you need to run some files manually through Bison in YACC mode:
```bash
cd binutils/binutils
bison -y -o sysinfo.c sysinfo.y
bison -y -d --defines=sysinfo.h -o sysinfo.c sysinfo.y
bison -d --defines=arparse.h -o arparse.c arparse.y

cd binutils/ld
bison -d --defines=ldgram.h -o ldgram.c ldgram.y
```