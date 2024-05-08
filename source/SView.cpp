#include "SView.h"
#include <ClassInfo.h>
#include <Message.h>
#include <SupportDefs.h>
#include <WindowScreen.h>
#include <stdio.h>

SView::SView(BRect frame, const char* name, uint32 resizeMask, uint32 flags)
	: BView(frame, name, resizeMask, flags)
{
	fScale = 1;
	fPressure = 255;
	fDevice = DEV_NONE;
	fSnoozeTime = 20000;
}

SView::~SView() {}

void
SView::GetMouse(BPoint* pos, uint32* buttons, bool checkqueue)
{
	// printf ("SView::GetMouse\n");
	inherited::GetMouse(pos, buttons, checkqueue);
	(*pos).x = int((*pos).x / fScale + 0.5);
	(*pos).y = int((*pos).y / fScale + 0.5);
}

void
SView::GetPosition(Position* position, bool setcursor)
{
	// clock_t start, end;
	// start = clock();
	BPoint point;
	uint32 buttons;
	GetMouse(&point, &buttons, true);
	BMessage* currentMessage = Window()->CurrentMessage();
	extern Tablet* wacom;
	if (wacom->IsValid() && wacom->Update() == B_OK) {
		if (wacom->Proximity()) {
			fDevice = DEV_TABLET;
			position->fPoint = wacom->Point();
			position->fTilt = wacom->Tilt();
			position->fPressure = uint8(255 * wacom->Pressure());
			position->fProximity = wacom->Proximity();
			if (setcursor) {
				BPoint scrpoint = ConvertToScreen(position->fPoint);
				set_mouse_position(int32(scrpoint.x), int32(scrpoint.y));
			}
			// printf ("%d\n", wacom->Buttons());
			switch (wacom->Buttons()) {
				case 0:	 // Actually, only proximity here...
					position->fButtons = 0;
					break;
				case 1:
				case 2:	 // Stylus point
					position->fButtons = B_PRIMARY_MOUSE_BUTTON;
					break;
				case 3:	 // Side switch
					position->fButtons = B_SECONDARY_MOUSE_BUTTON;
					break;
				case 4:
					position->fButtons = 0;
					break;
				case 5:	 // "Eraser"
					position->fButtons = B_TERTIARY_MOUSE_BUTTON;
					break;
				default:
					fprintf(stderr, "Unsupported button combination\n");
			}
			// end = clock();
			// printf ("Took %d ms\n", (end - start));
			return;
		}
	}
	// Else (i.e. no tablet or no proximity)
	if (fDevice == DEV_TABLET) {
		position->fButtons = 0;
		position->fProximity = false;
		fDevice = DEV_MOUSE;
		return;
	}
	fDevice = DEV_MOUSE;
	position->fPoint = point;
	float tx, ty;
	if (currentMessage->FindFloat("be:tilt_x", &tx) != B_OK ||
		currentMessage->FindFloat("be:tilt_y", &ty) != B_OK)
		position->fTilt = BPoint(0, 0);
	else
		position->fTilt = BPoint(tx, ty);
	float pres;
	if (currentMessage->FindFloat("be:tablet_pressure", &pres) == B_OK)
		fPressure = uint8(255 * pres);
	else
		fPressure = 255;
	position->fPressure = fPressure;
	//	printf ("Pressure: %d\n", fPressure);
	position->fButtons = buttons;
	position->fProximity = false;
}

inline void
SView::StrokeRect(BRect r, pattern p)
{
	inherited::StrokeRect(makePositive(r), p);
}

inline void
SView::FillRect(BRect r, pattern p)
{
	inherited::FillRect(makePositive(r), p);
}

inline void
SView::InvertRect(BRect r)
{
	inherited::InvertRect(makePositive(r));
}

inline void
SView::StrokeRoundRect(BRect r, float xRadius, float yRadius, pattern p)
{
	inherited::StrokeRoundRect(makePositive(r), xRadius, yRadius, p);
}

inline void
SView::FillRoundRect(BRect r, float xRadius, float yRadius, pattern p)
{
	inherited::FillRoundRect(makePositive(r), xRadius, yRadius, p);
}

inline void
SView::StrokeEllipse(BRect r, pattern p)
{
	inherited::StrokeEllipse(makePositive(r), p);
}

inline void
SView::StrokeEllipse(BPoint center, float xRadius, float yRadius, pattern p)
{
	inherited::StrokeEllipse(center, xRadius, yRadius, p);
}

inline void
SView::FillEllipse(BRect r, pattern p)
{
	inherited::FillEllipse(makePositive(r), p);
}

inline void
SView::FillEllipse(BPoint center, float xRadius, float yRadius, pattern p)
{
	inherited::FillEllipse(center, xRadius, yRadius, p);
}

inline void
SView::SStrokeRect(BRect r, pattern p)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	inherited::StrokeRect(makePositive(r), p);
}

inline void
SView::SFillRect(BRect r, pattern p)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	inherited::FillRect(makePositive(r), p);
}

inline void
SView::SInvertRect(BRect r)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	inherited::InvertRect(makePositive(r));
}

inline void
SView::SStrokeRoundRect(BRect r, float xRadius, float yRadius, pattern p)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	xRadius *= fScale;
	yRadius *= fScale;
	inherited::StrokeRoundRect(makePositive(r), xRadius, yRadius, p);
}

inline void
SView::SFillRoundRect(BRect r, float xRadius, float yRadius, pattern p)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	xRadius *= fScale;
	yRadius *= fScale;
	inherited::FillRoundRect(makePositive(r), xRadius, yRadius, p);
}

inline void
SView::SStrokeEllipse(BRect r, pattern p)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	inherited::StrokeEllipse(makePositive(r), p);
}

inline void
SView::SStrokeEllipse(BPoint center, float xRadius, float yRadius, pattern p)
{
	center.x *= fScale;
	center.y *= fScale;
	xRadius *= fScale;
	yRadius *= fScale;
	inherited::StrokeEllipse(center, xRadius, yRadius, p);
}

inline void
SView::SFillEllipse(BRect r, pattern p)
{
	r.left *= fScale;
	r.top *= fScale;
	r.right *= fScale;
	r.bottom *= fScale;
	inherited::FillEllipse(makePositive(r), p);
}

inline void
SView::SFillEllipse(BPoint center, float xRadius, float yRadius, pattern p)
{
	center.x *= fScale;
	center.y *= fScale;
	xRadius *= fScale;
	yRadius *= fScale;
	inherited::FillEllipse(center, xRadius, yRadius, p);
}

BRect
SView::makePositive(BRect r)
{
	BRect res;
	res.left = min_c(r.left, r.right);
	res.right = max_c(r.left, r.right);
	res.top = min_c(r.top, r.bottom);
	res.bottom = max_c(r.top, r.bottom);
	return res;
}