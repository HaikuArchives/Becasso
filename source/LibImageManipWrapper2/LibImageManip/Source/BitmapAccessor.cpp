/*
 *  BitmapAccessor.cpp
 *  Release 1.0.0 (Nov 18th 1999)
 *
 *  Proxy class for accessing bitmaps through libimagemanip.so
 *
 *  Written by Edmund Vermeulen <edmundv@xs4all.nl>
 *  Public domain
 */


#define _BUILDING_imagemanip


#include "BitmapAccessor.h"


BitmapAccessor::~BitmapAccessor() {}


float
BitmapAccessor::BytesPerPixel()
{
	size_t pixelChunk;
	size_t rowAlignment;
	size_t pixelsPerChunk;
	if (get_pixel_size_for(ColorSpace(),
			&pixelChunk, &rowAlignment, &pixelsPerChunk) != B_OK)
	{
		return 0.0;
	}
	
	return (float) pixelChunk / (float) pixelsPerChunk;
}


void BitmapAccessor::_ReservedBitmapAccessor1() {}
void BitmapAccessor::_ReservedBitmapAccessor2() {}
void BitmapAccessor::_ReservedBitmapAccessor3() {}
