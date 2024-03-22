#include "mmx.h"
#include <SupportDefs.h>
#include <stdio.h>

#if defined(__MWCC__)
// It is horrible, but GCC has a completely different assembly syntax.
// We'll have to use an external module (mmx_gcc.asm)

int
mmx_available(void)
{
#if defined(__INTEL__)
	int available;
	int* aPtr = &available;

	asm
	{
		pusha
		mov		edi,	aPtr // This is where we'll store the result
		mov		eax,	1 // request for feature flags
		cpuid // check...
		test	edx,	0x00800000 // test for MMX
		jnz		present // it's there
		mov		[edi],	0 // say it ain't so
		jmp		end
	present:
		mov		[edi],	1 // rejoice!
	end:
		popa
	}

	return available;
#else
	return false;
#endif
}
#endif // defined (__MWCC__)


#if defined(__GNUC__) && defined(__INTEL__)
void
mmx_alpha_blend_diff_ga(uint64 ga_64, uint32* s, uint32* d, int height, int difference, int width);
void
mmx_alpha_blend_nodiff_ga(uint64 ga_64, uint32* s, uint32* d, int size);
void
mmx_alpha_blend_diff(uint32* s, uint32* d, int height, int difference, int width);
void
mmx_alpha_blend_nodiff(uint32* s, uint32* d, int size);
#endif
#if defined(__GNUC__) && defined(__x86_64__)
void
mmx_alpha_blend_diff_ga(uint64 ga_64, uint32* s, uint32* d, int height, int difference, int width){
};
void
mmx_alpha_blend_nodiff_ga(uint64 ga_64, uint32* s, uint32* d, int size){};
void
mmx_alpha_blend_diff(uint32* s, uint32* d, int height, int difference, int width){};
void
mmx_alpha_blend_nodiff(uint32* s, uint32* d, int size){};
#endif

