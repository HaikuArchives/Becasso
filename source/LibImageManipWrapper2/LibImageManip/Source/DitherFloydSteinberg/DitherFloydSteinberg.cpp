/*
 *  DitherFloydSteinberg.cpp 1.5.0 (Oct 24th, 1999)
 *
 *  Image converter add-on that takes a 32-bit bitmap and
 *  converts it to 8-bit using Floyd-Steinberg dithering
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 */


#include <InterfaceDefs.h>
#include "BitmapAccessor.h"
#include "ImageManipAddon.h"


const char addonName[] = "Dither Floyd-Steinberg";
const char addonInfo[] = "Dither a 32-bit bitmap to 8-bit. By Edmund Vermeulen <edmundv@xs4all.nl>.";
const char addonCategory[] = "Color";
int32 addonVersion = 150;


status_t
Convert(
	BitmapAccessor *sourceBitmap,
	BitmapAccessor *destBitmap,
	BMessage * /*ioExtension*/,
	bool checkOnly)
{
	// We only support 32-bit bitmaps
	color_space cs = sourceBitmap->ColorSpace();
	if (cs != B_RGB32 && cs != B_RGBA32)
		return B_NOT_ALLOWED;

	if (checkOnly)
		return B_OK;

	// Get source pixel size;
	// we only use the first three bytes, and skip the rest
	int source_pixel_size = (int) sourceBitmap->BytesPerPixel();
	if (source_pixel_size < 3)
		return B_ERROR;

	BRect source_bounds(sourceBitmap->Bounds());
	float width = source_bounds.Width();
	float height = source_bounds.Height();

	// Create the 8-bit bitmap
	if (!destBitmap->CreateBitmap(BRect(0.0, 0.0, width, height), B_CMAP8))
		return B_ERROR;

	// Get destination pixel size;
	// we only use the first byte, and skip the rest
	int dest_pixel_size = (int) destBitmap->BytesPerPixel();
	if (dest_pixel_size < 1)
		return B_ERROR;

	// Get colour palette for 8-bit screens
	const rgb_color *palette = system_colors()->color_list;
	const uchar *index_list = system_colors()->index_map;

	// Create the delta buffer
	int32 size = 3 * (int(destBitmap->Bounds().Width()) + 3);
	float *delta_buffer = new float[size];
	for (int32 i = 0; i < size; ++i)
		delta_buffer[i] = 0.0;

	// Work our way through the data row by row
	for (float y = 0.0; y <= height; ++y)
	{
		int32 source_row_bytes;
		uchar *src = static_cast<uchar *>(sourceBitmap->AccessBits(
			BRect(
				source_bounds.left,
				source_bounds.top + y,
				source_bounds.right,
				source_bounds.top + y),
			&source_row_bytes));

		sourceBitmap->BitsNotChanged();  // read-only

		int32 dest_row_bytes;
		uchar *in_bitmap = static_cast<uchar *>(destBitmap->AccessBits(
			BRect(0.0, y, width, y), &dest_row_bytes));

		float *deltas = delta_buffer;

		float r_delta = 0.0;
		float g_delta = 0.0;
		float b_delta = 0.0;

		// Do all pixels on one row
		for (float x = 0.0; x <= width; ++x)
		{
			// Get a pixel from the input buffer and add corrections
			// from the pixels around it through Floyd-Steinberg diffusion
			int32 r = int32(src[2]) + int32(r_delta * 0.4375 + deltas[0] * 0.0625 + deltas[3] * 0.3125 + deltas[6] * 0.1875);
			int32 g = int32(src[1]) + int32(g_delta * 0.4375 + deltas[1] * 0.0625 + deltas[4] * 0.3125 + deltas[7] * 0.1875);
			int32 b = int32(src[0]) + int32(b_delta * 0.4375 + deltas[2] * 0.0625 + deltas[5] * 0.3125 + deltas[8] * 0.1875);

			// Store deltas for the current pixel
			deltas[0] = r_delta;
			deltas[1] = g_delta;
			deltas[2] = b_delta;
			deltas += 3;

			// Fix high and low values
			r = r > 255 ? 255 : r;
			r = r <   0 ?   0 : r;
			g = g > 255 ? 255 : g;
			g = g <   0 ?   0 : g;
			b = b > 255 ? 255 : b;
			b = b <   0 ?   0 : b;

			// Let's see which colour we get from the BeOS palette
			uchar colour = index_list[((r & 0xf8) << 7) | ((g & 0xf8) << 2) | ((b & 0xf8) >> 3)];

			// Output the pixel
			*in_bitmap = colour;
			in_bitmap += dest_pixel_size;

			// Remember the differences with the requested color
			r_delta = r - palette[colour].red;
			g_delta = g - palette[colour].green;
			b_delta = b - palette[colour].blue;

			src += source_pixel_size;
		}
	}
	
	delete[] delta_buffer;

	return B_OK;
}
