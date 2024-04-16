# Check error for MOVDIR64B 32-bit instructions

	.text
_start:
	movdir64b (%si),%eax
	movdir64b (%esi),%ax

	.intel_syntax noprefix
	movdir64b eax,[si]
	movdir64b ax,[esi]
