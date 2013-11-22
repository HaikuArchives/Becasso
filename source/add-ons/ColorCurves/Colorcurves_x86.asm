; gamma_MMX.asm - assemble with NAsm.

		BITS 32
		GLOBAL gamma_x86
		
		SECTION .text

; table needs to have _four_ tables, shifted in place.
; i.e.	0000 0000 0000 aaaa (256 entries)
;		0000 0000 rrrr 0000 (256 entries)
;		0000 gggg 0000 0000 (256 entries)
;		bbbb 0000 0000 0000 (256 entries)

; prototype gamma_x86 (uint32 *s, uint32 *d, uint32 *table, uint32 count)

gamma_x86:

		push		ebx
		push		ebp

		mov			esi,	[esp + 12]			; source pointer
		mov			edi,	[esp + 16]			; dest pointer
		mov			ebp,	[esp + 20]			; table pointer
		mov			ecx,	[esp + 24]			; # pixels
		
	.loop:
	
		mov			ebx,	[esi]				; get pixel (ARGB)
		mov			eax,	ebx
		mov			edx,	0;
		shr			eax,	24					; eax = 000A
		or			edx,	[ebp+4*eax]			; edx = a000
		mov			eax,	ebx;
		shr			eax,	16					; eax = 00AR
		and			eax,	0xFF;				; eax = 000R
		or			edx,	[ebp+4*eax+1024]	; edx = ar00
		mov			eax,	ebx	
		shr			eax,	8					; eax = 0ARG
		and			eax,	0xFF				; eax = 000G
		or			edx,	[ebp+4*eax+2048]	; edx = arg0
		mov			al,		bl					; eax = 000R
		or			edx,	[ebp+4*eax+3072]	; edx = argb
		
		mov			[edi],	edx					; store result
		add			esi,	4
		add			edi,	4
		
		loop		.loop

		pop			ebp
		pop			ebx
		ret
