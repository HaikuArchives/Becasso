// SBitmap: BBitmap replacement which doesn't clear on ctor

#include "SBitmap.h"
#include <stdlib.h>
#include <string.h>
#include <Debug.h>

SBitmap::SBitmap (BBitmap *src)
{
	fBounds = src->Bounds();
	switch (src->ColorSpace())
	{
		case B_COLOR_8_BIT:
		case B_GRAYSCALE_8_BIT:
			fBPP = 1;
			break;
		default:
			fBPP = 4;
			break;
	}
	fBPR = fBPP * (fBounds.IntegerWidth() + 1);
	if (fBPR % 4)	// padding required
	{
		uint32 pad = 4 - ((uint32) fBPR % 4);
		fBPR += pad;
	}


	int32 bitslength = fBPR*(fBounds.IntegerHeight() + 1);
	
	ASSERT_WITH_MESSAGE (bitslength == src->BitsLength(), "BitsLength() mismatch");
		
	fBits = malloc (bitslength);

	memcpy (fBits, src->Bits(), src->BitsLength());
}

SBitmap::SBitmap (const BRect bounds, const color_space cs)
: fBounds (bounds), fCS (cs)
{
	switch (cs)
	{
		case B_COLOR_8_BIT:
		case B_GRAYSCALE_8_BIT:
			fBPP = 1;
			break;
		default:
			fBPP = 4;
			break;
	}
	fBPR = fBPP * (fBounds.IntegerWidth() + 1);
	if (fBPR % 4)	// padding required
	{
		uint32 pad = 4 - ((uint32) fBPR % 4);
		fBPR += pad;
	}
	
	int32 bitslength = fBPR*(fBounds.IntegerHeight() + 1);
	fBits = malloc (bitslength);
}

SBitmap::~SBitmap ()
{
	free (fBits);
}

int32 SBitmap::BytesPerRow () const
{
	return fBPR;
}

int32 SBitmap::BytesPerPixel () const
{
	return fBPP;
}

int32 SBitmap::BitsLength () const
{
	return fBPR*(fBounds.IntegerHeight() + 1);
}

void *SBitmap::Bits ()
{
	return fBits;
}

BRect SBitmap::Bounds () const
{
	return fBounds;
}

color_space SBitmap::ColorSpace () const
{
	return fCS;
}
