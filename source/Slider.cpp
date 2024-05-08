#include "Slider.h"
#include <Handler.h>
#include <InterfaceDefs.h>
#include <MessageFilter.h>
#include <TextControl.h>
#include <Window.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Colors.h"

#define KNOBMARGIN 4
#define KNOBVAL 16

class EnterFilter : public BMessageFilter {
public:
	EnterFilter(BHandler* handler);
	virtual filter_result Filter(BMessage* message, BHandler** target);

private:
	BHandler* fHandler;
};

EnterFilter::EnterFilter(BHandler* handler)
	: BMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN)
{
	fHandler = handler;
}

filter_result
EnterFilter::Filter(BMessage* message, BHandler** target)
{
	int32 raw_char;
	if (message->FindInt32("raw_char", &raw_char) == B_OK) {
		if (raw_char == B_ENTER || raw_char == B_TAB || raw_char == B_ESCAPE) {
			*target = fHandler;
		}
	}
	return B_DISPATCH_MESSAGE;
}

Slider::Slider(BRect p, float _sep, const char* _name, float _min, float _max, float _step,
	BMessage* _msg, orientation _posture, int _f, const char* _fmt)
	: BView(p, _name, B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)
{
	value = _min;
	min = _min;
	max = _max;
	step = _step;
	pos = _posture;
	msg = _msg;
	width = p.Width() - _sep;
	height = p.Height();
	bounds.top = 0;
	bounds.left = 0;
	bounds.right = width + 1;
	bounds.bottom = height;
	f = _f;
	strcpy(fmt, _fmt);
	strcpy(name, _name);
	sep = _sep;
	offslid = new BBitmap(bounds, B_COLOR_8_BIT, true);
	offview = new BView(bounds, "Offscreen Slider View", B_FOLLOW_NONE, (uint32)NULL);
	offslid->AddChild(offview);
	SetViewColor(B_TRANSPARENT_32_BIT);
	get_click_speed(&dcspeed);
	click = 1;
	tc = NULL;
	target = NULL;
}

Slider::~Slider()
{
	offslid->RemoveChild(offview);
	delete offview;
	delete offslid;
	delete msg;
}

void
Slider::AttachedToWindow()
{
	BString valMin, valMax;
	fNumberFormat.Format(valMin, (double)min);
	fNumberFormat.Format(valMax, (double)max);

	if (!f)
		knobsize = max_c(StringWidth(valMin), StringWidth(valMax)) + 2 * KNOBMARGIN;
	else
		knobsize = f;
	target = Parent();
}

void
Slider::SetTarget(BHandler* h)
{
	target = h;
}

// void Slider::ValueChanged (long newValue)
//{
// }

void
Slider::Draw(BRect rect)
{
	BRect label = BRect(0, 0, sep, height);
	if (rect.Intersects(label))	 // Label needs to be redrawn
	{
		SetLowColor(LightGrey);
		SetHighColor(Black);
		FillRect(label, B_SOLID_LOW);
		DrawString(name, BPoint(0, label.bottom - 5));
	}
	offslid->Lock();
	offview->SetHighColor(Grey21);
	offview->FillRect(bounds);
	offview->SetHighColor(Grey30);
	offview->StrokeLine(bounds.RightTop(), bounds.RightBottom());
	offview->StrokeLine(bounds.LeftBottom());
	offview->SetHighColor(Grey14);
	offview->StrokeLine(bounds.LeftTop());
	offview->StrokeLine(bounds.RightTop());
	knobpos = BPoint(float(value - min) / (max - min) * (width - knobsize), 1);
	knob = BRect(knobpos.x + 1, knobpos.y + 1, knobpos.x + knobsize - 2, knobpos.y + height - 2);
	offview->SetHighColor(Grey27);
	offview->FillRect(knob);
	offview->SetHighColor(Black);
	offview->SetLowColor(Grey27);
	offview->SetFont(be_plain_font);
	BString val = fmt;
	fNumberFormat.Format(val, value);
	offview->DrawString(
		val.String(), BPoint(knobpos.x + (knobsize - StringWidth(val)) / 2 + 1, knobpos.y + 12));
	offview->SetHighColor(Grey30);
	offview->StrokeLine(
		BPoint(knobpos.x + knobsize - 1, knobpos.y), BPoint(knobpos.x + 1, knobpos.y));
	offview->StrokeLine(BPoint(knobpos.x + 1, knobpos.y + height - 2));
	offview->SetHighColor(Grey13);
	offview->StrokeLine(BPoint(knobpos.x + knobsize - 1, knobpos.y + height - 2));
	offview->StrokeLine(BPoint(knobpos.x + knobsize - 1, knobpos.y));
	if (IsFocus()) {
		// printf ("%s focused!\n", Name());
		BRect k(knobpos.x, knobpos.y - 1, knobpos.x + knobsize - 1, knobpos.y + height - 2);
		k.InsetBy(1, 1);
		offview->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		offview->StrokeRect(k);
	}
	offview->Sync();
	offslid->Unlock();
	DrawBitmapAsync(offslid, BPoint(sep, 0));
}

