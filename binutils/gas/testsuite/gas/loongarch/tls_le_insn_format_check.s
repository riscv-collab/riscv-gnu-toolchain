/* Assemble the following assembly statements using as.

   The test case is mainly to check the format of tls le type
   symbolic address fetch instruction.Because in tls le symbolic
   address acquisition, there will be a special add.d instruction,
   which has four operands (add.d op1,op2,op3,op4),the first three
   operands are registers, and the last operand is a relocation,
   we need to format check the fourth operand.If it is not a correct
   relocation type operand, we need to throw the relevant exception
   message.

   if a "no match insn" exception is thrown, the test passes;
   otherwise, the test fails.  */

add.d $a0,$a0,$a0,8
