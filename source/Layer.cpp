#include "Layer.h"
#include "DModes.h"
#include "BecassoAddOn.h"
#include <string.h>
#include <stdlib.h>

Layer::Layer (BRect bounds, const char *name)
: BBitmap (bounds, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32)
{
	fRect = bounds;
	fMode = DM_BLEND;
	fHide = false;
	strcpy (fName, name);
	fGlobalAlpha = 255;
	fAlphaMap = NULL;
	bzero (Bits(), BitsLength());
//	uint32 *bits = ((uint32 *) Bits()) - 1;
//	for (uint32 i = 0; i < BitsLength()/4; i++)
//		*(++bits) = COLOR_MASK;
}

Layer::Layer (const Layer& layer)
: BBitmap (layer.fRect, B_BITMAP_ACCEPTS_VIEWS, B_RGBA32)
{
	fRect = layer.fRect;
	fMode = layer.fMode;
	fHide = layer.fHide;
	strcpy (fName, layer.fName);
	fGlobalAlpha = layer.fGlobalAlpha;
	fAlphaMap = NULL;
	bzero (Bits(), BitsLength());
//	uint32 *bits = ((uint32 *) Bits()) - 1;
//	for (uint32 i = 0; i < BitsLength()/4; i++)
//		*(++bits) = COLOR_MASK;
}

Layer::~Layer ()
{
}

void Layer::setName (const char *name)
{
	strcpy (fName, name);
}

void Layer::ClearTo (bgra_pixel p)
{
	bgra_pixel *d = (bgra_pixel *) Bits() - 1;
	for (int32 i = 0; i < BitsLength()/4; i++)
		*(++d) = p;
}
