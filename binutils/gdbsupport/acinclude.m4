m4_include([../bfd/bfd.m4])
m4_include([common.m4])
m4_include([../config/ax_pthread.m4])
m4_include([../gdb/ax_cxx_compile_stdcxx.m4])
m4_include([../gdbsupport/libiberty.m4])
m4_include([selftest.m4])
m4_include([ptrace.m4])

dnl This gets AM_GDB_COMPILER_TYPE.
m4_include(compiler-type.m4)

dnl This gets AM_GDB_WARNINGS.
m4_include(warning.m4)
