#include "HelpView.h"
#include <string.h>

HelpView::HelpView(const BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_LEFT, B_WILL_DRAW)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetFont(be_plain_font);
	SetFontSize(10);
}

void
HelpView::Draw(BRect /* updaterect */)
{
	//	printf ("HelpView::Draw\n");
	SetLowColor(LightGrey);
	SetHighColor(DarkGrey);
	FillRect(Bounds(), B_SOLID_LOW);
	StrokeLine(Bounds().LeftBottom(), Bounds().RightBottom());
	SetHighColor(Grey30);
	StrokeLine(Bounds().LeftBottom() + BPoint(0, -2), Bounds().LeftTop());
	StrokeLine(Bounds().RightTop());
	SetHighColor(Grey24);
	StrokeLine(Bounds().LeftBottom() + BPoint(0, -1), Bounds().RightBottom() + BPoint(0, -1));
	SetHighColor(Grey10);
	DrawString(text, BPoint(Bounds().Width() - StringWidth(text) - 4, 13));
}

void
HelpView::setText(const char* t)
{
	strcpy(text, t);
	Invalidate();
}
