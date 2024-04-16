# Copyright (C) 2013-2024 Free Software Foundation, Inc.

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.

import sys
import re
import time

infname = sys.argv[1]
inf = file(infname)

print(
    """\
<?xml version="1.0"?>
<!-- Copyright (C) 2009-%s Free Software Foundation, Inc.

     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.  This file is offered as-is,
     without any warranty. -->

<!DOCTYPE feature SYSTEM "gdb-syscalls.dtd">

<!-- This file was generated using the following file:

     %s

     The file mentioned above belongs to the Linux Kernel.
     Some small hand-edits were made. -->

<syscalls_info>"""
    % (time.strftime("%Y"), infname)
)


def record(name, number, comment=None):
    # nm = 'name="%s"' % name
    # s = '  <syscall %-30s number="%d"/>' % (nm, number)
    s = '  <syscall name="%s" number="%d"/>' % (name, number)
    if comment:
        s += " <!-- %s -->" % comment
    print(s)


for line in inf:
    m = re.match(r"^#define __NR_(\w+)\s+\(__NR_SYSCALL_BASE\+\s*(\d+)\)", line)
    if m:
        record(m.group(1), int(m.group(2)))
        continue

    m = re.match(r"^\s+/\* (\d+) was sys_(\w+) \*/$", line)
    if m:
        record(m.group(2), int(m.group(1)), "removed")

    m = re.match(r"^#define __ARM_NR_(\w+)\s+\(__ARM_NR_BASE\+\s*(\d+)\)", line)
    if m:
        record("ARM_" + m.group(1), 0x0F0000 + int(m.group(2)))
        continue

print("</syscalls_info>")