void
Slider::MakeFocus(bool focused)
{
	inherited::MakeFocus(focused);
	Invalidate();
}

void
Slider::MouseDown(BPoint point)
// Note: Still assumes horizontal slider ATM!
{
	if (tc)	 // TextControl still visible...  Block other mouse movement.
		return;

	BPoint knobpos = BPoint(float(value - min) / (max - min) * (width - knobsize), 1);
	knob = BRect(knobpos.x + 1, knobpos.y + 1, knobpos.x + knobsize - 2, knobpos.y + height - 2);
	uint32 buttons;
	buttons = Window()->CurrentMessage()->FindInt32("buttons");
	float px = -1;
	bool dragging = false;
	if (click != 2 && buttons & B_PRIMARY_MOUSE_BUTTON && !(modifiers() & B_CONTROL_KEY)) {
		BPoint bp, pbp;
		uint32 bt;
		GetMouse(&pbp, &bt, true);
		bigtime_t start = system_time();
		if (knob.Contains(BPoint(pbp.x - sep, pbp.y))) {
			while (system_time() - start < dcspeed) {
				snooze(20000);
				GetMouse(&bp, &bt, true);
				if (!bt && click != 2) {
					click = 0;
				}
				if (bt && !click) {
					click = 2;
				}
				if (bp != pbp)
					break;
			}
		}
		if (click != 2) {
			// Now we're dragging...
			while (buttons) {
				BPoint p = BPoint(point.x - sep, point.y);
				float x = p.x;
				if (!(knob.Contains(p)) && !dragging) {
					if (x > knob.left) {
						value += step * (x - knob.right) / 10;
					} else {
						value += step * (x - knob.left) / 10;
					}
					if (value < min)
						value = min;
					if (value > max)
						value = max;
					if (step > 1 && fmod(value - min, step))  // Hack hack!
						value = int((value - min) / step) * step + min;
					//			if (fmt[strlen (fmt) - 2] == '0')
					//				value = int (value + 0.5);
					offslid->Lock();
					Invalidate(BRect(sep + 1, 0, offslid->Bounds().Width() + sep, height));
					offslid->Unlock();
					NotifyTarget();
				} else if (px != p.x && step <= 1)	// Hacks galore!
				{
					dragging = true;
					value = (x - knobsize / 2) / (width - knobsize) * (max - min) + min;
					// printf ("x = %f, knobsize = %f, value = %f\n", x, knobsize, value);
					// printf ("Value: %f ", value);
					if (value < min)
						value = min;
					if (value > max)
						value = max;
					if (step > 1 && fmod(value - min, step))
						value = int((value - min) / step) * step + min;
					if (fmt[strlen(fmt) - 2] == '0')
						value = int(value + 0.5);
					// printf ("-> %f\n", value);
					offslid->Lock();
					Invalidate(BRect(sep + 1, 0, offslid->Bounds().Width() + sep, height));
					offslid->Unlock();
					px = p.x;
					NotifyTarget();
				}
				knobpos = BPoint(float(value - min) / (max - min) * (width - knobsize), 1);
				knob = BRect(
					knobpos.x + 1, knobpos.y + 1, knobpos.x + knobsize - 2, knobpos.y + height - 2);
				snooze(20000);
				GetMouse(&point, &buttons, true);
			}
			click = 1;
		}
	}
	if (click == 2 || buttons & B_SECONDARY_MOUSE_BUTTON || modifiers() & B_CONTROL_KEY) {
		click = 1;
		if (tc) {
			RemoveChild(tc);
			delete tc;
		}
		knobpos = BPoint(float(value - min) / (max - min) * (width - knobsize), 1);
		BRect kbr = BRect(
			knobpos.x + sep, knobpos.y, knobpos.x + knobsize - 2 + sep, knobpos.y + height - 3);
		//		kbr.PrintToStream();
		tc = new BTextControl(kbr, "slider value field", "", "", new BMessage('tcVC'));
		tc->SetTarget(this);
		tc->SetDivider(0);
		EnterFilter* filter = new EnterFilter(this);
		tc->TextView()->AddFilter(filter);
		BString vs = fmt;
		fNumberFormat.Format(vs, value);
		tc->SetText(vs);
		AddChild(tc);
		tc->MakeFocus(true);
	}
	NotifyTarget();
}