void
mmx_alpha_blend(bgra_pixel* src, bgra_pixel* dest, int ga, int w, int l, int t, int r, int b)
// src and dest point to 32 bit pixel bitmaps, of equal width (w).
// the rectangle (l,t,r,b) is blended onto dest.
{
#if defined(__INTEL__)
	bgra_pixel *s, *d;
	int width, height;
	int size, difference;
	uint64 ga_64, ga_s64;

	// Setup s and d so that they point to the upper left pixel of the
	// rectangle to be blended.
	s = src + t * w + l;
	d = dest + t * w + l;

	// Compute the width and height of the rectangle to blend,
	// as well as the actual number of pixels
	width = r - l + 1;
	height = b - t + 1;
	size = width * height;
	difference = 4 * (w - width);

	// Precompute ga_64 into 00ga 00ga 00ga 00ga
	ga_s64 = ga;
	ga_64 = (ga_s64 << 48) | (ga_s64 << 32) | (ga_s64 << 16) | ga_s64;

	/* printf ("d: %d; l: %d t: %d r: %d b: %d; w: %d; width = %d, height = %d\n",
		difference, l, t, r, b, w, width, height);*/

	// Here comes the dirty work.
	if (difference) {
#if defined(__MWCC__)
		uint64 ff_64 = 0x00FF00FF00FF00FFLL;
		uint64 z_64 = 0x0000000000000000LL;
		asm
		{
			pusha
			mov			esi,	s // source pointer
			mov			edi,	d // dest pointer
			mov			ebx,	height // number of rows
			mov			edx,	difference // "makeup"
			movq		mm0,	ga_64 // mm0 = 00ga 00ga 00ga 00ga
			movq		mm1,	ff_64 // mm1 = 00FF 00FF 00FF 00FF

		fullloop:
			mov			ecx,	width // # pixels per row

		rowloop:
			mov			eax,	[esi] // src pixel

		  // Prepare alpha vector, source, and dest pixels
		  // The strange "interleaving" is to facilitate pairing (faster)
			movd		mm4,	[edi] // mm4 = 0000 0000 dadr dgdb
			shr			eax,	16 // eax = 0000 sasr
			movd		mm3,	[esi] // mm3 = 0000 0000 sasr sgsb
			mov			al,		ah // eax = 0000 sasa
			punpcklbw	mm3,	mm7 // mm3 = sa00 sr00 sg00 sb00
			shl			eax,	8 // eax = 00sa sa00
			punpcklbw	mm4,	mm7 // mm4 = da00 dr00 dg00 db00
			mov			al,		ah // eax = 00sa sasa
			movq		mm5,	mm1 // mm5 = 00FF 00FF 00FF 00FF
			shl			eax,	8 // eax = sasa sa00
			mov			al,		ah // eax = sasa sasa
			movd		mm2,	eax // mm6 = 0000 0000 sasa sasa
			punpcklbw	mm2,	mm7 // mm2 = 00sa 00sa 00sa 00sa
			pmullw		mm2,	mm0 // mm2 = global_alpha*local_alpha
			psrlw		mm2,	8 // mm2 = ga*la/255 = alpha
			pmullw		mm3,	mm2 // mm3 = src*alpha
			psubw		mm5,	mm2 // mm5 = 1 - alpha

			  // Calculate alpha*src + (255 - alpha)*dest
			pmullw		mm4,	mm5 // mm4 = dest*(255 - alpha)
			paddw		mm3,	mm4 // mm3 = 255*result
			psrlw		mm3,	8 // mm3 ~ result

		  // Store the resulting pixel
			packuswb	mm3,	mm3 // mm3 = xxxx xxxx rarr rgrb

		opaque:
			movd		[edi],	mm3 // store result
			
			add			edi,	4
			add			esi,	4
			loop		rowloop // Finish this row
			
			add			esi,	edx
			add			edi,	edx // Add the Makeup
			dec			ebx // More rows?
			jnz			fullloop // do them...
			
		end:
			emms
			popa
		}
#else
		if (ga == 255)
			mmx_alpha_blend_diff(s, d, height, difference, width);
		else
			mmx_alpha_blend_diff_ga(ga_64, s, d, height, difference, width);
#endif
	}
	else {
#if defined(__MWCC__)
		uint64 ff_64 = 0x00FF00FF00FF00FFLL;
		uint64 z_64 = 0x0000000000000000LL;
		asm
		{
			pusha
			mov			esi,	s // source pointer
			mov			edi,	d // dest pointer
			mov			ecx,	size // total # of pixels
			movq		mm0,	ga_64 // mm0 = 00ga 00ga 00ga 00ga
			movq		mm1,	ff_64 // mm1 = 00FF 00FF 00FF 00FF
			movq		mm7,	z_64 // mm7 = 0000 0000 0000 0000

		fullloop2:
			mov			eax,	[esi] // src pixel

		  // Prepare alpha vector, source, and dest pixels
		  // The strange "interleaving" is to facilitate pairing (faster)
			movd		mm4,	[edi] // mm4 = 0000 0000 dadr dgdb
			shr			eax,	16 // eax = 0000 sasr
			movd		mm3,	[esi] // mm3 = 0000 0000 sasr sgsb
			mov			al,		ah // eax = 0000 sasa
			punpcklbw	mm3,	mm7 // mm3 = sa00 sr00 sg00 sb00
			shl			eax,	8 // eax = 00sa sa00
			punpcklbw	mm4,	mm7 // mm4 = da00 dr00 dg00 db00
			mov			al,		ah // eax = 00sa sasa
			movq		mm5,	mm1 // mm5 = 00FF 00FF 00FF 00FF
			movd		mm2,	eax // mm6 = 0000 0000 00sa sasa
			punpcklbw	mm2,	mm7 // mm2 = 0000 00sa 00sa 00sa
			pmullw		mm2,	mm0 // mm2 = global_alpha*local_alpha
			psrlw		mm2,	8 // mm2 = ga*la/255 = alpha
			pmullw		mm3,	mm2 // mm3 = src*alpha
			psubw		mm5,	mm2 // mm5 = 1 - alpha

			  // Calculate alpha*src + (255 - alpha)*dest
			pmullw		mm4,	mm5 // mm4 = dest*(255 - alpha)
			paddw		mm3,	mm4 // mm3 = 255*result
			psrlw		mm3,	8 // mm3 ~ result

		  // Store the resulting pixel
			packuswb	mm3,	mm3 // mm3 = xxxx xxxx rarr rgrb
		opaque2:
			movd		[edi],	mm3 // store result
			
			add			edi,	4
			add			esi,	4
			loop		fullloop2 // Next pixel
			
		end2:
			emms
			popa
		}
#else
		if (ga == 255)
			mmx_alpha_blend_nodiff(s, d, size);
		else
			mmx_alpha_blend_nodiff_ga(ga_64, s, d, size);
#endif
	}
#else
	fprintf(stderr, "MMX code called on non-Intel CPU?!\n");
#endif
}
