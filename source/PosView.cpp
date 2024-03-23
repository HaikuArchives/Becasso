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
	BString deltaXData;
	BString deltaYData;
	BString mouseXData;
	BString mouseYData;
	BString radiusValue;
	BString plusSign = BString("+");
	BString positionString;
	fNumberFormat.Format(mouseXData, mouse_x);
	fNumberFormat.Format(mouseYData, mouse_y);
	if (set_x >= 0) {
		int delta_x = mouse_x - set_x;
		int delta_y = mouse_y - set_y;
		fNumberFormat.Format(deltaXData, delta_x);
		fNumberFormat.Format(deltaYData, delta_y);
		fNumberFormat.SetPrecision(1);
		fNumberFormat.Format(radiusValue, sqrt(delta_x * delta_x + delta_y * delta_y));
		if (do_radius) {
			positionString.SetToFormat(
				"(%s, %s) ∆ (%s%s, %s%s) %s", mouseXData.String(), mouseYData.String(),
				(delta_x > 0 ? plusSign.String() : ""), deltaXData.String(),
				(delta_y > 0 ? plusSign.String() : ""), deltaYData.String(), radiusValue.String()
			);
		} else {
			positionString.SetToFormat(
				"(%s, %s) ∆ (%s%s, %s%s)", mouseXData.String(), mouseYData.String(),
				(delta_x > 0 ? plusSign.String() : ""), deltaXData.String(),
				(delta_y > 0 ? plusSign.String() : ""), deltaYData.String()
			);
		}
	} else {
		positionString.SetToFormat("(%s, %s)", mouseXData.String(), mouseYData.String());
	}
	DrawString(
		positionString.String(), BPoint((POSWIDTH - StringWidth(positionString.String())) / 2, 11)
	);

	//	if (is_textlayer)
	//	{
	//		SetFontSize (12);
	//		DrawString ("T", BPoint (2, 12));
	//	}
}
