#include "Selection.h"
#include <stdlib.h>
#include <string.h>
#include "DModes.h"

Selection::Selection(BRect bounds) : BBitmap(bounds, B_BITMAP_ACCEPTS_VIEWS, B_GRAYSCALE_8_BIT)
{
	fRect = bounds;
	fMode = DM_BLEND;
	bzero(Bits(), BitsLength());
}

Selection::Selection(const Selection& selection)
	: BBitmap(selection.fRect, B_BITMAP_ACCEPTS_VIEWS, B_GRAYSCALE_8_BIT)
{
	fRect = selection.fRect;
	fMode = selection.fMode;
	bzero(Bits(), BitsLength());
}

Selection::~Selection() {}

void
Selection::ClearTo(grey_pixel p)
{
	memset(Bits(), p, BitsLength());
}
