#ifndef BRUSH_H
#define BRUSH_H

#include <Bitmap.h>

class Brush {
public:
	Brush(int h, int w, float s);
	~Brush();
	void Set(int x, int y, uchar v);
	uchar Get(int x, int y);

	int Height() { return fHeight; };

	int Width() { return fWidth; };

	float Spacing() { return fSpacing; };

	rgb_color ToColor(int x, int y, rgb_color src, rgb_color dst);
	BBitmap* ToBitmap(rgb_color hi);
	BBitmap* ToBitmap(rgb_color hi, rgb_color lo);

	uchar* Data() { return data; };

private:
	int fHeight;
	int fWidth;
	float fSpacing;
	uchar* data;
	BBitmap* bitmap;
};

#endif