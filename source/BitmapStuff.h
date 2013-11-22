#ifndef BITMAPSTUFF_H
#define BITMAPSTUFF_H

#include "Brush.h"
#include "AddOnSupport.h"
#include "Layer.h"
#include "Selection.h"
#include <Bitmap.h>

class SBitmap;

BBitmap *entry2bitmap (BEntry entry, bool silent/* = false*/);
float AverageAlpha (BBitmap *src);
void InvertAlpha (BBitmap *src);
void InsertGlobalAlpha (Layer *layers[], int numLayers);
void InvertWithInverseAlpha (BBitmap *src, BBitmap *selection);
void AddToSelection (Brush *src, BBitmap *dest, long x, long y, int strength = 255);
void CopyWithTransparency (BBitmap *src, BBitmap *dest, long x, long y);
void FastCopy (BBitmap *src, BBitmap *dest);
void FSDither (BBitmap *b32, BBitmap *b8, BRect place);
BRect GetSmallestRect (BBitmap *b);
int32 gcd (int32 p, int32 q);
void Rotate (SBitmap *s, Layer *d, BPoint origin, float rad, bool hiq);
void Rotate (SBitmap *s, Selection *d, BPoint origin, float rad, bool hiq);
void SBitmapToLayer (SBitmap *s, Layer *d, BPoint &p);
void SBitmapToLayer (SBitmap *s, Layer *d, BRect &r);
void SBitmapToSelection (SBitmap *s, Selection *d, BPoint &p);
void SBitmapToSelection (SBitmap *s, Selection *d, BRect &r);


/*
#pragma export on
int Scale (BBitmap *src, BBitmap *srcmap, BBitmap *dest, BBitmap *destmap);
void CutOrCopy (BBitmap *src, BBitmap *dest, BBitmap *selection, long offx, long offy, bool cut);
void AddWithAlpha (BBitmap *src, BBitmap *dest, long x, long y);
void BlendWithAlpha (BBitmap *src, BBitmap *dest, long x, long y, int strength = 255);
#pragma export reset
*/

#endif 