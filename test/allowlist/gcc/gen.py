#!/usr/bin/env python3
# Regenerate the GCC CI allowlists from downloaded workflow logs.
#
# Usage: drop the CI *.txt logs into this directory, then run:
#   python3 gen.py
#
# The script merges newly observed failures into the existing allowlists,
# only adding entries, never dropping them. Safe when CI fail-fast left some
# logs missing. To rebuild from scratch, delete the *.log files first.
#
# Completeness check: a CI log is considered complete only when it contains a
# "# of unexpected failures" summary line (written by dejagnu at the very end).
# An incomplete log (job cancelled mid-run) is excluded from the intersection
# recompute so that per-file categorization stays stable. Its base entries
# (read from the existing allowlist files) are still carried forward.

import glob
import os
import re
from tqdm import tqdm

STATUS_RE = re.compile(r'^.*((FAIL|UNRESOLVED|XPASS): .*\.(c|C|cc|cpp|f90|f95|f03|f08)) .*$')
# Also capture gcov prime-paths continuation lines (no filename, e.g. "FAIL: expected covered: '{...}'")
GCOV_PRIMEPATH_RE = re.compile(r'^.*(FAIL: expected covered: .*)$')
COMPLETE_RE = re.compile(r'# of unexpected failures')

# config -> (keywords selecting its CI log, allowlist files that config loads)
CONFIGS = {
    "newlib64": (["ubuntu-24.04", "newlib", "rv64gc-lp64d", "gcc"],
                 ["common.log", "newlib.log", "rv64.lp64d.log", "newlib.rv64.lp64d.log"]),
    "newlib32": (["ubuntu-24.04", "newlib", "rv32gc-ilp32d", "gcc"],
                 ["common.log", "newlib.log", "rv32.ilp32d.log", "newlib.rv32.ilp32d.log"]),
    "linux64":  (["ubuntu-24.04", "linux", "rv64gc-lp64d", "gcc"],
                 ["common.log", "glibc.log", "rv64.lp64d.log", "glibc.rv64.lp64d.log"]),
    "linux32":  (["ubuntu-24.04", "linux", "rv32gc-ilp32d", "gcc"],
                 ["common.log", "glibc.log", "rv32.ilp32d.log", "glibc.rv32.ilp32d.log"]),
}


def parse(lines):
    return {l.strip() for l in lines if l.strip() and not l.startswith("#")}


def read_log(keywords):
    """Return (failures, is_complete) for the first *.txt matching all keywords.

    is_complete is True only when the log contains dejagnu's end-of-run summary
    line "# of unexpected failures".  An incomplete log (cancelled job) should
    not participate in the intersection recompute — its entries have already
    been committed into the allowlist files from a previous complete run.
    """
    for f in glob.glob("*.txt"):
        if all(k in f for k in keywords):
            with open(f, encoding="utf-8", errors="replace") as fh:
                lines = fh.readlines()
            failures = {m.group(1) for m in (STATUS_RE.match(l) for l in tqdm(lines, desc=f)) if m}
            # Also capture gcov prime-paths continuation lines
            failures |= {m.group(1) for m in (GCOV_PRIMEPATH_RE.match(l) for l in lines) if m}
            complete = any(COMPLETE_RE.search(l) for l in lines)
            if not complete:
                print(f"  WARNING: {f} looks incomplete (no dejagnu summary); "
                      "skipping its failures from intersection recompute")
            return failures, complete
    return set(), False


def read_allowlist(fname):
    if os.path.exists(fname):
        with open(fname, encoding="utf-8") as fh:
            return parse(fh.readlines())
    return set()


def write(fname, entries):
    if entries:
        with open(fname, "w", encoding="utf-8") as f:
            f.write("\n".join(sorted(entries)) + "\n")
    elif os.path.exists(fname):
        os.remove(fname)


s = {}
for name, (keywords, files) in CONFIGS.items():
    base = set().union(*(read_allowlist(f) for f in files))
    failures, complete = read_log(keywords)
    if complete:
        # Full run: merge new failures and allow intersection to reclassify.
        s[name] = base | failures
    else:
        # Incomplete run (cancelled job): carry the base forward unchanged.
        # Do NOT add the partial failures — they would skew the intersection
        # and scatter entries across the wrong per-file buckets.
        s[name] = base

newlib_rv64 = s["newlib64"]
newlib_rv32 = s["newlib32"]
linux_rv64 = s["linux64"]
linux_rv32 = s["linux32"]

common = newlib_rv64 & newlib_rv32 & linux_rv64 & linux_rv32
rv32 = (newlib_rv32 & linux_rv32) - common
rv64 = (newlib_rv64 & linux_rv64) - common
glibc = (linux_rv32 & linux_rv64) - common
newlib = (newlib_rv64 & newlib_rv32) - common

write("common.log", common)
write("rv32.ilp32d.log", rv32)
write("rv64.lp64d.log", rv64)
write("glibc.log", glibc)
write("newlib.log", newlib)
write("glibc.rv32.ilp32d.log", linux_rv32 - rv32 - glibc - common)
write("glibc.rv64.lp64d.log", linux_rv64 - rv64 - glibc - common)
write("newlib.rv32.ilp32d.log", newlib_rv32 - rv32 - newlib - common)
write("newlib.rv64.lp64d.log", newlib_rv64 - rv64 - newlib - common)
