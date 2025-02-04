#include "hsv.h"
#include <SupportDefs.h>
#include <math.h>
#include "BecassoAddOn.h"  // For HUE_UNDEF

IMPEXP hsv_color
rgb2hsv(rgb_color c)
{
	hsv_color res;
	float r = float(c.red) / 255;
	float g = float(c.green) / 255;
	float b = float(c.blue) / 255;
	float max = max_c(max_c(r, g), b);
	float min = min_c(min_c(r, g), b);
	res.value = max;
	res.saturation = (max != 0.0) ? ((max - min) / max) : 0.0;
	if (res.saturation == 0.0)
		res.hue = HUE_UNDEF;
	else {
		float delta = max - min;
		if (r == max)
			res.hue = (g - b) / delta;
		else if (g == max)
			res.hue = 2.0 + (b - r) / delta;
		else if (b == max)
			res.hue = 4.0 + (r - g) / delta;
		res.hue *= 60.0;
		if (res.hue < 0.0)
			res.hue += 360.0;
	}
	res.alpha = c.alpha;
	return (res);
}

IMPEXP rgb_color
hsv2rgb(hsv_color c)
{
	float r = 0, g = 0, b = 0, h = 0, s = 0, v = 0;
	rgb_color res;
	h = c.hue;
	s = c.saturation;
	v = c.value;
	if (s == 0.0) {
		if (h == HUE_UNDEF) {
			r = v;
			g = v;
			b = v;
		} else {
			//			fprintf (stderr, "Invalid HSV color\n");
			r = v;
			g = v;
			b = v;
		}
	} else {
		float f, p, q, t;
		int i;

		if (h == 360.0)
			h = 0.0;
		h /= 60.0;
		i = int(floor(h));
		f = h - i;
		p = v * (1.0 - s);
		q = v * (1.0 - s * f);
		t = v * (1.0 - s * (1.0 - f));
		switch (i) {
			case 0:
				r = v;
				g = t;
				b = p;
				break;
			case 1:
				r = q;
				g = v;
				b = p;
				break;
			case 2:
				r = p;
				g = v;
				b = t;
				break;
			case 3:
				r = p;
				g = q;
				b = v;
				break;
			case 4:
				r = t;
				g = p;
				b = v;
				break;
			case 5:
				r = v;
				g = p;
				b = q;
				break;
		}
	}
	res.red = uint8(r * 255);
	res.green = uint8(g * 255);
	res.blue = uint8(b * 255);
	res.alpha = c.alpha;
	return (res);
}

// #define USE_OLD_WEIGHTS 1

#ifndef SIMPLE_DISTANCE

IMPEXP float
diff(rgb_color a, rgb_color b)
// Note: Doesn't take alpha channel into account.
// Distance is weighed via 0.213R, 0.715G, and 0.072B.
// (Should actually be: 0.2125R, 0.7154G, and 0.0721B)
// And NOT 0.299R, 0.587G, 0.114B, which is widely used but
// reflects 1953 monitor phosphor characteristics!
{
	static bool lutsfilled = false;
	static long rlut[512];
	static long glut[512];
	static long blut[512];
	static long* irlut = rlut + 256;
	static long* iglut = glut + 256;
	static long* iblut = blut + 256;

	if (lutsfilled) {
		int dr = (int)a.red - b.red;
		int dg = (int)a.green - b.green;
		int db = (int)a.blue - b.blue;
		return (sqrt(irlut[dr] + iglut[dg] + iblut[db]) / 32);	// 32 ~ sqrt(1000); 1% error.
	} else {
		for (int i = 0; i < 512; i++) {
			long j = (i - 256) * (i - 256);
#if defined(USE_OLD_WEIGHTS)
			rlut[i] = 299 * j;
			glut[i] = 587 * j;
			blut[i] = 114 * j;
#else
			rlut[i] = 213 * j;
			glut[i] = 715 * j;
			blut[i] = 072 * j;
#endif
		}
		lutsfilled = true;
		return (diff(a, b));
	}
}

#else

IMPEXP float
cdiff(rgb_color a, rgb_color b)
// Note: Doesn't take alpha channel into account.
{
	return (sqrt((a.red - b.red) * (a.red - b.red) + (a.green - b.green) * (a.green - b.green)
				 + (a.blue - b.blue) * (a.blue - b.blue)));
}

#endif


IMPEXP uchar
clipchar(float x)
{
	return ((x < 0) ? 0 : ((x > 255) ? 255 : uchar(x)));
}

IMPEXP uchar
clipchar(int x)
{
	return ((x < 0) ? 0 : ((x > 255) ? 255 : x));
}

IMPEXP float
clipone(float x)
{
	return ((x < 0) ? 0 : ((x > 1) ? 1 : x));
}

IMPEXP float
clipdegr(float x)
{
	if (x < 0)
		return (clipdegr(x + 360));
	if (x > 360)
		return (clipdegr(x - 360));
	return x;
}

IMPEXP bgra_pixel
cmyk2bgra(cmyk_pixel x)
// Alpha is always set to 255
{
	int r, g, b;
	r = g = b = 255 - BLACK(x);
	b -= YELLOW(x);
	g -= MAGENTA(x);
	r -= CYAN(x);
	return (PIXEL(r, g, b, 255));
}

IMPEXP cmyk_pixel
bgra2cmyk(bgra_pixel x)
// Alpha is lost
{
	int c, m, y, k;
	y = 255 - BLUE(x);
	m = 255 - GREEN(x);
	c = 255 - RED(x);
	k = min_c(min_c(c, m), y);
	y -= k;
	m -= k;
	c -= k;
	return (PIXEL(y, m, c, k));
}

IMPEXP rgb_color
bgra2rgb(bgra_pixel c)
{
	rgb_color res;
	res.blue = BLUE(c);
	res.green = GREEN(c);
	res.red = RED(c);
	res.alpha = ALPHA(c);
	return (res);
}

IMPEXP rgb_color
cmyk2rgb(cmyk_pixel c)
{
	return (bgra2rgb(cmyk2bgra(c)));
}

IMPEXP hsv_color
bgra2hsv(bgra_pixel c)
{
	return (rgb2hsv(bgra2rgb(c)));
}

IMPEXP hsv_color
cmyk2hsv(cmyk_pixel c)
{
	return (rgb2hsv(cmyk2rgb(c)));
}

IMPEXP bgra_pixel
rgb2bgra(rgb_color c)
{
#if defined(__POWERPC__)
	return (((c.blue << 24) & 0xFF000000) | ((c.green << 16) & 0x00FF0000)
			| ((c.red << 8) & 0x0000FF00) | ((c.alpha) & 0x000000FF));
#else
	return (((c.blue) & 0x000000FF) | ((c.green << 8) & 0x0000FF00) | ((c.red << 16) & 0x00FF0000)
			| ((c.alpha << 24) & 0xFF000000));
#endif
}

IMPEXP bgra_pixel
hsv2bgra(hsv_color c)
{
	return (rgb2bgra(hsv2rgb(c)));
}

IMPEXP cmyk_pixel
rgb2cmyk(rgb_color c)
{
	return (bgra2cmyk(rgb2bgra(c)));
}

IMPEXP cmyk_pixel
hsv2cmyk(hsv_color c)
{
	return (bgra2cmyk(hsv2bgra(c)));
}
