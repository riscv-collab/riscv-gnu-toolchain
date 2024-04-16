# mips64 specific r6 tests - paired LL/SC variants
# mach:  mips64r6
# as:    -mabi=eabi
# ld:    -N -Ttext=0x80010000 -Tdata=0x80020000
# output: *\\npass\\n

  .include "testutils.inc"
  .include "utils-r6.inc"

  .data
  .align 16
test_data:
	.word 0xaaaaaaaa
	.word 0xbbbbbbbb
	.word 0xcccccccc
	.word 0xdddddddd
end_check:
	.byte 0
	.byte 0
	.byte 0
	.byte 0x1

  .text

  setup

  .ent DIAG
DIAG:
  writemsg "[1] Test LLWP"
  llwp	$2, $3, test_data
  checkpair_dword $2, $3, test_data, end_check

  sll $2, $2, 1
  srl $3, $3, 1
  move  $s0, $2

  scwp	$2, $3, test_data
  check32 $2, 1
  checkpair_dword $s0, $3, test_data, end_check
  writemsg "[2] Test SCWP, done"

  writemsg "[3] Test LLDP"
  lldp	$2, $3, test_data
  checkpair_qword $2, $3, test_data, end_check

  dsll $2, $2, 1
  dsrl $3, $3, 1
  move	$s0, $2

  scdp	$2, $3, test_data
  check32 $2, 1
  checkpair_qword $s0, $3, test_data, end_check
  writemsg "[4] Test SCDP, done"

  pass

  .end DIAG
