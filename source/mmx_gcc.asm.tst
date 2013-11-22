; mmx.asm - assemble with NAsm.
		
		BITS 32
		GLOBAL mmx_available
		GLOBAL mmx_alpha_blend_diff
		GLOBAL mmx_alpha_blend_nodiff
		GLOBAL mmx_alpha_blend_diff_ga
		GLOBAL mmx_alpha_blend_nodiff_ga
		
		SECTION .text

; prototype: int mmx_available (void)

mmx_available:

		pusha
		mov		eax,	1			; request for feature flags
		cpuid						; check...
		test	edx,	0x00800000	; test for MMX
		jnz		present				; it's there
		popa
		mov		eax,	0			; say it ain't so
		ret

	present:
		popa
		mov		eax,	1			; rejoice!
		ret

; prototype: void mmx_alpha_blend_diff_ga (
;		uint64 ga_64, uint64 ff_64, 
;		int32 *s, int32 *d, 
;		int height, int difference, int width)

mmx_alpha_blend_diff_ga:

		push		ebp
		mov			ebp,	esp

		movq		mm0,	[ebp + 8]	; mm0 = 00ga 00ga 00ga 00ga
		movq		mm1,	[ebp + 16]	; mm1 = 00FF 00FF 00FF 00FF
		mov			esi,	[ebp + 24]	; source pointer
		mov			edi,	[ebp + 28]	; dest pointer
		mov			ebx,	[ebp + 32]	; number of rows
		mov			edx,	[ebp + 36]	; "makeup"

	fullloop:
		mov			ecx,	[ebp + 40]	; # pixels per row

	rowloop:
		mov			eax,	[esi]		; src pixel
		
		; Prepare alpha vector, source, and dest pixels
		; The strange "interleaving" is to facilitate pairing (faster)
		shr			eax,	16			; eax = 0000 sasr
		cmp			ah,		0			; Fully transparent?
		jz			transparent			; Skip the blending
		movd		mm3,	[esi]		; mm3 = 0000 0000 sasr sgsb
		mov			al,		ah			; eax = 0000 sasa
		movd		mm4,	[edi]		; mm4 = 0000 0000 dadr dgdb
		punpcklbw	mm3,	mm7			; mm3 = sa00 sr00 sg00 sb00
		shl			eax,	8			; eax = 00sa sa00
		punpcklbw	mm4,	mm7			; mm4 = da00 dr00 dg00 db00
		mov			al,		ah			; eax = 00sa sasa
		movq		mm5,	mm1			; mm5 = 00FF 00FF 00FF 00FF
		shl			eax,	8			; eax = sasa sa00
		mov			al,		ah			; eax = sasa sasa
		movd		mm2,	eax			; mm6 = 0000 0000 sasa sasa
		punpcklbw	mm2,	mm7			; mm2 = 00sa 00sa 00sa 00sa
		pmullw		mm2,	mm0			; mm2 = global_alpha*local_alpha
		psrlw		mm2,	8			; mm2 = ga*la/255 = alpha
		pmullw		mm3,	mm2			; mm3 = src*alpha
		psubw		mm5,	mm2			; mm5 = 1 - alpha
		
		; Calculate alpha*src + (255 - alpha)*dest
		pmullw		mm4,	mm5			; mm4 = dest*(255 - alpha)
		paddw		mm3,	mm4			; mm3 = 255*result
		psrlw		mm3,	8			; mm3 ~ result
		
		; Store the resulting pixel
		packuswb	mm3,	mm3			; mm3 = xxxx xxxx rarr rgrb

		movd		[edi],	mm3			; store result

	transparent:
		add			edi,	4
		add			esi,	4
		loop		rowloop				; Finish this row
		
		add			esi,	edx
		add			edi,	edx			; Add the Makeup
		dec			ebx					; More rows?
		jnz			fullloop			; do them...
		
	end:
		emms
		mov			esp,	ebp
		pop			ebp
		ret

; prototype: void mmx_alpha_blend_nodiff_ga (
;		uint64 ga_64, uint64 ff_64, uint64 z_64,
;		int32 *s, int32 *d,
;		int size)

