#include "AddOnSupport.h"
#include "ColorMenuButton.h"
#include "PatternMenuButton.h"
#include "PicMenuButton.h"
#include "hsv.h"

bgra_pixel
average4(bgra_pixel a, bgra_pixel b, bgra_pixel c, bgra_pixel d)
{
	//  This is valid for both IA and PPC
	bgra_pixel r = ((a >> 24) + (b >> 24) + (c >> 24) + (d >> 24)) << 22;
	r += ((a & 0x00FF0000) + (b & 0x00FF0000) + (c & 0x00FF0000) + (d & 0x00FF0000)) / 4;
	r += ((a & 0x0000FF00) + (b & 0x0000FF00) + (c & 0x0000FF00) + (d & 0x0000FF00)) / 4;
	r += ((a & 0x000000FF) + (b & 0x000000FF) + (c & 0x000000FF) + (d & 0x000000FF)) / 4;
	return (r);
}

bgra_pixel
average6(bgra_pixel a, bgra_pixel b, bgra_pixel c, bgra_pixel d, bgra_pixel e, bgra_pixel f)
{
	//  This is valid for both IA and PPC
	bgra_pixel r = (((a >> 24) + (b >> 24) + (c >> 24) + (d >> 24) + (e >> 24) + (f >> 24)) / 6)
				   << 24;
	r += ((a & 0x00FF0000) + (b & 0x00FF0000) + (c & 0x00FF0000) + (d & 0x00FF0000) +
		  (e & 0x00FF0000) + (f & 0x00FF0000)) /
		 6;
	r &= 0xFFFF0000;
	r += ((a & 0x0000FF00) + (b & 0x0000FF00) + (c & 0x0000FF00) + (d & 0x0000FF00) +
		  (e & 0x0000FF00) + (f & 0x0000FF00)) /
		 6;
	r &= 0xFFFFFF00;
	r += ((a & 0x000000FF) + (b & 0x000000FF) + (c & 0x000000FF) + (d & 0x000000FF) +
		  (e & 0x000000FF) + (f & 0x000000FF)) /
		 6;
	return (r);
}

bgra_pixel
average9(
	bgra_pixel a, bgra_pixel b, bgra_pixel c, bgra_pixel d, bgra_pixel e, bgra_pixel f,
	bgra_pixel g, bgra_pixel h, bgra_pixel i
)
{
	//  This is valid for both IA and PPC
	bgra_pixel r = (((a >> 24) + (b >> 24) + (c >> 24) + (d >> 24) + (e >> 24) + (f >> 24) +
					 (g >> 24) + (h >> 24) + (i >> 24)) /
					9)
				   << 24;
	r += ((a & 0x00FF0000) + (b & 0x00FF0000) + (c & 0x00FF0000) + (d & 0x00FF0000) +
		  (e & 0x00FF0000) + (f & 0x00FF0000) + (g & 0x00FF0000) + (h & 0x00FF0000) +
		  (i & 0x00FF0000)) /
		 9;
	r &= 0xFFFF0000;
	r += ((a & 0x0000FF00) + (b & 0x0000FF00) + (c & 0x0000FF00) + (d & 0x0000FF00) +
		  (e & 0x0000FF00) + (f & 0x0000FF00) + (g & 0x0000FF00) + (h & 0x0000FF00) +
		  (i & 0x0000FF00)) /
		 9;
	r &= 0xFFFFFF00;
	r += ((a & 0x000000FF) + (b & 0x000000FF) + (c & 0x000000FF) + (d & 0x000000FF) +
		  (e & 0x000000FF) + (f & 0x000000FF) + (g & 0x000000FF) + (h & 0x000000FF) +
		  (i & 0x000000FF)) /
		 9;
	return (r);
}

