#include "Dial.h"
#include <string.h>
#include <MessageFilter.h>
#include "Colors.h"

public:
EnterFilter(BHandler* handler);
virtual filter_result
Filter(BMessage* message, BHandler** target);

private:
BHandler* fHandler;
}
;

EnterFilter::EnterFilter(BHandler* handler)
	: BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fHandler = handler;
}

filter_result
EnterFilter::Filter(BMessage* message, BHandler** target)
{
	// message->PrintToStream();
	// printf ("to: %s\n", (*target)->Name());
	int32 raw_char;
	if (message->FindInt32("raw_char", &raw_char) == B_OK) {
		if (raw_char == B_ENTER || raw_char == B_TAB) {
			// printf ("ENTER or TAB\n");
			*target = fHandler;
		}
	}
	return B_DISPATCH_MESSAGE;
}

Dial::Dial(BRect frame, const char* name, int type, BMessage* msg)
{
	get_click_speed(&dcspeed);
	click = 1;
	tc = NULL;
	strcpy(fName, name);
	fType = type;
}

Dial::~Dial() { delete msg; }

void
Dial::MouseDown(BPoint point)
{
}

void
Dial::Draw(BRect update)
{
	SetLowColor(LightGrey);
	SetHighColor(DarkGrey);
	FillRect(Bounds(), B_SOLID_LOW);
	if (fType == DIAL_360) {
		StrokeEllipse(bounds)
	} else {
	}
}

void
Dial::AttachedToWindow()
{
	inherited::AttachedToWindow();
}

void
Dial::MessageReceived(BMessage* msg)
{
}

void
Dial::KeyDown(const char* bytes, int32 numBytes)
{
}

void
Dial::MakeFocus(bool focused)
{
	inherited::MakeFocus(focused);
	Invalidate();
}

float
Dial::Value()
{
	return fValue;
}

void
Dial::SetValue(float _v)
{
	fValue = _v;
	Invalidate();
}

void
Dial::NotifyTarget()
{
	BMessage* changed = new BMessage(fMsg);
	changed->AddFloat("value", value);
	// printf ("%f\n", value);
	Window()->Lock();
	Parent()->MessageReceived(changed);
	Window()->Unlock();
	delete changed;
}