mmx_alpha_blend_nodiff_ga:

		push		ebp
		mov			ebp,	esp

		movq		mm0,	[ebp + 8]	; mm0 = 00ga 00ga 00ga 00ga
		movq		mm1,	[ebp + 16]	; mm1 = 00FF 00FF 00FF 00FF
		movq		mm7,	[ebp + 24]	; mm7 = 0000 0000 0000 0000
		mov			esi,	[ebp + 32]	; source pointer
		mov			edi,	[ebp + 36]	; dest pointer
		mov			ecx,	[ebp + 40]	; total # of pixels

	fullloop2:
		mov			eax,	[esi]		; src pixel
		
		; Prepare alpha vector, source, and dest pixels
		; The strange "interleaving" is to facilitate pairing (faster)
		shr			eax,	16			; eax = 0000 sasr
		cmp			ah,		0			; Fully transparent?
		jz			transparent2		; Skip the blending
		movd		mm3,	[esi]		; mm3 = 0000 0000 sasr sgsb
		mov			al,		ah			; eax = 0000 sasa
		movd		mm4,	[edi]		; mm4 = 0000 0000 dadr dgdb
		punpcklbw	mm3,	mm7			; mm3 = sa00 sr00 sg00 sb00
		shl			eax,	8			; eax = 00sa sa00
		punpcklbw	mm4,	mm7			; mm4 = da00 dr00 dg00 db00
		mov			al,		ah			; eax = 00sa sasa
		movq		mm5,	mm1			; mm5 = 00FF 00FF 00FF 00FF
		movd		mm2,	eax			; mm6 = 0000 0000 00sa sasa
		punpcklbw	mm2,	mm7			; mm2 = 0000 00sa 00sa 00sa
		pmullw		mm2,	mm0			; mm2 = global_alpha*local_alpha
		psrlw		mm2,	8			; mm2 = ga*la/255 = alpha
		pmullw		mm3,	mm2			; mm3 = src*alpha
		psubw		mm5,	mm2			; mm5 = 1 - alpha
		
		; Calculate alpha*src + (255 - alpha)*dest
		pmullw		mm4,	mm5			; mm4 = dest*(255 - alpha)
		paddw		mm3,	mm4			; mm3 = 255*result
		psrlw		mm3,	8			; mm3 ~ result
		
		; Store the resulting pixel
		packuswb	mm3,	mm3			; mm3 = xxxx xxxx rarr rgrb

		movd		[edi],	mm3			; store result
	
	transparent2:	
		add			edi,	4
		add			esi,	4
		loop		fullloop2			; Next pixel
		
	end2:
		emms
		mov			esp,	ebp
		pop			ebp
		ret


; Optimized cases for when global alpha == 255
; (Otherwise, GCC generated, non-MMX code is about 15% _faster_ :-| )

; prototype: void mmx_alpha_blend_diff (
;		uint64 ff_64, 
;		int32 *s, int32 *d, 
;		int height, int difference, int width)

