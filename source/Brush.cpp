#include "Brush.h"
#include "hsv.h"	// For the clipchar()

Brush::Brush (int h, int w, float s)
{
	fHeight = h;
	fWidth = w;
	fSpacing = s;
	data = new unsigned char [(w + 1)*(h + 1)];	// Yes I know this is 1 off.
	for (int i = 0; i < (w + 1)*(h + 1); i++)	// So much to do, so little time :-)
		data[i] = 0;
	bitmap = NULL;
}

Brush::~Brush ()
{
	delete [] data;
	if (bitmap)
		delete bitmap;
}

void Brush::Set (int x, int y, uchar v)
{
	if (x >= 0 && x < fWidth && y >= 0 && y < fHeight)
		data[fWidth*y + x] = v;
}

uchar Brush::Get (int x, int y)
{
	if (x >= 0 && x < fWidth && y >= 0 && y < fHeight)
		return (data[fWidth*y + x]);
	else
		return 0;
}

rgb_color Brush::ToColor (int x, int y, rgb_color src, rgb_color dst)
{
	dst.red = clipchar (dst.red - (255 - src.red)*Get (x, y)/255);
	dst.green = clipchar (dst.green - (255 - src.green)*Get (x, y)/255);
	dst.blue = clipchar (dst.blue - (255 - src.blue)*Get (x, y)/255);
	return (dst);
}

BBitmap *Brush::ToBitmap (rgb_color hi)
{
// Note: A brush map contains alpha = opacity.
	if (!bitmap)
		bitmap = new BBitmap (BRect (0, 0, fWidth - 1, fHeight - 1), B_RGBA32);
	uchar *cdata = (uchar *) bitmap->Bits();
	uchar *pdata = data - 1;
//	long bpr = bitmap->BytesPerRow();
	cdata--;
	int maxopacity = hi.alpha*255/256;	// Hack to fix black center pixel bug
	for (int j = 0; j < fHeight; j++)
		for (int i = 0; i < fWidth; i++)
		{
//			pdata = (cdata + j*bpr + 4*i);
			*(++cdata) = hi.blue;
			*(++cdata) = hi.green;
			*(++cdata) = hi.red;
			*(++cdata) = *(++pdata)*maxopacity/255;
		}
	return bitmap;
}

BBitmap *Brush::ToBitmap (rgb_color hi, rgb_color lo)
{
	if (!bitmap)
		bitmap = new BBitmap (BRect (0, 0, fWidth - 1, fHeight - 1), B_RGBA32);
	uchar *cdata = (uchar *) bitmap->Bits() - 1;
//	long bpr = bitmap->BytesPerRow();
	for (int j = 0; j < fHeight; j++)
		for (int i = 0; i < fWidth; i++)
		{
			int f = data[fWidth*j + i];
			*(++cdata) = (f*hi.blue + (255 - f)*lo.blue)/255;
			*(++cdata) = (f*hi.green + (255 - f)*lo.green)/255;
			*(++cdata) = (f*hi.red + (255 - f)*lo.red)/255;
			*(++cdata) = 255;
		}
	return bitmap;
}