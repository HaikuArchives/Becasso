/*
 *  GammaCorrect.cpp 1.1.0 (Oct 24th, 1999)
 *
 *  Image manipulator that makes a 32-bit image brighter
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 */


#include <Message.h>
#include "BitmapAccessor.h"
#include "ImageManipAddon.h"


// Determins the amount of gamma correction
#define GAMMA_FACTOR 8.0


const char addonName[] = "Gamma Correct";
const char addonInfo[] = "Make an image brighter. By Edmund Vermeulen <edmundv@xs4all.nl>.";
const char addonCategory[] = "Color";
int32 addonVersion = 110;


status_t
Manipulate(
	BitmapAccessor *sourceBitmap,
	BMessage *ioExtension,
	bool checkOnly)
{
	// We only support 32-bit bitmaps
	color_space cs = sourceBitmap->ColorSpace();
	if (cs != B_RGB32 && cs != B_RGBA32)
		return B_NOT_ALLOWED;

	if (checkOnly)
		return B_OK;

	// Get pixel size; we only use the first three bytes, and skip the rest
	int pixel_size = (int) sourceBitmap->BytesPerPixel();
	if (pixel_size < 3)
		return B_ERROR;

	// If we can't find the selection rect extension, do the whole bitmap
	BRect sel_rect;
	if (!ioExtension || ioExtension->FindRect("selection_rect", &sel_rect) != B_OK)
		sel_rect = sourceBitmap->Bounds();

	float alpha = pow(2.0, GAMMA_FACTOR);
	float beta = 255.0 / log(255.0 / alpha + 1.0);

	// Create the gamma correction look-up table
	uchar gamma_table[256];
	gamma_table[0] = 0;
	for (int i = 1; i < 256; i++)
		gamma_table[i] = uchar(beta * log(float(i) / alpha + 1.0));

	for (float y = sel_rect.top; y <= sel_rect.bottom; ++y)
	{
		int32 rowBytes;
		uchar *pixel = static_cast<uchar *>(sourceBitmap->AccessBits(
			BRect(sel_rect.left, y, sel_rect.right, y), &rowBytes));

		for (float x = sel_rect.left; x <= sel_rect.right; ++x)
		{
			// Gamma correct
			pixel[0] = gamma_table[pixel[0]];
			pixel[1] = gamma_table[pixel[1]];
			pixel[2] = gamma_table[pixel[2]];
	
			// Go to the next pixel	
			pixel += pixel_size;
		}
	}

	return B_OK;
}


status_t
GetConfigMessage(
	BMessage * /*ioExtension*/,
	BMessage *ioCapability)
{
	// Let them know that we support the selection rect extension
	ioCapability->AddInt32("selection_rect", B_RECT_TYPE);
	return B_OK;
}
