#include "BitmapView.h"
#include <stdio.h>
#include "Colors.h"

BitmapView::BitmapView(
	BRect bounds, const char* name, BBitmap* map, drawing_mode mode, bool center, bool own)
	: BView(bounds, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	if (own)
		fBitmap = map;
	else
		fBitmap = new BBitmap(map);
	SetViewColor(White);
	SetDrawingMode(mode);
	fCenter = center;
	fPosition = B_ORIGIN;
}

BitmapView::BitmapView(BRect bounds, const char* name)
	: BView(bounds, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fBitmap = NULL;
	SetViewColor(White);
	fCenter = true;
}

BitmapView::~BitmapView()
{
	delete fBitmap;
}

void
BitmapView::SetPosition(BPoint point)
{
	fPosition = point;
	fCenter = false;
}

void
BitmapView::SetBitmap(BBitmap* bitmap)
{
	delete fBitmap;
	fBitmap = new BBitmap(bitmap);
}

void
BitmapView::Draw(BRect /*update*/)
{
	if (fBitmap) {
		if (fCenter) {
			BPoint place = BPoint((Bounds().Width() - fBitmap->Bounds().Width()) / 2,
				(Bounds().Height() - fBitmap->Bounds().Height()) / 2);
			DrawBitmap(fBitmap, place);
		} else
			DrawBitmap(fBitmap, fPosition);
	}
}