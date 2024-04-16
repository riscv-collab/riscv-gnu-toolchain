#as: -Os
#objdump: -drw
#name: x86-64 APX NDD optimized encoding
#source: x86-64-apx-ndd-optimize.s

.*: +file format .*


Disassembly of section .text:

0+ <_start>:
\s*[a-f0-9]+:\s*d5 4d 01 f8          	add    %r31,%r8
\s*[a-f0-9]+:\s*62 44 3c 18 00 f8    	add    %r31b,%r8b,%r8b
\s*[a-f0-9]+:\s*d5 4d 01 f8          	add    %r31,%r8
\s*[a-f0-9]+:\s*d5 1d 03 c7          	add    %r31,%r8
\s*[a-f0-9]+:\s*d5 4d 03 38          	add    \(%r8\),%r31
\s*[a-f0-9]+:\s*d5 1d 03 07          	add    \(%r31\),%r8
\s*[a-f0-9]+:\s*49 81 c7 33 44 34 12 	add    \$0x12344433,%r15
\s*[a-f0-9]+:\s*49 81 c0 11 22 33 f4 	add    \$0xfffffffff4332211,%r8
\s*[a-f0-9]+:\s*d5 19 ff c7          	inc    %r31
\s*[a-f0-9]+:\s*62 dc 04 10 fe c7    	inc    %r31b,%r31b
\s*[a-f0-9]+:\s*d5 1c 29 f9          	sub    %r15,%r17
\s*[a-f0-9]+:\s*62 7c 74 10 28 f9    	sub    %r15b,%r17b,%r17b
\s*[a-f0-9]+:\s*62 54 84 18 29 38    	sub    %r15,\(%r8\),%r15
\s*[a-f0-9]+:\s*d5 49 2b 04 07       	sub    \(%r15,%rax,1\),%r16
\s*[a-f0-9]+:\s*d5 19 81 ee 34 12 00 00 	sub    \$0x1234,%r30
\s*[a-f0-9]+:\s*d5 18 ff c9          	dec    %r17
\s*[a-f0-9]+:\s*62 fc 74 10 fe c9    	dec    %r17b,%r17b
\s*[a-f0-9]+:\s*d5 1c 19 f9          	sbb    %r15,%r17
\s*[a-f0-9]+:\s*62 7c 74 10 18 f9    	sbb    %r15b,%r17b,%r17b
\s*[a-f0-9]+:\s*62 54 84 18 19 38    	sbb    %r15,\(%r8\),%r15
\s*[a-f0-9]+:\s*d5 49 1b 04 07       	sbb    \(%r15,%rax,1\),%r16
\s*[a-f0-9]+:\s*d5 19 81 de 34 12 00 00 	sbb    \$0x1234,%r30
\s*[a-f0-9]+:\s*d5 1c 21 f9          	and    %r15,%r17
\s*[a-f0-9]+:\s*62 7c 74 10 20 f9    	and    %r15b,%r17b,%r17b
\s*[a-f0-9]+:\s*4d 23 38             	and    \(%r8\),%r15
\s*[a-f0-9]+:\s*d5 49 23 04 07       	and    \(%r15,%rax,1\),%r16
\s*[a-f0-9]+:\s*d5 11 81 e6 34 12 00 00 	and    \$0x1234,%r30d
\s*[a-f0-9]+:\s*d5 1c 09 f9          	or     %r15,%r17
\s*[a-f0-9]+:\s*62 7c 74 10 08 f9    	or     %r15b,%r17b,%r17b
\s*[a-f0-9]+:\s*4d 0b 38             	or     \(%r8\),%r15
\s*[a-f0-9]+:\s*d5 49 0b 04 07       	or     \(%r15,%rax,1\),%r16
\s*[a-f0-9]+:\s*d5 19 81 ce 34 12 00 00 	or     \$0x1234,%r30
\s*[a-f0-9]+:\s*d5 1c 31 f9          	xor    %r15,%r17
\s*[a-f0-9]+:\s*62 7c 74 10 30 f9    	xor    %r15b,%r17b,%r17b
\s*[a-f0-9]+:\s*4d 33 38             	xor    \(%r8\),%r15
\s*[a-f0-9]+:\s*d5 49 33 04 07       	xor    \(%r15,%rax,1\),%r16
\s*[a-f0-9]+:\s*d5 19 81 f6 34 12 00 00 	xor    \$0x1234,%r30
\s*[a-f0-9]+:\s*d5 1c 11 f9          	adc    %r15,%r17
\s*[a-f0-9]+:\s*62 7c 74 10 10 f9    	adc    %r15b,%r17b,%r17b
\s*[a-f0-9]+:\s*4d 13 38             	adc    \(%r8\),%r15
\s*[a-f0-9]+:\s*d5 49 13 04 07       	adc    \(%r15,%rax,1\),%r16
\s*[a-f0-9]+:\s*d5 19 81 d6 34 12 00 00 	adc    \$0x1234,%r30
\s*[a-f0-9]+:\s*d5 18 f7 d9          	neg    %r17
\s*[a-f0-9]+:\s*62 fc 74 10 f6 d9    	neg    %r17b,%r17b
\s*[a-f0-9]+:\s*d5 18 f7 d1          	not    %r17
\s*[a-f0-9]+:\s*62 fc 74 10 f6 d1    	not    %r17b,%r17b
\s*[a-f0-9]+:\s*67 0f af 90 09 09 09 00 	imul   0x90909\(%eax\),%edx
\s*[a-f0-9]+:\s*d5 aa af 94 f8 09 09 00 00 	imul   0x909\(%rax,%r31,8\),%rdx
\s*[a-f0-9]+:\s*48 0f af d0          	imul   %rax,%rdx
\s*[a-f0-9]+:\s*d5 19 d1 c7          	rol    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 c7    	rol    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 c4 02          	rol    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 c4 02 	rol    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 cf          	ror    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 cf    	ror    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 cc 02          	ror    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 cc 02 	ror    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 d7          	rcl    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 d7    	rcl    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 d4 02          	rcl    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 d4 02 	rcl    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 df          	rcr    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 df    	rcr    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 dc 02          	rcr    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 dc 02 	rcr    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 e7          	shl    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 e7    	shl    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 e4 02          	shl    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 e4 02 	shl    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 e7          	shl    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 e7    	shl    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 e4 02          	shl    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 e4 02 	shl    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 ef          	shr    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 ef    	shr    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 ec 02          	shr    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 ec 02 	shr    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*d5 19 d1 ff          	sar    \$1,%r31
\s*[a-f0-9]+:\s*62 dc 04 10 d0 ff    	sar    \$1,%r31b,%r31b
\s*[a-f0-9]+:\s*49 c1 fc 02          	sar    \$0x2,%r12
\s*[a-f0-9]+:\s*62 d4 1c 18 c0 fc 02 	sar    \$0x2,%r12b,%r12b
\s*[a-f0-9]+:\s*62 74 9c 18 24 20 01 	shld   \$0x1,%r12,\(%rax\),%r12
\s*[a-f0-9]+:\s*4d 0f a4 c4 02       	shld   \$0x2,%r8,%r12
\s*[a-f0-9]+:\s*62 54 bc 18 24 c4 02 	shld   \$0x2,%r8,%r12,%r8
\s*[a-f0-9]+:\s*62 74 b4 18 a5 08    	shld   %cl,%r9,\(%rax\),%r9
\s*[a-f0-9]+:\s*d5 9c a5 e0          	shld   %cl,%r12,%r16
\s*[a-f0-9]+:\s*62 7c 9c 18 a5 e0    	shld   %cl,%r12,%r16,%r12
\s*[a-f0-9]+:\s*62 74 9c 18 2c 20 01 	shrd   \$0x1,%r12,\(%rax\),%r12
\s*[a-f0-9]+:\s*4d 0f ac ec 01       	shrd   \$0x1,%r13,%r12
\s*[a-f0-9]+:\s*62 54 94 18 2c ec 01 	shrd   \$0x1,%r13,%r12,%r13
\s*[a-f0-9]+:\s*62 74 b4 18 ad 08    	shrd   %cl,%r9,\(%rax\),%r9
\s*[a-f0-9]+:\s*d5 9c ad e0          	shrd   %cl,%r12,%r16
\s*[a-f0-9]+:\s*62 7c 9c 18 ad e0    	shrd   %cl,%r12,%r16,%r12
\s*[a-f0-9]+:\s*67 0f 40 90 90 90 90 90 	cmovo  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 41 90 90 90 90 90 	cmovno -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 42 90 90 90 90 90 	cmovb  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 43 90 90 90 90 90 	cmovae -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 44 90 90 90 90 90 	cmove  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 45 90 90 90 90 90 	cmovne -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 46 90 90 90 90 90 	cmovbe -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 47 90 90 90 90 90 	cmova  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 48 90 90 90 90 90 	cmovs  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 49 90 90 90 90 90 	cmovns -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 4a 90 90 90 90 90 	cmovp  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 4b 90 90 90 90 90 	cmovnp -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 4c 90 90 90 90 90 	cmovl  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 4d 90 90 90 90 90 	cmovge -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 4e 90 90 90 90 90 	cmovle -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*67 0f 4f 90 90 90 90 90 	cmovg  -0x6f6f6f70\(%eax\),%edx
\s*[a-f0-9]+:\s*66 0f 38 f6 c3       	adcx   %ebx,%eax
\s*[a-f0-9]+:\s*66 0f 38 f6 c3       	adcx   %ebx,%eax
\s*[a-f0-9]+:\s*62 f4 fd 18 66 c3    	adcx   %rbx,%rax,%rax
\s*[a-f0-9]+:\s*62 74 3d 18 66 c0    	adcx   %eax,%r8d,%r8d
\s*[a-f0-9]+:\s*62 d4 7d 18 66 c7    	adcx   %r15d,%eax,%eax
\s*[a-f0-9]+:\s*67 66 0f 38 f6 04 0a 	adcx   \(%edx,%ecx,1\),%eax
\s*[a-f0-9]+:\s*f3 0f 38 f6 c3       	adox   %ebx,%eax
\s*[a-f0-9]+:\s*f3 0f 38 f6 c3       	adox   %ebx,%eax
\s*[a-f0-9]+:\s*62 f4 fe 18 66 c3    	adox   %rbx,%rax,%rax
\s*[a-f0-9]+:\s*62 74 3e 18 66 c0    	adox   %eax,%r8d,%r8d
\s*[a-f0-9]+:\s*62 d4 7e 18 66 c7    	adox   %r15d,%eax,%eax
\s*[a-f0-9]+:\s*67 f3 0f 38 f6 04 0a 	adox   \(%edx,%ecx,1\),%eax
#pass
