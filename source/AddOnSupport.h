// © 1997-2001 Sum Software

#ifndef ADDONSUPPORT_H
#define ADDONSUPPORT_H

#include "Build.h"
#include <support/SupportDefs.h>
#include <interface/GraphicsDefs.h>
#include <Entry.h>
#include <Bitmap.h>

typedef uint32 bgra_pixel;
typedef uint32 cmyk_pixel;
typedef uint8  grey_pixel;
typedef struct
{
	float hue;			// [0,360>
	float saturation;	// [0,1]
	float value;		// [0,1]
	uint8 alpha;		// [0,255]
} hsv_color;

extern "C" IMPEXP bgra_pixel average4 (bgra_pixel a, bgra_pixel b, bgra_pixel c, bgra_pixel d);
extern "C" IMPEXP bgra_pixel average6 (bgra_pixel a, bgra_pixel b, bgra_pixel c, bgra_pixel d, bgra_pixel e, bgra_pixel f);
extern "C" IMPEXP bgra_pixel average9 (bgra_pixel a, bgra_pixel b, bgra_pixel c, bgra_pixel d, bgra_pixel e, bgra_pixel f, bgra_pixel g, bgra_pixel h, bgra_pixel i);
extern "C" IMPEXP bgra_pixel pixelblend (bgra_pixel d, bgra_pixel s);
extern "C" IMPEXP uint8  clip8 (int32 c);
extern "C" IMPEXP rgb_color highcolor (void);
extern "C" IMPEXP rgb_color lowcolor (void);
extern "C" IMPEXP int32 currentmode (void);
extern "C" IMPEXP rgb_color contrastingcolor (rgb_color a, rgb_color b);
extern "C" IMPEXP rgb_color *highpalette (void);
extern "C" IMPEXP int highpalettesize (void);
extern "C" IMPEXP rgb_color closesthigh (rgb_color a);
extern "C" IMPEXP rgb_color *lowpalette (void);
extern "C" IMPEXP int lowpalettesize (void);
extern "C" IMPEXP rgb_color closestlow (rgb_color a);
extern "C" IMPEXP pattern currentpattern (void);
extern "C" IMPEXP bgra_pixel weighted_average (bgra_pixel a, uint8 wa, bgra_pixel b, uint8 wb);
extern "C" IMPEXP bgra_pixel weighted_average_rgb (rgb_color a, uint8 wa, rgb_color b, uint8 wb);
extern "C" IMPEXP int Scale (BBitmap *src, BBitmap *srcmap, BBitmap *dest, BBitmap *destmap);
extern "C" IMPEXP void AddWithAlpha (BBitmap *src, BBitmap *dest, long x, long y, int strength = 255);
extern "C" IMPEXP void BlendWithAlpha (BBitmap *src, BBitmap *dest, long x, long y, int strength = 255);
extern "C" IMPEXP void CutOrCopy (BBitmap *src, BBitmap *dest, BBitmap *selection, long offx, long offy, bool cut);
extern "C" IMPEXP BBitmap *entry2bitmap (BEntry entry, bool silent = false);
extern "C" IMPEXP rgb_color hsv2rgb (hsv_color c);
extern "C" IMPEXP rgb_color bgra2rgb (bgra_pixel c);
extern "C" IMPEXP rgb_color cmyk2rgb (cmyk_pixel c);
extern "C" IMPEXP hsv_color rgb2hsv (rgb_color c);
extern "C" IMPEXP hsv_color bgra2hsv (bgra_pixel c);
extern "C" IMPEXP hsv_color cmyk2hsv (cmyk_pixel c);
extern "C" IMPEXP bgra_pixel cmyk2bgra (cmyk_pixel c);
extern "C" IMPEXP bgra_pixel rgb2bgra (rgb_color c);
extern "C" IMPEXP bgra_pixel hsv2bgra (hsv_color c);
extern "C" IMPEXP cmyk_pixel bgra2cmyk (bgra_pixel c);
extern "C" IMPEXP cmyk_pixel rgb2cmyk (rgb_color c);
extern "C" IMPEXP cmyk_pixel hsv2cmyk (hsv_color c);
extern "C" IMPEXP float diff (rgb_color a, rgb_color b);
extern "C" IMPEXP uchar clipchar (float x);
extern "C" IMPEXP float clipone (float x);
extern "C" IMPEXP float clipdegr (float x);

#endif