mmx_alpha_blend_diff:

		push		ebp
		mov			ebp,	esp

		movq		mm1,	[ebp + 8]	; mm1 = 00FF 00FF 00FF 00FF
		mov			esi,	[ebp + 16]	; source pointer
		mov			edi,	[ebp + 20]	; dest pointer
		mov			ebx,	[ebp + 24]	; number of rows
		mov			edx,	[ebp + 28]	; "makeup"

	fullloopnga:
		mov			ecx,	[ebp + 32]	; # pixels per row

	rowloopnga:
		mov			eax,	[esi]		; src pixel
		
		; Prepare alpha vector, source, and dest pixels
		; The strange "interleaving" is to facilitate pairing (faster)
		shr			eax,	16			; eax = 0000 sasr
		cmp			ah,		0			; Fully transparent
		jz			transparentnga		; Skip the blending
		movd		mm3,	[esi]		; mm3 = 0000 0000 sasr sgsb
		cmp			ah,		255			; Fully opaque
		jz			opaquenga			; Also skip the blending
		movd		mm4,	[edi]		; mm4 = 0000 0000 dadr dgdb
		mov			al,		ah			; eax = 0000 sasa
		punpcklbw	mm3,	mm7			; mm3 = sa00 sr00 sg00 sb00
		shl			eax,	8			; eax = 00sa sa00
		punpcklbw	mm4,	mm7			; mm4 = da00 dr00 dg00 db00
		mov			al,		ah			; eax = 00sa sasa
		movq		mm5,	mm1			; mm5 = 00FF 00FF 00FF 00FF
		shl			eax,	8			; eax = sasa sa00
		mov			al,		ah			; eax = sasa sasa
		movd		mm2,	eax			; mm6 = 0000 0000 sasa sasa
		punpcklbw	mm2,	mm7			; mm2 = 00sa 00sa 00sa 00sa

		pmullw		mm3,	mm2			; mm3 = src*alpha
		psubw		mm5,	mm2			; mm5 = 1 - alpha
		
		; Calculate alpha*src + (255 - alpha)*dest
		pmullw		mm4,	mm5			; mm4 = dest*(255 - alpha)
		paddw		mm3,	mm4			; mm3 = 255*result
		psrlw		mm3,	8			; mm3 ~ result
		
		; Store the resulting pixel
		packuswb	mm3,	mm3			; mm3 = xxxx xxxx rarr rgrb

	opaquenga:
		movd		[edi],	mm3			; store result
	
	transparentnga:	
		add			edi,	4
		add			esi,	4
		loop		rowloopnga			; Finish this row
		
		add			esi,	edx
		add			edi,	edx			; Add the Makeup
		dec			ebx					; More rows?
		jnz			fullloopnga			; do them...
		
	endnga:
		emms
		mov			esp,	ebp
		pop			ebp
		ret

; prototype: void mmx_alpha_blend_nodiff (
;		uint64 ff_64, uint64 z_64,
;		int32 *s, int32 *d,
;		int size)

mmx_alpha_blend_nodiff:

		push		ebp
		mov			ebp,	esp

		movq		mm1,	[ebp + 8]	; mm1 = 00FF 00FF 00FF 00FF
		movq		mm7,	[ebp + 16]	; mm7 = 0000 0000 0000 0000
		mov			esi,	[ebp + 24]	; source pointer
		mov			edi,	[ebp + 28]	; dest pointer
		mov			ecx,	[ebp + 32]	; total # of pixels

	fullloop2nga:
		mov			eax,	[esi]		; src pixel
		
		; Prepare alpha vector, source, and dest pixels
		; The strange "interleaving" is to facilitate pairing (faster)
		shr			eax,	24			; eax = 0000 00sa
		cmp			al,		0			; Fully transparent?
		jz			transparent2nga		; Skip the blending
		movd		mm3,	[esi]		; mm3 = 0000 0000 sasr sgsb
		cmp			al,		255			; Fully opaque?
		jz			opaque2nga			; Also skip the blending!
		movd		mm4,	[edi]		; mm4 = 0000 0000 dadr dgdb
		movd		mm2,	eax			; mm2 = 0000 0000 0000 00sa
		punpcklwd	mm2,	mm2			; mm2 = 0000 0000 00sa 00sa
		punpcklwd	mm2,	mm2			; mm2 = 00sa 00sa 00sa 00sa
		punpcklbw	mm3,	mm7			; mm3 = sa00 sr00 sg00 sb00
		punpcklbw	mm4,	mm7			; mm4 = da00 dr00 dg00 db00
		movq		mm5,	mm1			; mm5 = 00FF 00FF 00FF 00FF
		pmullw		mm3,	mm2			; mm3 = src*alpha
		psubw		mm5,	mm2			; mm5 = 1 - alpha
		
		; Calculate alpha*src + (255 - alpha)*dest
		pmullw		mm4,	mm5			; mm4 = dest*(255 - alpha)
		paddw		mm3,	mm4			; mm3 = 255*result
		psrlw		mm3,	8			; mm3 ~ result
		
		; Store the resulting pixel
		packuswb	mm3,	mm3			; mm3 = xxxx xxxx rarr rgrb
	opaque2nga:
		movd		[edi],	mm3			; store result
	
	transparent2nga:	
		add			edi,	4
		add			esi,	4
		loop		fullloop2nga		; Next pixel
		
	end2nga:
		emms
		mov			esp,	ebp
		pop			ebp
		ret
				