void
Slider::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'tcVC':
		{
			float tv = 0;
			if (tc) {
				tv = atof(tc->Text());
				if (tv < min)
					tv = min;
				if (tv > max)
					tv = max;
				if (step > 1 && fmod(tv - min, step))
					tv = int((tv - min) / step) * step + min;
				if (fmt[strlen(fmt) - 2] == '0')
					tv = int(tv + 0.5);
				RemoveChild(tc);
			}
			delete tc;
			tc = NULL;
			SetValue(tv);
			NotifyTarget();
			break;
		}
		default:
			// message->PrintToStream();
			inherited::MessageReceived(message);
	}
}

void
Slider::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch (*bytes) {
			case B_ESCAPE:
			{
				if (tc) {
					// printf ("TextControl is open\n");
					RemoveChild(tc);
					delete (tc);
					tc = NULL;
				}
				break;
			}
			case B_SPACE:
			case B_ENTER:
			{
				// printf ("Enter\n");
				if (tc) {
					// printf ("TextControl is open\n");
					BMessage* key = new BMessage('tcVC');
					MessageReceived(key);
					delete key;
				} else {
					knobpos = BPoint(float(value - min) / (max - min) * (width - knobsize), 1);
					BRect kbr = BRect(knobpos.x + sep, knobpos.y, knobpos.x + knobsize - 2 + sep,
						knobpos.y + height - 3);
					//		kbr.PrintToStream();
					tc = new BTextControl(kbr, "slider value field", "", "", new BMessage('tcVC'));
					tc->SetTarget(this);
					tc->SetDivider(0);
					EnterFilter* filter = new EnterFilter(this);
					tc->TextView()->AddFilter(filter);
					BString vs = fmt;
					fNumberFormat.Format(vs, value);
					tc->SetText(vs.String());
					AddChild(tc);
					tc->MakeFocus(true);
					inherited::KeyDown(bytes, numBytes);
				}
				break;
			}
			case B_LEFT_ARROW:
				// printf ("Left\n");
				if (value > min) {
					value -= step;
					Invalidate();
					NotifyTarget();
				}
				break;
			case B_RIGHT_ARROW:
				// printf ("Right\n");
				if (value < max) {
					value += step;
					Invalidate();
					NotifyTarget();
				}
				break;
			case B_TAB:
				// printf ("Tab\n");
				if (tc) {
					// printf ("TextControl is open\n");
					BMessage* key = new BMessage('tcVC');
					MessageReceived(key);
					delete key;
				} else {
					// MakeFocus (false);
					inherited::KeyDown(bytes, numBytes);
					Invalidate();
					break;
				}
			default:
				inherited::KeyDown(bytes, numBytes);
		}
	} else
		inherited::KeyDown(bytes, numBytes);
}

float
Slider::Value()
{
	return (value);
}

void
Slider::SetValue(float _v)
{
	value = _v;
	Invalidate();
}

void
Slider::NotifyTarget()
{
	if (!target) {
		fprintf(stderr, "Slider: Null target\n");
		return;
	}
	BMessage* changed = new BMessage(*msg);
	changed->AddFloat("value", value);
	//	printf ("%f\n", value);
	target->LockLooper();
	target->MessageReceived(changed);
	target->UnlockLooper();
	delete changed;
}
