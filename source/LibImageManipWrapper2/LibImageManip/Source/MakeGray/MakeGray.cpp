/*
 *  MakeGray.cpp 1.5.0 (Oct 24th, 1999)
 *
 *  Image manipulator that makes a 32-bit image grayscale
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 */


#include <Message.h>
#include "BitmapAccessor.h"
#include "ImageManipAddon.h"


const char addonName[] = "Make Gray";
const char addonInfo[] = "Make an image grayscale. By Edmund Vermeulen <edmundv@xs4all.nl>.";
const char addonCategory[] = "Color";
int32 addonVersion = 150;


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

	for (float y = sel_rect.top; y <= sel_rect.bottom; ++y)
	{
		int32 rowBytes;
		uchar *pixel = (uchar *) sourceBitmap->AccessBits(
			BRect(sel_rect.left, y, sel_rect.right, y), &rowBytes);

		for (float x = sel_rect.left; x <= sel_rect.right; ++x)
		{
			// Calculate gray
			uchar gray = uchar(
				float(pixel[0]) * 0.11 +
				float(pixel[1]) * 0.59 +
				float(pixel[2]) * 0.30);
			
			// Set all colour components to the same gray level
			pixel[2] = pixel[1] = pixel[0] = gray;
	
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
