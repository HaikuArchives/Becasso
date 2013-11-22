#include <Screen.h>
#include <AppDefs.h>
#include <Message.h>
#include "ColorView.h"
#include "RoColor.h"
#include "hsv.h"
#include <stdio.h>

ColorView::ColorView (BRect frame, const char *name, rgb_color c)
: BView (frame, name, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
	SetViewColor (B_TRANSPARENT_32_BIT);
	color_space cs = B_RGB32;
	{
		BScreen screen;
		if (screen.ColorSpace() == B_COLOR_8_BIT)
			cs = B_COLOR_8_BIT;
	}
	BRect bitFrame = frame;
	bitFrame.OffsetTo (B_ORIGIN);
	bitmap = new BBitmap (bitFrame, cs);
	SetColor (c);
}

ColorView::~ColorView ()
{
	delete bitmap;
}

void ColorView::ScreenChanged (BRect /* frame */, color_space cs)
{
	if (bitmap->ColorSpace() == B_COLOR_8_BIT && cs != B_COLOR_8_BIT)
	{
		BRect bitFrame = bitmap->Bounds();
		delete bitmap;
		bitmap = new BBitmap (bitFrame, B_RGB32);
		SetColor (color);
	}
	else if (bitmap->ColorSpace() != B_COLOR_8_BIT && cs == B_COLOR_8_BIT)
	{
		BRect bitFrame = bitmap->Bounds();
		delete bitmap;
		bitmap = new BBitmap (bitFrame, B_COLOR_8_BIT);
		SetColor (color);
	}
}

void ColorView::Draw (BRect /* update */)
{
	DrawBitmap (bitmap, B_ORIGIN);
}

void ColorView::SetColor (rgb_color c)
{
	int h = int (Bounds().Height() + 1);
	int w = int (Bounds().Width() + 1);
	uchar *data = new uchar[h*w*3];
	for (int i = 0; i < h; i++)
		for (int j = 0; j < w; j++)
		{
			data[i*w*3 + j*3] = c.red;
			data[i*w*3 + j*3 + 1] = c.green;
			data[i*w*3 + j*3 + 2] = c.blue;
		}
	bitmap->SetBits (data, h*w*3, 0, B_RGB32);
	color = c;
	Invalidate();
	delete [] data;
}

rgb_color ColorView::GetColor ()
{
	return color;
}

void ColorView::MouseDown (BPoint /* point */)
{
	BBitmap *screenBitmap;
	color_space cs = B_RGB32;
	{
		BScreen screen;
		if (screen.ColorSpace() == B_COLOR_8_BIT)
			cs = B_COLOR_8_BIT;
	}
	screenBitmap = new BBitmap (BRect (0, 0, 15, 15), cs);
	char data[16*16*3];
	for (int i = 0; i < 16*16; i++)
	{
		data[3*i] = color.red;
		data[3*i + 1] = color.green;
		data[3*i + 2] = color.blue;
	}
	screenBitmap->SetBits (data, 16*16*3, 0, B_RGB32);
	roSColor colorData;
	char htmlString[8];
	hsv_color h = rgb2hsv (color);
	colorData.m_Red = float (color.red)/255;
	colorData.m_Green = float (color.green)/255;
	colorData.m_Blue = float (color.blue)/255;
	colorData.m_Alpha = float (color.alpha)/255;
	colorData.m_Hue = float (h.hue)/360;
	sprintf (htmlString, "#%02X%02X%02X", color.red, color.green, color.blue);
	BMessage *drag = new BMessage (B_PASTE);
	drag->AddData ("roColour", RO_COLOR_STRUCT_128_TYPE, &colorData, sizeof (struct roSColor));
	drag->AddData ("RGBColor", B_RGB_COLOR_TYPE, &color, sizeof (struct rgb_color));
	drag->AddData ("text/plain", B_MIME_TYPE, htmlString, 7);
	DragMessage (drag, screenBitmap, B_OP_COPY, BPoint (7, 7));
	delete drag;
}