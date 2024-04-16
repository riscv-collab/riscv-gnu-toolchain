#as:
#objdump: -dw
#name: x86-64 APX NDD instructions with evex prefix encoding
#source: x86-64-apx-ndd.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
\s*[a-f0-9]+:\s*62 f4 0d 10 81 d0 34 12 	adc    \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 7c 6c 10 10 f9    	adc    %r15b,%r17b,%r18b
\s*[a-f0-9]+:\s*62 54 6c 10 11 38    	adc    %r15d,\(%r8\),%r18d
\s*[a-f0-9]+:\s*62 c4 3c 18 12 04 07 	adc    \(%r15,%rax,1\),%r16b,%r8b
\s*[a-f0-9]+:\s*62 c4 3d 18 13 04 07 	adc    \(%r15,%rax,1\),%r16w,%r8w
\s*[a-f0-9]+:\s*62 fc 5c 10 83 14 83 11 	adc    \$0x11,\(%r19,%rax,4\),%r20d
\s*[a-f0-9]+:\s*62 54 6d 10 66 c7    	adcx   %r15d,%r8d,%r18d
\s*[a-f0-9]+:\s*62 14 f9 08 66 04 3f 	adcx   \(%r15,%r31,1\),%r8
\s*[a-f0-9]+:\s*62 14 69 10 66 04 3f 	adcx   \(%r15,%r31,1\),%r8d,%r18d
\s*[a-f0-9]+:\s*62 f4 0d 10 81 c0 34 12 	add    \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 d4 fc 10 81 c7 33 44 34 12 	add    \$0x12344433,%r15,%r16
\s*[a-f0-9]+:\s*62 d4 74 10 80 c5 34 	add    \$0x34,%r13b,%r17b
\s*[a-f0-9]+:\s*62 f4 bc 18 81 c0 11 22 33 f4 	add    \$0xfffffffff4332211,%rax,%r8
\s*[a-f0-9]+:\s*62 44 fc 10 01 f8    	add    %r31,%r8,%r16
\s*[a-f0-9]+:\s*62 44 fc 10 01 38    	add    %r31,\(%r8\),%r16
\s*[a-f0-9]+:\s*62 44 f8 10 01 3c c0 	add    %r31,\(%r8,%r16,8\),%r16
\s*[a-f0-9]+:\s*62 44 7c 10 00 f8    	add    %r31b,%r8b,%r16b
\s*[a-f0-9]+:\s*62 44 7c 10 01 f8    	add    %r31d,%r8d,%r16d
\s*[a-f0-9]+:\s*62 44 7d 10 01 f8    	add    %r31w,%r8w,%r16w
\s*[a-f0-9]+:\s*62 5c fc 10 03 07    	add    \(%r31\),%r8,%r16
\s*[a-f0-9]+:\s*62 5c f8 10 03 84 07 90 90 00 00 	add    0x9090\(%r31,%r16,1\),%r8,%r16
\s*[a-f0-9]+:\s*62 44 7c 10 00 f8    	add    %r31b,%r8b,%r16b
\s*[a-f0-9]+:\s*62 44 7c 10 01 f8    	add    %r31d,%r8d,%r16d
\s*[a-f0-9]+:\s*62 fc 5c 10 83 04 83 11 	add    \$0x11,\(%r19,%rax,4\),%r20d
\s*[a-f0-9]+:\s*62 44 fc 10 01 f8    	add    %r31,%r8,%r16
\s*[a-f0-9]+:\s*62 d4 fc 10 81 04 8f 33 44 34 12 	add    \$0x12344433,\(%r15,%rcx,4\),%r16
\s*[a-f0-9]+:\s*62 44 7d 10 01 f8    	add    %r31w,%r8w,%r16w
\s*[a-f0-9]+:\s*62 54 6e 10 66 c7    	adox   %r15d,%r8d,%r18d
\s*[a-f0-9]+:\s*62 5c fc 10 03 c7    	add    %r31,%r8,%r16
\s*[a-f0-9]+:\s*62 44 fc 10 01 f8    	add    %r31,%r8,%r16
\s*[a-f0-9]+:\s*62 14 fa 08 66 04 3f 	adox   \(%r15,%r31,1\),%r8
\s*[a-f0-9]+:\s*62 14 6a 10 66 04 3f 	adox   \(%r15,%r31,1\),%r8d,%r18d
\s*[a-f0-9]+:\s*62 f4 0d 10 81 e0 34 12 	and    \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 7c 6c 10 20 f9    	and    %r15b,%r17b,%r18b
\s*[a-f0-9]+:\s*62 54 6c 10 21 38    	and    %r15d,\(%r8\),%r18d
\s*[a-f0-9]+:\s*62 c4 3c 18 22 04 07 	and    \(%r15,%rax,1\),%r16b,%r8b
\s*[a-f0-9]+:\s*62 c4 3d 18 23 04 07 	and    \(%r15,%rax,1\),%r16w,%r8w
\s*[a-f0-9]+:\s*62 fc 5c 10 83 24 83 11 	and    \$0x11,\(%r19,%rax,4\),%r20d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 47 90 90 90 90 90 	cmova  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 43 90 90 90 90 90 	cmovae -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 42 90 90 90 90 90 	cmovb  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 46 90 90 90 90 90 	cmovbe -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 44 90 90 90 90 90 	cmove  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 4f 90 90 90 90 90 	cmovg  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 4d 90 90 90 90 90 	cmovge -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 4c 90 90 90 90 90 	cmovl  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 4e 90 90 90 90 90 	cmovle -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 45 90 90 90 90 90 	cmovne -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 41 90 90 90 90 90 	cmovno -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 4b 90 90 90 90 90 	cmovnp -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 49 90 90 90 90 90 	cmovns -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 40 90 90 90 90 90 	cmovo  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 4a 90 90 90 90 90 	cmovp  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*67 62 f4 3c 18 48 90 90 90 90 90 	cmovs  -0x6f6f6f70\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*62 f4 f4 10 ff c8    	dec    %rax,%r17
\s*[a-f0-9]+:\s*62 9c 3c 18 fe 0c 27 	dec    \(%r31,%r12,1\),%r8b
\s*[a-f0-9]+:\s*62 b4 b0 10 af 94 f8 09 09 00 00 	imul   0x909\(%rax,%r31,8\),%rdx,%r25
\s*[a-f0-9]+:\s*67 62 f4 3c 18 af 90 09 09 09 00 	imul   0x90909\(%eax\),%edx,%r8d
\s*[a-f0-9]+:\s*62 dc fc 10 ff c7    	inc    %r31,%r16
\s*[a-f0-9]+:\s*62 dc bc 18 ff c7    	inc    %r31,%r8
\s*[a-f0-9]+:\s*62 f4 e4 18 ff c0    	inc    %rax,%rbx
\s*[a-f0-9]+:\s*62 f4 f4 10 f7 d8    	neg    %rax,%r17
\s*[a-f0-9]+:\s*62 9c 3c 18 f6 1c 27 	neg    \(%r31,%r12,1\),%r8b
\s*[a-f0-9]+:\s*62 f4 f4 10 f7 d0    	not    %rax,%r17
\s*[a-f0-9]+:\s*62 9c 3c 18 f6 14 27 	not    \(%r31,%r12,1\),%r8b
\s*[a-f0-9]+:\s*62 f4 0d 10 81 c8 34 12 	or     \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 7c 6c 10 08 f9    	or     %r15b,%r17b,%r18b
\s*[a-f0-9]+:\s*62 54 6c 10 09 38    	or     %r15d,\(%r8\),%r18d
\s*[a-f0-9]+:\s*62 c4 3c 18 0a 04 07 	or     \(%r15,%rax,1\),%r16b,%r8b
\s*[a-f0-9]+:\s*62 c4 3d 18 0b 04 07 	or     \(%r15,%rax,1\),%r16w,%r8w
\s*[a-f0-9]+:\s*62 fc 5c 10 83 0c 83 11 	or     \$0x11,\(%r19,%rax,4\),%r20d
\s*[a-f0-9]+:\s*62 d4 04 10 c0 d4 02 	rcl    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 d0    	rcl    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 10    	rcl    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 f4 04 10 c1 10 02 	rcl    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 10    	rcl    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 14 83 	rcl    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 d4 04 10 c0 dc 02 	rcr    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 d8    	rcr    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 18    	rcr    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 f4 04 10 c1 18 02 	rcr    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 18    	rcr    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 1c 83 	rcr    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 d4 04 10 c0 c4 02 	rol    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 c0    	rol    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 00    	rol    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 f4 04 10 c1 00 02 	rol    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 00    	rol    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 04 83 	rol    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 d4 04 10 c0 cc 02 	ror    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 c8    	ror    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 08    	ror    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 f4 04 10 c1 08 02 	ror    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 08    	ror    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 0c 83 	ror    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 d4 04 10 c0 fc 02 	sar    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 f8    	sar    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 38    	sar    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 f4 04 10 c1 38 02 	sar    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 38    	sar    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 3c 83 	sar    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 f4 0d 10 81 d8 34 12 	sbb    \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 7c 6c 10 18 f9    	sbb    %r15b,%r17b,%r18b
\s*[a-f0-9]+:\s*62 54 6c 10 19 38    	sbb    %r15d,\(%r8\),%r18d
\s*[a-f0-9]+:\s*62 c4 3c 18 1a 04 07 	sbb    \(%r15,%rax,1\),%r16b,%r8b
\s*[a-f0-9]+:\s*62 c4 3d 18 1b 04 07 	sbb    \(%r15,%rax,1\),%r16w,%r8w
\s*[a-f0-9]+:\s*62 fc 5c 10 83 1c 83 11 	sbb    \$0x11,\(%r19,%rax,4\),%r20d
\s*[a-f0-9]+:\s*62 d4 04 10 c0 e4 02 	shl    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 d4 04 10 c0 e4 02 	shl    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 e0    	shl    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 e0    	shl    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 20    	shl    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 20    	shl    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 74 84 10 24 20 01 	shld   \$0x1,%r12,\(%rax\),%r31
\s*[a-f0-9]+:\s*62 74 04 10 24 38 02 	shld   \$0x2,%r15d,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 54 05 10 24 c4 02 	shld   \$0x2,%r8w,%r12w,%r31w
\s*[a-f0-9]+:\s*62 7c bc 18 a5 e0    	shld   %cl,%r12,%r16,%r8
\s*[a-f0-9]+:\s*62 7c 05 10 a5 2c 83 	shld   %cl,%r13w,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 74 05 10 a5 08    	shld   %cl,%r9w,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 f4 04 10 c1 20 02 	shl    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 04 10 c1 20 02 	shl    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 20    	shl    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 f4 05 10 d1 20    	shl    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 24 83 	shl    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 24 83 	shl    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 d4 04 10 c0 ec 02 	shr    \$0x2,%r12b,%r31b
\s*[a-f0-9]+:\s*62 fc 3c 18 d2 e8    	shr    %cl,%r16b,%r8b
\s*[a-f0-9]+:\s*62 f4 04 10 d0 28    	shr    \$1,\(%rax\),%r31b
\s*[a-f0-9]+:\s*62 74 84 10 2c 20 01 	shrd   \$0x1,%r12,\(%rax\),%r31
\s*[a-f0-9]+:\s*62 74 04 10 2c 38 02 	shrd   \$0x2,%r15d,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 54 05 10 2c c4 02 	shrd   \$0x2,%r8w,%r12w,%r31w
\s*[a-f0-9]+:\s*62 7c bc 18 ad e0    	shrd   %cl,%r12,%r16,%r8
\s*[a-f0-9]+:\s*62 7c 05 10 ad 2c 83 	shrd   %cl,%r13w,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 74 05 10 ad 08    	shrd   %cl,%r9w,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 f4 04 10 c1 28 02 	shr    \$0x2,\(%rax\),%r31d
\s*[a-f0-9]+:\s*62 f4 05 10 d1 28    	shr    \$1,\(%rax\),%r31w
\s*[a-f0-9]+:\s*62 fc 05 10 d3 2c 83 	shr    %cl,\(%r19,%rax,4\),%r31w
\s*[a-f0-9]+:\s*62 f4 0d 10 81 e8 34 12 	sub    \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 7c 6c 10 28 f9    	sub    %r15b,%r17b,%r18b
\s*[a-f0-9]+:\s*62 54 6c 10 29 38    	sub    %r15d,\(%r8\),%r18d
\s*[a-f0-9]+:\s*62 c4 3c 18 2a 04 07 	sub    \(%r15,%rax,1\),%r16b,%r8b
\s*[a-f0-9]+:\s*62 c4 3d 18 2b 04 07 	sub    \(%r15,%rax,1\),%r16w,%r8w
\s*[a-f0-9]+:\s*62 fc 5c 10 83 2c 83 11 	sub    \$0x11,\(%r19,%rax,4\),%r20d
\s*[a-f0-9]+:\s*62 f4 0d 10 81 f0 34 12 	xor    \$0x1234,%ax,%r30w
\s*[a-f0-9]+:\s*62 7c 6c 10 30 f9    	xor    %r15b,%r17b,%r18b
\s*[a-f0-9]+:\s*62 54 6c 10 31 38    	xor    %r15d,\(%r8\),%r18d
\s*[a-f0-9]+:\s*62 c4 3c 18 32 04 07 	xor    \(%r15,%rax,1\),%r16b,%r8b
\s*[a-f0-9]+:\s*62 c4 3d 18 33 04 07 	xor    \(%r15,%rax,1\),%r16w,%r8w
\s*[a-f0-9]+:\s*62 fc 5c 10 83 34 83 11 	xor    \$0x11,\(%r19,%rax,4\),%r20d
#pass
