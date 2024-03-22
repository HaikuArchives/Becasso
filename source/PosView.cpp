#include "PosView.h"
#include "CanvasWindow.h"

PosView::PosView(const BRect frame, const char* name, CanvasView* _canvas)
	: BView(frame, name, B_FOLLOW_BOTTOM, B_WILL_DRAW | B_PULSE_NEEDED)
{
	SetViewColor(LightGrey);
	SetFont(be_plain_font);
	mouse_x = 0;
	mouse_y = 0;
	delta_x = 0;
	delta_y = 0;
	set_x = -1;
	set_y = -1;
	radius = 0;
	do_radius = false;
	canvas = _canvas;
}

void
PosView::Pulse()
{
	uint32 buttons;
	BPoint pos;
	GetMouse(&pos, &buttons, false);
	pos = canvas->ConvertFromScreen(ConvertToScreen(pos));
	if (pos != prev) {
		mouse_x = int(pos.x / canvas->getScale());
		mouse_y = int(pos.y / canvas->getScale());
		Draw(Bounds());
		prev = pos;
	}
}

void
PosView::SetPoint(const int x, const int y)
{
	set_x = x;
	set_y = y;
	if (x == y == -1)
		DoRadius(false);
	Invalidate();
}

void
PosView::DoRadius(const bool r)
{
	do_radius = r;
}

void
PosView::SetTextLayer(const bool t)
{
	is_textlayer = t;
}

void
PosView::Draw(BRect /* updaterect */)
{
	SetLowColor(LightGrey);
	SetHighColor(DarkGrey);
	FillRect(Bounds(), B_SOLID_LOW);
	StrokeRect(Bounds());
	SetHighColor(Grey31);
	StrokeLine(Bounds().LeftTop() + BPoint(1, 1), Bounds().RightTop() + BPoint(-1, 1));
	if (canvas->Bounds().Contains(BPoint(mouse_x * canvas->getScale(), mouse_y * canvas->getScale())
		) &&
		(Window()->IsActive()))
		SetHighColor(Black);
	else
		SetHighColor(DarkGrey);
	SetFont(be_plain_font);
	SetFontSize(9);
	char s[32];
	if (set_x >= 0) {
		int delta_x = mouse_x - set_x;
		int delta_y = mouse_y - set_y;
		if (do_radius)
			sprintf(
				s, "(%i,%i)∆(%s%i,%s%i) %.1f", mouse_x, mouse_y, delta_x > 0 ? "+" : "", delta_x,
				delta_y > 0 ? "+" : "", delta_y, sqrt(delta_x * delta_x + delta_y * delta_y)
			);
		else
			sprintf(
				s, "(%i,%i) ∆(%s%i,%s%i)", mouse_x, mouse_y, delta_x > 0 ? "+" : "", delta_x,
				delta_y > 0 ? "+" : "", delta_y
			);
	}
	else
		sprintf(s, "(%i, %i)", mouse_x, mouse_y);
	DrawString(s, BPoint((POSWIDTH - StringWidth(s)) / 2, 11));
	//	if (is_textlayer)
	//	{
	//		SetFontSize (12);
	//		DrawString ("T", BPoint (2, 12));
	//	}
}