bgra_pixel
pixelblend(bgra_pixel d, bgra_pixel s)
{
	bgra_pixel res;
#if defined(__POWERPC__)
	int sa = s & 0xFF;
	int da = 255 - sa;
	int ta = 255;
	if (sa == 255 || !(d & 0xFF)) // Fully opaque
	{
		res = s;
	} else if (sa == 0) // Fully transparent
	{
		res = d;
	} else {
		res =
			((((d & 0xFF000000) / ta) * da + ((s & 0xFF000000) / ta) * sa) & 0xFF000000) |
			((((d & 0x00FF0000) / ta) * da + ((s & 0x00FF0000) / ta) * sa) & 0x00FF0000) |
			((((d & 0x0000FF00) * da + (s & 0x0000FF00) * sa) / ta) & 0x0000FF00) |
			//		res = (((((d >> 24)*da + (s >> 24)*sa)/ta) << 24) & 0xFF000000) |
			//			 ((((((d >> 16) & 0xFF)*da + (s >> 16) & 0xFF)*sa) << 16) & 0x00FF0000) |
			//			 ((((((d >>  8) & 0xFF)*da + (s >>  8) & 0xFF)*sa) <<  8) & 0x0000FF00) |
			clipchar(sa + int(d & 0xFF));
	}
#else
	int sa = s >> 24;
	int da = 255 - sa;
	int ta = 255;
	if (sa == 255 || !(d >> 24)) // Fully opaque
	{
		res = s;
	} else if (sa == 0) // Fully transparent
	{
		res = d;
	} else {
		res = ((((d & 0x00FF0000) * da + (s & 0x00FF0000) * sa) / ta) & 0x00FF0000) |
			  ((((d & 0x0000FF00) * da + (s & 0x0000FF00) * sa) / ta) & 0x0000FF00) |
			  ((((d & 0x000000FF) * da + (s & 0x000000FF) * sa) / ta) & 0x000000FF) |
			  (clipchar(sa + int(d >> 24)) << 24);
	}
#endif
	return res;
}

uint8
clip8(int32 c)
{
	return ((c < 0) ? 0 : (c > 255) ? 255 : c);
}

rgb_color
highcolor(void)
{
	extern ColorMenuButton* hicolor;
	return hicolor->color();
}

rgb_color
lowcolor(void)
{
	extern ColorMenuButton* locolor;
	return locolor->color();
}

int32
currentmode(void)
{
	extern PicMenuButton* mode;
	return mode->selected();
}

pattern
currentpattern(void)
{
	extern PatternMenuButton* pat;
	return pat->pat();
}

rgb_color*
highpalette(void)
{
	extern ColorMenuButton* hicolor;
	return hicolor->palette();
}

rgb_color*
lowpalette(void)
{
	extern ColorMenuButton* locolor;
	return locolor->palette();
}

int
highpalettesize(void)
{
	extern ColorMenuButton* hicolor;
	return hicolor->numColors();
}

int
lowpalettesize(void)
{
	extern ColorMenuButton* locolor;
	return locolor->numColors();
}

rgb_color
closesthigh(rgb_color a)
{
	extern ColorMenuButton* hicolor;
	return hicolor->getClosest(a);
}

rgb_color
closestlow(rgb_color a)
{
	extern ColorMenuButton* locolor;
	return locolor->getClosest(a);
}

rgb_color
contrastingcolor(rgb_color a, rgb_color b)
{
	if ((a.red > a.green + 100 && a.red > a.blue + 100) ||
		(b.red > b.green + 100 && b.red > b.blue + 100)) // "one is sort of red"
	{
		if (a.red + a.green + a.blue + b.red + b.green + b.blue > 768) // light colors
			return (Black);
		else
			return (White);
	} else
		return (Red);
}

bgra_pixel
weighted_average(bgra_pixel a, uint8 wa, bgra_pixel b, uint8 wb)
{
	//  This is valid for both IA and PPC
	uint32 t = wa + wb;
	//	return ((((((a & 0xFF000000)/t)*wa + (b & 0xFF000000)/t)*wb) & 0xFF000000) |
	//			(((((a & 0x00FF0000)/t)*wa + (b & 0x00FF0000)/t)*wb) & 0x00FF0000) |
	//			 ((((a & 0x0000FF00)*wa + (b & 0x0000FF00)*wb)/t) & 0x0000FF00) |
	//			 ((((a & 0x000000FF)*wa + (b & 0x000000FF)*wb)/t) & 0x000000FF));
	return (
		(((((a >> 24) * wa + (b >> 24) * wb) / t) << 24) & 0xFF000000) |
		((((((a >> 16) & 0xFF) * wa + ((b >> 16) & 0xFF) * wb) / t) << 16) & 0x00FF0000) |
		((((((a >> 8) & 0xFF) * wa + ((b >> 8) & 0xFF) * wb) / t) << 8) & 0x0000FF00) |
		(((a & 0xFF) * wa + (b & 0xFF) * wb) / t & 0x000000FF)
	);
}

bgra_pixel
weighted_average_rgb(rgb_color ca, uint8 wa, rgb_color cb, uint8 wb)
{
#if defined(__POWERPC__)
	bgra_pixel a = ca.blue << 24 | ca.green << 16 | ca.red << 8;
	bgra_pixel b = cb.blue << 24 | cb.green << 16 | cb.red << 8;
#else
	bgra_pixel a = ca.red << 16 | ca.green << 8 | ca.blue;
	bgra_pixel b = cb.red << 16 | cb.green << 8 | cb.blue;
#endif
	return (weighted_average(a, wa, b, wb));
}
